/* $Id: texformat.h,v 1.7 2001/06/15 14:18:46 brianp Exp $ */

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

#ifndef TEXFORMAT_H
#define TEXFORMAT_H

#include "mtypes.h"


/* The Mesa internal texture image types.  These will be set to their
 * default value, but may be changed by drivers as required.
 */
enum _format {
   /* Hardware-friendly formats.  Drivers can override the default
    * formats and convert texture images to one of these as required.
    * These formats are all little endian, as shown below.  They will be
    * most useful for x86-based PC graphics card drivers.
    *
    * NOTE: In the default case, some of these formats will be
    * duplicates of the default formats listed above.  However, these
    * formats guarantee their internal component sizes, while GLchan may
    * vary betwen GLubyte, GLushort and GLfloat.
    */
				/* msb <------ TEXEL BITS -----------> lsb */
				/* ---- ---- ---- ---- ---- ---- ---- ---- */
   MESA_FORMAT_RGBA8888,	/* RRRR RRRR GGGG GGGG BBBB BBBB AAAA AAAA */
   MESA_FORMAT_ARGB8888,	/* AAAA AAAA RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_RGB888,		/*           RRRR RRRR GGGG GGGG BBBB BBBB */
   MESA_FORMAT_RGB565,		/*                     RRRR RGGG GGGB BBBB */
   MESA_FORMAT_ARGB4444,	/*                     AAAA RRRR GGGG BBBB */
   MESA_FORMAT_ARGB1555,	/*                     ARRR RRGG GGGB BBBB */
   MESA_FORMAT_AL88,		/*                     AAAA AAAA LLLL LLLL */
   MESA_FORMAT_RGB332,		/*                               RRRG GGBB */
   MESA_FORMAT_A8,		/*                               AAAA AAAA */
   MESA_FORMAT_L8,		/*                               LLLL LLLL */
   MESA_FORMAT_I8,		/*                               IIII IIII */
   MESA_FORMAT_CI8,		/*                               CCCC CCCC */

   /* Generic GLchan-based formats.  These are the default formats used
    * by the software rasterizer and, unless the driver overrides the
    * texture image functions, incoming images will be converted to one
    * of these formats.  Components are arrays of GLchan values, so
    * there will be no big/little endian issues.
    *
    * NOTE: Because these are based on the GLchan datatype, one cannot
    * assume 8 bits per channel with these formats.  If you require
    * GLubyte per channel, use one of the hardware formats below.
    */
   MESA_FORMAT_RGBA,
   MESA_FORMAT_RGB,
   MESA_FORMAT_ALPHA,
   MESA_FORMAT_LUMINANCE,
   MESA_FORMAT_LUMINANCE_ALPHA,
   MESA_FORMAT_INTENSITY,
   MESA_FORMAT_COLOR_INDEX,
   MESA_FORMAT_DEPTH_COMPONENT
};


extern GLboolean
_mesa_is_hardware_tex_format( const struct gl_texture_format *format );

extern const struct gl_texture_format *
_mesa_choose_tex_format( GLcontext *ctx, GLint internalFormat,
                         GLenum format, GLenum type );

extern GLint
_mesa_base_compressed_texformat(GLcontext *ctx, GLint intFormat);

extern GLint
_mesa_compressed_texture_size(GLcontext *ctx,
                              const struct gl_texture_image *texImage);


/* The default formats, GLchan per component:
 */
extern const struct gl_texture_format _mesa_texformat_rgba;
extern const struct gl_texture_format _mesa_texformat_rgb;
extern const struct gl_texture_format _mesa_texformat_alpha;
extern const struct gl_texture_format _mesa_texformat_luminance;
extern const struct gl_texture_format _mesa_texformat_luminance_alpha;
extern const struct gl_texture_format _mesa_texformat_intensity;
extern const struct gl_texture_format _mesa_texformat_color_index;
extern const struct gl_texture_format _mesa_texformat_depth_component;

/* The hardware-friendly formats:
 */
extern const struct gl_texture_format _mesa_texformat_rgba8888;
extern const struct gl_texture_format _mesa_texformat_argb8888;
extern const struct gl_texture_format _mesa_texformat_rgb888;
extern const struct gl_texture_format _mesa_texformat_rgb565;
extern const struct gl_texture_format _mesa_texformat_argb4444;
extern const struct gl_texture_format _mesa_texformat_argb1555;
extern const struct gl_texture_format _mesa_texformat_al88;
extern const struct gl_texture_format _mesa_texformat_rgb332;
extern const struct gl_texture_format _mesa_texformat_a8;
extern const struct gl_texture_format _mesa_texformat_l8;
extern const struct gl_texture_format _mesa_texformat_i8;
extern const struct gl_texture_format _mesa_texformat_ci8;

/* The null format:
 */
extern const struct gl_texture_format _mesa_null_texformat;

#endif
