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

#include "genhw/genhw.h"
#include "util/u_pack_color.h"

#include "ilo_3d.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_blit.h"
#include "ilo_resource.h"
#include "ilo_blitter.h"

#define MI_FLUSH_DW           GEN_MI_CMD(MI_FLUSH_DW)
#define MI_LOAD_REGISTER_IMM  GEN_MI_CMD(MI_LOAD_REGISTER_IMM)
#define COLOR_BLT             GEN_BLITTER_CMD(COLOR_BLT)
#define XY_COLOR_BLT          GEN_BLITTER_CMD(XY_COLOR_BLT)
#define SRC_COPY_BLT          GEN_BLITTER_CMD(SRC_COPY_BLT)
#define XY_SRC_COPY_BLT       GEN_BLITTER_CMD(XY_SRC_COPY_BLT)

enum gen6_blt_mask {
   GEN6_BLT_MASK_8,
   GEN6_BLT_MASK_16,
   GEN6_BLT_MASK_32,
   GEN6_BLT_MASK_32_LO,
   GEN6_BLT_MASK_32_HI,
};

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
gen6_emit_MI_FLUSH_DW(struct ilo_dev_info *dev, struct ilo_cp *cp)
{
   const uint8_t cmd_len = 4;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, MI_FLUSH_DW | (cmd_len - 2));
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_write(cp, 0);
   ilo_cp_end(cp);
}

static void
gen6_emit_MI_LOAD_REGISTER_IMM(struct ilo_dev_info *dev,
                               uint32_t reg, uint32_t val,
                               struct ilo_cp *cp)
{
   const uint8_t cmd_len = 3;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, MI_LOAD_REGISTER_IMM | (cmd_len - 2));
   ilo_cp_write(cp, reg);
   ilo_cp_write(cp, val);
   ilo_cp_end(cp);
}

static uint32_t
gen6_translate_blt_value_mask(enum gen6_blt_mask value_mask)
{
   switch (value_mask) {
   case GEN6_BLT_MASK_8:  return GEN6_BLITTER_BR13_FORMAT_8;
   case GEN6_BLT_MASK_16: return GEN6_BLITTER_BR13_FORMAT_565;
   default:               return GEN6_BLITTER_BR13_FORMAT_8888;
   }
}

static uint32_t
gen6_translate_blt_write_mask(enum gen6_blt_mask write_mask)
{
   switch (write_mask) {
   case GEN6_BLT_MASK_32:    return GEN6_BLITTER_BR00_WRITE_RGB |
                                    GEN6_BLITTER_BR00_WRITE_A;
   case GEN6_BLT_MASK_32_LO: return GEN6_BLITTER_BR00_WRITE_RGB;
   case GEN6_BLT_MASK_32_HI: return GEN6_BLITTER_BR00_WRITE_A;
   default:                  return 0;
   }
}

static uint32_t
gen6_translate_blt_cpp(enum gen6_blt_mask mask)
{
   switch (mask) {
   case GEN6_BLT_MASK_8:  return 1;
   case GEN6_BLT_MASK_16: return 2;
   default:               return 4;
   }
}

static void
gen6_emit_COLOR_BLT(struct ilo_dev_info *dev,
                    struct intel_bo *dst_bo,
                    int16_t dst_pitch, uint32_t dst_offset,
                    uint16_t width, uint16_t height,
                    uint32_t pattern, uint8_t rop,
                    enum gen6_blt_mask value_mask,
                    enum gen6_blt_mask write_mask,
                    struct ilo_cp *cp)
{
   const uint8_t cmd_len = 5;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   uint32_t dw0, dw1;

   dw0 = COLOR_BLT |
         gen6_translate_blt_write_mask(write_mask) |
         (cmd_len - 2);

   assert(width < gen6_max_bytes_per_scanline);
   assert(height < gen6_max_scanlines);
   /* offsets are naturally aligned and pitches are dword-aligned */
   assert(dst_offset % cpp == 0 && dst_pitch % 4 == 0);

   dw1 = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
         gen6_translate_blt_value_mask(value_mask) |
         dst_pitch;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, dw0);
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, height << 16 | width);
   ilo_cp_write_bo(cp, dst_offset, dst_bo, INTEL_DOMAIN_RENDER,
                                           INTEL_DOMAIN_RENDER);
   ilo_cp_write(cp, pattern);
   ilo_cp_end(cp);
}

static void
gen6_emit_XY_COLOR_BLT(struct ilo_dev_info *dev,
                       struct intel_bo *dst_bo,
                       enum intel_tiling_mode dst_tiling,
                       int16_t dst_pitch, uint32_t dst_offset,
                       int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                       uint32_t pattern, uint8_t rop,
                       enum gen6_blt_mask value_mask,
                       enum gen6_blt_mask write_mask,
                       struct ilo_cp *cp)
{
   const uint8_t cmd_len = 6;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   int dst_align, dst_pitch_shift;
   uint32_t dw0, dw1;

   dw0 = XY_COLOR_BLT |
         gen6_translate_blt_write_mask(write_mask) |
         (cmd_len - 2);

   if (dst_tiling == INTEL_TILING_NONE) {
      dst_align = 4;
      dst_pitch_shift = 0;
   }
   else {
      dw0 |= GEN6_BLITTER_BR00_DST_TILED;

      dst_align = (dst_tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      dst_pitch_shift = 2;
   }

   assert((x2 - x1) * cpp < gen6_max_bytes_per_scanline);
   assert(y2 - y1 < gen6_max_scanlines);
   assert(dst_offset % dst_align == 0 && dst_pitch % dst_align == 0);

   dw1 = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
         gen6_translate_blt_value_mask(value_mask) |
         dst_pitch >> dst_pitch_shift;

   ilo_cp_begin(cp, cmd_len);
   ilo_cp_write(cp, dw0);
   ilo_cp_write(cp, dw1);
   ilo_cp_write(cp, y1 << 16 | x1);
   ilo_cp_write(cp, y2 << 16 | x2);
   ilo_cp_write_bo(cp, dst_offset, dst_bo,
                   INTEL_DOMAIN_RENDER, INTEL_DOMAIN_RENDER);
   ilo_cp_write(cp, pattern);
   ilo_cp_end(cp);
}

static void
gen6_emit_SRC_COPY_BLT(struct ilo_dev_info *dev,
                       struct intel_bo *dst_bo,
                       int16_t dst_pitch, uint32_t dst_offset,
                       uint16_t width, uint16_t height,
                       struct intel_bo *src_bo,
                       int16_t src_pitch, uint32_t src_offset,
                       bool dir_rtl, uint8_t rop,
                       enum gen6_blt_mask value_mask,
                       enum gen6_blt_mask write_mask,
                       struct ilo_cp *cp)
{
   const uint8_t cmd_len = 6;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   uint32_t dw0, dw1;

   dw0 = SRC_COPY_BLT |
         gen6_translate_blt_write_mask(write_mask) |
         (cmd_len - 2);

   assert(width < gen6_max_bytes_per_scanline);
   assert(height < gen6_max_scanlines);
   /* offsets are naturally aligned and pitches are dword-aligned */
   assert(dst_offset % cpp == 0 && dst_pitch % 4 == 0);
   assert(src_offset % cpp == 0 && src_pitch % 4 == 0);

   dw1 = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
         gen6_translate_blt_value_mask(value_mask) |
         dst_pitch;

   if (dir_rtl)
      dw1 |= GEN6_BLITTER_BR13_DIR_RTL;

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
gen6_emit_XY_SRC_COPY_BLT(struct ilo_dev_info *dev,
                          struct intel_bo *dst_bo,
                          enum intel_tiling_mode dst_tiling,
                          int16_t dst_pitch, uint32_t dst_offset,
                          int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                          struct intel_bo *src_bo,
                          enum intel_tiling_mode src_tiling,
                          int16_t src_pitch, uint32_t src_offset,
                          int16_t src_x, int16_t src_y, uint8_t rop,
                          enum gen6_blt_mask value_mask,
                          enum gen6_blt_mask write_mask,
                          struct ilo_cp *cp)
{
   const uint8_t cmd_len = 8;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   int dst_align, dst_pitch_shift;
   int src_align, src_pitch_shift;
   uint32_t dw0, dw1;

   dw0 = XY_SRC_COPY_BLT |
         gen6_translate_blt_write_mask(write_mask) |
         (cmd_len - 2);

   if (dst_tiling == INTEL_TILING_NONE) {
      dst_align = 4;
      dst_pitch_shift = 0;
   }
   else {
      dw0 |= GEN6_BLITTER_BR00_DST_TILED;

      dst_align = (dst_tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      dst_pitch_shift = 2;
   }

   if (src_tiling == INTEL_TILING_NONE) {
      src_align = 4;
      src_pitch_shift = 0;
   }
   else {
      dw0 |= GEN6_BLITTER_BR00_SRC_TILED;

      src_align = (src_tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      src_pitch_shift = 2;
   }

   assert((x2 - x1) * cpp < gen6_max_bytes_per_scanline);
   assert(y2 - y1 < gen6_max_scanlines);
   assert(dst_offset % dst_align == 0 && dst_pitch % dst_align == 0);
   assert(src_offset % src_align == 0 && src_pitch % src_align == 0);

   dw1 = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
         gen6_translate_blt_value_mask(value_mask) |
         dst_pitch >> dst_pitch_shift;

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

static uint32_t
ilo_blitter_blt_begin(struct ilo_blitter *blitter, int max_cmd_size,
                      struct intel_bo *dst, enum intel_tiling_mode dst_tiling,
                      struct intel_bo *src, enum intel_tiling_mode src_tiling)
{
   struct ilo_context *ilo = blitter->ilo;
   struct intel_bo *aper_check[3];
   int count;
   uint32_t swctrl;

   /* change ring */
   ilo_cp_set_ring(ilo->cp, INTEL_RING_BLT);
   ilo_cp_set_owner(ilo->cp, NULL, 0);

   /* check aperture space */
   aper_check[0] = ilo->cp->bo;
   aper_check[1] = dst;
   count = 2;

   if (src) {
      aper_check[2] = src;
      count++;
   }

   if (!intel_winsys_can_submit_bo(ilo->winsys, aper_check, count))
      ilo_cp_flush(ilo->cp, "out of aperture");

   /* set BCS_SWCTRL */
   swctrl = 0x0;

   if (dst_tiling == INTEL_TILING_Y) {
      swctrl |= GEN6_REG_BCS_SWCTRL_DST_TILING_Y << 16 |
                GEN6_REG_BCS_SWCTRL_DST_TILING_Y;
   }

   if (src && src_tiling == INTEL_TILING_Y) {
      swctrl |= GEN6_REG_BCS_SWCTRL_SRC_TILING_Y << 16 |
                GEN6_REG_BCS_SWCTRL_SRC_TILING_Y;
   }

   if (swctrl) {
      /*
       * Most clients expect BLT engine to be stateless.  If we have to set
       * BCS_SWCTRL to a non-default value, we have to set it back in the same
       * batch buffer.
       */
      if (ilo_cp_space(ilo->cp) < (4 + 3) * 2 + max_cmd_size)
         ilo_cp_flush(ilo->cp, "out of space");

      ilo_cp_assert_no_implicit_flush(ilo->cp, true);

      /*
       * From the Ivy Bridge PRM, volume 1 part 4, page 133:
       *
       *     "SW is required to flush the HW before changing the polarity of
       *      this bit (Tile Y Destination/Source)."
       */
      gen6_emit_MI_FLUSH_DW(ilo->dev, ilo->cp);
      gen6_emit_MI_LOAD_REGISTER_IMM(ilo->dev,
            GEN6_REG_BCS_SWCTRL, swctrl, ilo->cp);

      swctrl &= ~(GEN6_REG_BCS_SWCTRL_DST_TILING_Y |
                  GEN6_REG_BCS_SWCTRL_SRC_TILING_Y);
   }

   return swctrl;
}

static void
ilo_blitter_blt_end(struct ilo_blitter *blitter, uint32_t swctrl)
{
   struct ilo_context *ilo = blitter->ilo;

   /* set BCS_SWCTRL back */
   if (swctrl) {
      gen6_emit_MI_FLUSH_DW(ilo->dev, ilo->cp);
      gen6_emit_MI_LOAD_REGISTER_IMM(ilo->dev, GEN6_REG_BCS_SWCTRL, swctrl, ilo->cp);

      ilo_cp_assert_no_implicit_flush(ilo->cp, false);
   }
}

static bool
buf_clear_region(struct ilo_blitter *blitter,
                 struct ilo_buffer *dst,
                 unsigned dst_offset, unsigned dst_size,
                 uint32_t val,
                 enum gen6_blt_mask value_mask,
                 enum gen6_blt_mask write_mask)
{
   const uint8_t rop = 0xf0; /* PATCOPY */
   const int cpp = gen6_translate_blt_cpp(value_mask);
   struct ilo_context *ilo = blitter->ilo;
   unsigned offset = 0;

   if (dst_offset % cpp || dst_size % cpp)
      return false;

   ilo_blitter_blt_begin(blitter, 0,
         dst->bo, INTEL_TILING_NONE, NULL, INTEL_TILING_NONE);

   while (dst_size) {
      unsigned width, height;
      int16_t pitch;

      width = dst_size;
      height = 1;
      pitch = 0;

      if (width > gen6_max_bytes_per_scanline) {
         /* less than INT16_MAX and dword-aligned */
         pitch = 32764;

         width = pitch;
         height = dst_size / width;
         if (height > gen6_max_scanlines)
            height = gen6_max_scanlines;
      }

      gen6_emit_COLOR_BLT(ilo->dev, dst->bo, pitch, dst_offset + offset,
            width, height, val, rop, value_mask, write_mask, ilo->cp);

      offset += pitch * height;
      dst_size -= width * height;
   }

   ilo_blitter_blt_end(blitter, 0);

   return true;
}

static bool
buf_copy_region(struct ilo_blitter *blitter,
                struct ilo_buffer *dst, unsigned dst_offset,
                struct ilo_buffer *src, unsigned src_offset,
                unsigned size)
{
   const uint8_t rop = 0xcc; /* SRCCOPY */
   struct ilo_context *ilo = blitter->ilo;
   unsigned offset = 0;

   ilo_blitter_blt_begin(blitter, 0,
         dst->bo, INTEL_TILING_NONE, src->bo, INTEL_TILING_NONE);

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

      gen6_emit_SRC_COPY_BLT(ilo->dev,
            dst->bo, pitch, dst_offset + offset,
            width, height,
            src->bo, pitch, src_offset + offset,
            false, rop, GEN6_BLT_MASK_8, GEN6_BLT_MASK_8,
            ilo->cp);

      offset += pitch * height;
      size -= width * height;
   }

   ilo_blitter_blt_end(blitter, 0);

   return true;
}

static bool
tex_clear_region(struct ilo_blitter *blitter,
                 struct ilo_texture *dst, unsigned dst_level,
                 const struct pipe_box *dst_box,
                 uint32_t val,
                 enum gen6_blt_mask value_mask,
                 enum gen6_blt_mask write_mask)
{
   const int cpp = gen6_translate_blt_cpp(value_mask);
   const unsigned max_extent = 32767; /* INT16_MAX */
   const uint8_t rop = 0xf0; /* PATCOPY */
   struct ilo_context *ilo = blitter->ilo;
   uint32_t swctrl;
   int slice;

   /* no W-tiling support */
   if (dst->separate_s8)
      return false;

   if (dst->bo_stride > max_extent)
      return false;

   swctrl = ilo_blitter_blt_begin(blitter, dst_box->depth * 6,
         dst->bo, dst->tiling, NULL, INTEL_TILING_NONE);

   for (slice = 0; slice < dst_box->depth; slice++) {
      const struct ilo_texture_slice *dst_slice =
         ilo_texture_get_slice(dst, dst_level, dst_box->z + slice);
      unsigned x1, y1, x2, y2;

      x1 = dst_slice->x + dst_box->x;
      y1 = dst_slice->y + dst_box->y;
      x2 = x1 + dst_box->width;
      y2 = y1 + dst_box->height;

      if (x2 > max_extent || y2 > max_extent ||
          (x2 - x1) * cpp > gen6_max_bytes_per_scanline)
         break;

      gen6_emit_XY_COLOR_BLT(ilo->dev,
            dst->bo, dst->tiling, dst->bo_stride, 0,
            x1, y1, x2, y2, val, rop, value_mask, write_mask,
            ilo->cp);
   }

   ilo_blitter_blt_end(blitter, swctrl);

   return (slice == dst_box->depth);
}

static bool
tex_copy_region(struct ilo_blitter *blitter,
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
   struct ilo_context *ilo = blitter->ilo;
   enum gen6_blt_mask mask;
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

   switch (cpp) {
   case 1:
      mask = GEN6_BLT_MASK_8;
      break;
   case 2:
      mask = GEN6_BLT_MASK_16;
      break;
   case 4:
      mask = GEN6_BLT_MASK_32;
      break;
   default:
      return false;
      break;
   }

   swctrl = ilo_blitter_blt_begin(blitter, src_box->depth * 8,
         dst->bo, dst->tiling, src->bo, src->tiling);

   for (slice = 0; slice < src_box->depth; slice++) {
      const struct ilo_texture_slice *dst_slice =
         ilo_texture_get_slice(dst, dst_level, dst_z + slice);
      const struct ilo_texture_slice *src_slice =
         ilo_texture_get_slice(src, src_level, src_box->z + slice);
      unsigned x1, y1, x2, y2, src_x, src_y;

      x1 = (dst_slice->x + dst_x) * xscale;
      y1 = dst_slice->y + dst_y;
      x2 = (x1 + src_box->width) * xscale;
      y2 = y1 + src_box->height;
      src_x = (src_slice->x + src_box->x) * xscale;
      src_y = src_slice->y + src_box->y;

      /* in blocks */
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

      gen6_emit_XY_SRC_COPY_BLT(ilo->dev,
            dst->bo, dst->tiling, dst->bo_stride, 0,
            x1, y1, x2, y2,
            src->bo, src->tiling, src->bo_stride, 0,
            src_x, src_y, rop, mask, mask,
            ilo->cp);
   }

   ilo_blitter_blt_end(blitter, swctrl);

   return (slice == src_box->depth);
}

bool
ilo_blitter_blt_copy_resource(struct ilo_blitter *blitter,
                              struct pipe_resource *dst, unsigned dst_level,
                              unsigned dst_x, unsigned dst_y, unsigned dst_z,
                              struct pipe_resource *src, unsigned src_level,
                              const struct pipe_box *src_box)
{
   bool success;

   ilo_blit_resolve_slices(blitter->ilo, src, src_level,
         src_box->z, src_box->depth, ILO_TEXTURE_BLT_READ);
   ilo_blit_resolve_slices(blitter->ilo, dst, dst_level,
         dst_z, src_box->depth, ILO_TEXTURE_BLT_WRITE);

   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      const unsigned dst_offset = dst_x;
      const unsigned src_offset = src_box->x;
      const unsigned size = src_box->width;

      assert(dst_level == 0 && dst_y == 0 && dst_z == 0);
      assert(src_level == 0 &&
             src_box->y == 0 &&
             src_box->z == 0 &&
             src_box->height == 1 &&
             src_box->depth == 1);

      success = buf_copy_region(blitter,
            ilo_buffer(dst), dst_offset, ilo_buffer(src), src_offset, size);
   }
   else if (dst->target != PIPE_BUFFER && src->target != PIPE_BUFFER) {
      success = tex_copy_region(blitter,
            ilo_texture(dst), dst_level, dst_x, dst_y, dst_z,
            ilo_texture(src), src_level, src_box);
   }
   else {
      success = false;
   }

   return success;
}

bool
ilo_blitter_blt_clear_rt(struct ilo_blitter *blitter,
                         struct pipe_surface *rt,
                         const union pipe_color_union *color,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height)
{
   const int cpp = util_format_get_blocksize(rt->format);
   enum gen6_blt_mask mask;
   union util_color packed;
   bool success;

   if (!ilo_3d_pass_render_condition(blitter->ilo))
      return true;

   switch (cpp) {
   case 1:
      mask = GEN6_BLT_MASK_8;
      break;
   case 2:
      mask = GEN6_BLT_MASK_16;
      break;
   case 4:
      mask = GEN6_BLT_MASK_32;
      break;
   default:
      return false;
      break;
   }

   if (util_format_is_pure_integer(rt->format) ||
       util_format_is_compressed(rt->format))
      return false;

   ilo_blit_resolve_surface(blitter->ilo, rt, ILO_TEXTURE_BLT_WRITE);

   util_pack_color(color->f, rt->format, &packed);

   if (rt->texture->target == PIPE_BUFFER) {
      unsigned offset, end, size;

      assert(y == 0 && height == 1);

      offset = (rt->u.buf.first_element + x) * cpp;
      end = (rt->u.buf.last_element + 1) * cpp;

      size = width * cpp;
      if (offset + size > end)
         size = end - offset;

      success = buf_clear_region(blitter, ilo_buffer(rt->texture),
            offset, size, packed.ui[0], mask, mask);
   }
   else {
      struct pipe_box box;

      u_box_3d(x, y, rt->u.tex.first_layer, width, height,
            rt->u.tex.last_layer - rt->u.tex.first_layer + 1, &box);

      success = tex_clear_region(blitter, ilo_texture(rt->texture),
            rt->u.tex.level, &box, packed.ui[0], mask, mask);
   }

   return success;
}

bool
ilo_blitter_blt_clear_zs(struct ilo_blitter *blitter,
                         struct pipe_surface *zs,
                         unsigned clear_flags,
                         double depth, unsigned stencil,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height)
{
   enum gen6_blt_mask value_mask, write_mask;
   struct pipe_box box;
   uint32_t val;

   if (!ilo_3d_pass_render_condition(blitter->ilo))
      return true;

   switch (zs->format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (!(clear_flags & PIPE_CLEAR_DEPTH))
         return true;

      value_mask = GEN6_BLT_MASK_16;
      write_mask = GEN6_BLT_MASK_16;
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      if (!(clear_flags & PIPE_CLEAR_DEPTH))
         return true;

      value_mask = GEN6_BLT_MASK_32;
      write_mask = GEN6_BLT_MASK_32;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
      if (!(clear_flags & PIPE_CLEAR_DEPTH))
         return true;

      value_mask = GEN6_BLT_MASK_32;
      write_mask = GEN6_BLT_MASK_32_LO;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (!(clear_flags & PIPE_CLEAR_DEPTHSTENCIL))
         return true;

      value_mask = GEN6_BLT_MASK_32;

      if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) == PIPE_CLEAR_DEPTHSTENCIL)
         write_mask = GEN6_BLT_MASK_32;
      else if (clear_flags & PIPE_CLEAR_DEPTH)
         write_mask = GEN6_BLT_MASK_32_LO;
      else
         write_mask = GEN6_BLT_MASK_32_HI;
      break;
   default:
      return false;
      break;
   }

   ilo_blit_resolve_surface(blitter->ilo, zs, ILO_TEXTURE_BLT_WRITE);

   val = util_pack_z_stencil(zs->format, depth, stencil);

   u_box_3d(x, y, zs->u.tex.first_layer, width, height,
         zs->u.tex.last_layer - zs->u.tex.first_layer + 1, &box);

   assert(zs->texture->target != PIPE_BUFFER);

   return tex_clear_region(blitter, ilo_texture(zs->texture),
         zs->u.tex.level, &box, val, value_mask, write_mask);
}
