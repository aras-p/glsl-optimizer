/* $Id: teximage.c,v 1.24 2000/03/22 17:38:11 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#include "glheader.h"
#include "context.h"
#include "image.h"
#include "mem.h"
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


/*
 * Compute log base 2 of n.
 * If n isn't an exact power of two return -1.
 * If n<0 return -1.
 */
static int
logbase2( int n )
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
static GLint
decode_internal_format( GLint format )
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
static GLint
components_in_intformat( GLint format )
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



/*
 * Examine the texImage->Format field and set the Red, Green, Blue, etc
 * texel component sizes to default values.
 * These fields are set only here by core Mesa but device drivers may
 * overwritting these fields to indicate true texel resolution.
 */
static void
set_teximage_component_sizes( struct gl_texture_image *texImage )
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



/*
 * Return new gl_texture_image struct with all fields initialized to zero.
 */
struct gl_texture_image *
gl_alloc_texture_image( void )
{
   return CALLOC_STRUCT(gl_texture_image);
}



/*
 * Initialize most fields of a gl_texture_image struct.
 */
static void
init_texture_image( struct gl_texture_image *img,
                    GLsizei width, GLsizei height, GLsizei depth,
                    GLint border, GLenum internalFormat )
{
   ASSERT(img);
   ASSERT(!img->Data);
   img->Format = (GLenum) decode_internal_format(internalFormat);
   set_teximage_component_sizes( img );
   img->IntFormat = (GLenum) internalFormat;
   img->Border = border;
   img->Width = width;
   img->Height = height;
   img->Depth = depth;
   img->WidthLog2 = logbase2(width - 2 * border);
   if (height == 1)  /* 1-D texture */
      img->HeightLog2 = 0;
   else
      img->HeightLog2 = logbase2(height - 2 * border);
   if (depth == 1)   /* 2-D texture */
      img->DepthLog2 = 0;
   else
      img->DepthLog2 = logbase2(depth - 2 * border);
   img->Width2 = 1 << img->WidthLog2;
   img->Height2 = 1 << img->HeightLog2;
   img->Depth2 = 1 << img->DepthLog2;
   img->MaxLog2 = MAX2(img->WidthLog2, img->HeightLog2);
}



void
gl_free_texture_image( struct gl_texture_image *teximage )
{
   if (teximage->Data) {
      FREE( teximage->Data );
      teximage->Data = NULL;
   }
   FREE( teximage );
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
 * Called by glTexImage[123]D.  Fill in a texture image with data given
 * by the client.  All pixel transfer and unpack modes are handled here.
 * NOTE: All texture image parameters should have already been error checked.
 */
static void
make_texture_image( GLcontext *ctx,
                    struct gl_texture_image *texImage,
                    GLenum srcFormat, GLenum srcType, const GLvoid *pixels,
                    const struct gl_pixelstore_attrib *unpacking)
{
   GLint components, numPixels;
   GLint internalFormat, width, height, depth, border;

   ASSERT(ctx);
   ASSERT(texImage);
   ASSERT(!texImage->Data);
   ASSERT(pixels);
   ASSERT(unpacking);

   internalFormat = texImage->IntFormat;
   width = texImage->Width;
   height = texImage->Height;
   depth = texImage->Depth;
   border = texImage->Border;
   components = components_in_intformat(internalFormat);

   ASSERT(width > 0);
   ASSERT(height > 0);
   ASSERT(depth > 0);
   ASSERT(border == 0 || border == 1);
   ASSERT(pixels);
   ASSERT(unpacking);
   ASSERT(components);

   numPixels = width * height * depth;

   texImage->Data = (GLubyte *) MALLOC(numPixels * components + EXTRA_BYTE);
   if (!texImage->Data)
      return;      /* out of memory */

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

      if (srcFormat == internalFormat ||
          (srcFormat == GL_LUMINANCE && internalFormat == 1) ||
          (srcFormat == GL_LUMINANCE_ALPHA && internalFormat == 2) ||
          (srcFormat == GL_RGB && internalFormat == 3) ||
          (srcFormat == GL_RGBA && internalFormat == 4)) {
         /* This will cover the common GL_RGB, GL_RGBA, GL_ALPHA,
          * GL_LUMINANCE_ALPHA, etc. texture formats.
          */
         const GLubyte *src = (const GLubyte *) _mesa_image_address(
            unpacking, pixels, width, height, srcFormat, srcType, 0, 0, 0);
         const GLint srcStride = _mesa_image_row_stride(unpacking, width,
                                                        srcFormat, srcType);
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
         return;  /* all done */
      }
      else if (srcFormat == GL_RGBA && internalFormat == GL_RGB) {
         /* commonly used by Quake */
         const GLubyte *src = (const GLubyte *) _mesa_image_address(
            unpacking, pixels, width, height, srcFormat, srcType, 0, 0, 0);
         const GLint srcStride = _mesa_image_row_stride(unpacking, width,
                                                        srcFormat, srcType);
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
         return;  /* all done */
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
            const GLvoid *source = _mesa_image_address(unpacking,
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
            const GLvoid *source = _mesa_image_address(unpacking,
                   pixels, width, height, srcFormat, srcType, img, row, 0);
            _mesa_unpack_ubyte_color_span(ctx, width, dstFormat, dest,
                   srcFormat, srcType, source, unpacking, GL_TRUE);
            dest += destBytesPerRow;
         }
      }
   }
}



/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.  This function creates an empty image for the given texture image.
 */
static void
make_null_texture( struct gl_texture_image *texImage )
{
   GLint components;
   GLint numPixels;

   ASSERT(texImage);
   ASSERT(!texImage->Data);

   components = components_in_intformat(texImage->IntFormat);
   numPixels = texImage->Width * texImage->Height * texImage->Depth;

   texImage->Data = (GLubyte *) MALLOC( numPixels * components + EXTRA_BYTE );

   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (texImage->Data) {
      static const char message[8][32] = {
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
      for (i = 0; i < texImage->Height; i++) {
         GLint srcRow = 7 - i % 8;
         for (j = 0; j < texImage->Width; j++) {
            GLint srcCol = j % 32;
            GLint texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
            for (k=0;k<components;k++) {
               *imgPtr++ = (GLubyte) texel;
            }
         }
      }
   }
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

   if (!_mesa_is_legal_format_and_type( format, type )) {
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
subtexture_error_check( GLcontext *ctx, GLuint dimensions,
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

   if (!_mesa_is_legal_format_and_type(format, type)) {
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
copytexture_error_check( GLcontext *ctx, GLuint dimensions,
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
copytexsubimage_error_check( GLcontext *ctx, GLuint dimensions,
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
void
_mesa_TexImage1D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLint border, GLenum format,
                  GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage1D");

   if (target==GL_TEXTURE_1D) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check( ctx, target, level, internalFormat,
                               format, type, 1, width, 1, 1, border )) {
         return;   /* error in texture image was detected */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = texUnit->CurrentD[1];
      texImage = texObj->Image[level];

      if (!texImage) {
         texImage = gl_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
            return;
         }
      }
      else if (texImage->Data) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }

      /* setup the teximage struct's fields */
      init_texture_image(texImage, width, 1, 1, border, internalFormat);

      /* process the texture image */
      if (pixels) {
         GLboolean retain = GL_TRUE;
         GLboolean success = GL_FALSE;
         if (!ctx->Pixel.MapColorFlag && !ctx->Pixel.ScaleOrBiasRGBA
             && ctx->Driver.TexImage1D) {
            /* let device driver try to use raw image */
            success = (*ctx->Driver.TexImage1D)( ctx, target, level, format,
                                                 type, pixels, &ctx->Unpack,
                                                 texObj, texImage, &retain);
         }
         if (retain || !success) {
            /* make internal copy of the texture image */
            make_texture_image(ctx, texImage, format, type,
                               pixels, &ctx->Unpack);
            if (!success && ctx->Driver.TexImage1D) {
               /* let device driver try to use unpacked image */
               (*ctx->Driver.TexImage1D)( ctx, target, level, texImage->Format,
                                          GL_UNSIGNED_BYTE, texImage->Data,
                                          &_mesa_native_packing,
                                          texObj, texImage, &retain);
            }
         }
         if (!retain && texImage->Data) {
            FREE(texImage->Data);
            texImage->Data = NULL;
         }
      }
      else {
         make_null_texture(texImage);
         if (ctx->Driver.TexImage1D) {
            GLboolean retain;
            (*ctx->Driver.TexImage1D)( ctx, target, level, texImage->Format,
                                       GL_UNSIGNED_BYTE, texImage->Data,
                                       &_mesa_native_packing,
                                       texObj, texImage, &retain);
         }
      }

      /* state update */
      gl_put_texobj_on_dirty_list( ctx, texObj );
      ctx->NewState |= NEW_TEXTURING;
   }
   else if (target==GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalFormat,
                               format, type, 1, width, 1, 1, border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy1D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy1D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy1D->Image[level] );
         ctx->Texture.Proxy1D->Image[level]->IntFormat = (GLenum) internalFormat;
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


void
_mesa_TexImage2D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage2D");

   if (target==GL_TEXTURE_2D) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check( ctx, target, level, internalFormat,
                               format, type, 2, width, height, 1, border )) {
         return;   /* error in texture image was detected */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = texUnit->CurrentD[2];
      texImage = texObj->Image[level];

      if (!texImage) {
         texImage = gl_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
         }
      }
      else if (texImage->Data) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }

      /* setup the teximage struct's fields */
      init_texture_image(texImage, width, height, 1, border, internalFormat);

      /* process the texture image */
      if (pixels) {
         GLboolean retain = GL_TRUE;
         GLboolean success = GL_FALSE;
         if (!ctx->Pixel.MapColorFlag && !ctx->Pixel.ScaleOrBiasRGBA
             && ctx->Driver.TexImage2D) {
            /* let device driver try to use raw image */
            success = (*ctx->Driver.TexImage2D)( ctx, target, level, format,
                                                 type, pixels, &ctx->Unpack,
                                                 texObj, texImage, &retain);
         }
         if (retain || !success) {
            /* make internal copy of the texture image */
            make_texture_image(ctx, texImage, format, type,
                               pixels, &ctx->Unpack);
            if (!success && ctx->Driver.TexImage2D) {
               /* let device driver try to use unpacked image */
               (*ctx->Driver.TexImage2D)( ctx, target, level, texImage->Format,
                                          GL_UNSIGNED_BYTE, texImage->Data,
                                          &_mesa_native_packing,
                                          texObj, texImage, &retain);
            }
         }
         if (!retain && texImage->Data) {
            FREE(texImage->Data);
            texImage->Data = NULL;
         }
      }
      else {
         make_null_texture(texImage);
         if (ctx->Driver.TexImage2D) {
            GLboolean retain;
            (*ctx->Driver.TexImage2D)( ctx, target, level, texImage->Format,
                                       GL_UNSIGNED_BYTE, texImage->Data,
                                       &_mesa_native_packing,
                                       texObj, texImage, &retain);
         }
      }

#define OLD_DD_TEXTURE
#ifdef OLD_DD_TEXTURE
      /* XXX this will be removed in the future */
      if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)( ctx, target, texObj, level, internalFormat,
                                  texImage );
      }
#endif

      /* state update */
      gl_put_texobj_on_dirty_list( ctx, texObj );
      ctx->NewState |= NEW_TEXTURING;
   }
   else if (target==GL_PROXY_TEXTURE_2D) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalFormat,
                               format, type, 2, width, height, 1, border )) {
         if (level>=0 && level<ctx->Const.MaxTextureLevels) {
            MEMSET( ctx->Texture.Proxy2D->Image[level], 0,
                    sizeof(struct gl_texture_image) );
         }
      }
      else {
         ctx->Texture.Proxy2D->Image[level]->Format = (GLenum) format;
         set_teximage_component_sizes( ctx->Texture.Proxy2D->Image[level] );
         ctx->Texture.Proxy2D->Image[level]->IntFormat = (GLenum) internalFormat;
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
void
_mesa_TexImage3D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLsizei depth,
                  GLint border, GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glTexImage3D");

   if (target==GL_TEXTURE_3D_EXT) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;
      if (texture_error_check( ctx, target, level, internalFormat,
                               format, type, 3, width, height, depth,
                               border )) {
         return;   /* error in texture image was detected */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = texUnit->CurrentD[3];
      texImage = texObj->Image[level];

      if (!texImage) {
         texImage = gl_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
            return;
         }
      }
      else if (texImage->Data) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }

      /* setup the teximage struct's fields */
      init_texture_image(texImage, width, height, depth,
                         border, internalFormat);

      /* process the texture image */
      if (pixels) {
         GLboolean retain = GL_TRUE;
         GLboolean success = GL_FALSE;
         if (!ctx->Pixel.MapColorFlag && !ctx->Pixel.ScaleOrBiasRGBA
             && ctx->Driver.TexImage3D) {
            /* let device driver try to use raw image */
            success = (*ctx->Driver.TexImage3D)( ctx, target, level, format,
                                                 type, pixels, &ctx->Unpack,
                                                 texObj, texImage, &retain);
         }
         if (retain || !success) {
            /* make internal copy of the texture image */
            make_texture_image(ctx, texImage, format, type,
                               pixels, &ctx->Unpack);
            if (!success && ctx->Driver.TexImage3D) {
               /* let device driver try to use unpacked image */
               (*ctx->Driver.TexImage3D)( ctx, target, level, texImage->Format,
                                          GL_UNSIGNED_BYTE, texImage->Data,
                                          &_mesa_native_packing,
                                          texObj, texImage, &retain);
            }
         }
         if (!retain && texImage->Data) {
            FREE(texImage->Data);
            texImage->Data = NULL;
         }
      }
      else {
         make_null_texture(texImage);
         if (ctx->Driver.TexImage3D) {
            GLboolean retain;
            (*ctx->Driver.TexImage3D)( ctx, target, level, texImage->Format,
                                       GL_UNSIGNED_BYTE, texImage->Data,
                                       &_mesa_native_packing,
                                       texObj, texImage, &retain);
         }
      }

      /* state update */
      gl_put_texobj_on_dirty_list( ctx, texObj );
      ctx->NewState |= NEW_TEXTURING;
   }
   else if (target==GL_PROXY_TEXTURE_3D_EXT) {
      /* Proxy texture: check for errors and update proxy state */
      if (texture_error_check( ctx, target, level, internalFormat,
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
         ctx->Texture.Proxy3D->Image[level]->IntFormat = (GLenum) internalFormat;
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


void
_mesa_TexImage3DEXT( GLenum target, GLint level, GLenum internalFormat,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLint border, GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   _mesa_TexImage3D(target, level, (GLint) internalFormat, width, height,
                    depth, border, format, type, pixels);
}


/*
 * Fetch a texture image from the device driver.
 * Store the results in the given texture object at the given mipmap level.
 */
static void
get_teximage_from_driver( GLcontext *ctx, GLenum target, GLint level,
                          const struct gl_texture_object *texObj )
{
   GLvoid *image;
   GLenum imgFormat, imgType;
   GLboolean freeImage;
   struct gl_texture_image *texImage;
   GLint destComponents, numPixels, srcBytesPerTexel;

   if (!ctx->Driver.GetTexImage)
      return;

   image = (*ctx->Driver.GetTexImage)( ctx, target, level,
                                       &imgFormat, &imgType, &freeImage);
   if (!image)
      return;

   texImage = texObj->Image[level];
   ASSERT(texImage);
   if (!texImage)
      return;

   destComponents = components_in_intformat(texImage->Format);
   assert(destComponents > 0);
   numPixels = texImage->Width * texImage->Height * texImage->Depth;
   assert(numPixels > 0);
   srcBytesPerTexel = _mesa_bytes_per_pixel(imgFormat, imgType);
   assert(srcBytesPerTexel > 0);

   if (!texImage->Data) {
      /* Allocate memory for the texture image data */
      texImage->Data = (GLubyte *) MALLOC(numPixels * destComponents + EXTRA_BYTE);
   }

   if (imgFormat == texImage->Format && imgType == GL_UNSIGNED_BYTE) {
      /* We got lucky!  The driver's format and type match Mesa's format. */
      if (texImage->Data) {
         MEMCPY(texImage->Data, image, numPixels * destComponents);
      }
   }
   else {
      /* Convert the texture image from the driver's format to Mesa's
       * internal format.
       */
      const GLint width = texImage->Width;
      const GLint height = texImage->Height;
      const GLint depth = texImage->Depth;
      const GLint destBytesPerRow = width * destComponents * sizeof(GLchan);
      const GLint srcBytesPerRow = width * srcBytesPerTexel;
      const GLenum dstType = GL_UNSIGNED_BYTE;
      const GLenum dstFormat = texImage->Format;
      const GLubyte *srcPtr = (const GLubyte *) image;
      GLubyte *destPtr = texImage->Data;

      if (texImage->Format == GL_COLOR_INDEX) {
         /* color index texture */
         GLint img, row;
         assert(imgFormat == GL_COLOR_INDEX);
         for (img = 0; img < depth; img++) {
            for (row = 0; row < height; row++) {
               _mesa_unpack_index_span(ctx, width, dstType, destPtr,
                             imgType, srcPtr, &_mesa_native_packing, GL_FALSE);
               destPtr += destBytesPerRow;
               srcPtr += srcBytesPerRow;
            }
         }
      }
      else {
         /* color texture */
         GLint img, row;
         for (img = 0; img < depth; img++) {
            for (row = 0; row < height; row++) {
               _mesa_unpack_ubyte_color_span(ctx, width, dstFormat, destPtr,
                  imgFormat, imgType, srcPtr, &_mesa_native_packing, GL_FALSE);
               destPtr += destBytesPerRow;
               srcPtr += srcBytesPerRow;
            }
         }
      }
   }

   if (freeImage)
      FREE(image);
}


void
_mesa_GetTexImage( GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   const struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLboolean discardImage;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glGetTexImage");

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glGetTexImage(level)" );
      return;
   }

   if (_mesa_sizeof_type(type) <= 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(type)" );
      return;
   }

   if (_mesa_components_in_format(format) <= 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexImage(format)" );
      return;
   }

   if (!pixels)
      return;

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

   texImage = texObj->Image[level];
   if (!texImage) {
      /* invalid mipmap level */
      return;
   }

   if (!texImage->Data) {
      /* try to get the texture image from the device driver */
      get_teximage_from_driver(ctx, target, level, texObj);
      discardImage = GL_TRUE;
   }
   else {
      discardImage = GL_FALSE;
   }

   if (texImage->Data) {
      GLint width = texImage->Width;
      GLint height = texImage->Height;
      GLint row;

      for (row = 0; row < height; row++) {
         /* compute destination address in client memory */
         GLvoid *dest = _mesa_image_address( &ctx->Unpack, pixels,
                                                width, height,
                                                format, type, 0, row, 0);

         assert(dest);
         if (texImage->Format == GL_RGBA) {
            const GLubyte *src = texImage->Data + row * width * 4 * sizeof(GLubyte);
            _mesa_pack_rgba_span( ctx, width, (CONST GLubyte (*)[4]) src,
                                  format, type, dest, &ctx->Pack, GL_TRUE );
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
            _mesa_pack_rgba_span( ctx, width, (const GLubyte (*)[4])rgba,
                                  format, type, dest, &ctx->Pack, GL_TRUE );
         }
      }

      /* if we got the teximage from the device driver we'll discard it now */
      if (discardImage) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }
   }
}



void
_mesa_TexSubImage1D( GLenum target, GLint level,
                     GLint xoffset, GLsizei width,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLboolean success = GL_FALSE;

   if (subtexture_error_check(ctx, 1, target, level, xoffset, 0, 0,
                              width, 1, 1, format, type)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = texUnit->CurrentD[1];
   texImage = texObj->Image[level];
   assert(texImage);

   if (width == 0 || !pixels)
      return;  /* no-op, not an error */


   if (!ctx->Pixel.MapColorFlag && !ctx->Pixel.ScaleOrBiasRGBA
       && ctx->Driver.TexSubImage1D) {
      success = (*ctx->Driver.TexSubImage1D)( ctx, target, level, xoffset,
                                              width, format, type, pixels,
                                              &ctx->Unpack, texObj, texImage );
   }
   if (!success) {
      /* XXX if Driver.TexSubImage1D, unpack image and try again? */

      const GLint texComponents = components_in_intformat(texImage->Format);
      const GLenum texFormat = texImage->Format;
      const GLint xoffsetb = xoffset + texImage->Border;
      GLboolean retain = GL_TRUE;
      if (!texImage->Data) {
         get_teximage_from_driver( ctx, target, level, texObj );
         if (!texImage->Data) {
            make_null_texture(texImage);
         }
         if (!texImage->Data)
            return;  /* we're really out of luck! */
      }

      if (texFormat == GL_COLOR_INDEX) {
         /* color index texture */
         GLubyte *dst = texImage->Data + xoffsetb * texComponents;
         const GLvoid *src = _mesa_image_address(&ctx->Unpack, pixels, width,
                                                 1, format, type, 0, 0, 0);
         _mesa_unpack_index_span(ctx, width, GL_UNSIGNED_BYTE, dst,
                                 type, src, &ctx->Unpack, GL_TRUE);
      }
      else {
         /* color texture */
         GLubyte *dst = texImage->Data + xoffsetb * texComponents;
         const GLvoid *src = _mesa_image_address(&ctx->Unpack, pixels, width,
                                                 1, format, type, 0, 0, 0);
         _mesa_unpack_ubyte_color_span(ctx, width, texFormat, dst, format,
                                       type, src, &ctx->Unpack, GL_TRUE);
      }

      if (ctx->Driver.TexImage1D) {
         (*ctx->Driver.TexImage1D)( ctx, target, level, texImage->Format,
                                    GL_UNSIGNED_BYTE, texImage->Data,
                                    &_mesa_native_packing, texObj, texImage,
                                    &retain );
      }

      if (!retain && texImage->Data) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }
   }

   /*gl_put_texobj_on_dirty_list( ctx, texUnit->CurrentD[1] );*/
}


void
_mesa_TexSubImage2D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLboolean success = GL_FALSE;

   if (subtexture_error_check(ctx, 2, target, level, xoffset, yoffset, 0,
                              width, height, 1, format, type)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = texUnit->CurrentD[2];
   texImage = texObj->Image[level];
   assert(texImage);

   if (width == 0 || height == 0 || !pixels)
      return;  /* no-op, not an error */

   if (!ctx->Pixel.MapColorFlag && !ctx->Pixel.ScaleOrBiasRGBA
       && ctx->Driver.TexSubImage2D) {
      success = (*ctx->Driver.TexSubImage2D)( ctx, target, level, xoffset,
                                     yoffset, width, height, format, type,
                                     pixels, &ctx->Unpack, texObj, texImage );
   }
   if (!success) {
      /* XXX if Driver.TexSubImage2D, unpack image and try again? */

      const GLint texComponents = components_in_intformat(texImage->Format);
      const GLenum texFormat = texImage->Format;
      const GLint xoffsetb = xoffset + texImage->Border;
      const GLint yoffsetb = yoffset + texImage->Border;
      const GLint srcStride = _mesa_image_row_stride(&ctx->Unpack, width,
                                                     format, type);
      const GLint dstStride = texImage->Width * texComponents *sizeof(GLubyte);
      GLboolean retain = GL_TRUE;

      if (!texImage->Data) {
         get_teximage_from_driver( ctx, target, level, texObj );
         if (!texImage->Data) {
            make_null_texture(texImage);
         }
         if (!texImage->Data)
            return;  /* we're really out of luck! */
      }

      if (texFormat == GL_COLOR_INDEX) {
         /* color index texture */
         GLubyte *dst = texImage->Data
                    + (yoffsetb * texImage->Width + xoffsetb) * texComponents;
         const GLubyte *src = _mesa_image_address(&ctx->Unpack, pixels,
                                     width, height, format, type, 0, 0, 0);
         GLint row;
         for (row = 0; row < height; row++) {
            _mesa_unpack_index_span(ctx, width, GL_UNSIGNED_BYTE, dst, type,
                                 (const GLvoid *) src, &ctx->Unpack, GL_TRUE);
            src += srcStride;
            dst += dstStride;
         }
      }
      else {
         /* color texture */
         GLubyte *dst = texImage->Data
                    + (yoffsetb * texImage->Width + xoffsetb) * texComponents;
         const GLubyte *src = _mesa_image_address(&ctx->Unpack, pixels,
                                     width, height, format, type, 0, 0, 0);
         GLint row;
         for (row = 0; row < height; row++) {
            _mesa_unpack_ubyte_color_span(ctx, width, texFormat, dst, format,
                           type, (const GLvoid *) src, &ctx->Unpack, GL_TRUE);
            src += srcStride;
            dst += dstStride;
         }
      }

      if (ctx->Driver.TexImage2D) {
         (*ctx->Driver.TexImage2D)(ctx, target, level, texImage->Format,
                                   GL_UNSIGNED_BYTE, texImage->Data,
                                   &_mesa_native_packing, texObj, texImage,
                                   &retain);
      }

      if (!retain && texImage->Data) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }

#ifdef OLD_DD_TEXTURE
      /* XXX this will be removed in the future */
      if (ctx->Driver.TexSubImage) {
         (*ctx->Driver.TexSubImage)(ctx, target, texObj, level,
                                    xoffset, yoffset, width, height,
                                    texImage->IntFormat, texImage);
      }
      else if (ctx->Driver.TexImage) {
         (*ctx->Driver.TexImage)(ctx, GL_TEXTURE_1D, texObj,
                                 level, texImage->IntFormat, texImage );
      }
#endif
   }
}



void
_mesa_TexSubImage3D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLboolean success = GL_FALSE;

   if (subtexture_error_check(ctx, 3, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, type)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = texUnit->CurrentD[3];
   texImage = texObj->Image[level];
   assert(texImage);

   if (width == 0 || height == 0 || height == 0 || !pixels)
      return;  /* no-op, not an error */

   if (!ctx->Pixel.MapColorFlag && !ctx->Pixel.ScaleOrBiasRGBA
       && ctx->Driver.TexSubImage3D) {
      success = (*ctx->Driver.TexSubImage3D)( ctx, target, level, xoffset,
                                yoffset, zoffset, width, height, depth, format,
                                type, pixels, &ctx->Unpack, texObj, texImage );
   }
   if (!success) {
      /* XXX if Driver.TexSubImage3D, unpack image and try again? */

      const GLint texComponents = components_in_intformat(texImage->Format);
      const GLenum texFormat = texImage->Format;
      const GLint xoffsetb = xoffset + texImage->Border;
      const GLint yoffsetb = yoffset + texImage->Border;
      const GLint zoffsetb = zoffset + texImage->Border;
      const GLint texWidth = texImage->Width;
      const GLint dstRectArea = texWidth * texImage->Height;
      const GLint srcStride = _mesa_image_row_stride(&ctx->Unpack,
                                                     width, format, type);
      const GLint dstStride = texWidth * texComponents * sizeof(GLubyte);
      GLboolean retain = GL_TRUE;

      if (texFormat == GL_COLOR_INDEX) {
         /* color index texture */
         GLint img, row;
         for (img = 0; img < depth; img++) {
            const GLubyte *src = _mesa_image_address(&ctx->Unpack, pixels,
                                    width, height, format, type, img, 0, 0);
            GLubyte *dst = texImage->Data + ((zoffsetb + img) * dstRectArea
                     + yoffsetb * texWidth + xoffsetb) * texComponents;
            for (row = 0; row < height; row++) {
               _mesa_unpack_index_span(ctx, width, GL_UNSIGNED_BYTE, dst,
                         type, (const GLvoid *) src, &ctx->Unpack, GL_TRUE);
               src += srcStride;
               dst += dstStride;
            }
         }
      }
      else {
         /* color texture */
         GLint img, row;
         for (img = 0; img < depth; img++) {
            const GLubyte *src = _mesa_image_address(&ctx->Unpack, pixels,
                                     width, height, format, type, img, 0, 0);
            GLubyte *dst = texImage->Data + ((zoffsetb + img) * dstRectArea
                     + yoffsetb * texWidth + xoffsetb) * texComponents;
            for (row = 0; row < height; row++) {
               _mesa_unpack_ubyte_color_span(ctx, width, texFormat, dst,
                   format, type, (const GLvoid *) src, &ctx->Unpack, GL_TRUE);
               src += srcStride;
               dst += dstStride;
            }
         }
      }

      if (ctx->Driver.TexImage3D) {
         (*ctx->Driver.TexImage3D)(ctx, target, level, texImage->Format,
                                   GL_UNSIGNED_BYTE, texImage->Data,
                                   &_mesa_native_packing, texObj, texImage,
                                   &retain);
      }

      if (!retain && texImage->Data) {
         FREE(texImage->Data);
         texImage->Data = NULL;
      }
   }
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

   image = (GLubyte *) MALLOC(width * height * 4 * sizeof(GLubyte));
   if (!image)
      return NULL;

   /* Select buffer to read from */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   dst = image;
   stride = width * 4 * sizeof(GLubyte);
   for (i = 0; i < height; i++) {
      gl_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y + i,
                         (GLubyte (*)[4]) dst );
      dst += stride;
   }

   /* Read from draw buffer (the default) */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );

   return image;
}



void
_mesa_CopyTexImage1D( GLenum target, GLint level,
                      GLenum internalFormat,
                      GLint x, GLint y,
                      GLsizei width, GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexImage1D");

   if (copytexture_error_check(ctx, 1, target, level, internalFormat,
                               width, 1, border))
      return;

   if (ctx->Pixel.MapColorFlag || ctx->Pixel.ScaleOrBiasRGBA
       || !ctx->Driver.CopyTexImage1D 
       || !(*ctx->Driver.CopyTexImage1D)(ctx, target, level,
                         internalFormat, x, y, width, border))
   {
      GLubyte *image  = read_color_image( ctx, x, y, width, 1 );
      if (!image) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D" );
         return;
      }
      (*ctx->Exec->TexImage1D)( target, level, internalFormat, width,
                                border, GL_RGBA, GL_UNSIGNED_BYTE, image );
      FREE(image);
   }
}



void
_mesa_CopyTexImage2D( GLenum target, GLint level, GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexImage2D");

   if (copytexture_error_check(ctx, 2, target, level, internalFormat,
                               width, height, border))
      return;

   if (ctx->Pixel.MapColorFlag || ctx->Pixel.ScaleOrBiasRGBA
       || !ctx->Driver.CopyTexImage2D
       || !(*ctx->Driver.CopyTexImage2D)(ctx, target, level,
                         internalFormat, x, y, width, height, border))
   {
      GLubyte *image  = read_color_image( ctx, x, y, width, height );
      if (!image) {
         gl_error( ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D" );
         return;
      }
      (ctx->Exec->TexImage2D)( target, level, internalFormat, width,
                      height, border, GL_RGBA, GL_UNSIGNED_BYTE, image );
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
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   for (i = 0;i < height; i++) {
      GLubyte rgba[MAX_WIDTH][4];
      GLubyte *dst;
      gl_read_rgba_span( ctx, ctx->ReadBuffer, width, srcx, srcy + i, rgba );
      dst = dest->Data + ( zoffset + (dsty+i) * texwidth + dstx) * components;
      _mesa_unpack_ubyte_color_span(ctx, width, format, dst,
                                    GL_RGBA, GL_UNSIGNED_BYTE, rgba,
                                    &_mesa_native_packing, GL_TRUE);
   }

   /* Read from draw buffer (the default) */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );
}




void
_mesa_CopyTexSubImage1D( GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage1D");

   if (copytexsubimage_error_check(ctx, 1, target, level,
                                   xoffset, 0, 0, width, 1))
      return;

   if (ctx->Pixel.MapColorFlag || ctx->Pixel.ScaleOrBiasRGBA
       || !ctx->Driver.CopyTexSubImage1D
       || !(*ctx->Driver.CopyTexSubImage1D)(ctx, target, level,
                                            xoffset, x, y, width)) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_image *teximage;
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      teximage = texUnit->CurrentD[1]->Image[level];
      assert(teximage);
      if (teximage->Data) {
         copy_tex_sub_image(ctx, teximage, width, 1, x, y, xoffset, 0, 0);
         /* tell driver about the change */
         /* XXX call Driver.TexSubImage instead? */
         if (ctx->Driver.TexImage) {
            (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_1D,
                                     texUnit->CurrentD[1],
                                     level, teximage->IntFormat, teximage );
         }
      }
   }
}



void
_mesa_CopyTexSubImage2D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage2D");

   if (copytexsubimage_error_check(ctx, 2, target, level,
                                   xoffset, yoffset, 0, width, height))
      return;

   if (ctx->Pixel.MapColorFlag || ctx->Pixel.ScaleOrBiasRGBA
       || !ctx->Driver.CopyTexSubImage2D
       || !(*ctx->Driver.CopyTexSubImage2D)(ctx, target, level,
                                xoffset, yoffset, x, y, width, height )) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_image *teximage;
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      teximage = texUnit->CurrentD[2]->Image[level];
      assert(teximage);
      if (teximage->Data) {
         copy_tex_sub_image(ctx, teximage, width, height,
                            x, y, xoffset, yoffset, 0);
         /* tell driver about the change */
         /* XXX call Driver.TexSubImage instead? */
         if (ctx->Driver.TexImage) {
            (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_2D,
                                     texUnit->CurrentD[2],
                                     level, teximage->IntFormat, teximage );
	 }
      }
   }
}



void
_mesa_CopyTexSubImage3D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyTexSubImage3D");

   if (copytexsubimage_error_check(ctx, 3, target, level,
                    xoffset, yoffset, zoffset, width, height))
      return;

   if (ctx->Pixel.MapColorFlag || ctx->Pixel.ScaleOrBiasRGBA
       || !ctx->Driver.CopyTexSubImage3D
       || !(*ctx->Driver.CopyTexSubImage3D)(ctx, target, level,
                     xoffset, yoffset, zoffset, x, y, width, height )) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_image *teximage;
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      teximage = texUnit->CurrentD[3]->Image[level];
      assert(teximage);
      if (teximage->Data) {
         copy_tex_sub_image(ctx, teximage, width, height, 
                            x, y, xoffset, yoffset, zoffset);
         /* tell driver about the change */
         /* XXX call Driver.TexSubImage instead? */
         if (ctx->Driver.TexImage) {
            (*ctx->Driver.TexImage)( ctx, GL_TEXTURE_3D,
                                     texUnit->CurrentD[3],
                                     level, teximage->IntFormat, teximage );
         }
      }
   }
}
