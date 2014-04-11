/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (c) 2008-2009  VMware, Inc.
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
 * \file texfetch_tmp.h
 * Texel fetch functions template.
 *
 * This template file is used by texfetch.c to generate texel fetch functions
 * for 1-D, 2-D and 3-D texture images.
 *
 * It should be expanded by defining \p DIM as the number texture dimensions
 * (1, 2 or 3).  According to the value of \p DIM a series of macros is defined
 * for the texel lookup in the gl_texture_image::Data.
 *
 * \author Gareth Hughes
 * \author Brian Paul
 */


#if DIM == 1

#define TEXEL_ADDR( type, image, i, j, k, size ) \
	((void) (j), (void) (k), ((type *)(image)->ImageSlices[0] + (i) * (size)))

#define FETCH(x) fetch_texel_1d_##x

#elif DIM == 2

#define TEXEL_ADDR( type, image, i, j, k, size )			\
       ((void) (k),							\
        ((type *)((GLubyte *) (image)->ImageSlices[0] + (image)->RowStride * (j)) + \
          (i) * (size)))

#define FETCH(x) fetch_texel_2d_##x

#elif DIM == 3

#define TEXEL_ADDR( type, image, i, j, k, size )			\
        ((type *)((GLubyte *) (image)->ImageSlices[k] +                      \
                  (image)->RowStride * (j)) + (i) * (size))

#define FETCH(x) fetch_texel_3d_##x

#else
#error	illegal number of texture dimensions
#endif


static void
FETCH(Z_UNORM32)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[0] = src[0] * (1.0F / 0xffffffff);
}


static void
FETCH(Z_UNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[0] = src[0] * (1.0F / 65535.0F);
}


static void
FETCH(RGBA_FLOAT32)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 4);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = src[3];
}


static void
FETCH(RGBA_FLOAT16)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 4);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = _mesa_half_to_float(src[3]);
}


static void
FETCH(RGB_FLOAT32)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 3);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = 1.0F;
}


static void
FETCH(RGB_FLOAT16)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 3);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(A_FLOAT32)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = src[0];
}


static void
FETCH(A_FLOAT16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = _mesa_half_to_float(src[0]);
}


static void
FETCH(L_FLOAT32)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = 1.0F;
}


static void
FETCH(L_FLOAT16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = _mesa_half_to_float(src[0]);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(LA_FLOAT32)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[1];
}


static void
FETCH(LA_FLOAT16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = _mesa_half_to_float(src[0]);
   texel[ACOMP] = _mesa_half_to_float(src[1]);
}


static void
FETCH(I_FLOAT32)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = src[0];
}


static void
FETCH(I_FLOAT16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = _mesa_half_to_float(src[0]);
}


static void
FETCH(R_FLOAT32)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 1);
   texel[RCOMP] = src[0];
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(R_FLOAT16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 1);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(RG_FLOAT32)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(RG_FLOAT16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *src = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 2);
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(A8B8G8R8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
}


static void
FETCH(R8G8B8A8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
}


static void
FETCH(B8G8R8A8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
}


static void
FETCH(A8R8G8B8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
}


static void
FETCH(X8B8G8R8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[ACOMP] = 1.0f;
}


static void
FETCH(R8G8B8X8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[ACOMP] = 1.0f;
}


static void
FETCH(B8G8R8X8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff );
   texel[ACOMP] = 1.0f;
}


static void
FETCH(X8R8G8B8_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( (s >>  8) & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( (s >> 16) & 0xff );
   texel[BCOMP] = UBYTE_TO_FLOAT( (s >> 24)        );
   texel[ACOMP] = 1.0f;
}


static void
FETCH(BGR_UNORM8)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(RGB_UNORM8)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(B5G6R5_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 11) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >> 5 ) & 0x3f) * (1.0F / 63.0F);
   texel[BCOMP] = ((s      ) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(R5G6B5_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = (*src >> 8) | (*src << 8); /* byte swap */
   texel[RCOMP] = UBYTE_TO_FLOAT( ((s >> 8) & 0xf8) | ((s >> 13) & 0x7) );
   texel[GCOMP] = UBYTE_TO_FLOAT( ((s >> 3) & 0xfc) | ((s >>  9) & 0x3) );
   texel[BCOMP] = UBYTE_TO_FLOAT( ((s << 3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(B4G4R4A4_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >>  8) & 0xf) * (1.0F / 15.0F);
   texel[GCOMP] = ((s >>  4) & 0xf) * (1.0F / 15.0F);
   texel[BCOMP] = ((s      ) & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s >> 12) & 0xf) * (1.0F / 15.0F);
}


static void
FETCH(A4R4G4B4_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = ((s >>  4) & 0xf) * (1.0F / 15.0F);
   texel[GCOMP] = ((s >>  8) & 0xf) * (1.0F / 15.0F);
   texel[BCOMP] = ((s >> 12) & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s      ) & 0xf) * (1.0F / 15.0F);
}


static void
FETCH(A1B5G5R5_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 11) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >>  6) & 0x1f) * (1.0F / 31.0F);
   texel[BCOMP] = ((s >>  1) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = ((s      ) & 0x01) * 1.0F;
}


static void
FETCH(B5G5R5A1_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 10) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >>  5) & 0x1f) * (1.0F / 31.0F);
   texel[BCOMP] = ((s >>  0) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = ((s >> 15) & 0x01) * 1.0F;
}


static void
FETCH(A1R5G5B5_UNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   const GLushort s = (*src << 8) | (*src >> 8); /* byteswap */
   texel[RCOMP] = UBYTE_TO_FLOAT( ((s >>  7) & 0xf8) | ((s >> 12) & 0x7) );
   texel[GCOMP] = UBYTE_TO_FLOAT( ((s >>  2) & 0xf8) | ((s >>  7) & 0x7) );
   texel[BCOMP] = UBYTE_TO_FLOAT( ((s <<  3) & 0xf8) | ((s >>  2) & 0x7) );
   texel[ACOMP] = UBYTE_TO_FLOAT( ((s >> 15) & 0x01) * 255 );
}


static void
FETCH(B10G10R10A2_UNORM)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLuint s = *src;
   texel[RCOMP] = ((s >> 20) & 0x3ff) * (1.0F / 1023.0F);
   texel[GCOMP] = ((s >> 10) & 0x3ff) * (1.0F / 1023.0F);
   texel[BCOMP] = ((s >>  0) & 0x3ff) * (1.0F / 1023.0F);
   texel[ACOMP] = ((s >> 30) & 0x03) * (1.0F / 3.0F);
}


static void
FETCH(R10G10B10A2_UNORM)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLuint s = *src;
   texel[RCOMP] = ((s >>  0) & 0x3ff) * (1.0F / 1023.0F);
   texel[GCOMP] = ((s >> 10) & 0x3ff) * (1.0F / 1023.0F);
   texel[BCOMP] = ((s >> 20) & 0x3ff) * (1.0F / 1023.0F);
   texel[ACOMP] = ((s >> 30) & 0x03) * (1.0F / 3.0F);
}


static void
FETCH(R8G8_UNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( s & 0xff );
   texel[GCOMP] = UBYTE_TO_FLOAT( s >> 8 );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}


static void
FETCH(G8R8_UNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT( s >> 8 );
   texel[GCOMP] = UBYTE_TO_FLOAT( s & 0xff );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}


static void
FETCH(L4A4_UNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte s = *TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = (s & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s >> 4) & 0xf) * (1.0F / 15.0F);
}


static void
FETCH(L8A8_UNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_FLOAT( s & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( s >> 8 );
}


static void
FETCH(R_UNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte s = *TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] = UBYTE_TO_FLOAT(s);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}


static void
FETCH(R_UNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] = USHORT_TO_FLOAT(s);
   texel[GCOMP] = 0.0;
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}


static void
FETCH(A8L8_UNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_FLOAT( s >> 8 );
   texel[ACOMP] = UBYTE_TO_FLOAT( s & 0xff );
}


static void
FETCH(R16G16_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = USHORT_TO_FLOAT( s & 0xffff );
   texel[GCOMP] = USHORT_TO_FLOAT( s >> 16 );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}


static void
FETCH(G16R16_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = USHORT_TO_FLOAT( s >> 16 );
   texel[GCOMP] = USHORT_TO_FLOAT( s & 0xffff );
   texel[BCOMP] = 0.0;
   texel[ACOMP] = 1.0;
}


static void
FETCH(L16A16_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = USHORT_TO_FLOAT( s & 0xffff );
   texel[ACOMP] = USHORT_TO_FLOAT( s >> 16 );
}


static void
FETCH(A16L16_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = USHORT_TO_FLOAT( s >> 16 );
   texel[ACOMP] = USHORT_TO_FLOAT( s & 0xffff );
}


static void
FETCH(B2G3R3_UNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   const GLubyte s = *src;
   texel[RCOMP] = ((s >> 5) & 0x7) * (1.0F / 7.0F);
   texel[GCOMP] = ((s >> 2) & 0x7) * (1.0F / 7.0F);
   texel[BCOMP] = ((s     ) & 0x3) * (1.0F / 3.0F);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(A_UNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}


static void
FETCH(A_UNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = USHORT_TO_FLOAT( src[0] );
}


static void
FETCH(L_UNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(L_UNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = USHORT_TO_FLOAT( src[0] );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(I_UNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}


static void
FETCH(I_UNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = USHORT_TO_FLOAT( src[0] );
}


static void
FETCH(BGR_SRGB8)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 3);
   texel[RCOMP] = nonlinear_to_linear(src[2]);
   texel[GCOMP] = nonlinear_to_linear(src[1]);
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(A8B8G8R8_SRGB)(const struct swrast_texture_image *texImage,
                     GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = nonlinear_to_linear( (s >> 24) );
   texel[GCOMP] = nonlinear_to_linear( (s >> 16) & 0xff );
   texel[BCOMP] = nonlinear_to_linear( (s >>  8) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s      ) & 0xff ); /* linear! */
}


static void
FETCH(B8G8R8A8_SRGB)(const struct swrast_texture_image *texImage,
                     GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = nonlinear_to_linear( (s >> 16) & 0xff );
   texel[GCOMP] = nonlinear_to_linear( (s >>  8) & 0xff );
   texel[BCOMP] = nonlinear_to_linear( (s      ) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24) ); /* linear! */
}


static void
FETCH(R8G8B8A8_SRGB)(const struct swrast_texture_image *texImage,
                     GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = nonlinear_to_linear( (s      ) & 0xff );
   texel[GCOMP] = nonlinear_to_linear( (s >>  8) & 0xff );
   texel[BCOMP] = nonlinear_to_linear( (s >> 16) & 0xff );
   texel[ACOMP] = UBYTE_TO_FLOAT( (s >> 24) ); /* linear! */
}


static void
FETCH(R8G8B8X8_SRGB)(const struct swrast_texture_image *texImage,
                     GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = nonlinear_to_linear( (s      ) & 0xff );
   texel[GCOMP] = nonlinear_to_linear( (s >>  8) & 0xff );
   texel[BCOMP] = nonlinear_to_linear( (s >> 16) & 0xff );
   texel[ACOMP] = 1.0f;
}


static void
FETCH(L_SRGB8)(const struct swrast_texture_image *texImage,
               GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(L8A8_SRGB)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = nonlinear_to_linear(src[0]);
   texel[ACOMP] = UBYTE_TO_FLOAT(src[1]); /* linear */
}


static void
FETCH(RGBA_SINT8)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLbyte *src = TEXEL_ADDR(GLbyte, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}


static void
FETCH(RGBA_SINT16)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *src = TEXEL_ADDR(GLshort, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}


static void
FETCH(RGBA_SINT32)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLint *src = TEXEL_ADDR(GLint, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}


static void
FETCH(RGBA_UINT8)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLubyte *src = TEXEL_ADDR(GLubyte, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}


static void
FETCH(RGBA_UINT16)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src = TEXEL_ADDR(GLushort, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}


static void
FETCH(RGBA_UINT32)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 4);
   texel[RCOMP] = (GLfloat) src[0];
   texel[GCOMP] = (GLfloat) src[1];
   texel[BCOMP] = (GLfloat) src[2];
   texel[ACOMP] = (GLfloat) src[3];
}


/**
 * This format by definition produces 0,0,0,1 as rgba values,
 * however we'll return the dudv values as rg and fix up elsewhere.
 */
static void
FETCH(DUDV8)(const struct swrast_texture_image *texImage,
             GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLbyte *src = TEXEL_ADDR(GLbyte, texImage, i, j, k, 2);
   texel[RCOMP] = BYTE_TO_FLOAT(src[0]);
   texel[GCOMP] = BYTE_TO_FLOAT(src[1]);
   texel[BCOMP] = 0;
   texel[ACOMP] = 0;
}


static void
FETCH(R_SNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( s );
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(A_SNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] = 0.0F;
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( s );
}


static void
FETCH(L_SNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( s );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(I_SNORM8)(const struct swrast_texture_image *texImage,
                GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLbyte s = *TEXEL_ADDR(GLbyte, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( s );
}


static void
FETCH(R8G8_SNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s & 0xff) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 8) );
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(L8A8_SNORM)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s & 0xff) );
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 8) );
}


static void
FETCH(X8B8G8R8_SNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 24) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 16) );
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >>  8) );
   texel[ACOMP] = 1.0f;
}


static void
FETCH(A8B8G8R8_SNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 24) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 16) );
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >>  8) );
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s      ) );
}


static void
FETCH(R8G8B8A8_SNORM)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint s = *TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   texel[RCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s      ) );
   texel[GCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >>  8) );
   texel[BCOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 16) );
   texel[ACOMP] = BYTE_TO_FLOAT_TEX( (GLbyte) (s >> 24) );
}


static void
FETCH(R_SNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s );
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(A_SNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] = 0.0F;
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s );
}


static void
FETCH(L_SNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(I_SNORM16)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort s = *TEXEL_ADDR(GLshort, texImage, i, j, k, 1);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] =
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s );
}


static void
FETCH(R16G16_SNORM)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 2);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[GCOMP] = SHORT_TO_FLOAT_TEX( s[1] );
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}


static void
FETCH(LA_SNORM16)(const struct swrast_texture_image *texImage,
                  GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 2);
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s[1] );
}


static void

FETCH(RGB_SNORM16)(const struct swrast_texture_image *texImage,
                   GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 3);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[GCOMP] = SHORT_TO_FLOAT_TEX( s[1] );
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s[2] );
   texel[ACOMP] = 1.0F;
}


static void
FETCH(RGBA_SNORM16)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLshort *s = TEXEL_ADDR(GLshort, texImage, i, j, k, 4);
   texel[RCOMP] = SHORT_TO_FLOAT_TEX( s[0] );
   texel[GCOMP] = SHORT_TO_FLOAT_TEX( s[1] );
   texel[BCOMP] = SHORT_TO_FLOAT_TEX( s[2] );
   texel[ACOMP] = SHORT_TO_FLOAT_TEX( s[3] );
}


static void
FETCH(RGBA_UNORM16)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *s = TEXEL_ADDR(GLushort, texImage, i, j, k, 4);
   texel[RCOMP] = USHORT_TO_FLOAT( s[0] );
   texel[GCOMP] = USHORT_TO_FLOAT( s[1] );
   texel[BCOMP] = USHORT_TO_FLOAT( s[2] );
   texel[ACOMP] = USHORT_TO_FLOAT( s[3] );
}


static void
FETCH(RGBX_UNORM16)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *s = TEXEL_ADDR(GLushort, texImage, i, j, k, 4);
   texel[RCOMP] = USHORT_TO_FLOAT(s[0]);
   texel[GCOMP] = USHORT_TO_FLOAT(s[1]);
   texel[BCOMP] = USHORT_TO_FLOAT(s[2]);
   texel[ACOMP] = 1.0f;
}


static void
FETCH(RGBX_FLOAT16)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLhalfARB *s = TEXEL_ADDR(GLhalfARB, texImage, i, j, k, 4);
   texel[RCOMP] = _mesa_half_to_float(s[0]);
   texel[GCOMP] = _mesa_half_to_float(s[1]);
   texel[BCOMP] = _mesa_half_to_float(s[2]);
   texel[ACOMP] = 1.0f;
}


static void
FETCH(RGBX_FLOAT32)(const struct swrast_texture_image *texImage,
                    GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *s = TEXEL_ADDR(GLfloat, texImage, i, j, k, 4);
   texel[RCOMP] = s[0];
   texel[GCOMP] = s[1];
   texel[BCOMP] = s[2];
   texel[ACOMP] = 1.0f;
}


/**
 * Fetch texel from 1D, 2D or 3D ycbcr texture, returning RGBA.
 */
static void
FETCH(YCBCR)(const struct swrast_texture_image *texImage,
             GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src0 = TEXEL_ADDR(GLushort, texImage, (i & ~1), j, k, 1); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
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
   texel[RCOMP] = CLAMP(r, 0.0F, 1.0F);
   texel[GCOMP] = CLAMP(g, 0.0F, 1.0F);
   texel[BCOMP] = CLAMP(b, 0.0F, 1.0F);
   texel[ACOMP] = 1.0F;
}


/**
 * Fetch texel from 1D, 2D or 3D ycbcr texture, returning RGBA.
 */
static void
FETCH(YCBCR_REV)(const struct swrast_texture_image *texImage,
                 GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLushort *src0 = TEXEL_ADDR(GLushort, texImage, (i & ~1), j, k, 1); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
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
   texel[RCOMP] = CLAMP(r, 0.0F, 1.0F);
   texel[GCOMP] = CLAMP(g, 0.0F, 1.0F);
   texel[BCOMP] = CLAMP(b, 0.0F, 1.0F);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(S8_UINT_Z24_UNORM)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* only return Z, not stencil data */
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   texel[0] = (GLfloat) (((*src) >> 8) * scale);
   ASSERT(texImage->Base.TexFormat == MESA_FORMAT_S8_UINT_Z24_UNORM ||
	  texImage->Base.TexFormat == MESA_FORMAT_X8_UINT_Z24_UNORM);
   ASSERT(texel[0] >= 0.0F);
   ASSERT(texel[0] <= 1.0F);
}


static void
FETCH(Z24_UNORM_S8_UINT)(const struct swrast_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel)
{
   /* only return Z, not stencil data */
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   const GLdouble scale = 1.0 / (GLdouble) 0xffffff;
   texel[0] = (GLfloat) (((*src) & 0x00ffffff) * scale);
   ASSERT(texImage->Base.TexFormat == MESA_FORMAT_Z24_UNORM_S8_UINT ||
	  texImage->Base.TexFormat == MESA_FORMAT_Z24_UNORM_X8_UINT);
   ASSERT(texel[0] >= 0.0F);
   ASSERT(texel[0] <= 1.0F);
}


static void
FETCH(R9G9B9E5_FLOAT)(const struct swrast_texture_image *texImage,
                      GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   rgb9e5_to_float3(*src, texel);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(R11G11B10_FLOAT)(const struct swrast_texture_image *texImage,
                       GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLuint *src = TEXEL_ADDR(GLuint, texImage, i, j, k, 1);
   r11g11b10f_to_float3(*src, texel);
   texel[ACOMP] = 1.0F;
}


static void
FETCH(Z32_FLOAT_S8X24_UINT)(const struct swrast_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel)
{
   const GLfloat *src = TEXEL_ADDR(GLfloat, texImage, i, j, k, 2);
   texel[RCOMP] = src[0];
   texel[GCOMP] = 0.0F;
   texel[BCOMP] = 0.0F;
   texel[ACOMP] = 1.0F;
}



#undef TEXEL_ADDR
#undef DIM
#undef FETCH
