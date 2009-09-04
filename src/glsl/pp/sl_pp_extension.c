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
sl_pp_process_extension(struct sl_pp_context *context,
                        const struct sl_pp_token_info *input,
                        unsigned int first,
                        unsigned int last,
                        struct sl_pp_process_state *state)
{
   int all_extensions = -1;
   const char *extension_name = NULL;
   const char *behavior = NULL;
   struct sl_pp_token_info out;

   all_extensions = sl_pp_context_add_unique_str(context, "all");
   if (all_extensions == -1) {
      return -1;
   }

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      extension_name = sl_pp_context_cstr(context, input[first].data.identifier);
      first++;
   }
   if (!extension_name) {
      strcpy(context->error_msg, "expected identifier after `#extension'");
      return -1;
   }

   if (!strcmp(extension_name, "all")) {
      out.data.extension = all_extensions;
   } else {
      out.data.extension = -1;
   }

   while (first < last && input[first].token == SL_PP_WHITESPACE) {
      first++;
   }

   if (first < last && input[first].token == SL_PP_COLON) {
      first++;
   } else {
      strcpy(context->error_msg, "expected `:' after extension name");
      return -1;
   }

   while (first < last && input[first].token == SL_PP_WHITESPACE) {
      first++;
   }

   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      behavior = sl_pp_context_cstr(context, input[first].data.identifier);
      first++;
   }
   if (!behavior) {
      strcpy(context->error_msg, "expected identifier after `:'");
      return -1;
   }

   if (!strcmp(behavior, "require")) {
      strcpy(context->error_msg, "unable to enable required extension");
      return -1;
   } else if (!strcmp(behavior, "enable")) {
      if (out.data.extension == all_extensions) {
         strcpy(context->error_msg, "unable to enable all extensions");
         return -1;
      } else {
         return 0;
      }
   } else if (!strcmp(behavior, "warn")) {
      if (out.data.extension == all_extensions) {
         out.token = SL_PP_EXTENSION_WARN;
      } else {
         return 0;
      }
   } else if (!strcmp(behavior, "disable")) {
      if (out.data.extension == all_extensions) {
         out.token = SL_PP_EXTENSION_DISABLE;
      } else {
         return 0;
      }
   } else {
      strcpy(context->error_msg, "unrecognised behavior name");
      return -1;
   }

   while (first < last && input[first].token == SL_PP_WHITESPACE) {
      first++;
   }

   if (first < last) {
      strcpy(context->error_msg, "expected end of line after behavior name");
      return -1;
   }

   if (sl_pp_process_out(state, &out)) {
      return -1;
   }

   return 0;
}
