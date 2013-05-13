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

#include "util/u_blitter.h"
#include "util/u_clear.h"
#include "util/u_pack_color.h"
#include "util/u_surface.h"
#include "intel_reg.h"

#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_resource.h"
#include "ilo_screen.h"
#include "ilo_blit.h"

static bool
blitter_xy_color_blt(struct pipe_context *pipe,
                     struct pipe_resource *res,
                     int16_t x1, int16_t y1,
                     int16_t x2, int16_t y2,
                     uint32_t color)
{
   const int cmd_len = 6;
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_texture *tex = ilo_texture(res);
   uint32_t cmd, br13;
   int cpp, stride;
   struct intel_bo *bo_check[2];

   /* how to support Y-tiling? */
   if (tex->tiling == INTEL_TILING_Y)
      return false;

   /* nothing to clear */
   if (x1 >= x2 || y1 >= y2)
      return true;

   cmd = XY_COLOR_BLT_CMD | (cmd_len - 2);
   br13 = 0xf0 << 16;

   cpp = util_format_get_blocksize(tex->base.format);
   switch (cpp) {
   case 4:
      cmd |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
      br13 |= BR13_8888;
      break;
   case 2:
      br13 |= BR13_565;
      break;
   case 1:
      br13 |= BR13_8;
      break;
   default:
      return false;
      break;
   }

   stride = tex->bo_stride;
   if (tex->tiling != INTEL_TILING_NONE) {
      assert(tex->tiling == INTEL_TILING_X);

      cmd |= XY_DST_TILED;
      /* in dwords */
      stride /= 4;
   }

   /* make room if necessary */
   bo_check[0] = ilo->cp->bo;
   bo_check[1] = tex->bo;
   if (ilo->winsys->check_aperture_space(ilo->winsys, bo_check, 2))
      ilo_cp_flush(ilo->cp);

   ilo_cp_set_ring(ilo->cp, ILO_CP_RING_BLT);

   ilo_cp_begin(ilo->cp, cmd_len);
   ilo_cp_write(ilo->cp, cmd);
   ilo_cp_write(ilo->cp, br13 | stride);
   ilo_cp_write(ilo->cp, (y1 << 16) | x1);
   ilo_cp_write(ilo->cp, (y2 << 16) | x2);
   ilo_cp_write_bo(ilo->cp, 0, tex->bo,
                   INTEL_DOMAIN_RENDER,
                   INTEL_DOMAIN_RENDER);
   ilo_cp_write(ilo->cp, color);
   ilo_cp_end(ilo->cp);

   return true;
}

enum ilo_blitter_op {
   ILO_BLITTER_CLEAR,
   ILO_BLITTER_CLEAR_SURFACE,
   ILO_BLITTER_BLIT,
};

static void
ilo_blitter_begin(struct ilo_context *ilo, enum ilo_blitter_op op)
{
   /* as documented in util/u_blitter.h */
   util_blitter_save_vertex_buffer_slot(ilo->blitter,
         ilo->vertex_buffers.buffers);
   util_blitter_save_vertex_elements(ilo->blitter, ilo->vertex_elements);
   util_blitter_save_vertex_shader(ilo->blitter, ilo->vs);
   util_blitter_save_geometry_shader(ilo->blitter, ilo->gs);
   util_blitter_save_so_targets(ilo->blitter,
         ilo->stream_output_targets.num_targets,
         ilo->stream_output_targets.targets);

   util_blitter_save_fragment_shader(ilo->blitter, ilo->fs);
   util_blitter_save_depth_stencil_alpha(ilo->blitter,
         ilo->depth_stencil_alpha);
   util_blitter_save_blend(ilo->blitter, ilo->blend);

   /* undocumented? */
   util_blitter_save_viewport(ilo->blitter, &ilo->viewport);
   util_blitter_save_stencil_ref(ilo->blitter, &ilo->stencil_ref);
   util_blitter_save_sample_mask(ilo->blitter, ilo->sample_mask);

   switch (op) {
   case ILO_BLITTER_CLEAR:
      util_blitter_save_rasterizer(ilo->blitter, ilo->rasterizer);
      break;
   case ILO_BLITTER_CLEAR_SURFACE:
      util_blitter_save_framebuffer(ilo->blitter, &ilo->framebuffer);
      break;
   case ILO_BLITTER_BLIT:
      util_blitter_save_rasterizer(ilo->blitter, ilo->rasterizer);
      util_blitter_save_framebuffer(ilo->blitter, &ilo->framebuffer);

      util_blitter_save_fragment_sampler_states(ilo->blitter,
            ilo->samplers[PIPE_SHADER_FRAGMENT].num_samplers,
            (void **) ilo->samplers[PIPE_SHADER_FRAGMENT].samplers);

      util_blitter_save_fragment_sampler_views(ilo->blitter,
            ilo->sampler_views[PIPE_SHADER_FRAGMENT].num_views,
            ilo->sampler_views[PIPE_SHADER_FRAGMENT].views);

      /* disable render condition? */
      break;
   default:
      break;
   }
}

static void
ilo_blitter_end(struct ilo_context *ilo)
{
}

static void
ilo_clear(struct pipe_context *pipe,
          unsigned buffers,
          const union pipe_color_union *color,
          double depth,
          unsigned stencil)
{
   struct ilo_context *ilo = ilo_context(pipe);

   /* TODO we should pause/resume some queries */
   ilo_blitter_begin(ilo, ILO_BLITTER_CLEAR);

   util_blitter_clear(ilo->blitter,
         ilo->framebuffer.width, ilo->framebuffer.height,
         ilo->framebuffer.nr_cbufs, buffers,
         (ilo->framebuffer.nr_cbufs) ? ilo->framebuffer.cbufs[0]->format :
                                        PIPE_FORMAT_NONE,
         color, depth, stencil);

   ilo_blitter_end(ilo);
}

static void
ilo_clear_render_target(struct pipe_context *pipe,
                        struct pipe_surface *dst,
                        const union pipe_color_union *color,
                        unsigned dstx, unsigned dsty,
                        unsigned width, unsigned height)
{
   struct ilo_context *ilo = ilo_context(pipe);
   union util_color packed;

   if (!width || !height || dstx >= dst->width || dsty >= dst->height)
      return;

   if (dstx + width > dst->width)
      width = dst->width - dstx;
   if (dsty + height > dst->height)
      height = dst->height - dsty;

   util_pack_color(color->f, dst->format, &packed);

   /* try HW blit first */
   if (blitter_xy_color_blt(pipe, dst->texture,
                            dstx, dsty,
                            dstx + width, dsty + height,
                            packed.ui))
      return;

   ilo_blitter_begin(ilo, ILO_BLITTER_CLEAR_SURFACE);
   util_blitter_clear_render_target(ilo->blitter,
         dst, color, dstx, dsty, width, height);
   ilo_blitter_end(ilo);
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

   /*
    * The PRM claims that HW blit supports Y-tiling since GEN6, but it does
    * not tell us how to program it.  Since depth buffers are always Y-tiled,
    * HW blit will not work.
    */
   ilo_blitter_begin(ilo, ILO_BLITTER_CLEAR_SURFACE);
   util_blitter_clear_depth_stencil(ilo->blitter,
         dst, clear_flags, depth, stencil, dstx, dsty, width, height);
   ilo_blitter_end(ilo);
}

static void
ilo_blit(struct pipe_context *pipe, const struct pipe_blit_info *info)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct pipe_blit_info skip_stencil;

   if (util_try_blit_via_copy_region(pipe, info))
      return;

   if (!util_blitter_is_blit_supported(ilo->blitter, info)) {
      /* try without stencil */
      if (info->mask & PIPE_MASK_S) {
         skip_stencil = *info;
         skip_stencil.mask = info->mask & ~PIPE_MASK_S;

         if (util_blitter_is_blit_supported(ilo->blitter, &skip_stencil))
            info = &skip_stencil;
      }

      if (info == &skip_stencil) {
         ilo_warn("ignore stencil buffer blitting\n");
      }
      else {
         ilo_warn("failed to blit with the generic blitter\n");
         return;
      }
   }

   ilo_blitter_begin(ilo, ILO_BLITTER_BLIT);
   util_blitter_blit(ilo->blitter, info);
   ilo_blitter_end(ilo);
}

/**
 * Initialize blit-related functions.
 */
void
ilo_init_blit_functions(struct ilo_context *ilo)
{
   ilo->base.resource_copy_region = util_resource_copy_region;
   ilo->base.blit = ilo_blit;

   ilo->base.clear = ilo_clear;
   ilo->base.clear_render_target = ilo_clear_render_target;
   ilo->base.clear_depth_stencil = ilo_clear_depth_stencil;
}
