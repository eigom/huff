/*
 Main program for huffman (de)compression
 Eigo Madaloja
*/

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "huffman.h"

char* usage =
         "\n    usage: compr [-d] infile outfile\n"
         "          -d: decompress\n";


int main(int argc, char** args)
{
   struct stat stin;
   struct stat stout;
   int in, out;
   char* ifname;
   char* ofname;
   int decompr;

   if(argc<3 || argc>4)
   {
      puts(usage);
      return EXIT_FAILURE;
   }

   if(argc==4)
   {
      if(strcmp(args[1], "-d") != 0)
      {
         printf("%s", args[1]);
         printf(" - unknown option, type 'compr' for usage\n");
         return EXIT_FAILURE;
      }
      ifname = args[2];
      ofname = args[3];
      decompr = 1;
   }
   else
   {
      ifname = args[1];
      ofname = args[2];
      decompr = 0;
   }


   /* open in read/write mode, this will
   not allow action on dirs */
   if((in = open(ifname, O_RDWR)) == -1)
   {
      perror(ifname);
      return EXIT_FAILURE;
   }
   if(fstat(in, &stin) == -1)
   {
      perror("fstat failed");
      return EXIT_FAILURE;
   }

   if((out = open(ofname, O_WRONLY | O_CREAT | O_TRUNC, stin.st_mode)) == -1)
   {
      perror(ofname);
      return EXIT_FAILURE;
   }

   if(fstat(out, &stout) == -1)
   {
      perror("fstat failed");
      return EXIT_FAILURE;
   }

   if((stin.st_dev == stout.st_dev) && (stin.st_ino == stout.st_ino))
   {
      fprintf(stderr,
              "%s: %s: cannot (de)compress file to itself\n",
              ifname, ofname);
      return EXIT_FAILURE;
   }

   if(decompr) decompress(in, out);
   else compress(in, out);

   close(in);
   close(out);

   return EXIT_SUCCESS;
}
