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

#include "util/u_surface.h"

#include "ilo_blitter.h"
#include "ilo_context.h"
#include "ilo_blit.h"

static void
ilo_resource_copy_region(struct pipe_context *pipe,
                         struct pipe_resource *dst,
                         unsigned dst_level,
                         unsigned dstx, unsigned dsty, unsigned dstz,
                         struct pipe_resource *src,
                         unsigned src_level,
                         const struct pipe_box *src_box)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (ilo_blitter_blt_copy_resource(ilo->blitter,
            dst, dst_level, dstx, dsty, dstz,
            src, src_level, src_box))
      return;

   if (ilo_blitter_pipe_copy_resource(ilo->blitter,
            dst, dst_level, dstx, dsty, dstz,
            src, src_level, src_box))
      return;

   util_resource_copy_region(&ilo->base, dst, dst_level,
         dstx, dsty, dstz, src, src_level, src_box);
}

static void
ilo_clear(struct pipe_context *pipe,
          unsigned buffers,
          const union pipe_color_union *color,
          double depth,
          unsigned stencil)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_blitter_pipe_clear_fb(ilo->blitter, buffers, color, depth, stencil);
}

static void
ilo_clear_render_target(struct pipe_context *pipe,
                        struct pipe_surface *dst,
                        const union pipe_color_union *color,
                        unsigned dstx, unsigned dsty,
                        unsigned width, unsigned height)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (!width || !height || dstx >= dst->width || dsty >= dst->height)
      return;

   if (dstx + width > dst->width)
      width = dst->width - dstx;
   if (dsty + height > dst->height)
      height = dst->height - dsty;

   if (ilo_blitter_blt_clear_rt(ilo->blitter,
            dst, color, dstx, dsty, width, height))
      return;

   ilo_blitter_pipe_clear_rt(ilo->blitter,
         dst, color, dstx, dsty, width, height);
}

static void
ilo_clear_depth_stencil(struct pipe_context *pipe,
                        struct pipe_surface *dst,
                        unsigned clear_flags,
                        double depth,
                        unsigned stencil,
                        unsigned dstx, unsigned dsty,
                        unsigned width, unsigned height)
{
   struct ilo_context *ilo = ilo_context(pipe);

   if (!width || !height || dstx >= dst->width || dsty >= dst->height)
      return;

   if (dstx + width > dst->width)
      width = dst->width - dstx;
   if (dsty + height > dst->height)
      height = dst->height - dsty;

   if (ilo_blitter_blt_clear_zs(ilo->blitter,
            dst, clear_flags, depth, stencil, dstx, dsty, width, height))
      return;

   ilo_blitter_pipe_clear_zs(ilo->blitter,
         dst, clear_flags, depth, stencil, dstx, dsty, width, height);
}

static void
ilo_blit(struct pipe_context *pipe, const struct pipe_blit_info *info)
{
   struct ilo_context *ilo = ilo_context(pipe);

   ilo_blitter_pipe_blit(ilo->blitter, info);
}

/**
 * Initialize blit-related functions.
 */
void
ilo_init_blit_functions(struct ilo_context *ilo)
{
   ilo->base.resource_copy_region = ilo_resource_copy_region;
   ilo->base.blit = ilo_blit;

   ilo->base.clear = ilo_clear;
   ilo->base.clear_render_target = ilo_clear_render_target;
   ilo->base.clear_depth_stencil = ilo_clear_depth_stencil;
}
