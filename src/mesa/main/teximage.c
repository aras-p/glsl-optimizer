/* $Id: teximage.c,v 1.11 1999/11/08 07:36:44 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "image.h"
#include "macros.h"
#include "mmath.h"
#include "span.h"
#include "teximage.h"
#include "texstate.h"
#include "types.h"
#endif


/*
 * NOTES:
 *
 * Mesa's native texture datatype is GLubyte.  Native formats are
 * GL_ALPHA, GL_LUMINANCE, GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, GL_RGBA,
 * and GL_COLOR_INDEX.
 * Device drivers are free to implement any internal format they want.
 */



static struct gl_pixelstore_attrib defaultPacking = {
   1,            /* Alignment */
   0,            /* RowLength */
   0,            /* SkipPixels */
   0,            /* SkipRows */
   0,            /* ImageHeight */
   0,            /* SkipImages */
   GL_FALSE,     /* SwapBytes */
   GL_FALSE      /* LsbFirst */
};



/*
 * Compute log base 2 of n.
 * If n isn't an exact power of two return -1.
 * If n<0 return -1.
 */
static int logbase2( int n )
{
   GLint i = 1;
   GLint log2 = 0;

   if (n<0) {
      return -1;
   }

   while ( n > i ) {
      i *= 2;
      log2++;
   }
   if (i != n) {
      return -1;
   }
   else {
      return log2;
   }
}



/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.
 * Return -1 if invalid enum.
 */
static GLint decode_internal_format( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return GL_LUMINANCE_ALPHA;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return GL_INTENSITY;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_RGBA;
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return GL_COLOR_INDEX;
      default:
         return -1;  /* error */
   }
}



/*
 * Given an internal texture format enum or 1, 2, 3, 4 return the
 * corresponding _base_ internal format:  GL_ALPHA, GL_LUMINANCE,
 * GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, or GL_RGBA.  Return the
 * number of components for the format.  Return -1 if invalid enum.
 */
static GLint components_in_intformat( GLint format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return 1;
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return 1;
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
         return 2;
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
         return 1;
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return 3;
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return 4;
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return 1;
      default:
         return -1;  /* error */
   }
}



struct gl_texture_image *gl_alloc_texture_image( void )
{
   return CALLOC_STRUCT(gl_texture_image);
}



void gl_free_texture_image( struct gl_texture_image *teximage )
{
   if (teximage->Data) {
      FREE( teximage->Data );
      teximage->Data = NULL;
   }
   FREE( teximage );
}



/*
 * Examine the texImage->Format field and set the Red, Green, Blue, etc
 * texel component sizes to default values.
 * These fields are set only here by core Mesa but device drivers may
 * overwritting these fields to indicate true texel resolution.
 */
static void set_teximage_component_sizes( struct gl_texture_image *texImage )
{
   switch (texImage->Format) {
      case GL_ALPHA:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 8;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_LUMINANCE:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 8;
         texImage->IndexBits = 0;
         break;
      case GL_LUMINANCE_ALPHA:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 8;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 8;
         texImage->IndexBits = 0;
         break;
      case GL_INTENSITY:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 8;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RED:
         texImage->RedBits = 8;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_GREEN:
         texImage->RedBits = 0;
         texImage->GreenBits = 8;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_BLUE:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 8;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RGB:
      case GL_BGR:
         texImage->RedBits = 8;
         texImage->GreenBits = 8;
         texImage->BlueBits = 8;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_RGBA:
      case GL_BGRA:
      case GL_ABGR_EXT:
         texImage->RedBits = 8;
         texImage->GreenBits = 8;
         texImage->BlueBits = 8;
         texImage->AlphaBits = 8;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 0;
         break;
      case GL_COLOR_INDEX:
         texImage->RedBits = 0;
         texImage->GreenBits = 0;
         texImage->BlueBits = 0;
         texImage->AlphaBits = 0;
         texImage->IntensityBits = 0;
         texImage->LuminanceBits = 0;
         texImage->IndexBits = 8;
         break;
      default:
         gl_problem(NULL, "unexpected format in set_teximage_component_sizes");
   }
}


/* Need this to prevent an out-of-bounds memory access when using
 * X86 optimized code.
 */
#ifdef USE_X86_ASM
#  define EXTRA_BYTE 1
#else
#  define EXTRA_BYTE 0
#endif



/*
 * This is called by glTexImage[123]D in order to build a gl_texture_image
 * object given the client's parameters and image data.
 * 
 * NOTES: Width, height and depth should include the border.
 *        All texture image parameters should have already been error checked.
 */
static struct gl_texture_image *
make_texture_image( GLcontext *ctx, GLint internalFormat,
                    GLint width, GLint height, GLint depth, GLint border,
                    GLenum srcFormat, GLenum srcType, const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *unpacking)
{
   GLint components, numPixels;
   struct gl_texture_image *texImage;

   assert(width > 0);
   assert(height > 0);
   assert(depth > 0);
   assert(border == 0 || border == 1);
   assert(pixels);
   assert(unpacking);


   /*
    * Allocate and initialize the texture_image struct
    */
   texImage = gl_alloc_texture_image();
   if (!texImage)
      return NULL;

   texImage->Format = (GLenum) decode_internal_format(internalFormat);
   set_teximage_component_sizes( texImage );
   texImage->IntFormat = (GLenum) internalFormat;
   texImage->Border = border;
   texImage->Width = width;
   texImage->Height = height;
   texImage->Depth = depth;
   texImage->WidthLog2 = logbase2(width - 2 * border);
   if (height == 1)  /* 1-D texture */
      texImage->HeightLog2 = 0;
   else
      texImage->HeightLog2 = logbase2(height - 2 * border);
   if (depth == 1)   /* 2-D texture */
      texImage->DepthLog2 = 0;
   else
      texImage->DepthLog2 = logbase2(depth - 2 * border);
   texImage->Width2 = 1 << texImage->WidthLog2;
   texImage->Height2 = 1 << texImage->HeightLog2;
   texImage->Depth2 = 1 << texImage->DepthLog2;
   texImage->MaxLog2 = MAX2(texImage->WidthLog2, texImage->HeightLog2);

   components = components_in_intformat(internalFormat);
   numPixels = texImage->Width * texImage->Height * texImage->Depth;

   texImage->Data = (GLubyte *) MALLOC(numPixels * components + EXTRA_BYTE);

   if (!texImage->Data) {
      /* out of memory */
      gl_free_texture_image(texImage);
      return NULL;
   }


   /*
    * OK, the texture image struct has been initialized and the texture
    * image memory has been allocated.
    * Now fill in the texture image from the source data.
    * This includes applying the pixel transfer operations.
    */

   /* try common 2D texture cases first */
   if (!ctx->Pixel.ScaleOrBiasRGBA && !ctx->Pixel.MapColorFlag
       && !ctx->Pixel.IndexOffset && !ctx->Pixel.IndexShift
       && srcType == GL_UNSIGNED_BYTE && depth == 1) {

      if (srcFormat == internalFormat) {
         /* This will cover the common GL_RGB, GL_RGBA, GL_ALPHA,
          * GL_LUMINANCE_ALPHA, etc. texture formats.
          */
         const GLubyte *src = gl_pixel_addr_in_image(unpacking,
                pixels, width, height, srcFormat, srcType, 0, 0, 0);
         const GLubyte *src1 = gl_pixel_addr_in_image(unpacking,
                pixels, width, height, srcFormat, srcType, 0, 1, 0);
         const GLint srcStride = src1 - src;
         GLubyte *dst = texImage->Data;
         GLint dstBytesPerRow = width * components * sizeof(GLubyte);
         if (srcStride == dstBytesPerRow) {
            MEMCPY(dst, src, height * dstBytesPerRow);
         }
         else {
            GLint i;
            for (i = 0; i < height; i++) {
               MEMCPY(dst, src, dstBytesPerRow);
               src += srcStride;
               dst += dstBytesPerRow;
            }
         }
         return texImage;  /* all done */
      }
      else if (srcFormat == GL_RGBA && internalFormat == GL_RGB) {
         /* commonly used by Quake */
         const GLubyte *src = gl_pixel_addr_in_image(unpacking,
                pixels, width, height, srcFormat, srcType, 0, 0, 0);
         const GLubyte *src1 = gl_pixel_addr_in_image(unpacking,
                pixels, width, height, srcFormat, srcType, 0, 1, 0);
         const GLint srcStride = src1 - src;
         GLubyte *dst = texImage->Data;
         GLint i, j;
         for (i = 0; i < height; i++) {
            const GLubyte *s = src;
            for (j = 0; j < width; j++) {
               *dst++ = *s++;  /*red*/
               *dst++ = *s++;  /*green*/
               *dst++ = *s++;  /*blue*/
               s++;            /*alpha*/
            }
            src += srcStride;
         }
         return texImage;  /* all done */
      }
   }      


   /*
    * General case solutions
    */
   if (texImage->Format == GL_COLOR_INDEX) {
      /* color index texture */
      const GLint destBytesPerRow = width * components * sizeof(GLubyte);
      const GLenum dstType = GL_UNSIGNED_BYTE;
      GLubyte *dest = texImage->Data;
      GLint img, row;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            const GLvoid *source = gl_pixel_addr_in_image(unpacking,
                pixels, width, height, srcFormat, srcType, img, row, 0);
            _mesa_unpack_index_span(ctx, width, dstType, dest,
                                    srcType, source, unpacking, GL_TRUE);
            dest += destBytesPerRow;
         }
      }
   }
   else {
      /* regular, color texture */
      const GLint destBytesPerRow = width * components * sizeof(GLubyte);
      const GLenum dstFormat = texImage->Format;
      GLubyte *dest = texImage->Data;
      GLint img, row;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            const GLvoid *source = gl_pixel_addr_in_image(unpacking,
                   pixels, width, height, srcFormat, srcType, img, row, 0);
            _mesa_unpack_ubyte_color_span(ctx, width, dstFormat, dest,
                   srcFormat, srcType, source, unpacking, GL_TRUE);
            dest += destBytesPerRow;
         }
      }
   }

   return texImage;   /* All done! */
}



/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.
 */
static struct gl_texture_image *
make_null_texture( GLcontext *ctx, GLenum internalFormat,
                   GLsizei width, GLsizei height, GLsizei depth, GLint border )
{
   GLint components;
   struct gl_texture_image *texImage;
   GLint numPixels;
   (void) ctx;

   /*internalFormat = decode_internal_format(internalFormat);*/
   components = components_in_intformat(internalFormat);
   numPixels = width * height * depth;

   texImage = gl_alloc_texture_image();
   if (!texImage)
      return NULL;

   texImage->Format = (GLenum) decode_internal_format(internalFormat);
   set_teximage_component_sizes( texImage );
   texImage->IntFormat = internalFormat;
   texImage->Border = border;
   texImage->Width = width;
   texImage->Height = height;
   texImage->Depth = depth;
   texImage->WidthLog2 = logbase2(width - 2*border);
   if (height==1)  /* 1-D texture */
      texImage->HeightLog2 = 0;
   else
      texImage->HeightLog2 = logbase2(height - 2*border);
   if (depth==1)   /* 2-D texture */
      texImage->DepthLog2 = 0;
   else
      texImage->DepthLog2 = logbase2(depth - 2*border);
   texImage->Width2 = 1 << texImage->WidthLog2;
   texImage->Height2 = 1 << texImage->HeightLog2;
   texImage->Depth2 = 1 << texImage->DepthLog2;
   texImage->MaxLog2 = MAX2( texImage->WidthLog2, texImage->HeightLog2 );

   /* XXX should we really allocate memory for the image or let it be NULL? */
   /*texImage->Data = NULL;*/

   texImage->Data = (GLubyte *) MALLOC( numPixels * components + EXTRA_BYTE );

   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (texImage->Data) {
      char message[8][32] = {
         "   X   X  XXXXX   XXX     X    ",
         "   XX XX  X      X   X   X X   ",
         "   X X X  X      X      X   X  ",
         "   X   X  XXXX    XXX   XXXXX  ",
         "   X   X  X          X  X   X  ",
         "   X   X  X      X   X  X   X  ",
         "   X   X  XXXXX   XXX   X   X  ",
         "                               "
      };

      GLubyte *imgPtr = texImage->Data;
      GLint i, j, k;
      for (i=0;i<height;i++) {
         GLint srcRow = 7 - i % 8;
         for (j=0;j<width;j++) {
            GLint srcCol = j % 32;
            GLint texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
            for (k=0;k<components;k++) {
               *imgPtr++ = (GLubyte) texel;
            }
         }
      }
   }

   return texImage;
}



/*
 * Test glTexImage[123]D() parameters for errors.
 * Input:
 *         dimensions - must be 1 or 2 or 3
 * Return:  GL_TRUE = an error was detected, GL_FALSE = no errors
 */
static GLboolean
texture_error_check( GLcontext *ctx, GLenum target,
                     GLint level, GLint internalFormat,
                     GLenum format, GLenum type,
                     GLuint dimensions,
                     GLint width, GLint height,
                     GLint depth, GLint border )
{
   GLboolean isProxy;
   GLint iformat;

   if (dimensions == 1) {
      isProxy = (GLboolean) (target == GL_PROXY_TEXTURE_1D);
      if (target != GL_TEXTURE_1D && !isProxy) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      isProxy = (GLboolean) (target == GL_PROXY_TEXTURE_2D);
      if (target != GL_TEXTURE_2D && !isProxy) {
          gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
          return GL_TRUE;
      }
   }
   else if (dimensions == 3) {
      isProxy = (GLboolean) (target == GL_PROXY_TEXTURE_3D);
      if (target != GL_TEXTURE_3D && !isProxy) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
         return GL_TRUE;
      }
   }
   else {
      gl_problem( ctx, "bad dims in texture_error_check" );
      return GL_TRUE;
   }

   /* Border */
   if (border!=0 && border!=1) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(border)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   /* Width */
   if (width < 2 * border || width > 2 + ctx->Const.MaxTextureSize
       || logbase2( width - 2 * border ) < 0) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(width)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   /* Height */
   if (dimensions >= 2) {
      if (height < 2 * border || height > 2 + ctx->Const.MaxTextureSize
          || logbase2( height - 2 * border ) < 0) {
         if (!isProxy) {
            char message[100];
            sprintf(message, "glTexImage%dD(height)", dimensions);
            gl_error(ctx, GL_INVALID_VALUE, message);
         }
         return GL_TRUE;
      }
   }

   /* Depth */
   if (dimensions >= 3) {
      if (depth < 2 * border || depth > 2 + ctx->Const.MaxTextureSize
          || logbase2( depth - 2 * border ) < 0) {
         if (!isProxy) {
            gl_error( ctx, GL_INVALID_VALUE, "glTexImage3D(depth)" );
         }
         return GL_TRUE;
      }
   }

   /* Level */
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(level)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   iformat = decode_internal_format( internalFormat );
   if (iformat < 0) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(internalFormat)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   if (!gl_is_legal_format_and_type( format, type )) {
      /* Yes, generate GL_INVALID_OPERATION, not GL_INVALID_ENUM, if there
       * is a type/format mismatch.  See 1.2 spec page 94, sec 3.6.4.
       */
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(format or type)", dimensions);
         gl_error(ctx, GL_INVALID_OPERATION, message);
      }
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}



/*
 * Test glTexSubImage[123]D() parameters for errors.
 * Input:
 *         dimensions - must be 1 or 2 or 3
 * Return:  GL_TRUE = an error was detected, GL_FALSE = no errors
 */
static GLboolean
subtexture_error_check( GLcontext *ctx, GLint dimensions,
                        GLenum target, GLint level,
                        GLint xoffset, GLint yoffset, GLint zoffset,
                        GLint width, GLint height, GLint depth,
                        GLenum format, GLenum type )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (dimensions == 1) {
      if (target != GL_TEXTURE_1D) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 2) {
      if (target != GL_TEXTURE_2D) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
         return GL_TRUE;
      }
   }
   else if (dimensions == 3) {
      if (target != GL_TEXTURE_3D) {
         gl_error( ctx, GL_INVALID_ENUM, "glTexSubImage3D(target)" );
         return GL_TRUE;
      }
   }
   else {
      gl_problem( ctx, "bad dims in texture_error_check" );
      return GL_TRUE;
   }

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      gl_error(ctx, GL_INVALID_ENUM, "glTexSubImage2D(level)");
      return GL_TRUE;
   }

   if (width < 0) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(width)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (height < 0 && dimensions > 1) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(height)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (depth < 0 && dimensions > 2) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(depth)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   destTex = texUnit->CurrentD[2]->Image[level];
   if (!destTex) {
      gl_error(ctx, GL_INVALID_OPERATION, "glTexSubImage2D");
      return GL_TRUE;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      gl_error(ctx, GL_INVALID_VALUE, "glTexSubImage1/2/3D(xoffset)");
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (destTex->Width + destTex->Border)) {
      gl_error(ctx, GL_INVALID_VALUE, "glTexSubImage1/2/3D(xoffset+width)");
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)destTex->Border)) {
         gl_error(ctx, GL_INVALID_VALUE, "glTexSubImage2/3D(yoffset)");
         return GL_TRUE;
      }
      if (yoffset + height > (GLint) (destTex->Height + destTex->Border)) {
         gl_error(ctx, GL_INVALID_VALUE, "glTexSubImage2/3D(yoffset+height)");
         return GL_TRUE;
      }
   }
   if (dimensions > 2) {
      if (zoffset < -((GLint)destTex->Border)) {
         gl_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset)");
         return GL_TRUE;
      }
      if (zoffset + depth  > (GLint) (destTex->Depth+destTex->Border)) {
         gl_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset+depth)");
         return GL_TRUE;
      }
   }

   if (!gl_is_legal_format_and_type(format, type)) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(format or type)", dimensions);
      gl_error(ctx, GL_INVALID_ENUM, message);
      return GL_TRUE;
   }

   return GL_FALSE;
}


/*
 * Test glCopyTexImage[12]D() parameters for errors.
 * Input:  dimensions - must be 1 or 2 or 3
 * Return:  GL_TRUE = an error was detected, GL_FALSE = no errors
 */
static GLboolean
copytexture_error_check( GLcontext *ctx, GLint dimensions,
                         GLenum target, GLint level, GLint internalFormat,
                         GLint width, GLint height, GLint border )
{
   GLint iformat;

   if (target != GL_TEXTURE_1D && target != GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage1/2D(target)" );
      return GL_TRUE;
   }

   if (dimensions == 1 && target != GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage1D(target)" );
      return GL_TRUE;
   }
   else if (dimensions == 2 && target != GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
      return GL_TRUE;
   }

   /* Border */
   if (border!=0 && border!=1) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(border)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   /* Width */
   if (width < 2 * border || width > 2 + ctx->Const.MaxTextureSize
       || logbase2( width - 2 * border ) < 0) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(width)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   /* Height */
   if (dimensions >= 2) {
      if (height < 2 * border || height > 2 + ctx->Const.MaxTextureSize
          || logbase2( height - 2 * border ) < 0) {
         char message[100];
         sprintf(message, "glCopyTexImage%dD(height)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
   }

   /* Level */
   if (level<0 || level>=ctx->Const.MaxTextureLevels) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(level)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   iformat = decode_internal_format( internalFormat );
   if (iformat < 0) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(internalFormat)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}


static GLboolean
copytexsubimage_error_check( GLcontext *ctx, GLint dimensions,
                             GLenum target, GLint level,
                             GLint xoffset, GLint yoffset, GLint zoffset,
                             GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *teximage;

   if (dimensions == 1 && target != GL_TEXTURE_1D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage1D(target)" );
      return GL_TRUE;
   }
   else if (dimensions == 2 && target != GL_TEXTURE_2D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
      return GL_TRUE;
   }
   else if (dimensions == 3 && target != GL_TEXTURE_3D) {
      gl_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage3D(target)" );
      return GL_TRUE;
   }

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(level)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   if (width < 0) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(width)", dimensions );
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (dimensions > 1 && height < 0) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(height)", dimensions );
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   teximage = texUnit->CurrentD[dimensions]->Image[level];
   if (!teximage) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(undefined texture)", dimensions);
      gl_error(ctx, GL_INVALID_OPERATION, message);
      return GL_TRUE;
   }

   if (xoffset < -((GLint)teximage->Border)) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(xoffset)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (xoffset+width > (GLint) (teximage->Width+teximage->Border)) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(xoffset+width)", dimensions);
      gl_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(yoffset)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (yoffset+height > (GLint) (teximage->Height+teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(yoffset+height)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
   }

   if (dimensions > 2) {
      if (zoffset < -((GLint)teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(zoffset)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
      if (zoffset > (GLint) (teximage->Depth+teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(zoffset+depth)", dimensions);
         gl_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}




/*
 * Called from the API.  Note that width includes the border.
 */
void gl_TexImage1D( GLcontext *ctx, GLenum target, GLint level,
                    GLint internalformat,
                    GLsizei width, GLint border, GLenum format,
                    GLenum type, const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage1D");

   if (target==GL_TEXTURE_1D) {
      struct gl_texture_image *teximage;
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 1, width, 1, 1, border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (texUnit->CurrentD[1]->Image[level]) {
         gl_free_texture_image( texUnit->CurrentD[1]->Image[level] );
      }

      /* make new texture from source image */
      if (pixels) {
         teximage = make_texture_image(ctx, internalformat, width, 1, 1,
                              border, format, type, pixels, &ctx->Unpack);
      }
      else {
         teximage = make_null_texture(ctx, (GLenum) internalformat,
                                      width, 1, 1, border);
      }

      /* install new texture image */
      texUnit->CurrentD[1]->Image[level] = teximage;
      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[1] );
      ctx->NewState |= NEW_TEXTURING;

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D,
                                  texUnit->CurrentD[1],
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 1, width, 1, 1, border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy1D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy1D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy1D->Image[level] );
         ctx->Texture.Proxy1D->Image[level]->IntFormat = (GLenum) internalformat;
         ctx->Texture.Proxy1D->Image[level]->Border = border;
         ctx->Texture.Proxy1D->Image[level]->Width = width;
         ctx->Texture.Proxy1D->Image[level]->Height = 1;
         ctx->Texture.Proxy1D->Image[level]->Depth = 1;
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
      return;
   }
}


void gl_TexImage2D( GLcontext *ctx, GLenum target, GLint level,
                    GLint internalformat,
                    GLsizei width, GLsizei height, GLint border,
                    GLenum format, GLenum type,
                    const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage2D");

   if (target==GL_TEXTURE_2D) {
      struct gl_texture_image *teximage;
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 2, width, height, 1, border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (texUnit->CurrentD[2]->Image[level]) {
         gl_free_texture_image( texUnit->CurrentD[2]->Image[level] );
      }

      /* make new texture from source image */
      if (pixels) {
         teximage = make_texture_image(ctx, internalformat, width, height, 1,
                                  border, format, type, pixels, &ctx->Unpack);
      }
      else {
         teximage = make_null_texture(ctx, (GLenum) internalformat,
                                      width, height, 1, border);
      }

      /* install new texture image */
      texUnit->CurrentD[2]->Image[level] = teximage;
      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[2] );
      ctx->NewState |= NEW_TEXTURING;

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D,
                                  texUnit->CurrentD[2],
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_2D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 2, width, height, 1, border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy2D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy2D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy2D->Image[level] );
         ctx->Texture.Proxy2D->Image[level]->IntFormat = (GLenum) internalformat;
         ctx->Texture.Proxy2D->Image[level]->Border = border;
         ctx->Texture.Proxy2D->Image[level]->Width = width;
         ctx->Texture.Proxy2D->Image[level]->Height = height;
         ctx->Texture.Proxy2D->Image[level]->Depth = 1;
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
      return;
   }
}



/*
 * Called by the API or display list executor.
 * Note that width and height include the border.
 */
void gl_TexImage3D( GLcontext *ctx, GLenum target, GLint level,
                    GLint internalformat,
                    GLsizei width, GLsizei height, GLsizei depth,
                    GLint border, GLenum format, GLenum type,
                    const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage3D");

   if (target==GL_TEXTURE_3D) {
      struct gl_texture_image *teximage;
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 3, width, height, depth,
                               border )) {
         /* error in texture image was detected */
         return;
      }

      /* free current texture image, if any */
      if (texUnit->CurrentD[3]->Image[level]) {
         gl_free_texture_image( texUnit->CurrentD[3]->Image[level] );
      }

      /* make new texture from source image */
      if (pixels) {
         teximage = make_texture_image(ctx, internalformat, width, height,
                       depth, border, format, type, pixels, &ctx->Unpack);
      }
      else {
         teximage = make_null_texture(ctx, (GLenum) internalformat,
                                      width, height, depth, border);
      }

      /* install new texture image */
      texUnit->CurrentD[3]->Image[level] = teximage;
      gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[3] );
      ctx->NewState |= NEW_TEXTURING;

      /* tell driver about change */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_3D_EXT,
                                  texUnit->CurrentD[3],
                                  level, internalformat, teximage );
      }
   }
   else if (target==GL_PROXY_TEXTURE_3D_EXT) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalformat,
                               format, type, 3, width, height, depth,
                               border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy3D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy3D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy3D->Image[level] );
         ctx->Texture.Proxy3D->Image[level]->IntFormat = (GLenum) internalformat;
         ctx->Texture.Proxy3D->Image[level]->Border = border;
         ctx->Texture.Proxy3D->Image[level]->Width = width;
         ctx->Texture.Proxy3D->Image[level]->Height = height;
         ctx->Texture.Proxy3D->Image[level]->Depth  = depth;
      }
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
      return;
   }
}



void gl_GetTexImage( GLcontext *ctx, GLenum target, GLint level, GLenum format,
                     GLenum type, GLvoid *pixels )
{
   const struct gl_texture_object *texObj;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexImage");

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glGetTexImage(level)" );
      return;
   }

   if (gl_sizeof_type(type) <= 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(type)" );
      return;
   }

   if (gl_components_in_format(format) <= 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(format)" );
      return;
   }

   if (!pixels)
      return;  /* XXX generate an error??? */

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentD[1];
         break;
      case GL_TEXTURE_2D:
         texObj = ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentD[2];
         break;
      case GL_TEXTURE_3D:
         texObj = ctx->Texture.Unit[ctx->Texture.CurrentUnit].CurrentD[3];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(target)" );
         return;
   }

   if (texObj->Image[level] && texObj->Image[level]->Data) {
      const struct gl_texture_image *texImage = texObj->Image[level];
      GLint width = texImage->Width;
      GLint height = texImage->Height;
      GLint row;

      for (row = 0; row < height; row++) {
         /* compute destination address in client memory */
         GLvoid *dest = gl_pixel_addr_in_image( &ctx->Unpack, pixels,
                                                width, height,
                                                format, type, 0, row, 0);

         assert(dest);
         if (texImage->Format == GL_RGBA) {
            const GLubyte *src = texImage->Data + row * width * 4 * sizeof(GLubyte);
            gl_pack_rgba_span( ctx, width, (void *) src, format, type, dest,
                               &ctx->Pack, GL_TRUE );
         }
         else {
            /* fetch RGBA row from texture image then pack it in client mem */
            GLubyte rgba[MAX_WIDTH][4];
            GLint i;
            const GLubyte *src;
            switch (texImage->Format) {
               case GL_ALPHA:
                  src = texImage->Data + row * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = 255;
                     rgba[i][GCOMP] = 255;
                     rgba[i][BCOMP] = 255;
                     rgba[i][ACOMP] = src[i];
                  }
                  break;
               case GL_LUMINANCE:
                  src = texImage->Data + row * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i];
                     rgba[i][GCOMP] = src[i];
                     rgba[i][BCOMP] = src[i];
                     rgba[i][ACOMP] = 255;
                   }
                  break;
               case GL_LUMINANCE_ALPHA:
                  src = texImage->Data + row * 2 * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i*2+0];
                     rgba[i][GCOMP] = src[i*2+0];
                     rgba[i][BCOMP] = src[i*2+0];
                     rgba[i][ACOMP] = src[i*2+1];
                  }
                  break;
               case GL_INTENSITY:
                  src = texImage->Data + row * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i];
                     rgba[i][GCOMP] = src[i];
                     rgba[i][BCOMP] = src[i];
                     rgba[i][ACOMP] = 255;
                  }
                  break;
               case GL_RGB:
                  src = texImage->Data + row * 3 * width * sizeof(GLubyte);
                  for (i = 0; i < width; i++) {
                     rgba[i][RCOMP] = src[i*3+0];
                     rgba[i][GCOMP] = src[i*3+1];
                     rgba[i][BCOMP] = src[i*3+2];
                     rgba[i][ACOMP] = 255;
                  }
                  break;
               case GL_RGBA:
                  /* this special case should have been handled above! */
                  gl_problem( ctx, "error 1 in gl_GetTexImage" );
                  break;
               case GL_COLOR_INDEX:
                  gl_problem( ctx, "GL_COLOR_INDEX not implemented in gl_GetTexImage" );
                  break;
               default:
                  gl_problem( ctx, "bad format in gl_GetTexImage" );
            }
            gl_pack_rgba_span( ctx, width, (const GLubyte (*)[4])rgba,
                               format, type, dest, &ctx->Pack, GL_TRUE );
         }
      }
   }
}



void gl_TexSubImage1D( GLcontext *ctx, GLenum target, GLint level,
                       GLint xoffset, GLsizei width,
                       GLenum format, GLenum type,
                       const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (subtexture_error_check(ctx, 1, target, level, xoffset, 0, 0,
                              width, 1, 1, format, type)) {
      /* error was detected */
      return;
   }

   destTex = texUnit->CurrentD[1]->Image[level];
   assert(destTex);

   if (width == 0 || !pixels)
      return;  /* no-op, not an error */


   /*
    * Replace the texture subimage
    */
   {
      const GLint texComponents = components_in_intformat(destTex->Format);
      const GLenum texFormat = destTex->Format;
      const GLint xoffsetb = xoffset + destTex->Border;
      GLubyte *dst = destTex->Data + xoffsetb * texComponents;
      if (texFormat == GL_COLOR_INDEX) {
         /* color index texture */
         const GLvoid *src = gl_pixel_addr_in_image(&ctx->Unpack, pixels,
                                  width, 1, format, type, 0, 0, 0);
         _mesa_unpack_index_span(ctx, width, GL_UNSIGNED_BYTE, dst,
                                    type, src, &ctx->Unpack, GL_TRUE);
      }
      else {
         /* color texture */
         const GLvoid *src = gl_pixel_addr_in_image(&ctx->Unpack, pixels,
                                  width, 1, format, type, 0, 0, 0);
         _mesa_unpack_ubyte_color_span(ctx, width, texFormat, dst,
                              format, type, src, &ctx->Unpack, GL_TRUE);
      }
   }

   gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[1] );

   /*
    * Inform device driver of texture image change.
    */
   if (ctx->Driver.TexSubImage) {
      (*ctx->Driver.TexSubImage)(ctx, GL_TEXTURE_1D, texUnit->CurrentD[1],
                                 level, xoffset, 0, width, 1,
                                 texUnit->CurrentD[1]->Image[level]->IntFormat,
                                 destTex );
   }
   else {
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)(ctx, GL_TEXTURE_1D, texUnit->CurrentD[1],
                                 level,
                                 texUnit->CurrentD[1]->Image[level]->IntFormat,
                                 destTex );
      }
   }
}


void gl_TexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
                       GLint xoffset, GLint yoffset,
                       GLsizei width, GLsizei height,
                       GLenum format, GLenum type,
                       const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (subtexture_error_check(ctx, 2, target, level, xoffset, yoffset, 0,
                              width, height, 1, format, type)) {
      /* error was detected */
      return;
   }

   destTex = texUnit->CurrentD[2]->Image[level];
   assert(destTex);

   if (width == 0 || height == 0 || !pixels)
      return;  /* no-op, not an error */


   /*
    * Replace the texture subimage
    */
   {
      const GLint texComponents = components_in_intformat(destTex->Format);
      const GLenum texFormat = destTex->Format;
      const GLint xoffsetb = xoffset + destTex->Border;
      const GLint yoffsetb = yoffset + destTex->Border;
      GLubyte *dst = destTex->Data
                   + (yoffsetb * destTex->Width + xoffsetb) * texComponents;
      if (texFormat == GL_COLOR_INDEX) {
         /* color index texture */
         const GLint stride = destTex->Width * sizeof(GLubyte);
         GLint row;
         for (row = 0; row < height; row++) {
            const GLvoid *src = gl_pixel_addr_in_image(&ctx->Unpack, pixels,
                                     width, height, format, type, 0, row, 0);
            _mesa_unpack_index_span(ctx, width, GL_UNSIGNED_BYTE, dst,
                                    type, src, &ctx->Unpack, GL_TRUE);
            dst += stride;
         }
      }
      else {
         /* color texture */
         const GLint stride = destTex->Width * texComponents * sizeof(GLubyte);
         GLint row;
         for (row = 0; row < height; row++) {
            const GLvoid *src = gl_pixel_addr_in_image(&ctx->Unpack, pixels,
                                     width, height, format, type, 0, row, 0);
            _mesa_unpack_ubyte_color_span(ctx, width, texFormat, dst,
                                 format, type, src, &ctx->Unpack, GL_TRUE);
            dst += stride;
         }
      }
   }

   gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[2] );

   /*
    * Inform device driver of texture image change.
    */
   if (ctx->Driver.TexSubImage) {
      (*ctx->Driver.TexSubImage)(ctx, GL_TEXTURE_2D, texUnit->CurrentD[2],
                                 level, xoffset, yoffset, width, height,
                                 texUnit->CurrentD[2]->Image[level]->IntFormat,
                                 destTex );
   }
   else {
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)(ctx, GL_TEXTURE_2D, texUnit->CurrentD[2],
                                 level,
                                 texUnit->CurrentD[2]->Image[level]->IntFormat,
                                 destTex );
      }
   }
}



void gl_TexSubImage3D( GLcontext *ctx, GLenum target, GLint level,
                       GLint xoffset, GLint yoffset, GLint zoffset,
                       GLsizei width, GLsizei height, GLsizei depth,
                       GLenum format, GLenum type,
                       const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *destTex;

   if (subtexture_error_check(ctx, 3, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, type)) {
      /* error was detected */
      return;
   }

   destTex = texUnit->CurrentD[3]->Image[level];
   assert(destTex);

   if (width == 0 || height == 0 || height == 0 || !pixels)
      return;  /* no-op, not an error */

   /*
    * Replace the texture subimage
    */
   {
      const GLint texComponents = components_in_intformat(destTex->Format);
      const GLenum texFormat = destTex->Format;
      const GLint xoffsetb = xoffset + destTex->Border;
      const GLint yoffsetb = yoffset + destTex->Border;
      const GLint zoffsetb = zoffset + destTex->Border;
      GLint dstRectArea = destTex->Width * destTex->Height;
      GLubyte *dst = destTex->Data 
            + (zoffsetb * dstRectArea +  yoffsetb * destTex->Width + xoffsetb)
            * texComponents;

      if (texFormat == GL_COLOR_INDEX) {
         /* color index texture */
         const GLint stride = destTex->Width * sizeof(GLubyte);
         GLint img, row;
         for (img = 0; img < depth; img++) {
            for (row = 0; row < height; row++) {
               const GLvoid *src = gl_pixel_addr_in_image(&ctx->Unpack, pixels,
                                    width, height, format, type, img, row, 0);
               _mesa_unpack_index_span(ctx, width, GL_UNSIGNED_BYTE, dst,
                                       type, src, &ctx->Unpack, GL_TRUE);
               dst += stride;
            }
         }
      }
      else {
         /* color texture */
         const GLint stride = destTex->Width * texComponents * sizeof(GLubyte);
         GLint img, row;
         for (img = 0; img < depth; img++) {
            for (row = 0; row < height; row++) {
               const GLvoid *src = gl_pixel_addr_in_image(&ctx->Unpack, pixels,
                                     width, height, format, type, img, row, 0);
               _mesa_unpack_ubyte_color_span(ctx, width, texFormat, dst,
                                    format, type, src, &ctx->Unpack, GL_TRUE);
               dst += stride;
            }
         }
      }
   }

   gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[1] );

   /*
    * Inform device driver of texture image change.
    */
   /* XXX todo */
}



/*
 * Read an RGBA image from the frame buffer.
 * This is used by glCopyTexSubImage[12]D().
 * Input:  ctx - the context
 *         x, y - lower left corner
 *         width, height - size of region to read
 * Return: pointer to block of GL_RGBA, GLubyte data.
 */
static GLubyte *
read_color_image( GLcontext *ctx, GLint x, GLint y,
                  GLsizei width, GLsizei height )
{
   GLint stride, i;
   GLubyte *image, *dst;

   image = MALLOC(width * height * 4 * sizeof(GLubyte));
   if (!image)
      return NULL;

   /* Select buffer to read from */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.DriverReadBuffer );

   dst = image;
   stride = width * 4 * sizeof(GLubyte);
   for (i = 0; i < height; i++) {
      gl_read_rgba_span( ctx, width, x, y + i, (GLubyte (*)[4]) dst );
      dst += stride;
   }

   /* Restore drawing buffer */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DriverDrawBuffer );

   return image;
}



void gl_CopyTexImage1D( GLcontext *ctx, GLenum target, GLint level,
                        GLenum internalFormat,
                        GLint x, GLint y,
                        GLsizei width, GLint border )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexImage1D");

   if (!copytexture_error_check(ctx, 1, target, level, internalFormat,
                               width, 1, border)) {
      GLubyte *image  = read_color_image( ctx, x, y, width, 1 );
      if (!image) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D" );
         return;
      }
      (*ctx->Exec.TexImage1D)( ctx, target, level, internalFormat, width,
                               border, GL_RGBA, GL_UNSIGNED_BYTE, image );
      FREE(image);
   }
}



void gl_CopyTexImage2D( GLcontext *ctx, GLenum target, GLint level,
                        GLenum internalFormat,
                        GLint x, GLint y, GLsizei width, GLsizei height,
                        GLint border )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexImage2D");

   if (!copytexture_error_check(ctx, 2, target, level, internalFormat,
                               width, height, border)) {
      GLubyte *image  = read_color_image( ctx, x, y, width, height );
      if (!image) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D" );
         return;
      }
      {
         struct gl_pixelstore_attrib save = ctx->Unpack;
         ctx->Unpack = defaultPacking;
         (ctx->Exec.TexImage2D)( ctx, target, level, internalFormat, width,
                         height, border, GL_RGBA, GL_UNSIGNED_BYTE, image );
         ctx->Unpack = save;  /* restore */
      }
      FREE(image);
   }
}



/*
 * Do the work of glCopyTexSubImage[123]D.
 */
static void
copy_tex_sub_image( GLcontext *ctx, struct gl_texture_image *dest,
                    GLint width, GLint height,
                    GLint srcx, GLint srcy,
                    GLint dstx, GLint dsty, GLint dstz )
{
   static struct gl_pixelstore_attrib packing = {
      1,            /* Alignment */
      0,            /* RowLength */
      0,            /* SkipPixels */
      0,            /* SkipRows */
      0,            /* ImageHeight */
      0,            /* SkipImages */
      GL_FALSE,     /* SwapBytes */
      GL_FALSE      /* LsbFirst */
   };

   GLint i;
   GLint format, components, rectarea;
   GLint texwidth, texheight, zoffset;

   /* dst[xyz] may be negative if we have a texture border! */
   dstx += dest->Border;
   dsty += dest->Border;
   dstz += dest->Border;
   texwidth = dest->Width;
   texheight = dest->Height;
   rectarea = texwidth * texheight;
   zoffset = dstz * rectarea; 
   format = dest->Format;
   components = components_in_intformat( format );

   /* Select buffer to read from */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Pixel.DriverReadBuffer );

   for (i = 0;i < height; i++) {
      GLubyte rgba[MAX_WIDTH][4];
      GLubyte *dst;
      gl_read_rgba_span( ctx, width, srcx, srcy + i, rgba );
      dst = dest->Data + ( zoffset + (dsty+i) * texwidth + dstx) * components;
      _mesa_unpack_ubyte_color_span(ctx, width, format, dst,
                                    GL_RGBA, GL_UNSIGNED_BYTE, rgba,
                                    &packing, GL_TRUE);
   }

   /* Restore drawing buffer */
   (void) (*ctx->Driver.SetBuffer)( ctx, ctx->Color.DriverDrawBuffer );
}




void gl_CopyTexSubImage1D( GLcontext *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint x, GLint y, GLsizei width )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage1D");

   if (!copytexsubimage_error_check(ctx, 1, target, level,
                    xoffset, 0, 0, width, 1)) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_image *teximage;
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      teximage = texUnit->CurrentD[1]->Image[level];
      assert(teximage);
      if (teximage->Data) {
         copy_tex_sub_image(ctx, teximage, width, 1, x, y, xoffset, 0, 0);
	 /* tell driver about the change */
	 if (ctx->Driver.TexImage) {
	   (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D,
                                    texUnit->CurrentD[1],
				    level, teximage->IntFormat, teximage );
	 }
      }
   }
}



void gl_CopyTexSubImage2D( GLcontext *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint yoffset,
                           GLint x, GLint y, GLsizei width, GLsizei height )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage2D");

   if (!copytexsubimage_error_check(ctx, 2, target, level,
                    xoffset, yoffset, 0, width, height)) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_image *teximage;
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      teximage = texUnit->CurrentD[2]->Image[level];
      assert(teximage);
      if (teximage->Data) {
         copy_tex_sub_image(ctx, teximage, width, height,
                            x, y, xoffset, yoffset, 0);
	 /* tell driver about the change */
	 if (ctx->Driver.TexImage) {
	   (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D,
                                    texUnit->CurrentD[2],
				    level, teximage->IntFormat, teximage );
	 }
      }
   }
}



void gl_CopyTexSubImage3D( GLcontext *ctx, GLenum target, GLint level,
                           GLint xoffset, GLint yoffset, GLint zoffset,
                           GLint x, GLint y, GLsizei width, GLsizei height )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage3D");

   if (!copytexsubimage_error_check(ctx, 3, target, level,
                    xoffset, yoffset, zoffset, width, height)) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_image *teximage;
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      teximage = texUnit->CurrentD[3]->Image[level];
      assert(teximage);
      if (teximage->Data) {
         copy_tex_sub_image(ctx, teximage, width, height, 
                            x, y, xoffset, yoffset, zoffset);
	 /* tell driver about the change */
	 if (ctx->Driver.TexImage) {
	   (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_3D,
                                    texUnit->CurrentD[3],
				    level, teximage->IntFormat, teximage );
	 }
      }
   }
}

