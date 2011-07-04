/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file mipmap.c  mipmap generation and teximage resizing functions.
 */

#include "imports.h"
#include "formats.h"
#include "mipmap.h"
#include "mtypes.h"
#include "teximage.h"
#include "texstore.h"
#include "image.h"
#include "macros.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"



static GLint
bytes_per_pixel(GLenum datatype, GLuint comps)
{
   GLint b = _mesa_sizeof_packed_type(datatype);
   assert(b >= 0);

   if (_mesa_type_is_packed(datatype))
       return b;
   else
       return b * comps;
}


/**
 * \name Support macros for do_row and do_row_3d
 *
 * The macro madness is here for two reasons.  First, it compacts the code
 * slightly.  Second, it makes it much easier to adjust the specifics of the
 * filter to tune the rounding characteristics.
 */
/*@{*/
#define DECLARE_ROW_POINTERS(t, e) \
      const t(*rowA)[e] = (const t(*)[e]) srcRowA; \
      const t(*rowB)[e] = (const t(*)[e]) srcRowB; \
      const t(*rowC)[e] = (const t(*)[e]) srcRowC; \
      const t(*rowD)[e] = (const t(*)[e]) srcRowD; \
      t(*dst)[e] = (t(*)[e]) dstRow

#define DECLARE_ROW_POINTERS0(t) \
      const t *rowA = (const t *) srcRowA; \
      const t *rowB = (const t *) srcRowB; \
      const t *rowC = (const t *) srcRowC; \
      const t *rowD = (const t *) srcRowD; \
      t *dst = (t *) dstRow

#define FILTER_SUM_3D(Aj, Ak, Bj, Bk, Cj, Ck, Dj, Dk) \
   ((unsigned) Aj + (unsigned) Ak \
    + (unsigned) Bj + (unsigned) Bk \
    + (unsigned) Cj + (unsigned) Ck \
    + (unsigned) Dj + (unsigned) Dk \
    + 4) >> 3

#define FILTER_3D(e) \
   do { \
      dst[i][e] = FILTER_SUM_3D(rowA[j][e], rowA[k][e], \
                                rowB[j][e], rowB[k][e], \
                                rowC[j][e], rowC[k][e], \
                                rowD[j][e], rowD[k][e]); \
   } while(0)

#define FILTER_SUM_3D_SIGNED(Aj, Ak, Bj, Bk, Cj, Ck, Dj, Dk) \
   (Aj + Ak \
    + Bj + Bk \
    + Cj + Ck \
    + Dj + Dk \
    + 4) / 8

#define FILTER_3D_SIGNED(e) \
   do { \
      dst[i][e] = FILTER_SUM_3D_SIGNED(rowA[j][e], rowA[k][e], \
                                       rowB[j][e], rowB[k][e], \
                                       rowC[j][e], rowC[k][e], \
                                       rowD[j][e], rowD[k][e]); \
   } while(0)

#define FILTER_F_3D(e) \
   do { \
      dst[i][e] = (rowA[j][e] + rowA[k][e] \
                   + rowB[j][e] + rowB[k][e] \
                   + rowC[j][e] + rowC[k][e] \
                   + rowD[j][e] + rowD[k][e]) * 0.125F; \
   } while(0)

#define FILTER_HF_3D(e) \
   do { \
      const GLfloat aj = _mesa_half_to_float(rowA[j][e]); \
      const GLfloat ak = _mesa_half_to_float(rowA[k][e]); \
      const GLfloat bj = _mesa_half_to_float(rowB[j][e]); \
      const GLfloat bk = _mesa_half_to_float(rowB[k][e]); \
      const GLfloat cj = _mesa_half_to_float(rowC[j][e]); \
      const GLfloat ck = _mesa_half_to_float(rowC[k][e]); \
      const GLfloat dj = _mesa_half_to_float(rowD[j][e]); \
      const GLfloat dk = _mesa_half_to_float(rowD[k][e]); \
      dst[i][e] = _mesa_float_to_half((aj + ak + bj + bk + cj + ck + dj + dk) \
                                      * 0.125F); \
   } while(0)
/*@}*/


/**
 * Average together two rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 * \param datatype  GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, GL_FLOAT, etc.
 * \param comps  number of components per pixel (1..4)
 */
static void
do_row(GLenum datatype, GLuint comps, GLint srcWidth,
       const GLvoid *srcRowA, const GLvoid *srcRowB,
       GLint dstWidth, GLvoid *dstRow)
{
   const GLuint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const GLuint colStride = (srcWidth == dstWidth) ? 1 : 2;

   ASSERT(comps >= 1);
   ASSERT(comps <= 4);

   /* This assertion is no longer valid with non-power-of-2 textures
   assert(srcWidth == dstWidth || srcWidth == 2 * dstWidth);
   */

   if (datatype == GL_UNSIGNED_BYTE && comps == 4) {
      GLuint i, j, k;
      const GLubyte(*rowA)[4] = (const GLubyte(*)[4]) srcRowA;
      const GLubyte(*rowB)[4] = (const GLubyte(*)[4]) srcRowB;
      GLubyte(*dst)[4] = (GLubyte(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == GL_UNSIGNED_BYTE && comps == 3) {
      GLuint i, j, k;
      const GLubyte(*rowA)[3] = (const GLubyte(*)[3]) srcRowA;
      const GLubyte(*rowB)[3] = (const GLubyte(*)[3]) srcRowB;
      GLubyte(*dst)[3] = (GLubyte(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == GL_UNSIGNED_BYTE && comps == 2) {
      GLuint i, j, k;
      const GLubyte(*rowA)[2] = (const GLubyte(*)[2]) srcRowA;
      const GLubyte(*rowB)[2] = (const GLubyte(*)[2]) srcRowB;
      GLubyte(*dst)[2] = (GLubyte(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) >> 2;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) >> 2;
      }
   }
   else if (datatype == GL_UNSIGNED_BYTE && comps == 1) {
      GLuint i, j, k;
      const GLubyte *rowA = (const GLubyte *) srcRowA;
      const GLubyte *rowB = (const GLubyte *) srcRowB;
      GLubyte *dst = (GLubyte *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) >> 2;
      }
   }

   else if (datatype == GL_BYTE && comps == 4) {
      GLuint i, j, k;
      const GLbyte(*rowA)[4] = (const GLbyte(*)[4]) srcRowA;
      const GLbyte(*rowB)[4] = (const GLbyte(*)[4]) srcRowB;
      GLbyte(*dst)[4] = (GLbyte(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == GL_BYTE && comps == 3) {
      GLuint i, j, k;
      const GLbyte(*rowA)[3] = (const GLbyte(*)[3]) srcRowA;
      const GLbyte(*rowB)[3] = (const GLbyte(*)[3]) srcRowB;
      GLbyte(*dst)[3] = (GLbyte(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == GL_BYTE && comps == 2) {
      GLuint i, j, k;
      const GLbyte(*rowA)[2] = (const GLbyte(*)[2]) srcRowA;
      const GLbyte(*rowB)[2] = (const GLbyte(*)[2]) srcRowB;
      GLbyte(*dst)[2] = (GLbyte(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
      }
   }
   else if (datatype == GL_BYTE && comps == 1) {
      GLuint i, j, k;
      const GLbyte *rowA = (const GLbyte *) srcRowA;
      const GLbyte *rowB = (const GLbyte *) srcRowB;
      GLbyte *dst = (GLbyte *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
      }
   }

   else if (datatype == GL_UNSIGNED_SHORT && comps == 4) {
      GLuint i, j, k;
      const GLushort(*rowA)[4] = (const GLushort(*)[4]) srcRowA;
      const GLushort(*rowB)[4] = (const GLushort(*)[4]) srcRowB;
      GLushort(*dst)[4] = (GLushort(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == GL_UNSIGNED_SHORT && comps == 3) {
      GLuint i, j, k;
      const GLushort(*rowA)[3] = (const GLushort(*)[3]) srcRowA;
      const GLushort(*rowB)[3] = (const GLushort(*)[3]) srcRowB;
      GLushort(*dst)[3] = (GLushort(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == GL_UNSIGNED_SHORT && comps == 2) {
      GLuint i, j, k;
      const GLushort(*rowA)[2] = (const GLushort(*)[2]) srcRowA;
      const GLushort(*rowB)[2] = (const GLushort(*)[2]) srcRowB;
      GLushort(*dst)[2] = (GLushort(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
      }
   }
   else if (datatype == GL_UNSIGNED_SHORT && comps == 1) {
      GLuint i, j, k;
      const GLushort *rowA = (const GLushort *) srcRowA;
      const GLushort *rowB = (const GLushort *) srcRowB;
      GLushort *dst = (GLushort *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
      }
   }

   else if (datatype == GL_SHORT && comps == 4) {
      GLuint i, j, k;
      const GLshort(*rowA)[4] = (const GLshort(*)[4]) srcRowA;
      const GLshort(*rowB)[4] = (const GLshort(*)[4]) srcRowB;
      GLshort(*dst)[4] = (GLshort(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
         dst[i][3] = (rowA[j][3] + rowA[k][3] + rowB[j][3] + rowB[k][3]) / 4;
      }
   }
   else if (datatype == GL_SHORT && comps == 3) {
      GLuint i, j, k;
      const GLshort(*rowA)[3] = (const GLshort(*)[3]) srcRowA;
      const GLshort(*rowB)[3] = (const GLshort(*)[3]) srcRowB;
      GLshort(*dst)[3] = (GLshort(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
         dst[i][2] = (rowA[j][2] + rowA[k][2] + rowB[j][2] + rowB[k][2]) / 4;
      }
   }
   else if (datatype == GL_SHORT && comps == 2) {
      GLuint i, j, k;
      const GLshort(*rowA)[2] = (const GLshort(*)[2]) srcRowA;
      const GLshort(*rowB)[2] = (const GLshort(*)[2]) srcRowB;
      GLshort(*dst)[2] = (GLshort(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] + rowB[j][0] + rowB[k][0]) / 4;
         dst[i][1] = (rowA[j][1] + rowA[k][1] + rowB[j][1] + rowB[k][1]) / 4;
      }
   }
   else if (datatype == GL_SHORT && comps == 1) {
      GLuint i, j, k;
      const GLshort *rowA = (const GLshort *) srcRowA;
      const GLshort *rowB = (const GLshort *) srcRowB;
      GLshort *dst = (GLshort *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) / 4;
      }
   }

   else if (datatype == GL_FLOAT && comps == 4) {
      GLuint i, j, k;
      const GLfloat(*rowA)[4] = (const GLfloat(*)[4]) srcRowA;
      const GLfloat(*rowB)[4] = (const GLfloat(*)[4]) srcRowB;
      GLfloat(*dst)[4] = (GLfloat(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
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
   else if (datatype == GL_FLOAT && comps == 3) {
      GLuint i, j, k;
      const GLfloat(*rowA)[3] = (const GLfloat(*)[3]) srcRowA;
      const GLfloat(*rowB)[3] = (const GLfloat(*)[3]) srcRowB;
      GLfloat(*dst)[3] = (GLfloat(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
         dst[i][2] = (rowA[j][2] + rowA[k][2] +
                      rowB[j][2] + rowB[k][2]) * 0.25F;
      }
   }
   else if (datatype == GL_FLOAT && comps == 2) {
      GLuint i, j, k;
      const GLfloat(*rowA)[2] = (const GLfloat(*)[2]) srcRowA;
      const GLfloat(*rowB)[2] = (const GLfloat(*)[2]) srcRowB;
      GLfloat(*dst)[2] = (GLfloat(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i][0] = (rowA[j][0] + rowA[k][0] +
                      rowB[j][0] + rowB[k][0]) * 0.25F;
         dst[i][1] = (rowA[j][1] + rowA[k][1] +
                      rowB[j][1] + rowB[k][1]) * 0.25F;
      }
   }
   else if (datatype == GL_FLOAT && comps == 1) {
      GLuint i, j, k;
      const GLfloat *rowA = (const GLfloat *) srcRowA;
      const GLfloat *rowB = (const GLfloat *) srcRowB;
      GLfloat *dst = (GLfloat *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (rowA[j] + rowA[k] + rowB[j] + rowB[k]) * 0.25F;
      }
   }

   else if (datatype == GL_HALF_FLOAT_ARB && comps == 4) {
      GLuint i, j, k, comp;
      const GLhalfARB(*rowA)[4] = (const GLhalfARB(*)[4]) srcRowA;
      const GLhalfARB(*rowB)[4] = (const GLhalfARB(*)[4]) srcRowB;
      GLhalfARB(*dst)[4] = (GLhalfARB(*)[4]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 4; comp++) {
            GLfloat aj, ak, bj, bk;
            aj = _mesa_half_to_float(rowA[j][comp]);
            ak = _mesa_half_to_float(rowA[k][comp]);
            bj = _mesa_half_to_float(rowB[j][comp]);
            bk = _mesa_half_to_float(rowB[k][comp]);
            dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == GL_HALF_FLOAT_ARB && comps == 3) {
      GLuint i, j, k, comp;
      const GLhalfARB(*rowA)[3] = (const GLhalfARB(*)[3]) srcRowA;
      const GLhalfARB(*rowB)[3] = (const GLhalfARB(*)[3]) srcRowB;
      GLhalfARB(*dst)[3] = (GLhalfARB(*)[3]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 3; comp++) {
            GLfloat aj, ak, bj, bk;
            aj = _mesa_half_to_float(rowA[j][comp]);
            ak = _mesa_half_to_float(rowA[k][comp]);
            bj = _mesa_half_to_float(rowB[j][comp]);
            bk = _mesa_half_to_float(rowB[k][comp]);
            dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == GL_HALF_FLOAT_ARB && comps == 2) {
      GLuint i, j, k, comp;
      const GLhalfARB(*rowA)[2] = (const GLhalfARB(*)[2]) srcRowA;
      const GLhalfARB(*rowB)[2] = (const GLhalfARB(*)[2]) srcRowB;
      GLhalfARB(*dst)[2] = (GLhalfARB(*)[2]) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         for (comp = 0; comp < 2; comp++) {
            GLfloat aj, ak, bj, bk;
            aj = _mesa_half_to_float(rowA[j][comp]);
            ak = _mesa_half_to_float(rowA[k][comp]);
            bj = _mesa_half_to_float(rowB[j][comp]);
            bk = _mesa_half_to_float(rowB[k][comp]);
            dst[i][comp] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
         }
      }
   }
   else if (datatype == GL_HALF_FLOAT_ARB && comps == 1) {
      GLuint i, j, k;
      const GLhalfARB *rowA = (const GLhalfARB *) srcRowA;
      const GLhalfARB *rowB = (const GLhalfARB *) srcRowB;
      GLhalfARB *dst = (GLhalfARB *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         GLfloat aj, ak, bj, bk;
         aj = _mesa_half_to_float(rowA[j]);
         ak = _mesa_half_to_float(rowA[k]);
         bj = _mesa_half_to_float(rowB[j]);
         bk = _mesa_half_to_float(rowB[k]);
         dst[i] = _mesa_float_to_half((aj + ak + bj + bk) * 0.25F);
      }
   }

   else if (datatype == GL_UNSIGNED_INT && comps == 1) {
      GLuint i, j, k;
      const GLuint *rowA = (const GLuint *) srcRowA;
      const GLuint *rowB = (const GLuint *) srcRowB;
      GLuint *dst = (GLuint *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         dst[i] = (GLfloat)(rowA[j] / 4 + rowA[k] / 4 + rowB[j] / 4 + rowB[k] / 4);
      }
   }

   else if (datatype == GL_UNSIGNED_SHORT_5_6_5 && comps == 3) {
      GLuint i, j, k;
      const GLushort *rowA = (const GLushort *) srcRowA;
      const GLushort *rowB = (const GLushort *) srcRowB;
      GLushort *dst = (GLushort *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x1f;
         const GLint rowAr1 = rowA[k] & 0x1f;
         const GLint rowBr0 = rowB[j] & 0x1f;
         const GLint rowBr1 = rowB[k] & 0x1f;
         const GLint rowAg0 = (rowA[j] >> 5) & 0x3f;
         const GLint rowAg1 = (rowA[k] >> 5) & 0x3f;
         const GLint rowBg0 = (rowB[j] >> 5) & 0x3f;
         const GLint rowBg1 = (rowB[k] >> 5) & 0x3f;
         const GLint rowAb0 = (rowA[j] >> 11) & 0x1f;
         const GLint rowAb1 = (rowA[k] >> 11) & 0x1f;
         const GLint rowBb0 = (rowB[j] >> 11) & 0x1f;
         const GLint rowBb1 = (rowB[k] >> 11) & 0x1f;
         const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         dst[i] = (blue << 11) | (green << 5) | red;
      }
   }
   else if (datatype == GL_UNSIGNED_SHORT_4_4_4_4 && comps == 4) {
      GLuint i, j, k;
      const GLushort *rowA = (const GLushort *) srcRowA;
      const GLushort *rowB = (const GLushort *) srcRowB;
      GLushort *dst = (GLushort *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0xf;
         const GLint rowAr1 = rowA[k] & 0xf;
         const GLint rowBr0 = rowB[j] & 0xf;
         const GLint rowBr1 = rowB[k] & 0xf;
         const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
         const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
         const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
         const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
         const GLint rowAb0 = (rowA[j] >> 8) & 0xf;
         const GLint rowAb1 = (rowA[k] >> 8) & 0xf;
         const GLint rowBb0 = (rowB[j] >> 8) & 0xf;
         const GLint rowBb1 = (rowB[k] >> 8) & 0xf;
         const GLint rowAa0 = (rowA[j] >> 12) & 0xf;
         const GLint rowAa1 = (rowA[k] >> 12) & 0xf;
         const GLint rowBa0 = (rowB[j] >> 12) & 0xf;
         const GLint rowBa1 = (rowB[k] >> 12) & 0xf;
         const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 12) | (blue << 8) | (green << 4) | red;
      }
   }
   else if (datatype == GL_UNSIGNED_SHORT_1_5_5_5_REV && comps == 4) {
      GLuint i, j, k;
      const GLushort *rowA = (const GLushort *) srcRowA;
      const GLushort *rowB = (const GLushort *) srcRowB;
      GLushort *dst = (GLushort *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x1f;
         const GLint rowAr1 = rowA[k] & 0x1f;
         const GLint rowBr0 = rowB[j] & 0x1f;
         const GLint rowBr1 = rowB[k] & 0x1f;
         const GLint rowAg0 = (rowA[j] >> 5) & 0x1f;
         const GLint rowAg1 = (rowA[k] >> 5) & 0x1f;
         const GLint rowBg0 = (rowB[j] >> 5) & 0x1f;
         const GLint rowBg1 = (rowB[k] >> 5) & 0x1f;
         const GLint rowAb0 = (rowA[j] >> 10) & 0x1f;
         const GLint rowAb1 = (rowA[k] >> 10) & 0x1f;
         const GLint rowBb0 = (rowB[j] >> 10) & 0x1f;
         const GLint rowBb1 = (rowB[k] >> 10) & 0x1f;
         const GLint rowAa0 = (rowA[j] >> 15) & 0x1;
         const GLint rowAa1 = (rowA[k] >> 15) & 0x1;
         const GLint rowBa0 = (rowB[j] >> 15) & 0x1;
         const GLint rowBa1 = (rowB[k] >> 15) & 0x1;
         const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 15) | (blue << 10) | (green << 5) | red;
      }
   }
   else if (datatype == GL_UNSIGNED_SHORT_5_5_5_1 && comps == 4) {
      GLuint i, j, k;
      const GLushort *rowA = (const GLushort *) srcRowA;
      const GLushort *rowB = (const GLushort *) srcRowB;
      GLushort *dst = (GLushort *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = (rowA[j] >> 11) & 0x1f;
         const GLint rowAr1 = (rowA[k] >> 11) & 0x1f;
         const GLint rowBr0 = (rowB[j] >> 11) & 0x1f;
         const GLint rowBr1 = (rowB[k] >> 11) & 0x1f;
         const GLint rowAg0 = (rowA[j] >> 6) & 0x1f;
         const GLint rowAg1 = (rowA[k] >> 6) & 0x1f;
         const GLint rowBg0 = (rowB[j] >> 6) & 0x1f;
         const GLint rowBg1 = (rowB[k] >> 6) & 0x1f;
         const GLint rowAb0 = (rowA[j] >> 1) & 0x1f;
         const GLint rowAb1 = (rowA[k] >> 1) & 0x1f;
         const GLint rowBb0 = (rowB[j] >> 1) & 0x1f;
         const GLint rowBb1 = (rowB[k] >> 1) & 0x1f;
         const GLint rowAa0 = (rowA[j] & 0x1);
         const GLint rowAa1 = (rowA[k] & 0x1);
         const GLint rowBa0 = (rowB[j] & 0x1);
         const GLint rowBa1 = (rowB[k] & 0x1);
         const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (red << 11) | (green << 6) | (blue << 1) | alpha;
      }
   }

   else if (datatype == GL_UNSIGNED_BYTE_3_3_2 && comps == 3) {
      GLuint i, j, k;
      const GLubyte *rowA = (const GLubyte *) srcRowA;
      const GLubyte *rowB = (const GLubyte *) srcRowB;
      GLubyte *dst = (GLubyte *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x3;
         const GLint rowAr1 = rowA[k] & 0x3;
         const GLint rowBr0 = rowB[j] & 0x3;
         const GLint rowBr1 = rowB[k] & 0x3;
         const GLint rowAg0 = (rowA[j] >> 2) & 0x7;
         const GLint rowAg1 = (rowA[k] >> 2) & 0x7;
         const GLint rowBg0 = (rowB[j] >> 2) & 0x7;
         const GLint rowBg1 = (rowB[k] >> 2) & 0x7;
         const GLint rowAb0 = (rowA[j] >> 5) & 0x7;
         const GLint rowAb1 = (rowA[k] >> 5) & 0x7;
         const GLint rowBb0 = (rowB[j] >> 5) & 0x7;
         const GLint rowBb1 = (rowB[k] >> 5) & 0x7;
         const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         dst[i] = (blue << 5) | (green << 2) | red;
      }
   }

   else if (datatype == MESA_UNSIGNED_BYTE_4_4 && comps == 2) {
      GLuint i, j, k;
      const GLubyte *rowA = (const GLubyte *) srcRowA;
      const GLubyte *rowB = (const GLubyte *) srcRowB;
      GLubyte *dst = (GLubyte *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0xf;
         const GLint rowAr1 = rowA[k] & 0xf;
         const GLint rowBr0 = rowB[j] & 0xf;
         const GLint rowBr1 = rowB[k] & 0xf;
         const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
         const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
         const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
         const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
         const GLint r = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint g = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         dst[i] = (g << 4) | r;
      }
   }

   else if (datatype == GL_UNSIGNED_INT_2_10_10_10_REV && comps == 4) {
      GLuint i, j, k;
      const GLuint *rowA = (const GLuint *) srcRowA;
      const GLuint *rowB = (const GLuint *) srcRowB;
      GLuint *dst = (GLuint *) dstRow;
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x3ff;
         const GLint rowAr1 = rowA[k] & 0x3ff;
         const GLint rowBr0 = rowB[j] & 0x3ff;
         const GLint rowBr1 = rowB[k] & 0x3ff;
         const GLint rowAg0 = (rowA[j] >> 10) & 0x3ff;
         const GLint rowAg1 = (rowA[k] >> 10) & 0x3ff;
         const GLint rowBg0 = (rowB[j] >> 10) & 0x3ff;
         const GLint rowBg1 = (rowB[k] >> 10) & 0x3ff;
         const GLint rowAb0 = (rowA[j] >> 20) & 0x3ff;
         const GLint rowAb1 = (rowA[k] >> 20) & 0x3ff;
         const GLint rowBb0 = (rowB[j] >> 20) & 0x3ff;
         const GLint rowBb1 = (rowB[k] >> 20) & 0x3ff;
         const GLint rowAa0 = (rowA[j] >> 30) & 0x3;
         const GLint rowAa1 = (rowA[k] >> 30) & 0x3;
         const GLint rowBa0 = (rowB[j] >> 30) & 0x3;
         const GLint rowBa1 = (rowB[k] >> 30) & 0x3;
         const GLint red = (rowAr0 + rowAr1 + rowBr0 + rowBr1) >> 2;
         const GLint green = (rowAg0 + rowAg1 + rowBg0 + rowBg1) >> 2;
         const GLint blue = (rowAb0 + rowAb1 + rowBb0 + rowBb1) >> 2;
         const GLint alpha = (rowAa0 + rowAa1 + rowBa0 + rowBa1) >> 2;
         dst[i] = (alpha << 30) | (blue << 20) | (green << 10) | red;
      }
   }

   else if (datatype == GL_UNSIGNED_INT_5_9_9_9_REV && comps == 3) {
      GLuint i, j, k;
      const GLuint *rowA = (const GLuint*) srcRowA;
      const GLuint *rowB = (const GLuint*) srcRowB;
      GLuint *dst = (GLuint*)dstRow;
      GLfloat res[3], rowAj[3], rowBj[3], rowAk[3], rowBk[3];
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         rgb9e5_to_float3(rowA[j], rowAj);
         rgb9e5_to_float3(rowB[j], rowBj);
         rgb9e5_to_float3(rowA[k], rowAk);
         rgb9e5_to_float3(rowB[k], rowBk);
         res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0]) * 0.25F;
         res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1]) * 0.25F;
         res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2]) * 0.25F;
         dst[i] = float3_to_rgb9e5(res);
      }
   }

   else if (datatype == GL_UNSIGNED_INT_10F_11F_11F_REV && comps == 3) {
      GLuint i, j, k;
      const GLuint *rowA = (const GLuint*) srcRowA;
      const GLuint *rowB = (const GLuint*) srcRowB;
      GLuint *dst = (GLuint*)dstRow;
      GLfloat res[3], rowAj[3], rowBj[3], rowAk[3], rowBk[3];
      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         r11g11b10f_to_float3(rowA[j], rowAj);
         r11g11b10f_to_float3(rowB[j], rowBj);
         r11g11b10f_to_float3(rowA[k], rowAk);
         r11g11b10f_to_float3(rowB[k], rowBk);
         res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0]) * 0.25F;
         res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1]) * 0.25F;
         res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2]) * 0.25F;
         dst[i] = float3_to_r11g11b10f(res);
      }
   }

   else {
      _mesa_problem(NULL, "bad format in do_row()");
   }
}


/**
 * Average together four rows of a source image to produce a single new
 * row in the dest image.  It's legal for the two source rows to point
 * to the same data.  The source width must be equal to either the
 * dest width or two times the dest width.
 *
 * \param datatype  GL pixel type \c GL_UNSIGNED_BYTE, \c GL_UNSIGNED_SHORT,
 *                  \c GL_FLOAT, etc.
 * \param comps     number of components per pixel (1..4)
 * \param srcWidth  Width of a row in the source data
 * \param srcRowA   Pointer to one of the rows of source data
 * \param srcRowB   Pointer to one of the rows of source data
 * \param srcRowC   Pointer to one of the rows of source data
 * \param srcRowD   Pointer to one of the rows of source data
 * \param dstWidth  Width of a row in the destination data
 * \param srcRowA   Pointer to the row of destination data
 */
static void
do_row_3D(GLenum datatype, GLuint comps, GLint srcWidth,
          const GLvoid *srcRowA, const GLvoid *srcRowB,
          const GLvoid *srcRowC, const GLvoid *srcRowD,
          GLint dstWidth, GLvoid *dstRow)
{
   const GLuint k0 = (srcWidth == dstWidth) ? 0 : 1;
   const GLuint colStride = (srcWidth == dstWidth) ? 1 : 2;
   GLuint i, j, k;

   ASSERT(comps >= 1);
   ASSERT(comps <= 4);

   if ((datatype == GL_UNSIGNED_BYTE) && (comps == 4)) {
      DECLARE_ROW_POINTERS(GLubyte, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
         FILTER_3D(3);
      }
   }
   else if ((datatype == GL_UNSIGNED_BYTE) && (comps == 3)) {
      DECLARE_ROW_POINTERS(GLubyte, 3);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
      }
   }
   else if ((datatype == GL_UNSIGNED_BYTE) && (comps == 2)) {
      DECLARE_ROW_POINTERS(GLubyte, 2);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
      }
   }
   else if ((datatype == GL_UNSIGNED_BYTE) && (comps == 1)) {
      DECLARE_ROW_POINTERS(GLubyte, 1);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
      }
   }
   else if ((datatype == GL_BYTE) && (comps == 4)) {
      DECLARE_ROW_POINTERS(GLbyte, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D_SIGNED(0);
         FILTER_3D_SIGNED(1);
         FILTER_3D_SIGNED(2);
         FILTER_3D_SIGNED(3);
      }
   }
   else if ((datatype == GL_BYTE) && (comps == 3)) {
      DECLARE_ROW_POINTERS(GLbyte, 3);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D_SIGNED(0);
         FILTER_3D_SIGNED(1);
         FILTER_3D_SIGNED(2);
      }
   }
   else if ((datatype == GL_BYTE) && (comps == 2)) {
      DECLARE_ROW_POINTERS(GLbyte, 2);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D_SIGNED(0);
         FILTER_3D_SIGNED(1);
       }
   }
   else if ((datatype == GL_BYTE) && (comps == 1)) {
      DECLARE_ROW_POINTERS(GLbyte, 1);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D_SIGNED(0);
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 4)) {
      DECLARE_ROW_POINTERS(GLushort, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
         FILTER_3D(3);
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 3)) {
      DECLARE_ROW_POINTERS(GLushort, 3);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 2)) {
      DECLARE_ROW_POINTERS(GLushort, 2);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT) && (comps == 1)) {
      DECLARE_ROW_POINTERS(GLushort, 1);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
      }
   }
   else if ((datatype == GL_SHORT) && (comps == 4)) {
      DECLARE_ROW_POINTERS(GLshort, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
         FILTER_3D(3);
      }
   }
   else if ((datatype == GL_SHORT) && (comps == 3)) {
      DECLARE_ROW_POINTERS(GLshort, 3);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
         FILTER_3D(2);
      }
   }
   else if ((datatype == GL_SHORT) && (comps == 2)) {
      DECLARE_ROW_POINTERS(GLshort, 2);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
         FILTER_3D(1);
      }
   }
   else if ((datatype == GL_SHORT) && (comps == 1)) {
      DECLARE_ROW_POINTERS(GLshort, 1);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_3D(0);
      }
   }
   else if ((datatype == GL_FLOAT) && (comps == 4)) {
      DECLARE_ROW_POINTERS(GLfloat, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
         FILTER_F_3D(1);
         FILTER_F_3D(2);
         FILTER_F_3D(3);
      }
   }
   else if ((datatype == GL_FLOAT) && (comps == 3)) {
      DECLARE_ROW_POINTERS(GLfloat, 3);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
         FILTER_F_3D(1);
         FILTER_F_3D(2);
      }
   }
   else if ((datatype == GL_FLOAT) && (comps == 2)) {
      DECLARE_ROW_POINTERS(GLfloat, 2);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
         FILTER_F_3D(1);
      }
   }
   else if ((datatype == GL_FLOAT) && (comps == 1)) {
      DECLARE_ROW_POINTERS(GLfloat, 1);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_F_3D(0);
      }
   }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 4)) {
      DECLARE_ROW_POINTERS(GLhalfARB, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
         FILTER_HF_3D(1);
         FILTER_HF_3D(2);
         FILTER_HF_3D(3);
      }
   }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 3)) {
      DECLARE_ROW_POINTERS(GLhalfARB, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
         FILTER_HF_3D(1);
         FILTER_HF_3D(2);
      }
   }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 2)) {
      DECLARE_ROW_POINTERS(GLhalfARB, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
         FILTER_HF_3D(1);
      }
   }
   else if ((datatype == GL_HALF_FLOAT_ARB) && (comps == 1)) {
      DECLARE_ROW_POINTERS(GLhalfARB, 4);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         FILTER_HF_3D(0);
      }
   }
   else if ((datatype == GL_UNSIGNED_INT) && (comps == 1)) {
      const GLuint *rowA = (const GLuint *) srcRowA;
      const GLuint *rowB = (const GLuint *) srcRowB;
      const GLuint *rowC = (const GLuint *) srcRowC;
      const GLuint *rowD = (const GLuint *) srcRowD;
      GLfloat *dst = (GLfloat *) dstRow;

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const uint64_t tmp = (((uint64_t) rowA[j] + (uint64_t) rowA[k])
                               + ((uint64_t) rowB[j] + (uint64_t) rowB[k])
                               + ((uint64_t) rowC[j] + (uint64_t) rowC[k])
                               + ((uint64_t) rowD[j] + (uint64_t) rowD[k]));
         dst[i] = (GLfloat)((double) tmp * 0.125);
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT_5_6_5) && (comps == 3)) {
      DECLARE_ROW_POINTERS0(GLushort);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x1f;
         const GLint rowAr1 = rowA[k] & 0x1f;
         const GLint rowBr0 = rowB[j] & 0x1f;
         const GLint rowBr1 = rowB[k] & 0x1f;
         const GLint rowCr0 = rowC[j] & 0x1f;
         const GLint rowCr1 = rowC[k] & 0x1f;
         const GLint rowDr0 = rowD[j] & 0x1f;
         const GLint rowDr1 = rowD[k] & 0x1f;
         const GLint rowAg0 = (rowA[j] >> 5) & 0x3f;
         const GLint rowAg1 = (rowA[k] >> 5) & 0x3f;
         const GLint rowBg0 = (rowB[j] >> 5) & 0x3f;
         const GLint rowBg1 = (rowB[k] >> 5) & 0x3f;
         const GLint rowCg0 = (rowC[j] >> 5) & 0x3f;
         const GLint rowCg1 = (rowC[k] >> 5) & 0x3f;
         const GLint rowDg0 = (rowD[j] >> 5) & 0x3f;
         const GLint rowDg1 = (rowD[k] >> 5) & 0x3f;
         const GLint rowAb0 = (rowA[j] >> 11) & 0x1f;
         const GLint rowAb1 = (rowA[k] >> 11) & 0x1f;
         const GLint rowBb0 = (rowB[j] >> 11) & 0x1f;
         const GLint rowBb1 = (rowB[k] >> 11) & 0x1f;
         const GLint rowCb0 = (rowC[j] >> 11) & 0x1f;
         const GLint rowCb1 = (rowC[k] >> 11) & 0x1f;
         const GLint rowDb0 = (rowD[j] >> 11) & 0x1f;
         const GLint rowDb1 = (rowD[k] >> 11) & 0x1f;
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         dst[i] = (b << 11) | (g << 5) | r;
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT_4_4_4_4) && (comps == 4)) {
      DECLARE_ROW_POINTERS0(GLushort);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0xf;
         const GLint rowAr1 = rowA[k] & 0xf;
         const GLint rowBr0 = rowB[j] & 0xf;
         const GLint rowBr1 = rowB[k] & 0xf;
         const GLint rowCr0 = rowC[j] & 0xf;
         const GLint rowCr1 = rowC[k] & 0xf;
         const GLint rowDr0 = rowD[j] & 0xf;
         const GLint rowDr1 = rowD[k] & 0xf;
         const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
         const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
         const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
         const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
         const GLint rowCg0 = (rowC[j] >> 4) & 0xf;
         const GLint rowCg1 = (rowC[k] >> 4) & 0xf;
         const GLint rowDg0 = (rowD[j] >> 4) & 0xf;
         const GLint rowDg1 = (rowD[k] >> 4) & 0xf;
         const GLint rowAb0 = (rowA[j] >> 8) & 0xf;
         const GLint rowAb1 = (rowA[k] >> 8) & 0xf;
         const GLint rowBb0 = (rowB[j] >> 8) & 0xf;
         const GLint rowBb1 = (rowB[k] >> 8) & 0xf;
         const GLint rowCb0 = (rowC[j] >> 8) & 0xf;
         const GLint rowCb1 = (rowC[k] >> 8) & 0xf;
         const GLint rowDb0 = (rowD[j] >> 8) & 0xf;
         const GLint rowDb1 = (rowD[k] >> 8) & 0xf;
         const GLint rowAa0 = (rowA[j] >> 12) & 0xf;
         const GLint rowAa1 = (rowA[k] >> 12) & 0xf;
         const GLint rowBa0 = (rowB[j] >> 12) & 0xf;
         const GLint rowBa1 = (rowB[k] >> 12) & 0xf;
         const GLint rowCa0 = (rowC[j] >> 12) & 0xf;
         const GLint rowCa1 = (rowC[k] >> 12) & 0xf;
         const GLint rowDa0 = (rowD[j] >> 12) & 0xf;
         const GLint rowDa1 = (rowD[k] >> 12) & 0xf;
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         const GLint a = FILTER_SUM_3D(rowAa0, rowAa1, rowBa0, rowBa1,
                                       rowCa0, rowCa1, rowDa0, rowDa1);

         dst[i] = (a << 12) | (b << 8) | (g << 4) | r;
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT_1_5_5_5_REV) && (comps == 4)) {
      DECLARE_ROW_POINTERS0(GLushort);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x1f;
         const GLint rowAr1 = rowA[k] & 0x1f;
         const GLint rowBr0 = rowB[j] & 0x1f;
         const GLint rowBr1 = rowB[k] & 0x1f;
         const GLint rowCr0 = rowC[j] & 0x1f;
         const GLint rowCr1 = rowC[k] & 0x1f;
         const GLint rowDr0 = rowD[j] & 0x1f;
         const GLint rowDr1 = rowD[k] & 0x1f;
         const GLint rowAg0 = (rowA[j] >> 5) & 0x1f;
         const GLint rowAg1 = (rowA[k] >> 5) & 0x1f;
         const GLint rowBg0 = (rowB[j] >> 5) & 0x1f;
         const GLint rowBg1 = (rowB[k] >> 5) & 0x1f;
         const GLint rowCg0 = (rowC[j] >> 5) & 0x1f;
         const GLint rowCg1 = (rowC[k] >> 5) & 0x1f;
         const GLint rowDg0 = (rowD[j] >> 5) & 0x1f;
         const GLint rowDg1 = (rowD[k] >> 5) & 0x1f;
         const GLint rowAb0 = (rowA[j] >> 10) & 0x1f;
         const GLint rowAb1 = (rowA[k] >> 10) & 0x1f;
         const GLint rowBb0 = (rowB[j] >> 10) & 0x1f;
         const GLint rowBb1 = (rowB[k] >> 10) & 0x1f;
         const GLint rowCb0 = (rowC[j] >> 10) & 0x1f;
         const GLint rowCb1 = (rowC[k] >> 10) & 0x1f;
         const GLint rowDb0 = (rowD[j] >> 10) & 0x1f;
         const GLint rowDb1 = (rowD[k] >> 10) & 0x1f;
         const GLint rowAa0 = (rowA[j] >> 15) & 0x1;
         const GLint rowAa1 = (rowA[k] >> 15) & 0x1;
         const GLint rowBa0 = (rowB[j] >> 15) & 0x1;
         const GLint rowBa1 = (rowB[k] >> 15) & 0x1;
         const GLint rowCa0 = (rowC[j] >> 15) & 0x1;
         const GLint rowCa1 = (rowC[k] >> 15) & 0x1;
         const GLint rowDa0 = (rowD[j] >> 15) & 0x1;
         const GLint rowDa1 = (rowD[k] >> 15) & 0x1;
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         const GLint a = FILTER_SUM_3D(rowAa0, rowAa1, rowBa0, rowBa1,
                                       rowCa0, rowCa1, rowDa0, rowDa1);

         dst[i] = (a << 15) | (b << 10) | (g << 5) | r;
      }
   }
   else if ((datatype == GL_UNSIGNED_SHORT_5_5_5_1) && (comps == 4)) {
      DECLARE_ROW_POINTERS0(GLushort);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = (rowA[j] >> 11) & 0x1f;
         const GLint rowAr1 = (rowA[k] >> 11) & 0x1f;
         const GLint rowBr0 = (rowB[j] >> 11) & 0x1f;
         const GLint rowBr1 = (rowB[k] >> 11) & 0x1f;
         const GLint rowCr0 = (rowC[j] >> 11) & 0x1f;
         const GLint rowCr1 = (rowC[k] >> 11) & 0x1f;
         const GLint rowDr0 = (rowD[j] >> 11) & 0x1f;
         const GLint rowDr1 = (rowD[k] >> 11) & 0x1f;
         const GLint rowAg0 = (rowA[j] >> 6) & 0x1f;
         const GLint rowAg1 = (rowA[k] >> 6) & 0x1f;
         const GLint rowBg0 = (rowB[j] >> 6) & 0x1f;
         const GLint rowBg1 = (rowB[k] >> 6) & 0x1f;
         const GLint rowCg0 = (rowC[j] >> 6) & 0x1f;
         const GLint rowCg1 = (rowC[k] >> 6) & 0x1f;
         const GLint rowDg0 = (rowD[j] >> 6) & 0x1f;
         const GLint rowDg1 = (rowD[k] >> 6) & 0x1f;
         const GLint rowAb0 = (rowA[j] >> 1) & 0x1f;
         const GLint rowAb1 = (rowA[k] >> 1) & 0x1f;
         const GLint rowBb0 = (rowB[j] >> 1) & 0x1f;
         const GLint rowBb1 = (rowB[k] >> 1) & 0x1f;
         const GLint rowCb0 = (rowC[j] >> 1) & 0x1f;
         const GLint rowCb1 = (rowC[k] >> 1) & 0x1f;
         const GLint rowDb0 = (rowD[j] >> 1) & 0x1f;
         const GLint rowDb1 = (rowD[k] >> 1) & 0x1f;
         const GLint rowAa0 = (rowA[j] & 0x1);
         const GLint rowAa1 = (rowA[k] & 0x1);
         const GLint rowBa0 = (rowB[j] & 0x1);
         const GLint rowBa1 = (rowB[k] & 0x1);
         const GLint rowCa0 = (rowC[j] & 0x1);
         const GLint rowCa1 = (rowC[k] & 0x1);
         const GLint rowDa0 = (rowD[j] & 0x1);
         const GLint rowDa1 = (rowD[k] & 0x1);
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         const GLint a = FILTER_SUM_3D(rowAa0, rowAa1, rowBa0, rowBa1,
                                       rowCa0, rowCa1, rowDa0, rowDa1);

         dst[i] = (r << 11) | (g << 6) | (b << 1) | a;
      }
   }
   else if ((datatype == GL_UNSIGNED_BYTE_3_3_2) && (comps == 3)) {
      DECLARE_ROW_POINTERS0(GLubyte);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x3;
         const GLint rowAr1 = rowA[k] & 0x3;
         const GLint rowBr0 = rowB[j] & 0x3;
         const GLint rowBr1 = rowB[k] & 0x3;
         const GLint rowCr0 = rowC[j] & 0x3;
         const GLint rowCr1 = rowC[k] & 0x3;
         const GLint rowDr0 = rowD[j] & 0x3;
         const GLint rowDr1 = rowD[k] & 0x3;
         const GLint rowAg0 = (rowA[j] >> 2) & 0x7;
         const GLint rowAg1 = (rowA[k] >> 2) & 0x7;
         const GLint rowBg0 = (rowB[j] >> 2) & 0x7;
         const GLint rowBg1 = (rowB[k] >> 2) & 0x7;
         const GLint rowCg0 = (rowC[j] >> 2) & 0x7;
         const GLint rowCg1 = (rowC[k] >> 2) & 0x7;
         const GLint rowDg0 = (rowD[j] >> 2) & 0x7;
         const GLint rowDg1 = (rowD[k] >> 2) & 0x7;
         const GLint rowAb0 = (rowA[j] >> 5) & 0x7;
         const GLint rowAb1 = (rowA[k] >> 5) & 0x7;
         const GLint rowBb0 = (rowB[j] >> 5) & 0x7;
         const GLint rowBb1 = (rowB[k] >> 5) & 0x7;
         const GLint rowCb0 = (rowC[j] >> 5) & 0x7;
         const GLint rowCb1 = (rowC[k] >> 5) & 0x7;
         const GLint rowDb0 = (rowD[j] >> 5) & 0x7;
         const GLint rowDb1 = (rowD[k] >> 5) & 0x7;
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         dst[i] = (b << 5) | (g << 2) | r;
      }
   }
   else if (datatype == MESA_UNSIGNED_BYTE_4_4 && comps == 2) {
      DECLARE_ROW_POINTERS0(GLubyte);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0xf;
         const GLint rowAr1 = rowA[k] & 0xf;
         const GLint rowBr0 = rowB[j] & 0xf;
         const GLint rowBr1 = rowB[k] & 0xf;
         const GLint rowCr0 = rowC[j] & 0xf;
         const GLint rowCr1 = rowC[k] & 0xf;
         const GLint rowDr0 = rowD[j] & 0xf;
         const GLint rowDr1 = rowD[k] & 0xf;
         const GLint rowAg0 = (rowA[j] >> 4) & 0xf;
         const GLint rowAg1 = (rowA[k] >> 4) & 0xf;
         const GLint rowBg0 = (rowB[j] >> 4) & 0xf;
         const GLint rowBg1 = (rowB[k] >> 4) & 0xf;
         const GLint rowCg0 = (rowC[j] >> 4) & 0xf;
         const GLint rowCg1 = (rowC[k] >> 4) & 0xf;
         const GLint rowDg0 = (rowD[j] >> 4) & 0xf;
         const GLint rowDg1 = (rowD[k] >> 4) & 0xf;
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         dst[i] = (g << 4) | r;
      }
   }
   else if ((datatype == GL_UNSIGNED_INT_2_10_10_10_REV) && (comps == 4)) {
      DECLARE_ROW_POINTERS0(GLuint);

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         const GLint rowAr0 = rowA[j] & 0x3ff;
         const GLint rowAr1 = rowA[k] & 0x3ff;
         const GLint rowBr0 = rowB[j] & 0x3ff;
         const GLint rowBr1 = rowB[k] & 0x3ff;
         const GLint rowCr0 = rowC[j] & 0x3ff;
         const GLint rowCr1 = rowC[k] & 0x3ff;
         const GLint rowDr0 = rowD[j] & 0x3ff;
         const GLint rowDr1 = rowD[k] & 0x3ff;
         const GLint rowAg0 = (rowA[j] >> 10) & 0x3ff;
         const GLint rowAg1 = (rowA[k] >> 10) & 0x3ff;
         const GLint rowBg0 = (rowB[j] >> 10) & 0x3ff;
         const GLint rowBg1 = (rowB[k] >> 10) & 0x3ff;
         const GLint rowCg0 = (rowC[j] >> 10) & 0x3ff;
         const GLint rowCg1 = (rowC[k] >> 10) & 0x3ff;
         const GLint rowDg0 = (rowD[j] >> 10) & 0x3ff;
         const GLint rowDg1 = (rowD[k] >> 10) & 0x3ff;
         const GLint rowAb0 = (rowA[j] >> 20) & 0x3ff;
         const GLint rowAb1 = (rowA[k] >> 20) & 0x3ff;
         const GLint rowBb0 = (rowB[j] >> 20) & 0x3ff;
         const GLint rowBb1 = (rowB[k] >> 20) & 0x3ff;
         const GLint rowCb0 = (rowC[j] >> 20) & 0x3ff;
         const GLint rowCb1 = (rowC[k] >> 20) & 0x3ff;
         const GLint rowDb0 = (rowD[j] >> 20) & 0x3ff;
         const GLint rowDb1 = (rowD[k] >> 20) & 0x3ff;
         const GLint rowAa0 = (rowA[j] >> 30) & 0x3;
         const GLint rowAa1 = (rowA[k] >> 30) & 0x3;
         const GLint rowBa0 = (rowB[j] >> 30) & 0x3;
         const GLint rowBa1 = (rowB[k] >> 30) & 0x3;
         const GLint rowCa0 = (rowC[j] >> 30) & 0x3;
         const GLint rowCa1 = (rowC[k] >> 30) & 0x3;
         const GLint rowDa0 = (rowD[j] >> 30) & 0x3;
         const GLint rowDa1 = (rowD[k] >> 30) & 0x3;
         const GLint r = FILTER_SUM_3D(rowAr0, rowAr1, rowBr0, rowBr1,
                                       rowCr0, rowCr1, rowDr0, rowDr1);
         const GLint g = FILTER_SUM_3D(rowAg0, rowAg1, rowBg0, rowBg1,
                                       rowCg0, rowCg1, rowDg0, rowDg1);
         const GLint b = FILTER_SUM_3D(rowAb0, rowAb1, rowBb0, rowBb1,
                                       rowCb0, rowCb1, rowDb0, rowDb1);
         const GLint a = FILTER_SUM_3D(rowAa0, rowAa1, rowBa0, rowBa1,
                                       rowCa0, rowCa1, rowDa0, rowDa1);

         dst[i] = (a << 30) | (b << 20) | (g << 10) | r;
      }
   }

   else if (datatype == GL_UNSIGNED_INT_5_9_9_9_REV && comps == 3) {
      DECLARE_ROW_POINTERS0(GLuint);

      GLfloat res[3];
      GLfloat rowAj[3], rowBj[3], rowCj[3], rowDj[3];
      GLfloat rowAk[3], rowBk[3], rowCk[3], rowDk[3];

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         rgb9e5_to_float3(rowA[j], rowAj);
         rgb9e5_to_float3(rowB[j], rowBj);
         rgb9e5_to_float3(rowC[j], rowCj);
         rgb9e5_to_float3(rowD[j], rowDj);
         rgb9e5_to_float3(rowA[k], rowAk);
         rgb9e5_to_float3(rowB[k], rowBk);
         rgb9e5_to_float3(rowC[k], rowCk);
         rgb9e5_to_float3(rowD[k], rowDk);
         res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0] +
                   rowCj[0] + rowCk[0] + rowDj[0] + rowDk[0]) * 0.125F;
         res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1] +
                   rowCj[1] + rowCk[1] + rowDj[1] + rowDk[1]) * 0.125F;
         res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2] +
                   rowCj[2] + rowCk[2] + rowDj[2] + rowDk[2]) * 0.125F;
         dst[i] = float3_to_rgb9e5(res);
      }
   }

   else if (datatype == GL_UNSIGNED_INT_10F_11F_11F_REV && comps == 3) {
      DECLARE_ROW_POINTERS0(GLuint);

      GLfloat res[3];
      GLfloat rowAj[3], rowBj[3], rowCj[3], rowDj[3];
      GLfloat rowAk[3], rowBk[3], rowCk[3], rowDk[3];

      for (i = j = 0, k = k0; i < (GLuint) dstWidth;
           i++, j += colStride, k += colStride) {
         r11g11b10f_to_float3(rowA[j], rowAj);
         r11g11b10f_to_float3(rowB[j], rowBj);
         r11g11b10f_to_float3(rowC[j], rowCj);
         r11g11b10f_to_float3(rowD[j], rowDj);
         r11g11b10f_to_float3(rowA[k], rowAk);
         r11g11b10f_to_float3(rowB[k], rowBk);
         r11g11b10f_to_float3(rowC[k], rowCk);
         r11g11b10f_to_float3(rowD[k], rowDk);
         res[0] = (rowAj[0] + rowAk[0] + rowBj[0] + rowBk[0] +
                   rowCj[0] + rowCk[0] + rowDj[0] + rowDk[0]) * 0.125F;
         res[1] = (rowAj[1] + rowAk[1] + rowBj[1] + rowBk[1] +
                   rowCj[1] + rowCk[1] + rowDj[1] + rowDk[1]) * 0.125F;
         res[2] = (rowAj[2] + rowAk[2] + rowBj[2] + rowBk[2] +
                   rowCj[2] + rowCk[2] + rowDj[2] + rowDk[2]) * 0.125F;
         dst[i] = float3_to_r11g11b10f(res);
      }
   }

   else {
      _mesa_problem(NULL, "bad format in do_row()");
   }
}


/*
 * These functions generate a 1/2-size mipmap image from a source image.
 * Texture borders are handled by copying or averaging the source image's
 * border texels, depending on the scale-down factor.
 */

static void
make_1d_mipmap(GLenum datatype, GLuint comps, GLint border,
               GLint srcWidth, const GLubyte *srcPtr,
               GLint dstWidth, GLubyte *dstPtr)
{
   const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLubyte *src;
   GLubyte *dst;

   /* skip the border pixel, if any */
   src = srcPtr + border * bpt;
   dst = dstPtr + border * bpt;

   /* we just duplicate the input row, kind of hack, saves code */
   do_row(datatype, comps, srcWidth - 2 * border, src, src,
          dstWidth - 2 * border, dst);

   if (border) {
      /* copy left-most pixel from source */
      assert(dstPtr);
      assert(srcPtr);
      memcpy(dstPtr, srcPtr, bpt);
      /* copy right-most pixel from source */
      memcpy(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt,
             bpt);
   }
}


static void
make_2d_mipmap(GLenum datatype, GLuint comps, GLint border,
               GLint srcWidth, GLint srcHeight,
	       const GLubyte *srcPtr, GLint srcRowStride,
               GLint dstWidth, GLint dstHeight,
	       GLubyte *dstPtr, GLint dstRowStride)
{
   const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint srcRowBytes = bpt * srcRowStride;
   const GLint dstRowBytes = bpt * dstRowStride;
   const GLubyte *srcA, *srcB;
   GLubyte *dst;
   GLint row, srcRowStep;

   /* Compute src and dst pointers, skipping any border */
   srcA = srcPtr + border * ((srcWidth + 1) * bpt);
   if (srcHeight > 1 && srcHeight > dstHeight) {
      /* sample from two source rows */
      srcB = srcA + srcRowBytes;
      srcRowStep = 2;
   }
   else {
      /* sample from one source row */
      srcB = srcA;
      srcRowStep = 1;
   }

   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (row = 0; row < dstHeightNB; row++) {
      do_row(datatype, comps, srcWidthNB, srcA, srcB,
             dstWidthNB, dst);
      srcA += srcRowStep * srcRowBytes;
      srcB += srcRowStep * srcRowBytes;
      dst += dstRowBytes;
   }

   /* This is ugly but probably won't be used much */
   if (border > 0) {
      /* fill in dest border */
      /* lower-left border pixel */
      assert(dstPtr);
      assert(srcPtr);
      memcpy(dstPtr, srcPtr, bpt);
      /* lower-right border pixel */
      memcpy(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt, bpt);
      /* upper-left border pixel */
      memcpy(dstPtr + dstWidth * (dstHeight - 1) * bpt,
             srcPtr + srcWidth * (srcHeight - 1) * bpt, bpt);
      /* upper-right border pixel */
      memcpy(dstPtr + (dstWidth * dstHeight - 1) * bpt,
             srcPtr + (srcWidth * srcHeight - 1) * bpt, bpt);
      /* lower border */
      do_row(datatype, comps, srcWidthNB,
             srcPtr + bpt,
             srcPtr + bpt,
             dstWidthNB, dstPtr + bpt);
      /* upper border */
      do_row(datatype, comps, srcWidthNB,
             srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
             srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
             dstWidthNB,
             dstPtr + (dstWidth * (dstHeight - 1) + 1) * bpt);
      /* left and right borders */
      if (srcHeight == dstHeight) {
         /* copy border pixel from src to dst */
         for (row = 1; row < srcHeight; row++) {
            memcpy(dstPtr + dstWidth * row * bpt,
                   srcPtr + srcWidth * row * bpt, bpt);
            memcpy(dstPtr + (dstWidth * row + dstWidth - 1) * bpt,
                   srcPtr + (srcWidth * row + srcWidth - 1) * bpt, bpt);
         }
      }
      else {
         /* average two src pixels each dest pixel */
         for (row = 0; row < dstHeightNB; row += 2) {
            do_row(datatype, comps, 1,
                   srcPtr + (srcWidth * (row * 2 + 1)) * bpt,
                   srcPtr + (srcWidth * (row * 2 + 2)) * bpt,
                   1, dstPtr + (dstWidth * row + 1) * bpt);
            do_row(datatype, comps, 1,
                   srcPtr + (srcWidth * (row * 2 + 1) + srcWidth - 1) * bpt,
                   srcPtr + (srcWidth * (row * 2 + 2) + srcWidth - 1) * bpt,
                   1, dstPtr + (dstWidth * row + 1 + dstWidth - 1) * bpt);
         }
      }
   }
}


static void
make_3d_mipmap(GLenum datatype, GLuint comps, GLint border,
               GLint srcWidth, GLint srcHeight, GLint srcDepth,
               const GLubyte *srcPtr, GLint srcRowStride,
               GLint dstWidth, GLint dstHeight, GLint dstDepth,
               GLubyte *dstPtr, GLint dstRowStride)
{
   const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint srcDepthNB = srcDepth - 2 * border;
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint dstDepthNB = dstDepth - 2 * border;
   GLint img, row;
   GLint bytesPerSrcImage, bytesPerDstImage;
   GLint bytesPerSrcRow, bytesPerDstRow;
   GLint srcImageOffset, srcRowOffset;

   (void) srcDepthNB; /* silence warnings */


   bytesPerSrcImage = srcWidth * srcHeight * bpt;
   bytesPerDstImage = dstWidth * dstHeight * bpt;

   bytesPerSrcRow = srcWidth * bpt;
   bytesPerDstRow = dstWidth * bpt;

   /* Offset between adjacent src images to be averaged together */
   srcImageOffset = (srcDepth == dstDepth) ? 0 : bytesPerSrcImage;

   /* Offset between adjacent src rows to be averaged together */
   srcRowOffset = (srcHeight == dstHeight) ? 0 : srcWidth * bpt;

   /*
    * Need to average together up to 8 src pixels for each dest pixel.
    * Break that down into 3 operations:
    *   1. take two rows from source image and average them together.
    *   2. take two rows from next source image and average them together.
    *   3. take the two averaged rows and average them for the final dst row.
    */

   /*
   printf("mip3d %d x %d x %d  ->  %d x %d x %d\n",
          srcWidth, srcHeight, srcDepth, dstWidth, dstHeight, dstDepth);
   */

   for (img = 0; img < dstDepthNB; img++) {
      /* first source image pointer, skipping border */
      const GLubyte *imgSrcA = srcPtr
         + (bytesPerSrcImage + bytesPerSrcRow + border) * bpt * border
         + img * (bytesPerSrcImage + srcImageOffset);
      /* second source image pointer, skipping border */
      const GLubyte *imgSrcB = imgSrcA + srcImageOffset;
      /* address of the dest image, skipping border */
      GLubyte *imgDst = dstPtr
         + (bytesPerDstImage + bytesPerDstRow + border) * bpt * border
         + img * bytesPerDstImage;

      /* setup the four source row pointers and the dest row pointer */
      const GLubyte *srcImgARowA = imgSrcA;
      const GLubyte *srcImgARowB = imgSrcA + srcRowOffset;
      const GLubyte *srcImgBRowA = imgSrcB;
      const GLubyte *srcImgBRowB = imgSrcB + srcRowOffset;
      GLubyte *dstImgRow = imgDst;

      for (row = 0; row < dstHeightNB; row++) {
         do_row_3D(datatype, comps, srcWidthNB, 
                   srcImgARowA, srcImgARowB,
                   srcImgBRowA, srcImgBRowB,
                   dstWidthNB, dstImgRow);

         /* advance to next rows */
         srcImgARowA += bytesPerSrcRow + srcRowOffset;
         srcImgARowB += bytesPerSrcRow + srcRowOffset;
         srcImgBRowA += bytesPerSrcRow + srcRowOffset;
         srcImgBRowB += bytesPerSrcRow + srcRowOffset;
         dstImgRow += bytesPerDstRow;
      }
   }


   /* Luckily we can leverage the make_2d_mipmap() function here! */
   if (border > 0) {
      /* do front border image */
      make_2d_mipmap(datatype, comps, 1, srcWidth, srcHeight, srcPtr, srcRowStride,
                     dstWidth, dstHeight, dstPtr, dstRowStride);
      /* do back border image */
      make_2d_mipmap(datatype, comps, 1, srcWidth, srcHeight,
                     srcPtr + bytesPerSrcImage * (srcDepth - 1), srcRowStride,
                     dstWidth, dstHeight,
                     dstPtr + bytesPerDstImage * (dstDepth - 1), dstRowStride);
      /* do four remaining border edges that span the image slices */
      if (srcDepth == dstDepth) {
         /* just copy border pixels from src to dst */
         for (img = 0; img < dstDepthNB; img++) {
            const GLubyte *src;
            GLubyte *dst;

            /* do border along [img][row=0][col=0] */
            src = srcPtr + (img + 1) * bytesPerSrcImage;
            dst = dstPtr + (img + 1) * bytesPerDstImage;
            memcpy(dst, src, bpt);

            /* do border along [img][row=dstHeight-1][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcHeight - 1) * bytesPerSrcRow;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstHeight - 1) * bytesPerDstRow;
            memcpy(dst, src, bpt);

            /* do border along [img][row=0][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcWidth - 1) * bpt;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstWidth - 1) * bpt;
            memcpy(dst, src, bpt);

            /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (bytesPerSrcImage - bpt);
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (bytesPerDstImage - bpt);
            memcpy(dst, src, bpt);
         }
      }
      else {
         /* average border pixels from adjacent src image pairs */
         ASSERT(srcDepthNB == 2 * dstDepthNB);
         for (img = 0; img < dstDepthNB; img++) {
            const GLubyte *src;
            GLubyte *dst;

            /* do border along [img][row=0][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage;
            dst = dstPtr + (img + 1) * bytesPerDstImage;
            do_row(datatype, comps, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=dstHeight-1][col=0] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcHeight - 1) * bytesPerSrcRow;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstHeight - 1) * bytesPerDstRow;
            do_row(datatype, comps, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=0][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (srcWidth - 1) * bpt;
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (dstWidth - 1) * bpt;
            do_row(datatype, comps, 1, src, src + srcImageOffset, 1, dst);

            /* do border along [img][row=dstHeight-1][col=dstWidth-1] */
            src = srcPtr + (img * 2 + 1) * bytesPerSrcImage
                         + (bytesPerSrcImage - bpt);
            dst = dstPtr + (img + 1) * bytesPerDstImage
                         + (bytesPerDstImage - bpt);
            do_row(datatype, comps, 1, src, src + srcImageOffset, 1, dst);
         }
      }
   }
}


static void
make_1d_stack_mipmap(GLenum datatype, GLuint comps, GLint border,
                     GLint srcWidth, const GLubyte *srcPtr, GLuint srcRowStride,
                     GLint dstWidth, GLint dstHeight,
		     GLubyte *dstPtr, GLuint dstRowStride )
{
   const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint srcRowBytes = bpt * srcRowStride;
   const GLint dstRowBytes = bpt * dstRowStride;
   const GLubyte *src;
   GLubyte *dst;
   GLint row;

   /* Compute src and dst pointers, skipping any border */
   src = srcPtr + border * ((srcWidth + 1) * bpt);
   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (row = 0; row < dstHeightNB; row++) {
      do_row(datatype, comps, srcWidthNB, src, src,
             dstWidthNB, dst);
      src += srcRowBytes;
      dst += dstRowBytes;
   }

   if (border) {
      /* copy left-most pixel from source */
      assert(dstPtr);
      assert(srcPtr);
      memcpy(dstPtr, srcPtr, bpt);
      /* copy right-most pixel from source */
      memcpy(dstPtr + (dstWidth - 1) * bpt,
             srcPtr + (srcWidth - 1) * bpt,
             bpt);
   }
}


/**
 * \bug
 * There is quite a bit of refactoring that could be done with this function
 * and \c make_2d_mipmap.
 */
static void
make_2d_stack_mipmap(GLenum datatype, GLuint comps, GLint border,
                     GLint srcWidth, GLint srcHeight,
		     const GLubyte *srcPtr, GLint srcRowStride,
                     GLint dstWidth, GLint dstHeight, GLint dstDepth,
                     GLubyte *dstPtr, GLint dstRowStride)
{
   const GLint bpt = bytes_per_pixel(datatype, comps);
   const GLint srcWidthNB = srcWidth - 2 * border;  /* sizes w/out border */
   const GLint dstWidthNB = dstWidth - 2 * border;
   const GLint dstHeightNB = dstHeight - 2 * border;
   const GLint dstDepthNB = dstDepth - 2 * border;
   const GLint srcRowBytes = bpt * srcRowStride;
   const GLint dstRowBytes = bpt * dstRowStride;
   const GLubyte *srcA, *srcB;
   GLubyte *dst;
   GLint layer;
   GLint row;

   /* Compute src and dst pointers, skipping any border */
   srcA = srcPtr + border * ((srcWidth + 1) * bpt);
   if (srcHeight > 1) 
      srcB = srcA + srcRowBytes;
   else
      srcB = srcA;
   dst = dstPtr + border * ((dstWidth + 1) * bpt);

   for (layer = 0; layer < dstDepthNB; layer++) {
      for (row = 0; row < dstHeightNB; row++) {
         do_row(datatype, comps, srcWidthNB, srcA, srcB,
                dstWidthNB, dst);
         srcA += 2 * srcRowBytes;
         srcB += 2 * srcRowBytes;
         dst += dstRowBytes;
      }

      /* This is ugly but probably won't be used much */
      if (border > 0) {
         /* fill in dest border */
         /* lower-left border pixel */
         assert(dstPtr);
         assert(srcPtr);
         memcpy(dstPtr, srcPtr, bpt);
         /* lower-right border pixel */
         memcpy(dstPtr + (dstWidth - 1) * bpt,
                srcPtr + (srcWidth - 1) * bpt, bpt);
         /* upper-left border pixel */
         memcpy(dstPtr + dstWidth * (dstHeight - 1) * bpt,
                srcPtr + srcWidth * (srcHeight - 1) * bpt, bpt);
         /* upper-right border pixel */
         memcpy(dstPtr + (dstWidth * dstHeight - 1) * bpt,
                srcPtr + (srcWidth * srcHeight - 1) * bpt, bpt);
         /* lower border */
         do_row(datatype, comps, srcWidthNB,
                srcPtr + bpt,
                srcPtr + bpt,
                dstWidthNB, dstPtr + bpt);
         /* upper border */
         do_row(datatype, comps, srcWidthNB,
                srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
                srcPtr + (srcWidth * (srcHeight - 1) + 1) * bpt,
                dstWidthNB,
                dstPtr + (dstWidth * (dstHeight - 1) + 1) * bpt);
         /* left and right borders */
         if (srcHeight == dstHeight) {
            /* copy border pixel from src to dst */
            for (row = 1; row < srcHeight; row++) {
               memcpy(dstPtr + dstWidth * row * bpt,
                      srcPtr + srcWidth * row * bpt, bpt);
               memcpy(dstPtr + (dstWidth * row + dstWidth - 1) * bpt,
                      srcPtr + (srcWidth * row + srcWidth - 1) * bpt, bpt);
            }
         }
         else {
            /* average two src pixels each dest pixel */
            for (row = 0; row < dstHeightNB; row += 2) {
               do_row(datatype, comps, 1,
                      srcPtr + (srcWidth * (row * 2 + 1)) * bpt,
                      srcPtr + (srcWidth * (row * 2 + 2)) * bpt,
                      1, dstPtr + (dstWidth * row + 1) * bpt);
               do_row(datatype, comps, 1,
                      srcPtr + (srcWidth * (row * 2 + 1) + srcWidth - 1) * bpt,
                      srcPtr + (srcWidth * (row * 2 + 2) + srcWidth - 1) * bpt,
                      1, dstPtr + (dstWidth * row + 1 + dstWidth - 1) * bpt);
            }
         }
      }
   }
}


/**
 * Down-sample a texture image to produce the next lower mipmap level.
 * \param comps  components per texel (1, 2, 3 or 4)
 * \param srcRowStride  stride between source rows, in texels
 * \param dstRowStride  stride between destination rows, in texels
 */
void
_mesa_generate_mipmap_level(GLenum target,
                            GLenum datatype, GLuint comps,
                            GLint border,
                            GLint srcWidth, GLint srcHeight, GLint srcDepth,
                            const GLubyte *srcData,
                            GLint srcRowStride,
                            GLint dstWidth, GLint dstHeight, GLint dstDepth,
                            GLubyte *dstData,
                            GLint dstRowStride)
{
   /*
    * We use simple 2x2 averaging to compute the next mipmap level.
    */
   switch (target) {
   case GL_TEXTURE_1D:
      make_1d_mipmap(datatype, comps, border,
                     srcWidth, srcData,
                     dstWidth, dstData);
      break;
   case GL_TEXTURE_2D:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      make_2d_mipmap(datatype, comps, border,
                     srcWidth, srcHeight, srcData, srcRowStride,
                     dstWidth, dstHeight, dstData, dstRowStride);
      break;
   case GL_TEXTURE_3D:
      make_3d_mipmap(datatype, comps, border,
                     srcWidth, srcHeight, srcDepth,
                     srcData, srcRowStride,
                     dstWidth, dstHeight, dstDepth,
                     dstData, dstRowStride);
      break;
   case GL_TEXTURE_1D_ARRAY_EXT:
      make_1d_stack_mipmap(datatype, comps, border,
                           srcWidth, srcData, srcRowStride,
                           dstWidth, dstHeight,
                           dstData, dstRowStride);
      break;
   case GL_TEXTURE_2D_ARRAY_EXT:
      make_2d_stack_mipmap(datatype, comps, border,
                           srcWidth, srcHeight,
                           srcData, srcRowStride,
                           dstWidth, dstHeight,
                           dstDepth, dstData, dstRowStride);
      break;
   case GL_TEXTURE_RECTANGLE_NV:
      /* no mipmaps, do nothing */
      break;
   default:
      _mesa_problem(NULL, "bad dimensions in _mesa_generate_mipmaps");
      return;
   }
}


/**
 * compute next (level+1) image size
 * \return GL_FALSE if no smaller size can be generated (eg. src is 1x1x1 size)
 */
static GLboolean
next_mipmap_level_size(GLenum target, GLint border,
                       GLint srcWidth, GLint srcHeight, GLint srcDepth,
                       GLint *dstWidth, GLint *dstHeight, GLint *dstDepth)
{
   if (srcWidth - 2 * border > 1) {
      *dstWidth = (srcWidth - 2 * border) / 2 + 2 * border;
   }
   else {
      *dstWidth = srcWidth; /* can't go smaller */
   }

   if ((srcHeight - 2 * border > 1) && 
       (target != GL_TEXTURE_1D_ARRAY_EXT)) {
      *dstHeight = (srcHeight - 2 * border) / 2 + 2 * border;
   }
   else {
      *dstHeight = srcHeight; /* can't go smaller */
   }

   if ((srcDepth - 2 * border > 1) &&
       (target != GL_TEXTURE_2D_ARRAY_EXT)) {
      *dstDepth = (srcDepth - 2 * border) / 2 + 2 * border;
   }
   else {
      *dstDepth = srcDepth; /* can't go smaller */
   }

   if (*dstWidth == srcWidth &&
       *dstHeight == srcHeight &&
       *dstDepth == srcDepth) {
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}

static void
generate_mipmap_uncompressed(struct gl_context *ctx, GLenum target,
			     struct gl_texture_object *texObj,
			     const struct gl_texture_image *srcImage,
			     GLuint maxLevel)
{
   GLint level;
   GLenum datatype;
   GLuint comps;

   _mesa_format_to_type_and_comps(srcImage->TexFormat, &datatype, &comps);

   for (level = texObj->BaseLevel; level < maxLevel; level++) {
      /* generate image[level+1] from image[level] */
      const struct gl_texture_image *srcImage;
      struct gl_texture_image *dstImage;
      GLint srcWidth, srcHeight, srcDepth;
      GLint dstWidth, dstHeight, dstDepth;
      GLint border;
      GLboolean nextLevel;

      /* get src image parameters */
      srcImage = _mesa_select_tex_image(ctx, texObj, target, level);
      ASSERT(srcImage);
      srcWidth = srcImage->Width;
      srcHeight = srcImage->Height;
      srcDepth = srcImage->Depth;
      border = srcImage->Border;

      nextLevel = next_mipmap_level_size(target, border,
                                         srcWidth, srcHeight, srcDepth,
                                         &dstWidth, &dstHeight, &dstDepth);
      if (!nextLevel)
         return;

      /* get dest gl_texture_image */
      dstImage = _mesa_get_tex_image(ctx, texObj, target, level + 1);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      /* Free old image data */
      if (dstImage->Data)
         ctx->Driver.FreeTexImageData(ctx, dstImage);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->InternalFormat,
                                 srcImage->TexFormat);
      dstImage->DriverData = NULL;
      dstImage->FetchTexelc = srcImage->FetchTexelc;
      dstImage->FetchTexelf = srcImage->FetchTexelf;

      /* Alloc new teximage data buffer */
      {
         GLuint size = _mesa_format_image_size(dstImage->TexFormat,
                                               dstWidth, dstHeight, dstDepth);
         dstImage->Data = _mesa_alloc_texmemory(size);
         if (!dstImage->Data) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
            return;
         }
      }

      ASSERT(dstImage->TexFormat);
      ASSERT(dstImage->FetchTexelc);
      ASSERT(dstImage->FetchTexelf);

      _mesa_generate_mipmap_level(target, datatype, comps, border,
                                  srcWidth, srcHeight, srcDepth,
                                  srcImage->Data, srcImage->RowStride,
                                  dstWidth, dstHeight, dstDepth,
                                  dstImage->Data, dstImage->RowStride);

   } /* loop over mipmap levels */
}

static void
generate_mipmap_compressed(struct gl_context *ctx, GLenum target,
			   struct gl_texture_object *texObj,
			   const struct gl_texture_image *srcImage,
			   GLuint maxLevel)
{
   GLint level;
   gl_format temp_format;
   GLenum datatype;
   GLuint comps;
   GLuint row;
   GLint components;
   GLuint temp_src_stride, temp_dst_stride; /* in bytes */
   GLchan *temp_src = NULL, *temp_dst = NULL;

   /* Choose the format we will do _mesa_generate_mipmap_level() in,
    * and uncompress the firstImage into a temporary of that format.
    */
   assert(texObj->Target == GL_TEXTURE_2D ||
	  texObj->Target == GL_TEXTURE_CUBE_MAP_ARB);

   if (srcImage->_BaseFormat == GL_RGB) {
      temp_format = MESA_FORMAT_RGB888;
      components = 3;
   } else if (srcImage->_BaseFormat == GL_RED) {
      temp_format = MESA_FORMAT_R8;
      components = 1;
   } else if (srcImage->_BaseFormat == GL_RG) {
      temp_format = MESA_FORMAT_RG88;
      components = 2;
   } else if (srcImage->_BaseFormat == GL_RGBA) {
      temp_format = MESA_FORMAT_RGBA8888;
      components = 4;
   } else if (srcImage->_BaseFormat == GL_LUMINANCE) {
      temp_format = MESA_FORMAT_L8;
      components = 1;
   } else if (srcImage->_BaseFormat == GL_LUMINANCE_ALPHA) {
      temp_format = MESA_FORMAT_AL88;
      components = 2;
   } else {
      _mesa_problem(ctx, "bad srcImage->_BaseFormat in _mesa_generate_mipmaps");
      return;
   }

   /* allocate storage for uncompressed GL_RGB or GL_RGBA images */
   temp_src_stride = _mesa_format_row_stride(temp_format, srcImage->Width);
   /* 20 extra bytes, just be safe when calling last FetchTexel */
   temp_src = (GLubyte *) malloc(temp_src_stride * srcImage->Height + 20);
   if (!temp_src) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
      return;
   }

   /* decompress base image to the temporary */
   for (row = 0; row < srcImage->Height; row++) {
      GLuint col;
      GLchan *dst = (GLchan *) temp_src + temp_src_stride * row;
      for (col = 0; col < srcImage->Width; col++) {
	 srcImage->FetchTexelc(srcImage, col, row, 0, dst);
	 dst += components;
      }
   }

   _mesa_format_to_type_and_comps(temp_format, &datatype, &comps);

   for (level = texObj->BaseLevel; level < maxLevel; level++) {
      /* generate image[level+1] from image[level] */
      const struct gl_texture_image *srcImage;
      struct gl_texture_image *dstImage;
      GLint srcWidth, srcHeight, srcDepth;
      GLint dstWidth, dstHeight, dstDepth;
      GLint border;
      GLboolean nextLevel;

      /* get src image parameters */
      srcImage = _mesa_select_tex_image(ctx, texObj, target, level);
      ASSERT(srcImage);
      srcWidth = srcImage->Width;
      srcHeight = srcImage->Height;
      srcDepth = srcImage->Depth;
      border = srcImage->Border;

      nextLevel = next_mipmap_level_size(target, border,
                                         srcWidth, srcHeight, srcDepth,
                                         &dstWidth, &dstHeight, &dstDepth);
      if (!nextLevel)
	 break;

      temp_dst_stride = _mesa_format_row_stride(temp_format, dstWidth);
      if (!temp_dst) {
	 temp_dst = (GLubyte *) malloc(temp_dst_stride * dstHeight);
	 if (!temp_dst) {
	    _mesa_error(ctx, GL_OUT_OF_MEMORY, "generate mipmaps");
	    break;
	 }
      }

      /* get dest gl_texture_image */
      dstImage = _mesa_get_tex_image(ctx, texObj, target, level + 1);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      _mesa_generate_mipmap_level(target, datatype, comps, border,
                                  srcWidth, srcHeight, srcDepth,
                                  temp_src, temp_src_stride / components,
                                  dstWidth, dstHeight, dstDepth,
                                  temp_dst, temp_dst_stride / components);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->InternalFormat,
                                 srcImage->TexFormat);

      ctx->Driver.TexImage2D(ctx, target, level + 1,
			     srcImage->InternalFormat,
			     dstWidth, dstHeight, border,
			     _mesa_get_format_base_format(temp_format),
			     GL_UNSIGNED_BYTE,
			     temp_dst, &ctx->DefaultPacking, texObj, dstImage);

      /* swap src and dest pointers */
      {
	 GLchan *temp = temp_src;
	 temp_src = temp_dst;
	 temp_dst = temp;

	 temp_src_stride = temp_dst_stride;
      }
   } /* loop over mipmap levels */

   free((void *) temp_src);
   free(temp_dst);
}

/**
 * Automatic mipmap generation.
 * This is the fallback/default function for ctx->Driver.GenerateMipmap().
 * Generate a complete set of mipmaps from texObj's BaseLevel image.
 * Stop at texObj's MaxLevel or when we get to the 1x1 texture.
 * For cube maps, target will be one of
 * GL_TEXTURE_CUBE_MAP_POSITIVE/NEGATIVE_X/Y/Z; never GL_TEXTURE_CUBE_MAP.
 */
void
_mesa_generate_mipmap(struct gl_context *ctx, GLenum target,
                      struct gl_texture_object *texObj)
{
   const struct gl_texture_image *srcImage;
   GLint maxLevel;

   ASSERT(texObj);
   srcImage = _mesa_select_tex_image(ctx, texObj, target, texObj->BaseLevel);
   ASSERT(srcImage);

   maxLevel = _mesa_max_texture_levels(ctx, texObj->Target) - 1;
   ASSERT(maxLevel >= 0);  /* bad target */

   maxLevel = MIN2(maxLevel, texObj->MaxLevel);

   if (_mesa_is_format_compressed(srcImage->TexFormat)) {
      generate_mipmap_compressed(ctx, target, texObj, srcImage, maxLevel);
   } else {
      generate_mipmap_uncompressed(ctx, target, texObj, srcImage, maxLevel);
   }
}


/**
 * Helper function for drivers which need to rescale texture images to
 * certain aspect ratios.
 * Nearest filtering only (for broken hardware that can't support
 * all aspect ratios).  This can be made a lot faster, but I don't
 * really care enough...
 */
void
_mesa_rescale_teximage2d(GLuint bytesPerPixel,
			 GLuint srcStrideInPixels,
			 GLuint dstRowStride,
			 GLint srcWidth, GLint srcHeight,
			 GLint dstWidth, GLint dstHeight,
			 const GLvoid *srcImage, GLvoid *dstImage)
{
   GLint row, col;

#define INNER_LOOP( TYPE, HOP, WOP )					\
   for ( row = 0 ; row < dstHeight ; row++ ) {				\
      GLint srcRow = row HOP hScale;					\
      for ( col = 0 ; col < dstWidth ; col++ ) {			\
	 GLint srcCol = col WOP wScale;					\
	 dst[col] = src[srcRow * srcStrideInPixels + srcCol];		\
      }									\
      dst = (TYPE *) ((GLubyte *) dst + dstRowStride);			\
   }									\

#define RESCALE_IMAGE( TYPE )						\
do {									\
   const TYPE *src = (const TYPE *)srcImage;				\
   TYPE *dst = (TYPE *)dstImage;					\
									\
   if ( srcHeight < dstHeight ) {					\
      const GLint hScale = dstHeight / srcHeight;			\
      if ( srcWidth < dstWidth ) {					\
	 const GLint wScale = dstWidth / srcWidth;			\
	 INNER_LOOP( TYPE, /, / );					\
      }									\
      else {								\
	 const GLint wScale = srcWidth / dstWidth;			\
	 INNER_LOOP( TYPE, /, * );					\
      }									\
   }									\
   else {								\
      const GLint hScale = srcHeight / dstHeight;			\
      if ( srcWidth < dstWidth ) {					\
	 const GLint wScale = dstWidth / srcWidth;			\
	 INNER_LOOP( TYPE, *, / );					\
      }									\
      else {								\
	 const GLint wScale = srcWidth / dstWidth;			\
	 INNER_LOOP( TYPE, *, * );					\
      }									\
   }									\
} while (0)

   switch ( bytesPerPixel ) {
   case 4:
      RESCALE_IMAGE( GLuint );
      break;

   case 2:
      RESCALE_IMAGE( GLushort );
      break;

   case 1:
      RESCALE_IMAGE( GLubyte );
      break;
   default:
      _mesa_problem(NULL,"unexpected bytes/pixel in _mesa_rescale_teximage2d");
   }
}


/**
 * Upscale an image by replication, not (typical) stretching.
 * We use this when the image width or height is less than a
 * certain size (4, 8) and we need to upscale an image.
 */
void
_mesa_upscale_teximage2d(GLsizei inWidth, GLsizei inHeight,
                         GLsizei outWidth, GLsizei outHeight,
                         GLint comps, const GLchan *src, GLint srcRowStride,
                         GLchan *dest )
{
   GLint i, j, k;

   ASSERT(outWidth >= inWidth);
   ASSERT(outHeight >= inHeight);
#if 0
   ASSERT(inWidth == 1 || inWidth == 2 || inHeight == 1 || inHeight == 2);
   ASSERT((outWidth & 3) == 0);
   ASSERT((outHeight & 3) == 0);
#endif

   for (i = 0; i < outHeight; i++) {
      const GLint ii = i % inHeight;
      for (j = 0; j < outWidth; j++) {
         const GLint jj = j % inWidth;
         for (k = 0; k < comps; k++) {
            dest[(i * outWidth + j) * comps + k]
               = src[ii * srcRowStride + jj * comps + k];
         }
      }
   }
}

