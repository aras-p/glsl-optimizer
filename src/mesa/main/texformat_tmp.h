/* $Id: texformat_tmp.h,v 1.3 2001/03/18 13:37:18 gareth Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * Author:
 *    Gareth Hughes <gareth@valinux.com>
 */

#if DIM == 1

#define CHAN_SRC( t, i, j, k, sz )					\
	((GLchan *)(t)->Data + (i) * (sz))
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + (i) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + (i))
#define FLOAT_SRC( t, i, j, k )						\
	((GLfloat *)(t)->Data + (i))

#define FETCH(x) fetch_1d_texel_##x

#elif DIM == 2

#define CHAN_SRC( t, i, j, k, sz )					\
	((GLchan *)(t)->Data + ((t)->Width * (j) + (i)) * (sz))
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + ((t)->Width * (j) + (i)) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + ((t)->Width * (j) + (i)))
#define FLOAT_SRC( t, i, j, k )						\
	((GLfloat *)(t)->Data + ((t)->Width * (j) + (i)))

#define FETCH(x) fetch_2d_texel_##x

#elif DIM == 3

#define CHAN_SRC( t, i, j, k, sz )					\
	(GLchan *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				(t)->Width + (i)) * (sz)
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				 (t)->Width + (i)) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->Width + (i)))
#define FLOAT_SRC( t, i, j, k )						\
	((GLfloat *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->Width + (i)))

#define FETCH(x) fetch_3d_texel_##x

#else
#error	illegal number of texture dimensions
#endif


static void FETCH(rgba)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   COPY_CHAN4( rgba, src );
}

static void FETCH(rgb)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 3 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[1];
   rgba[BCOMP] = src[2];
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(alpha)( const struct gl_texture_image *texImage,
			  GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = 0;
   rgba[GCOMP] = 0;
   rgba[BCOMP] = 0;
   rgba[ACOMP] = src[0];
}

static void FETCH(luminance)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[0];
   rgba[BCOMP] = src[0];
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(luminance_alpha)( const struct gl_texture_image *texImage,
				    GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 2 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[0];
   rgba[BCOMP] = src[0];
   rgba[ACOMP] = src[1];
}

static void FETCH(intensity)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = src[0];
   rgba[GCOMP] = src[0];
   rgba[BCOMP] = src[0];
   rgba[ACOMP] = src[0];
}

static void FETCH(color_index)( const struct gl_texture_image *texImage,
				GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   GLchan *index = (GLchan *) texel;
   *index = *src;
}

static void FETCH(depth_component)( const struct gl_texture_image *texImage,
				    GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLfloat *src = FLOAT_SRC( texImage, i, j, k );
   GLfloat *depth = (GLfloat *) texel;
   *depth = *src;
}

static void FETCH(rgba8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[3] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(argb8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[3] );
}

static void FETCH(rgb888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 3 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[2] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[1] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(rgb565)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   GLchan *rgba = (GLchan *) texel; GLushort s = *src;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) * 255 / 0xf8 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) * 255 / 0xfc );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) * 255 / 0xf8 );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(argb4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   GLchan *rgba = (GLchan *) texel; GLushort s = *src;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) * 255 / 0xf );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) * 255 / 0xf );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) * 255 / 0xf );
   rgba[ACOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) * 255 / 0xf );
}

static void FETCH(argb1555)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   GLchan *rgba = (GLchan *) texel; GLushort s = *src;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s >> 10) & 0xf8) * 255 / 0xf8 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s >>  5) & 0xf8) * 255 / 0xf8 );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf8) * 255 / 0xf8 );
   rgba[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

static void FETCH(al88)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 2 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[1] );
}

static void FETCH(rgb332)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel; GLubyte s = *src;
   rgba[RCOMP] = UBYTE_TO_CHAN( ((s     ) & 0xe0) * 255 / 0xe0 );
   rgba[GCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xe0) * 255 / 0xe0 );
   rgba[BCOMP] = UBYTE_TO_CHAN( ((s << 5) & 0xc0) * 255 / 0xc0 );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(a8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = 0;
   rgba[GCOMP] = 0;
   rgba[BCOMP] = 0;
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(l8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = CHAN_MAX;
}

static void FETCH(i8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[GCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[BCOMP] = UBYTE_TO_CHAN( src[0] );
   rgba[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(ci8)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLvoid *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *index = (GLchan *) texel;
   *index = UBYTE_TO_CHAN( *src );
}


#undef CHAN_SRC
#undef UBYTE_SRC
#undef USHORT_SRC
#undef FLOAT_SRC
#undef FETCH
#undef DIM
