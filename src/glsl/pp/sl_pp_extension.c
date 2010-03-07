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
#include "sl_pp_public.h"


/**
 * Declare an extension to the preprocessor.  This tells the preprocessor
 * which extensions are supported by Mesa.
 * The shader still needs to have a "#extension name: behavior" line to enable
 * the extension.
 */
int
sl_pp_context_add_extension(struct sl_pp_context *context,
                            const char *name)
{
   struct sl_pp_extension ext;

   if (context->num_extensions == SL_PP_MAX_EXTENSIONS) {
      return -1;
   }

   ext.name = sl_pp_context_add_unique_str(context, name);
   if (ext.name == -1) {
      return -1;
   }

   context->extensions[context->num_extensions++] = ext;

   assert(context->num_extensions <= sizeof(context->extensions));

   return 0;
}


/**
 * Process a "#extension name: behavior" directive.
 */
int
sl_pp_process_extension(struct sl_pp_context *context,
                        const struct sl_pp_token_info *input,
                        unsigned int first,
                        unsigned int last,
                        struct sl_pp_process_state *state)
{
   int extension_name = -1;
   int behavior = -1;
   struct sl_pp_token_info out;

   /* Grab the extension name. */
   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      extension_name = input[first].data.identifier;
      first++;
   }
   if (extension_name == -1) {
      strcpy(context->error_msg, "expected identifier after `#extension'");
      return -1;
   }

   /* Make sure the extension is supported. */
   if (extension_name == context->dict.all) {
      out.data.extension = extension_name;
   } else {
      unsigned int i;

      out.data.extension = -1;
      for (i = 0; i < context->num_extensions; i++) {
         if (extension_name == context->extensions[i].name) {
            out.data.extension = extension_name;
            break;
         }
      }
   }

   /* Grab the colon separating the extension name and behavior. */
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

   /* Grab the behavior name. */
   if (first < last && input[first].token == SL_PP_IDENTIFIER) {
      behavior = input[first].data.identifier;
      first++;
   }
   if (behavior == -1) {
      strcpy(context->error_msg, "expected identifier after `:'");
      return -1;
   }

   if (behavior == context->dict.require) {
      if (out.data.extension == -1) {
         strcpy(context->error_msg, "the required extension is not supported");
         return -1;
      }
      if (out.data.extension == context->dict.all) {
         strcpy(context->error_msg, "invalid behavior for `all' extension: `require'");
         return -1;
      }
      out.token = SL_PP_EXTENSION_REQUIRE;
   } else if (behavior == context->dict.enable) {
      if (out.data.extension == -1) {
         /* Warning: the extension cannot be enabled. */
         return 0;
      }
      if (out.data.extension == context->dict.all) {
         strcpy(context->error_msg, "invalid behavior for `all' extension: `enable'");
         return -1;
      }
      out.token = SL_PP_EXTENSION_ENABLE;
   } else if (behavior == context->dict.warn) {
      if (out.data.extension == -1) {
         /* Warning: the extension is not supported. */
         return 0;
      }
      out.token = SL_PP_EXTENSION_WARN;
   } else if (behavior == context->dict.disable) {
      if (out.data.extension == -1) {
         /* Warning: the extension is not supported. */
         return 0;
      }
      out.token = SL_PP_EXTENSION_DISABLE;
   } else {
      strcpy(context->error_msg, "unrecognised behavior name");
      return -1;
   }

   /* Grab the end of line. */
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
