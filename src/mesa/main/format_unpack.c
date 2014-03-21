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


#include "colormac.h"
#include "format_unpack.h"
#include "macros.h"
#include "../../gallium/auxiliary/util/u_format_rgb9e5.h"
#include "../../gallium/auxiliary/util/u_format_r11g11b10f.h"


/** Helper struct for MESA_FORMAT_Z32_FLOAT_S8X24_UINT */
struct z32f_x24s8
{
   float z;
   uint32_t x24s8;
};


/* Expand 1, 2, 3, 4, 5, 6-bit values to fill 8 bits */

#define EXPAND_1_8(X)  ( (X) ? 0xff : 0x0 )

#define EXPAND_2_8(X)  ( ((X) << 6) | ((X) << 4) | ((X) << 2) | (X) )

#define EXPAND_3_8(X)  ( ((X) << 5) | ((X) << 2) | ((X) >> 1) )

#define EXPAND_4_8(X)  ( ((X) << 4) | (X) )

#define EXPAND_5_8(X)  ( ((X) << 3) | ((X) >> 2) )

#define EXPAND_6_8(X)  ( ((X) << 2) | ((X) >> 4) )


/**
 * Convert an 8-bit sRGB value from non-linear space to a
 * linear RGB value in [0, 1].
 * Implemented with a 256-entry lookup table.
 */
GLfloat
_mesa_nonlinear_to_linear(GLubyte cs8)
{
   static GLfloat table[256];
   static GLboolean tableReady = GL_FALSE;
   if (!tableReady) {
      /* compute lookup table now */
      GLuint i;
      for (i = 0; i < 256; i++) {
         const GLfloat cs = UBYTE_TO_FLOAT(i);
         if (cs <= 0.04045) {
            table[i] = cs / 12.92f;
         }
         else {
            table[i] = (GLfloat) pow((cs + 0.055) / 1.055, 2.4);
         }
      }
      tableReady = GL_TRUE;
   }
   return table[cs8];
}


/**********************************************************************/
/*  Unpack, returning GLfloat colors                                  */
/**********************************************************************/

typedef void (*unpack_rgba_func)(const void *src, GLfloat dst[][4], GLuint n);


static void
unpack_A8B8G8R8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
   }
}

static void
unpack_R8G8B8A8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
   }
}

static void
unpack_B8G8R8A8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
   }
}

static void
unpack_A8R8G8B8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
   }
}

static void
unpack_RGBX8888(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_RGBX8888_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_B8G8R8X8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i]      ) & 0xff );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_X8R8G8B8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( (s[i] >>  8) & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( (s[i] >> 24)        );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_BGR_UNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( s[i*3+2] );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( s[i*3+1] );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i*3+0] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RGB_UNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( s[i*3+0] );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( s[i*3+1] );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i*3+2] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_B5G6R5_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 11) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >> 5 ) & 0x3f) * (1.0F / 63.0F);
      dst[i][BCOMP] = ((s[i]      ) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_R5G6B5_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   /* Warning: this function does not match the current Mesa definition
    * of MESA_FORMAT_R5G6B5_UNORM.
    */
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLuint t = (s[i] >> 8) | (s[i] << 8); /* byte swap */
      dst[i][RCOMP] = UBYTE_TO_FLOAT( ((t >> 8) & 0xf8) | ((t >> 13) & 0x7) );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( ((t >> 3) & 0xfc) | ((t >>  9) & 0x3) );
      dst[i][BCOMP] = UBYTE_TO_FLOAT( ((t << 3) & 0xf8) | ((t >>  2) & 0x7) );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_B4G4R4A4_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >>  8) & 0xf) * (1.0F / 15.0F);
      dst[i][GCOMP] = ((s[i] >>  4) & 0xf) * (1.0F / 15.0F);
      dst[i][BCOMP] = ((s[i]      ) & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = ((s[i] >> 12) & 0xf) * (1.0F / 15.0F);
   }
}

static void
unpack_A4R4G4B4_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >>  4) & 0xf) * (1.0F / 15.0F);
      dst[i][GCOMP] = ((s[i] >>  8) & 0xf) * (1.0F / 15.0F);
      dst[i][BCOMP] = ((s[i] >> 12) & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = ((s[i]      ) & 0xf) * (1.0F / 15.0F);
   }
}

static void
unpack_A1B5G5R5_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 11) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >>  6) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((s[i] >>  1) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = ((s[i]      ) & 0x01) * 1.0F;
   }
}

static void
unpack_B5G5R5A1_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 10) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >>  5) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((s[i] >>  0) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = ((s[i] >> 15) & 0x01) * 1.0F;
   }
}

static void
unpack_A1R5G5B5_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   /* Warning: this function does not match the current Mesa definition
    * of MESA_FORMAT_A1R5G5B5_UNORM.
    */
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLushort tmp = (s[i] << 8) | (s[i] >> 8); /* byteswap */
      dst[i][RCOMP] = ((tmp >> 10) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((tmp >>  5) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((tmp >>  0) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = ((tmp >> 15) & 0x01) * 1.0F;
   }
}

static void
unpack_L4A4_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (s[i] & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = ((s[i] >> 4) & 0xf) * (1.0F / 15.0F);
   }
}

static void
unpack_L8A8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i] & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] >> 8 );
   }
}

static void
unpack_A8L8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = UBYTE_TO_FLOAT( s[i] >> 8 );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] & 0xff );
   }
}

static void
unpack_L16A16_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i] & 0xffff );
      dst[i][ACOMP] = USHORT_TO_FLOAT( s[i] >> 16 );
   }
}

static void
unpack_A16L16_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i] >> 16 );
      dst[i][ACOMP] = USHORT_TO_FLOAT( s[i] & 0xffff );
   }
}

static void
unpack_B2G3R3_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 5) & 0x7) * (1.0F / 7.0F);
      dst[i][GCOMP] = ((s[i] >> 2) & 0x7) * (1.0F / 7.0F);
      dst[i][BCOMP] = ((s[i]     ) & 0x3) * (1.0F / 3.0F);
      dst[i][ACOMP] = 1.0F;
   }
}


static void
unpack_A_UNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = UBYTE_TO_FLOAT(s[i]);
   }
}

static void
unpack_A_UNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = USHORT_TO_FLOAT(s[i]);
   }
}

static void
unpack_L_UNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = UBYTE_TO_FLOAT(s[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_L_UNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = USHORT_TO_FLOAT(s[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_I_UNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = UBYTE_TO_FLOAT(s[i]);
   }
}

static void
unpack_I_UNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = USHORT_TO_FLOAT(s[i]);
   }
}

static void
unpack_YCBCR(const void *src, GLfloat dst[][4], GLuint n)
{
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLushort *src0 = ((const GLushort *) src) + i * 2; /* even */
      const GLushort *src1 = src0 + 1;         /* odd */
      const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
      const GLubyte cb = *src0 & 0xff;         /* chroma U */
      const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
      const GLubyte cr = *src1 & 0xff;         /* chroma V */
      const GLubyte y = (i & 1) ? y1 : y0;     /* choose even/odd luminance */
      GLfloat r = 1.164F * (y - 16) + 1.596F * (cr - 128);
      GLfloat g = 1.164F * (y - 16) - 0.813F * (cr - 128) - 0.391F * (cb - 128);
      GLfloat b = 1.164F * (y - 16) + 2.018F * (cb - 128);
      r *= (1.0F / 255.0F);
      g *= (1.0F / 255.0F);
      b *= (1.0F / 255.0F);
      dst[i][RCOMP] = CLAMP(r, 0.0F, 1.0F);
      dst[i][GCOMP] = CLAMP(g, 0.0F, 1.0F);
      dst[i][BCOMP] = CLAMP(b, 0.0F, 1.0F);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_YCBCR_REV(const void *src, GLfloat dst[][4], GLuint n)
{
   GLuint i;
   for (i = 0; i < n; i++) {
      const GLushort *src0 = ((const GLushort *) src) + i * 2; /* even */
      const GLushort *src1 = src0 + 1;         /* odd */
      const GLubyte y0 = *src0 & 0xff;         /* luminance */
      const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
      const GLubyte y1 = *src1 & 0xff;         /* luminance */
      const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
      const GLubyte y = (i & 1) ? y1 : y0;     /* choose even/odd luminance */
      GLfloat r = 1.164F * (y - 16) + 1.596F * (cr - 128);
      GLfloat g = 1.164F * (y - 16) - 0.813F * (cr - 128) - 0.391F * (cb - 128);
      GLfloat b = 1.164F * (y - 16) + 2.018F * (cb - 128);
      r *= (1.0F / 255.0F);
      g *= (1.0F / 255.0F);
      b *= (1.0F / 255.0F);
      dst[i][RCOMP] = CLAMP(r, 0.0F, 1.0F);
      dst[i][GCOMP] = CLAMP(g, 0.0F, 1.0F);
      dst[i][BCOMP] = CLAMP(b, 0.0F, 1.0F);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_R_UNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] = UBYTE_TO_FLOAT(s[i]);
      dst[i][1] =
      dst[i][2] = 0.0F;
      dst[i][3] = 1.0F;
   }
}

static void
unpack_R8G8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( s[i] & 0xff );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( s[i] >> 8 );
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_G8R8_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = UBYTE_TO_FLOAT( s[i] >> 8 );
      dst[i][GCOMP] = UBYTE_TO_FLOAT( s[i] & 0xff );
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_R_UNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = USHORT_TO_FLOAT(s[i]);
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_R16G16_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = USHORT_TO_FLOAT( s[i] & 0xffff );
      dst[i][GCOMP] = USHORT_TO_FLOAT( s[i] >> 16 );
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_G16R16_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = USHORT_TO_FLOAT( s[i] >> 16 );
      dst[i][GCOMP] = USHORT_TO_FLOAT( s[i] & 0xffff );
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_B10G10R10A2_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 20) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][GCOMP] = ((s[i] >> 10) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][BCOMP] = ((s[i] >>  0) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][ACOMP] = ((s[i] >> 30) &  0x03) * (1.0F / 3.0F);
   }
}


static void
unpack_B10G10R10A2_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat)((s[i] >> 20) & 0x3ff);
      dst[i][GCOMP] = (GLfloat)((s[i] >> 10) & 0x3ff);
      dst[i][BCOMP] = (GLfloat)((s[i] >>  0) & 0x3ff);
      dst[i][ACOMP] = (GLfloat)((s[i] >> 30) &  0x03);
   }
}


static void
unpack_R10G10B10A2_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat)((s[i] >>  0) & 0x3ff);
      dst[i][GCOMP] = (GLfloat)((s[i] >> 10) & 0x3ff);
      dst[i][BCOMP] = (GLfloat)((s[i] >> 20) & 0x3ff);
      dst[i][ACOMP] = (GLfloat)((s[i] >> 30) &  0x03);
   }
}


static void
unpack_S8_UINT_Z24_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = (GLfloat) ((s[i] >> 8) * scale);
      dst[i][3] = 1.0F;
      ASSERT(dst[i][0] >= 0.0F);
      ASSERT(dst[i][0] <= 1.0F);
   }
}

static void
unpack_Z24_UNORM_S8_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = (float) ((s[i] & 0x00ffffff) * scale);
      dst[i][3] = 1.0F;
      ASSERT(dst[i][0] >= 0.0F);
      ASSERT(dst[i][0] <= 1.0F);
   }
}

static void
unpack_Z_UNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = s[i] * (1.0F / 65535.0F);
      dst[i][3] = 1.0F;
   }
}

static void
unpack_Z24_UNORM_X8_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   unpack_Z24_UNORM_S8_UINT(src, dst, n);
}

static void
unpack_X8_UINT_Z24_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   unpack_S8_UINT_Z24_UNORM(src, dst, n);
}

static void
unpack_Z_UNORM32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = s[i] * (1.0F / 0xffffffff);
      dst[i][3] = 1.0F;
   }
}

static void
unpack_Z32_FLOAT_S8X24_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const struct z32f_x24s8 *s = (const struct z32f_x24s8 *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = s[i].z;
      dst[i][3] = 1.0F;
   }
}

static void
unpack_Z_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = ((const GLfloat *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = s[i];
      dst[i][3] = 1.0F;
   }
}


static void
unpack_S8(const void *src, GLfloat dst[][4], GLuint n)
{
   /* should never be used */
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] =
      dst[i][1] =
      dst[i][2] = 0.0F;
      dst[i][3] = 1.0F;
   }
}


static void
unpack_BGR_SRGB8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_nonlinear_to_linear(s[i*3+2]);
      dst[i][GCOMP] = _mesa_nonlinear_to_linear(s[i*3+1]);
      dst[i][BCOMP] = _mesa_nonlinear_to_linear(s[i*3+0]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_A8B8G8R8_SRGB(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_nonlinear_to_linear( (s[i] >> 24) );
      dst[i][GCOMP] = _mesa_nonlinear_to_linear( (s[i] >> 16) & 0xff );
      dst[i][BCOMP] = _mesa_nonlinear_to_linear( (s[i] >>  8) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] & 0xff ); /* linear! */
   }
}

static void
unpack_B8G8R8A8_SRGB(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_nonlinear_to_linear( (s[i] >> 16) & 0xff );
      dst[i][GCOMP] = _mesa_nonlinear_to_linear( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = _mesa_nonlinear_to_linear( (s[i]      ) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] >> 24 ); /* linear! */
   }
}

static void
unpack_R8G8B8A8_SRGB(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_nonlinear_to_linear( (s[i]      ) & 0xff );
      dst[i][GCOMP] = _mesa_nonlinear_to_linear( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = _mesa_nonlinear_to_linear( (s[i] >> 16) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] >> 24 ); /* linear! */
   }
}

static void
unpack_L_SRGB8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = _mesa_nonlinear_to_linear(s[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_L8A8_SRGB(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = _mesa_nonlinear_to_linear(s[i] & 0xff);
      dst[i][ACOMP] = UBYTE_TO_FLOAT(s[i] >> 8); /* linear! */
   }
}

static void
unpack_SRGB_DXT1(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_SRGBA_DXT1(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_SRGBA_DXT3(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_SRGBA_DXT5(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_RGB_FXT1(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_RGBA_FXT1(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_RGB_DXT1(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_RGBA_DXT1(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_RGBA_DXT3(const void *src, GLfloat dst[][4], GLuint n)
{
}

static void
unpack_RGBA_DXT5(const void *src, GLfloat dst[][4], GLuint n)
{
}


static void
unpack_RGBA_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*4+0];
      dst[i][GCOMP] = s[i*4+1];
      dst[i][BCOMP] = s[i*4+2];
      dst[i][ACOMP] = s[i*4+3];
   }
}

static void
unpack_RGBA_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_half_to_float(s[i*4+0]);
      dst[i][GCOMP] = _mesa_half_to_float(s[i*4+1]);
      dst[i][BCOMP] = _mesa_half_to_float(s[i*4+2]);
      dst[i][ACOMP] = _mesa_half_to_float(s[i*4+3]);
   }
}

static void
unpack_RGB_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*3+0];
      dst[i][GCOMP] = s[i*3+1];
      dst[i][BCOMP] = s[i*3+2];
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RGB_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_half_to_float(s[i*3+0]);
      dst[i][GCOMP] = _mesa_half_to_float(s[i*3+1]);
      dst[i][BCOMP] = _mesa_half_to_float(s[i*3+2]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_A_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = s[i];
   }
}

static void
unpack_A_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = _mesa_half_to_float(s[i]);
   }
}

static void
unpack_L_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = s[i];
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_L_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = _mesa_half_to_float(s[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_LA_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = s[i*2+0];
      dst[i][ACOMP] = s[i*2+1];
   }
}

static void
unpack_LA_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = _mesa_half_to_float(s[i*2+0]);
      dst[i][ACOMP] = _mesa_half_to_float(s[i*2+1]);
   }
}

static void
unpack_I_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = s[i];
   }
}

static void
unpack_I_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = _mesa_half_to_float(s[i]);
   }
}

static void
unpack_R_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i];
      dst[i][GCOMP] = 0.0F;
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_R_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_half_to_float(s[i]);
      dst[i][GCOMP] = 0.0F;
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RG_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*2+0];
      dst[i][GCOMP] = s[i*2+1];
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RG_FLOAT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLhalfARB *s = (const GLhalfARB *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_half_to_float(s[i*2+0]);
      dst[i][GCOMP] = _mesa_half_to_float(s[i*2+1]);
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_ALPHA_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_ALPHA_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_ALPHA_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_ALPHA_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_ALPHA_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_ALPHA_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_INTENSITY_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_INTENSITY_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_INTENSITY_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_INTENSITY_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_INTENSITY_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_INTENSITY_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = (GLfloat) s[i];
   }
}

static void
unpack_LUMINANCE_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = dst[i][GCOMP] = dst[i][BCOMP] = (GLfloat) s[i];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_LUMINANCE_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = dst[i][GCOMP] = dst[i][BCOMP] = (GLfloat) s[i];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_LUMINANCE_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = dst[i][GCOMP] = dst[i][BCOMP] = (GLfloat) s[i];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_LUMINANCE_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = dst[i][GCOMP] = dst[i][BCOMP] = (GLfloat) s[i];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_LUMINANCE_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = dst[i][GCOMP] = dst[i][BCOMP] = (GLfloat) s[i];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_LUMINANCE_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = dst[i][GCOMP] = dst[i][BCOMP] = (GLfloat) s[i];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_LUMINANCE_ALPHA_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (GLfloat) s[2*i+0];
      dst[i][ACOMP] = (GLfloat) s[2*i+1];
   }
}

static void
unpack_LUMINANCE_ALPHA_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (GLfloat) s[2*i+0];
      dst[i][ACOMP] = (GLfloat) s[2*i+1];
   }
}

static void
unpack_LUMINANCE_ALPHA_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (GLfloat) s[2*i+0];
      dst[i][ACOMP] = (GLfloat) s[2*i+1];
   }
}

static void
unpack_LUMINANCE_ALPHA_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (GLfloat) s[2*i+0];
      dst[i][ACOMP] = (GLfloat) s[2*i+1];
   }
}

static void
unpack_LUMINANCE_ALPHA_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (GLfloat) s[2*i+0];
      dst[i][ACOMP] = (GLfloat) s[2*i+1];
   }
}

static void
unpack_LUMINANCE_ALPHA_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = (GLfloat) s[2*i+0];
      dst[i][ACOMP] = (GLfloat) s[2*i+1];
   }
}

static void
unpack_R_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i];
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RG_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*2+0];
      dst[i][GCOMP] = (GLfloat) s[i*2+1];
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGB_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*3+0];
      dst[i][GCOMP] = (GLfloat) s[i*3+1];
      dst[i][BCOMP] = (GLfloat) s[i*3+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBA_INT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_R_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i];
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RG_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*2+0];
      dst[i][GCOMP] = (GLfloat) s[i*2+1];
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGB_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*3+0];
      dst[i][GCOMP] = (GLfloat) s[i*3+1];
      dst[i][BCOMP] = (GLfloat) s[i*3+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBA_INT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_R_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i];
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RG_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*2+0];
      dst[i][GCOMP] = (GLfloat) s[i*2+1];
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGB_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*3+0];
      dst[i][GCOMP] = (GLfloat) s[i*3+1];
      dst[i][BCOMP] = (GLfloat) s[i*3+2];
      dst[i][ACOMP] = 1.0;
   }
}


static void
unpack_RGBA_INT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_R_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i];
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RG_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*2+0];
      dst[i][GCOMP] = (GLfloat) s[i*2+1];
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGB_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*3+0];
      dst[i][GCOMP] = (GLfloat) s[i*3+1];
      dst[i][BCOMP] = (GLfloat) s[i*3+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBA_UINT8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_R_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i];
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RG_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*2+0];
      dst[i][GCOMP] = (GLfloat) s[i*2+1];
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGB_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*3+0];
      dst[i][GCOMP] = (GLfloat) s[i*3+1];
      dst[i][BCOMP] = (GLfloat) s[i*3+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBA_UINT16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_R_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i];
      dst[i][GCOMP] = 0.0;
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RG_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*2+0];
      dst[i][GCOMP] = (GLfloat) s[i*2+1];
      dst[i][BCOMP] = 0.0;
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGB_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*3+0];
      dst[i][GCOMP] = (GLfloat) s[i*3+1];
      dst[i][BCOMP] = (GLfloat) s[i*3+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBA_UINT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = (GLfloat) s[i*4+3];
   }
}

static void
unpack_DUDV8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT(s[i*2+0]);
      dst[i][GCOMP] = BYTE_TO_FLOAT(s[i*2+1]);
      dst[i][BCOMP] = 0;
      dst[i][ACOMP] = 0;
   }
}

static void
unpack_R_SNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = ((const GLbyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( s[i] );
      dst[i][GCOMP] = 0.0F;
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_R8G8_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] & 0xff) );
      dst[i][GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 8) );
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_X8B8G8R8_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 24) );
      dst[i][GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 16) );
      dst[i][BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >>  8) );
      dst[i][ACOMP] = 1.0f;
   }
}

static void
unpack_A8B8G8R8_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 24) );
      dst[i][GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 16) );
      dst[i][BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >>  8) );
      dst[i][ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i]      ) );
   }
}

static void
unpack_R8G8B8A8_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i]      ) );
      dst[i][GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >>  8) );
      dst[i][BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 16) );
      dst[i][ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 24) );
   }
}

static void
unpack_R_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = ((const GLshort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( s[i] );
      dst[i][GCOMP] = 0.0F;
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_R16G16_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( (GLshort) (s[i] & 0xffff) );
      dst[i][GCOMP] = SHORT_TO_FLOAT_TEX( (GLshort) (s[i] >> 16) );
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RGB_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( s[i*3+0] );
      dst[i][GCOMP] = SHORT_TO_FLOAT_TEX( s[i*3+1] );
      dst[i][BCOMP] = SHORT_TO_FLOAT_TEX( s[i*3+2] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_RGBA_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+0] );
      dst[i][GCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+1] );
      dst[i][BCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+2] );
      dst[i][ACOMP] = SHORT_TO_FLOAT_TEX( s[i*4+3] );
   }
}

static void
unpack_RGBA_16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = USHORT_TO_FLOAT( s[i*4+0] );
      dst[i][GCOMP] = USHORT_TO_FLOAT( s[i*4+1] );
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i*4+2] );
      dst[i][ACOMP] = USHORT_TO_FLOAT( s[i*4+3] );
   }
}

static void
unpack_RED_RGTC1(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_SIGNED_RED_RGTC1(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_RG_RGTC2(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_SIGNED_RG_RGTC2(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_L_LATC1(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_SIGNED_L_LATC1(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_LA_LATC2(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_SIGNED_LA_LATC2(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC1_RGB8(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_RGB8(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_SRGB8(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_RGBA8_EAC(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_SRGB8_ALPHA8_EAC(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_R11_EAC(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_RG11_EAC(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_SIGNED_R11_EAC(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_SIGNED_RG11_EAC(const void *src, GLfloat dst[][4], GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_RGB8_PUNCHTHROUGH_ALPHA1(const void *src, GLfloat dst[][4],
                                      GLuint n)
{
   /* XXX to do */
}

static void
unpack_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1(const void *src, GLfloat dst[][4],
                                      GLuint n)
{
   /* XXX to do */
}

static void
unpack_A_SNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = ((const GLbyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 0.0F;
      dst[i][GCOMP] = 0.0F;
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = BYTE_TO_FLOAT_TEX( s[i] );
   }
}

static void
unpack_L_SNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = ((const GLbyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = BYTE_TO_FLOAT_TEX( s[i] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_L8A8_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = ((const GLshort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] & 0xff) );
      dst[i][ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 8) );
   }
}

static void
unpack_I_SNORM8(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = ((const GLbyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = BYTE_TO_FLOAT_TEX( s[i] );
   }
}

static void
unpack_A_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = ((const GLshort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 0.0F;
      dst[i][GCOMP] = 0.0F;
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = SHORT_TO_FLOAT_TEX( s[i] );
   }
}

static void
unpack_L_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = ((const GLshort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = SHORT_TO_FLOAT_TEX( s[i] );
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_LA_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = SHORT_TO_FLOAT_TEX( s[i*2+0] );
      dst[i][ACOMP] = SHORT_TO_FLOAT_TEX( s[i*2+1] );
   }
}

static void
unpack_I_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = ((const GLshort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = SHORT_TO_FLOAT_TEX( s[i] );
   }
}

static void
unpack_R9G9B9E5_FLOAT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      rgb9e5_to_float3(s[i], dst[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_R11G11B10_FLOAT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      r11g11b10f_to_float3(s[i], dst[i]);
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_XRGB4444_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >>  8) & 0xf) * (1.0F / 15.0F);
      dst[i][GCOMP] = ((s[i] >>  4) & 0xf) * (1.0F / 15.0F);
      dst[i][BCOMP] = ((s[i]      ) & 0xf) * (1.0F / 15.0F);
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XRGB1555_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 10) & 0x1f) * (1.0F / 31.0F);
      dst[i][GCOMP] = ((s[i] >>  5) & 0x1f) * (1.0F / 31.0F);
      dst[i][BCOMP] = ((s[i] >>  0) & 0x1f) * (1.0F / 31.0F);
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR8888_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i]      ) );
      dst[i][GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >>  8) );
      dst[i][BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 16) );
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_R8G8B8X8_SRGB(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_nonlinear_to_linear( (s[i]      ) & 0xff );
      dst[i][GCOMP] = _mesa_nonlinear_to_linear( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = _mesa_nonlinear_to_linear( (s[i] >> 16) & 0xff );
      dst[i][ACOMP] = UBYTE_TO_FLOAT( s[i] >> 24 ); /* linear! */
   }
}

static void
unpack_XBGR8888_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*4+0];
      dst[i][GCOMP] = s[i*4+1];
      dst[i][BCOMP] = s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR8888_SINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLbyte *s = (const GLbyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*4+0];
      dst[i][GCOMP] = s[i*4+1];
      dst[i][BCOMP] = s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_B10G10R10X2_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 20) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][GCOMP] = ((s[i] >> 10) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][BCOMP] = ((s[i] >>  0) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBX_UNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = USHORT_TO_FLOAT( s[i*4+0] );
      dst[i][GCOMP] = USHORT_TO_FLOAT( s[i*4+1] );
      dst[i][BCOMP] = USHORT_TO_FLOAT( s[i*4+2] );
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBX_SNORM16(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+0] );
      dst[i][GCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+1] );
      dst[i][BCOMP] = SHORT_TO_FLOAT_TEX( s[i*4+2] );
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR16161616_FLOAT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_half_to_float(s[i*4+0]);
      dst[i][GCOMP] = _mesa_half_to_float(s[i*4+1]);
      dst[i][BCOMP] = _mesa_half_to_float(s[i*4+2]);
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR16161616_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = (const GLushort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR16161616_SINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLshort *s = (const GLshort *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_RGBX_FLOAT32(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLfloat *s = (const GLfloat *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*4+0];
      dst[i][GCOMP] = s[i*4+1];
      dst[i][BCOMP] = s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR32323232_UINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = (const GLuint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_XBGR32323232_SINT(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLint *s = (const GLint *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLfloat) s[i*4+0];
      dst[i][GCOMP] = (GLfloat) s[i*4+1];
      dst[i][BCOMP] = (GLfloat) s[i*4+2];
      dst[i][ACOMP] = 1.0;
   }
}

static void
unpack_R10G10B10A2_UNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = ((s[i] >> 0) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][GCOMP] = ((s[i] >> 10) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][BCOMP] = ((s[i] >> 20) & 0x3ff) * (1.0F / 1023.0F);
      dst[i][ACOMP] = ((s[i] >> 30) &  0x03) * (1.0F / 3.0F);
   }
}

static void
unpack_G8R8_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] >> 8) );
      dst[i][GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s[i] & 0xff) );
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_G16R16_SNORM(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = SHORT_TO_FLOAT_TEX( (GLshort) (s[i] >> 16) );
      dst[i][GCOMP] = SHORT_TO_FLOAT_TEX( (GLshort) (s[i] & 0xffff) );
      dst[i][BCOMP] = 0.0F;
      dst[i][ACOMP] = 1.0F;
   }
}

static void
unpack_B8G8R8X8_SRGB(const void *src, GLfloat dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = _mesa_nonlinear_to_linear( (s[i] >> 16) & 0xff );
      dst[i][GCOMP] = _mesa_nonlinear_to_linear( (s[i] >>  8) & 0xff );
      dst[i][BCOMP] = _mesa_nonlinear_to_linear( (s[i]      ) & 0xff );
      dst[i][ACOMP] = 1.0F;
   }
}

/**
 * Return the unpacker function for the given format.
 */
static unpack_rgba_func
get_unpack_rgba_function(mesa_format format)
{
   static unpack_rgba_func table[MESA_FORMAT_COUNT];
   static GLboolean initialized = GL_FALSE;

   if (!initialized) {
      table[MESA_FORMAT_NONE] = NULL;

      table[MESA_FORMAT_A8B8G8R8_UNORM] = unpack_A8B8G8R8_UNORM;
      table[MESA_FORMAT_R8G8B8A8_UNORM] = unpack_R8G8B8A8_UNORM;
      table[MESA_FORMAT_B8G8R8A8_UNORM] = unpack_B8G8R8A8_UNORM;
      table[MESA_FORMAT_A8R8G8B8_UNORM] = unpack_A8R8G8B8_UNORM;
      table[MESA_FORMAT_X8B8G8R8_UNORM] = unpack_RGBX8888;
      table[MESA_FORMAT_R8G8B8X8_UNORM] = unpack_RGBX8888_REV;
      table[MESA_FORMAT_B8G8R8X8_UNORM] = unpack_B8G8R8X8_UNORM;
      table[MESA_FORMAT_X8R8G8B8_UNORM] = unpack_X8R8G8B8_UNORM;
      table[MESA_FORMAT_BGR_UNORM8] = unpack_BGR_UNORM8;
      table[MESA_FORMAT_RGB_UNORM8] = unpack_RGB_UNORM8;
      table[MESA_FORMAT_B5G6R5_UNORM] = unpack_B5G6R5_UNORM;
      table[MESA_FORMAT_R5G6B5_UNORM] = unpack_R5G6B5_UNORM;
      table[MESA_FORMAT_B4G4R4A4_UNORM] = unpack_B4G4R4A4_UNORM;
      table[MESA_FORMAT_A4R4G4B4_UNORM] = unpack_A4R4G4B4_UNORM;
      table[MESA_FORMAT_A1B5G5R5_UNORM] = unpack_A1B5G5R5_UNORM;
      table[MESA_FORMAT_B5G5R5A1_UNORM] = unpack_B5G5R5A1_UNORM;
      table[MESA_FORMAT_A1R5G5B5_UNORM] = unpack_A1R5G5B5_UNORM;
      table[MESA_FORMAT_L4A4_UNORM] = unpack_L4A4_UNORM;
      table[MESA_FORMAT_L8A8_UNORM] = unpack_L8A8_UNORM;
      table[MESA_FORMAT_A8L8_UNORM] = unpack_A8L8_UNORM;
      table[MESA_FORMAT_L16A16_UNORM] = unpack_L16A16_UNORM;
      table[MESA_FORMAT_A16L16_UNORM] = unpack_A16L16_UNORM;
      table[MESA_FORMAT_B2G3R3_UNORM] = unpack_B2G3R3_UNORM;
      table[MESA_FORMAT_A_UNORM8] = unpack_A_UNORM8;
      table[MESA_FORMAT_A_UNORM16] = unpack_A_UNORM16;
      table[MESA_FORMAT_L_UNORM8] = unpack_L_UNORM8;
      table[MESA_FORMAT_L_UNORM16] = unpack_L_UNORM16;
      table[MESA_FORMAT_I_UNORM8] = unpack_I_UNORM8;
      table[MESA_FORMAT_I_UNORM16] = unpack_I_UNORM16;
      table[MESA_FORMAT_YCBCR] = unpack_YCBCR;
      table[MESA_FORMAT_YCBCR_REV] = unpack_YCBCR_REV;
      table[MESA_FORMAT_R_UNORM8] = unpack_R_UNORM8;
      table[MESA_FORMAT_R8G8_UNORM] = unpack_R8G8_UNORM;
      table[MESA_FORMAT_G8R8_UNORM] = unpack_G8R8_UNORM;
      table[MESA_FORMAT_R_UNORM16] = unpack_R_UNORM16;
      table[MESA_FORMAT_R16G16_UNORM] = unpack_R16G16_UNORM;
      table[MESA_FORMAT_G16R16_UNORM] = unpack_G16R16_UNORM;
      table[MESA_FORMAT_B10G10R10A2_UNORM] = unpack_B10G10R10A2_UNORM;
      table[MESA_FORMAT_B10G10R10A2_UINT] = unpack_B10G10R10A2_UINT;
      table[MESA_FORMAT_R10G10B10A2_UINT] = unpack_R10G10B10A2_UINT;
      table[MESA_FORMAT_S8_UINT_Z24_UNORM] = unpack_S8_UINT_Z24_UNORM;
      table[MESA_FORMAT_Z24_UNORM_S8_UINT] = unpack_Z24_UNORM_S8_UINT;
      table[MESA_FORMAT_Z_UNORM16] = unpack_Z_UNORM16;
      table[MESA_FORMAT_Z24_UNORM_X8_UINT] = unpack_Z24_UNORM_X8_UINT;
      table[MESA_FORMAT_X8_UINT_Z24_UNORM] = unpack_X8_UINT_Z24_UNORM;
      table[MESA_FORMAT_Z_UNORM32] = unpack_Z_UNORM32;
      table[MESA_FORMAT_S_UINT8] = unpack_S8;
      table[MESA_FORMAT_BGR_SRGB8] = unpack_BGR_SRGB8;
      table[MESA_FORMAT_A8B8G8R8_SRGB] = unpack_A8B8G8R8_SRGB;
      table[MESA_FORMAT_B8G8R8A8_SRGB] = unpack_B8G8R8A8_SRGB;
      table[MESA_FORMAT_R8G8B8A8_SRGB] = unpack_R8G8B8A8_SRGB;
      table[MESA_FORMAT_L_SRGB8] = unpack_L_SRGB8;
      table[MESA_FORMAT_L8A8_SRGB] = unpack_L8A8_SRGB;
      table[MESA_FORMAT_SRGB_DXT1] = unpack_SRGB_DXT1;
      table[MESA_FORMAT_SRGBA_DXT1] = unpack_SRGBA_DXT1;
      table[MESA_FORMAT_SRGBA_DXT3] = unpack_SRGBA_DXT3;
      table[MESA_FORMAT_SRGBA_DXT5] = unpack_SRGBA_DXT5;

      table[MESA_FORMAT_RGB_FXT1] = unpack_RGB_FXT1;
      table[MESA_FORMAT_RGBA_FXT1] = unpack_RGBA_FXT1;
      table[MESA_FORMAT_RGB_DXT1] = unpack_RGB_DXT1;
      table[MESA_FORMAT_RGBA_DXT1] = unpack_RGBA_DXT1;
      table[MESA_FORMAT_RGBA_DXT3] = unpack_RGBA_DXT3;
      table[MESA_FORMAT_RGBA_DXT5] = unpack_RGBA_DXT5;

      table[MESA_FORMAT_RGBA_FLOAT32] = unpack_RGBA_FLOAT32;
      table[MESA_FORMAT_RGBA_FLOAT16] = unpack_RGBA_FLOAT16;
      table[MESA_FORMAT_RGB_FLOAT32] = unpack_RGB_FLOAT32;
      table[MESA_FORMAT_RGB_FLOAT16] = unpack_RGB_FLOAT16;
      table[MESA_FORMAT_A_FLOAT32] = unpack_A_FLOAT32;
      table[MESA_FORMAT_A_FLOAT16] = unpack_A_FLOAT16;
      table[MESA_FORMAT_L_FLOAT32] = unpack_L_FLOAT32;
      table[MESA_FORMAT_L_FLOAT16] = unpack_L_FLOAT16;
      table[MESA_FORMAT_LA_FLOAT32] = unpack_LA_FLOAT32;
      table[MESA_FORMAT_LA_FLOAT16] = unpack_LA_FLOAT16;
      table[MESA_FORMAT_I_FLOAT32] = unpack_I_FLOAT32;
      table[MESA_FORMAT_I_FLOAT16] = unpack_I_FLOAT16;
      table[MESA_FORMAT_R_FLOAT32] = unpack_R_FLOAT32;
      table[MESA_FORMAT_R_FLOAT16] = unpack_R_FLOAT16;
      table[MESA_FORMAT_RG_FLOAT32] = unpack_RG_FLOAT32;
      table[MESA_FORMAT_RG_FLOAT16] = unpack_RG_FLOAT16;

      table[MESA_FORMAT_A_UINT8] = unpack_ALPHA_UINT8;
      table[MESA_FORMAT_A_UINT16] = unpack_ALPHA_UINT16;
      table[MESA_FORMAT_A_UINT32] = unpack_ALPHA_UINT32;
      table[MESA_FORMAT_A_SINT8] = unpack_ALPHA_INT8;
      table[MESA_FORMAT_A_SINT16] = unpack_ALPHA_INT16;
      table[MESA_FORMAT_A_SINT32] = unpack_ALPHA_INT32;

      table[MESA_FORMAT_I_UINT8] = unpack_INTENSITY_UINT8;
      table[MESA_FORMAT_I_UINT16] = unpack_INTENSITY_UINT16;
      table[MESA_FORMAT_I_UINT32] = unpack_INTENSITY_UINT32;
      table[MESA_FORMAT_I_SINT8] = unpack_INTENSITY_INT8;
      table[MESA_FORMAT_I_SINT16] = unpack_INTENSITY_INT16;
      table[MESA_FORMAT_I_SINT32] = unpack_INTENSITY_INT32;

      table[MESA_FORMAT_L_UINT8] = unpack_LUMINANCE_UINT8;
      table[MESA_FORMAT_L_UINT16] = unpack_LUMINANCE_UINT16;
      table[MESA_FORMAT_L_UINT32] = unpack_LUMINANCE_UINT32;
      table[MESA_FORMAT_L_SINT8] = unpack_LUMINANCE_INT8;
      table[MESA_FORMAT_L_SINT16] = unpack_LUMINANCE_INT16;
      table[MESA_FORMAT_L_SINT32] = unpack_LUMINANCE_INT32;

      table[MESA_FORMAT_LA_UINT8] = unpack_LUMINANCE_ALPHA_UINT8;
      table[MESA_FORMAT_LA_UINT16] = unpack_LUMINANCE_ALPHA_UINT16;
      table[MESA_FORMAT_LA_UINT32] = unpack_LUMINANCE_ALPHA_UINT32;
      table[MESA_FORMAT_LA_SINT8] = unpack_LUMINANCE_ALPHA_INT8;
      table[MESA_FORMAT_LA_SINT16] = unpack_LUMINANCE_ALPHA_INT16;
      table[MESA_FORMAT_LA_SINT32] = unpack_LUMINANCE_ALPHA_INT32;

      table[MESA_FORMAT_R_SINT8] = unpack_R_INT8;
      table[MESA_FORMAT_RG_SINT8] = unpack_RG_INT8;
      table[MESA_FORMAT_RGB_SINT8] = unpack_RGB_INT8;
      table[MESA_FORMAT_RGBA_SINT8] = unpack_RGBA_INT8;
      table[MESA_FORMAT_R_SINT16] = unpack_R_INT16;
      table[MESA_FORMAT_RG_SINT16] = unpack_RG_INT16;
      table[MESA_FORMAT_RGB_SINT16] = unpack_RGB_INT16;
      table[MESA_FORMAT_RGBA_SINT16] = unpack_RGBA_INT16;
      table[MESA_FORMAT_R_SINT32] = unpack_R_INT32;
      table[MESA_FORMAT_RG_SINT32] = unpack_RG_INT32;
      table[MESA_FORMAT_RGB_SINT32] = unpack_RGB_INT32;
      table[MESA_FORMAT_RGBA_SINT32] = unpack_RGBA_INT32;
      table[MESA_FORMAT_R_UINT8] = unpack_R_UINT8;
      table[MESA_FORMAT_RG_UINT8] = unpack_RG_UINT8;
      table[MESA_FORMAT_RGB_UINT8] = unpack_RGB_UINT8;
      table[MESA_FORMAT_RGBA_UINT8] = unpack_RGBA_UINT8;
      table[MESA_FORMAT_R_UINT16] = unpack_R_UINT16;
      table[MESA_FORMAT_RG_UINT16] = unpack_RG_UINT16;
      table[MESA_FORMAT_RGB_UINT16] = unpack_RGB_UINT16;
      table[MESA_FORMAT_RGBA_UINT16] = unpack_RGBA_UINT16;
      table[MESA_FORMAT_R_UINT32] = unpack_R_UINT32;
      table[MESA_FORMAT_RG_UINT32] = unpack_RG_UINT32;
      table[MESA_FORMAT_RGB_UINT32] = unpack_RGB_UINT32;
      table[MESA_FORMAT_RGBA_UINT32] = unpack_RGBA_UINT32;

      table[MESA_FORMAT_DUDV8] = unpack_DUDV8;
      table[MESA_FORMAT_R_SNORM8] = unpack_R_SNORM8;
      table[MESA_FORMAT_R8G8_SNORM] = unpack_R8G8_SNORM;
      table[MESA_FORMAT_X8B8G8R8_SNORM] = unpack_X8B8G8R8_SNORM;
      table[MESA_FORMAT_A8B8G8R8_SNORM] = unpack_A8B8G8R8_SNORM;
      table[MESA_FORMAT_R8G8B8A8_SNORM] = unpack_R8G8B8A8_SNORM;
      table[MESA_FORMAT_R_SNORM16] = unpack_R_SNORM16;
      table[MESA_FORMAT_R16G16_SNORM] = unpack_R16G16_SNORM;
      table[MESA_FORMAT_RGB_SNORM16] = unpack_RGB_SNORM16;
      table[MESA_FORMAT_RGBA_SNORM16] = unpack_RGBA_SNORM16;
      table[MESA_FORMAT_RGBA_UNORM16] = unpack_RGBA_16;

      table[MESA_FORMAT_R_RGTC1_UNORM] = unpack_RED_RGTC1;
      table[MESA_FORMAT_R_RGTC1_SNORM] = unpack_SIGNED_RED_RGTC1;
      table[MESA_FORMAT_RG_RGTC2_UNORM] = unpack_RG_RGTC2;
      table[MESA_FORMAT_RG_RGTC2_SNORM] = unpack_SIGNED_RG_RGTC2;

      table[MESA_FORMAT_L_LATC1_UNORM] = unpack_L_LATC1;
      table[MESA_FORMAT_L_LATC1_SNORM] = unpack_SIGNED_L_LATC1;
      table[MESA_FORMAT_LA_LATC2_UNORM] = unpack_LA_LATC2;
      table[MESA_FORMAT_LA_LATC2_SNORM] = unpack_SIGNED_LA_LATC2;

      table[MESA_FORMAT_ETC1_RGB8] = unpack_ETC1_RGB8;
      table[MESA_FORMAT_ETC2_RGB8] = unpack_ETC2_RGB8;
      table[MESA_FORMAT_ETC2_SRGB8] = unpack_ETC2_SRGB8;
      table[MESA_FORMAT_ETC2_RGBA8_EAC] = unpack_ETC2_RGBA8_EAC;
      table[MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = unpack_ETC2_SRGB8_ALPHA8_EAC;
      table[MESA_FORMAT_ETC2_R11_EAC] = unpack_ETC2_R11_EAC;
      table[MESA_FORMAT_ETC2_RG11_EAC] = unpack_ETC2_RG11_EAC;
      table[MESA_FORMAT_ETC2_SIGNED_R11_EAC] = unpack_ETC2_SIGNED_R11_EAC;
      table[MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = unpack_ETC2_SIGNED_RG11_EAC;
      table[MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] =
         unpack_ETC2_RGB8_PUNCHTHROUGH_ALPHA1;
      table[MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] =
         unpack_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1;
      table[MESA_FORMAT_A_SNORM8] = unpack_A_SNORM8;
      table[MESA_FORMAT_L_SNORM8] = unpack_L_SNORM8;
      table[MESA_FORMAT_L8A8_SNORM] = unpack_L8A8_SNORM;
      table[MESA_FORMAT_I_SNORM8] = unpack_I_SNORM8;
      table[MESA_FORMAT_A_SNORM16] = unpack_A_SNORM16;
      table[MESA_FORMAT_L_SNORM16] = unpack_L_SNORM16;
      table[MESA_FORMAT_LA_SNORM16] = unpack_LA_SNORM16;
      table[MESA_FORMAT_I_SNORM16] = unpack_I_SNORM16;

      table[MESA_FORMAT_R9G9B9E5_FLOAT] = unpack_R9G9B9E5_FLOAT;
      table[MESA_FORMAT_R11G11B10_FLOAT] = unpack_R11G11B10_FLOAT;

      table[MESA_FORMAT_Z_FLOAT32] = unpack_Z_FLOAT32;
      table[MESA_FORMAT_Z32_FLOAT_S8X24_UINT] = unpack_Z32_FLOAT_S8X24_UINT;

      table[MESA_FORMAT_B4G4R4X4_UNORM] = unpack_XRGB4444_UNORM;
      table[MESA_FORMAT_B5G5R5X1_UNORM] = unpack_XRGB1555_UNORM;
      table[MESA_FORMAT_R8G8B8X8_SNORM] = unpack_XBGR8888_SNORM;
      table[MESA_FORMAT_R8G8B8X8_SRGB] = unpack_R8G8B8X8_SRGB;
      table[MESA_FORMAT_RGBX_UINT8] = unpack_XBGR8888_UINT;
      table[MESA_FORMAT_RGBX_SINT8] = unpack_XBGR8888_SINT;
      table[MESA_FORMAT_B10G10R10X2_UNORM] = unpack_B10G10R10X2_UNORM;
      table[MESA_FORMAT_RGBX_UNORM16] = unpack_RGBX_UNORM16;
      table[MESA_FORMAT_RGBX_SNORM16] = unpack_RGBX_SNORM16;
      table[MESA_FORMAT_RGBX_FLOAT16] = unpack_XBGR16161616_FLOAT;
      table[MESA_FORMAT_RGBX_UINT16] = unpack_XBGR16161616_UINT;
      table[MESA_FORMAT_RGBX_SINT16] = unpack_XBGR16161616_SINT;
      table[MESA_FORMAT_RGBX_FLOAT32] = unpack_RGBX_FLOAT32;
      table[MESA_FORMAT_RGBX_UINT32] = unpack_XBGR32323232_UINT;
      table[MESA_FORMAT_RGBX_SINT32] = unpack_XBGR32323232_SINT;

      table[MESA_FORMAT_R10G10B10A2_UNORM] = unpack_R10G10B10A2_UNORM;

      table[MESA_FORMAT_G8R8_SNORM] = unpack_G8R8_SNORM;
      table[MESA_FORMAT_G16R16_SNORM] = unpack_G16R16_SNORM;

      table[MESA_FORMAT_B8G8R8X8_SRGB] = unpack_B8G8R8X8_SRGB;

      initialized = GL_TRUE;
   }

   if (table[format] == NULL) {
      _mesa_problem(NULL, "unsupported unpack for format %s",
                    _mesa_get_format_name(format));
   }

   return table[format];
}


/**
 * Unpack rgba colors, returning as GLfloat values.
 */
void
_mesa_unpack_rgba_row(mesa_format format, GLuint n,
                      const void *src, GLfloat dst[][4])
{
   unpack_rgba_func unpack = get_unpack_rgba_function(format);
   unpack(src, dst, n);
}


/**********************************************************************/
/*  Unpack, returning GLubyte colors                                  */
/**********************************************************************/


static void
unpack_ubyte_A8B8G8R8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 24);
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >>  8) & 0xff;
      dst[i][ACOMP] = (s[i]      ) & 0xff;
   }
}

static void
unpack_ubyte_R8G8B8A8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i]      ) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i] >> 16) & 0xff;
      dst[i][ACOMP] = (s[i] >> 24);
   }
}

static void
unpack_ubyte_B8G8R8A8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 16) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i]      ) & 0xff;
      dst[i][ACOMP] = (s[i] >> 24);
   }
}

static void
unpack_ubyte_A8R8G8B8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >>  8) & 0xff;
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >> 24);
      dst[i][ACOMP] = (s[i]      ) & 0xff;
   }
}

static void
unpack_ubyte_RGBX8888(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 24);
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >>  8) & 0xff;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_RGBX8888_REV(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i]      ) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i] >> 16) & 0xff;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_B8G8R8X8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >> 16) & 0xff;
      dst[i][GCOMP] = (s[i] >>  8) & 0xff;
      dst[i][BCOMP] = (s[i]      ) & 0xff;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_X8R8G8B8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (s[i] >>  8) & 0xff;
      dst[i][GCOMP] = (s[i] >> 16) & 0xff;
      dst[i][BCOMP] = (s[i] >> 24);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_BGR_UNORM8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*3+2];
      dst[i][GCOMP] = s[i*3+1];
      dst[i][BCOMP] = s[i*3+0];
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_RGB_UNORM8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = (const GLubyte *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i*3+0];
      dst[i][GCOMP] = s[i*3+1];
      dst[i][BCOMP] = s[i*3+2];
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_B5G6R5_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_5_8((s[i] >> 11) & 0x1f);
      dst[i][GCOMP] = EXPAND_6_8((s[i] >> 5 ) & 0x3f);
      dst[i][BCOMP] = EXPAND_5_8( s[i]        & 0x1f);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_R5G6B5_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   /* Warning: this function does not match the current Mesa definition
    * of MESA_FORMAT_R5G6B5_UNORM.
    */
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLuint t = (s[i] >> 8) | (s[i] << 8); /* byte swap */
      dst[i][RCOMP] = EXPAND_5_8((t >> 11) & 0x1f);
      dst[i][GCOMP] = EXPAND_6_8((t >> 5 ) & 0x3f);
      dst[i][BCOMP] = EXPAND_5_8( t        & 0x1f);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_B4G4R4A4_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_4_8((s[i] >>  8) & 0xf);
      dst[i][GCOMP] = EXPAND_4_8((s[i] >>  4) & 0xf);
      dst[i][BCOMP] = EXPAND_4_8((s[i]      ) & 0xf);
      dst[i][ACOMP] = EXPAND_4_8((s[i] >> 12) & 0xf);
   }
}

static void
unpack_ubyte_A4R4G4B4_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_4_8((s[i] >>  4) & 0xf);
      dst[i][GCOMP] = EXPAND_4_8((s[i] >>  8) & 0xf);
      dst[i][BCOMP] = EXPAND_4_8((s[i] >> 12) & 0xf);
      dst[i][ACOMP] = EXPAND_4_8((s[i]      ) & 0xf);
   }
}

static void
unpack_ubyte_A1B5G5R5_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_5_8((s[i] >> 11) & 0x1f);
      dst[i][GCOMP] = EXPAND_5_8((s[i] >>  6) & 0x1f);
      dst[i][BCOMP] = EXPAND_5_8((s[i] >>  1) & 0x1f);
      dst[i][ACOMP] = EXPAND_1_8((s[i]      ) & 0x01);
   }
}

static void
unpack_ubyte_B5G5R5A1_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_5_8((s[i] >> 10) & 0x1f);
      dst[i][GCOMP] = EXPAND_5_8((s[i] >>  5) & 0x1f);
      dst[i][BCOMP] = EXPAND_5_8((s[i] >>  0) & 0x1f);
      dst[i][ACOMP] = EXPAND_1_8((s[i] >> 15) & 0x01);
   }
}

static void
unpack_ubyte_A1R5G5B5_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   /* Warning: this function does not match the current Mesa definition
    * of MESA_FORMAT_A1R5G5B5_UNORM.
    */
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      GLushort tmp = (s[i] << 8) | (s[i] >> 8); /* byteswap */
      dst[i][RCOMP] = EXPAND_5_8((tmp >> 10) & 0x1f);
      dst[i][GCOMP] = EXPAND_5_8((tmp >>  5) & 0x1f);
      dst[i][BCOMP] = EXPAND_5_8((tmp >>  0) & 0x1f);
      dst[i][ACOMP] = EXPAND_1_8((tmp >> 15) & 0x01);
   }
}

static void
unpack_ubyte_L4A4_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = EXPAND_4_8(s[i] & 0xf);
      dst[i][ACOMP] = EXPAND_4_8(s[i] >> 4);
   }
}

static void
unpack_ubyte_L8A8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = EXPAND_4_8(s[i] & 0xff);
      dst[i][ACOMP] = EXPAND_4_8(s[i] >> 8);
   }
}

static void
unpack_ubyte_A8L8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = 
      dst[i][GCOMP] = 
      dst[i][BCOMP] = EXPAND_4_8(s[i] >> 8);
      dst[i][ACOMP] = EXPAND_4_8(s[i] & 0xff);
   }
}

static void
unpack_ubyte_B2G3R3_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = EXPAND_3_8((s[i] >> 5) & 0x7);
      dst[i][GCOMP] = EXPAND_3_8((s[i] >> 2) & 0x7);
      dst[i][BCOMP] = EXPAND_2_8((s[i]     ) & 0x3);
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_A_UNORM8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = 0;
      dst[i][ACOMP] = s[i];
   }
}

static void
unpack_ubyte_L_UNORM8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] = s[i];
      dst[i][ACOMP] = 0xff;
   }
}


static void
unpack_ubyte_I_UNORM8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] =
      dst[i][GCOMP] =
      dst[i][BCOMP] =
      dst[i][ACOMP] = s[i];
   }
}

static void
unpack_ubyte_R_UNORM8(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLubyte *s = ((const GLubyte *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][0] = s[i];
      dst[i][1] =
      dst[i][2] = 0;
      dst[i][3] = 0xff;
   }
}

static void
unpack_ubyte_R8G8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i] & 0xff;
      dst[i][GCOMP] = s[i] >> 8;
      dst[i][BCOMP] = 0;
      dst[i][ACOMP] = 0xff;
   }
}

static void
unpack_ubyte_G8R8_UNORM(const void *src, GLubyte dst[][4], GLuint n)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = s[i] >> 8;
      dst[i][GCOMP] = s[i] & 0xff;
      dst[i][BCOMP] = 0;
      dst[i][ACOMP] = 0xff;
   }
}


/**
 * Unpack rgba colors, returning as GLubyte values.  This should usually
 * only be used for unpacking formats that use 8 bits or less per channel.
 */
void
_mesa_unpack_ubyte_rgba_row(mesa_format format, GLuint n,
                            const void *src, GLubyte dst[][4])
{
   switch (format) {
   case MESA_FORMAT_A8B8G8R8_UNORM:
      unpack_ubyte_A8B8G8R8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_R8G8B8A8_UNORM:
      unpack_ubyte_R8G8B8A8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_B8G8R8A8_UNORM:
      unpack_ubyte_B8G8R8A8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_A8R8G8B8_UNORM:
      unpack_ubyte_A8R8G8B8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_X8B8G8R8_UNORM:
      unpack_ubyte_RGBX8888(src, dst, n);
      break;
   case MESA_FORMAT_R8G8B8X8_UNORM:
      unpack_ubyte_RGBX8888_REV(src, dst, n);
      break;
   case MESA_FORMAT_B8G8R8X8_UNORM:
      unpack_ubyte_B8G8R8X8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_X8R8G8B8_UNORM:
      unpack_ubyte_X8R8G8B8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_BGR_UNORM8:
      unpack_ubyte_BGR_UNORM8(src, dst, n);
      break;
   case MESA_FORMAT_RGB_UNORM8:
      unpack_ubyte_RGB_UNORM8(src, dst, n);
      break;
   case MESA_FORMAT_B5G6R5_UNORM:
      unpack_ubyte_B5G6R5_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_R5G6B5_UNORM:
      unpack_ubyte_R5G6B5_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_B4G4R4A4_UNORM:
      unpack_ubyte_B4G4R4A4_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_A4R4G4B4_UNORM:
      unpack_ubyte_A4R4G4B4_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_A1B5G5R5_UNORM:
      unpack_ubyte_A1B5G5R5_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_B5G5R5A1_UNORM:
      unpack_ubyte_B5G5R5A1_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_A1R5G5B5_UNORM:
      unpack_ubyte_A1R5G5B5_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_L4A4_UNORM:
      unpack_ubyte_L4A4_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_L8A8_UNORM:
      unpack_ubyte_L8A8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_A8L8_UNORM:
      unpack_ubyte_A8L8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_B2G3R3_UNORM:
      unpack_ubyte_B2G3R3_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_A_UNORM8:
      unpack_ubyte_A_UNORM8(src, dst, n);
      break;
   case MESA_FORMAT_L_UNORM8:
      unpack_ubyte_L_UNORM8(src, dst, n);
      break;
   case MESA_FORMAT_I_UNORM8:
      unpack_ubyte_I_UNORM8(src, dst, n);
      break;
   case MESA_FORMAT_R_UNORM8:
      unpack_ubyte_R_UNORM8(src, dst, n);
      break;
   case MESA_FORMAT_R8G8_UNORM:
      unpack_ubyte_R8G8_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_G8R8_UNORM:
      unpack_ubyte_G8R8_UNORM(src, dst, n);
      break;
   default:
      /* get float values, convert to ubyte */
      {
         GLfloat *tmp = malloc(n * 4 * sizeof(GLfloat));
         if (tmp) {
            GLuint i;
            _mesa_unpack_rgba_row(format, n, src, (GLfloat (*)[4]) tmp);
            for (i = 0; i < n; i++) {
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][0], tmp[i*4+0]);
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][1], tmp[i*4+1]);
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][2], tmp[i*4+2]);
               UNCLAMPED_FLOAT_TO_UBYTE(dst[i][3], tmp[i*4+3]);
            }
            free(tmp);
         }
      }
      break;
   }
}


/**********************************************************************/
/*  Unpack, returning GLuint colors                                   */
/**********************************************************************/

static void
unpack_int_rgba_RGBA_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   memcpy(dst, src, n * 4 * sizeof(GLuint));
}

static void
unpack_int_rgba_RGBA_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGBA_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGBA_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_RGBA_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_B8G8R8A8_UNORM(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLubyte) src[i * 4 + 2];
      dst[i][GCOMP] = (GLubyte) src[i * 4 + 1];
      dst[i][BCOMP] = (GLubyte) src[i * 4 + 0];
      dst[i][ACOMP] = (GLubyte) src[i * 4 + 3];
   }
}

static void
unpack_int_rgba_B8G8R8X8_UNORM(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][RCOMP] = (GLubyte) src[i * 4 + 2];
      dst[i][GCOMP] = (GLubyte) src[i * 4 + 1];
      dst[i][BCOMP] = (GLubyte) src[i * 4 + 0];
      dst[i][ACOMP] = (GLubyte) 0xff;
   }
}

static void
unpack_int_rgba_RGB_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RGB_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 3 + 0];
      dst[i][1] = src[i * 3 + 1];
      dst[i][2] = src[i * 3 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RG_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 2 + 0];
      dst[i][1] = src[i * 2 + 1];
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RG_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 2 + 0];
      dst[i][1] = src[i * 2 + 1];
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RG_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 2 + 0];
      dst[i][1] = src[i * 2 + 1];
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RG_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 2 + 0];
      dst[i][1] = src[i * 2 + 1];
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_RG_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 2 + 0];
      dst[i][1] = src[i * 2 + 1];
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_R_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i];
      dst[i][1] = 0;
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_R_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i];
      dst[i][1] = 0;
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_R_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i];
      dst[i][1] = 0;
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_R_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i];
      dst[i][1] = 0;
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_R_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i];
      dst[i][1] = 0;
      dst[i][2] = 0;
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_ALPHA_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_ALPHA_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = 0;
      dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_LUMINANCE_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_LUMINANCE_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i];
      dst[i][3] = 1;
   }
}


static void
unpack_int_rgba_LUMINANCE_ALPHA_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_LUMINANCE_ALPHA_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = src[i * 2 + 0];
      dst[i][3] = src[i * 2 + 1];
   }
}

static void
unpack_int_rgba_INTENSITY_UINT32(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_UINT16(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_INT16(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_UINT8(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_INTENSITY_INT8(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = dst[i][1] = dst[i][2] = dst[i][3] = src[i];
   }
}

static void
unpack_int_rgba_B10G10R10A2_UINT(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      GLuint tmp = src[i];
      dst[i][0] = (tmp >> 20) & 0x3ff;
      dst[i][1] = (tmp >> 10) & 0x3ff;
      dst[i][2] = (tmp >> 0) & 0x3ff;
      dst[i][3] = (tmp >> 30) & 0x3;
   }
}

static void
unpack_int_rgba_R10G10B10A2_UINT(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      GLuint tmp = src[i];
      dst[i][0] = (tmp >> 0) & 0x3ff;
      dst[i][1] = (tmp >> 10) & 0x3ff;
      dst[i][2] = (tmp >> 20) & 0x3ff;
      dst[i][3] = (tmp >> 30) & 0x3;
   }
}

static void
unpack_int_rgba_B10G10R10A2_UNORM(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      GLuint tmp = src[i];
      dst[i][0] = (tmp >> 20) & 0x3ff;
      dst[i][1] = (tmp >> 10) & 0x3ff;
      dst[i][2] = (tmp >> 0) & 0x3ff;
      dst[i][3] = (tmp >> 30) & 0x3;
   }
}

static void
unpack_int_rgba_XBGR8888_UINT(const GLubyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_XBGR8888_SINT(const GLbyte *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_XBGR16161616_UINT(const GLushort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_XBGR16161616_SINT(const GLshort *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_XBGR32323232_UINT(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      dst[i][0] = src[i * 4 + 0];
      dst[i][1] = src[i * 4 + 1];
      dst[i][2] = src[i * 4 + 2];
      dst[i][3] = 1;
   }
}

static void
unpack_int_rgba_R10G10B10A2_UNORM(const GLuint *src, GLuint dst[][4], GLuint n)
{
   unsigned int i;

   for (i = 0; i < n; i++) {
      GLuint tmp = src[i];
      dst[i][0] = (tmp >> 0) & 0x3ff;
      dst[i][1] = (tmp >> 10) & 0x3ff;
      dst[i][2] = (tmp >> 20) & 0x3ff;
      dst[i][3] = (tmp >> 30) & 0x3;
   }
}

void
_mesa_unpack_uint_rgba_row(mesa_format format, GLuint n,
                           const void *src, GLuint dst[][4])
{
   switch (format) {
      /* Since there won't be any sign extension happening, there's no need to
       * make separate paths for 32-bit-to-32-bit integer unpack.
       */
   case MESA_FORMAT_RGBA_UINT32:
   case MESA_FORMAT_RGBA_SINT32:
      unpack_int_rgba_RGBA_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_RGBA_UINT16:
      unpack_int_rgba_RGBA_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_RGBA_SINT16:
      unpack_int_rgba_RGBA_INT16(src, dst, n);
      break;

   case MESA_FORMAT_RGBA_UINT8:
      unpack_int_rgba_RGBA_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_RGBA_SINT8:
      unpack_int_rgba_RGBA_INT8(src, dst, n);
      break;

   case MESA_FORMAT_B8G8R8A8_UNORM:
      unpack_int_rgba_B8G8R8A8_UNORM(src, dst, n);
      break;

   case MESA_FORMAT_B8G8R8X8_UNORM:
      unpack_int_rgba_B8G8R8X8_UNORM(src, dst, n);
      break;

   case MESA_FORMAT_RGB_UINT32:
   case MESA_FORMAT_RGB_SINT32:
      unpack_int_rgba_RGB_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_RGB_UINT16:
      unpack_int_rgba_RGB_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_RGB_SINT16:
      unpack_int_rgba_RGB_INT16(src, dst, n);
      break;

   case MESA_FORMAT_RGB_UINT8:
      unpack_int_rgba_RGB_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_RGB_SINT8:
      unpack_int_rgba_RGB_INT8(src, dst, n);
      break;

   case MESA_FORMAT_RG_UINT32:
   case MESA_FORMAT_RG_SINT32:
      unpack_int_rgba_RG_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_RG_UINT16:
      unpack_int_rgba_RG_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_RG_SINT16:
      unpack_int_rgba_RG_INT16(src, dst, n);
      break;

   case MESA_FORMAT_RG_UINT8:
      unpack_int_rgba_RG_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_RG_SINT8:
      unpack_int_rgba_RG_INT8(src, dst, n);
      break;

   case MESA_FORMAT_R_UINT32:
   case MESA_FORMAT_R_SINT32:
      unpack_int_rgba_R_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_R_UINT16:
      unpack_int_rgba_R_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_R_SINT16:
      unpack_int_rgba_R_INT16(src, dst, n);
      break;

   case MESA_FORMAT_R_UINT8:
      unpack_int_rgba_R_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_R_SINT8:
      unpack_int_rgba_R_INT8(src, dst, n);
      break;

   case MESA_FORMAT_A_UINT32:
   case MESA_FORMAT_A_SINT32:
      unpack_int_rgba_ALPHA_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_A_UINT16:
      unpack_int_rgba_ALPHA_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_A_SINT16:
      unpack_int_rgba_ALPHA_INT16(src, dst, n);
      break;

   case MESA_FORMAT_A_UINT8:
      unpack_int_rgba_ALPHA_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_A_SINT8:
      unpack_int_rgba_ALPHA_INT8(src, dst, n);
      break;

   case MESA_FORMAT_L_UINT32:
   case MESA_FORMAT_L_SINT32:
      unpack_int_rgba_LUMINANCE_UINT32(src, dst, n);
      break;
   case MESA_FORMAT_L_UINT16:
      unpack_int_rgba_LUMINANCE_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_L_SINT16:
      unpack_int_rgba_LUMINANCE_INT16(src, dst, n);
      break;

   case MESA_FORMAT_L_UINT8:
      unpack_int_rgba_LUMINANCE_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_L_SINT8:
      unpack_int_rgba_LUMINANCE_INT8(src, dst, n);
      break;

   case MESA_FORMAT_LA_UINT32:
   case MESA_FORMAT_LA_SINT32:
      unpack_int_rgba_LUMINANCE_ALPHA_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_LA_UINT16:
      unpack_int_rgba_LUMINANCE_ALPHA_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_LA_SINT16:
      unpack_int_rgba_LUMINANCE_ALPHA_INT16(src, dst, n);
      break;

   case MESA_FORMAT_LA_UINT8:
      unpack_int_rgba_LUMINANCE_ALPHA_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_LA_SINT8:
      unpack_int_rgba_LUMINANCE_ALPHA_INT8(src, dst, n);
      break;

   case MESA_FORMAT_I_UINT32:
   case MESA_FORMAT_I_SINT32:
      unpack_int_rgba_INTENSITY_UINT32(src, dst, n);
      break;

   case MESA_FORMAT_I_UINT16:
      unpack_int_rgba_INTENSITY_UINT16(src, dst, n);
      break;
   case MESA_FORMAT_I_SINT16:
      unpack_int_rgba_INTENSITY_INT16(src, dst, n);
      break;

   case MESA_FORMAT_I_UINT8:
      unpack_int_rgba_INTENSITY_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_I_SINT8:
      unpack_int_rgba_INTENSITY_INT8(src, dst, n);
      break;

   case MESA_FORMAT_B10G10R10A2_UINT:
      unpack_int_rgba_B10G10R10A2_UINT(src, dst, n);
      break;

   case MESA_FORMAT_R10G10B10A2_UINT:
      unpack_int_rgba_R10G10B10A2_UINT(src, dst, n);
      break;

   case MESA_FORMAT_B10G10R10A2_UNORM:
      unpack_int_rgba_B10G10R10A2_UNORM(src, dst, n);
      break;

   case MESA_FORMAT_RGBX_UINT8:
      unpack_int_rgba_XBGR8888_UINT(src, dst, n);
      break;

   case MESA_FORMAT_RGBX_SINT8:
      unpack_int_rgba_XBGR8888_SINT(src, dst, n);
      break;

   case MESA_FORMAT_RGBX_UINT16:
      unpack_int_rgba_XBGR16161616_UINT(src, dst, n);
      break;

   case MESA_FORMAT_RGBX_SINT16:
      unpack_int_rgba_XBGR16161616_SINT(src, dst, n);
      break;

   case MESA_FORMAT_RGBX_UINT32:
   case MESA_FORMAT_RGBX_SINT32:
      unpack_int_rgba_XBGR32323232_UINT(src, dst, n);
      break;

   case MESA_FORMAT_R10G10B10A2_UNORM:
      unpack_int_rgba_R10G10B10A2_UNORM(src, dst, n);
      break;

   default:
      _mesa_problem(NULL, "%s: bad format %s", __FUNCTION__,
                    _mesa_get_format_name(format));
      return;
   }
}

/**
 * Unpack a 2D rect of pixels returning float RGBA colors.
 * \param format  the source image format
 * \param src  start address of the source image
 * \param srcRowStride  source image row stride in bytes
 * \param dst  start address of the dest image
 * \param dstRowStride  dest image row stride in bytes
 * \param x  source image start X pos
 * \param y  source image start Y pos
 * \param width  width of rect region to convert
 * \param height  height of rect region to convert
 */
void
_mesa_unpack_rgba_block(mesa_format format,
                        const void *src, GLint srcRowStride,
                        GLfloat dst[][4], GLint dstRowStride,
                        GLuint x, GLuint y, GLuint width, GLuint height)
{
   unpack_rgba_func unpack = get_unpack_rgba_function(format);
   const GLuint srcPixStride = _mesa_get_format_bytes(format);
   const GLuint dstPixStride = 4 * sizeof(GLfloat);
   const GLubyte *srcRow;
   GLubyte *dstRow;
   GLuint i;

   /* XXX needs to be fixed for compressed formats */

   srcRow = ((const GLubyte *) src) + srcRowStride * y + srcPixStride * x;
   dstRow = ((GLubyte *) dst) + dstRowStride * y + dstPixStride * x;

   for (i = 0; i < height; i++) {
      unpack(srcRow, (GLfloat (*)[4]) dstRow, width);

      dstRow += dstRowStride;
      srcRow += srcRowStride;
   }
}




typedef void (*unpack_float_z_func)(GLuint n, const void *src, GLfloat *dst);

static void
unpack_float_z_X8_UINT_Z24_UNORM(GLuint n, const void *src, GLfloat *dst)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (GLfloat) ((s[i] >> 8) * scale);
      ASSERT(dst[i] >= 0.0F);
      ASSERT(dst[i] <= 1.0F);
   }
}

static void
unpack_float_z_Z24_UNORM_X8_UINT(GLuint n, const void *src, GLfloat *dst)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (GLfloat) ((s[i] & 0x00ffffff) * scale);
      ASSERT(dst[i] >= 0.0F);
      ASSERT(dst[i] <= 1.0F);
   }
}

static void
unpack_float_Z_UNORM16(GLuint n, const void *src, GLfloat *dst)
{
   const GLushort *s = ((const GLushort *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = s[i] * (1.0F / 65535.0F);
   }
}

static void
unpack_float_Z_UNORM32(GLuint n, const void *src, GLfloat *dst)
{
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = s[i] * (1.0F / 0xffffffff);
   }
}

static void
unpack_float_Z_FLOAT32(GLuint n, const void *src, GLfloat *dst)
{
   memcpy(dst, src, n * sizeof(float));
}

static void
unpack_float_z_Z32X24S8(GLuint n, const void *src, GLfloat *dst)
{
   const struct z32f_x24s8 *s = (const struct z32f_x24s8 *) src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = s[i].z;
   }
}



/**
 * Unpack Z values.
 * The returned values will always be in the range [0.0, 1.0].
 */
void
_mesa_unpack_float_z_row(mesa_format format, GLuint n,
                         const void *src, GLfloat *dst)
{
   unpack_float_z_func unpack;

   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      unpack = unpack_float_z_X8_UINT_Z24_UNORM;
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      unpack = unpack_float_z_Z24_UNORM_X8_UINT;
      break;
   case MESA_FORMAT_Z_UNORM16:
      unpack = unpack_float_Z_UNORM16;
      break;
   case MESA_FORMAT_Z_UNORM32:
      unpack = unpack_float_Z_UNORM32;
      break;
   case MESA_FORMAT_Z_FLOAT32:
      unpack = unpack_float_Z_FLOAT32;
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      unpack = unpack_float_z_Z32X24S8;
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_unpack_float_z_row",
                    _mesa_get_format_name(format));
      return;
   }

   unpack(n, src, dst);
}



typedef void (*unpack_uint_z_func)(const void *src, GLuint *dst, GLuint n);

static void
unpack_uint_z_X8_UINT_Z24_UNORM(const void *src, GLuint *dst, GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] & 0xffffff00) | (s[i] >> 24);
   }
}

static void
unpack_uint_z_Z24_UNORM_X8_UINT(const void *src, GLuint *dst, GLuint n)
{
   /* only return Z, not stencil data */
   const GLuint *s = ((const GLuint *) src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] << 8) | ((s[i] >> 16) & 0xff);
   }
}

static void
unpack_uint_Z_UNORM16(const void *src, GLuint *dst, GLuint n)
{
   const GLushort *s = ((const GLushort *)src);
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = (s[i] << 16) | s[i];
   }
}

static void
unpack_uint_Z_UNORM32(const void *src, GLuint *dst, GLuint n)
{
   memcpy(dst, src, n * sizeof(GLuint));
}

static void
unpack_uint_Z_FLOAT32(const void *src, GLuint *dst, GLuint n)
{
   const float *s = (const float *)src;
   GLuint i;
   for (i = 0; i < n; i++) {
      dst[i] = FLOAT_TO_UINT(CLAMP(s[i], 0.0F, 1.0F));
   }
}

static void
unpack_uint_Z_FLOAT32_X24S8(const void *src, GLuint *dst, GLuint n)
{
   const struct z32f_x24s8 *s = (const struct z32f_x24s8 *) src;
   GLuint i;

   for (i = 0; i < n; i++) {
      dst[i] = FLOAT_TO_UINT(CLAMP(s[i].z, 0.0F, 1.0F));
   }
}


/**
 * Unpack Z values.
 * The returned values will always be in the range [0, 0xffffffff].
 */
void
_mesa_unpack_uint_z_row(mesa_format format, GLuint n,
                        const void *src, GLuint *dst)
{
   unpack_uint_z_func unpack;
   const GLubyte *srcPtr = (GLubyte *) src;

   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
   case MESA_FORMAT_X8_UINT_Z24_UNORM:
      unpack = unpack_uint_z_X8_UINT_Z24_UNORM;
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      unpack = unpack_uint_z_Z24_UNORM_X8_UINT;
      break;
   case MESA_FORMAT_Z_UNORM16:
      unpack = unpack_uint_Z_UNORM16;
      break;
   case MESA_FORMAT_Z_UNORM32:
      unpack = unpack_uint_Z_UNORM32;
      break;
   case MESA_FORMAT_Z_FLOAT32:
      unpack = unpack_uint_Z_FLOAT32;
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      unpack = unpack_uint_Z_FLOAT32_X24S8;
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_unpack_uint_z_row",
                    _mesa_get_format_name(format));
      return;
   }

   unpack(srcPtr, dst, n);
}


static void
unpack_ubyte_s_S_UINT8(const void *src, GLubyte *dst, GLuint n)
{
   memcpy(dst, src, n);
}

static void
unpack_ubyte_s_S8_UINT_Z24_UNORM(const void *src, GLubyte *dst, GLuint n)
{
   GLuint i;
   const GLuint *src32 = src;

   for (i = 0; i < n; i++)
      dst[i] = src32[i] & 0xff;
}

static void
unpack_ubyte_s_Z24_UNORM_S8_UINT(const void *src, GLubyte *dst, GLuint n)
{
   GLuint i;
   const GLuint *src32 = src;

   for (i = 0; i < n; i++)
      dst[i] = src32[i] >> 24;
}

static void
unpack_ubyte_s_Z32_FLOAT_S8X24_UINT(const void *src, GLubyte *dst, GLuint n)
{
   GLuint i;
   const struct z32f_x24s8 *s = (const struct z32f_x24s8 *) src;

   for (i = 0; i < n; i++)
      dst[i] = s[i].x24s8 & 0xff;
}

void
_mesa_unpack_ubyte_stencil_row(mesa_format format, GLuint n,
			       const void *src, GLubyte *dst)
{
   switch (format) {
   case MESA_FORMAT_S_UINT8:
      unpack_ubyte_s_S_UINT8(src, dst, n);
      break;
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      unpack_ubyte_s_S8_UINT_Z24_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      unpack_ubyte_s_Z24_UNORM_S8_UINT(src, dst, n);
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      unpack_ubyte_s_Z32_FLOAT_S8X24_UINT(src, dst, n);
      break;
   default:
      _mesa_problem(NULL, "bad format %s in _mesa_unpack_ubyte_s_row",
                    _mesa_get_format_name(format));
      return;
   }
}

static void
unpack_uint_24_8_depth_stencil_Z24_UNORM_S8_UINT(const GLuint *src, GLuint *dst, GLuint n)
{
   GLuint i;

   for (i = 0; i < n; i++) {
      GLuint val = src[i];
      dst[i] = val >> 24 | val << 8;
   }
}

static void
unpack_uint_24_8_depth_stencil_Z32_S8X24(const GLuint *src,
                                         GLuint *dst, GLuint n)
{
   GLuint i;

   for (i = 0; i < n; i++) {
      /* 8 bytes per pixel (float + uint32) */
      GLfloat zf = ((GLfloat *) src)[i * 2 + 0];
      GLuint z24 = (GLuint) (zf * (GLfloat) 0xffffff);
      GLuint s = src[i * 2 + 1] & 0xff;
      dst[i] = (z24 << 8) | s;
   }
}

static void
unpack_uint_24_8_depth_stencil_S8_UINT_Z24_UNORM(const GLuint *src, GLuint *dst, GLuint n)
{
   memcpy(dst, src, n * 4);
}

/**
 * Unpack depth/stencil returning as GL_UNSIGNED_INT_24_8.
 * \param format  the source data format
 */
void
_mesa_unpack_uint_24_8_depth_stencil_row(mesa_format format, GLuint n,
					 const void *src, GLuint *dst)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      unpack_uint_24_8_depth_stencil_S8_UINT_Z24_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      unpack_uint_24_8_depth_stencil_Z24_UNORM_S8_UINT(src, dst, n);
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      unpack_uint_24_8_depth_stencil_Z32_S8X24(src, dst, n);
      break;
   default:
      _mesa_problem(NULL,
                    "bad format %s in _mesa_unpack_uint_24_8_depth_stencil_row",
                    _mesa_get_format_name(format));
      return;
   }
}

static void
unpack_float_32_uint_24_8_Z24_UNORM_S8_UINT(const GLuint *src,
                                            GLuint *dst, GLuint n)
{
   GLuint i;
   struct z32f_x24s8 *d = (struct z32f_x24s8 *) dst;
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;

   for (i = 0; i < n; i++) {
      const GLuint z24 = src[i] & 0xffffff;
      d[i].z = z24 * scale;
      d[i].x24s8 = src[i] >> 24;
      assert(d[i].z >= 0.0f);
      assert(d[i].z <= 1.0f);
   }
}

static void
unpack_float_32_uint_24_8_Z32_FLOAT_S8X24_UINT(const GLuint *src,
                                               GLuint *dst, GLuint n)
{
   memcpy(dst, src, n * sizeof(struct z32f_x24s8));
}

static void
unpack_float_32_uint_24_8_S8_UINT_Z24_UNORM(const GLuint *src,
                                            GLuint *dst, GLuint n)
{
   GLuint i;
   struct z32f_x24s8 *d = (struct z32f_x24s8 *) dst;
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;

   for (i = 0; i < n; i++) {
      const GLuint z24 = src[i] >> 8;
      d[i].z = z24 * scale;
      d[i].x24s8 = src[i] & 0xff;
      assert(d[i].z >= 0.0f);
      assert(d[i].z <= 1.0f);
   }
}

/**
 * Unpack depth/stencil returning as GL_FLOAT_32_UNSIGNED_INT_24_8_REV.
 * \param format  the source data format
 *
 * In GL_FLOAT_32_UNSIGNED_INT_24_8_REV lower 4 bytes contain float
 * component and higher 4 bytes contain packed 24-bit and 8-bit
 * components.
 *
 *    31 30 29 28 ... 4 3 2 1 0    31 30 29 ... 9 8 7 6 5 ... 2 1 0
 *    +-------------------------+  +--------------------------------+
 *    |    Float Component      |  | Unused         | 8 bit stencil |
 *    +-------------------------+  +--------------------------------+
 *          lower 4 bytes                  higher 4 bytes
 */
void
_mesa_unpack_float_32_uint_24_8_depth_stencil_row(mesa_format format, GLuint n,
			                          const void *src, GLuint *dst)
{
   switch (format) {
   case MESA_FORMAT_S8_UINT_Z24_UNORM:
      unpack_float_32_uint_24_8_S8_UINT_Z24_UNORM(src, dst, n);
      break;
   case MESA_FORMAT_Z24_UNORM_S8_UINT:
      unpack_float_32_uint_24_8_Z24_UNORM_S8_UINT(src, dst, n);
      break;
   case MESA_FORMAT_Z32_FLOAT_S8X24_UINT:
      unpack_float_32_uint_24_8_Z32_FLOAT_S8X24_UINT(src, dst, n);
      break;
   default:
      _mesa_problem(NULL,
                    "bad format %s in _mesa_unpack_uint_24_8_depth_stencil_row",
                    _mesa_get_format_name(format));
      return;
   }
}

/**
 * Unpack depth/stencil
 * \param format  the source data format
 * \param type the destination data type
 */
void
_mesa_unpack_depth_stencil_row(mesa_format format, GLuint n,
	                       const void *src, GLenum type,
                               GLuint *dst)
{
   assert(type == GL_UNSIGNED_INT_24_8 ||
          type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV);

   switch (type) {
   case GL_UNSIGNED_INT_24_8:
      _mesa_unpack_uint_24_8_depth_stencil_row(format, n, src, dst);
      break;
   case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
      _mesa_unpack_float_32_uint_24_8_depth_stencil_row(format, n, src, dst);
      break;
   default:
      _mesa_problem(NULL,
                    "bad type 0x%x in _mesa_unpack_depth_stencil_row",
                    type);
      return;
   }
}
