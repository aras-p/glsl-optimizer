/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#ifndef ILO_BUILDER_BLT_H
#define ILO_BUILDER_BLT_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_builder.h"

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

static inline uint32_t
gen6_translate_blt_value_mask(enum gen6_blt_mask value_mask)
{
   switch (value_mask) {
   case GEN6_BLT_MASK_8:  return GEN6_BLITTER_BR13_FORMAT_8;
   case GEN6_BLT_MASK_16: return GEN6_BLITTER_BR13_FORMAT_565;
   default:               return GEN6_BLITTER_BR13_FORMAT_8888;
   }
}

static inline uint32_t
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

static inline uint32_t
gen6_translate_blt_cpp(enum gen6_blt_mask mask)
{
   switch (mask) {
   case GEN6_BLT_MASK_8:  return 1;
   case GEN6_BLT_MASK_16: return 2;
   default:               return 4;
   }
}

static inline void
gen6_COLOR_BLT(struct ilo_builder *builder,
               struct intel_bo *dst_bo,
               int16_t dst_pitch, uint32_t dst_offset,
               uint16_t width, uint16_t height,
               uint32_t pattern, uint8_t rop,
               enum gen6_blt_mask value_mask,
               enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 5;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   uint32_t dw0, dw1, *dw;
   unsigned pos;

   dw0 = GEN6_BLITTER_CMD(COLOR_BLT) |
         gen6_translate_blt_write_mask(write_mask) |
         (cmd_len - 2);

   assert(width < gen6_max_bytes_per_scanline);
   assert(height < gen6_max_scanlines);
   /* offsets are naturally aligned and pitches are dword-aligned */
   assert(dst_offset % cpp == 0 && dst_pitch % 4 == 0);

   dw1 = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
         gen6_translate_blt_value_mask(value_mask) |
         dst_pitch;

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = height << 16 | width;
   dw[4] = pattern;

   ilo_builder_batch_reloc(builder, pos + 3,
         dst_bo, dst_offset, INTEL_RELOC_WRITE);
}

static inline void
gen6_XY_COLOR_BLT(struct ilo_builder *builder,
                  struct intel_bo *dst_bo,
                  enum intel_tiling_mode dst_tiling,
                  int16_t dst_pitch, uint32_t dst_offset,
                  int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                  uint32_t pattern, uint8_t rop,
                  enum gen6_blt_mask value_mask,
                  enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 6;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   int dst_align, dst_pitch_shift;
   uint32_t dw0, dw1, *dw;
   unsigned pos;

   dw0 = GEN6_BLITTER_CMD(XY_COLOR_BLT) |
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

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = y1 << 16 | x1;
   dw[3] = y2 << 16 | x2;
   dw[5] = pattern;

   ilo_builder_batch_reloc(builder, pos + 4,
         dst_bo, dst_offset, INTEL_RELOC_WRITE);
}

static inline void
gen6_SRC_COPY_BLT(struct ilo_builder *builder,
                  struct intel_bo *dst_bo,
                  int16_t dst_pitch, uint32_t dst_offset,
                  uint16_t width, uint16_t height,
                  struct intel_bo *src_bo,
                  int16_t src_pitch, uint32_t src_offset,
                  bool dir_rtl, uint8_t rop,
                  enum gen6_blt_mask value_mask,
                  enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 6;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   uint32_t dw0, dw1, *dw;
   unsigned pos;

   dw0 = GEN6_BLITTER_CMD(SRC_COPY_BLT) |
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

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = height << 16 | width;
   dw[4] = src_pitch;

   ilo_builder_batch_reloc(builder, pos + 3,
         dst_bo, dst_offset, INTEL_RELOC_WRITE);
   ilo_builder_batch_reloc(builder, pos + 5,
         src_bo, src_offset, 0);
}

static inline void
gen6_XY_SRC_COPY_BLT(struct ilo_builder *builder,
                     struct intel_bo *dst_bo,
                     enum intel_tiling_mode dst_tiling,
                     int16_t dst_pitch, uint32_t dst_offset,
                     int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                     struct intel_bo *src_bo,
                     enum intel_tiling_mode src_tiling,
                     int16_t src_pitch, uint32_t src_offset,
                     int16_t src_x, int16_t src_y, uint8_t rop,
                     enum gen6_blt_mask value_mask,
                     enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 8;
   const int cpp = gen6_translate_blt_cpp(value_mask);
   int dst_align, dst_pitch_shift;
   int src_align, src_pitch_shift;
   uint32_t dw0, dw1, *dw;
   unsigned pos;

   dw0 = GEN6_BLITTER_CMD(XY_SRC_COPY_BLT) |
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

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);
   dw[0] = dw0;
   dw[1] = dw1;
   dw[2] = y1 << 16 | x1;
   dw[3] = y2 << 16 | x2;
   dw[5] = src_y << 16 | src_x;
   dw[6] = src_pitch >> src_pitch_shift;

   ilo_builder_batch_reloc(builder, pos + 4,
         dst_bo, dst_offset, INTEL_RELOC_WRITE);
   ilo_builder_batch_reloc(builder, pos + 7,
         src_bo, src_offset, 0);
}

#endif /* ILO_BUILDER_BLT_H */
