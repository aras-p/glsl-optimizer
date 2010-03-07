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
#include "sl_pp_context.h"
#include "sl_pp_public.h"
#include "sl_pp_process.h"


int
sl_pp_process_line(struct sl_pp_context *context,
                   struct sl_pp_token_buffer *buffer,
                   struct sl_pp_process_state *pstate)
{
   struct sl_pp_process_state state;
   int found_end = 0;
   int line_number = -1;
   int file_number = -1;
   unsigned int line;
   unsigned int file;

   memset(&state, 0, sizeof(state));
   while (!found_end) {
      struct sl_pp_token_info input;

      sl_pp_token_buffer_get(buffer, &input);
      switch (input.token) {
      case SL_PP_WHITESPACE:
         break;

      case SL_PP_IDENTIFIER:
         sl_pp_token_buffer_unget(buffer, &input);
         if (sl_pp_macro_expand(context, buffer, NULL, &state, sl_pp_macro_expand_normal)) {
            free(state.out);
            return -1;
         }
         break;

      case SL_PP_NEWLINE:
      case SL_PP_EOF:
         found_end = 1;
         break;

      default:
         if (sl_pp_process_out(&state, &input)) {
            strcpy(context->error_msg, "out of memory");
            free(state.out);
            return -1;
         }
      }
   }

   if (state.out_len > 0 && state.out[0].token == SL_PP_UINT) {
      line_number = state.out[0].data._uint;
   } else {
      strcpy(context->error_msg, "expected a number after `#line'");
      free(state.out);
      return -1;
   }

   if (state.out_len > 1) {
      if (state.out[1].token == SL_PP_UINT) {
         file_number = state.out[1].data._uint;
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
   if (file_number != -1) {
      file = atoi(sl_pp_context_cstr(context, file_number));
   } else {
      file = context->file;
   }

   if (context->line != line || context->file != file) {
      struct sl_pp_token_info ti;

      ti.token = SL_PP_LINE;
      ti.data.line.lineno = line;
      ti.data.line.fileno = file;
      if (sl_pp_process_out(pstate, &ti)) {
         strcpy(context->error_msg, "out of memory");
         return -1;
      }

      context->line = line;
      context->file = file;
   }

   return 0;
}
