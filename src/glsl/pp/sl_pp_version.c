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

#include <stdlib.h>
#include <string.h>
#include "sl_pp_public.h"
#include "sl_pp_context.h"


int
sl_pp_version(struct sl_pp_context *context,
              unsigned int *version)
{
   struct sl_pp_token_peek peek;
   unsigned int line = context->line;

   /* Default values if `#version' is not present. */
   *version = 110;

   if (sl_pp_token_peek_init(&peek, &context->tokens)) {
      return -1;
   }

   /* There can be multiple `#version' directives present.
    * Accept the value of the last one.
    */
   for (;;) {
      struct sl_pp_token_info input;
      int found_hash = 0;
      int found_version = 0;
      int found_number = 0;
      int found_end = 0;

      /* Skip whitespace and newlines and seek for hash. */
      while (!found_hash) {
         if (sl_pp_token_peek_get(&peek, &input)) {
            sl_pp_token_peek_destroy(&peek);
            return -1;
         }

         switch (input.token) {
         case SL_PP_NEWLINE:
            line++;
            break;

         case SL_PP_WHITESPACE:
            break;

         case SL_PP_HASH:
            found_hash = 1;
            break;

         default:
            sl_pp_token_peek_destroy(&peek);
            return 0;
         }
      }

      /* Skip whitespace and seek for `version'. */
      while (!found_version) {
         if (sl_pp_token_peek_get(&peek, &input)) {
            sl_pp_token_peek_destroy(&peek);
            return -1;
         }

         switch (input.token) {
         case SL_PP_WHITESPACE:
            break;

         case SL_PP_IDENTIFIER:
            if (input.data.identifier != context->dict.version) {
               sl_pp_token_peek_destroy(&peek);
               return 0;
            }
            found_version = 1;
            break;

         default:
            sl_pp_token_peek_destroy(&peek);
            return 0;
         }
      }

      sl_pp_token_peek_commit(&peek);

      /* Skip whitespace and seek for version number. */
      while (!found_number) {
         if (sl_pp_token_buffer_get(&context->tokens, &input)) {
            sl_pp_token_peek_destroy(&peek);
            return -1;
         }

         switch (input.token) {
         case SL_PP_WHITESPACE:
            break;

         case SL_PP_UINT:
            *version = atoi(sl_pp_context_cstr(context, input.data._uint));
            found_number = 1;
            break;

         default:
            strcpy(context->error_msg, "expected version number after `#version'");
            sl_pp_token_peek_destroy(&peek);
            return -1;
         }
      }

      /* Skip whitespace and seek for either newline or eof. */
      while (!found_end) {
         if (sl_pp_token_buffer_get(&context->tokens, &input)) {
            sl_pp_token_peek_destroy(&peek);
            return -1;
         }

         switch (input.token) {
         case SL_PP_WHITESPACE:
            break;

         case SL_PP_NEWLINE:
            line++;
            /* pass thru */
         case SL_PP_EOF:
            context->line = line;
            found_end = 1;
            break;

         default:
            strcpy(context->error_msg, "expected end of line after version number");
            sl_pp_token_peek_destroy(&peek);
            return -1;
         }
      }
   }

   /* Should not get here. */
}
