/*
 Huffman encoding/decoding
 Eigo Madaloja
*/
#ifndef _HUFFMAN_H_
#define _HUFFMAN_H_

#define _XOPEN_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "bitio.h"

#define bits_to_bytes(b) ( ((b)/8) + (((b)%8)? 1:0) )
#define bits_to_words(b) ( ((b)/32) + (((b)%32)? 1:0) )
#define bytes_to_words(b) ( ((b)/4) + (((b)%4)? 1:0) )

typedef unsigned char byte;
typedef unsigned int uint;

typedef struct _node{
   int symbol;
   int dist;
   struct _node* left;
   struct _node* right;
} node;

typedef struct _encoding{
   int symbol;
   int dist;
   uint32 code;
   int length;
} encoding;

typedef struct _table_entry{
   uint32 start;
   int length;
   encoding** elems;
} table_entry;

typedef struct _table{
   table_entry* entries;
} table;

enum error_codes{
   OUT_OF_MEM
};

void     fatal(int);
void     fatale(int, char*, int);

uint*    collect_dists(int, size_t);
int      node_cmp_dist(const void*, const void*);
node**   make_nodes(int*, int*);
node*    make_tree(node**, int);
int      make_lengths(node*);
int      make_lengths_traverse(node*, int);
void     rescale_dists(uint*);
void     make_codes(node*, int, uint32);

int      make_code_lengths_count(void);
void     make_canon_codes_start(int);
void     make_canon_codes(void);

long     file_size(void);
int      file_header_size(void);
void     encode(int, int, size_t);

int      read_encodings(void);
int      read_lengths(void);
void     decode(int, int, int, int);
void     make_decode_table(int, int);
int      find_table_index(int, uint32);

int      count_code_lengths(void);
void     free_encodings(void);
void     free_tree(node*);

int compress(int in, int out);
int decompress(int in, int out);

#endif
