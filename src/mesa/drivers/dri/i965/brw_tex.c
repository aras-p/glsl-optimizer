/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
        

#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "simple_list.h"
#include "enums.h"
#include "image.h"
#include "texstore.h"
#include "texformat.h"
#include "texmem.h"

#include "intel_ioctl.h"
#include "brw_context.h"
#include "brw_defines.h"




static const struct gl_texture_format *
brwChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			 GLenum format, GLenum type )
{
   switch ( internalFormat ) {
   case 4:
   case GL_RGBA:
   case GL_COMPRESSED_RGBA:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
   case GL_RGBA4:
   case GL_RGBA2:
   case GL_RGB5_A1:
      return &_mesa_texformat_argb8888; 
/*       return &_mesa_texformat_rgba8888_rev; */

   case 3:
   case GL_RGB:
   case GL_COMPRESSED_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
   case GL_RGB5:
   case GL_RGB4:
   case GL_R3_G3_B2:
/*       return &_mesa_texformat_rgb888; */
      return &_mesa_texformat_argb8888;

   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
   case GL_COMPRESSED_ALPHA:
      return &_mesa_texformat_a8;

   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
   case GL_COMPRESSED_LUMINANCE:
      return &_mesa_texformat_l8;

   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
   case GL_COMPRESSED_LUMINANCE_ALPHA:
      return &_mesa_texformat_al88;

   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
   case GL_COMPRESSED_INTENSITY:
      return &_mesa_texformat_i8;

   case GL_YCBCR_MESA:
      if (type == GL_UNSIGNED_SHORT_8_8_MESA ||
	  type == GL_UNSIGNED_BYTE)
         return &_mesa_texformat_ycbcr;
      else
         return &_mesa_texformat_ycbcr_rev;

   case GL_COMPRESSED_RGB_FXT1_3DFX:
   case GL_COMPRESSED_RGBA_FXT1_3DFX:
     return &_mesa_texformat_rgb_fxt1;

   case GL_RGB_S3TC:
   case GL_RGB4_S3TC:
   case GL_RGBA_S3TC:
   case GL_RGBA4_S3TC:
   case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
   case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
   case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
     return &_mesa_texformat_rgb_dxt1; /* there is no rgba support? */

   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT24:
   case GL_DEPTH_COMPONENT32:
      return &_mesa_texformat_z16;

   default:
      fprintf(stderr, "unexpected texture format %s in %s\n", 
	      _mesa_lookup_enum_by_nr(internalFormat),
	      __FUNCTION__);
      return NULL;
   }

   return NULL; /* never get here */
}


void brwInitTextureFuncs( struct dd_function_table *functions )
{
   functions->ChooseTextureFormat = brwChooseTextureFormat;
}
