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
#include "pipe/p_winsys.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_memory.h"
#include "util/u_draw_quad.h"
#include "util/u_gen_mipmap.h"
#include "util/u_simple_shaders.h"

#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"

#include "cso_cache/cso_context.h"


struct gen_mipmap_state
{
   struct pipe_context *pipe;
   struct cso_context *cso;

   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct pipe_sampler_state sampler;
   struct pipe_viewport_state viewport;

   struct pipe_shader_state vert_shader;
   struct pipe_shader_state frag_shader;
   void *vs;
   void *fs;

   struct pipe_buffer *vbuf;  /**< quad vertices */
   float vertices[4][2][4];   /**< vertex/texcoords for quad */
};



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
   case PIPE_FORMAT_X8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
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
   case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_A8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
      *datatype = UBYTE;
      *comps = 1;
      return;
   case PIPE_FORMAT_A8L8_UNORM:
      *datatype = UBYTE;
      *comps = 2;
      return;
   default:
      assert(0);
      *datatype = UBYTE;
      *comps = 0;
      break;
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
   const uint zslice = 0;
   uint dstLevel;

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_surface *srcSurf, *dstSurf;
      void *srcMap, *dstMap;
      
      srcSurf = screen->get_tex_surface(screen, pt, face, srcLevel, zslice,
                                        PIPE_BUFFER_USAGE_CPU_READ);

      dstSurf = screen->get_tex_surface(screen, pt, face, dstLevel, zslice,
                                        PIPE_BUFFER_USAGE_CPU_WRITE);

      srcMap = ((ubyte *) pipe_buffer_map(screen, srcSurf->buffer,
                                          PIPE_BUFFER_USAGE_CPU_READ)
                + srcSurf->offset);
      dstMap = ((ubyte *) pipe_buffer_map(screen, dstSurf->buffer,
                                          PIPE_BUFFER_USAGE_CPU_WRITE)
                + dstSurf->offset);

      reduce_1d(pt->format,
                srcSurf->width, srcMap,
                dstSurf->width, dstMap);

      pipe_buffer_unmap(screen, srcSurf->buffer);
      pipe_buffer_unmap(screen, dstSurf->buffer);

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
   const uint zslice = 0;
   uint dstLevel;
   
   assert(pt->block.width == 1);
   assert(pt->block.height == 1);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_surface *srcSurf, *dstSurf;
      ubyte *srcMap, *dstMap;
      
      srcSurf = screen->get_tex_surface(screen, pt, face, srcLevel, zslice,
                                        PIPE_BUFFER_USAGE_CPU_READ);
      dstSurf = screen->get_tex_surface(screen, pt, face, dstLevel, zslice,
                                        PIPE_BUFFER_USAGE_CPU_WRITE);

      srcMap = ((ubyte *) pipe_buffer_map(screen, srcSurf->buffer,
                                          PIPE_BUFFER_USAGE_CPU_READ)
                + srcSurf->offset);
      dstMap = ((ubyte *) pipe_buffer_map(screen, dstSurf->buffer,
                                          PIPE_BUFFER_USAGE_CPU_WRITE)
                + dstSurf->offset);

      reduce_2d(pt->format,
                srcSurf->width, srcSurf->height,
                srcSurf->stride, srcMap,
                dstSurf->width, dstSurf->height,
                dstSurf->stride, dstMap);

      pipe_buffer_unmap(screen, srcSurf->buffer);
      pipe_buffer_unmap(screen, dstSurf->buffer);

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
 * Create a mipmap generation context.
 * The idea is to create one of these and re-use it each time we need to
 * generate a mipmap.
 */
struct gen_mipmap_state *
util_create_gen_mipmap(struct pipe_context *pipe,
                       struct cso_context *cso)
{
   struct gen_mipmap_state *ctx;
   uint i;

   ctx = CALLOC_STRUCT(gen_mipmap_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;
   ctx->cso = cso;

   /* disabled blending/masking */
   memset(&ctx->blend, 0, sizeof(ctx->blend));
   ctx->blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   ctx->blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   ctx->blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   ctx->blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   ctx->blend.colormask = PIPE_MASK_RGBA;

   /* no-op depth/stencil/alpha */
   memset(&ctx->depthstencil, 0, sizeof(ctx->depthstencil));

   /* rasterizer */
   memset(&ctx->rasterizer, 0, sizeof(ctx->rasterizer));
   ctx->rasterizer.front_winding = PIPE_WINDING_CW;
   ctx->rasterizer.cull_mode = PIPE_WINDING_NONE;
   ctx->rasterizer.bypass_clipping = 1;
   /*ctx->rasterizer.bypass_vs = 1;*/

   /* sampler state */
   memset(&ctx->sampler, 0, sizeof(ctx->sampler));
   ctx->sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   ctx->sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   ctx->sampler.normalized_coords = 1;

   /* viewport state (identity, verts are in wincoords) */
   ctx->viewport.scale[0] = 1.0;
   ctx->viewport.scale[1] = 1.0;
   ctx->viewport.scale[2] = 1.0;
   ctx->viewport.scale[3] = 1.0;
   ctx->viewport.translate[0] = 0.0;
   ctx->viewport.translate[1] = 0.0;
   ctx->viewport.translate[2] = 0.0;
   ctx->viewport.translate[3] = 0.0;

   /* vertex shader */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      ctx->vs = util_make_vertex_passthrough_shader(pipe, 2, semantic_names,
                                                    semantic_indexes,
                                                    &ctx->vert_shader);
   }

   /* fragment shader */
   ctx->fs = util_make_fragment_tex_shader(pipe, &ctx->frag_shader);

   ctx->vbuf = pipe_buffer_create(pipe->screen,
                                  32,
                                  PIPE_BUFFER_USAGE_VERTEX,
                                  sizeof(ctx->vertices));
   if (!ctx->vbuf) {
      FREE(ctx);
      return NULL;
   }

   /* vertex data that doesn't change */
   for (i = 0; i < 4; i++) {
      ctx->vertices[i][0][2] = 0.0f; /* z */
      ctx->vertices[i][0][3] = 1.0f; /* w */
      ctx->vertices[i][1][2] = 0.0f; /* r */
      ctx->vertices[i][1][3] = 1.0f; /* q */
   }

   return ctx;
}


static void
set_vertex_data(struct gen_mipmap_state *ctx, float width, float height)
{
   void *buf;

   ctx->vertices[0][0][0] = 0.0f; /*x*/
   ctx->vertices[0][0][1] = 0.0f; /*y*/
   ctx->vertices[0][1][0] = 0.0f; /*s*/
   ctx->vertices[0][1][1] = 0.0f; /*t*/

   ctx->vertices[1][0][0] = width;
   ctx->vertices[1][0][1] = 0.0f;
   ctx->vertices[1][1][0] = 1.0f;
   ctx->vertices[1][1][1] = 0.0f;

   ctx->vertices[2][0][0] = width;
   ctx->vertices[2][0][1] = height;
   ctx->vertices[2][1][0] = 1.0f;
   ctx->vertices[2][1][1] = 1.0f;

   ctx->vertices[3][0][0] = 0.0f;
   ctx->vertices[3][0][1] = height;
   ctx->vertices[3][1][0] = 0.0f;
   ctx->vertices[3][1][1] = 1.0f;

   buf = pipe_buffer_map(ctx->pipe->screen, ctx->vbuf,
                         PIPE_BUFFER_USAGE_CPU_WRITE);

   memcpy(buf, ctx->vertices, sizeof(ctx->vertices));

   pipe_buffer_unmap(ctx->pipe->screen, ctx->vbuf);
}



/**
 * Destroy a mipmap generation context
 */
void
util_destroy_gen_mipmap(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   pipe->delete_vs_state(pipe, ctx->vs);
   pipe->delete_fs_state(pipe, ctx->fs);

   FREE((void*) ctx->vert_shader.tokens);
   FREE((void*) ctx->frag_shader.tokens);

   pipe_buffer_reference(pipe->screen, &ctx->vbuf, NULL);

   FREE(ctx);
}


/**
 * Generate mipmap images.  It's assumed all needed texture memory is
 * already allocated.
 *
 * \param pt  the texture to generate mipmap levels for
 * \param face  which cube face to generate mipmaps for (0 for non-cube maps)
 * \param baseLevel  the first mipmap level to use as a src
 * \param lastLevel  the last mipmap level to generate
 * \param filter  the minification filter used to generate mipmap levels with
 * \param filter  one of PIPE_TEX_FILTER_LINEAR, PIPE_TEX_FILTER_NEAREST
 */
void
util_gen_mipmap(struct gen_mipmap_state *ctx,
                struct pipe_texture *pt,
                uint face, uint baseLevel, uint lastLevel, uint filter)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   uint dstLevel;
   uint zslice = 0;

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, pt->format, PIPE_TEXTURE_2D,
                                    PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
      fallback_gen_mipmap(ctx, pt, face, baseLevel, lastLevel);
      return;
   }

   /* save state (restored below) */
   cso_save_blend(ctx->cso);
   cso_save_depth_stencil_alpha(ctx->cso);
   cso_save_rasterizer(ctx->cso);
   cso_save_samplers(ctx->cso);
   cso_save_sampler_textures(ctx->cso);
   cso_save_framebuffer(ctx->cso);
   cso_save_fragment_shader(ctx->cso);
   cso_save_vertex_shader(ctx->cso);
   cso_save_viewport(ctx->cso);

   /* bind our state */
   cso_set_blend(ctx->cso, &ctx->blend);
   cso_set_depth_stencil_alpha(ctx->cso, &ctx->depthstencil);
   cso_set_rasterizer(ctx->cso, &ctx->rasterizer);
   cso_set_viewport(ctx->cso, &ctx->viewport);

   cso_set_fragment_shader_handle(ctx->cso, ctx->fs);
   cso_set_vertex_shader_handle(ctx->cso, ctx->vs);

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));
   fb.num_cbufs = 1;

   /* set min/mag to same filter for faster sw speed */
   ctx->sampler.mag_img_filter = filter;
   ctx->sampler.min_img_filter = filter;

   /*
    * XXX for small mipmap levels, it may be faster to use the software
    * fallback path...
    */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;

      struct pipe_surface *surf = 
         screen->get_tex_surface(screen, pt, face, dstLevel, zslice,
                                 PIPE_BUFFER_USAGE_GPU_WRITE);

      /*
       * Setup framebuffer / dest surface
       */
      fb.cbufs[0] = surf;
      fb.width = pt->width[dstLevel];
      fb.height = pt->height[dstLevel];
      cso_set_framebuffer(ctx->cso, &fb);

      /*
       * Setup sampler state
       * Note: we should only have to set the min/max LOD clamps to ensure
       * we grab texels from the right mipmap level.  But some hardware
       * has trouble with min clamping so we also set the lod_bias to
       * try to work around that.
       */
      ctx->sampler.min_lod = ctx->sampler.max_lod = (float) srcLevel;
      ctx->sampler.lod_bias = (float) srcLevel;
      cso_single_sampler(ctx->cso, 0, &ctx->sampler);
      cso_single_sampler_done(ctx->cso);

      cso_set_sampler_textures(ctx->cso, 1, &pt);

      /* quad coords in window coords (bypassing clipping, viewport mapping) */
      set_vertex_data(ctx,
                      (float) pt->width[dstLevel],
                      (float) pt->height[dstLevel]);
      util_draw_vertex_buffer(ctx->pipe, ctx->vbuf,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

      /* need to signal that the texture has changed _after_ rendering to it */
      pipe_surface_reference( &surf, NULL );
   }

   /* restore state we changed */
   cso_restore_blend(ctx->cso);
   cso_restore_depth_stencil_alpha(ctx->cso);
   cso_restore_rasterizer(ctx->cso);
   cso_restore_samplers(ctx->cso);
   cso_restore_sampler_textures(ctx->cso);
   cso_restore_framebuffer(ctx->cso);
   cso_restore_fragment_shader(ctx->cso);
   cso_restore_vertex_shader(ctx->cso);
   cso_restore_viewport(ctx->cso);
}
