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


struct sl_pp_context *
sl_pp_context_create(void)
{
   struct sl_pp_context *context;

   context = calloc(1, sizeof(struct sl_pp_context));
   if (!context) {
      return NULL;
   }

   if (sl_pp_dict_init(context)) {
      sl_pp_context_destroy(context);
      return NULL;
   }

   context->getc_buf = malloc(64 * sizeof(char));
   if (!context->getc_buf) {
      sl_pp_context_destroy(context);
      return NULL;
   }

   context->macro_tail = &context->macro;
   context->if_ptr = SL_PP_MAX_IF_NESTING;
   context->if_value = 1;
   memset(context->error_msg, 0, sizeof(context->error_msg));
   context->line = 1;
   context->file = 0;

   return context;
}

void
sl_pp_context_destroy(struct sl_pp_context *context)
{
   if (context) {
      free(context->cstr_pool);
      sl_pp_macro_free(context->macro);
      free(context->getc_buf);
      free(context);
   }
}

const char *
sl_pp_context_error_message(const struct sl_pp_context *context)
{
   return context->error_msg;
}

int
sl_pp_context_add_unique_str(struct sl_pp_context *context,
                             const char *str)
{
   unsigned int size;
   unsigned int offset = 0;

   size = strlen(str) + 1;

   /* Find out if this is a unique string. */
   while (offset < context->cstr_pool_len) {
      const char *str2;
      unsigned int size2;

      str2 = &context->cstr_pool[offset];
      size2 = strlen(str2) + 1;
      if (size == size2 && !memcmp(str, str2, size - 1)) {
         return offset;
      }

      offset += size2;
   }

   if (context->cstr_pool_len + size > context->cstr_pool_max) {
      context->cstr_pool_max = (context->cstr_pool_len + size + 0xffff) & ~0xffff;
      context->cstr_pool = realloc(context->cstr_pool, context->cstr_pool_max);
   }

   if (!context->cstr_pool) {
      strcpy(context->error_msg, "out of memory");
      return -1;
   }

   offset = context->cstr_pool_len;
   memcpy(&context->cstr_pool[offset], str, size);
   context->cstr_pool_len += size;

   return offset;
}

const char *
sl_pp_context_cstr(const struct sl_pp_context *context,
                   int offset)
{
   if (offset == -1) {
      return NULL;
   }
   return &context->cstr_pool[offset];
}
