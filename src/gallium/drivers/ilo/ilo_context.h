/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_CONTEXT_H
#define ILO_CONTEXT_H

#include "pipe/p_context.h"
#include "util/u_slab.h"

#include "ilo_common.h"
#include "ilo_cp.h"
#include "ilo_draw.h"
#include "ilo_state.h"

struct u_upload_mgr;
struct intel_winsys;

struct ilo_blitter;
struct ilo_render;
struct ilo_screen;
struct ilo_shader_cache;

struct ilo_context {
   struct pipe_context base;

   struct intel_winsys *winsys;
   struct ilo_dev_info *dev;

   struct util_slab_mempool transfer_mempool;

   struct ilo_cp *cp;

   struct ilo_shader_cache *shader_cache;
   struct ilo_blitter *blitter;
   struct ilo_render *render;

   struct u_upload_mgr *uploader;

   struct ilo_state_vector state_vector;

   struct {
      struct pipe_query *query;
      bool condition;
      unsigned mode;
   } render_condition;

   struct {
      struct ilo_cp_owner cp_owner;
      struct list_head queries;
   } draw;
};

static inline struct ilo_context *
ilo_context(struct pipe_context *pipe)
{
   return (struct ilo_context *) pipe;
}

void
ilo_init_context_functions(struct ilo_screen *is);

bool
ilo_skip_rendering(struct ilo_context *ilo);

#endif /* ILO_CONTEXT_H */
