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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../pp/sl_pp_public.h"
#include "../cl/sl_cl_parse.h"


static void
usage(void)
{
   printf("Usage:\n");
   printf("  compile fragment|vertex <source> <output>\n");
}

int
main(int argc,
     char *argv[])
{
   FILE *in;
   long size;
   char *inbuf;
   struct sl_pp_purify_options options;
   char errmsg[100] = "";
   struct sl_pp_context *context;
   struct sl_pp_token_info *tokens;
   unsigned int version;
   unsigned int tokens_eaten;
   struct sl_pp_token_info *outtokens;
   FILE *out;
   unsigned int i, j;
   unsigned char *outbytes;
   unsigned int cboutbytes;
   unsigned int shader_type;

   if (argc != 4) {
      usage();
      return 1;
   }

   if (!strcmp(argv[1], "fragment")) {
      shader_type = 1;
   } else if (!strcmp(argv[1], "vertex")) {
      shader_type = 2;
   } else {
      usage();
      return 1;
   }

   in = fopen(argv[2], "rb");
   if (!in) {
      printf("Could not open `%s' for read.\n", argv[2]);
      usage();
      return 1;
   }

   fseek(in, 0, SEEK_END);
   size = ftell(in);
   fseek(in, 0, SEEK_SET);

   out = fopen(argv[3], "w");
   if (!out) {
      fclose(in);
      printf("Could not open `%s' for write.\n", argv[3]);
      usage();
      return 1;
   }

   inbuf = malloc(size + 1);
   if (!inbuf) {
      fprintf(out, "$OOMERROR\n");

      fclose(out);
      fclose(in);
      printf("Out of memory.\n");
      return 0;
   }

   if (fread(inbuf, 1, size, in) != size) {
      fprintf(out, "$READERROR\n");

      free(inbuf);
      fclose(out);
      fclose(in);
      printf("Could not read from `%s'.\n", argv[2]);
      return 0;
   }
   inbuf[size] = '\0';

   fclose(in);

   memset(&options, 0, sizeof(options));

   context = sl_pp_context_create();
   if (!context) {
      fprintf(out, "$CONTEXERROR\n");

      free(inbuf);
      fclose(out);
      printf("Could not create parse context.\n");
      return 0;
   }

   if (sl_pp_tokenise(context, inbuf, &options, &tokens)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      printf("Error: %s.\n", sl_pp_context_error_message(context));
      sl_pp_context_destroy(context);
      free(inbuf);
      fclose(out);
      return 0;
   }

   free(inbuf);

   if (sl_pp_version(context, tokens, &version, &tokens_eaten)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      printf("Error: %s\n", sl_pp_context_error_message(context));
      sl_pp_context_destroy(context);
      free(tokens);
      fclose(out);
      return 0;
   }

   if (sl_pp_context_add_extension(context, "ARB_draw_buffers", "GL_ARB_draw_buffers") ||
       sl_pp_context_add_extension(context, "ARB_texture_rectangle", "GL_ARB_texture_rectangle")) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      printf("Error: %s\n", sl_pp_context_error_message(context));
      sl_pp_context_destroy(context);
      free(tokens);
      fclose(out);
      return 0;
   }

   if (sl_pp_process(context, &tokens[tokens_eaten], &outtokens)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      printf("Error: %s\n", sl_pp_context_error_message(context));
      sl_pp_context_destroy(context);
      free(tokens);
      fclose(out);
      return 0;
   }

   free(tokens);

   for (i = j = 0; outtokens[i].token != SL_PP_EOF; i++) {
      switch (outtokens[i].token) {
      case SL_PP_NEWLINE:
      case SL_PP_EXTENSION_REQUIRE:
      case SL_PP_EXTENSION_ENABLE:
      case SL_PP_EXTENSION_WARN:
      case SL_PP_EXTENSION_DISABLE:
      case SL_PP_LINE:
         break;
      default:
         outtokens[j++] = outtokens[i];
      }
   }
   outtokens[j] = outtokens[i];

   if (sl_cl_compile(context, outtokens, shader_type, 1, &outbytes, &cboutbytes, errmsg, sizeof(errmsg)) == 0) {
      unsigned int i;
      unsigned int line = 0;

      fprintf(out, "\n/* DO NOT EDIT - THIS FILE IS AUTOMATICALLY GENERATED FROM THE FOLLOWING FILE: */");
      fprintf(out, "\n/* %s */", argv[2]);
      fprintf(out, "\n\n");

      for (i = 0; i < cboutbytes; i++) {
         unsigned int a;

         if (outbytes[i] < 10) {
            a = 1;
         } else if (outbytes[i] < 100) {
            a = 2;
         } else {
            a = 3;
         }
         if (i < cboutbytes - 1) {
            a++;
         }
         if (line + a >= 100) {
            fprintf (out, "\n");
            line = 0;
         }
         line += a;
         fprintf (out, "%u", outbytes[i]);
         if (i < cboutbytes - 1) {
            fprintf (out, ",");
         }
      }
      fprintf (out, "\n");
      free(outbytes);
   } else {
      fprintf(out, "$SYNTAXERROR: `%s'\n", errmsg);

      printf("Error: %s\n", errmsg);
   }

   sl_pp_context_destroy(context);
   free(outtokens);
   fclose(out);
   return 0;
}
