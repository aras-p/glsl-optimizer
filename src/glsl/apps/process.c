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


int
main(int argc,
     char *argv[])
{
   FILE *in;
   long size;
   char *inbuf;
   struct sl_pp_purify_options options;
   struct sl_pp_context *context;
   unsigned int version;
   struct sl_pp_token_info *outtokens;
   FILE *out;
   unsigned int i;

   if (argc != 3) {
      printf("Usage: process infile outfile\n");
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

   context = sl_pp_context_create(inbuf, &options);
   if (!context) {
      fprintf(out, "$CONTEXERROR\n");

      free(inbuf);
      fclose(out);
      return 1;
   }

   if (sl_pp_version(context, &version)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      sl_pp_context_destroy(context);
      free(inbuf);
      fclose(out);
      return -1;
   }

   if (sl_pp_context_add_extension(context, "GL_ARB_draw_buffers") ||
       sl_pp_context_add_extension(context, "GL_ARB_texture_rectangle")) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      printf("Error: %s\n", sl_pp_context_error_message(context));
      sl_pp_context_destroy(context);
      free(inbuf);
      fclose(out);
      return 0;
   }

   if (sl_pp_context_add_predefined(context, "__GLSL_PP_PREDEFINED_MACRO_TEST", "1")) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      printf("Error: %s\n", sl_pp_context_error_message(context));
      sl_pp_context_destroy(context);
      free(inbuf);
      fclose(out);
      return 0;
   }

   if (sl_pp_process(context, &outtokens)) {
      fprintf(out, "$ERROR: `%s'\n", sl_pp_context_error_message(context));

      sl_pp_context_destroy(context);
      free(inbuf);
      fclose(out);
      return -1;
   }

   free(inbuf);

   for (i = 0; outtokens[i].token != SL_PP_EOF; i++) {
      switch (outtokens[i].token) {
      case SL_PP_NEWLINE:
         fprintf(out, "\n");
         break;

      case SL_PP_COMMA:
         fprintf(out, ", ");
         break;

      case SL_PP_SEMICOLON:
         fprintf(out, "; ");
         break;

      case SL_PP_LBRACE:
         fprintf(out, "{ ");
         break;

      case SL_PP_RBRACE:
         fprintf(out, "} ");
         break;

      case SL_PP_LPAREN:
         fprintf(out, "( ");
         break;

      case SL_PP_RPAREN:
         fprintf(out, ") ");
         break;

      case SL_PP_LBRACKET:
         fprintf(out, "[ ");
         break;

      case SL_PP_RBRACKET:
         fprintf(out, "] ");
         break;

      case SL_PP_DOT:
         fprintf(out, ". ");
         break;

      case SL_PP_INCREMENT:
         fprintf(out, "++ ");
         break;

      case SL_PP_ADDASSIGN:
         fprintf(out, "+= ");
         break;

      case SL_PP_PLUS:
         fprintf(out, "+ ");
         break;

      case SL_PP_DECREMENT:
         fprintf(out, "-- ");
         break;

      case SL_PP_SUBASSIGN:
         fprintf(out, "-= ");
         break;

      case SL_PP_MINUS:
         fprintf(out, "- ");
         break;

      case SL_PP_BITNOT:
         fprintf(out, "~ ");
         break;

      case SL_PP_NOTEQUAL:
         fprintf(out, "!= ");
         break;

      case SL_PP_NOT:
         fprintf(out, "! ");
         break;

      case SL_PP_MULASSIGN:
         fprintf(out, "*= ");
         break;

      case SL_PP_STAR:
         fprintf(out, "* ");
         break;

      case SL_PP_DIVASSIGN:
         fprintf(out, "/= ");
         break;

      case SL_PP_SLASH:
         fprintf(out, "/ ");
         break;

      case SL_PP_MODASSIGN:
         fprintf(out, "%%= ");
         break;

      case SL_PP_MODULO:
         fprintf(out, "%% ");
         break;

      case SL_PP_LSHIFTASSIGN:
         fprintf(out, "<<= ");
         break;

      case SL_PP_LSHIFT:
         fprintf(out, "<< ");
         break;

      case SL_PP_LESSEQUAL:
         fprintf(out, "<= ");
         break;

      case SL_PP_LESS:
         fprintf(out, "< ");
         break;

      case SL_PP_RSHIFTASSIGN:
         fprintf(out, ">>= ");
         break;

      case SL_PP_RSHIFT:
         fprintf(out, ">> ");
         break;

      case SL_PP_GREATEREQUAL:
         fprintf(out, ">= ");
         break;

      case SL_PP_GREATER:
         fprintf(out, "> ");
         break;

      case SL_PP_EQUAL:
         fprintf(out, "== ");
         break;

      case SL_PP_ASSIGN:
         fprintf(out, "= ");
         break;

      case SL_PP_AND:
         fprintf(out, "&& ");
         break;

      case SL_PP_BITANDASSIGN:
         fprintf(out, "&= ");
         break;

      case SL_PP_BITAND:
         fprintf(out, "& ");
         break;

      case SL_PP_XOR:
         fprintf(out, "^^ ");
         break;

      case SL_PP_BITXORASSIGN:
         fprintf(out, "^= ");
         break;

      case SL_PP_BITXOR:
         fprintf(out, "^ ");
         break;

      case SL_PP_OR:
         fprintf(out, "|| ");
         break;

      case SL_PP_BITORASSIGN:
         fprintf(out, "|= ");
         break;

      case SL_PP_BITOR:
         fprintf(out, "| ");
         break;

      case SL_PP_QUESTION:
         fprintf(out, "? ");
         break;

      case SL_PP_COLON:
         fprintf(out, ": ");
         break;

      case SL_PP_IDENTIFIER:
         fprintf(out, "%s ", sl_pp_context_cstr(context, outtokens[i].data.identifier));
         break;

      case SL_PP_UINT:
         fprintf(out, "%s ", sl_pp_context_cstr(context, outtokens[i].data._uint));
         break;

      case SL_PP_FLOAT:
         fprintf(out, "%s ", sl_pp_context_cstr(context, outtokens[i].data._float));
         break;

      case SL_PP_OTHER:
         fprintf(out, "%c", outtokens[i].data.other);
         break;

      case SL_PP_PRAGMA_OPTIMIZE:
         fprintf(out, "#pragma optimize(%s)", outtokens[i].data.pragma ? "on" : "off");
         break;

      case SL_PP_PRAGMA_DEBUG:
         fprintf(out, "#pragma debug(%s)", outtokens[i].data.pragma ? "on" : "off");
         break;

      case SL_PP_EXTENSION_REQUIRE:
         fprintf(out, "#extension %s : require", sl_pp_context_cstr(context, outtokens[i].data.extension));
         break;

      case SL_PP_EXTENSION_ENABLE:
         fprintf(out, "#extension %s : enable", sl_pp_context_cstr(context, outtokens[i].data.extension));
         break;

      case SL_PP_EXTENSION_WARN:
         fprintf(out, "#extension %s : warn", sl_pp_context_cstr(context, outtokens[i].data.extension));
         break;

      case SL_PP_EXTENSION_DISABLE:
         fprintf(out, "#extension %s : disable", sl_pp_context_cstr(context, outtokens[i].data.extension));
         break;

      case SL_PP_LINE:
         fprintf(out, "#line %u %u", outtokens[i].data.line.lineno, outtokens[i].data.line.fileno);
         break;

      default:
         assert(0);
      }
   }

   sl_pp_context_destroy(context);
   free(outtokens);
   fclose(out);

   return 0;
}
