/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../pp/sl_pp_public.h"


int
main(int argc,
     char *argv[])
{
   FILE *in;
   long size;
   char *inbuf;
   struct sl_pp_purify_options options;
   char *outbuf;
   char errmsg[100] = "";
   unsigned int errline = 0;
   FILE *out;

   if (argc != 3) {
      printf("Usage: purify infile outfile\n");
      return 1;
   }

   in = fopen(argv[1], "rb");
   if (!in) {
      return 1;
   }

   fseek(in, 0, SEEK_END);
   size = ftell(in);
   assert(size != -1);
   fseek(in, 0, SEEK_SET);

   out = fopen(argv[2], "wb");
   if (!out) {
      fclose(in);
      return 1;
   }

   inbuf = malloc(size + 1);
   if (!inbuf) {
      fprintf(out, "$OOMERROR\n");

      fclose(out);
      fclose(in);
      return 1;
   }

   if (fread(inbuf, 1, size, in) != size) {
      fprintf(out, "$READERROR\n");

      free(inbuf);
      fclose(out);
      fclose(in);
      return 1;
   }
   inbuf[size] = '\0';

   fclose(in);

   memset(&options, 0, sizeof(options));

   if (sl_pp_purify(inbuf, &options, &outbuf, errmsg, sizeof(errmsg), &errline)) {
      fprintf(out, "$PURIFYERROR %u: %s\n", errline, errmsg);

      free(inbuf);
      fclose(out);
      return 1;
   }

   free(inbuf);

   fwrite(outbuf, 1, strlen(outbuf), out);

   free(outbuf);
   fclose(out);

   return 0;
}
