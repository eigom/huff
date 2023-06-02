/*
 Main module for huffman (de)compression
 Eigo Madaloja
*/

#include "huffman.h"

static encoding* encodings[256];
static table decode_table;
static uint32 codes_start[33];   /* 0 to 32 */
static int lengths_count[33];

static char* errors[] = {
   "error allocating memory"
};

/*
 Exit with simple error report
*/
void fatal(int code)
{
   perror(errors[code]);
   exit(code+1);
}

/*
 Exit with an extended error report
 should be called with __FILE__ and __LINE__
*/
void fatale(int code, char* file, int line)
{
   fprintf(stderr, "%s: near line %d: ", file, line);
   perror(errors[code]);
   exit(code+1);
}

/*
 Collect the char distributions for file 'fd'. Data will
 be read in chunks size of which is specified by
 'block_size'. This should be set to the disk's file block
 size for maximum performance (stat.st_blksize).
*/
uint* collect_dists(int fd, size_t block_size)
{
   uint* dists;
   byte* buffer;
   ssize_t nread;
   int i;

   lseek(fd, 0, SEEK_SET);

   if((dists = (uint*)calloc(256,sizeof(int))) == NULL)
      fatal(OUT_OF_MEM);

   if((buffer = (byte*)malloc(block_size)) == NULL)
      fatal(OUT_OF_MEM);

   while(i=0,
        (nread = read(fd, buffer, block_size)) > 0)   /* read byte chunks */
      while((ssize_t)i<nread)
         (*(dists+(int)*(buffer+i++)))++;                  /* count rates */
                                                /* dists[buffer[i++]]++; */
   lseek(fd, 0, SEEK_SET);
   free(buffer);

   return dists;
}

/*
 Create nodes from the distribution array
*/
node** make_nodes(int* dist_lst, int* node_count)
{
   node** node_lst;
   int i;

   *node_count = 0;

   if((node_lst =
      (node**)malloc(256*sizeof(node*))) == NULL)  /* node pointers */
         fatal(OUT_OF_MEM);

   for(i=0; i<256; i++)
   {
      if(dist_lst[i])
      {
         if((node_lst[*node_count] =
            (node*)malloc(sizeof(node))) == NULL)
               fatal(OUT_OF_MEM);
         node_lst[*node_count]->symbol = i;
         node_lst[*node_count]->dist = dist_lst[i];
         node_lst[*node_count]->left = NULL;
         node_lst[*node_count]->right = NULL;
         (*node_count)++;
      }
   }
   return node_lst;
}

/*
 Compare distributions for ascending order.
*/
int node_cmp_dist(const void* n1, const void* n2)
{
   return (*(node**)n1)->dist - (*(node**)n2)->dist;
}

/*
 Create a tree from array of pointers to nodes
*/
node* make_tree(node** node_lst, int len)
{
   node* n;
   node* tmp;
   int j, i=0;

   /* sort the nodes by distribution */
   qsort(node_lst, (size_t)len, sizeof(node*), node_cmp_dist);

   while(i<len-1)
   {
      if((n = (node*)malloc(sizeof(node))) == NULL)
         fatal(OUT_OF_MEM);

      /* combine a new node */
      n->left = node_lst[i];
      n->right = node_lst[i+1];
      n->dist = n->left->dist + n->right->dist;
      n->symbol = 256;  /* fictive */
      node_lst[++i] = n;
      j = i;

      while(j<len-1 && node_lst[j]->dist > node_lst[j+1]->dist)  /* sort */
      {
         tmp = node_lst[j+1];             /* swap */
         node_lst[j+1] = node_lst[j];
         node_lst[j] = tmp;
         j++;
      }
   }
   return node_lst[i];
}

/*
 The code lengths
*/
int make_lengths(node* n)
{
   memset(encodings, 0, sizeof(encoding*)*256);

   if(n->left == NULL && n->right == NULL)   /* the only node */
   {
      if((encodings[n->symbol] =
         (encoding*)malloc(sizeof(encoding))) == NULL)
            fatal(OUT_OF_MEM);
      encodings[n->symbol]->symbol = n->symbol;
      encodings[n->symbol]->dist = n->dist;
      encodings[n->symbol]->code = (uint32)0;
      encodings[n->symbol]->length = 1;
      return 1;
   }

   return make_lengths_traverse(n, 0);
}

/*
 Tree traverse for code lengths. return -1 if length
 will be longer than 32, 1 otherwise
*/
int make_lengths_traverse(node* n, int depth)
{
   if(depth>32) return -1;
   if(n->symbol == 256)
   {
      make_lengths_traverse(n->left, depth+1);
      make_lengths_traverse(n->right, depth+1);
   }
   else
   {
      if((encodings[n->symbol] =
         (encoding*)malloc(sizeof(encoding))) == NULL)
            fatal(OUT_OF_MEM);
      encodings[n->symbol]->symbol = n->symbol;
      encodings[n->symbol]->dist = n->dist;
      encodings[n->symbol]->code = (uint32)0;
      encodings[n->symbol]->length = depth;
   }
   return 1;
}

/*
 Divide the dists by 4 and OR in 1 to avoid zeroing
*/
void rescale_dists(uint* dists)
{
   int i;

   for(i=0; i<256; i++)
      if(dists[i])
         if(!(dists[i]=(dists[i]>>2)))
	 	dists[i] |= 1;
}

/*
 Free all encodings data.
*/
void free_encodings()
{
   int i;

   for(i=0; i<256; i++)
      if(encodings[i])
         free(encodings[i]);
}

/*
 Free all tree nodes
*/
void free_tree(node* n)
{
   if(n->left == NULL || n->right == NULL) free(n);
   else
   {
      free_tree(n->left);
      free_tree(n->right);
      free(n);
   }
}

/*
 Make tree codes
*/
void make_codes(node* n, int depth, uint32 code)
{
   if(n->symbol == 256)
   {
      make_codes(n->left, depth+1, code<<=1);
      make_codes(n->right, depth+1, code|=(uint32)1);
   }
   else
   {
      encodings[n->symbol]->code = code;
      encodings[n->symbol]->length = depth;
   }
}

/*
 Count different lengths. return maximum length
*/
int make_code_lengths_count()
{
   int maxlen=0, i;

   memset(lengths_count, 0, sizeof(int)*33);
   for(i=0; i<256; i++)
      if(encodings[i])
      {
         lengths_count[encodings[i]->length]++; /* count lengths */
         if(encodings[i]->length > maxlen)    /* find max length */
            maxlen = encodings[i]->length;
      }
   return maxlen;
}

/*
 How many lengths?
*/
int count_code_lengths(void)
{
   int count, i;

   for(i=0, count=0; i<33; i++)
      if(lengths_count[i])
         count++;

   return count;
}

/*
 Start of the canonical codes
*/
void make_canon_codes_start(int maxlen)
{
   int i;

   memset(codes_start, 0, sizeof(uint32)*33);
   codes_start[maxlen] = (uint32)0;  /* max length is all bits 0 */
   for(i=maxlen-1; i>0; i--)
   {
      codes_start[maxlen-1] = codes_start[maxlen] + lengths_count[maxlen];
      codes_start[maxlen-1] >>= 1;
      maxlen--;
   }
}

/*
 Assignes the final canonical codes
*/
void make_canon_codes()
{
   int i;

   make_canon_codes_start(make_code_lengths_count());
   for(i=0; i<256; i++)
      if(encodings[i])
         encodings[i]->code =
            codes_start[encodings[i]->length]++;
}

/*
 The encodings' size in bits
*/
long file_size(void)
{
   int i;
   long size = 0;

   for(i=0; i<256; i++)
      if(encodings[i])
         size += (encodings[i]->dist*encodings[i]->length);

   return size;
}

/*
 Header size
*/
int file_header_size(void)
{
   int i, size = 0;

   for(i=0; i<256; i++)
      if(encodings[i])
         size += 5;

  return (size+256);
}

/*
 File structure: [header][encoded file]

 header:

  [4 bytes]  file bit-length (without header)
  [256 bits] each bit determines whether the character
             was present in the file or not

  [n*5 bits] the code lengths; n is the number of characters
             that were present in the file; each length
             will take 5 bits (2^5=32)
*/
void encode(int fdin, int fdout, size_t block_size)
{
   byte* buffer;
   uint32* bitbuf;
   ssize_t nread;
   int i;

   if((buffer = (byte*)malloc(block_size)) == NULL)
      fatal(OUT_OF_MEM);

   if((bitbuf = (uint32*)malloc(block_size*4)) == NULL)
      fatal(OUT_OF_MEM);

   bitio_init_put(bitbuf, block_size, fdout);

   /* total file length word */
   bitio_put_bits((uint32)(32+         /* file length word */
                  file_header_size()+  /* header length */
                  file_size()),        /* file length */
                  32);

   for(i=0; i<256; i++)    /* the 'char exists' bits */
      if(!encodings[i])
         bitio_put_bits((uint32)0, 1);
      else
         bitio_put_bits((uint32)1, 1);

   for(i=0; i<256; i++)    /* char encoding lengths */
      if(encodings[i])
         bitio_put_bits(encodings[i]->length, 5);

   lseek(fdin, 0, SEEK_SET);

   while(i=0,
        (nread = read(fdin, buffer, block_size)) > 0)
      while((size_t)i<nread)
         bitio_put_bits(encodings[buffer[i]]->code,
                        encodings[buffer[i]]->length),
         i++;

   bitio_flush();
   free(buffer);
   free(bitbuf);
}

/*
 The main compression function
*/
int compress(int in, int out)
{
   struct stat st;
   int blksize;
   int nodec;
   uint* dists;
   node** nodes;
   node* rootn;

   if(fstat(in, &st) == -1)
   {
      perror("fstat failed");
      exit(EXIT_FAILURE);
   }
   if(!st.st_size) return 1; /* just rename an empty file */

   blksize = st.st_blksize;
   dists = collect_dists(in, blksize);

   nodes = make_nodes(dists, &nodec);
   rootn = make_tree(nodes, nodec);
   while(make_lengths(rootn) == -1) /* downscale needed? */
   {
      free_tree(rootn);
      rescale_dists(dists);
      nodes = make_nodes(dists, &nodec);
      rootn = make_tree(nodes, nodec);
   }
   free(dists);
   make_canon_codes();
   free_tree(rootn);
   encode(in, out, blksize);
   free_encodings();

   return 1;
}

/*
 The main decompression function
*/
int decompress(int in, int out)
{
   struct stat st;
   uint32* bitbuf;
   uint32 bits;
   ssize_t nread;
   int count, maxlen, num_of_lengths;
   size_t blksize;

   if(fstat(in, &st) == -1)
   {
      perror("fstat failed");
      exit(EXIT_FAILURE);
   }
   else{
      if(!st.st_size) return 1; /* just rename an empty file */
      blksize = st.st_blksize;
      if((nread = read(in, &bits, 4)) < 4)
      {
         fprintf(stderr, "error reading file size: ");
         fprintf(stderr, "file not an archive or corrupt archive\n");
         exit(EXIT_FAILURE);
      }else if(bytes_to_words(st.st_size) != bits_to_words(bits))
         {
            fprintf(stderr, "file sizes don't match: ");
            fprintf(stderr, "file not an archive or corrupt archive\n");
            printf("  external size: %d bytes\n", (int)st.st_size);
            printf("  internal size: %d bytes\n", bits_to_bytes(bits));
            exit(EXIT_FAILURE);
         }
      }

   if((bitbuf = (uint32*)malloc(blksize*4)) == NULL)
      fatal(OUT_OF_MEM);

   bitio_init_get(bitbuf, blksize, in, bits-32);
   count = read_encodings();
   maxlen = read_lengths();
   make_canon_codes_start(make_code_lengths_count());
   num_of_lengths = count_code_lengths();
   make_decode_table(num_of_lengths, maxlen);
   decode(maxlen, num_of_lengths, out, blksize);
   free(bitbuf);
   free_encodings();

   return 1;
}

/*
 Read and create the encodings that are needed
*/
int read_encodings(void)
{
   int count, i;

   memset(encodings, 0, sizeof(encoding*)*256);

   for(i=0,count=0; i<256; i++)
      if(bitio_get_bits(1))    /* == (uint32)1 */
      {
         if((encodings[i] =
            (encoding*)malloc(sizeof(encoding))) == NULL)
               fatal(OUT_OF_MEM);
         encodings[i]->symbol = i;
         count++;
      }

   return count;
}

/*
 The lengths
*/
int read_lengths(void)
{
   int maxlen, i;

   for(i=0,maxlen=0; i<256; i++)
      if(encodings[i])
      {
         encodings[i]->length = bitio_get_bits(5);
         if(encodings[i]->length > maxlen)
            maxlen = encodings[i]->length;
      }
   return maxlen;
}

/*
 Deal with the last bits
*/
void end_decode(int maxlen, int num_of_lengths, uint32 code,
   int out, int block_size, byte* buffer, int cur_byte)
{
   int shift=1, firstpass=1, i;
   uint32 leftover;

   while(shift)
   {
      i = find_table_index(num_of_lengths-1, code);
      shift = maxlen-decode_table.entries[i].length;
      if(!shift && !firstpass) break;
      leftover = ~(uint32)0>>(32-shift);
      leftover &= code;
      leftover <<= (maxlen-shift);
      if(!shift) leftover = (uint32)0;
      code -= decode_table.entries[i].start;
      code >>= shift;
      buffer[cur_byte++] = decode_table.entries[i].elems[code]->symbol;
      code = (uint32)0;
      code |= leftover;
      firstpass = 0;
      if(cur_byte==block_size)   /* buffer full */
      {
         write(out, buffer, block_size);
         cur_byte = 0;
      }
   }
   write(out, buffer, cur_byte);
}

/*
 Decoding process
*/
void decode(int maxlen, int num_of_lengths, int out, int block_size)
{
   byte* buffer;
   long available;
   int cur_byte, shift, i;
   uint32 code, leftover;

   if((buffer = (byte*)malloc(block_size)) == NULL)
      fatal(OUT_OF_MEM);

   cur_byte = shift = 0;
   leftover = code = (uint32)0;
   while(1)
   {
      available = bitio_available();
      if(available<=(maxlen-shift))
      {
         if(shift)
         {
            leftover >>= ((maxlen-shift)-available);
            leftover |= bitio_get_bits(available);
            leftover <<= ((maxlen-shift)-available);
            end_decode(maxlen, num_of_lengths, leftover, out, block_size, buffer, cur_byte);
            break;
         }
         if(available==0)
         {
            write(out, buffer, cur_byte);
            break;
         }
         else
         {
            code = bitio_get_bits(available);
            code <<= (maxlen-available);
            end_decode(maxlen, num_of_lengths, code, out, block_size, buffer, cur_byte);
         }
         break;
      }
      code = (uint32)0;
      code |= leftover;
      code |= bitio_get_bits(maxlen-shift);
      i = find_table_index(num_of_lengths-1, code);
      shift = maxlen-decode_table.entries[i].length;
      leftover = ~(uint32)0>>(32-shift);
      leftover &= code;
      leftover <<= (maxlen-shift);
      if(!shift) leftover = (uint32)0;
      code -= decode_table.entries[i].start;
      code >>= shift;
      buffer[cur_byte++] = decode_table.entries[i].elems[code]->symbol;
      if(cur_byte==block_size)   /* buffer full */
      {
         write(out, buffer, block_size);
         cur_byte = 0;
      }
   }

   free(buffer);
}

/*
 The table for decoding, each entry will contain:

      [start of canonical code]
      [length]
      [pointer to array of encodings of the same length]

 Start codes will be extended to the length of the longest
 code ('maxlen').
*/
void make_decode_table(int lengths, int maxlen)
{
   int cur, i, j;
   int index[32];

   memset(index, 0, sizeof(int)*32);

   if((decode_table.entries =
      (table_entry*)malloc(sizeof(table_entry)*lengths)) == NULL)
         fatal(OUT_OF_MEM);

   for(i=0, cur=0; i<33; i++)
      if(lengths_count[i])
      {
         decode_table.entries[cur].start = (codes_start[i]<<(maxlen-i));
         decode_table.entries[cur].length = i;
         if((decode_table.entries[cur].elems =
            (encoding**)malloc(sizeof(encoding*)*lengths_count[i])) == NULL)
               fatal(OUT_OF_MEM);
         cur++;
      }

   for(i=0; i<256; i++)
      if(encodings[i])
         for(j=0; j<maxlen; j++)
            if(decode_table.entries[j].length == encodings[i]->length)
            {
               decode_table.entries[j].elems[index[encodings[i]->length]++] =
                  encodings[i];
               break;
            }
}

/*
 Finds the index of the entry by comparing the start
 code with the 'code'. TODO: Should be done with binary search instead!
*/
int find_table_index(int len, uint32 code)
{
   int i;

   for(i=len; i>0; i--)
   {
      if(decode_table.entries[i].start == code)
         return i;
      if(decode_table.entries[i-1].start > code)
         return i;
   }

   return i;
}
