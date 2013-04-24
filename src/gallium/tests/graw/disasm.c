/**************************************************************************
 *
 * Copyright 2013 VMware, Inc.
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
 * IN NO EVENT SHALL THE AUTHOR AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/*
 * Small utility to disassemble a memory dump of TGSI tokens.
 *
 * Dump can be easily obtained from gdb through the tgsi_dump.gdb helper:
 *
 *  (gdb) source tgsi_dump.gdb
 *  (gdb) tgsi_dump state->tokens
 *
 * which will generate a tgsi_dump.bin file in the current directory.
 */


#include <stdio.h>
#include <stdlib.h>

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_dump.h"


static void
usage(const char *arg0)
{
   fprintf(stderr, "usage: %s [ options ] <tgsi_dump.bin> ...\n", arg0);
}


static void
disasm(const char *filename)
{
   FILE *fp;
   const size_t max_tokens = 1024*1024;
   struct tgsi_token *tokens;

   fp = fopen(filename, "rb");
   if (!fp) {
      exit(1);
   }
   tokens = malloc(max_tokens * sizeof *tokens);
   fread(tokens, sizeof *tokens, max_tokens, fp);

   tgsi_dump(tokens, 0);

   free(tokens);
   fclose(fp);
}


int main( int argc, char *argv[] )
{
   int i;

   if (argc < 2) {
      usage(argv[0]);
      return 0;
   }

   for (i = 1; i < argc; ++i) {
      disasm(argv[i]);
   }

   return 0;
}
