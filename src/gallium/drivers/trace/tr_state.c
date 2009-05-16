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

#include "tr_state.h"

#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "tgsi/tgsi_parse.h"

struct trace_shader * trace_shader_create(struct trace_context *tr_ctx,
                                          const struct pipe_shader_state *state,
                                          void *result,
                                          enum trace_shader_type type)
{
   struct trace_shader *tr_shdr = CALLOC_STRUCT(trace_shader);

   tr_shdr->state = result;
   tr_shdr->type = type;
   tr_shdr->tokens = tgsi_dup_tokens(state->tokens);

   /* works on context as well */
   trace_screen_add_to_list(tr_ctx, shaders, tr_shdr);

   return tr_shdr;
}

void trace_shader_destroy(struct trace_context *tr_ctx,
                          struct trace_shader *tr_shdr)
{
   trace_screen_remove_from_list(tr_ctx, shaders, tr_shdr);

   if (tr_shdr->replaced) {
      if (tr_shdr->type == TRACE_SHADER_FRAGMENT)
         tr_ctx->pipe->delete_fs_state(tr_ctx->pipe, tr_shdr->replaced);
      else if (tr_shdr->type == TRACE_SHADER_VERTEX)
         tr_ctx->pipe->delete_vs_state(tr_ctx->pipe, tr_shdr->replaced);
      else
         assert(0);
   }

   FREE(tr_shdr->replaced_tokens);
   FREE(tr_shdr->tokens);
   FREE(tr_shdr);
}
