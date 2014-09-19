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

#ifndef ILO_STATE_GEN_H
#define ILO_STATE_GEN_H

#include "genhw/genhw.h"
#include "intel_winsys.h"

#include "ilo_common.h"
#include "ilo_gpe.h"

/**
 * Translate winsys tiling to hardware tiling.
 */
static inline int
ilo_gpe_gen6_translate_winsys_tiling(enum intel_tiling_mode tiling)
{
   switch (tiling) {
   case INTEL_TILING_NONE:
      return GEN6_TILING_NONE;
   case INTEL_TILING_X:
      return GEN6_TILING_X;
   case INTEL_TILING_Y:
      return GEN6_TILING_Y;
   default:
      assert(!"unknown tiling");
      return GEN6_TILING_NONE;
   }
}

/**
 * Translate a pipe texture target to the matching hardware surface type.
 */
static inline int
ilo_gpe_gen6_translate_texture(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_BUFFER:
      return GEN6_SURFTYPE_BUFFER;
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return GEN6_SURFTYPE_1D;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
      return GEN6_SURFTYPE_2D;
   case PIPE_TEXTURE_3D:
      return GEN6_SURFTYPE_3D;
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      return GEN6_SURFTYPE_CUBE;
   default:
      assert(!"unknown texture target");
      return GEN6_SURFTYPE_BUFFER;
   }
}

static inline void
zs_align_surface(const struct ilo_dev_info *dev,
                 unsigned align_w, unsigned align_h,
                 struct ilo_zs_surface *zs)
{
   unsigned mask, shift_w, shift_h;
   unsigned width, height;
   uint32_t dw3;

   ILO_DEV_ASSERT(dev, 6, 7.5);

   if (ilo_dev_gen(dev) >= ILO_GEN(7)) {
      shift_w = 4;
      shift_h = 18;
      mask = 0x3fff;
   }
   else {
      shift_w = 6;
      shift_h = 19;
      mask = 0x1fff;
   }

   dw3 = zs->payload[2];

   /* aligned width and height */
   width = align(((dw3 >> shift_w) & mask) + 1, align_w);
   height = align(((dw3 >> shift_h) & mask) + 1, align_h);

   dw3 = (dw3 & ~((mask << shift_w) | (mask << shift_h))) |
      (width - 1) << shift_w |
      (height - 1) << shift_h;

   zs->payload[2] = dw3;
}

#endif /* ILO_STATE_GEN_H */
