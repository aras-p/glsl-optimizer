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

#include "ilo_gpe.h"
#include "ilo_common.h"

struct pipe_draw_info;
struct u_upload_mgr;
struct intel_winsys;
struct intel_bo;
struct ilo_3d;
struct ilo_blitter;
struct ilo_cp;
struct ilo_screen;
struct ilo_shader_state;

struct ilo_context {
   struct pipe_context base;

   struct intel_winsys *winsys;
   struct ilo_dev_info *dev;

   struct util_slab_mempool transfer_mempool;

   struct ilo_cp *cp;
   struct intel_bo *last_cp_bo;

   struct ilo_shader_cache *shader_cache;
   struct ilo_3d *hw3d;
   struct ilo_blitter *blitter;

   struct u_upload_mgr *uploader;

   const struct pipe_draw_info *draw;
   uint32_t dirty;

   struct ilo_vb_state vb;
   const struct ilo_ve_state *ve;
   struct ilo_ib_state ib;

   struct ilo_shader_state *vs;
   struct ilo_shader_state *gs;

   struct ilo_so_state so;

   struct pipe_clip_state clip;
   struct ilo_viewport_state viewport;
   struct ilo_scissor_state scissor;

   const struct ilo_rasterizer_state *rasterizer;
   struct pipe_poly_stipple poly_stipple;
   unsigned sample_mask;

   struct ilo_shader_state *fs;

   const struct ilo_dsa_state *dsa;
   struct pipe_stencil_ref stencil_ref;
   const struct ilo_blend_state *blend;
   struct pipe_blend_color blend_color;
   struct ilo_fb_state fb;

   /* shader resources */
   struct ilo_sampler_state sampler[PIPE_SHADER_TYPES];
   struct ilo_view_state view[PIPE_SHADER_TYPES];
   struct ilo_cbuf_state cbuf[PIPE_SHADER_TYPES];
   struct ilo_resource_state resource;

   /* GPGPU */
   struct ilo_shader_state *cs;
   struct ilo_resource_state cs_resource;
   struct ilo_global_binding global_binding;
};

static inline struct ilo_context *
ilo_context(struct pipe_context *pipe)
{
   return (struct ilo_context *) pipe;
}

void
ilo_init_context_functions(struct ilo_screen *is);

#endif /* ILO_CONTEXT_H */
