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

/*
 * From the Sandy Bridge PRM, volume 1 part 5, page 7:
 *
 *     "The BLT engine is capable of transferring very large quantities of
 *      graphics data. Any graphics data read from and written to the
 *      destination is permitted to represent a number of pixels that occupies
 *      up to 65,536 scan lines and up to 32,768 bytes per scan line at the
 *      destination. The maximum number of pixels that may be represented per
 *      scan line's worth of graphics data depends on the color depth."
 */
static const int gen6_max_bytes_per_scanline = 32768;
static const int gen6_max_scanlines = 65536;

static void
ilo_blit_own_blt_ring(struct ilo_context *ilo)
{
   ilo_cp_set_ring(ilo->cp, ILO_CP_RING_BLT);
   ilo_cp_set_owner(ilo->cp, NULL, 0);
}

static void
gen6_MI_FLUSH_DW(struct ilo_context *ilo)
{
   const uint8_t cmd_len = 4;
   struct ilo_cp *cp = ilo->cp;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, MI_FLUSH_DW | (cmd_len - 2));
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen6_MI_LOAD_REGISTER_IMM(struct ilo_context *ilo, uint32_t reg, uint32_t val)
{
   const uint8_t cmd_len = 3;
   struct ilo_cp *cp = ilo->cp;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, MI_LOAD_REGISTER_IMM | (cmd_len - 2));
   ilo_cp_write(cp, reg);
   ilo_cp_write(cp, val);
   ilo_cp_end(cp);
}

static void
gen6_XY_COLOR_BLT(struct ilo_context *ilo, struct intel_bo *dst_bo,
                  uint32_t dst_offset, int16_t dst_pitch,
                  enum intel_tiling_mode dst_tiling,
                  int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                  uint32_t color,
                  uint8_t rop, int cpp, bool write_alpha)
{
   const uint8_t cmd_len = 6;
   struct ilo_cp *cp = ilo->cp;
   int dst_align, dst_pitch_shift;
   uint32_t dw0, dw1;

   dw0 = XY_COLOR_BLT_CMD | (cmd_len - 2);

   if (dst_tiling == INTEL_TILING_NONE) {
      dst_align = 4;
      dst_pitch_shift = 0;
   }
   else {
      dw0 |= XY_DST_TILED;

      dst_align = (dst_tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      dst_pitch_shift = 2;
   }

   assert(cpp == 4 || cpp == 2 || cpp == 1);
   assert((x2 - x1) * cpp < gen6_max_bytes_per_scanline);
   assert(y2 - y1 < gen6_max_scanlines);
   assert(dst_offset % dst_align == 0 && dst_pitch % dst_align == 0);

   dw1 = rop << 16 |
         dst_pitch >> dst_pitch_shift;

   switch (cpp) {
   case 4:
      dw0 |= XY_BLT_WRITE_RGB;
      if (write_alpha)
         dw0 |= XY_BLT_WRITE_ALPHA;

      dw1 |= BR13_8888;
      break;
   case 2:
      dw1 |= BR13_565;
      break;
   case 1:
      dw1 |= BR13_8;
      break;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, dw0);
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, y1 << 16 | x1);
   ilo_cp_write(cp, y2 << 16 | x2);
   ilo_cp_write_bo(cp, dst_offset, dst_bo,
                   INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_write(cp, color);
   ilo_cp_end(cp);
}

static void
gen6_SRC_COPY_BLT(struct ilo_context *ilo, struct intel_bo *dst_bo,
                  uint32_t dst_offset, int16_t dst_pitch,
                  uint16_t width, uint16_t height,
                  struct intel_bo *src_bo,
                  uint32_t src_offset, int16_t src_pitch,
                  uint8_t rop, int cpp, bool write_alpha, bool dir_rtl)
{
   const uint8_t cmd_len = 6;
   struct ilo_cp *cp = ilo->cp;
   uint32_t dw0, dw1;

   assert(cpp == 4 || cpp == 2 || cpp == 1);
   assert(width < gen6_max_bytes_per_scanline);
   assert(height < gen6_max_scanlines);
   /* offsets are naturally aligned and pitches are dword-aligned */
   assert(dst_offset % cpp == 0 && dst_pitch % 4 == 0);
   assert(src_offset % cpp == 0 && src_pitch % 4 == 0);

#ifndef SRC_COPY_BLT_CMD
#define SRC_COPY_BLT_CMD (CMD_2D | (0x43 << 22))
#endif
   dw0 = SRC_COPY_BLT_CMD | (cmd_len - 2);
   dw1 = rop << 16 | dst_pitch;

   if (dir_rtl)
      dw1 |= 1 << 30;

   switch (cpp) {
   case 4:
      dw0 |= XY_BLT_WRITE_RGB;
      if (write_alpha)
         dw0 |= XY_BLT_WRITE_ALPHA;

      dw1 |= BR13_8888;
      break;
   case 2:
      dw1 |= BR13_565;
      break;
   case 1:
      dw1 |= BR13_8;
      break;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, dw0);
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, height << 16 | width);
   ilo_cp_write_bo(cp, dst_offset, dst_bo, INTEL_DOMAIN_RENDER,
                                           INTEL_DOMAIN_RENDER);
   ilo_cp_write(cp, src_pitch);
   ilo_cp_write_bo(cp, src_offset, src_bo, INTEL_DOMAIN_RENDER, 0);
   ilo_cp_end(cp);
}

static void
gen6_XY_SRC_COPY_BLT(struct ilo_context *ilo, struct intel_bo *dst_bo,
                     uint32_t dst_offset, int16_t dst_pitch,
                     enum intel_tiling_mode dst_tiling,
                     int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                     struct intel_bo *src_bo,
                     uint32_t src_offset, int16_t src_pitch,
                     enum intel_tiling_mode src_tiling,
                     int16_t src_x, int16_t src_y,
                     uint8_t rop, int cpp, bool write_alpha)
{
   const uint8_t cmd_len = 8;
   struct ilo_cp *cp = ilo->cp;
   int dst_align, dst_pitch_shift;
   int src_align, src_pitch_shift;
   uint32_t dw0, dw1;

   dw0 = XY_SRC_COPY_BLT_CMD | (cmd_len - 2);

   if (dst_tiling == INTEL_TILING_NONE) {
      dst_align = 4;
      dst_pitch_shift = 0;
   }
   else {
      dw0 |= XY_DST_TILED;

      dst_align = (dst_tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      dst_pitch_shift = 2;
   }

   if (src_tiling == INTEL_TILING_NONE) {
      src_align = 4;
      src_pitch_shift = 0;
   }
   else {
      dw0 |= XY_SRC_TILED;

      src_align = (src_tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      src_pitch_shift = 2;
   }

   assert(cpp == 4 || cpp == 2 || cpp == 1);
   assert((x2 - x1) * cpp < gen6_max_bytes_per_scanline);
   assert(y2 - y1 < gen6_max_scanlines);
   assert(dst_offset % dst_align == 0 && dst_pitch % dst_align == 0);
   assert(src_offset % src_align == 0 && src_pitch % src_align == 0);

   dw1 = rop << 16 |
         dst_pitch >> dst_pitch_shift;

   switch (cpp) {
   case 4:
      dw0 |= XY_BLT_WRITE_RGB;
      if (write_alpha)
         dw0 |= XY_BLT_WRITE_ALPHA;

      dw1 |= BR13_8888;
      break;
   case 2:
      dw1 |= BR13_565;
      break;
   case 1:
      dw1 |= BR13_8;
      break;
   }

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, dw0);
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, y1 << 16 | x1);
   ilo_cp_write(cp, y2 << 16 | x2);
   ilo_cp_write_bo(cp, dst_offset, dst_bo, INTEL_DOMAIN_RENDER,
                                           INTEL_DOMAIN_RENDER);
   ilo_cp_write(cp, src_y << 16 | src_x);
   ilo_cp_write(cp, src_pitch >> src_pitch_shift);
   ilo_cp_write_bo(cp, src_offset, src_bo, INTEL_DOMAIN_RENDER, 0);
   ilo_cp_end(cp);
}

static bool
tex_copy_region(struct ilo_context *ilo,
                struct ilo_texture *dst,
                unsigned dst_level,
                unsigned dst_x, unsigned dst_y, unsigned dst_z,
                struct ilo_texture *src,
                unsigned src_level,
                const struct pipe_box *src_box)
{
   const struct util_format_description *desc =
      util_format_description(dst->bo_format);
   const unsigned max_extent = 32767; /* INT16_MAX */
   const uint8_t rop = 0xcc; /* SRCCOPY */
   struct intel_bo *aper_check[3];
   uint32_t swctrl;
   int cpp, xscale, slice;

   /* no W-tiling support */
   if (dst->separate_s8 || src->separate_s8)
      return false;

   if (dst->bo_stride > max_extent || src->bo_stride > max_extent)
      return false;

   cpp = desc->block.bits / 8;
   xscale = 1;

   /* accommodate for larger cpp */
   if (cpp > 4) {
      if (cpp % 2 == 1)
         return false;

      cpp = (cpp % 4 == 0) ? 4 : 2;
      xscale = (desc->block.bits / 8) / cpp;
   }

   ilo_blit_own_blt_ring(ilo);

   /* make room if necessary */
   aper_check[0] = ilo->cp->bo;
   aper_check[1] = dst->bo;
   aper_check[2] = src->bo;
   if (intel_winsys_check_aperture_space(ilo->winsys, aper_check, 3))
      ilo_cp_flush(ilo->cp);

   swctrl = 0x0;

   if (dst->tiling == INTEL_TILING_Y) {
      swctrl |= BCS_SWCTRL_DST_Y << 16 |
                BCS_SWCTRL_DST_Y;
   }

   if (src->tiling == INTEL_TILING_Y) {
      swctrl |= BCS_SWCTRL_SRC_Y << 16 |
                BCS_SWCTRL_SRC_Y;
   }

   if (swctrl) {
      /*
       * Most clients expect BLT engine to be stateless.  If we have to set
       * BCS_SWCTRL to a non-default value, we have to set it back in the same
       * batch buffer.
       */
      if (ilo_cp_space(ilo->cp) < (4 + 3) * 2 + src_box->depth * 8)
         ilo_cp_flush(ilo->cp);

      ilo_cp_assert_no_implicit_flush(ilo->cp, true);

      /*
       * From the Ivy Bridge PRM, volume 1 part 4, page 133:
       *
       *     "SW is required to flush the HW before changing the polarity of
       *      this bit (Tile Y Destination/Source)."
       */
      gen6_MI_FLUSH_DW(ilo);
      gen6_MI_LOAD_REGISTER_IMM(ilo, BCS_SWCTRL, swctrl);

      swctrl &= ~(BCS_SWCTRL_DST_Y | BCS_SWCTRL_SRC_Y);
   }

   for (slice = 0; slice < src_box->depth; slice++) {
      const struct ilo_texture_slice *dst_slice =
         &dst->slice_offsets[dst_level][dst_z + slice];
      const struct ilo_texture_slice *src_slice =
         &src->slice_offsets[src_level][src_box->z + slice];
      unsigned x1, y1, x2, y2, src_x, src_y;

      x1 = (dst_slice->x + dst_x) * xscale;
      y1 = dst_slice->y + dst_y;
      x2 = (x1 + src_box->width) * xscale;
      y2 = y1 + src_box->height;
      src_x = (src_slice->x + src_box->x) * xscale;
      src_y = src_slice->y + src_box->y;

      x1 /= desc->block.width;
      y1 /= desc->block.height;
      x2 = (x2 + desc->block.width - 1) / desc->block.width;
      y2 = (y2 + desc->block.height - 1) / desc->block.height;
      src_x /= desc->block.width;
      src_y /= desc->block.height;

      if (x2 > max_extent || y2 > max_extent ||
          src_x > max_extent || src_y > max_extent ||
          (x2 - x1) * cpp > gen6_max_bytes_per_scanline)
         break;

      gen6_XY_SRC_COPY_BLT(ilo,
            dst->bo, 0, dst->bo_stride, dst->tiling,
            x1, y1, x2, y2,
            src->bo, 0, src->bo_stride, src->tiling,
            src_x, src_y, rop, cpp, true);
   }

   if (swctrl) {
      gen6_MI_FLUSH_DW(ilo);
      gen6_MI_LOAD_REGISTER_IMM(ilo, BCS_SWCTRL, swctrl);

      ilo_cp_assert_no_implicit_flush(ilo->cp, false);
   }

   return (slice == src_box->depth);
}

static bool
buf_copy_region(struct ilo_context *ilo,
                struct ilo_buffer *dst, unsigned dst_offset,
                struct ilo_buffer *src, unsigned src_offset,
                unsigned size)
{
   const uint8_t rop = 0xcc; /* SRCCOPY */
   unsigned offset = 0;
   struct intel_bo *aper_check[3];

   ilo_blit_own_blt_ring(ilo);

   /* make room if necessary */
   aper_check[0] = ilo->cp->bo;
   aper_check[1] = dst->bo;
   aper_check[2] = src->bo;
   if (intel_winsys_check_aperture_space(ilo->winsys, aper_check, 3))
      ilo_cp_flush(ilo->cp);

   while (size) {
      unsigned width, height;
      int16_t pitch;

      width = size;
      height = 1;
      pitch = 0;

      if (width > gen6_max_bytes_per_scanline) {
         /* less than INT16_MAX and dword-aligned */
         pitch = 32764;

         width = pitch;
         height = size / width;
         if (height > gen6_max_scanlines)
            height = gen6_max_scanlines;
      }

      gen6_SRC_COPY_BLT(ilo,
            dst->bo, dst_offset + offset, pitch,
            width, height,
            src->bo, src_offset + offset, pitch,
            rop, 1, true, false);

      offset += pitch * height;
      size -= width * height;
   }

   return true;
}

static void
ilo_resource_copy_region(struct pipe_context *pipe,
                         struct pipe_resource *dst,
                         unsigned dst_level,
                         unsigned dstx, unsigned dsty, unsigned dstz,
                         struct pipe_resource *src,
                         unsigned src_level,
                         const struct pipe_box *src_box)
{
   bool success;

   if (dst->target != PIPE_BUFFER && src->target != PIPE_BUFFER) {
      success = tex_copy_region(ilo_context(pipe),
            ilo_texture(dst), dst_level, dstx, dsty, dstz,
            ilo_texture(src), src_level, src_box);
   }
   else if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      const unsigned dst_offset = dstx;
      const unsigned src_offset = src_box->x;
      const unsigned size = src_box->width;

      assert(dst_level == 0 && dsty == 0 && dstz == 0);
      assert(src_level == 0 &&
             src_box->y == 0 &&
             src_box->z == 0 &&
             src_box->height == 1 &&
             src_box->depth == 1);

      success = buf_copy_region(ilo_context(pipe),
            ilo_buffer(dst), dst_offset, ilo_buffer(src), src_offset, size);
   }
   else {
      success = false;
   }

   if (!success) {
      util_resource_copy_region(pipe, dst, dst_level,
            dstx, dsty, dstz, src, src_level, src_box);
   }
}

static bool
blitter_xy_color_blt(struct pipe_context *pipe,
                     struct pipe_resource *res,
                     int16_t x1, int16_t y1,
                     int16_t x2, int16_t y2,
                     uint32_t color)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_texture *tex = ilo_texture(res);
   const int cpp = util_format_get_blocksize(tex->bo_format);
   const uint8_t rop = 0xf0; /* PATCOPY */
   struct intel_bo *aper_check[2];

   /* how to support Y-tiling? */
   if (tex->tiling == INTEL_TILING_Y)
      return false;

   /* nothing to clear */
   if (x1 >= x2 || y1 >= y2)
      return true;

   ilo_blit_own_blt_ring(ilo);

   /* make room if necessary */
   aper_check[0] = ilo->cp->bo;
   aper_check[1] = tex->bo;
   if (intel_winsys_check_aperture_space(ilo->winsys, aper_check, 2))
      ilo_cp_flush(ilo->cp);

   gen6_XY_COLOR_BLT(ilo,
         tex->bo, 0, tex->bo_stride, tex->tiling,
         x1, y1, x2, y2, color, rop, cpp, true);

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
   util_blitter_save_vertex_buffer_slot(ilo->blitter, ilo->vb.states);
   util_blitter_save_vertex_elements(ilo->blitter, (void *) ilo->ve);
   util_blitter_save_vertex_shader(ilo->blitter, ilo->vs);
   util_blitter_save_geometry_shader(ilo->blitter, ilo->gs);
   util_blitter_save_so_targets(ilo->blitter, ilo->so.count, ilo->so.states);

   util_blitter_save_fragment_shader(ilo->blitter, ilo->fs);
   util_blitter_save_depth_stencil_alpha(ilo->blitter, (void *) ilo->dsa);
   util_blitter_save_blend(ilo->blitter, (void *) ilo->blend);

   /* undocumented? */
   util_blitter_save_viewport(ilo->blitter, &ilo->viewport.viewport0);
   util_blitter_save_stencil_ref(ilo->blitter, &ilo->stencil_ref);
   util_blitter_save_sample_mask(ilo->blitter, ilo->sample_mask);

   switch (op) {
   case ILO_BLITTER_CLEAR:
      util_blitter_save_rasterizer(ilo->blitter, (void *) ilo->rasterizer);
      break;
   case ILO_BLITTER_CLEAR_SURFACE:
      util_blitter_save_framebuffer(ilo->blitter, &ilo->fb.state);
      break;
   case ILO_BLITTER_BLIT:
      util_blitter_save_rasterizer(ilo->blitter, (void *) ilo->rasterizer);
      util_blitter_save_framebuffer(ilo->blitter, &ilo->fb.state);

      util_blitter_save_fragment_sampler_states(ilo->blitter,
            ilo->sampler[PIPE_SHADER_FRAGMENT].count,
            (void **) ilo->sampler[PIPE_SHADER_FRAGMENT].cso);

      util_blitter_save_fragment_sampler_views(ilo->blitter,
            ilo->view[PIPE_SHADER_FRAGMENT].count,
            ilo->view[PIPE_SHADER_FRAGMENT].states);

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
         ilo->fb.state.width, ilo->fb.state.height,
         buffers, color, depth, stencil);

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
   ilo->base.resource_copy_region = ilo_resource_copy_region;
   ilo->base.blit = ilo_blit;

   ilo->base.clear = ilo_clear;
   ilo->base.clear_render_target = ilo_clear_render_target;
   ilo->base.clear_depth_stencil = ilo_clear_depth_stencil;
}
