# Data compression using Huffman's method

## Encoding/Decoding

Program uses canonical Huffman codes which have several advantages compared to "normal (tree) Huffman codes:

- the codes can be formed by knowing only the length of the code for a symbol
- the resulting codes can be decoded efficiently

Rules for canonical codes are as follows:

- shorter codes have a numerically higher value
- within the same length, numerical values increase with alphabet

File structure:

    [filelength (32 bits)]
    [the symbol exists bits (256 bits)]
    [lengths for each symbol (5 bits each)]
    [the codes]

Minimum header size would be: 32+256+0\*5=288b (36B)

Maximum: 32+256+256\*5=1568b (196B)

Sources for canonical Huffman:

1. 'Practical Huffman coding' by Michael Schindler [[www.compressconsult.com](http://www.compressconsult.com "www.compressconsult.com")]
2. Texts by Arturo Campos [[www.arturocampos.com](http://www.arturocampos.com "www.arturocampos.com")]

## Bit I/O
The bitio module implements efficient bitwise file I/O by making use of an internal buffer.

## Building the program

Type ****make**** to build the program.
