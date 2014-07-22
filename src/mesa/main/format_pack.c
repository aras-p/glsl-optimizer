/*
 * Mesa 3-D graphics library
 *
 * Copyright (c) 2011 VMware, Inc.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * Color, depth, stencil packing functions.
 * Used to pack basic color, depth and stencil formats to specific
 * hardware formats.
 *
 * There are both per-pixel and per-row packing functions:
 * - The former will be used by swrast to write values to the color, depth,
 *   stencil buffers when drawing points, lines and masked spans.
 * - The later will be used for image-oriented functions like glDrawPixels,
 *   glAccum, and glTexImage.
 */


#include "colormac.h"
#include "format_pack.h"
#include "macros.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"
#include "util/format_srgb.h"


/** Helper struct for MESA_FORMAT_Z32_FLOAT_S8X24_UINT */
struct z32f_x24s8
{
   float z;
   uint32_t x24s8;
};


typedef void (*pack_ubyte_rgba_row_func)(GLuint n,
                                         const GLubyte src[][4], void *dst);

typedef void (*pack_float_rgba_row_func)(GLuint n,
                                         const GLfloat src[][4], void *dst);


/*
 * MESA_FORMAT_A8B8G8R8_UNORM
 */

static void
pack_ubyte_A8B8G8R8_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[RCOMP], src[GCOMP], src[BCOMP], src[ACOMP]);
}

static void
pack_float_A8B8G8R8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_A8B8G8R8_UNORM(v, dst);
}

static void
pack_row_ubyte_A8B8G8R8_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][RCOMP], src[i][GCOMP],
                             src[i][BCOMP], src[i][ACOMP]);
   }
}

static void
pack_row_float_A8B8G8R8_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_A8B8G8R8_UNORM(v, d + i);
   }
}



/*
 * MESA_FORMAT_R8G8B8A8_UNORM
 */

static void
pack_ubyte_R8G8B8A8_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[ACOMP], src[BCOMP], src[GCOMP], src[RCOMP]);
}

static void
pack_float_R8G8B8A8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_R8G8B8A8_UNORM(v, dst);
}

static void
pack_row_ubyte_R8G8B8A8_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][ACOMP], src[i][BCOMP],
                             src[i][GCOMP], src[i][RCOMP]);
   }
}

static void
pack_row_float_R8G8B8A8_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_R8G8B8A8_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_B8G8R8A8_UNORM
 */

static void
pack_ubyte_B8G8R8A8_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_B8G8R8A8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_B8G8R8A8_UNORM(v, dst);
}

static void
pack_row_ubyte_B8G8R8A8_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][ACOMP], src[i][RCOMP],
                             src[i][GCOMP], src[i][BCOMP]);
   }
}

static void
pack_row_float_B8G8R8A8_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_B8G8R8A8_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_A8R8G8B8_UNORM
 */

static void
pack_ubyte_A8R8G8B8_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[BCOMP], src[GCOMP], src[RCOMP], src[ACOMP]);
}

static void
pack_float_A8R8G8B8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_A8R8G8B8_UNORM(v, dst);
}

static void
pack_row_ubyte_A8R8G8B8_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][BCOMP], src[i][GCOMP],
                             src[i][RCOMP], src[i][ACOMP]);
   }
}

static void
pack_row_float_A8R8G8B8_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_A8R8G8B8_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_B8G8R8X8_UNORM
 */

static void
pack_ubyte_B8G8R8X8_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(0x0, src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_B8G8R8X8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_B8G8R8X8_UNORM(v, dst);
}

static void
pack_row_ubyte_B8G8R8X8_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(0, src[i][RCOMP], src[i][GCOMP], src[i][BCOMP]);
   }
}

static void
pack_row_float_B8G8R8X8_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_B8G8R8X8_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_X8R8G8B8_UNORM
 */

static void
pack_ubyte_X8R8G8B8_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = PACK_COLOR_8888(src[BCOMP], src[GCOMP], src[RCOMP], 0);
}

static void
pack_float_X8R8G8B8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_X8R8G8B8_UNORM(v, dst);
}

static void
pack_row_ubyte_X8R8G8B8_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i] = PACK_COLOR_8888(src[i][BCOMP], src[i][GCOMP], src[i][RCOMP], 0);
   }
}

static void
pack_row_float_X8R8G8B8_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_X8R8G8B8_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_BGR_UNORM8
 */

static void
pack_ubyte_BGR_UNORM8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = src[RCOMP];
   d[1] = src[GCOMP];
   d[0] = src[BCOMP];
}

static void
pack_float_BGR_UNORM8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[2], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[BCOMP]);
}

static void
pack_row_ubyte_BGR_UNORM8(GLuint n, const GLubyte src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i*3+2] = src[i][RCOMP];
      d[i*3+1] = src[i][GCOMP];
      d[i*3+0] = src[i][BCOMP];
   }
}

static void
pack_row_float_BGR_UNORM8(GLuint n, const GLfloat src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      d[i*3+2] = v[RCOMP];
      d[i*3+1] = v[GCOMP];
      d[i*3+0] = v[BCOMP];
   }
}


/*
 * MESA_FORMAT_RGB_UNORM8
 */

static void
pack_ubyte_RGB_UNORM8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = src[BCOMP];
   d[1] = src[GCOMP];
   d[0] = src[RCOMP];
}

static void
pack_float_RGB_UNORM8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[2], src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[RCOMP]);
}

static void
pack_row_ubyte_RGB_UNORM8(GLuint n, const GLubyte src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      d[i*3+2] = src[i][BCOMP];
      d[i*3+1] = src[i][GCOMP];
      d[i*3+0] = src[i][RCOMP];
   }
}

static void
pack_row_float_RGB_UNORM8(GLuint n, const GLfloat src[][4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      d[i*3+2] = v[BCOMP];
      d[i*3+1] = v[GCOMP];
      d[i*3+0] = v[RCOMP];
   }
}


/*
 * MESA_FORMAT_B5G6R5_UNORM
 */

static void
pack_ubyte_B5G6R5_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_565(src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_B5G6R5_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[3];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], src[BCOMP]);
   pack_ubyte_B5G6R5_UNORM(v, dst);
}

static void
pack_row_ubyte_B5G6R5_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      pack_ubyte_B5G6R5_UNORM(src[i], d + i);
   }
}

static void
pack_row_float_B5G6R5_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_B5G6R5_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_R5G6B5_UNORM
 * Warning: these functions do not match the current Mesa definition
 * of MESA_FORMAT_R5G6B5_UNORM.
 */

static void
pack_ubyte_R5G6B5_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_565_REV(src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_R5G6B5_UNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte r, g, b;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, src[BCOMP]);
   *d = PACK_COLOR_565_REV(r, g, b);
}

static void
pack_row_ubyte_R5G6B5_UNORM(GLuint n, const GLubyte src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      pack_ubyte_R5G6B5_UNORM(src[i], d + i);
   }
}

static void
pack_row_float_R5G6B5_UNORM(GLuint n, const GLfloat src[][4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLubyte v[4];
      _mesa_unclamped_float_rgba_to_ubyte(v, src[i]);
      pack_ubyte_R5G6B5_UNORM(v, d + i);
   }
}


/*
 * MESA_FORMAT_B4G4R4A4_UNORM
 */

static void
pack_ubyte_B4G4R4A4_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_4444(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_B4G4R4A4_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_B4G4R4A4_UNORM(v, dst);
}

/* use fallback row packing functions */


/*
 * MESA_FORMAT_A4R4G4B4_UNORM
 */

static void
pack_ubyte_A4R4G4B4_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_4444(src[BCOMP], src[GCOMP], src[RCOMP], src[ACOMP]);
}

static void
pack_float_A4R4G4B4_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_A4R4G4B4_UNORM(v, dst);
}

/* use fallback row packing functions */


/*
 * MESA_FORMAT_A1B5G5R5_UNORM
 */

static void
pack_ubyte_A1B5G5R5_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_5551(src[RCOMP], src[GCOMP], src[BCOMP], src[ACOMP]);
}

static void
pack_float_A1B5G5R5_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_A1B5G5R5_UNORM(v, dst);
}

/* use fallback row packing functions */


/*
 * MESA_FORMAT_B5G5R5A1_UNORM
 */

static void
pack_ubyte_B5G5R5A1_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_1555(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_B5G5R5A1_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_B5G5R5A1_UNORM(v, dst);
}


/* MESA_FORMAT_A1R5G5B5_UNORM
 * Warning: these functions do not match the current Mesa definition
 * of MESA_FORMAT_A1R5G5B5_UNORM.
 */

static void
pack_ubyte_A1R5G5B5_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst), tmp;
   tmp = PACK_COLOR_1555(src[ACOMP], src[RCOMP], src[GCOMP], src[BCOMP]);
   *d = (tmp >> 8) | (tmp << 8);
}

static void
pack_float_A1R5G5B5_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_A1R5G5B5_UNORM(v, dst);
}


/* MESA_FORMAT_L4A4_UNORM */

static void
pack_ubyte_L4A4_UNORM(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_44(src[ACOMP], src[RCOMP]);
}

static void
pack_float_L4A4_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], src[ACOMP]);
   pack_ubyte_L4A4_UNORM(v, dst);
}


/* MESA_FORMAT_L8A8_UNORM */

static void
pack_ubyte_L8A8_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_88(src[ACOMP], src[RCOMP]);
}

static void
pack_float_L8A8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], src[ACOMP]);
   pack_ubyte_L8A8_UNORM(v, dst);
}


/* MESA_FORMAT_A8L8_UNORM */

static void
pack_ubyte_A8L8_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_88(src[RCOMP], src[ACOMP]);
}

static void
pack_float_A8L8_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[3], src[ACOMP]);
   pack_ubyte_A8L8_UNORM(v, dst);
}


/* MESA_FORMAT_L16A16_UNORM */

static void
pack_ubyte_L16A16_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_1616(a, l);
}

static void
pack_float_L16A16_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l, a;
   UNCLAMPED_FLOAT_TO_USHORT(l, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_1616(a, l);
}


/* MESA_FORMAT_A16L16_UNORM */

static void
pack_ubyte_A16L16_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_1616(l, a);
}

static void
pack_float_A16L16_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort l, a;
   UNCLAMPED_FLOAT_TO_USHORT(l, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_1616(l, a);
}


/* MESA_FORMAT_B2G3R3_UNORM */

static void
pack_ubyte_B2G3R3_UNORM(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_332(src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_B2G3R3_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   UNCLAMPED_FLOAT_TO_UBYTE(v[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(v[2], src[BCOMP]);
   pack_ubyte_B2G3R3_UNORM(v, dst);
}


/* MESA_FORMAT_A_UNORM8 */

static void
pack_ubyte_A_UNORM8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = src[ACOMP];
}

static void
pack_float_A_UNORM8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[ACOMP]);
}


/* MESA_FORMAT_A_UNORM16 */

static void
pack_ubyte_A_UNORM16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = UBYTE_TO_USHORT(src[ACOMP]);
}

static void
pack_float_A_UNORM16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[ACOMP]);
}


/* MESA_FORMAT_L_UNORM8 */

static void
pack_ubyte_L_UNORM8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = src[RCOMP];
}

static void
pack_float_L_UNORM8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   UNCLAMPED_FLOAT_TO_UBYTE(d[0], src[RCOMP]);
}


/* MESA_FORMAT_L_UNORM16 */

static void
pack_ubyte_L_UNORM16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = UBYTE_TO_USHORT(src[RCOMP]);
}

static void
pack_float_L_UNORM16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
}


/* MESA_FORMAT_YCBCR */

static void
pack_ubyte_YCBCR(const GLubyte src[4], void *dst)
{
   /* todo */
}

static void
pack_float_YCBCR(const GLfloat src[4], void *dst)
{
   /* todo */
}


/* MESA_FORMAT_YCBCR_REV */

static void
pack_ubyte_YCBCR_REV(const GLubyte src[4], void *dst)
{
   /* todo */
}

static void
pack_float_YCBCR_REV(const GLfloat src[4], void *dst)
{
   /* todo */
}


/* MESA_FORMAT_R_UNORM8 */

static void
pack_ubyte_R_UNORM8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = src[RCOMP];
}

static void
pack_float_R_UNORM8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLubyte r;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   d[0] = r;
}


/* MESA_FORMAT_R8G8_UNORM */

static void
pack_ubyte_R8G8_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_88(src[GCOMP], src[RCOMP]);
}

static void
pack_float_R8G8_UNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte r, g;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, src[GCOMP]);
   *d = PACK_COLOR_88(g, r);
}


/* MESA_FORMAT_G8R8_UNORM */

static void
pack_ubyte_G8R8_UNORM(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = PACK_COLOR_88(src[RCOMP], src[GCOMP]);
}

static void
pack_float_G8R8_UNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte r, g;
   UNCLAMPED_FLOAT_TO_UBYTE(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, src[GCOMP]);
   *d = PACK_COLOR_88(r, g);
}


/* MESA_FORMAT_R_UNORM16 */

static void
pack_ubyte_R_UNORM16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = UBYTE_TO_USHORT(src[RCOMP]);
}

static void
pack_float_R_UNORM16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
}


/* MESA_FORMAT_R16G16_UNORM */

static void
pack_ubyte_R16G16_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   *d = PACK_COLOR_1616(g, r);
}

static void
pack_float_R16G16_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   *d = PACK_COLOR_1616(g, r);
}


/* MESA_FORMAT_G16R16_UNORM */

static void
pack_ubyte_G16R16_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   *d = PACK_COLOR_1616(r, g);
}


static void
pack_float_G16R16_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   *d = PACK_COLOR_1616(r, g);
}


/* MESA_FORMAT_B10G10R10A2_UNORM */

static void
pack_ubyte_B10G10R10A2_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   GLushort b = UBYTE_TO_USHORT(src[BCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, r, g, b);
}

static void
pack_float_B10G10R10A2_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g, b, a;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, r, g, b);
}


/* MESA_FORMAT_R10G10B10A2_UINT */

static void
pack_ubyte_R10G10B10A2_UINT(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   GLushort b = UBYTE_TO_USHORT(src[BCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, b, g, r);
}

static void
pack_float_R10G10B10A2_UINT(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g, b, a;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, b, g, r);
}


/* MESA_FORMAT_BGR_SRGB8 */

static void
pack_ubyte_BGR_SRGB8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   d[1] = util_format_linear_to_srgb_8unorm(src[GCOMP]);
   d[0] = util_format_linear_to_srgb_8unorm(src[BCOMP]);
}

static void
pack_float_BGR_SRGB8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   d[2] = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   d[1] = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   d[0] = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
}


/* MESA_FORMAT_A8B8G8R8_SRGB */

static void
pack_ubyte_A8B8G8R8_SRGB(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(r, g, b, src[ACOMP]);
}

static void
pack_float_A8B8G8R8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r, g, b, a;
   r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_8888(r, g, b, a);
}


/* MESA_FORMAT_B8G8R8A8_SRGB */

static void
pack_ubyte_B8G8R8A8_SRGB(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(src[ACOMP], r, g, b);
}

static void
pack_float_B8G8R8A8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r, g, b, a;
   r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_8888(a, r, g, b);
}


/* MESA_FORMAT_A8R8G8B8_SRGB */

static void
pack_ubyte_A8R8G8B8_SRGB(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(b, g, r, src[ACOMP]);
}

static void
pack_float_A8R8G8B8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r, g, b, a;
   r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_8888(b, g, r, a);
}


/* MESA_FORMAT_R8G8B8A8_SRGB */

static void
pack_ubyte_R8G8B8A8_SRGB(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(src[ACOMP], b, g, r);
}

static void
pack_float_R8G8B8A8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLubyte r, g, b, a;
   r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_8888(a, b, g, r);
}


/* MESA_FORMAT_L_SRGB8 */

static void
pack_ubyte_L_SRGB8(const GLubyte src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   *d = util_format_linear_to_srgb_8unorm(src[RCOMP]);
}

static void
pack_float_L_SRGB8(const GLfloat src[4], void *dst)
{
   GLubyte *d = ((GLubyte *) dst);
   GLubyte l = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   *d = l;
}


/* MESA_FORMAT_L8A8_SRGB */

static void
pack_ubyte_L8A8_SRGB(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte l = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   *d = PACK_COLOR_88(src[ACOMP], l);
}

/* MESA_FORMAT_A8L8_SRGB */

static void
pack_ubyte_A8L8_SRGB(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte l = util_format_linear_to_srgb_8unorm(src[RCOMP]);
   *d = PACK_COLOR_88(l, src[ACOMP]);
}

static void
pack_float_L8A8_SRGB(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte a, l = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   CLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_88(a, l);
}

static void
pack_float_A8L8_SRGB(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   GLubyte a, l = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   CLAMPED_FLOAT_TO_UBYTE(a, src[ACOMP]);
   *d = PACK_COLOR_88(l, a);
}


/* MESA_FORMAT_RGBA_FLOAT32 */

static void
pack_ubyte_RGBA_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[0]);
   d[1] = UBYTE_TO_FLOAT(src[1]);
   d[2] = UBYTE_TO_FLOAT(src[2]);
   d[3] = UBYTE_TO_FLOAT(src[3]);
}

static void
pack_float_RGBA_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[0];
   d[1] = src[1];
   d[2] = src[2];
   d[3] = src[3];
}


/* MESA_FORMAT_RGBA_FLOAT16 */

static void
pack_ubyte_RGBA_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[0]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[1]));
   d[2] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[2]));
   d[3] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[3]));
}

static void
pack_float_RGBA_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[0]);
   d[1] = _mesa_float_to_half(src[1]);
   d[2] = _mesa_float_to_half(src[2]);
   d[3] = _mesa_float_to_half(src[3]);
}


/* MESA_FORMAT_RGB_FLOAT32 */

static void
pack_ubyte_RGB_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[0]);
   d[1] = UBYTE_TO_FLOAT(src[1]);
   d[2] = UBYTE_TO_FLOAT(src[2]);
}

static void
pack_float_RGB_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[0];
   d[1] = src[1];
   d[2] = src[2];
}


/* MESA_FORMAT_RGB_FLOAT16 */

static void
pack_ubyte_RGB_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[0]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[1]));
   d[2] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[2]));
}

static void
pack_float_RGB_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[0]);
   d[1] = _mesa_float_to_half(src[1]);
   d[2] = _mesa_float_to_half(src[2]);
}


/* MESA_FORMAT_A_FLOAT32 */

static void
pack_ubyte_A_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[ACOMP]);
}

static void
pack_float_A_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[ACOMP];
}


/* MESA_FORMAT_A_FLOAT16 */

static void
pack_ubyte_A_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[ACOMP]));
}

static void
pack_float_A_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[ACOMP]);
}


/* MESA_FORMAT_L_FLOAT32 (and I_FLOAT32, R_FLOAT32) */

static void
pack_ubyte_L_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[RCOMP]);
}

static void
pack_float_L_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
}


/* MESA_FORMAT_L_FLOAT16 (and I_FLOAT16, R_FLOAT32) */

static void
pack_ubyte_L_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[RCOMP]));
}

static void
pack_float_L_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
}


/* MESA_FORMAT_LA_FLOAT32 */

static void
pack_ubyte_LA_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   d[1] = UBYTE_TO_FLOAT(src[ACOMP]);
}

static void
pack_float_LA_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
   d[1] = src[ACOMP];
}


/* MESA_FORMAT_LA_FLOAT16 */

static void
pack_ubyte_LA_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[RCOMP]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[ACOMP]));
}

static void
pack_float_LA_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
   d[1] = _mesa_float_to_half(src[ACOMP]);
}


/* MESA_FORMAT_RG_FLOAT32 */

static void
pack_ubyte_RG_FLOAT32(const GLubyte src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   d[1] = UBYTE_TO_FLOAT(src[GCOMP]);
}

static void
pack_float_RG_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
   d[1] = src[GCOMP];
}


/* MESA_FORMAT_RG_FLOAT16 */

static void
pack_ubyte_RG_FLOAT16(const GLubyte src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[RCOMP]));
   d[1] = _mesa_float_to_half(UBYTE_TO_FLOAT(src[GCOMP]));
}

static void
pack_float_RG_FLOAT16(const GLfloat src[4], void *dst)
{
   GLhalfARB *d = ((GLhalfARB *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
   d[1] = _mesa_float_to_half(src[GCOMP]);
}


/* MESA_FORMAT_RGBA_UNORM16 */

static void
pack_ubyte_RGBA_16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   d[0] = UBYTE_TO_USHORT(src[RCOMP]);
   d[1] = UBYTE_TO_USHORT(src[GCOMP]);
   d[2] = UBYTE_TO_USHORT(src[BCOMP]);
   d[3] = UBYTE_TO_USHORT(src[ACOMP]);
}

static void
pack_float_RGBA_16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[2], src[BCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[3], src[ACOMP]);
}



/*
 * MESA_FORMAT_R_SNORM8
 */

static void
pack_float_R_SNORM8(const GLfloat src[4], void *dst)
{
   GLbyte *d = (GLbyte *) dst;
   *d = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_R8G8_SNORM
 */

static void
pack_float_R8G8_SNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = (GLushort *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   *d = (g << 8) | r;
}


/*
 * MESA_FORMAT_X8B8G8R8_SNORM
 */

static void
pack_float_X8B8G8R8_SNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   GLbyte a = 127;
   *d = PACK_COLOR_8888(r, g, b, a);
}


/*
 * MESA_FORMAT_A8B8G8R8_SNORM
 */

static void
pack_float_A8B8G8R8_SNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_8888(r, g, b, a);
}


/*
 * MESA_FORMAT_R8G8B8A8_SNORM
 */

static void
pack_float_R8G8B8A8_SNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_8888(a, b, g, r);
}


/*
 * MESA_FORMAT_R_SNORM16
 */

static void
pack_float_R_SNORM16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   *d = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_R16G16_SNORM
 */

static void
pack_float_R16G16_SNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLshort r = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLshort g = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   *d = (g << 16) | (r & 0xffff);
}


/*
 * MESA_FORMAT_RGB_SNORM16
 */

static void
pack_float_RGB_SNORM16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   d[0] = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   d[1] = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   d[2] = FLOAT_TO_SHORT(CLAMP(src[BCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_RGBA_SNORM16
 */

static void
pack_float_RGBA_SNORM16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   d[0] = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   d[1] = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   d[2] = FLOAT_TO_SHORT(CLAMP(src[BCOMP], -1.0f, 1.0f));
   d[3] = FLOAT_TO_SHORT(CLAMP(src[ACOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_A_SNORM8
 */

static void
pack_float_A_SNORM8(const GLfloat src[4], void *dst)
{
   GLbyte *d = (GLbyte *) dst;
   *d = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_L_SNORM8
 */

static void
pack_float_L_SNORM8(const GLfloat src[4], void *dst)
{
   GLbyte *d = (GLbyte *) dst;
   *d = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_L8A8_SNORM
 */

static void
pack_float_L8A8_SNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = (GLushort *) dst;
   GLbyte l = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = (a << 8) | l;
}


/*
 * MESA_FORMAT_A8L8_SNORM
 */

static void
pack_float_A8L8_SNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = (GLushort *) dst;
   GLbyte l = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte a = FLOAT_TO_BYTE(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = (l << 8) | a;
}


/*
 * MESA_FORMAT_A_SNORM16
 */

static void
pack_float_A_SNORM16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   *d = FLOAT_TO_SHORT(CLAMP(src[ACOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_L_SNORM16
 */

static void
pack_float_L_SNORM16(const GLfloat src[4], void *dst)
{
   GLshort *d = (GLshort *) dst;
   *d = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
}


/*
 * MESA_FORMAT_LA_SNORM16
 */

static void
pack_float_LA_SNORM16(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLshort l = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLshort a = FLOAT_TO_SHORT(CLAMP(src[ACOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_1616(a, l);
}


/*
 * MESA_FORMAT_R9G9B9E5_FLOAT;
 */

static void
pack_float_R9G9B9E5_FLOAT(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   *d = float3_to_rgb9e5(src);
}

static void
pack_ubyte_R9G9B9E5_FLOAT(const GLubyte src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLfloat rgb[3];
   rgb[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   rgb[1] = UBYTE_TO_FLOAT(src[GCOMP]);
   rgb[2] = UBYTE_TO_FLOAT(src[BCOMP]);
   *d = float3_to_rgb9e5(rgb);
}



/*
 * MESA_FORMAT_R11G11B10_FLOAT;
 */

static void
pack_ubyte_R11G11B10_FLOAT(const GLubyte src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLfloat rgb[3];
   rgb[0] = UBYTE_TO_FLOAT(src[RCOMP]);
   rgb[1] = UBYTE_TO_FLOAT(src[GCOMP]);
   rgb[2] = UBYTE_TO_FLOAT(src[BCOMP]);
   *d = float3_to_r11g11b10f(rgb);
}

static void
pack_float_R11G11B10_FLOAT(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   *d = float3_to_r11g11b10f(src);
}


/*
 * MESA_FORMAT_B4G4R4X4_UNORM
 */

static void
pack_ubyte_XRGB4444_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_4444(255, src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_XRGB4444_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_XRGB4444_UNORM(v, dst);
}


/*
 * MESA_FORMAT_B5G5R5X1_UNORM
 */

static void
pack_ubyte_XRGB1555_UNORM(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = PACK_COLOR_1555(255, src[RCOMP], src[GCOMP], src[BCOMP]);
}

static void
pack_float_XRGB1555_UNORM(const GLfloat src[4], void *dst)
{
   GLubyte v[4];
   _mesa_unclamped_float_rgba_to_ubyte(v, src);
   pack_ubyte_XRGB1555_UNORM(v, dst);
}


/*
 * MESA_FORMAT_R8G8B8X8_SNORM
 */

static void
pack_float_XBGR8888_SNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   GLbyte b = FLOAT_TO_BYTE(CLAMP(src[BCOMP], -1.0f, 1.0f));
   *d = PACK_COLOR_8888(127, b, g, r);
}


/*
 * MESA_FORMAT_R8G8B8X8_SRGB
 */

static void
pack_float_R8G8B8X8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLubyte r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(255, b, g, r);
}


/*
 * MESA_FORMAT_X8B8G8R8_SRGB
 */

static void
pack_float_X8B8G8R8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLubyte r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(r, g, b, 255);
}


/* MESA_FORMAT_B10G10R10X2_UNORM */

static void
pack_ubyte_B10G10R10X2_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   GLushort b = UBYTE_TO_USHORT(src[BCOMP]);
   *d = PACK_COLOR_2101010_US(3, r, g, b);
}

static void
pack_float_B10G10R10X2_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g, b;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
   *d = PACK_COLOR_2101010_US(3, r, g, b);
}


/* MESA_FORMAT_RGBX_UNORM16 */

static void
pack_ubyte_RGBX_UNORM16(const GLubyte src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   d[0] = UBYTE_TO_USHORT(src[RCOMP]);
   d[1] = UBYTE_TO_USHORT(src[GCOMP]);
   d[2] = UBYTE_TO_USHORT(src[BCOMP]);
   d[3] = 65535;
}

static void
pack_float_RGBX_UNORM16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_USHORT(d[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(d[2], src[BCOMP]);
   d[3] = 65535;
}


/* MESA_FORMAT_RGBX_SNORM16 */

static void
pack_float_RGBX_SNORM16(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   UNCLAMPED_FLOAT_TO_SHORT(d[0], src[RCOMP]);
   UNCLAMPED_FLOAT_TO_SHORT(d[1], src[GCOMP]);
   UNCLAMPED_FLOAT_TO_SHORT(d[2], src[BCOMP]);
   d[3] = 32767;
}


/* MESA_FORMAT_RGBX_FLOAT16 */

static void
pack_float_XBGR16161616_FLOAT(const GLfloat src[4], void *dst)
{
   GLushort *d = ((GLushort *) dst);
   d[0] = _mesa_float_to_half(src[RCOMP]);
   d[1] = _mesa_float_to_half(src[GCOMP]);
   d[2] = _mesa_float_to_half(src[BCOMP]);
   d[3] = _mesa_float_to_half(1.0);
}

/* MESA_FORMAT_RGBX_FLOAT32 */

static void
pack_float_RGBX_FLOAT32(const GLfloat src[4], void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[0] = src[RCOMP];
   d[1] = src[GCOMP];
   d[2] = src[BCOMP];
   d[3] = 1.0;
}

/* MESA_FORMAT_R10G10B10A2_UNORM */

static void
pack_ubyte_R10G10B10A2_UNORM(const GLubyte src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r = UBYTE_TO_USHORT(src[RCOMP]);
   GLushort g = UBYTE_TO_USHORT(src[GCOMP]);
   GLushort b = UBYTE_TO_USHORT(src[BCOMP]);
   GLushort a = UBYTE_TO_USHORT(src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, b, g, r);
}

static void
pack_float_R10G10B10A2_UNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = ((GLuint *) dst);
   GLushort r, g, b, a;
   UNCLAMPED_FLOAT_TO_USHORT(r, src[RCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(g, src[GCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(b, src[BCOMP]);
   UNCLAMPED_FLOAT_TO_USHORT(a, src[ACOMP]);
   *d = PACK_COLOR_2101010_US(a, b, g, r);
}

/*
 * MESA_FORMAT_G8R8_SNORM
 */

static void
pack_float_G8R8_SNORM(const GLfloat src[4], void *dst)
{
   GLushort *d = (GLushort *) dst;
   GLbyte r = FLOAT_TO_BYTE(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLbyte g = FLOAT_TO_BYTE(CLAMP(src[GCOMP], -1.0f, 1.0f));
   *d = (r << 8) | (g & 0xff);
}

/*
 * MESA_FORMAT_G16R16_SNORM
 */

static void
pack_float_G16R16_SNORM(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLshort r = FLOAT_TO_SHORT(CLAMP(src[RCOMP], -1.0f, 1.0f));
   GLshort g = FLOAT_TO_SHORT(CLAMP(src[GCOMP], -1.0f, 1.0f));
   *d = (r << 16) | (g & 0xffff);
}

/*
 * MESA_FORMAT_B8G8R8X8_SRGB
 */

static void
pack_float_B8G8R8X8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLubyte r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(127, r, g, b);
}

/*
 * MESA_FORMAT_X8R8G8B8_SRGB
 */

static void
pack_float_X8R8G8B8_SRGB(const GLfloat src[4], void *dst)
{
   GLuint *d = (GLuint *) dst;
   GLubyte r = util_format_linear_float_to_srgb_8unorm(src[RCOMP]);
   GLubyte g = util_format_linear_float_to_srgb_8unorm(src[GCOMP]);
   GLubyte b = util_format_linear_float_to_srgb_8unorm(src[BCOMP]);
   *d = PACK_COLOR_8888(b, g, r, 255);
}

/**
 * Return a function that can pack a GLubyte rgba[4] color.
 */
gl_pack_ubyte_rgba_func
_mesa_get_pack_ubyte_rgba_function(mesa_format format)
{
   static gl_pack_ubyte_rgba_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_NONE] = NULL;

      table[MESA_FORMAT_A8B8G8R8_UNORM] = pack_ubyte_A8B8G8R8_UNORM;
      table[MESA_FORMAT_R8G8B8A8_UNORM] = pack_ubyte_R8G8B8A8_UNORM;
      table[MESA_FORMAT_B8G8R8A8_UNORM] = pack_ubyte_B8G8R8A8_UNORM;
      table[MESA_FORMAT_A8R8G8B8_UNORM] = pack_ubyte_A8R8G8B8_UNORM;
      table[MESA_FORMAT_X8B8G8R8_UNORM] = pack_ubyte_A8B8G8R8_UNORM; /* reused */
      table[MESA_FORMAT_R8G8B8X8_UNORM] = pack_ubyte_R8G8B8A8_UNORM; /* reused */
      table[MESA_FORMAT_B8G8R8X8_UNORM] = pack_ubyte_B8G8R8X8_UNORM;
      table[MESA_FORMAT_X8R8G8B8_UNORM] = pack_ubyte_X8R8G8B8_UNORM;
      table[MESA_FORMAT_BGR_UNORM8] = pack_ubyte_BGR_UNORM8;
      table[MESA_FORMAT_RGB_UNORM8] = pack_ubyte_RGB_UNORM8;
      table[MESA_FORMAT_B5G6R5_UNORM] = pack_ubyte_B5G6R5_UNORM;
      table[MESA_FORMAT_R5G6B5_UNORM] = pack_ubyte_R5G6B5_UNORM;
      table[MESA_FORMAT_B4G4R4A4_UNORM] = pack_ubyte_B4G4R4A4_UNORM;
      table[MESA_FORMAT_A4R4G4B4_UNORM] = pack_ubyte_A4R4G4B4_UNORM;
      table[MESA_FORMAT_A1B5G5R5_UNORM] = pack_ubyte_A1B5G5R5_UNORM;
      table[MESA_FORMAT_B5G5R5A1_UNORM] = pack_ubyte_B5G5R5A1_UNORM;
      table[MESA_FORMAT_A1R5G5B5_UNORM] = pack_ubyte_A1R5G5B5_UNORM;
      table[MESA_FORMAT_L4A4_UNORM] = pack_ubyte_L4A4_UNORM;
      table[MESA_FORMAT_L8A8_UNORM] = pack_ubyte_L8A8_UNORM;
      table[MESA_FORMAT_A8L8_UNORM] = pack_ubyte_A8L8_UNORM;
      table[MESA_FORMAT_L16A16_UNORM] = pack_ubyte_L16A16_UNORM;
      table[MESA_FORMAT_A16L16_UNORM] = pack_ubyte_A16L16_UNORM;
      table[MESA_FORMAT_B2G3R3_UNORM] = pack_ubyte_B2G3R3_UNORM;
      table[MESA_FORMAT_A_UNORM8] = pack_ubyte_A_UNORM8;
      table[MESA_FORMAT_A_UNORM16] = pack_ubyte_A_UNORM16;
      table[MESA_FORMAT_L_UNORM8] = pack_ubyte_L_UNORM8;
      table[MESA_FORMAT_L_UNORM16] = pack_ubyte_L_UNORM16;
      table[MESA_FORMAT_I_UNORM8] = pack_ubyte_L_UNORM8; /* reuse pack_ubyte_L_UNORM8 */
      table[MESA_FORMAT_I_UNORM16] = pack_ubyte_L_UNORM16; /* reuse pack_ubyte_L_UNORM16 */
      table[MESA_FORMAT_YCBCR] = pack_ubyte_YCBCR;
      table[MESA_FORMAT_YCBCR_REV] = pack_ubyte_YCBCR_REV;
      table[MESA_FORMAT_R_UNORM8] = pack_ubyte_R_UNORM8;
      table[MESA_FORMAT_R8G8_UNORM] = pack_ubyte_R8G8_UNORM;
      table[MESA_FORMAT_G8R8_UNORM] = pack_ubyte_G8R8_UNORM;
      table[MESA_FORMAT_R_UNORM16] = pack_ubyte_R_UNORM16;
      table[MESA_FORMAT_R16G16_UNORM] = pack_ubyte_R16G16_UNORM;
      table[MESA_FORMAT_G16R16_UNORM] = pack_ubyte_G16R16_UNORM;
      table[MESA_FORMAT_B10G10R10A2_UNORM] = pack_ubyte_B10G10R10A2_UNORM;
      table[MESA_FORMAT_R10G10B10A2_UINT] = pack_ubyte_R10G10B10A2_UINT;

      /* should never convert RGBA to these formats */
      table[MESA_FORMAT_S8_UINT_Z24_UNORM] = NULL;
      table[MESA_FORMAT_Z24_UNORM_S8_UINT] = NULL;
      table[MESA_FORMAT_Z_UNORM16] = NULL;
      table[MESA_FORMAT_Z24_UNORM_X8_UINT] = NULL;
      table[MESA_FORMAT_X8_UINT_Z24_UNORM] = NULL;
      table[MESA_FORMAT_Z_UNORM32] = NULL;
      table[MESA_FORMAT_S_UINT8] = NULL;

      /* sRGB */
      table[MESA_FORMAT_BGR_SRGB8] = pack_ubyte_BGR_SRGB8;
      table[MESA_FORMAT_A8B8G8R8_SRGB] = pack_ubyte_A8B8G8R8_SRGB;
      table[MESA_FORMAT_B8G8R8A8_SRGB] = pack_ubyte_B8G8R8A8_SRGB;
      table[MESA_FORMAT_A8R8G8B8_SRGB] = pack_ubyte_A8R8G8B8_SRGB;
      table[MESA_FORMAT_R8G8B8A8_SRGB] = pack_ubyte_R8G8B8A8_SRGB;
      table[MESA_FORMAT_L_SRGB8] = pack_ubyte_L_SRGB8;
      table[MESA_FORMAT_L8A8_SRGB] = pack_ubyte_L8A8_SRGB;
      table[MESA_FORMAT_A8L8_SRGB] = pack_ubyte_A8L8_SRGB;
      /* n/a */
      table[MESA_FORMAT_SRGB_DXT1] = NULL; /* pack_ubyte_SRGB_DXT1; */
      table[MESA_FORMAT_SRGBA_DXT1] = NULL; /* pack_ubyte_SRGBA_DXT1; */
      table[MESA_FORMAT_SRGBA_DXT3] = NULL; /* pack_ubyte_SRGBA_DXT3; */
      table[MESA_FORMAT_SRGBA_DXT5] = NULL; /* pack_ubyte_SRGBA_DXT5; */

      table[MESA_FORMAT_RGB_FXT1] = NULL; /* pack_ubyte_RGB_FXT1; */
      table[MESA_FORMAT_RGBA_FXT1] = NULL; /* pack_ubyte_RGBA_FXT1; */
      table[MESA_FORMAT_RGB_DXT1] = NULL; /* pack_ubyte_RGB_DXT1; */
      table[MESA_FORMAT_RGBA_DXT1] = NULL; /* pack_ubyte_RGBA_DXT1; */
      table[MESA_FORMAT_RGBA_DXT3] = NULL; /* pack_ubyte_RGBA_DXT3; */
      table[MESA_FORMAT_RGBA_DXT5] = NULL; /* pack_ubyte_RGBA_DXT5; */

      table[MESA_FORMAT_RGBA_FLOAT32] = pack_ubyte_RGBA_FLOAT32;
      table[MESA_FORMAT_RGBA_FLOAT16] = pack_ubyte_RGBA_FLOAT16;
      table[MESA_FORMAT_RGB_FLOAT32] = pack_ubyte_RGB_FLOAT32;
      table[MESA_FORMAT_RGB_FLOAT16] = pack_ubyte_RGB_FLOAT16;
      table[MESA_FORMAT_A_FLOAT32] = pack_ubyte_A_FLOAT32;
      table[MESA_FORMAT_A_FLOAT16] = pack_ubyte_A_FLOAT16;
      table[MESA_FORMAT_L_FLOAT32] = pack_ubyte_L_FLOAT32;
      table[MESA_FORMAT_L_FLOAT16] = pack_ubyte_L_FLOAT16;
      table[MESA_FORMAT_LA_FLOAT32] = pack_ubyte_LA_FLOAT32;
      table[MESA_FORMAT_LA_FLOAT16] = pack_ubyte_LA_FLOAT16;
      table[MESA_FORMAT_I_FLOAT32] = pack_ubyte_L_FLOAT32;
      table[MESA_FORMAT_I_FLOAT16] = pack_ubyte_L_FLOAT16;
      table[MESA_FORMAT_R_FLOAT32] = pack_ubyte_L_FLOAT32;
      table[MESA_FORMAT_R_FLOAT16] = pack_ubyte_L_FLOAT16;
      table[MESA_FORMAT_RG_FLOAT32] = pack_ubyte_RG_FLOAT32;
      table[MESA_FORMAT_RG_FLOAT16] = pack_ubyte_RG_FLOAT16;

      /* n/a */
      table[MESA_FORMAT_RGBA_SINT8] = NULL; /* pack_ubyte_RGBA_INT8 */
      table[MESA_FORMAT_RGBA_SINT16] = NULL; /* pack_ubyte_RGBA_INT16 */
      table[MESA_FORMAT_RGBA_SINT32] = NULL; /* pack_ubyte_RGBA_INT32 */
      table[MESA_FORMAT_RGBA_UINT8] = NULL; /* pack_ubyte_RGBA_UINT8 */
      table[MESA_FORMAT_RGBA_UINT16] = NULL; /* pack_ubyte_RGBA_UINT16 */
      table[MESA_FORMAT_RGBA_UINT32] = NULL; /* pack_ubyte_RGBA_UINT32 */

      table[MESA_FORMAT_RGBA_UNORM16] = pack_ubyte_RGBA_16;

      /* n/a */
      table[MESA_FORMAT_R_SNORM8] = NULL;
      table[MESA_FORMAT_R8G8_SNORM] = NULL;
      table[MESA_FORMAT_X8B8G8R8_SNORM] = NULL;
      table[MESA_FORMAT_A8B8G8R8_SNORM] = NULL;
      table[MESA_FORMAT_R8G8B8A8_SNORM] = NULL;
      table[MESA_FORMAT_R_SNORM16] = NULL;
      table[MESA_FORMAT_R16G16_SNORM] = NULL;
      table[MESA_FORMAT_RGB_SNORM16] = NULL;
      table[MESA_FORMAT_RGBA_SNORM16] = NULL;
      table[MESA_FORMAT_A_SNORM8] = NULL;
      table[MESA_FORMAT_L_SNORM8] = NULL;
      table[MESA_FORMAT_L8A8_SNORM] = NULL;
      table[MESA_FORMAT_A8L8_SNORM] = NULL;
      table[MESA_FORMAT_I_SNORM8] = NULL;
      table[MESA_FORMAT_A_SNORM16] = NULL;
      table[MESA_FORMAT_L_SNORM16] = NULL;
      table[MESA_FORMAT_LA_SNORM16] = NULL;
      table[MESA_FORMAT_I_SNORM16] = NULL;


      table[MESA_FORMAT_RGBA_UNORM16] = pack_ubyte_RGBA_16;

      table[MESA_FORMAT_R9G9B9E5_FLOAT] = pack_ubyte_R9G9B9E5_FLOAT;
      table[MESA_FORMAT_R11G11B10_FLOAT] = pack_ubyte_R11G11B10_FLOAT;

      table[MESA_FORMAT_B4G4R4X4_UNORM] = pack_ubyte_XRGB4444_UNORM;
      table[MESA_FORMAT_B5G5R5X1_UNORM] = pack_ubyte_XRGB1555_UNORM;
      table[MESA_FORMAT_R8G8B8X8_SNORM] = NULL;
      table[MESA_FORMAT_R8G8B8X8_SRGB] = NULL;
      table[MESA_FORMAT_X8B8G8R8_SRGB] = NULL;
      table[MESA_FORMAT_RGBX_UINT8] = NULL;
      table[MESA_FORMAT_RGBX_SINT8] = NULL;
      table[MESA_FORMAT_B10G10R10X2_UNORM] = pack_ubyte_B10G10R10X2_UNORM;
      table[MESA_FORMAT_RGBX_UNORM16] = pack_ubyte_RGBX_UNORM16;
      table[MESA_FORMAT_RGBX_SNORM16] = NULL;
      table[MESA_FORMAT_RGBX_FLOAT16] = NULL;
      table[MESA_FORMAT_RGBX_UINT16] = NULL;
      table[MESA_FORMAT_RGBX_SINT16] = NULL;
      table[MESA_FORMAT_RGBX_FLOAT32] = NULL;
      table[MESA_FORMAT_RGBX_UINT32] = NULL;
      table[MESA_FORMAT_RGBX_SINT32] = NULL;

      table[MESA_FORMAT_R10G10B10A2_UNORM] = pack_ubyte_R10G10B10A2_UNORM;

      table[MESA_FORMAT_B8G8R8X8_SRGB] = NULL;
      table[MESA_FORMAT_X8R8G8B8_SRGB] = NULL;

      initialized = GL_TRUE;
   }

   return table[format];
}



/**
 * Return a function that can pack a GLfloat rgba[4] color.
 */
gl_pack_float_rgba_func
_mesa_get_pack_float_rgba_function(mesa_format format)
{
   static gl_pack_float_rgba_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_NONE] = NULL;

      table[MESA_FORMAT_A8B8G8R8_UNORM] = pack_float_A8B8G8R8_UNORM;
      table[MESA_FORMAT_R8G8B8A8_UNORM] = pack_float_R8G8B8A8_UNORM;
      table[MESA_FORMAT_B8G8R8A8_UNORM] = pack_float_B8G8R8A8_UNORM;
      table[MESA_FORMAT_A8R8G8B8_UNORM] = pack_float_A8R8G8B8_UNORM;
      table[MESA_FORMAT_X8B8G8R8_UNORM] = pack_float_A8B8G8R8_UNORM; /* reused */
      table[MESA_FORMAT_R8G8B8X8_UNORM] = pack_float_R8G8B8A8_UNORM; /* reused */
      table[MESA_FORMAT_B8G8R8X8_UNORM] = pack_float_B8G8R8X8_UNORM;
      table[MESA_FORMAT_X8R8G8B8_UNORM] = pack_float_X8R8G8B8_UNORM;
      table[MESA_FORMAT_BGR_UNORM8] = pack_float_BGR_UNORM8;
      table[MESA_FORMAT_RGB_UNORM8] = pack_float_RGB_UNORM8;
      table[MESA_FORMAT_B5G6R5_UNORM] = pack_float_B5G6R5_UNORM;
      table[MESA_FORMAT_R5G6B5_UNORM] = pack_float_R5G6B5_UNORM;
      table[MESA_FORMAT_B4G4R4A4_UNORM] = pack_float_B4G4R4A4_UNORM;
      table[MESA_FORMAT_A4R4G4B4_UNORM] = pack_float_A4R4G4B4_UNORM;
      table[MESA_FORMAT_A1B5G5R5_UNORM] = pack_float_A1B5G5R5_UNORM;
      table[MESA_FORMAT_B5G5R5A1_UNORM] = pack_float_B5G5R5A1_UNORM;
      table[MESA_FORMAT_A1R5G5B5_UNORM] = pack_float_A1R5G5B5_UNORM;

      table[MESA_FORMAT_L4A4_UNORM] = pack_float_L4A4_UNORM;
      table[MESA_FORMAT_L8A8_UNORM] = pack_float_L8A8_UNORM;
      table[MESA_FORMAT_A8L8_UNORM] = pack_float_A8L8_UNORM;
      table[MESA_FORMAT_L16A16_UNORM] = pack_float_L16A16_UNORM;
      table[MESA_FORMAT_A16L16_UNORM] = pack_float_A16L16_UNORM;
      table[MESA_FORMAT_B2G3R3_UNORM] = pack_float_B2G3R3_UNORM;
      table[MESA_FORMAT_A_UNORM8] = pack_float_A_UNORM8;
      table[MESA_FORMAT_A_UNORM16] = pack_float_A_UNORM16;
      table[MESA_FORMAT_L_UNORM8] = pack_float_L_UNORM8;
      table[MESA_FORMAT_L_UNORM16] = pack_float_L_UNORM16;
      table[MESA_FORMAT_I_UNORM8] = pack_float_L_UNORM8; /* reuse pack_float_L_UNORM8 */
      table[MESA_FORMAT_I_UNORM16] = pack_float_L_UNORM16; /* reuse pack_float_L_UNORM16 */
      table[MESA_FORMAT_YCBCR] = pack_float_YCBCR;
      table[MESA_FORMAT_YCBCR_REV] = pack_float_YCBCR_REV;
      table[MESA_FORMAT_R_UNORM8] = pack_float_R_UNORM8;
      table[MESA_FORMAT_R8G8_UNORM] = pack_float_R8G8_UNORM;
      table[MESA_FORMAT_G8R8_UNORM] = pack_float_G8R8_UNORM;
      table[MESA_FORMAT_R_UNORM16] = pack_float_R_UNORM16;
      table[MESA_FORMAT_R16G16_UNORM] = pack_float_R16G16_UNORM;
      table[MESA_FORMAT_G16R16_UNORM] = pack_float_G16R16_UNORM;
      table[MESA_FORMAT_B10G10R10A2_UNORM] = pack_float_B10G10R10A2_UNORM;
      table[MESA_FORMAT_R10G10B10A2_UINT] = pack_float_R10G10B10A2_UINT;

      /* should never convert RGBA to these formats */
      table[MESA_FORMAT_S8_UINT_Z24_UNORM] = NULL;
      table[MESA_FORMAT_Z24_UNORM_S8_UINT] = NULL;
      table[MESA_FORMAT_Z_UNORM16] = NULL;
      table[MESA_FORMAT_Z24_UNORM_X8_UINT] = NULL;
      table[MESA_FORMAT_X8_UINT_Z24_UNORM] = NULL;
      table[MESA_FORMAT_Z_UNORM32] = NULL;
      table[MESA_FORMAT_S_UINT8] = NULL;

      table[MESA_FORMAT_BGR_SRGB8] = pack_float_BGR_SRGB8;
      table[MESA_FORMAT_A8B8G8R8_SRGB] = pack_float_A8B8G8R8_SRGB;
      table[MESA_FORMAT_B8G8R8A8_SRGB] = pack_float_B8G8R8A8_SRGB;
      table[MESA_FORMAT_A8R8G8B8_SRGB] = pack_float_A8R8G8B8_SRGB;
      table[MESA_FORMAT_R8G8B8A8_SRGB] = pack_float_R8G8B8A8_SRGB;
      table[MESA_FORMAT_L_SRGB8] = pack_float_L_SRGB8;
      table[MESA_FORMAT_L8A8_SRGB] = pack_float_L8A8_SRGB;
      table[MESA_FORMAT_A8L8_SRGB] = pack_float_A8L8_SRGB;

      /* n/a */
      table[MESA_FORMAT_SRGB_DXT1] = NULL;
      table[MESA_FORMAT_SRGBA_DXT1] = NULL;
      table[MESA_FORMAT_SRGBA_DXT3] = NULL;
      table[MESA_FORMAT_SRGBA_DXT5] = NULL;

      table[MESA_FORMAT_RGB_FXT1] = NULL;
      table[MESA_FORMAT_RGBA_FXT1] = NULL;
      table[MESA_FORMAT_RGB_DXT1] = NULL;
      table[MESA_FORMAT_RGBA_DXT1] = NULL;
      table[MESA_FORMAT_RGBA_DXT3] = NULL;
      table[MESA_FORMAT_RGBA_DXT5] = NULL;

      table[MESA_FORMAT_RGBA_FLOAT32] = pack_float_RGBA_FLOAT32;
      table[MESA_FORMAT_RGBA_FLOAT16] = pack_float_RGBA_FLOAT16;
      table[MESA_FORMAT_RGB_FLOAT32] = pack_float_RGB_FLOAT32;
      table[MESA_FORMAT_RGB_FLOAT16] = pack_float_RGB_FLOAT16;
      table[MESA_FORMAT_A_FLOAT32] = pack_float_A_FLOAT32;
      table[MESA_FORMAT_A_FLOAT16] = pack_float_A_FLOAT16;
      table[MESA_FORMAT_L_FLOAT32] = pack_float_L_FLOAT32;
      table[MESA_FORMAT_L_FLOAT16] = pack_float_L_FLOAT16;
      table[MESA_FORMAT_LA_FLOAT32] = pack_float_LA_FLOAT32;
      table[MESA_FORMAT_LA_FLOAT16] = pack_float_LA_FLOAT16;

      table[MESA_FORMAT_I_FLOAT32] = pack_float_L_FLOAT32;
      table[MESA_FORMAT_I_FLOAT16] = pack_float_L_FLOAT16;
      table[MESA_FORMAT_R_FLOAT32] = pack_float_L_FLOAT32;
      table[MESA_FORMAT_R_FLOAT16] = pack_float_L_FLOAT16;
      table[MESA_FORMAT_RG_FLOAT32] = pack_float_RG_FLOAT32;
      table[MESA_FORMAT_RG_FLOAT16] = pack_float_RG_FLOAT16;

      /* n/a */
      table[MESA_FORMAT_RGBA_SINT8] = NULL;
      table[MESA_FORMAT_RGBA_SINT16] = NULL;
      table[MESA_FORMAT_RGBA_SINT32] = NULL;
      table[MESA_FORMAT_RGBA_UINT8] = NULL;
      table[MESA_FORMAT_RGBA_UINT16] = NULL;
      table[MESA_FORMAT_RGBA_UINT32] = NULL;

      table[MESA_FORMAT_RGBA_UNORM16] = pack_float_RGBA_16;

      table[MESA_FORMAT_R_SNORM8] = pack_float_R_SNORM8;
      table[MESA_FORMAT_R8G8_SNORM] = pack_float_R8G8_SNORM;
      table[MESA_FORMAT_X8B8G8R8_SNORM] = pack_float_X8B8G8R8_SNORM;
      table[MESA_FORMAT_A8B8G8R8_SNORM] = pack_float_A8B8G8R8_SNORM;
      table[MESA_FORMAT_R8G8B8A8_SNORM] = pack_float_R8G8B8A8_SNORM;
      table[MESA_FORMAT_R_SNORM16] = pack_float_R_SNORM16;
      table[MESA_FORMAT_R16G16_SNORM] = pack_float_R16G16_SNORM;
      table[MESA_FORMAT_RGB_SNORM16] = pack_float_RGB_SNORM16;
      table[MESA_FORMAT_RGBA_SNORM16] = pack_float_RGBA_SNORM16;
      table[MESA_FORMAT_A_SNORM8] = pack_float_A_SNORM8;
      table[MESA_FORMAT_L_SNORM8] = pack_float_L_SNORM8;
      table[MESA_FORMAT_L8A8_SNORM] = pack_float_L8A8_SNORM;
      table[MESA_FORMAT_A8L8_SNORM] = pack_float_A8L8_SNORM;
      table[MESA_FORMAT_I_SNORM8] = pack_float_L_SNORM8; /* reused */
      table[MESA_FORMAT_A_SNORM16] = pack_float_A_SNORM16;
      table[MESA_FORMAT_L_SNORM16] = pack_float_L_SNORM16;
      table[MESA_FORMAT_LA_SNORM16] = pack_float_LA_SNORM16;
      table[MESA_FORMAT_I_SNORM16] = pack_float_L_SNORM16; /* reused */

      table[MESA_FORMAT_R9G9B9E5_FLOAT] = pack_float_R9G9B9E5_FLOAT;
      table[MESA_FORMAT_R11G11B10_FLOAT] = pack_float_R11G11B10_FLOAT;

      table[MESA_FORMAT_B4G4R4X4_UNORM] = pack_float_XRGB4444_UNORM;
      table[MESA_FORMAT_B5G5R5X1_UNORM] = pack_float_XRGB1555_UNORM;
      table[MESA_FORMAT_R8G8B8X8_SNORM] = pack_float_XBGR8888_SNORM;
      table[MESA_FORMAT_R8G8B8X8_SRGB] = pack_float_R8G8B8X8_SRGB;
      table[MESA_FORMAT_X8B8G8R8_SRGB] = pack_float_X8B8G8R8_SRGB;
      table[MESA_FORMAT_RGBX_UINT8] = NULL;
      table[MESA_FORMAT_RGBX_SINT8] = NULL;
      table[MESA_FORMAT_B10G10R10X2_UNORM] = pack_float_B10G10R10X2_UNORM;
      table[MESA_FORMAT_RGBX_UNORM16] = pack_float_RGBX_UNORM16;
      table[MESA_FORMAT_RGBX_SNORM16] = pack_float_RGBX_SNORM16;
      table[MESA_FORMAT_RGBX_FLOAT16] = pack_float_XBGR16161616_FLOAT;
      table[MESA_FORMAT_RGBX_UINT16] = NULL;
      table[MESA_FORMAT_RGBX_SINT16] = NULL;
      table[MESA_FORMAT_RGBX_FLOAT32] = pack_float_RGBX_FLOAT32;
      table[MESA_FORMAT_RGBX_UINT32] = NULL;
      table[MESA_FORMAT_RGBX_SINT32] = NULL;

      table[MESA_FORMAT_R10G10B10A2_UNORM] = pack_float_R10G10B10A2_UNORM;

      table[MESA_FORMAT_G8R8_SNORM] = pack_float_G8R8_SNORM;
      table[MESA_FORMAT_G16R16_SNORM] = pack_float_G16R16_SNORM;

      table[MESA_FORMAT_B8G8R8X8_SRGB] = pack_float_B8G8R8X8_SRGB;
      table[MESA_FORMAT_X8R8G8B8_SRGB] = pack_float_X8R8G8B8_SRGB;

      initialized = GL_TRUE;
   }

   return table[format];
}



static pack_float_rgba_row_func
get_pack_float_rgba_row_function(mesa_format format)
{
   static pack_float_rgba_row_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      /* We don't need a special row packing function for each format.
       * There's a generic fallback which uses a per-pixel packing function.
       */
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_A8B8G8R8_UNORM] = pack_row_float_A8B8G8R8_UNORM;
      table[MESA_FORMAT_R8G8B8A8_UNORM] = pack_row_float_R8G8B8A8_UNORM;
      table[MESA_FORMAT_B8G8R8A8_UNORM] = pack_row_float_B8G8R8A8_UNORM;
      table[MESA_FORMAT_A8R8G8B8_UNORM] = pack_row_float_A8R8G8B8_UNORM;
      table[MESA_FORMAT_X8B8G8R8_UNORM] = pack_row_float_A8B8G8R8_UNORM; /* reused */
      table[MESA_FORMAT_R8G8B8X8_UNORM] = pack_row_float_R8G8B8A8_UNORM; /* reused */
      table[MESA_FORMAT_B8G8R8X8_UNORM] = pack_row_float_B8G8R8X8_UNORM;
      table[MESA_FORMAT_X8R8G8B8_UNORM] = pack_row_float_X8R8G8B8_UNORM;
      table[MESA_FORMAT_BGR_UNORM8] = pack_row_float_BGR_UNORM8;
      table[MESA_FORMAT_RGB_UNORM8] = pack_row_float_RGB_UNORM8;
      table[MESA_FORMAT_B5G6R5_UNORM] = pack_row_float_B5G6R5_UNORM;
      table[MESA_FORMAT_R5G6B5_UNORM] = pack_row_float_R5G6B5_UNORM;

      initialized = GL_TRUE;
   }

   return table[format];
}



static pack_ubyte_rgba_row_func
get_pack_ubyte_rgba_row_function(mesa_format format)
{
   static pack_ubyte_rgba_row_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      /* We don't need a special row packing function for each format.
       * There's a generic fallback which uses a per-pixel packing function.
       */
      memset(table, 0, sizeof(table));

      table[MESA_FORMAT_A8B8G8R8_UNORM] = pack_row_ubyte_A8B8G8R8_UNORM;
      table[MESA_FORMAT_R8G8B8A8_UNORM] = pack_row_ubyte_R8G8B8A8_UNORM;
      table[MESA_FORMAT_B8G8R8A8_UNORM] = pack_row_ubyte_B8G8R8A8_UNORM;
      table[MESA_FORMAT_A8R8G8B8_UNORM] = pack_row_ubyte_A8R8G8B8_UNORM;
      table[MESA_FORMAT_X8B8G8R8_UNORM] = pack_row_ubyte_A8B8G8R8_UNORM; /* reused */
      table[MESA_FORMAT_R8G8B8X8_UNORM] = pack_row_ubyte_R8G8B8A8_UNORM; /* reused */
      table[MESA_FORMAT_B8G8R8X8_UNORM] = pack_row_ubyte_B8G8R8X8_UNORM;
      table[MESA_FORMAT_X8R8G8B8_UNORM] = pack_row_ubyte_X8R8G8B8_UNORM;
      table[MESA_FORMAT_BGR_UNORM8] = pack_row_ubyte_BGR_UNORM8;
      table[MESA_FORMAT_RGB_UNORM8] = pack_row_ubyte_RGB_UNORM8;
      table[MESA_FORMAT_B5G6R5_UNORM] = pack_row_ubyte_B5G6R5_UNORM;
      table[MESA_FORMAT_R5G6B5_UNORM] = pack_row_ubyte_R5G6B5_UNORM;

      initialized = GL_TRUE;
   }

   return table[format];
}



/**
 * Pack a row of GLfloat rgba[4] values to the destination.
 */
void
_mesa_pack_float_rgba_row(mesa_format format, GLuint n,
                          const GLfloat src[][4], void *dst)
{
   pack_float_rgba_row_func packrow = get_pack_float_rgba_row_function(format);
   if (packrow) {
      /* use "fast" function */
      packrow(n, src, dst);
   }
   else {
      /* slower fallback */
      gl_pack_float_rgba_func pack = _mesa_get_pack_float_rgba_function(format);
      GLuint dstStride = _mesa_get_format_bytes(format);
      GLubyte *dstPtr = (GLubyte *) dst;
      GLuint i;

      assert(pack);
      if (!pack)
         return;

      for (i = 0; i < n; i++) {
         pack(src[i], dstPtr);
         dstPtr += dstStride;
      }
   }
}


/**
 * Pack a row of GLubyte rgba[4] values to the destination.
 */
void
_mesa_pack_ubyte_rgba_row(mesa_format format, GLuint n,
                          const GLubyte src[][4], void *dst)
{
   pack_ubyte_rgba_row_func packrow = get_pack_ubyte_rgba_row_function(format);
   if (packrow) {
      /* use "fast" function */
      packrow(n, src, dst);
   }
   else {
      /* slower fallback */
      gl_pack_ubyte_rgba_func pack = _mesa_get_pack_ubyte_rgba_function(format);
      const GLuint stride = _mesa_get_format_bytes(format);
      GLubyte *d = ((GLubyte *) dst);
      GLuint i;

      assert(pack);
      if (!pack)
         return;

      for (i = 0; i < n; i++) {
         pack(src[i], d);
         d += stride;
      }
   }
}


/**
 * Pack a 2D image of ubyte RGBA pixels in the given format.
 * \param srcRowStride  source image row stride in bytes
 * \param dstRowStride  destination image row stride in bytes
 */
void
_mesa_pack_ubyte_rgba_rect(mesa_format format, GLuint width, GLuint height,
                           const GLubyte *src, GLint srcRowStride,
                           void *dst, GLint dstRowStride)
{
   pack_ubyte_rgba_row_func packrow = get_pack_ubyte_rgba_row_function(format);
   GLubyte *dstUB = (GLubyte *) dst;
   GLuint i;

   if (packrow) {
      if (srcRowStride == width * 4 * sizeof(GLubyte) &&
          dstRowStride == _mesa_format_row_stride(format, width)) {
         /* do whole image at once */
         packrow(width * height, (const GLubyte (*)[4]) src, dst);
      }
      else {
         /* row by row */
         for (i = 0; i < height; i++) {
            packrow(width, (const GLubyte (*)[4]) src, dstUB);
            src += srcRowStride;
            dstUB += dstRowStride;
         }
      }
   }
   else {
      /* slower fallback */
      for (i = 0; i < height; i++) {
         _mesa_pack_ubyte_rgba_row(format, width,
                                   (const GLubyte (*)[4]) src, dstUB);
         src += srcRowStride;
         dstUB += dstRowStride;
      }
   }
}



/**
 ** Pack float Z pixels
 **/

static void
pack_float_S8_UINT_Z24_UNORM(const GLfloat *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = (GLdouble) 0xffffff;
   GLuint s = *d & 0xff;
   GLuint z = (GLuint) (*src * scale);
   assert(z <= 0xffffff);
   *d = (z << 8) | s;
}

static void
pack_float_Z24_UNORM_S8_UINT(const GLfloat *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = (GLdouble) 0xffffff;
   GLuint s = *d & 0xff000000;
   GLuint z = (GLuint) (*src * scale);
   assert(z <= 0xffffff);
   *d = s | z;
}

static void
pack_float_Z_UNORM16(const GLfloat *src, void *dst)
{
   GLushort *d = ((GLushort *) dst);
   const GLfloat scale = (GLfloat) 0xffff;
   *d = (GLushort) (*src * scale);
}

static void
pack_float_Z_UNORM32(const GLfloat *src, void *dst)
{
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = (GLdouble) 0xffffffff;
   *d = (GLuint) (*src * scale);
}

static void
pack_float_Z_FLOAT32(const GLfloat *src, void *dst)
{
   GLfloat *d = (GLfloat *) dst;
   *d = *src;
}

gl_pack_float_z_func
_mesa_get_pack_float_z_func(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      return pack_float_S8_UINT_Z24_UNORM;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      return pack_float_Z24_UNORM_S8_UINT;
   case MESA_FORMAT_Z_UNORM16:
      return pack_float_Z_UNORM16;
   case MESA_FORMAT_Z_UNORM32:
      return pack_float_Z_UNORM32;
   case MESA_FORMAT_Z_FLOAT32:
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return pack_float_Z_FLOAT32;
   default:
      _mesa_problem(NULL,
                    "unexpected format in _mesa_get_pack_float_z_func()");
      return NULL;
   }
}



/**
 ** Pack uint Z pixels.  The incoming src value is always in
 ** the range [0, 2^32-1].
 **/

static void
pack_uint_S8_UINT_Z24_UNORM(const GLuint *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *d & 0xff;
   GLuint z = *src & 0xffffff00;
   *d = z | s;
}

static void
pack_uint_Z24_UNORM_S8_UINT(const GLuint *src, void *dst)
{
   /* don't disturb the stencil values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *d & 0xff000000;
   GLuint z = *src >> 8;
   *d = s | z;
}

static void
pack_uint_Z_UNORM16(const GLuint *src, void *dst)
{
   GLushort *d = ((GLushort *) dst);
   *d = *src >> 16;
}

static void
pack_uint_Z_UNORM32(const GLuint *src, void *dst)
{
   GLuint *d = ((GLuint *) dst);
   *d = *src;
}

static void
pack_uint_Z_FLOAT32(const GLuint *src, void *dst)
{
   GLuint *d = ((GLuint *) dst);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
   *d = (GLuint) (*src * scale);
   assert(*d >= 0.0f);
   assert(*d <= 1.0f);
}

static void
pack_uint_Z_FLOAT32_X24S8(const GLuint *src, void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
   *d = (GLfloat) (*src * scale);
   assert(*d >= 0.0f);
   assert(*d <= 1.0f);
}

gl_pack_uint_z_func
_mesa_get_pack_uint_z_func(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      return pack_uint_S8_UINT_Z24_UNORM;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      return pack_uint_Z24_UNORM_S8_UINT;
   case MESA_FORMAT_Z_UNORM16:
      return pack_uint_Z_UNORM16;
   case MESA_FORMAT_Z_UNORM32:
      return pack_uint_Z_UNORM32;
   case MESA_FORMAT_Z_FLOAT32:
      return pack_uint_Z_FLOAT32;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return pack_uint_Z_FLOAT32_X24S8;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_get_pack_uint_z_func()");
      return NULL;
   }
}


/**
 ** Pack ubyte stencil pixels
 **/

static void
pack_ubyte_stencil_Z24_S8(const GLubyte *src, void *dst)
{
   /* don't disturb the Z values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *src;
   GLuint z = *d & 0xffffff00;
   *d = z | s;
}

static void
pack_ubyte_stencil_S8_Z24(const GLubyte *src, void *dst)
{
   /* don't disturb the Z values */
   GLuint *d = ((GLuint *) dst);
   GLuint s = *src << 24;
   GLuint z = *d & 0xffffff;
   *d = s | z;
}

static void
pack_ubyte_stencil_S8(const GLubyte *src, void *dst)
{
   GLubyte *d = (GLubyte *) dst;
   *d = *src;
}

static void
pack_ubyte_stencil_Z32_FLOAT_X24S8(const GLubyte *src, void *dst)
{
   GLfloat *d = ((GLfloat *) dst);
   d[1] = *src;
}


gl_pack_ubyte_stencil_func
_mesa_get_pack_ubyte_stencil_func(mesa_format format)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      return pack_ubyte_stencil_Z24_S8;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      return pack_ubyte_stencil_S8_Z24;
   case MESA_FORMAT_S_UINT8:
      return pack_ubyte_stencil_S8;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      return pack_ubyte_stencil_Z32_FLOAT_X24S8;
   default:
      _mesa_problem(NULL,
                    "unexpected format in _mesa_pack_ubyte_stencil_func()");
      return NULL;
   }
}



void
_mesa_pack_float_z_row(mesa_format format, GLuint n,
                       const GLfloat *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = (GLdouble) 0xffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff;
            GLuint z = (GLuint) (src[i] * scale);
            assert(z <= 0xffffff);
            d[i] = (z << 8) | s;
         }
      }
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = (GLdouble) 0xffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff000000;
            GLuint z = (GLuint) (src[i] * scale);
            assert(z <= 0xffffff);
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_Z_UNORM16:
      {
         GLushort *d = ((GLushort *) dst);
         const GLfloat scale = (GLfloat) 0xffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = (GLushort) (src[i] * scale);
         }
      }
      break;
   case MESA_FORMAT_Z_UNORM32:
      {
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = (GLdouble) 0xffffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = (GLuint) (src[i] * scale);
         }
      }
      break;
   case MESA_FORMAT_Z_FLOAT32:
      memcpy(dst, src, n * sizeof(GLfloat));
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      {
         struct z32f_x24s8 *d = (struct z32f_x24s8 *) dst;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i].z = src[i];
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_float_z_row()");
   }
}


/**
 * The incoming Z values are always in the range [0, 0xffffffff].
 */
void
_mesa_pack_uint_z_row(mesa_format format, GLuint n,
                      const GLuint *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff;
            GLuint z = src[i] & 0xffffff00;
            d[i] = z | s;
         }
      }
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      {
         /* don't disturb the stencil values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = d[i] & 0xff000000;
            GLuint z = src[i] >> 8;
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_Z_UNORM16:
      {
         GLushort *d = ((GLushort *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = src[i] >> 16;
         }
      }
      break;
   case MESA_FORMAT_Z_UNORM32:
      memcpy(dst, src, n * sizeof(GLfloat));
      break;
   case MESA_FORMAT_Z_FLOAT32:
      {
         GLuint *d = ((GLuint *) dst);
         const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i] = (GLuint) (src[i] * scale);
            assert(d[i] >= 0.0f);
            assert(d[i] <= 1.0f);
         }
      }
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      {
         struct z32f_x24s8 *d = (struct z32f_x24s8 *) dst;
         const GLdouble scale = 1.0 / (GLdouble) 0xffffffff;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i].z = (GLfloat) (src[i] * scale);
            assert(d[i].z >= 0.0f);
            assert(d[i].z <= 1.0f);
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_uint_z_row()");
   }
}


void
_mesa_pack_ubyte_stencil_row(mesa_format format, GLuint n,
                             const GLubyte *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      {
         /* don't disturb the Z values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = src[i];
            GLuint z = d[i] & 0xffffff00;
            d[i] = z | s;
         }
      }
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      {
         /* don't disturb the Z values */
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = src[i] << 24;
            GLuint z = d[i] & 0xffffff;
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_S_UINT8:
      memcpy(dst, src, n * sizeof(GLubyte));
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      {
         struct z32f_x24s8 *d = (struct z32f_x24s8 *) dst;
         GLuint i;
         for (i = 0; i < n; i++) {
            d[i].x24s8 = src[i];
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format in _mesa_pack_ubyte_stencil_row()");
   }
}


/**
 * Incoming Z/stencil values are always in uint_24_8 format.
 */
void
_mesa_pack_uint_24_8_depth_stencil_row(mesa_format format, GLuint n,
                                       const GLuint *src, void *dst)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      memcpy(dst, src, n * sizeof(GLuint));
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      {
         GLuint *d = ((GLuint *) dst);
         GLuint i;
         for (i = 0; i < n; i++) {
            GLuint s = src[i] << 24;
            GLuint z = src[i] >> 8;
            d[i] = s | z;
         }
      }
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      {
         const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
         struct z32f_x24s8 *d = (struct z32f_x24s8 *) dst;
         GLuint i;
         for (i = 0; i < n; i++) {
            GLfloat z = (GLfloat) ((src[i] >> 8) * scale);
            d[i].z = z;
            d[i].x24s8 = src[i];
         }
      }
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_pack_ubyte_s_row",
                    _mesa_get_format_name(format));
      return;
   }
}



/**
 * Convert a boolean color mask to a packed color where each channel of
 * the packed value at dst will be 0 or ~0 depending on the colorMask.
 */
void
_mesa_pack_colormask(mesa_format format, const GLubyte colorMask[4], void *dst)
{
   GLfloat maskColor[4];

   switch (_mesa_get_format_datatype(format)) {
   case GL_UNSIGNED_NORMALIZED:
      /* simple: 1.0 will convert to ~0 in the right bit positions */
      maskColor[0] = colorMask[0] ? 1.0f : 0.0f;
      maskColor[1] = colorMask[1] ? 1.0f : 0.0f;
      maskColor[2] = colorMask[2] ? 1.0f : 0.0f;
      maskColor[3] = colorMask[3] ? 1.0f : 0.0f;
      _mesa_pack_float_rgba_row(format, 1,
                                (const GLfloat (*)[4]) maskColor, dst);
      break;
   case GL_SIGNED_NORMALIZED:
   case GL_FLOAT:
      /* These formats are harder because it's hard to know the floating
       * point values that will convert to ~0 for each color channel's bits.
       * This solution just generates a non-zero value for each color channel
       * then fixes up the non-zero values to be ~0.
       * Note: we'll need to add special case code if we ever have to deal
       * with formats with unequal color channel sizes, like R11_G11_B10.
       * We issue a warning below for channel sizes other than 8,16,32.
       */
      {
         GLuint bits = _mesa_get_format_max_bits(format); /* bits per chan */
         GLuint bytes = _mesa_get_format_bytes(format);
         GLuint i;

         /* this should put non-zero values into the channels of dst */
         maskColor[0] = colorMask[0] ? -1.0f : 0.0f;
         maskColor[1] = colorMask[1] ? -1.0f : 0.0f;
         maskColor[2] = colorMask[2] ? -1.0f : 0.0f;
         maskColor[3] = colorMask[3] ? -1.0f : 0.0f;
         _mesa_pack_float_rgba_row(format, 1,
                                   (const GLfloat (*)[4]) maskColor, dst);

         /* fix-up the dst channels by converting non-zero values to ~0 */
         if (bits == 8) {
            GLubyte *d = (GLubyte *) dst;
            for (i = 0; i < bytes; i++) {
               d[i] = d[i] ? 0xff : 0x0;
            }
         }
         else if (bits == 16) {
            GLushort *d = (GLushort *) dst;
            for (i = 0; i < bytes / 2; i++) {
               d[i] = d[i] ? 0xffff : 0x0;
            }
         }
         else if (bits == 32) {
            GLuint *d = (GLuint *) dst;
            for (i = 0; i < bytes / 4; i++) {
               d[i] = d[i] ? 0xffffffffU : 0x0;
            }
         }
         else {
            _mesa_problem(NULL, "unexpected size in _mesa_pack_colormask()");
            return;
         }
      }
      break;
   default:
      _mesa_problem(NULL, "unexpected format data type in gen_color_mask()");
      return;
   }
}
