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

#ifndef SL_PP_PROCESS_H
#define SL_PP_PROCESS_H

#include "sl_pp_macro.h"
#include "sl_pp_token.h"


struct sl_pp_context;

struct sl_pp_process_state {
   struct sl_pp_token_info *out;
   unsigned int out_len;
   unsigned int out_max;
};

int
sl_pp_process_define(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last);

int
sl_pp_process_undef(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last);

int
sl_pp_process_if(struct sl_pp_context *context,
                 struct sl_pp_token_buffer *input);

int
sl_pp_process_ifdef(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last);

int
sl_pp_process_ifndef(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last);

int
sl_pp_process_elif(struct sl_pp_context *context,
                   struct sl_pp_token_buffer *buffer);

int
sl_pp_process_else(struct sl_pp_context *context,
                   const struct sl_pp_token_info *input,
                   unsigned int first,
                   unsigned int last);

int
sl_pp_process_endif(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last);

void
sl_pp_process_error(struct sl_pp_context *context,
                    const struct sl_pp_token_info *input,
                    unsigned int first,
                    unsigned int last);

int
sl_pp_process_pragma(struct sl_pp_context *context,
                     const struct sl_pp_token_info *input,
                     unsigned int first,
                     unsigned int last,
                     struct sl_pp_process_state *state);

int
sl_pp_process_extension(struct sl_pp_context *context,
                        const struct sl_pp_token_info *input,
                        unsigned int first,
                        unsigned int last,
                        struct sl_pp_process_state *state);

int
sl_pp_process_line(struct sl_pp_context *context,
                   struct sl_pp_token_buffer *buffer,
                   struct sl_pp_process_state *state);

int
sl_pp_process_out(struct sl_pp_process_state *state,
                  const struct sl_pp_token_info *token);

#endif /* SL_PP_PROCESS_H */
