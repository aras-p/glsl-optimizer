/**
 * \file texformat_tmp.h
 * Texel fetch functions template.
 * 
 * This template file is used by texformat.c to generate texel fetch functions
 * for 1-D, 2-D and 3-D texture images. 
 *
 * It should be expanded by definining \p DIM as the number texture dimensions
 * (1, 2 or 3).  According to the value of \p DIM a serie of macros is defined
 * for the texel lookup in the gl_texture_image::Data.
 * 
 * \sa texformat.c and FetchTexel.
 * 
 * \author Gareth Hughes
 * \author Brian Paul
 */

/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


#if DIM == 1

#define CHAN_SRC( t, i, j, k, sz )					\
	((GLchan *)(t)->Data + (i) * (sz))
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + (i) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + (i))
#define FLOAT_SRC( t, i, j, k, sz )					\
	((GLfloat *)(t)->Data + (i) * (sz))
#define HALF_SRC( t, i, j, k, sz )					\
	((GLhalfNV *)(t)->Data + (i) * (sz))

#define FETCH(x) fetch_texel_1d_##x

#elif DIM == 2

#define CHAN_SRC( t, i, j, k, sz )					\
	((GLchan *)(t)->Data + ((t)->RowStride * (j) + (i)) * (sz))
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + ((t)->RowStride * (j) + (i)) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + ((t)->RowStride * (j) + (i)))
#define FLOAT_SRC( t, i, j, k, sz )					\
	((GLfloat *)(t)->Data + ((t)->RowStride * (j) + (i)) * (sz))
#define HALF_SRC( t, i, j, k, sz )					\
	((GLhalfNV *)(t)->Data + ((t)->RowStride * (j) + (i)) * (sz))

#define FETCH(x) fetch_texel_2d_##x

#elif DIM == 3

#define CHAN_SRC( t, i, j, k, sz )					\
	(GLchan *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				(t)->RowStride + (i)) * (sz)
#define UBYTE_SRC( t, i, j, k, sz )					\
	((GLubyte *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				 (t)->RowStride + (i)) * (sz))
#define USHORT_SRC( t, i, j, k )					\
	((GLushort *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->RowStride + (i)))
#define FLOAT_SRC( t, i, j, k, sz )					\
	((GLfloat *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->RowStride + (i)) * (sz))
#define HALF_SRC( t, i, j, k, sz )					\
	((GLhalfNV *)(t)->Data + (((t)->Height * (k) + (j)) *		\
				  (t)->RowStride + (i)) * (sz))

#define FETCH(x) fetch_texel_3d_##x

#else
#error	illegal number of texture dimensions
#endif


/* Fetch color texel from 1D, 2D or 3D RGBA texture, returning 4 GLchans */
static void FETCH(rgba)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 4 );
   COPY_CHAN4( texel, src );
}

/* Fetch color texel from 1D, 2D or 3D RGBA texture, returning 4 GLfloats */
static void FETCH(f_rgba)( const struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = CHAN_TO_FLOAT(src[0]);
   texel[GCOMP] = CHAN_TO_FLOAT(src[1]);
   texel[BCOMP] = CHAN_TO_FLOAT(src[2]);
   texel[ACOMP] = CHAN_TO_FLOAT(src[3]);
}


/* Fetch color texel from 1D, 2D or 3D RGB texture, returning 4 GLchans */
static void FETCH(rgb)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D RGB texture, returning 4 GLfloats */
static void FETCH(f_rgb)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = CHAN_TO_FLOAT(src[0]);
   texel[GCOMP] = CHAN_TO_FLOAT(src[1]);
   texel[BCOMP] = CHAN_TO_FLOAT(src[2]);
   texel[ACOMP] = CHAN_MAXF;
}

/* Fetch color texel from 1D, 2D or 3D ALPHA texture, returning 4 GLchans */
static void FETCH(alpha)( const struct gl_texture_image *texImage,
			  GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0;
   texel[ACOMP] = src[0];
}

/* Fetch color texel from 1D, 2D or 3D ALPHA texture, returning 4 GLfloats */
static void FETCH(f_alpha)( const struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = 0.0;
   texel[ACOMP] = CHAN_TO_FLOAT(src[0]);
}

/* Fetch color texel from 1D, 2D or 3D LUMIN texture, returning 4 GLchans */
static void FETCH(luminance)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = src[0];
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D LUMIN texture, returning 4 GLchans */
static void FETCH(f_luminance)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] =
   texel[GCOMP] =
   texel[BCOMP] = CHAN_TO_FLOAT(src[0]);
   texel[ACOMP] = CHAN_MAXF;
}

/* Fetch color texel from 1D, 2D or 3D L_A texture, returning 4 GLchans */
static void FETCH(luminance_alpha)( const struct gl_texture_image *texImage,
				    GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 2 );
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[0];
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[1];
}

/* Fetch color texel from 1D, 2D or 3D L_A texture, returning 4 GLfloats */
static void FETCH(f_luminance_alpha)( const struct gl_texture_image *texImage,
                                  GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 2 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = CHAN_TO_FLOAT(src[0]);
   texel[ACOMP] = CHAN_TO_FLOAT(src[1]);
}


/* Fetch color texel from 1D, 2D or 3D INT. texture, returning 4 GLchans */
static void FETCH(intensity)( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[0];
   texel[BCOMP] = src[0];
   texel[ACOMP] = src[0];
}

/* Fetch color texel from 1D, 2D or 3D INT. texture, returning 4 GLfloats */
static void FETCH(f_intensity)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = 
   texel[ACOMP] = CHAN_TO_FLOAT(src[0]);
}


/* Fetch CI texel from 1D, 2D or 3D CI texture, returning 1 GLchan */
static void FETCH(color_index)( const struct gl_texture_image *texImage,
				GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[0] = src[0];
}

/* Fetch CI texel from 1D, 2D or 3D CI texture, returning 1 GLfloat */
static void FETCH(f_color_index)( const struct gl_texture_image *texImage,
				GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLchan *src = CHAN_SRC( texImage, i, j, k, 1 );
   texel[0] = (GLfloat) src[0];
}


/* Fetch depth texel from 1D, 2D or 3D DEPTH texture, returning 1 GLfloat */
/* Note: no GLchan version of this function */
static void FETCH(f_depth_component)( const struct gl_texture_image *texImage,
                                    GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = FLOAT_SRC( texImage, i, j, k, 1 );
   texel[0] = src[0];
}


/* Fetch color texel from 1D, 2D or 3D RGBA_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgba_f32)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = FLOAT_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = src[3];
}

/* Fetch color texel from 1D, 2D or 3D RGBA_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgba_f16)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfNV *src = HALF_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = _mesa_half_to_float(src[3]);
}


/* Fetch color texel from 1D, 2D or 3D RGB_FLOAT32 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgb_f32)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLfloat *src = FLOAT_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = src[0];
   texel[GCOMP] = src[1];
   texel[BCOMP] = src[2];
   texel[ACOMP] = CHAN_MAXF;
}

/* Fetch color texel from 1D, 2D or 3D RGB_FLOAT16 texture,
 * returning 4 GLfloats.
 */
static void FETCH(f_rgb_f16)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLhalfNV *src = HALF_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = _mesa_half_to_float(src[0]);
   texel[GCOMP] = _mesa_half_to_float(src[1]);
   texel[BCOMP] = _mesa_half_to_float(src[2]);
   texel[ACOMP] = CHAN_MAXF;
}



/*
 * Begin Hardware formats
 */

/* Fetch color texel from 1D, 2D or 3D rgba8888 texture, return 4 GLchans */
static void FETCH(rgba8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[3] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

/* Fetch color texel from 1D, 2D or 3D rgba8888 texture, return 4 GLfloats */
static void FETCH(f_rgba8888)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = UBYTE_TO_FLOAT( src[3] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}


/* Fetch color texel from 1D, 2D or 3D argb8888 texture, return 4 GLchans */
static void FETCH(argb8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[3] );
}

/* Fetch color texel from 1D, 2D or 3D argb8888 texture, return 4 GLfloats */
static void FETCH(f_argb8888)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = UBYTE_TO_FLOAT( src[3] );
}


/* Fetch color texel from 1D, 2D or 3D rgb888 texture, return 4 GLchans */
static void FETCH(rgb888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D rgb888 texture, return 4 GLfloats */
static void FETCH(f_rgb888)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = UBYTE_TO_FLOAT( src[2] );
   texel[GCOMP] = UBYTE_TO_FLOAT( src[1] );
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = CHAN_MAXF;
}


/* Fetch color texel from 1D, 2D or 3D rgb565 texture, return 4 GLchans */
static void FETCH(rgb565)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) * 255 / 0xf8 );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) * 255 / 0xfc );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) * 255 / 0xf8 );
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D rgb565 texture, return 4 GLfloats */
static void FETCH(f_rgb565)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 8) & 0xf8) * (1.0F / 248.0F);
   texel[GCOMP] = ((s >> 3) & 0xfc) * (1.0F / 252.0F);
   texel[BCOMP] = ((s << 3) & 0xf8) * (1.0F / 248.0F);
   texel[ACOMP] = CHAN_MAXF;
}


/* Fetch color texel from 1D, 2D or 3D rgb565 texture, return 4 GLchans */
static void FETCH(argb4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) * 255 / 0xf );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) * 255 / 0xf );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) * 255 / 0xf );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) * 255 / 0xf );
}

/* Fetch color texel from 1D, 2D or 3D rgb565 texture, return 4 GLfloats */
static void FETCH(f_argb4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = ((s >>  8) & 0xf) * (1.0F / 15.0F);
   texel[GCOMP] = ((s >>  4) & 0xf) * (1.0F / 15.0F);
   texel[BCOMP] = ((s      ) & 0xf) * (1.0F / 15.0F);
   texel[ACOMP] = ((s >> 12) & 0xf) * (1.0F / 15.0F);
}


/* Fetch color texel from 1D, 2D or 3D argb1555 texture, return 4 GLchans */
static void FETCH(argb1555)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 10) & 0x1f) * 255 / 0x1f );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  5) & 0x1f) * 255 / 0x1f );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0x1f) * 255 / 0x1f );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

/* Fetch color texel from 1D, 2D or 3D argb1555 texture, return 4 GLfloats */
static void FETCH(f_argb1555)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = ((s >> 10) & 0x1f) * (1.0F / 31.0F);
   texel[GCOMP] = ((s >>  5) & 0x1f) * (1.0F / 31.0F);
   texel[BCOMP] = ((s      ) & 0x1f) * (1.0F / 31.0F);
   texel[ACOMP] = ((s >> 15) & 0x01);
}


/* Fetch color texel from 1D, 2D or 3D al88 texture, return 4 GLchans */
static void FETCH(al88)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 2 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[1] );
}

/* Fetch color texel from 1D, 2D or 3D al88 texture, return 4 GLfloats */
static void FETCH(f_al88)( const struct gl_texture_image *texImage,
                           GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 2 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = UBYTE_TO_FLOAT( src[1] );
}


/* Fetch color texel from 1D, 2D or 3D rgb332 texture, return 4 GLchans */
static void FETCH(rgb332)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   const GLubyte s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s     ) & 0xe0) * 255 / 0xe0 );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xe0) * 255 / 0xe0 );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s << 5) & 0xc0) * 255 / 0xc0 );
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D rgb332 texture, return 4 GLfloats */
static void FETCH(f_rgb332)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   const GLubyte s = *src;
   texel[RCOMP] = ((s     ) & 0xe0) * (1.0F / 224.0F);
   texel[GCOMP] = ((s << 3) & 0xe0) * (1.0F / 224.0F);
   texel[BCOMP] = ((s << 5) & 0xc0) * (1.0F / 192.0F);
   texel[ACOMP] = CHAN_MAXF;
}


/* Fetch color texel from 1D, 2D or 3D a8 texture, return 4 GLchans */
static void FETCH(a8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = 0;
   texel[GCOMP] = 0;
   texel[BCOMP] = 0;
   texel[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

/* Fetch color texel from 1D, 2D or 3D a8 texture, return 4 GLfloats */
static void FETCH(f_a8)( const struct gl_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = 0.0;
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}


/* Fetch color texel from 1D, 2D or 3D l8 texture, return 4 GLchans */
static void FETCH(l8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D l8 texture, return 4 GLfloats */
static void FETCH(f_l8)( const struct gl_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = UBYTE_TO_FLOAT( src[0] );
   texel[ACOMP] = CHAN_MAXF;
}


/* Fetch color texel from 1D, 2D or 3D i8 texture, return 4 GLchans */
static void FETCH(i8)( const struct gl_texture_image *texImage,
		       GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

/* Fetch color texel from 1D, 2D or 3D i8 texture, return 4 GLfloats */
static void FETCH(f_i8)( const struct gl_texture_image *texImage,
                         GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[RCOMP] = 
   texel[GCOMP] = 
   texel[BCOMP] = 
   texel[ACOMP] = UBYTE_TO_FLOAT( src[0] );
}


/* Fetch color texel from 1D, 2D or 3D ci8 texture, return 1 GLchan */
static void FETCH(ci8)( const struct gl_texture_image *texImage,
			GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   GLchan *index = (GLchan *) texel;
   *index = UBYTE_TO_CHAN( *src );
}


/* Fetch color texel from 1D, 2D or 3D ci8 texture, return 1 GLfloat */
static void FETCH(f_ci8)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   texel[0] = UBYTE_TO_FLOAT( *src );
}


/* Fetch color texel from 1D, 2D or 3D ycbcr texture, return 4 GLchans */
/* We convert YCbCr to RGB here */
/* XXX this may break if GLchan != GLubyte */
static void FETCH(ycbcr)( const struct gl_texture_image *texImage,
                          GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src0 = USHORT_SRC( texImage, (i & ~1), j, k ); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
   const GLubyte cb = *src0 & 0xff;         /* chroma U */
   const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
   const GLubyte cr = *src1 & 0xff;         /* chroma V */
   GLint r, g, b;
   if (i & 1) {
      /* odd pixel: use y1,cr,cb */
      r = (GLint) (1.164 * (y1-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y1-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y1-16) + 2.018 * (cb-128));
   }
   else {
      /* even pixel: use y0,cr,cb */
      r = (GLint) (1.164 * (y0-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y0-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y0-16) + 2.018 * (cb-128));
   }
   texel[RCOMP] = CLAMP(r, 0, CHAN_MAX);
   texel[GCOMP] = CLAMP(g, 0, CHAN_MAX);
   texel[BCOMP] = CLAMP(b, 0, CHAN_MAX);
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D ycbcr texture, return 4 GLfloats */
/* We convert YCbCr to RGB here */
static void FETCH(f_ycbcr)( const struct gl_texture_image *texImage,
                            GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src0 = USHORT_SRC( texImage, (i & ~1), j, k ); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = (*src0 >> 8) & 0xff;  /* luminance */
   const GLubyte cb = *src0 & 0xff;         /* chroma U */
   const GLubyte y1 = (*src1 >> 8) & 0xff;  /* luminance */
   const GLubyte cr = *src1 & 0xff;         /* chroma V */
   GLfloat r, g, b;
   if (i & 1) {
      /* odd pixel: use y1,cr,cb */
      r = (1.164 * (y1-16) + 1.596 * (cr-128));
      g = (1.164 * (y1-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (1.164 * (y1-16) + 2.018 * (cb-128));
   }
   else {
      /* even pixel: use y0,cr,cb */
      r = (1.164 * (y0-16) + 1.596 * (cr-128));
      g = (1.164 * (y0-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (1.164 * (y0-16) + 2.018 * (cb-128));
   }
   /* XXX remove / 255 here by tweaking arithmetic above */
   r /= 255.0;
   g /= 255.0;
   b /= 255.0;
   /* XXX should we really clamp??? */
   texel[RCOMP] = CLAMP(r, 0.0, 1.0);
   texel[GCOMP] = CLAMP(g, 0.0, 1.0);
   texel[BCOMP] = CLAMP(b, 0.0, 1.0);
   texel[ACOMP] = CHAN_MAXF;
}


/* Fetch color texel from 1D, 2D or 3D ycbcr_rev texture, return 4 GLchans */
/* We convert YCbCr to RGB here */
/* XXX this may break if GLchan != GLubyte */
static void FETCH(ycbcr_rev)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src0 = USHORT_SRC( texImage, (i & ~1), j, k ); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = *src0 & 0xff;         /* luminance */
   const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
   const GLubyte y1 = *src1 & 0xff;         /* luminance */
   const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
   GLint r, g, b;
   if (i & 1) {
      /* odd pixel: use y1,cr,cb */
      r = (GLint) (1.164 * (y1-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y1-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y1-16) + 2.018 * (cb-128));
   }
   else {
      /* even pixel: use y0,cr,cb */
      r = (GLint) (1.164 * (y0-16) + 1.596 * (cr-128));
      g = (GLint) (1.164 * (y0-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (GLint) (1.164 * (y0-16) + 2.018 * (cb-128));
   }
   texel[RCOMP] = CLAMP(r, 0, CHAN_MAX);
   texel[GCOMP] = CLAMP(g, 0, CHAN_MAX);
   texel[BCOMP] = CLAMP(b, 0, CHAN_MAX);
   texel[ACOMP] = CHAN_MAX;
}

/* Fetch color texel from 1D, 2D or 3D ycbcr_rev texture, return 4 GLfloats */
/* We convert YCbCr to RGB here */
static void FETCH(f_ycbcr_rev)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   const GLushort *src0 = USHORT_SRC( texImage, (i & ~1), j, k ); /* even */
   const GLushort *src1 = src0 + 1;                               /* odd */
   const GLubyte y0 = *src0 & 0xff;         /* luminance */
   const GLubyte cr = (*src0 >> 8) & 0xff;  /* chroma V */
   const GLubyte y1 = *src1 & 0xff;         /* luminance */
   const GLubyte cb = (*src1 >> 8) & 0xff;  /* chroma U */
   GLfloat r, g, b;
   if (i & 1) {
      /* odd pixel: use y1,cr,cb */
      r = (1.164 * (y1-16) + 1.596 * (cr-128));
      g = (1.164 * (y1-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (1.164 * (y1-16) + 2.018 * (cb-128));
   }
   else {
      /* even pixel: use y0,cr,cb */
      r = (1.164 * (y0-16) + 1.596 * (cr-128));
      g = (1.164 * (y0-16) - 0.813 * (cr-128) - 0.391 * (cb-128));
      b = (1.164 * (y0-16) + 2.018 * (cb-128));
   }
   /* XXX remove / 255 here by tweaking arithmetic above */
   r /= 255.0;
   g /= 255.0;
   b /= 255.0;
   /* XXX should we really clamp??? */
   texel[RCOMP] = CLAMP(r, 0.0, 1.0);
   texel[GCOMP] = CLAMP(g, 0.0, 1.0);
   texel[BCOMP] = CLAMP(b, 0.0, 1.0);
   texel[ACOMP] = CHAN_MAXF;
}


#if DIM == 2  /* Only 2D compressed textures possible */

static void FETCH(rgb_fxt1)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLchan *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(f_rgb_fxt1)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(rgba_fxt1)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLchan *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(f_rgba_fxt1)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

#endif /* if DIM == 2 */


#if DIM == 2 /* only 2D is valid */

static void FETCH(rgb_dxt1)( const struct gl_texture_image *texImage,
                             GLint i, GLint j, GLint k, GLchan *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(f_rgb_dxt1)( const struct gl_texture_image *texImage,
                               GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}


static void FETCH(rgba_dxt1)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLchan *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(f_rgba_dxt1)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}


static void FETCH(rgba_dxt3)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLchan *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(f_rgba_dxt3)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}


static void FETCH(rgba_dxt5)( const struct gl_texture_image *texImage,
                              GLint i, GLint j, GLint k, GLchan *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

static void FETCH(f_rgba_dxt5)( const struct gl_texture_image *texImage,
                                GLint i, GLint j, GLint k, GLfloat *texel )
{
   /* Extract the (i,j) pixel from texImage->Data and return it
    * in texel[RCOMP], texel[GCOMP], texel[BCOMP], texel[ACOMP].
    */
}

#endif



/* big-endian */

#if 0
static void FETCH(abgr8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[3] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[0] );
}

static void FETCH(bgra8888)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 4 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[3] );
}

static void FETCH(bgr888)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 3 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[2] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[1] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = CHAN_MAX;
}

static void FETCH(bgr565)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 8) & 0xf8) * 255 / 0xf8 );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >> 3) & 0xfc) * 255 / 0xfc );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xf8) * 255 / 0xf8 );
   texel[ACOMP] = CHAN_MAX;
}

static void FETCH(bgra4444)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >>  8) & 0xf) * 255 / 0xf );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  4) & 0xf) * 255 / 0xf );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0xf) * 255 / 0xf );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 12) & 0xf) * 255 / 0xf );
}

static void FETCH(bgra5551)( const struct gl_texture_image *texImage,
			     GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLushort *src = USHORT_SRC( texImage, i, j, k );
   const GLushort s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s >> 10) & 0x1f) * 255 / 0x1f );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s >>  5) & 0x1f) * 255 / 0x1f );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s      ) & 0x1f) * 255 / 0x1f );
   texel[ACOMP] = UBYTE_TO_CHAN( ((s >> 15) & 0x01) * 255 );
}

static void FETCH(la88)( const struct gl_texture_image *texImage,
			 GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 2 );
   texel[RCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[GCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[BCOMP] = UBYTE_TO_CHAN( src[0] );
   texel[ACOMP] = UBYTE_TO_CHAN( src[1] );
}

static void FETCH(bgr233)( const struct gl_texture_image *texImage,
			   GLint i, GLint j, GLint k, GLchan *texel )
{
   const GLubyte *src = UBYTE_SRC( texImage, i, j, k, 1 );
   const GLubyte s = *src;
   texel[RCOMP] = UBYTE_TO_CHAN( ((s     ) & 0xe0) * 255 / 0xe0 );
   texel[GCOMP] = UBYTE_TO_CHAN( ((s << 3) & 0xe0) * 255 / 0xe0 );
   texel[BCOMP] = UBYTE_TO_CHAN( ((s << 5) & 0xc0) * 255 / 0xc0 );
   texel[ACOMP] = CHAN_MAX;
}
#endif


#undef CHAN_SRC
#undef UBYTE_SRC
#undef USHORT_SRC
#undef FLOAT_SRC
#undef HALF_SRC
#undef FETCH
#undef DIM
