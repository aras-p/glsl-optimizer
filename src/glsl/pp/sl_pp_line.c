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
#include "sl_pp_process.h"


int
sl_pp_process_line(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int first,
                   unsigned int last,
                   struct sl_pp_process_state *pstate)
{
   unsigned int i;
   struct sl_pp_process_state state;
   int line_number = -1;
   int file_number = -1;
   unsigned int line;

   memset(&state, 0, sizeof(state));
   for (i = first; i < last;) {
      switch (input[i].token) {
      case SL_PP_WHITESPACE:
         i++;
         break;

      case SL_PP_IDENTIFIER:
         if (sl_pp_macro_expand(context, input, &i, NULL, &state, 0)) {
            free(state.out);
            return -1;
         }
         break;

      default:
         if (sl_pp_process_out(&state, &input[i])) {
            strcpy(context->error_msg, "out of memory");
            free(state.out);
            return -1;
         }
         i++;
      }
   }

   if (state.out_len > 0 && state.out[0].token == SL_PP_NUMBER) {
      line_number = state.out[0].data.number;
   } else {
      strcpy(context->error_msg, "expected a number after `#line'");
      free(state.out);
      return -1;
   }

   if (state.out_len > 1) {
      if (state.out[1].token == SL_PP_NUMBER) {
         file_number = state.out[1].data.number;
      } else {
         strcpy(context->error_msg, "expected a number after line number");
         free(state.out);
         return -1;
      }

      if (state.out_len > 2) {
         strcpy(context->error_msg, "expected an end of line after file number");
         free(state.out);
         return -1;
      }
   }

   free(state.out);

   line = atoi(sl_pp_context_cstr(context, line_number));

   if (context->line != line) {
      struct sl_pp_token_info ti;

      ti.token = SL_PP_LINE;
      ti.data.line = line;
      if (sl_pp_process_out(pstate, &ti)) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }

      context->line = line;
   }

   if (file_number != -1) {
      unsigned int file;

      file_number = atoi(sl_pp_context_cstr(context, file_number));

      if (context->file != file) {
         struct sl_pp_token_info ti;

         ti.token = SL_PP_FILE;
         ti.data.file = file;
         if (sl_pp_process_out(pstate, &ti)) {
            strcpy(context->error_msg, "out of memory");
            return -1;
         }

         context->file = file;
      }
   }

   return 0;
}
