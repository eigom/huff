/*
 32bit file I/O for Huffman encoding/decoding
 Eigo Madaloja
*/

#include "bitio.h"

static uint32* buffer;
static size_t buffer_size;
static int current_word;
static int total_words;
static uint32 total_bits;
static int empty_bits;
static int in_out_file;

/*
 Initialize all needed data for input and output
*/
void bitio_init(uint32* buf, size_t size, int fd)
{
   buffer = buf;
   buffer_size = size;
   in_out_file = fd;
   current_word = 0;
   empty_bits = 32;
}

/*
 Initialize file write
*/
void bitio_init_put(uint32* buf, size_t size, int fd)
{
   bitio_init(buf, size, fd);
   total_words = 0;
   memset(buffer, 0, sizeof(uint32)*buffer_size);
}

/*
 Write bits from a word, return number of empty bits
*/
int bitio_put_bits(uint32 word, int bits)
{
   int bits_left;

   if(bits>empty_bits)
   {
      bits_left = bits - empty_bits;   /* bits that don't fit */
      buffer[current_word] |= (word>>bits_left);   /* push em' out */
      if(++current_word==buffer_size)  /* buffer full */
         bitio_buf_flush();
      empty_bits = 32;
      return bitio_put_bits(word&(~(uint32)0>>(32-bits_left)), bits_left); /* leftover */
   }
   else if(bits<empty_bits)
   {
      empty_bits = empty_bits - bits;
      buffer[current_word] |= (word<<empty_bits);
      return empty_bits;
   }
   else
   {
      buffer[current_word] |= word;
      if(++current_word==buffer_size)  /* buffer full */
         bitio_buf_flush();
      return(empty_bits = 32);
   }
}

/*
 Write out full buffer
*/
void bitio_buf_flush(void)
{
   (void)write(in_out_file, buffer, buffer_size*4);
   memset(buffer, 0, sizeof(uint32)*buffer_size);
   total_words += buffer_size;
   current_word = 0;
}

/*
 Initialize file read
*/
void bitio_init_get(uint32* buf, int size, int fd, uint32 bits)
{
   bitio_init(buf, size, fd);
   total_bits = bits;
   (void)read(in_out_file, buffer, buffer_size*4);
}

/*
 Read bits
*/
uint32 bitio_get_bits(int bits)
{
   int shift;
   uint32 word;

   if(bits>empty_bits)
   {
      total_bits-=empty_bits;
      word = ~(uint32)0>>(32-empty_bits);
      word&=buffer[current_word];
      word<<=(bits-empty_bits);
      if(++current_word==buffer_size)
      {
         (void)read(in_out_file, buffer, buffer_size*4);
         current_word = 0;
      }
      shift = bits-empty_bits;
      empty_bits = 32;
      return word|bitio_get_bits(shift);
   }
   else if(bits<empty_bits)
   {
      shift = ((32-empty_bits) + (empty_bits-bits));
      word = ~(uint32)0>>shift;
      word = (word<<(empty_bits-bits))&buffer[current_word];
      word >>= (empty_bits-bits);
      total_bits-=bits;
      empty_bits-=bits;
   }
   else
   {
      word = buffer[current_word]&(~(uint32)0>>(32-empty_bits));
      if(++current_word==buffer_size)
      {
         (void)read(in_out_file, buffer, buffer_size*4);
         current_word = 0;
      }
      total_bits-=bits;
      empty_bits = 32;
   }
   return word;
}

/*
 Bits available for read
*/
uint32 bitio_available(void)
{
   return total_bits;
}

/*
 Write any unwritten bytes
*/
ssize_t bitio_flush(void)
{
   int word_bytes = (current_word*4);
   if(empty_bits != 32) word_bytes+=4;

   return write(in_out_file, buffer, word_bytes);
}

/*
 The number of full bits on the last word
*/
int bitio_full_bits(void)
{
   return (32-empty_bits);
}

/*
 The number of full words written
*/
int bitio_total_words(void)
{
   return (total_words+current_word);
}

/*
 Total bytes written
*/
int bitio_total_bytes(void)
{
   return (current_word*4)+((empty_bits==32)?0:4);
}

/*
 Total bits written
*/
int bitio_total_bits(void)
{
   return bitio_total_words()*32+bitio_full_bits();
}
