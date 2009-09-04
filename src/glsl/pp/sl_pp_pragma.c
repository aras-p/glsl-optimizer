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
sl_pp_process_pragma(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last,
                     struct sl_pp_process_state *state)
{
   const char *pragma_name = NULL;
   struct sl_pp_token_info out;
   const char *arg_name = NULL;

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      pragma_name = sl_pp_context_cstr(context, input[first].data.identifier);
      first++;
   }
   if (!pragma_name) {
      return 0;
   }

   if (!strcmp(pragma_name, "optimize")) {
      out.token = SL_PP_PRAGMA_OPTIMIZE;
   } else if (!strcmp(pragma_name, "debug")) {
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
      arg_name = sl_pp_context_cstr(context, input[first].data.identifier);
      first++;
   }
   if (!arg_name) {
      return 0;
   }

   if (!strcmp(arg_name, "off")) {
      out.data.pragma = 0;
   } else if (!strcmp(arg_name, "on")) {
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
      return -1;
   }

   return 0;
}
