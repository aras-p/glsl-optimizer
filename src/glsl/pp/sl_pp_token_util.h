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

#ifndef SL_PP_TOKEN_UTIL_H
#define SL_PP_TOKEN_UTIL_H

#include <assert.h>
#include <stdlib.h>
#include "sl_pp_token.h"


struct sl_pp_context;

/*
 * A token buffer allows one to get and unget a token
 * from a preprocessor context.
 */
struct sl_pp_token_buffer {
   struct sl_pp_context *context;
   unsigned int size;
   unsigned int capacity;
   struct sl_pp_token_info *tokens;
};

int
sl_pp_token_buffer_init(struct sl_pp_token_buffer *buffer,
                        struct sl_pp_context *context);

void
sl_pp_token_buffer_destroy(struct sl_pp_token_buffer *buffer);

int
sl_pp_token_buffer_get(struct sl_pp_token_buffer *buffer,
                       struct sl_pp_token_info *out);

void
sl_pp_token_buffer_unget(struct sl_pp_token_buffer *buffer,
                         const struct sl_pp_token_info *in);

int
sl_pp_token_buffer_skip_white(struct sl_pp_token_buffer *buffer,
                              struct sl_pp_token_info *out);


/*
 * A token peek allows one to get a number of tokens from a buffer
 * and then either commit the operation or abort it,
 * effectively ungetting the peeked tokens.
 */
struct sl_pp_token_peek {
   struct sl_pp_token_buffer *buffer;
   unsigned int size;
   unsigned int capacity;
   struct sl_pp_token_info *tokens;
};

int
sl_pp_token_peek_init(struct sl_pp_token_peek *peek,
                      struct sl_pp_token_buffer *buffer);

void
sl_pp_token_peek_destroy(struct sl_pp_token_peek *peek);

int
sl_pp_token_peek_get(struct sl_pp_token_peek *peek,
                     struct sl_pp_token_info *out);

void
sl_pp_token_peek_commit(struct sl_pp_token_peek *peek);

int
sl_pp_token_peek_to_buffer(const struct sl_pp_token_peek *peek,
                           struct sl_pp_token_buffer *buffer);

int
sl_pp_token_peek_skip_white(struct sl_pp_token_peek *peek,
                            struct sl_pp_token_info *out);

#endif /* SL_PP_TOKEN_UTIL_H */
