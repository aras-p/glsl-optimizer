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

struct gen6_blt_bo {
   struct intel_bo *bo;
   uint32_t offset;
   int16_t pitch;
};

struct gen6_blt_xy_bo {
   struct intel_bo *bo;
   uint32_t offset;
   int16_t pitch;

   enum intel_tiling_mode tiling;
   int16_t x, y;
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
static const int gen6_blt_max_bytes_per_scanline = 32768;
static const int gen6_blt_max_scanlines = 65536;

static inline uint32_t
gen6_blt_translate_value_mask(enum gen6_blt_mask value_mask)
{
   switch (value_mask) {
   case GEN6_BLT_MASK_8:  return GEN6_BLITTER_BR13_FORMAT_8;
   case GEN6_BLT_MASK_16: return GEN6_BLITTER_BR13_FORMAT_565;
   default:               return GEN6_BLITTER_BR13_FORMAT_8888;
   }
}

static inline uint32_t
gen6_blt_translate_value_cpp(enum gen6_blt_mask value_mask)
{
   switch (value_mask) {
   case GEN6_BLT_MASK_8:  return 1;
   case GEN6_BLT_MASK_16: return 2;
   default:               return 4;
   }
}

static inline uint32_t
gen6_blt_translate_write_mask(enum gen6_blt_mask write_mask)
{
   switch (write_mask) {
   case GEN6_BLT_MASK_32:    return GEN6_BLITTER_BR00_WRITE_RGB |
                                    GEN6_BLITTER_BR00_WRITE_A;
   case GEN6_BLT_MASK_32_LO: return GEN6_BLITTER_BR00_WRITE_RGB;
   case GEN6_BLT_MASK_32_HI: return GEN6_BLITTER_BR00_WRITE_A;
   default:                  return 0;
   }
}

static inline void
gen6_COLOR_BLT(struct ilo_builder *builder,
               const struct gen6_blt_bo *dst, uint32_t pattern,
               uint16_t width, uint16_t height, uint8_t rop,
               enum gen6_blt_mask value_mask,
               enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 5;
   const int cpp = gen6_blt_translate_value_cpp(value_mask);
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(width < gen6_blt_max_bytes_per_scanline);
   assert(height < gen6_blt_max_scanlines);
   /* offsets are naturally aligned and pitches are dword-aligned */
   assert(dst->offset % cpp == 0 && dst->pitch % 4 == 0);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_BLITTER_CMD(COLOR_BLT) |
           gen6_blt_translate_write_mask(write_mask) |
           (cmd_len - 2);
   dw[1] = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
           gen6_blt_translate_value_mask(value_mask) |
           dst->pitch;
   dw[2] = height << 16 | width;
   dw[4] = pattern;

   ilo_builder_batch_reloc(builder, pos + 3,
         dst->bo, dst->offset, INTEL_RELOC_WRITE);
}

static inline void
gen6_XY_COLOR_BLT(struct ilo_builder *builder,
                  const struct gen6_blt_xy_bo *dst, uint32_t pattern,
                  uint16_t width, uint16_t height, uint8_t rop,
                  enum gen6_blt_mask value_mask,
                  enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 6;
   const int cpp = gen6_blt_translate_value_cpp(value_mask);
   int dst_align = 4, dst_pitch_shift = 0;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(width * cpp < gen6_blt_max_bytes_per_scanline);
   assert(height < gen6_blt_max_scanlines);
   /* INT16_MAX */
   assert(dst->x + width <= 32767 && dst->y + height <= 32767);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_BLITTER_CMD(XY_COLOR_BLT) |
           gen6_blt_translate_write_mask(write_mask) |
           (cmd_len - 2);

   if (dst->tiling != INTEL_TILING_NONE) {
      dw[0] |= GEN6_BLITTER_BR00_DST_TILED;

      dst_align = (dst->tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      dst_pitch_shift = 2;
   }

   assert(dst->offset % dst_align == 0 && dst->pitch % dst_align == 0);

   dw[1] = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
           gen6_blt_translate_value_mask(value_mask) |
           dst->pitch >> dst_pitch_shift;
   dw[2] = dst->y << 16 | dst->x;
   dw[3] = (dst->y + height) << 16 | (dst->x + width);
   dw[5] = pattern;

   ilo_builder_batch_reloc(builder, pos + 4,
         dst->bo, dst->offset, INTEL_RELOC_WRITE);
}

static inline void
gen6_SRC_COPY_BLT(struct ilo_builder *builder,
                  const struct gen6_blt_bo *dst,
                  const struct gen6_blt_bo *src,
                  uint16_t width, uint16_t height, uint8_t rop,
                  enum gen6_blt_mask value_mask,
                  enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 6;
   const int cpp = gen6_blt_translate_value_cpp(value_mask);
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(width < gen6_blt_max_bytes_per_scanline);
   assert(height < gen6_blt_max_scanlines);
   /* offsets are naturally aligned and pitches are dword-aligned */
   assert(dst->offset % cpp == 0 && dst->pitch % 4 == 0);
   assert(src->offset % cpp == 0 && src->pitch % 4 == 0);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_BLITTER_CMD(SRC_COPY_BLT) |
           gen6_blt_translate_write_mask(write_mask) |
           (cmd_len - 2);

   dw[1] = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
           gen6_blt_translate_value_mask(value_mask) |
           dst->pitch;

   dw[2] = height << 16 | width;
   dw[4] = src->pitch;

   ilo_builder_batch_reloc(builder, pos + 3,
         dst->bo, dst->offset, INTEL_RELOC_WRITE);
   ilo_builder_batch_reloc(builder, pos + 5, src->bo, src->offset, 0);
}

static inline void
gen6_XY_SRC_COPY_BLT(struct ilo_builder *builder,
                     const struct gen6_blt_xy_bo *dst,
                     const struct gen6_blt_xy_bo *src,
                     uint16_t width, uint16_t height, uint8_t rop,
                     enum gen6_blt_mask value_mask,
                     enum gen6_blt_mask write_mask)
{
   const uint8_t cmd_len = 8;
   const int cpp = gen6_blt_translate_value_cpp(value_mask);
   int dst_align = 4, dst_pitch_shift = 0;
   int src_align = 4, src_pitch_shift = 0;
   uint32_t *dw;
   unsigned pos;

   ILO_DEV_ASSERT(builder->dev, 6, 7.5);

   assert(width * cpp < gen6_blt_max_bytes_per_scanline);
   assert(height < gen6_blt_max_scanlines);
   /* INT16_MAX */
   assert(dst->x + width <= 32767 && dst->y + height <= 32767);

   pos = ilo_builder_batch_pointer(builder, cmd_len, &dw);

   dw[0] = GEN6_BLITTER_CMD(XY_SRC_COPY_BLT) |
           gen6_blt_translate_write_mask(write_mask) |
           (cmd_len - 2);

   if (dst->tiling != INTEL_TILING_NONE) {
      dw[0] |= GEN6_BLITTER_BR00_DST_TILED;

      dst_align = (dst->tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      dst_pitch_shift = 2;
   }

   if (src->tiling != INTEL_TILING_NONE) {
      dw[0] |= GEN6_BLITTER_BR00_SRC_TILED;

      src_align = (src->tiling == INTEL_TILING_Y) ? 128 : 512;
      /* in dwords when tiled */
      src_pitch_shift = 2;
   }

   assert(dst->offset % dst_align == 0 && dst->pitch % dst_align == 0);
   assert(src->offset % src_align == 0 && src->pitch % src_align == 0);

   dw[1] = rop << GEN6_BLITTER_BR13_ROP__SHIFT |
           gen6_blt_translate_value_mask(value_mask) |
           dst->pitch >> dst_pitch_shift;
   dw[2] = dst->y << 16 | dst->x;
   dw[3] = (dst->y + height) << 16 | (dst->x + width);
   dw[5] = src->y << 16 | src->x;
   dw[6] = src->pitch >> src_pitch_shift;

   ilo_builder_batch_reloc(builder, pos + 4,
         dst->bo, dst->offset, INTEL_RELOC_WRITE);
   ilo_builder_batch_reloc(builder, pos + 7, src->bo, src->offset, 0);
}

#endif /* ILO_BUILDER_BLT_H */
