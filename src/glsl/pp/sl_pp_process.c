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
#include "sl_pp_process.h"


enum process_state {
   state_seek_hash,
   state_seek_directive,
   state_seek_newline,
   state_expand
};

int
sl_pp_process(const struct sl_pp_token_info *input,
              struct sl_pp_token_info **output)
{
   unsigned int i = 0;
   enum process_state state = state_seek_hash;
   struct sl_pp_token_info *out = NULL;
   unsigned int out_len = 0;
   unsigned int out_max = 0;

   for (;;) {
      struct sl_pp_token_info info;
      int info_valid = 0;

      switch (input[i].token) {
      case SL_PP_WHITESPACE:
         /* Drop whitespace alltogether at this point. */
         i++;
         break;

      case SL_PP_NEWLINE:
      case SL_PP_EOF:
         /* Preserve newline just for the sake of line numbering. */
         info = input[i];
         info_valid = 1;
         i++;
         /* Restart directive parsing. */
         state = state_seek_hash;
         break;

      case SL_PP_HASH:
         if (state == state_seek_hash) {
            i++;
            state = state_seek_directive;
         } else {
            /* Error: unexpected token. */
            return -1;
         }
         break;

      case SL_PP_IDENTIFIER:
         if (state == state_seek_hash) {
            info = input[i];
            info_valid = 1;
            i++;
            state = state_expand;
         } else if (state == state_seek_directive) {
            i++;
            state = state_seek_newline;
         } else if (state == state_expand) {
            info = input[i];
            info_valid = 1;
            i++;
         } else {
            i++;
         }
         break;

      default:
         if (state == state_seek_hash) {
            info = input[i];
            info_valid = 1;
            i++;
            state = state_expand;
         } else if (state == state_seek_directive) {
            /* Error: expected directive name. */
            return -1;
         } else if (state == state_expand) {
            info = input[i];
            info_valid = 1;
            i++;
         } else {
            i++;
         }
      }

      if (info_valid) {
         if (out_len >= out_max) {
            unsigned int new_max = out_max;

            if (new_max < 0x100) {
               new_max = 0x100;
            } else if (new_max < 0x10000) {
               new_max *= 2;
            } else {
               new_max += 0x10000;
            }

            out = realloc(out, new_max * sizeof(struct sl_pp_token_info));
            if (!out) {
               return -1;
            }
            out_max = new_max;
         }

         if (info.token == SL_PP_IDENTIFIER) {
            info.data.identifier = strdup(info.data.identifier);
            if (!info.data.identifier) {
               return -1;
            }
         } else if (info.token == SL_PP_NUMBER) {
            info.data.number = strdup(info.data.number);
            if (!info.data.number) {
               return -1;
            }
         }

         out[out_len++] = info;

         if (info.token == SL_PP_EOF) {
            break;
         }
      }
   }

   *output = out;
   return 0;
}
