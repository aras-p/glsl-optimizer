/* $Id: texformat.c,v 1.11 2001/06/15 14:18:46 brianp Exp $ */

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

#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "image.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#include "texformat.h"
#include "teximage.h"
#include "texstate.h"
#include "swrast/s_span.h"
#endif


/* Texel fetch routines for all supported formats:
 */
#define DIM 1
#include "texformat_tmp.h"

#define DIM 2
#include "texformat_tmp.h"

#define DIM 3
#include "texformat_tmp.h"

/* Have to have this so the FetchTexel function pointer is never NULL.
 */
static void fetch_null_texel( const struct gl_texture_image *texImage,
			      GLint i, GLint j, GLint k, GLvoid *texel )
{
   GLchan *rgba = (GLchan *) texel;
   rgba[RCOMP] = 0;
   rgba[GCOMP] = 0;
   rgba[BCOMP] = 0;
   rgba[ACOMP] = 0;
}


/* =============================================================
 * Default GLchan-based formats:
 */

const struct gl_texture_format _mesa_texformat_rgba = {
   MESA_FORMAT_RGBA,			/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   CHAN_TYPE,				/* Type */
   CHAN_BITS,				/* RedBits */
   CHAN_BITS,				/* GreenBits */
   CHAN_BITS,				/* BlueBits */
   CHAN_BITS,				/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4 * CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_rgba,			/* FetchTexel1D */
   fetch_2d_texel_rgba,			/* FetchTexel2D */
   fetch_3d_texel_rgba,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb = {
   MESA_FORMAT_RGB,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   CHAN_TYPE,				/* Type */
   CHAN_BITS,				/* RedBits */
   CHAN_BITS,				/* GreenBits */
   CHAN_BITS,				/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   3 * CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_rgb,			/* FetchTexel1D */
   fetch_2d_texel_rgb,			/* FetchTexel2D */
   fetch_3d_texel_rgb,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_alpha = {
   MESA_FORMAT_ALPHA,			/* MesaFormat */
   GL_ALPHA,				/* BaseFormat */
   CHAN_TYPE,				/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   CHAN_BITS,				/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_alpha,		/* FetchTexel1D */
   fetch_2d_texel_alpha,		/* FetchTexel2D */
   fetch_3d_texel_alpha,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_luminance = {
   MESA_FORMAT_LUMINANCE,		/* MesaFormat */
   GL_LUMINANCE,			/* BaseFormat */
   CHAN_TYPE,				/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   CHAN_BITS,				/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_luminance,		/* FetchTexel1D */
   fetch_2d_texel_luminance,		/* FetchTexel2D */
   fetch_3d_texel_luminance,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_luminance_alpha = {
   MESA_FORMAT_LUMINANCE_ALPHA,		/* MesaFormat */
   GL_LUMINANCE_ALPHA,			/* BaseFormat */
   CHAN_TYPE,				/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   CHAN_BITS,				/* AlphaBits */
   CHAN_BITS,				/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2 * CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_luminance_alpha,	/* FetchTexel1D */
   fetch_2d_texel_luminance_alpha,	/* FetchTexel2D */
   fetch_3d_texel_luminance_alpha,	/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_intensity = {
   MESA_FORMAT_INTENSITY,		/* MesaFormat */
   GL_INTENSITY,			/* BaseFormat */
   CHAN_TYPE,				/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   CHAN_BITS,				/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_intensity,		/* FetchTexel1D */
   fetch_2d_texel_intensity,		/* FetchTexel2D */
   fetch_3d_texel_intensity,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_color_index = {
   MESA_FORMAT_COLOR_INDEX,		/* MesaFormat */
   GL_COLOR_INDEX,			/* BaseFormat */
   CHAN_TYPE,				/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   CHAN_BITS,				/* IndexBits */
   0,					/* DepthBits */
   CHAN_BITS / 8,			/* TexelBytes */
   fetch_1d_texel_color_index,		/* FetchTexel1D */
   fetch_2d_texel_color_index,		/* FetchTexel2D */
   fetch_3d_texel_color_index,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_depth_component = {
   MESA_FORMAT_DEPTH_COMPONENT,		/* MesaFormat */
   GL_DEPTH_COMPONENT,			/* BaseFormat */
   GL_FLOAT,				/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   sizeof(GLfloat) * 8,			/* DepthBits */
   sizeof(GLfloat),			/* TexelBytes */
   fetch_1d_texel_depth_component,	/* FetchTexel1D */
   fetch_2d_texel_depth_component,	/* FetchTexel2D */
   fetch_3d_texel_depth_component,	/* FetchTexel3D */
};


/* =============================================================
 * Hardware formats:
 */

const struct gl_texture_format _mesa_texformat_rgba8888 = {
   MESA_FORMAT_RGBA8888,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_INT_8_8_8_8,		/* Type */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4,					/* TexelBytes */
   fetch_1d_texel_rgba8888,		/* FetchTexel1D */
   fetch_2d_texel_rgba8888,		/* FetchTexel2D */
   fetch_3d_texel_rgba8888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_argb8888 = {
   MESA_FORMAT_ARGB8888,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_INT_8_8_8_8_REV,		/* Type */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   4,					/* TexelBytes */
   fetch_1d_texel_argb8888,		/* FetchTexel1D */
   fetch_2d_texel_argb8888,		/* FetchTexel2D */
   fetch_3d_texel_argb8888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb888 = {
   MESA_FORMAT_RGB888,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   8,					/* RedBits */
   8,					/* GreenBits */
   8,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   3,					/* TexelBytes */
   fetch_1d_texel_rgb888,		/* FetchTexel1D */
   fetch_2d_texel_rgb888,		/* FetchTexel2D */
   fetch_3d_texel_rgb888,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb565 = {
   MESA_FORMAT_RGB565,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_SHORT_5_6_5,		/* Type */
   5,					/* RedBits */
   6,					/* GreenBits */
   5,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_rgb565,		/* FetchTexel1D */
   fetch_2d_texel_rgb565,		/* FetchTexel2D */
   fetch_3d_texel_rgb565,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_argb4444 = {
   MESA_FORMAT_ARGB4444,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_SHORT_4_4_4_4_REV,	/* Type */
   4,					/* RedBits */
   4,					/* GreenBits */
   4,					/* BlueBits */
   4,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_argb4444,		/* FetchTexel1D */
   fetch_2d_texel_argb4444,		/* FetchTexel2D */
   fetch_3d_texel_argb4444,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_argb1555 = {
   MESA_FORMAT_ARGB1555,		/* MesaFormat */
   GL_RGBA,				/* BaseFormat */
   GL_UNSIGNED_SHORT_1_5_5_5_REV,	/* Type */
   5,					/* RedBits */
   5,					/* GreenBits */
   5,					/* BlueBits */
   1,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_argb1555,		/* FetchTexel1D */
   fetch_2d_texel_argb1555,		/* FetchTexel2D */
   fetch_3d_texel_argb1555,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_al88 = {
   MESA_FORMAT_AL88,			/* MesaFormat */
   GL_LUMINANCE_ALPHA,			/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   8,					/* AlphaBits */
   8,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   2,					/* TexelBytes */
   fetch_1d_texel_al88,			/* FetchTexel1D */
   fetch_2d_texel_al88,			/* FetchTexel2D */
   fetch_3d_texel_al88,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_rgb332 = {
   MESA_FORMAT_RGB332,			/* MesaFormat */
   GL_RGB,				/* BaseFormat */
   GL_UNSIGNED_BYTE_3_3_2,		/* Type */
   3,					/* RedBits */
   3,					/* GreenBits */
   2,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_rgb332,		/* FetchTexel1D */
   fetch_2d_texel_rgb332,		/* FetchTexel2D */
   fetch_3d_texel_rgb332,		/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_a8 = {
   MESA_FORMAT_A8,			/* MesaFormat */
   GL_ALPHA,				/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   8,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_a8,			/* FetchTexel1D */
   fetch_2d_texel_a8,			/* FetchTexel2D */
   fetch_3d_texel_a8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_l8 = {
   MESA_FORMAT_L8,			/* MesaFormat */
   GL_LUMINANCE,			/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   8,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_l8,			/* FetchTexel1D */
   fetch_2d_texel_l8,			/* FetchTexel2D */
   fetch_3d_texel_l8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_i8 = {
   MESA_FORMAT_I8,			/* MesaFormat */
   GL_INTENSITY,			/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   8,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_i8,			/* FetchTexel1D */
   fetch_2d_texel_i8,			/* FetchTexel2D */
   fetch_3d_texel_i8,			/* FetchTexel3D */
};

const struct gl_texture_format _mesa_texformat_ci8 = {
   MESA_FORMAT_CI8,			/* MesaFormat */
   GL_COLOR_INDEX,			/* BaseFormat */
   GL_UNSIGNED_BYTE,			/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   8,					/* IndexBits */
   0,					/* DepthBits */
   1,					/* TexelBytes */
   fetch_1d_texel_ci8,			/* FetchTexel1D */
   fetch_2d_texel_ci8,			/* FetchTexel2D */
   fetch_3d_texel_ci8,			/* FetchTexel3D */
};


/* =============================================================
 * Null format:
 */

const struct gl_texture_format _mesa_null_texformat = {
   -1,					/* MesaFormat */
   0,					/* BaseFormat */
   0,					/* Type */
   0,					/* RedBits */
   0,					/* GreenBits */
   0,					/* BlueBits */
   0,					/* AlphaBits */
   0,					/* LuminanceBits */
   0,					/* IntensityBits */
   0,					/* IndexBits */
   0,					/* DepthBits */
   0,					/* TexelBytes */
   fetch_null_texel,			/* FetchTexel1D */
   fetch_null_texel,			/* FetchTexel2D */
   fetch_null_texel,			/* FetchTexel3D */
};



GLboolean
_mesa_is_hardware_tex_format( const struct gl_texture_format *format )
{
   return (format->MesaFormat < MESA_FORMAT_RGBA);
}


/* Given an internal texture format (or 1, 2, 3, 4) return a pointer
 * to a gl_texture_format which which to store the texture.
 * This is called via ctx->Driver.ChooseTextureFormat().
 * Hardware drivers should not use this function, but instead a 
 * specialized function.
 */
const struct gl_texture_format *
_mesa_choose_tex_format( GLcontext *ctx, GLint internalFormat,
                         GLenum format, GLenum type )
{
   (void) format;
   (void) type;

   switch ( internalFormat ) {
      /* GH: Bias towards GL_RGB, GL_RGBA texture formats.  This has
       * got to be better than sticking them way down the end of this
       * huge list.
       */
   case 4:				/* Quake3 uses this... */
   case GL_RGBA:
      return &_mesa_texformat_rgba;

   case 3:				/* ... and this. */
   case GL_RGB:
      return &_mesa_texformat_rgb;

      /* GH: Okay, keep checking as normal.  Still test for GL_RGB,
       * GL_RGBA formats first.
       */
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return &_mesa_texformat_rgba;

   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_rgb;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return &_mesa_texformat_alpha;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      return &_mesa_texformat_luminance;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      return &_mesa_texformat_luminance_alpha;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return &_mesa_texformat_intensity;

   case GL_COLOR_INDEX:
   case GL_COLOR_INDEX1_EXT:
   case GL_COLOR_INDEX2_EXT:
   case GL_COLOR_INDEX4_EXT:
   case GL_COLOR_INDEX8_EXT:
   case GL_COLOR_INDEX12_EXT:
   case GL_COLOR_INDEX16_EXT:
      return &_mesa_texformat_color_index;

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16_SGIX:
   case GL_DEPTH_COMPONENT24_SGIX:
   case GL_DEPTH_COMPONENT32_SGIX:
      if (!ctx->Extensions.SGIX_depth_texture)
	 _mesa_problem(ctx, "depth format without GL_SGIX_depth_texture");
      return &_mesa_texformat_depth_component;

   case GL_COMPRESSED_ALPHA_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_alpha;
   case GL_COMPRESSED_LUMINANCE_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_luminance;
   case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_luminance_alpha;
   case GL_COMPRESSED_INTENSITY_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_intensity;
   case GL_COMPRESSED_RGB_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_rgb;
   case GL_COMPRESSED_RGBA_ARB:
      if (!ctx->Extensions.ARB_texture_compression)
	 _mesa_problem(ctx, "texture compression extension not enabled");
      return &_mesa_texformat_rgba;

   default:
      _mesa_problem(ctx, "unexpected format in _mesa_choose_tex_format()");
      printf("intformat = %d %x\n", internalFormat, internalFormat);
      return NULL;
   }
}




/*
 * Return the base texture format for the given compressed format
 * Called via ctx->Driver.BaseCompressedTexFormat().
 * This function is used by software rasterizers.  Hardware drivers
 * which support texture compression should not use this function but
 * a specialized function instead.
 */
GLint
_mesa_base_compressed_texformat(GLcontext *ctx, GLint intFormat)
{
   switch (intFormat) {
   case GL_COMPRESSED_ALPHA_ARB:
      return GL_ALPHA;
   case GL_COMPRESSED_LUMINANCE_ARB:
      return GL_LUMINANCE;
   case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      return GL_LUMINANCE_ALPHA;
   case GL_COMPRESSED_INTENSITY_ARB:
      return GL_INTENSITY;
   case GL_COMPRESSED_RGB_ARB:
      return GL_RGB;
   case GL_COMPRESSED_RGBA_ARB:
      return GL_RGBA;
   default:
      return -1;  /* not a recognized compressed format */
   }
}


/*
 * Called via ctx->Driver.CompressedTextureSize().
 * This function is only used by software rasterizers.
 * Hardware drivers will have to implement a specialized function.
 */
GLint
_mesa_compressed_texture_size(GLcontext *ctx,
                              const struct gl_texture_image *texImage)
{
   GLint b;
   assert(texImage);
   assert(texImage->TexFormat);

   b = texImage->Width * texImage->Height * texImage->Depth *
      texImage->TexFormat->TexelBytes;
   assert(b > 0);
   return b;
}
