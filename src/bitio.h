/*
 32bit file I/O for Huffman encoding/decoding
 Eigo Madaloja
*/

#ifndef _BITIO_H_
#define _BITIO_H_

#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* One of the following needs to be
commented out depending on host type. */

typedef u_int32_t uint32;

void     bitio_init(uint32*, size_t, int);
void     bitio_init_put(uint32*, size_t, int);
int      bitio_put_bits(uint32, int);
void     bitio_buf_flush(void);

void     bitio_init_get(uint32*, int, int, uint32);
uint32   bitio_get_bits(int);
uint32   bitio_get_bit(int);
uint32   bitio_available(void);

ssize_t  bitio_flush(void);
int      bitio_full_bits(void);
int      bitio_total_words(void);
int      bitio_total_bytes(void);
int      bitio_total_bits(void);

#endif
