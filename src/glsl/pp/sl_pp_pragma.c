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
#include "sl_pp_process.h"


int
sl_pp_process_pragma(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last,
                     struct sl_pp_process_state *state)
{
   int pragma_name = -1;
   struct sl_pp_token_info out;
   int arg_name = -1;

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      pragma_name = input[first].data.identifier;
      first++;
   }
   if (pragma_name == -1) {
      return 0;
   }

   if (pragma_name == context->dict.optimize) {
      out.token = SL_PP_PRAGMA_OPTIMIZE;
   } else if (pragma_name == context->dict.debug) {
      out.token = SL_PP_PRAGMA_DEBUG;
   } else {
      return 0;
   }

   while (first < last && input[first].token == SL_PP_WHITESPACE) {
      first++;
   }

   if (first < last && input[first].token == SL_PP_LPAREN) {
      first++;
   } else {
      return 0;
   }

   while (first < last && input[first].token == SL_PP_WHITESPACE) {
      first++;
   }

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      arg_name = input[first].data.identifier;
      first++;
   }
   if (arg_name == -1) {
      return 0;
   }

   if (arg_name == context->dict.off) {
      out.data.pragma = 0;
   } else if (arg_name == context->dict.on) {
      out.data.pragma = 1;
   } else {
      return 0;
   }

   while (first < last && input[first].token == SL_PP_WHITESPACE) {
      first++;
   }

   if (first < last && input[first].token == SL_PP_RPAREN) {
      first++;
   } else {
      return 0;
   }

   /* Ignore the tokens that follow. */

   if (sl_pp_process_out(state, &out)) {
      strcpy(context->error_msg, "out of memory");
      return -1;
   }

   return 0;
}
