/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Mipmap generation utility
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_debug.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_draw_quad.h"
#include "util/u_gen_mipmap.h"

#include "tgsi/util/tgsi_build.h"
#include "tgsi/util/tgsi_dump.h"
#include "tgsi/util/tgsi_parse.h"



enum dtype
{
   UBYTE,
   UBYTE_3_3_2,
   USHORT,
   USHORT_4_4_4_4,
   USHORT_5_6_5,
   USHORT_1_5_5_5_REV,
   UINT,
   FLOAT,
   HALF_FLOAT
};


typedef ushort half_float;


#if 0
extern half_float
float_to_half(float f);

extern float
half_to_float(half_float h);
#endif


/**
 * Average together two rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 * \param datatype  GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT, etc.
 * \param comps  number of components per pixel (1..4)
 */
static void
do_row(enum dtype datatype, uint comps, int srcWidth,
       const void *srcRowA, const void *srcRowB,
       int dstWidth, void *dstRow)
{
   const uint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const uint colStride = (srcWidth == dstWidth) ? 1 : 2;

   assert(comps >= 1);
   assert(comps <= 4);

   /* This assertion is no longer valid with non-power-of-2 textures
   assert(srcWidth == dstWidth || srcWidth == 2 * dstWidth);
   */

   if (datatype == UBYTE && comps == 4) {
      uint i, j, k;
      const ubyte(*rowA)[4] = (const ubyte(*)[4]) srcRowA;
      const ubyte(*rowB)[4] = (const ubyte(*)[4]) srcRowB;
      ubyte(*dst)[4] = (ubyte(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == UBYTE && comps == 3) {
      uint i, j, k;
      const ubyte(*rowA)[3] = (const ubyte(*)[3]) srcRowA;
      const ubyte(*rowB)[3] = (const ubyte(*)[3]) srcRowB;
      ubyte(*dst)[3] = (ubyte(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == UBYTE && comps == 2) {
      uint i, j, k;
      const ubyte(*rowA)[2] = (const ubyte(*)[2]) srcRowA;
      const ubyte(*rowB)[2] = (const ubyte(*)[2]) srcRowB;
      ubyte(*dst)[2] = (ubyte(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) >> 2;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) >> 2;
      }
   }
   else if (datatype == UBYTE && comps == 1) {
      uint i, j, k;
      const ubyte *rowA = (const ubyte *) srcRowA;
      const ubyte *rowB = (const ubyte *) srcRowB;
      ubyte *dst = (ubyte *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) >> 2;
      }
   }

   else if (datatype == USHORT && comps == 4) {
      uint i, j, k;
      const ushort(*rowA)[4] = (const ushort(*)[4]) srcRowA;
      const ushort(*rowB)[4] = (const ushort(*)[4]) srcRowB;
      ushort(*dst)[4] = (ushort(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == USHORT && comps == 3) {
      uint i, j, k;
      const ushort(*rowA)[3] = (const ushort(*)[3]) srcRowA;
      const ushort(*rowB)[3] = (const ushort(*)[3]) srcRowB;
      ushort(*dst)[3] = (ushort(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == USHORT && comps == 2) {
      uint i, j, k;
      const ushort(*rowA)[2] = (const ushort(*)[2]) srcRowA;
      const ushort(*rowB)[2] = (const ushort(*)[2]) srcRowB;
      ushort(*dst)[2] = (ushort(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
      }
   }
   else if (datatype == USHORT && comps == 1) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
      }
   }

   else if (datatype == FLOAT && comps == 4) {
      uint i, j, k;
      const float(*rowA)[4] = (const float(*)[4]) srcRowA;
      const float(*rowB)[4] = (const float(*)[4]) srcRowB;
      float(*dst)[4] = (float(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
                      rowB[j][2] + rowB[k][2]) * 0.25F;
         dst[i][3] = (rowA[j][3] + rowA[k][3] +
                      rowB[j][3] + rowB[k][3]) * 0.25F;
      }
   }
   else if (datatype == FLOAT && comps == 3) {
      uint i, j, k;
      const float(*rowA)[3] = (const float(*)[3]) srcRowA;
      const float(*rowB)[3] = (const float(*)[3]) srcRowB;
      float(*dst)[3] = (float(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
                      rowB[j][2] + rowB[k][2]) * 0.25F;
      }
   }
   else if (datatype == FLOAT && comps == 2) {
      uint i, j, k;
      const float(*rowA)[2] = (const float(*)[2]) srcRowA;
      const float(*rowB)[2] = (const float(*)[2]) srcRowB;
      float(*dst)[2] = (float(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
      }
   }
   else if (datatype == FLOAT && comps == 1) {
      uint i, j, k;
      const float *rowA = (const float *) srcRowA;
      const float *rowB = (const float *) srcRowB;
      float *dst = (float *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) * 0.25F;
      }
   }

#if 0
   else if (datatype == HALF_FLOAT && comps == 4) {
      uint i, j, k, comp;
      const half_float(*rowA)[4] = (const half_float(*)[4]) srcRowA;
      const half_float(*rowB)[4] = (const half_float(*)[4]) srcRowB;
      half_float(*dst)[4] = (half_float(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 4; comp++) {
            float aj, ak, bj, bk;
            aj = half_to_float(rowA[j][comp]);
            ak = half_to_float(rowA[k][comp]);
            bj = half_to_float(rowB[j][comp]);
            bk = half_to_float(rowB[k][comp]);
            dst[i][comp] = float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == HALF_FLOAT && comps == 3) {
      uint i, j, k, comp;
      const half_float(*rowA)[3] = (const half_float(*)[3]) srcRowA;
      const half_float(*rowB)[3] = (const half_float(*)[3]) srcRowB;
      half_float(*dst)[3] = (half_float(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 3; comp++) {
            float aj, ak, bj, bk;
            aj = half_to_float(rowA[j][comp]);
            ak = half_to_float(rowA[k][comp]);
            bj = half_to_float(rowB[j][comp]);
            bk = half_to_float(rowB[k][comp]);
            dst[i][comp] = float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == HALF_FLOAT && comps == 2) {
      uint i, j, k, comp;
      const half_float(*rowA)[2] = (const half_float(*)[2]) srcRowA;
      const half_float(*rowB)[2] = (const half_float(*)[2]) srcRowB;
      half_float(*dst)[2] = (half_float(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 2; comp++) {
            float aj, ak, bj, bk;
            aj = half_to_float(rowA[j][comp]);
            ak = half_to_float(rowA[k][comp]);
            bj = half_to_float(rowB[j][comp]);
            bk = half_to_float(rowB[k][comp]);
            dst[i][comp] = float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == HALF_FLOAT && comps == 1) {
      uint i, j, k;
      const half_float *rowA = (const half_float *) srcRowA;
      const half_float *rowB = (const half_float *) srcRowB;
      half_float *dst = (half_float *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         float aj, ak, bj, bk;
         aj = half_to_float(rowA[j]);
         ak = half_to_float(rowA[k]);
         bj = half_to_float(rowB[j]);
         bk = half_to_float(rowB[k]);
         dst[i] = float_to_half((aj + ak + bj + bk) * 0.25F);
      }
   }
#endif

   else if (datatype == UINT && comps == 1) {
      uint i, j, k;
      const uint *rowA = (const uint *) srcRowA;
      const uint *rowB = (const uint *) srcRowB;
      uint *dst = (uint *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = rowA[j] / 4 + rowA[k] / 4 + rowB[j] / 4 + rowB[k] / 4;
      }
   }

   else if (datatype == USHORT_5_6_5 && comps == 3) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x1f;
         const int rowAr1 = rowA[k] & 0x1f;
         const int rowBr0 = rowB[j] & 0x1f;
         const int rowBr1 = rowB[k] & 0x1f;
         const int rowAg0 = (rowA[j] >> 5) & 0x3f;
         const int rowAg1 = (rowA[k] >> 5) & 0x3f;
         const int rowBg0 = (rowB[j] >> 5) & 0x3f;
         const int rowBg1 = (rowB[k] >> 5) & 0x3f;
         const int rowAb0 = (rowA[j] >> 11) & 0x1f;
         const int rowAb1 = (rowA[k] >> 11) & 0x1f;
         const int rowBb0 = (rowB[j] >> 11) & 0x1f;
         const int rowBb1 = (rowB[k] >> 11) & 0x1f;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         dst[i] = (blue << 11) | (green << 5) | red;
      }
   }
   else if (datatype == USHORT_4_4_4_4 && comps == 4) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0xf;
         const int rowAr1 = rowA[k] & 0xf;
         const int rowBr0 = rowB[j] & 0xf;
         const int rowBr1 = rowB[k] & 0xf;
         const int rowAg0 = (rowA[j] >> 4) & 0xf;
         const int rowAg1 = (rowA[k] >> 4) & 0xf;
         const int rowBg0 = (rowB[j] >> 4) & 0xf;
         const int rowBg1 = (rowB[k] >> 4) & 0xf;
         const int rowAb0 = (rowA[j] >> 8) & 0xf;
         const int rowAb1 = (rowA[k] >> 8) & 0xf;
         const int rowBb0 = (rowB[j] >> 8) & 0xf;
         const int rowBb1 = (rowB[k] >> 8) & 0xf;
         const int rowAa0 = (rowA[j] >> 12) & 0xf;
         const int rowAa1 = (rowA[k] >> 12) & 0xf;
         const int rowBa0 = (rowB[j] >> 12) & 0xf;
         const int rowBa1 = (rowB[k] >> 12) & 0xf;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const int alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 12) | (blue << 8) | (green << 4) | red;
      }
   }
   else if (datatype == USHORT_1_5_5_5_REV && comps == 4) {
      uint i, j, k;
      const ushort *rowA = (const ushort *) srcRowA;
      const ushort *rowB = (const ushort *) srcRowB;
      ushort *dst = (ushort *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x1f;
         const int rowAr1 = rowA[k] & 0x1f;
         const int rowBr0 = rowB[j] & 0x1f;
         const int rowBr1 = rowB[k] & 0xf;
         const int rowAg0 = (rowA[j] >> 5) & 0x1f;
         const int rowAg1 = (rowA[k] >> 5) & 0x1f;
         const int rowBg0 = (rowB[j] >> 5) & 0x1f;
         const int rowBg1 = (rowB[k] >> 5) & 0x1f;
         const int rowAb0 = (rowA[j] >> 10) & 0x1f;
         const int rowAb1 = (rowA[k] >> 10) & 0x1f;
         const int rowBb0 = (rowB[j] >> 10) & 0x1f;
         const int rowBb1 = (rowB[k] >> 10) & 0x1f;
         const int rowAa0 = (rowA[j] >> 15) & 0x1;
         const int rowAa1 = (rowA[k] >> 15) & 0x1;
         const int rowBa0 = (rowB[j] >> 15) & 0x1;
         const int rowBa1 = (rowB[k] >> 15) & 0x1;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const int alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 15) | (blue << 10) | (green << 5) | red;
      }
   }
   else if (datatype == UBYTE_3_3_2 && comps == 3) {
      uint i, j, k;
      const ubyte *rowA = (const ubyte *) srcRowA;
      const ubyte *rowB = (const ubyte *) srcRowB;
      ubyte *dst = (ubyte *) dstRow;
      for (i = j = 0, k = k0; i < (uint) dstWidth;
           i++, j += colStride, k += colStride) {
         const int rowAr0 = rowA[j] & 0x3;
         const int rowAr1 = rowA[k] & 0x3;
         const int rowBr0 = rowB[j] & 0x3;
         const int rowBr1 = rowB[k] & 0x3;
         const int rowAg0 = (rowA[j] >> 2) & 0x7;
         const int rowAg1 = (rowA[k] >> 2) & 0x7;
         const int rowBg0 = (rowB[j] >> 2) & 0x7;
         const int rowBg1 = (rowB[k] >> 2) & 0x7;
         const int rowAb0 = (rowA[j] >> 5) & 0x7;
         const int rowAb1 = (rowA[k] >> 5) & 0x7;
         const int rowBb0 = (rowB[j] >> 5) & 0x7;
         const int rowBb1 = (rowB[k] >> 5) & 0x7;
         const int red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const int green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const int blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         dst[i] = (blue << 5) | (green << 2) | red;
      }
   }
   else {
      debug_printf("bad format in do_row()");
   }
}


static void
format_to_type_comps(enum pipe_format pformat,
                     enum dtype *datatype, uint *comps)
{
   switch (pformat) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      *datatype = UBYTE;
      *comps = 4;
      return;
   case PIPE_FORMAT_A1R5G5B5_UNORM:
      *datatype = USHORT_1_5_5_5_REV;
      *comps = 4;
      return;
   case PIPE_FORMAT_A4R4G4B4_UNORM:
      *datatype = USHORT_4_4_4_4;
      *comps = 4;
      return;
   case PIPE_FORMAT_R5G6B5_UNORM:
      *datatype = USHORT_5_6_5;
      *comps = 3;
      return;
   case PIPE_FORMAT_U_L8:
   case PIPE_FORMAT_U_A8:
   case PIPE_FORMAT_U_I8:
      *datatype = UBYTE;
      *comps = 1;
      return;
   case PIPE_FORMAT_U_A8_L8:
      *datatype = UBYTE;
      *comps = 2;
      return;
   default:
      assert(0);
   }
}


static void
reduce_1d(enum pipe_format pformat,
          int srcWidth, const ubyte *srcPtr,
          int dstWidth, ubyte *dstPtr)
{
   enum dtype datatype;
   uint comps;

   format_to_type_comps(pformat, &datatype, &comps);

   /* we just duplicate the input row, kind of hack, saves code */
   do_row(datatype, comps,
          srcWidth, srcPtr, srcPtr,
          dstWidth, dstPtr);
}


/**
 * Strides are in bytes.  If zero, it'll be computed as width * bpp.
 */
static void
reduce_2d(enum pipe_format pformat,
          int srcWidth, int srcHeight,
          int srcRowStride, const ubyte *srcPtr,
          int dstWidth, int dstHeight,
          int dstRowStride, ubyte *dstPtr)
{
   enum dtype datatype;
   uint comps;
   const int bpt = pf_get_size(pformat);
   const ubyte *srcA, *srcB;
   ubyte *dst;
   int row;

   format_to_type_comps(pformat, &datatype, &comps);

   if (!srcRowStride)
      srcRowStride = bpt * srcWidth;

   if (!dstRowStride)
      dstRowStride = bpt * dstWidth;

   /* Compute src and dst pointers */
   srcA = srcPtr;
   if (srcHeight > 1) 
      srcB = srcA + srcRowStride;
   else
      srcB = srcA;
   dst = dstPtr;

   for (row = 0; row < dstHeight; row++) {
      do_row(datatype, comps,
             srcWidth, srcA, srcB,
             dstWidth, dst);
      srcA += 2 * srcRowStride;
      srcB += 2 * srcRowStride;
      dst += dstRowStride;
   }
}


static void
make_1d_mipmap(struct gen_mipmap_state *ctx,
               struct pipe_texture *pt,
               uint face, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_winsys *winsys = pipe->winsys;
   const uint zslice = 0;
   uint dstLevel;

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_surface *srcSurf, *dstSurf;
      void *srcMap, *dstMap;
      
      srcSurf = screen->get_tex_surface(screen, pt, face, srcLevel, zslice);
      dstSurf = screen->get_tex_surface(screen, pt, face, dstLevel, zslice);

      srcMap = ((ubyte *) winsys->buffer_map(winsys, srcSurf->buffer,
                                            PIPE_BUFFER_USAGE_CPU_READ)
                + srcSurf->offset);
      dstMap = ((ubyte *) winsys->buffer_map(winsys, dstSurf->buffer,
                                            PIPE_BUFFER_USAGE_CPU_WRITE)
                + dstSurf->offset);

      reduce_1d(pt->format,
                srcSurf->width, srcMap,
                dstSurf->width, dstMap);

      winsys->buffer_unmap(winsys, srcSurf->buffer);
      winsys->buffer_unmap(winsys, dstSurf->buffer);

      pipe_surface_reference(&srcSurf, NULL);
      pipe_surface_reference(&dstSurf, NULL);
   }
}


static void
make_2d_mipmap(struct gen_mipmap_state *ctx,
               struct pipe_texture *pt,
               uint face, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_winsys *winsys = pipe->winsys;
   const uint zslice = 0;
   uint dstLevel;
   const int bpt = pf_get_size(pt->format);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_surface *srcSurf, *dstSurf;
      ubyte *srcMap, *dstMap;
      
      srcSurf = screen->get_tex_surface(screen, pt, face, srcLevel, zslice);
      dstSurf = screen->get_tex_surface(screen, pt, face, dstLevel, zslice);

      srcMap = ((ubyte *) winsys->buffer_map(winsys, srcSurf->buffer,
                                            PIPE_BUFFER_USAGE_CPU_READ)
                + srcSurf->offset);
      dstMap = ((ubyte *) winsys->buffer_map(winsys, dstSurf->buffer,
                                            PIPE_BUFFER_USAGE_CPU_WRITE)
                + dstSurf->offset);

      reduce_2d(pt->format,
                srcSurf->width, srcSurf->height,
                srcSurf->pitch * bpt, srcMap,
                dstSurf->width, dstSurf->height,
                dstSurf->pitch * bpt, dstMap);

      winsys->buffer_unmap(winsys, srcSurf->buffer);
      winsys->buffer_unmap(winsys, dstSurf->buffer);

      pipe_surface_reference(&srcSurf, NULL);
      pipe_surface_reference(&dstSurf, NULL);
   }
}


static void
make_3d_mipmap(struct gen_mipmap_state *ctx,
               struct pipe_texture *pt,
               uint face, uint baseLevel, uint lastLevel)
{
}


static void
fallback_gen_mipmap(struct gen_mipmap_state *ctx,
                    struct pipe_texture *pt,
                    uint face, uint baseLevel, uint lastLevel)
{
   switch (pt->target) {
   case PIPE_TEXTURE_1D:
      make_1d_mipmap(ctx, pt, face, baseLevel, lastLevel);
      break;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
      make_2d_mipmap(ctx, pt, face, baseLevel, lastLevel);
      break;
   case PIPE_TEXTURE_3D:
      make_3d_mipmap(ctx, pt, face, baseLevel, lastLevel);
      break;
   default:
      assert(0);
   }
}


/**
 * Make simple fragment shader:
 *  TEX OUT[0], IN[0], SAMP[0], 2D;
 *  END;
 */
static void
make_fragment_shader(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   uint maxTokens = 100;
   struct tgsi_token *tokens;
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_FRAGMENT;
   uint ti = 0;
   struct pipe_shader_state shader;

   tokens = (struct tgsi_token *) malloc(maxTokens * sizeof(tokens[0]));

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare TEX[0] input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   /* XXX this could be linear... */
   decl.Declaration.Interpolate = 1;
   decl.Interpolation.Interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare color[0] output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_COLOR;
   decl.Semantic.SemanticIndex = 0;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare sampler */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_SAMPLER;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* TEX instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_TEX;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 0;
   inst.Instruction.NumSrcRegs = 2;
   inst.InstructionExtTexture.Texture = TGSI_TEXTURE_2D;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 0;
   inst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
   inst.FullSrcRegisters[1].SrcRegister.Index = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;
   ctx->fs = pipe->create_fs_state(pipe, &shader);
}


/**
 * Make simple fragment shader:
 *  MOV OUT[0], IN[0];
 *  MOV OUT[1], IN[1];
 *  END;
 *
 * XXX eliminate this when vertex passthrough-mode is more solid.
 */
static void
make_vertex_shader(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   uint maxTokens = 100;
   struct tgsi_token *tokens;
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_VERTEX;
   uint ti = 0;
   struct pipe_shader_state shader;

   tokens = (struct tgsi_token *) malloc(maxTokens * sizeof(tokens[0]));

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare POS input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   /*
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_POSITION;
   decl.Semantic.SemanticIndex = 0;
   */
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);
   /* declare TEX[0] input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   /*
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   */
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 1;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare POS output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_POSITION;
   decl.Semantic.SemanticIndex = 0;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare TEX[0] output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 1;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* MOVE out[0], in[0];  # POS */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_MOV;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 0;
   inst.Instruction.NumSrcRegs = 1;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

   /* MOVE out[1], in[1];  # TEX */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_MOV;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 1;
   inst.Instruction.NumSrcRegs = 1;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 1;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;
   ctx->vs = pipe->create_vs_state(pipe, &shader);
}


/**
 * Create a mipmap generation context.
 * The idea is to create one of these and re-use it each time we need to
 * generate a mipmap.
 */
struct gen_mipmap_state *
util_create_gen_mipmap(struct pipe_context *pipe)
{
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct gen_mipmap_state *ctx;

   ctx = CALLOC_STRUCT(gen_mipmap_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;

   /* we don't use blending, but need to set valid values */
   memset(&blend, 0, sizeof(blend));
   blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.colormask = PIPE_MASK_RGBA;
   ctx->blend = pipe->create_blend_state(pipe, &blend);

   /* depth/stencil/alpha */
   memset(&depthstencil, 0, sizeof(depthstencil));
   ctx->depthstencil = pipe->create_depth_stencil_alpha_state(pipe, &depthstencil);

   /* rasterizer */
   memset(&rasterizer, 0, sizeof(rasterizer));
   rasterizer.front_winding = PIPE_WINDING_CW;
   rasterizer.cull_mode = PIPE_WINDING_NONE;
   rasterizer.bypass_clipping = 1;  /* bypasses viewport too */
   //rasterizer.bypass_vs = 1;
   ctx->rasterizer = pipe->create_rasterizer_state(pipe, &rasterizer);

#if 0
   /* viewport */
   ctx->viewport.scale[0] = 1.0;
   ctx->viewport.scale[1] = 1.0;
   ctx->viewport.scale[2] = 1.0;
   ctx->viewport.scale[3] = 1.0;
   ctx->viewport.translate[0] = 0.0;
   ctx->viewport.translate[1] = 0.0;
   ctx->viewport.translate[2] = 0.0;
   ctx->viewport.translate[3] = 0.0;
#endif

   make_vertex_shader(ctx);
   make_fragment_shader(ctx);

   return ctx;
}


/**
 * Destroy a mipmap generation context
 */
void
util_destroy_gen_mipmap(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   pipe->delete_blend_state(pipe, ctx->blend);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->depthstencil);
   pipe->delete_rasterizer_state(pipe, ctx->rasterizer);
   pipe->delete_vs_state(pipe, ctx->vs);
   pipe->delete_fs_state(pipe, ctx->fs);

   FREE(ctx);
}


#if 0
static void
simple_viewport(struct pipe_context *pipe, uint width, uint height)
{
   struct pipe_viewport_state vp;

   vp.scale[0] =  0.5 * width;
   vp.scale[1] = -0.5 * height;
   vp.scale[2] = 1.0;
   vp.scale[3] = 1.0;
   vp.translate[0] = 0.5 * width;
   vp.translate[1] = 0.5 * height;
   vp.translate[2] = 0.0;
   vp.translate[3] = 0.0;

   pipe->set_viewport_state(pipe, &vp);
}
#endif


/**
 * Generate mipmap images.  It's assumed all needed texture memory is
 * already allocated.
 *
 * \param pt  the texture to generate mipmap levels for
 * \param face  which cube face to generate mipmaps for (0 for non-cube maps)
 * \param baseLevel  the first mipmap level to use as a src
 * \param lastLevel  the last mipmap level to generate
 */
void
util_gen_mipmap(struct gen_mipmap_state *ctx,
                struct pipe_texture *pt,
                uint face, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   struct pipe_sampler_state sampler;
   void *sampler_cso;
   uint dstLevel;
   uint zslice = 0;

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, pt->format, PIPE_SURFACE)) {
      fallback_gen_mipmap(ctx, pt, face, baseLevel, lastLevel);
      return;
   }

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));
   fb.num_cbufs = 1;

   /* sampler state */
   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.normalized_coords = 1;

   /* bind our state */
   pipe->bind_blend_state(pipe, ctx->blend);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->depthstencil);
   pipe->bind_rasterizer_state(pipe, ctx->rasterizer);
   pipe->bind_vs_state(pipe, ctx->vs);
   pipe->bind_fs_state(pipe, ctx->fs);
#if 0
   pipe->set_viewport_state(pipe, &ctx->viewport);
#endif

   /*
    * XXX for small mipmap levels, it may be faster to use the software
    * fallback path...
    */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;

      /*
       * Setup framebuffer / dest surface
       */
      fb.cbufs[0] = screen->get_tex_surface(screen, pt, face, dstLevel, zslice);
      pipe->set_framebuffer_state(pipe, &fb);

      /*
       * Setup sampler state
       * Note: we should only have to set the min/max LOD clamps to ensure
       * we grab texels from the right mipmap level.  But some hardware
       * has trouble with min clamping so we also set the lod_bias to
       * try to work around that.
       */
      sampler.min_lod = sampler.max_lod = (float) srcLevel;
      sampler.lod_bias = (float) srcLevel;
      sampler_cso = pipe->create_sampler_state(pipe, &sampler);
      pipe->bind_sampler_states(pipe, 1, &sampler_cso);

#if 0
      simple_viewport(pipe, pt->width[dstLevel], pt->height[dstLevel]);
#endif

      pipe->set_sampler_textures(pipe, 1, &pt);

      /* quad coords in window coords (bypassing clipping, viewport mapping) */
      util_draw_texquad(pipe,
                        0.0F, 0.0F, /* x0, y0 */
                        (float) pt->width[dstLevel], /* x1 */
                        (float) pt->height[dstLevel], /* y1 */
                        0.0F);  /* z */


      pipe->flush(pipe, PIPE_FLUSH_WAIT);

      /*pipe->texture_update(pipe, pt);  not really needed */

      pipe->delete_sampler_state(pipe, sampler_cso);
   }

   /* Note: caller must restore pipe/gallium state at this time */
}
