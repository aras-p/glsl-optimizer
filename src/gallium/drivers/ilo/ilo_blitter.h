/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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

#ifndef ILO_BLITTER_H
#define ILO_BLITTER_H

#include "ilo_common.h"
#include "ilo_context.h"
#include "ilo_gpe.h"

enum ilo_blitter_uses {
   ILO_BLITTER_USE_DSA           = 1 << 0,
   ILO_BLITTER_USE_CC            = 1 << 1,
   ILO_BLITTER_USE_VIEWPORT      = 1 << 2,
   ILO_BLITTER_USE_FB_DEPTH      = 1 << 3,
   ILO_BLITTER_USE_FB_STENCIL    = 1 << 4,
};

enum ilo_blitter_rectlist_op {
   ILO_BLITTER_RECTLIST_CLEAR_ZS,
   ILO_BLITTER_RECTLIST_RESOLVE_Z,
   ILO_BLITTER_RECTLIST_RESOLVE_HIZ,
};

struct blitter_context;
struct pipe_resource;
struct pipe_surface;

struct ilo_blitter {
   struct ilo_context *ilo;
   struct blitter_context *pipe_blitter;

   /*
    * A minimal context with the goal to send RECTLISTs down the pipeline.
    */
   enum ilo_blitter_rectlist_op op;
   uint32_t uses;

   bool initialized;

   struct {
      struct pipe_resource *res;
      unsigned offset, size;
   } buffer;

   struct ilo_ve_state ve;
   struct ilo_vb_state vb;
   struct pipe_draw_info draw;

   struct ilo_viewport_cso viewport;
   struct ilo_dsa_state dsa;

   struct {
      struct pipe_stencil_ref stencil_ref;
      ubyte alpha_ref;
      struct pipe_blend_color blend_color;
   } cc;

   uint32_t depth_clear_value;

   struct {
      struct ilo_surface_cso dst;
      unsigned width, height;
      unsigned num_samples;
   } fb;
};

struct ilo_blitter *
ilo_blitter_create(struct ilo_context *ilo);

void
ilo_blitter_destroy(struct ilo_blitter *blitter);

bool
ilo_blitter_pipe_blit(struct ilo_blitter *blitter,
                      const struct pipe_blit_info *info);

bool
ilo_blitter_pipe_copy_resource(struct ilo_blitter *blitter,
                               struct pipe_resource *dst, unsigned dst_level,
                               unsigned dst_x, unsigned dst_y, unsigned dst_z,
                               struct pipe_resource *src, unsigned src_level,
                               const struct pipe_box *src_box);

bool
ilo_blitter_pipe_clear_rt(struct ilo_blitter *blitter,
                          struct pipe_surface *rt,
                          const union pipe_color_union *color,
                          unsigned x, unsigned y,
                          unsigned width, unsigned height);

bool
ilo_blitter_pipe_clear_zs(struct ilo_blitter *blitter,
                          struct pipe_surface *zs,
                          unsigned clear_flags,
                          double depth, unsigned stencil,
                          unsigned x, unsigned y,
                          unsigned width, unsigned height);

bool
ilo_blitter_pipe_clear_fb(struct ilo_blitter *blitter,
                          unsigned buffers,
                          const union pipe_color_union *color,
                          double depth, unsigned stencil);

bool
ilo_blitter_blt_copy_resource(struct ilo_blitter *blitter,
                              struct pipe_resource *dst, unsigned dst_level,
                              unsigned dst_x, unsigned dst_y, unsigned dst_z,
                              struct pipe_resource *src, unsigned src_level,
                              const struct pipe_box *src_box);

bool
ilo_blitter_blt_clear_rt(struct ilo_blitter *blitter,
                         struct pipe_surface *rt,
                         const union pipe_color_union *color,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height);

bool
ilo_blitter_blt_clear_zs(struct ilo_blitter *blitter,
                         struct pipe_surface *zs,
                         unsigned clear_flags,
                         double depth, unsigned stencil,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height);

bool
ilo_blitter_rectlist_clear_zs(struct ilo_blitter *blitter,
                              struct pipe_surface *zs,
                              unsigned clear_flags,
                              double depth, unsigned stencil);

void
ilo_blitter_rectlist_resolve_z(struct ilo_blitter *blitter,
                               struct pipe_resource *res,
                               unsigned level, unsigned slice);

void
ilo_blitter_rectlist_resolve_hiz(struct ilo_blitter *blitter,
                                 struct pipe_resource *res,
                                 unsigned level, unsigned slice);

#endif /* ILO_BLITTER_H */
