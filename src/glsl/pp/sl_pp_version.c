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
              const struct sl_pp_token_info *input,
              unsigned int *version,
              unsigned int *tokens_eaten)
{
   unsigned int i = 0;
   unsigned int line = context->line;

   /* Default values if `#version' is not present. */
   *version = 110;
   *tokens_eaten = 0;

   /* There can be multiple `#version' directives present.
    * Accept the value of the last one.
    */
   for (;;) {
      int found_hash = 0;
      int found_version = 0;
      int found_number = 0;
      int found_end = 0;

      /* Skip whitespace and newlines and seek for hash. */
      while (!found_hash) {
         switch (input[i].token) {
         case SL_PP_NEWLINE:
            line++;
            /* pass thru */
         case SL_PP_WHITESPACE:
            i++;
            break;

         case SL_PP_HASH:
            i++;
            found_hash = 1;
            break;

         default:
            return 0;
         }
      }

      /* Skip whitespace and seek for `version'. */
      while (!found_version) {
         switch (input[i].token) {
         case SL_PP_WHITESPACE:
            i++;
            break;

         case SL_PP_IDENTIFIER:
            if (input[i].data.identifier != context->dict.version) {
               return 0;
            }
            i++;
            found_version = 1;
            break;

         default:
            return 0;
         }
      }

      /* Skip whitespace and seek for version number. */
      while (!found_number) {
         switch (input[i].token) {
         case SL_PP_WHITESPACE:
            i++;
            break;

         case SL_PP_UINT:
            *version = atoi(sl_pp_context_cstr(context, input[i].data._uint));
            i++;
            found_number = 1;
            break;

         default:
            strcpy(context->error_msg, "expected version number after `#version'");
            return -1;
         }
      }

      /* Skip whitespace and seek for either newline or eof. */
      while (!found_end) {
         switch (input[i].token) {
         case SL_PP_WHITESPACE:
            i++;
            break;

         case SL_PP_NEWLINE:
            line++;
            /* pass thru */
         case SL_PP_EOF:
            i++;
            *tokens_eaten = i;
            context->line = line;
            found_end = 1;
            break;

         default:
            strcpy(context->error_msg, "expected end of line after version number");
            return -1;
         }
      }
   }

   /* Should not get here. */
}
