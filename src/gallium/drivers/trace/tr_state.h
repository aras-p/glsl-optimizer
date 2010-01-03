/*
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef TR_STATE_H_
#define TR_STATE_H_

#include "tr_context.h"

struct tgsi_token;

enum trace_shader_type {
   TRACE_SHADER_FRAGMENT = 0,
   TRACE_SHADER_VERTEX   = 1,
   TRACE_SHADER_GEOMETRY = 2
};

struct trace_shader
{
   struct tr_list list;

   enum trace_shader_type type;

   void *state;
   void *replaced;

   struct tgsi_token *tokens;
   struct tgsi_token *replaced_tokens;

   boolean disabled;
};


static INLINE struct trace_shader *
trace_shader(void *state)
{
   return (struct trace_shader *)state;
}

struct trace_shader * trace_shader_create(struct trace_context *tr_ctx,
                                          const struct pipe_shader_state *state,
                                          void *result,
                                          enum trace_shader_type type);

void trace_shader_destroy(struct trace_context *tr_ctx,
                          struct trace_shader *tr_shdr);

#endif
