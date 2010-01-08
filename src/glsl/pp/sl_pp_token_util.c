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

#include <assert.h>
#include <stdlib.h>
#include "sl_pp_token_util.h"


int
sl_pp_token_buffer_init(struct sl_pp_token_buffer *buffer,
                        struct sl_pp_context *context)
{
   buffer->context = context;
   buffer->size = 0;
   buffer->capacity = 64;
   buffer->tokens = malloc(buffer->capacity * sizeof(struct sl_pp_token_info));
   if (!buffer->tokens) {
      return -1;
   }
   return 0;
}

void
sl_pp_token_buffer_destroy(struct sl_pp_token_buffer *buffer)
{
   free(buffer->tokens);
}

int
sl_pp_token_buffer_get(struct sl_pp_token_buffer *buffer,
                       struct sl_pp_token_info *out)
{
   /* Pop from stack first if not empty. */
   if (buffer->size) {
      *out = buffer->tokens[--buffer->size];
      return 0;
   }

   assert(buffer->context);
   return sl_pp_token_get(buffer->context, out);
}

void
sl_pp_token_buffer_unget(struct sl_pp_token_buffer *buffer,
                         const struct sl_pp_token_info *in)
{
   /* Resize if needed. */
   if (buffer->size == buffer->capacity) {
      buffer->capacity += 64;
      buffer->tokens = realloc(buffer->tokens,
                               buffer->capacity * sizeof(struct sl_pp_token_info));
      assert(buffer->tokens);
   }

   /* Push token on stack. */
   buffer->tokens[buffer->size++] = *in;
}

int
sl_pp_token_buffer_skip_white(struct sl_pp_token_buffer *buffer,
                              struct sl_pp_token_info *out)
{
   if (sl_pp_token_buffer_get(buffer, out)) {
      return -1;
   }

   while (out->token == SL_PP_WHITESPACE) {
      if (sl_pp_token_buffer_get(buffer, out)) {
         return -1;
      }
   }

   return 0;
}



int
sl_pp_token_peek_init(struct sl_pp_token_peek *peek,
                      struct sl_pp_token_buffer *buffer)
{
   peek->buffer = buffer;
   peek->size = 0;
   peek->capacity = 64;
   peek->tokens = malloc(peek->capacity * sizeof(struct sl_pp_token_info));
   if (!peek->tokens) {
      return -1;
   }
   return 0;
}

void
sl_pp_token_peek_destroy(struct sl_pp_token_peek *peek)
{
   /* Abort. */
   while (peek->size) {
      sl_pp_token_buffer_unget(peek->buffer, &peek->tokens[--peek->size]);
   }
   free(peek->tokens);
}

int
sl_pp_token_peek_get(struct sl_pp_token_peek *peek,
                     struct sl_pp_token_info *out)
{
   /* Get token from buffer. */
   if (sl_pp_token_buffer_get(peek->buffer, out)) {
      return -1;
   }

   /* Save it. */
   if (peek->size == peek->capacity) {
      peek->capacity += 64;
      peek->tokens = realloc(peek->tokens,
                             peek->capacity * sizeof(struct sl_pp_token_info));
      assert(peek->tokens);
   }
   peek->tokens[peek->size++] = *out;
   return 0;
}

void
sl_pp_token_peek_commit(struct sl_pp_token_peek *peek)
{
   peek->size = 0;
}

int
sl_pp_token_peek_to_buffer(const struct sl_pp_token_peek *peek,
                           struct sl_pp_token_buffer *buffer)
{
   unsigned int i;

   if (sl_pp_token_buffer_init(buffer, NULL)) {
      return -1;
   }
   for (i = peek->size; i > 0; i--) {
      sl_pp_token_buffer_unget(buffer, &peek->tokens[i - 1]);
   }
   return 0;
}

int
sl_pp_token_peek_skip_white(struct sl_pp_token_peek *peek,
                            struct sl_pp_token_info *out)
{
   if (sl_pp_token_peek_get(peek, out)) {
      return -1;
   }

   while (out->token == SL_PP_WHITESPACE) {
      if (sl_pp_token_peek_get(peek, out)) {
         return -1;
      }
   }

   return 0;
}
