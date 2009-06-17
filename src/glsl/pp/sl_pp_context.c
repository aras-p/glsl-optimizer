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
#include "sl_pp_context.h"


void
sl_pp_context_init(struct sl_pp_context *context)
{
   memset(context, 0, sizeof(struct sl_pp_context));
}

void
sl_pp_context_destroy(struct sl_pp_context *context)
{
   free(context->cstr_pool);
}

int
sl_pp_context_add_str(struct sl_pp_context *context,
                      const char *str)
{
   unsigned int size;
   unsigned int offset;

   size = strlen(str) + 1;

   if (context->cstr_pool_len + size > context->cstr_pool_max) {
      context->cstr_pool_max = (context->cstr_pool_len + size + 0xffff) & ~0xffff;
      context->cstr_pool = realloc(context->cstr_pool, context->cstr_pool_max);
   }

   if (!context->cstr_pool) {
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
