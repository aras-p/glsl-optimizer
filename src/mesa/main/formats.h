/*
 * Mesa 3-D graphics library
 * Version:  7.7
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Brian Paul
 */


#ifndef FORMATS_H
#define FORMATS_H


#include "main/mtypes.h"



/**
 * Mesa texture/renderbuffer image formats.
 */
typedef enum
{
   MESA_FORMAT_NONE = 0,

   /** 
    * \name Basic hardware formats
    */
   /*@{*/
				/* msb <------ TEXEL BITS -----------> lsb */
				/* ---- ---- ---- ---- ---- ---- ---- ---- */
   MESA_FORMAT_RGBA8888,	/* RRRR RRRR GGGG GGGG BBBB BBBB AAAA AAAA */
   MESA_FORMAT_RGBA8888_REV,	/* AAAA AAAA BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_ARGB8888,	/* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_ARGB8888_REV,	/* BBBB BBBB GGGG GGGG RRRR RRRR AAAA AAAA */
   MESA_FORMAT_XRGB8888,	/* xxxx xxxx RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_XRGB8888_REV,	/* BBBB BBBB GGGG GGGG RRRR RRRR xxxx xxxx */
   MESA_FORMAT_RGB888,		/*           RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_BGR888,		/*           BBBB BBBB GGGG GGGG RRRR RRRR */
   MESA_FORMAT_RGB565,		/*                     RRRR RGGG GGGB BBBB */
   MESA_FORMAT_RGB565_REV,	/*                     GGGB BBBB RRRR RGGG */
   MESA_FORMAT_ARGB4444,	/*                     AAAA RRRR GGGG BBBB */
   MESA_FORMAT_ARGB4444_REV,	/*                     GGGG BBBB AAAA RRRR */
   MESA_FORMAT_RGBA5551,        /*                     RRRR RGGG GGBB BBBA */
   MESA_FORMAT_ARGB1555,	/*                     ARRR RRGG GGGB BBBB */
   MESA_FORMAT_ARGB1555_REV,	/*                     GGGB BBBB ARRR RRGG */
   MESA_FORMAT_AL88,		/*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_AL88_REV,	/*                     LLLL LLLL AAAA AAAA */
   MESA_FORMAT_AL1616,          /* AAAA AAAA AAAA AAAA LLLL LLLL LLLL LLLL */
   MESA_FORMAT_AL1616_REV,      /* LLLL LLLL LLLL LLLL AAAA AAAA AAAA AAAA */
   MESA_FORMAT_RGB332,		/*                               RRRG GGBB */
   MESA_FORMAT_A8,		/*                               AAAA AAAA */
   MESA_FORMAT_L8,		/*                               LLLL LLLL */
   MESA_FORMAT_I8,		/*                               IIII IIII */
   MESA_FORMAT_CI8,		/*                               CCCC CCCC */
   MESA_FORMAT_YCBCR,		/*                     YYYY YYYY UorV UorV */
   MESA_FORMAT_YCBCR_REV,	/*                     UorV UorV YYYY YYYY */
   MESA_FORMAT_Z24_S8,          /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ SSSS SSSS */
   MESA_FORMAT_S8_Z24,          /* SSSS SSSS ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_Z16,             /*                     ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_X8_Z24,          /* xxxx xxxx ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_Z24_X8,          /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ xxxx xxxx */
   MESA_FORMAT_Z32,             /* ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ ZZZZ */
   MESA_FORMAT_S8,              /*                               SSSS SSSS */
   /*@}*/

   /**
    * \name 8-bit/channel sRGB formats
    */
   /*@{*/
   MESA_FORMAT_SRGB8,
   MESA_FORMAT_SRGBA8,
   MESA_FORMAT_SARGB8,
   MESA_FORMAT_SL8,
   MESA_FORMAT_SLA8,
   MESA_FORMAT_SRGB_DXT1,
   MESA_FORMAT_SRGBA_DXT1,
   MESA_FORMAT_SRGBA_DXT3,
   MESA_FORMAT_SRGBA_DXT5,
   /*@}*/

   /**
    * \name Compressed texture formats.
    */
   /*@{*/
   MESA_FORMAT_RGB_FXT1,
   MESA_FORMAT_RGBA_FXT1,
   MESA_FORMAT_RGB_DXT1,
   MESA_FORMAT_RGBA_DXT1,
   MESA_FORMAT_RGBA_DXT3,
   MESA_FORMAT_RGBA_DXT5,
   /*@}*/

   /**
    * \name Floating point texture formats.
    */
   /*@{*/
   MESA_FORMAT_RGBA_FLOAT32,
   MESA_FORMAT_RGBA_FLOAT16,
   MESA_FORMAT_RGB_FLOAT32,
   MESA_FORMAT_RGB_FLOAT16,
   MESA_FORMAT_ALPHA_FLOAT32,
   MESA_FORMAT_ALPHA_FLOAT16,
   MESA_FORMAT_LUMINANCE_FLOAT32,
   MESA_FORMAT_LUMINANCE_FLOAT16,
   MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32,
   MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16,
   MESA_FORMAT_INTENSITY_FLOAT32,
   MESA_FORMAT_INTENSITY_FLOAT16,
   /*@}*/

   /**
    * \name Signed fixed point texture formats.
    */
   /*@{*/
   MESA_FORMAT_DUDV8,
   MESA_FORMAT_SIGNED_RGBA8888,
   MESA_FORMAT_SIGNED_RGBA8888_REV,
   MESA_FORMAT_SIGNED_RGBA_16,
   /*@}*/

   MESA_FORMAT_COUNT
} gl_format;


extern const char *
_mesa_get_format_name(gl_format format);

extern GLuint
_mesa_get_format_bytes(gl_format format);

extern GLint
_mesa_get_format_bits(gl_format format, GLenum pname);

extern GLenum
_mesa_get_format_datatype(gl_format format);

extern GLenum
_mesa_get_format_base_format(gl_format format);

extern void
_mesa_get_format_block_size(gl_format format, GLuint *bw, GLuint *bh);

extern GLboolean
_mesa_is_format_compressed(gl_format format);

extern GLenum
_mesa_get_format_color_encoding(gl_format format);

extern GLuint
_mesa_format_image_size(gl_format format, GLsizei width,
                        GLsizei height, GLsizei depth);

extern GLint
_mesa_format_row_stride(gl_format format, GLsizei width);

extern void
_mesa_format_to_type_and_comps(gl_format format,
                               GLenum *datatype, GLuint *comps);

extern void
_mesa_test_formats(void);

#endif /* FORMATS_H */
