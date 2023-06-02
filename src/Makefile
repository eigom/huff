CC=gcc
OPTS=-Wall -ansi
OBJECTS=compr.o bitio.o huffman.o

compr: $(OBJECTS)
	$(CC) $(OPTS) -o compr $(OBJECTS)

compr.o: compr.c
	$(CC) $(OPTS) -o compr.o -c compr.c

bitio.o: bitio.c bitio.h
	$(CC) $(OPTS) -o bitio.o -c bitio.c

huffman.o: huffman.c huffman.h
	$(CC) $(OPTS) -o huffman.o -c huffman.c