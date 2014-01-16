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

   if ((buffers & PIPE_CLEAR_DEPTHSTENCIL) && ilo->fb.state.zsbuf) {
      if (ilo_blitter_rectlist_clear_zs(ilo->blitter, ilo->fb.state.zsbuf,
               buffers & PIPE_CLEAR_DEPTHSTENCIL, depth, stencil))
         buffers &= ~PIPE_CLEAR_DEPTHSTENCIL;

      if (!buffers)
         return;
   }

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

static void
ilo_flush_resource(struct pipe_context *pipe, struct pipe_resource *res)
{
   struct ilo_context *ilo = ilo_context(pipe);
   const unsigned flags = ILO_TEXTURE_CPU_READ |
                          ILO_TEXTURE_BLT_READ |
                          ILO_TEXTURE_RENDER_READ;

   ilo_blit_resolve_resource(ilo, res, flags);
}

void
ilo_blit_resolve_slices_for_hiz(struct ilo_context *ilo,
                                struct pipe_resource *res, unsigned level,
                                unsigned first_slice, unsigned num_slices,
                                unsigned resolve_flags)
{
   struct ilo_texture *tex = ilo_texture(res);
   const unsigned any_reader =
      ILO_TEXTURE_RENDER_READ |
      ILO_TEXTURE_BLT_READ |
      ILO_TEXTURE_CPU_READ;
   const unsigned other_writers =
      ILO_TEXTURE_BLT_WRITE |
      ILO_TEXTURE_CPU_WRITE;
   unsigned i;

   assert(tex->base.target != PIPE_BUFFER &&
          ilo_texture_can_enable_hiz(tex, level, first_slice, num_slices));

   if (resolve_flags & ILO_TEXTURE_RENDER_WRITE) {
      /*
       * When ILO_TEXTURE_RENDER_WRITE is set, there can be no reader.  We
       * need to perform a HiZ Buffer Resolve in case the resource was
       * previously written by another writer, unless this is a clear.
       *
       * When slices have different clear values, we perform a Depth Buffer
       * Resolve on all slices not sharing the clear value of the first slice.
       * After resolving, those slices do not use 3DSTATE_CLEAR_PARAMS and can
       * be made to have the same clear value as the first slice does.  This
       * way,
       *
       *  - 3DSTATE_CLEAR_PARAMS can be set to the clear value of any slice
       *  - we will not resolve unnecessarily next time this function is
       *    called
       *
       * Since slice clear value is the value the slice is cleared to when
       * ILO_TEXTURE_CLEAR is set, the bit needs to be unset.
       */
      assert(!(resolve_flags & (other_writers | any_reader)));

      if (!(resolve_flags & ILO_TEXTURE_CLEAR)) {
         bool set_clear_value = false;
         uint32_t first_clear_value;

         for (i = 0; i < num_slices; i++) {
            const struct ilo_texture_slice *slice =
               ilo_texture_get_slice(tex, level, first_slice + i);

            if (slice->flags & other_writers) {
               ilo_blitter_rectlist_resolve_hiz(ilo->blitter,
                     res, level, first_slice + i);
            }
            else if (i == 0) {
               first_clear_value = slice->clear_value;
            }
            else if (slice->clear_value != first_clear_value &&
                     (slice->flags & ILO_TEXTURE_RENDER_WRITE)) {
               ilo_blitter_rectlist_resolve_z(ilo->blitter,
                     res, level, first_slice + i);
               set_clear_value = true;
            }
         }

         if (set_clear_value) {
            /* ILO_TEXTURE_CLEAR will be cleared later */
            ilo_texture_set_slice_clear_value(tex, level,
                  first_slice, num_slices, first_clear_value);
         }
      }
   }
   else if ((resolve_flags & any_reader) ||
            ((resolve_flags & other_writers) &&
             !(resolve_flags & ILO_TEXTURE_CLEAR))) {
      /*
       * When there is at least a reader or writer, we need to perform a
       * Depth Buffer Resolve in case the resource was previously written
       * by ILO_TEXTURE_RENDER_WRITE.
       */
      for (i = 0; i < num_slices; i++) {
         const struct ilo_texture_slice *slice =
            ilo_texture_get_slice(tex, level, first_slice + i);

         if (slice->flags & ILO_TEXTURE_RENDER_WRITE) {
            ilo_blitter_rectlist_resolve_z(ilo->blitter,
                  &tex->base, level, first_slice + i);
         }
      }
   }
}

/**
 * Initialize blit-related functions.
 */
void
ilo_init_blit_functions(struct ilo_context *ilo)
{
   ilo->base.resource_copy_region = ilo_resource_copy_region;
   ilo->base.blit = ilo_blit;
   ilo->base.flush_resource = ilo_flush_resource;

   ilo->base.clear = ilo_clear;
   ilo->base.clear_render_target = ilo_clear_render_target;
   ilo->base.clear_depth_stencil = ilo_clear_depth_stencil;
}
