/* $Id: teximage.c,v 1.108 2002/04/23 16:44:46 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
#include "convolve.h"
#include "image.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "texformat.h"
#include "teximage.h"
#include "texstate.h"
#include "texstore.h"
#include "mtypes.h"
#include "swrast/s_span.h" /* XXX SWRAST hack */
#endif


/*
 * NOTES:
 *
 * Mesa's native texture datatype is GLchan.  Native formats are
 * GL_ALPHA, GL_LUMINANCE, GL_LUMANCE_ALPHA, GL_INTENSITY, GL_RGB, GL_RGBA,
 * and GL_COLOR_INDEX.
 * Device drivers are free to implement any internal format they want.
 */


#ifdef DEBUG
static void PrintTexture(const struct gl_texture_image *img)
{
#if CHAN_TYPE == GL_FLOAT
   _mesa_problem(NULL, "PrintTexture doesn't support float channels");
#else
   GLuint i, j, c;
   const GLchan *data = (const GLchan *) img->Data;

   if (!data) {
      printf("No texture data\n");
      return;
   }

   switch (img->Format) {
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_INTENSITY:
      case GL_COLOR_INDEX:
         c = 1;
         break;
      case GL_LUMINANCE_ALPHA:
         c = 2;
         break;
      case GL_RGB:
         c = 3;
         break;
      case GL_RGBA:
         c = 4;
         break;
      default:
         _mesa_problem(NULL, "error in PrintTexture\n");
         return;
   }

   for (i = 0; i < img->Height; i++) {
      for (j = 0; j < img->Width; j++) {
         if (c==1)
            printf("%02x  ", data[0]);
         else if (c==2)
            printf("%02x%02x  ", data[0], data[1]);
         else if (c==3)
            printf("%02x%02x%02x  ", data[0], data[1], data[2]);
         else if (c==4)
            printf("%02x%02x%02x%02x  ", data[0], data[1], data[2], data[3]);
         data += c;
      }
      printf("\n");
   }
#endif
}
#endif



/*
 * Compute log base 2 of n.
 * If n isn't an exact power of two return -1.
 * If n < 0 return -1.
 */
static int
logbase2( int n )
{
   GLint i = 1;
   GLint log2 = 0;

   if (n < 0) {
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
GLint
_mesa_base_tex_format( GLcontext *ctx, GLint format )
{
   /*
    * Ask the driver for the base format, if it doesn't
    * know, it will return -1;
    */
   if (ctx->Driver.BaseCompressedTexFormat) {
      GLint ifmt = (*ctx->Driver.BaseCompressedTexFormat)(ctx, format);
      if (ifmt >= 0) {
         return ifmt;
      }
   }
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
      case GL_DEPTH_COMPONENT:
      case GL_DEPTH_COMPONENT16_SGIX:
      case GL_DEPTH_COMPONENT24_SGIX:
      case GL_DEPTH_COMPONENT32_SGIX:
         if (ctx->Extensions.SGIX_depth_texture)
            return GL_DEPTH_COMPONENT;
         else
            return -1;
      default:
         return -1;  /* error */
   }
}


/*
 * Test if the given image format is a color/rgba format.  That is,
 * not color index, depth, stencil, etc.
 */
static GLboolean
is_color_format(GLenum format)
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
      case 1:
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
      case 2:
      case GL_LUMINANCE_ALPHA:
      case GL_LUMINANCE4_ALPHA4:
      case GL_LUMINANCE6_ALPHA2:
      case GL_LUMINANCE8_ALPHA8:
      case GL_LUMINANCE12_ALPHA4:
      case GL_LUMINANCE12_ALPHA12:
      case GL_LUMINANCE16_ALPHA16:
      case GL_INTENSITY:
      case GL_INTENSITY4:
      case GL_INTENSITY8:
      case GL_INTENSITY12:
      case GL_INTENSITY16:
      case 3:
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
      case 4:
      case GL_RGBA:
      case GL_RGBA2:
      case GL_RGBA4:
      case GL_RGB5_A1:
      case GL_RGBA8:
      case GL_RGB10_A2:
      case GL_RGBA12:
      case GL_RGBA16:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


static GLboolean
is_index_format(GLenum format)
{
   switch (format) {
      case GL_COLOR_INDEX:
      case GL_COLOR_INDEX1_EXT:
      case GL_COLOR_INDEX2_EXT:
      case GL_COLOR_INDEX4_EXT:
      case GL_COLOR_INDEX8_EXT:
      case GL_COLOR_INDEX12_EXT:
      case GL_COLOR_INDEX16_EXT:
         return GL_TRUE;
      default:
         return GL_FALSE;
   }
}


/*
 * Return GL_TRUE if internalFormat is a compressed format, return GL_FALSE
 * otherwise.
 */
static GLboolean
is_compressed_format(GLcontext *ctx, GLenum internalFormat)
{
   if (ctx->Driver.BaseCompressedTexFormat) {
      GLint b = (*ctx->Driver.BaseCompressedTexFormat)(ctx, internalFormat);
      if (b > 0)
         return GL_TRUE;
      else
         return GL_FALSE;
   }
   return GL_FALSE;
}



/*
 * Store a gl_texture_image pointer in a gl_texture_object structure
 * according to the target and level parameters.
 * This was basically prompted by the introduction of cube maps.
 */
void
_mesa_set_tex_image(struct gl_texture_object *tObj,
                    GLenum target, GLint level,
                    struct gl_texture_image *texImage)
{
   ASSERT(tObj);
   ASSERT(texImage);
   switch (target) {
      case GL_TEXTURE_1D:
      case GL_TEXTURE_2D:
      case GL_TEXTURE_3D:
         tObj->Image[level] = texImage;
         return;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
         tObj->Image[level] = texImage;
         return;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
         tObj->NegX[level] = texImage;
         return;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
         tObj->PosY[level] = texImage;
         return;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
         tObj->NegY[level] = texImage;
         return;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
         tObj->PosZ[level] = texImage;
         return;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
         tObj->NegZ[level] = texImage;
         return;
      default:
         _mesa_problem(NULL, "bad target in _mesa_set_tex_image()");
         return;
   }
}



/*
 * Return new gl_texture_image struct with all fields initialized to zero.
 */
struct gl_texture_image *
_mesa_alloc_texture_image( void )
{
   return CALLOC_STRUCT(gl_texture_image);
}



void
_mesa_free_texture_image( struct gl_texture_image *teximage )
{
   if (teximage->Data) {
      MESA_PBUFFER_FREE( teximage->Data );
      teximage->Data = NULL;
   }
   FREE( teximage );
}


/*
 * Return GL_TRUE if the target is a proxy target.
 */
static GLboolean
is_proxy_target(GLenum target)
{
   return (target == GL_PROXY_TEXTURE_1D ||
           target == GL_PROXY_TEXTURE_2D ||
           target == GL_PROXY_TEXTURE_3D ||
           target == GL_PROXY_TEXTURE_CUBE_MAP_ARB);
}


/*
 * Given a texture unit and a texture target, return the corresponding
 * texture object.
 */
struct gl_texture_object *
_mesa_select_tex_object(GLcontext *ctx, const struct gl_texture_unit *texUnit,
                        GLenum target)
{
   switch (target) {
      case GL_TEXTURE_1D:
         return texUnit->Current1D;
      case GL_PROXY_TEXTURE_1D:
         return ctx->Texture.Proxy1D;
      case GL_TEXTURE_2D:
         return texUnit->Current2D;
      case GL_PROXY_TEXTURE_2D:
         return ctx->Texture.Proxy2D;
      case GL_TEXTURE_3D:
         return texUnit->Current3D;
      case GL_PROXY_TEXTURE_3D:
         return ctx->Texture.Proxy3D;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
      case GL_TEXTURE_CUBE_MAP_ARB:
         return ctx->Extensions.ARB_texture_cube_map
                ? texUnit->CurrentCubeMap : NULL;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         return ctx->Extensions.ARB_texture_cube_map
                ? ctx->Texture.ProxyCubeMap : NULL;
      default:
         _mesa_problem(NULL, "bad target in _mesa_select_tex_object()");
         return NULL;
   }
}


/*
 * Return the texture image struct which corresponds to target and level
 * for the given texture unit.
 */
struct gl_texture_image *
_mesa_select_tex_image(GLcontext *ctx, const struct gl_texture_unit *texUnit,
                       GLenum target, GLint level)
{
   ASSERT(texUnit);
   ASSERT(level < MAX_TEXTURE_LEVELS);
   switch (target) {
      case GL_TEXTURE_1D:
         return texUnit->Current1D->Image[level];
      case GL_PROXY_TEXTURE_1D:
         return ctx->Texture.Proxy1D->Image[level];
      case GL_TEXTURE_2D:
         return texUnit->Current2D->Image[level];
      case GL_PROXY_TEXTURE_2D:
         return ctx->Texture.Proxy2D->Image[level];
      case GL_TEXTURE_3D:
         return texUnit->Current3D->Image[level];
      case GL_PROXY_TEXTURE_3D:
         return ctx->Texture.Proxy3D->Image[level];
      case GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return texUnit->CurrentCubeMap->Image[level];
         else
            return NULL;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return texUnit->CurrentCubeMap->NegX[level];
         else
            return NULL;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return texUnit->CurrentCubeMap->PosY[level];
         else
            return NULL;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return texUnit->CurrentCubeMap->NegY[level];
         else
            return NULL;
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return texUnit->CurrentCubeMap->PosZ[level];
         else
            return NULL;
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return texUnit->CurrentCubeMap->NegZ[level];
         else
            return NULL;
      case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
         if (ctx->Extensions.ARB_texture_cube_map)
            return ctx->Texture.ProxyCubeMap->Image[level];
         else
            return NULL;
      default:
         _mesa_problem(ctx, "bad target in _mesa_select_tex_image()");
         return NULL;
   }
}



#if 000 /* not used anymore */
/*
 * glTexImage[123]D can accept a NULL image pointer.  In this case we
 * create a texture image with unspecified image contents per the OpenGL
 * spec.
 */
static GLubyte *
make_null_texture(GLint width, GLint height, GLint depth, GLenum format)
{
   const GLint components = _mesa_components_in_format(format);
   const GLint numPixels = width * height * depth;
   GLubyte *data = (GLubyte *) MALLOC(numPixels * components * sizeof(GLubyte));

#ifdef DEBUG
   /*
    * Let's see if anyone finds this.  If glTexImage2D() is called with
    * a NULL image pointer then load the texture image with something
    * interesting instead of leaving it indeterminate.
    */
   if (data) {
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

      GLubyte *imgPtr = data;
      GLint h, i, j, k;
      for (h = 0; h < depth; h++) {
         for (i = 0; i < height; i++) {
            GLint srcRow = 7 - (i % 8);
            for (j = 0; j < width; j++) {
               GLint srcCol = j % 32;
               GLubyte texel = (message[srcRow][srcCol]=='X') ? 255 : 70;
               for (k = 0; k < components; k++) {
                  *imgPtr++ = texel;
               }
            }
         }
      }
   }
#endif

   return data;
}
#endif



/*
 * Reset the fields of a gl_texture_image struct to zero.
 * This is called when a proxy texture test fails, we set all the
 * image members (except DriverData) to zero.
 * It's also used in glTexImage[123]D as a safeguard to be sure all
 * required fields get initialized properly by the Driver.TexImage[123]D
 * functions.
 */
static void
clear_teximage_fields(struct gl_texture_image *img)
{
   ASSERT(img);
   img->Format = 0;
   img->IntFormat = 0;
   img->Border = 0;
   img->Width = 0;
   img->Height = 0;
   img->Depth = 0;
   img->Width2 = 0;
   img->Height2 = 0;
   img->Depth2 = 0;
   img->WidthLog2 = 0;
   img->HeightLog2 = 0;
   img->DepthLog2 = 0;
   img->Data = NULL;
   img->TexFormat = &_mesa_null_texformat;
   img->FetchTexel = NULL;
   img->IsCompressed = 0;
   img->CompressedSize = 0;
}


/*
 * Initialize basic fields of the gl_texture_image struct.
 */
void
_mesa_init_teximage_fields(GLcontext *ctx,
                           struct gl_texture_image *img,
                           GLsizei width, GLsizei height, GLsizei depth,
                           GLint border, GLenum internalFormat)
{
   ASSERT(img);
   img->Format = _mesa_base_tex_format( ctx, internalFormat );
   ASSERT(img->Format > 0);
   img->IntFormat = internalFormat;
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
   img->IsCompressed = is_compressed_format(ctx, internalFormat);
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
   GLint maxLevels = 0, maxTextureSize;

   if (dimensions == 1) {
      isProxy = (GLboolean) (target == GL_PROXY_TEXTURE_1D);
      if (target != GL_TEXTURE_1D && !isProxy) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (dimensions == 2) {
      isProxy = (GLboolean) (target == GL_PROXY_TEXTURE_2D ||
                             target == GL_PROXY_TEXTURE_CUBE_MAP_ARB);
      if (target != GL_TEXTURE_2D && !isProxy &&
          !(ctx->Extensions.ARB_texture_cube_map &&
            target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
            target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)) {
          _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
          return GL_TRUE;
      }
      if (target == GL_PROXY_TEXTURE_2D || target == GL_TEXTURE_2D)
         maxLevels = ctx->Const.MaxTextureLevels;
      else
         maxLevels = ctx->Const.MaxCubeTextureLevels;
   }
   else if (dimensions == 3) {
      isProxy = (GLboolean) (target == GL_PROXY_TEXTURE_3D);
      if (target != GL_TEXTURE_3D && !isProxy) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.Max3DTextureLevels;
   }
   else {
      _mesa_problem( ctx, "bad dims in texture_error_check" );
      return GL_TRUE;
   }

   ASSERT(maxLevels > 0);
   maxTextureSize = 1 << (maxLevels - 1);

   /* Border */
   if (border != 0 && border != 1) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(border=%d)", dimensions, border);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   /* Width */
   if (width < 2 * border || width > 2 + maxTextureSize
       || logbase2( width - 2 * border ) < 0) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(width=%d)", dimensions, width);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   /* Height */
   if (dimensions >= 2) {
      if (height < 2 * border || height > 2 + maxTextureSize
          || logbase2( height - 2 * border ) < 0) {
         if (!isProxy) {
            char message[100];
            sprintf(message, "glTexImage%dD(height=%d)", dimensions, height);
            _mesa_error(ctx, GL_INVALID_VALUE, message);
         }
         return GL_TRUE;
      }
   }

   /* For cube map, width must equal height */
   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
      if (width != height) {
         if (!isProxy) {
            _mesa_error(ctx, GL_INVALID_VALUE, "glTexImage2D(width != height)");
         }
         return GL_TRUE;
      }
   }

   /* Depth */
   if (dimensions >= 3) {
      if (depth < 2 * border || depth > 2 + maxTextureSize
          || logbase2( depth - 2 * border ) < 0) {
         if (!isProxy) {
            char message[100];
            sprintf(message, "glTexImage3D(depth=%d)", depth );
            _mesa_error( ctx, GL_INVALID_VALUE, message );
         }
         return GL_TRUE;
      }
   }

   /* Level */
   if (level < 0 || level >= maxLevels) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(level=%d)", dimensions, level);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   /* For cube map, width must equal height */
   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
      if (width != height) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexImage2D(width != height)");
         return GL_TRUE;
      }
   }

   iformat = _mesa_base_tex_format( ctx, internalFormat );
   if (iformat < 0) {
      if (!isProxy) {
         char message[100];
         sprintf(message, "glTexImage%dD(internalFormat=0x%x)", dimensions,
                 internalFormat);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
      }
      return GL_TRUE;
   }

   ASSERT(iformat > 0);

   if (!is_compressed_format( ctx, internalFormat ) &&
       !_mesa_is_legal_format_and_type( format, type )) {
      /* Yes, generate GL_INVALID_OPERATION, not GL_INVALID_ENUM, if there
       * is a type/format mismatch.  See 1.2 spec page 94, sec 3.6.4.
       */
      if (!isProxy) {
	 char message[100];
	 sprintf(message, "glTexImage%dD(format or type)", dimensions);
	 _mesa_error(ctx, GL_INVALID_OPERATION, message);
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
   GLint maxLevels = 0;
   GLboolean compressed;

   if (dimensions == 1) {
      if (target != GL_TEXTURE_1D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage1D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (dimensions == 2) {
      if (ctx->Extensions.ARB_texture_cube_map) {
         if ((target < GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB ||
              target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) &&
             target != GL_TEXTURE_2D) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target != GL_TEXTURE_2D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage2D(target)" );
         return GL_TRUE;
      }
      if (target == GL_PROXY_TEXTURE_2D && target == GL_TEXTURE_2D)
         maxLevels = ctx->Const.MaxTextureLevels;
      else
         maxLevels = ctx->Const.MaxCubeTextureLevels;
   }
   else if (dimensions == 3) {
      if (target != GL_TEXTURE_3D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glTexSubImage3D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.Max3DTextureLevels;
   }
   else {
      _mesa_problem( ctx, "bad dims in texture_error_check" );
      return GL_TRUE;
   }

   ASSERT(maxLevels > 0);

   if (level < 0 || level >= maxLevels) {
      char message[100];
      sprintf(message, "glTexSubImage2D(level=%d)", level);
      _mesa_error(ctx, GL_INVALID_ENUM, message);
      return GL_TRUE;
   }

   if (width < 0) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(width=%d)", dimensions, width);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (height < 0 && dimensions > 1) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(height=%d)", dimensions, height);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (depth < 0 && dimensions > 2) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(depth=%d)", dimensions, depth);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   destTex = _mesa_select_tex_image(ctx, texUnit, target, level);

   if (!destTex) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glTexSubImage2D");
      return GL_TRUE;
   }

   if (xoffset < -((GLint)destTex->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage1/2/3D(xoffset)");
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (destTex->Width + destTex->Border)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage1/2/3D(xoffset+width)");
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage2/3D(yoffset)");
         return GL_TRUE;
      }
      if (yoffset + height > (GLint) (destTex->Height + destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage2/3D(yoffset+height)");
         return GL_TRUE;
      }
   }
   if (dimensions > 2) {
      if (zoffset < -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset)");
         return GL_TRUE;
      }
      if (zoffset + depth  > (GLint) (destTex->Depth + destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glTexSubImage3D(zoffset+depth)");
         return GL_TRUE;
      }
   }

   compressed = is_compressed_format(ctx, destTex->IntFormat);

   if (!compressed && !_mesa_is_legal_format_and_type(format, type)) {
      char message[100];
      sprintf(message, "glTexSubImage%dD(format or type)", dimensions);
      _mesa_error(ctx, GL_INVALID_ENUM, message);
      return GL_TRUE;
   }

   if (compressed) {
      if (xoffset != -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage1/2/3D(xoffset != -border");
         return GL_TRUE;
      }
      if (dimensions > 1 && yoffset != -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage2/3D(yoffset != -border");
         return GL_TRUE;
      }
      if (dimensions > 2 && zoffset != -((GLint)destTex->Border)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glTexSubImage3D(zoffset != -border");
         return GL_TRUE;
      }
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
   GLint maxLevels = 0, maxTextureSize;

   if (dimensions == 1) {
      if (target != GL_TEXTURE_1D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage1D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (dimensions == 2) {
      if (ctx->Extensions.ARB_texture_cube_map) {
         if ((target < GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB ||
              target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) &&
             target != GL_TEXTURE_2D) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target != GL_TEXTURE_2D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexImage2D(target)" );
         return GL_TRUE;
      }
      if (target == GL_PROXY_TEXTURE_2D && target == GL_TEXTURE_2D)
         maxLevels = ctx->Const.MaxTextureLevels;
      else
         maxLevels = ctx->Const.MaxCubeTextureLevels;
   }

   ASSERT(maxLevels > 0);
   maxTextureSize = 1 << (maxLevels - 1);

   /* Border */
   if (border != 0 && border != 1) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(border)", dimensions);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   /* Width */
   if (width < 2 * border || width > 2 + maxTextureSize
       || logbase2( width - 2 * border ) < 0) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(width=%d)", dimensions, width);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   /* Height */
   if (dimensions >= 2) {
      if (height < 2 * border || height > 2 + maxTextureSize
          || logbase2( height - 2 * border ) < 0) {
         char message[100];
         sprintf(message, "glCopyTexImage%dD(height=%d)", dimensions, height);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
   }

   /* For cube map, width must equal height */
   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
      if (width != height) {
         _mesa_error(ctx, GL_INVALID_VALUE, "glCopyTexImage2D(width != height)");
         return GL_TRUE;
      }
   }

   /* Level */
   if (level < 0 || level >= maxLevels) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(level=%d)", dimensions, level);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   iformat = _mesa_base_tex_format(ctx, internalFormat);
   if (iformat < 0) {
      char message[100];
      sprintf(message, "glCopyTexImage%dD(internalFormat)", dimensions);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
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
   GLint maxLevels = 0;
   GLboolean compressed;

   if (dimensions == 1) {
      if (target != GL_TEXTURE_1D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage1D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (dimensions == 2) {
      if (ctx->Extensions.ARB_texture_cube_map) {
         if ((target < GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB ||
              target > GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) &&
             target != GL_TEXTURE_2D) {
            _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
            return GL_TRUE;
         }
      }
      else if (target != GL_TEXTURE_2D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage2D(target)" );
         return GL_TRUE;
      }
      if (target == GL_PROXY_TEXTURE_2D && target == GL_TEXTURE_2D)
         maxLevels = ctx->Const.MaxTextureLevels;
      else
         maxLevels = ctx->Const.MaxCubeTextureLevels;
   }
   else if (dimensions == 3) {
      if (target != GL_TEXTURE_3D) {
         _mesa_error( ctx, GL_INVALID_ENUM, "glCopyTexSubImage3D(target)" );
         return GL_TRUE;
      }
      maxLevels = ctx->Const.Max3DTextureLevels;
   }

   ASSERT(maxLevels > 0);

   if (level < 0 || level >= maxLevels) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(level=%d)", dimensions, level);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   if (width < 0) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(width=%d)", dimensions, width);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (dimensions > 1 && height < 0) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(height=%d)", dimensions, height);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }

   teximage = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!teximage) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(undefined texture)", dimensions);
      _mesa_error(ctx, GL_INVALID_OPERATION, message);
      return GL_TRUE;
   }

   if (xoffset < -((GLint)teximage->Border)) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(xoffset=%d)", dimensions, xoffset);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (xoffset + width > (GLint) (teximage->Width + teximage->Border)) {
      char message[100];
      sprintf(message, "glCopyTexSubImage%dD(xoffset+width)", dimensions);
      _mesa_error(ctx, GL_INVALID_VALUE, message);
      return GL_TRUE;
   }
   if (dimensions > 1) {
      if (yoffset < -((GLint)teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(yoffset=%d)", dimensions, yoffset);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
      /* NOTE: we're adding the border here, not subtracting! */
      if (yoffset + height > (GLint) (teximage->Height + teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(yoffset+height)", dimensions);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
   }

   if (dimensions > 2) {
      if (zoffset < -((GLint)teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(zoffset)", dimensions);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
      if (zoffset > (GLint) (teximage->Depth + teximage->Border)) {
         char message[100];
         sprintf(message, "glCopyTexSubImage%dD(zoffset+depth)", dimensions);
         _mesa_error(ctx, GL_INVALID_VALUE, message);
         return GL_TRUE;
      }
   }

   compressed = is_compressed_format(ctx, teximage->IntFormat);
   if (compressed) {
      if (xoffset != -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexSubImage1/2/3D(xoffset != -border");
         return GL_TRUE;
      }
      if (dimensions > 1 && yoffset != -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexSubImage2/3D(yoffset != -border");
         return GL_TRUE;
      }
      if (dimensions > 2 && zoffset != -((GLint)teximage->Border)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glCopyTexSubImage3D(zoffset != -border");
         return GL_TRUE;
      }
   }

   /* if we get here, the parameters are OK */
   return GL_FALSE;
}



void
_mesa_GetTexImage( GLenum target, GLint level, GLenum format,
                   GLenum type, GLvoid *pixels )
{
   const struct gl_texture_unit *texUnit;
   const struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLint maxLevels = 0;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   texUnit = &(ctx->Texture.Unit[ctx->Texture.CurrentUnit]);
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!texObj || is_proxy_target(target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(target)");
      return;
   }

   if (target == GL_TEXTURE_1D || target == GL_TEXTURE_2D) {
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (target == GL_TEXTURE_3D) {
      maxLevels = ctx->Const.Max3DTextureLevels;
   }
   else {
      maxLevels = ctx->Const.MaxCubeTextureLevels;
   }

   ASSERT(maxLevels > 0);

   if (level < 0 || level >= maxLevels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glGetTexImage(level)" );
      return;
   }

   if (_mesa_sizeof_packed_type(type) <= 0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexImage(type)" );
      return;
   }

   if (_mesa_components_in_format(format) <= 0 ||
       format == GL_STENCIL_INDEX) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetTexImage(format)" );
      return;
   }

   if (!ctx->Extensions.EXT_paletted_texture && is_index_format(format)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
   }

   if (!ctx->Extensions.SGIX_depth_texture && format == GL_DEPTH_COMPONENT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetTexImage(format)");
   }

   /* XXX what if format/type doesn't match texture format/type? */

   if (!pixels)
      return;

   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!texImage) {
      /* invalid mipmap level, not an error */
      return;
   }

   if (!texImage->Data) {
      /* no image data, not an error */
      return;
   }

   {
      const GLint width = texImage->Width;
      const GLint height = texImage->Height;
      const GLint depth = texImage->Depth;
      GLint img, row;
      for (img = 0; img < depth; img++) {
         for (row = 0; row < height; row++) {
            /* compute destination address in client memory */
            GLvoid *dest = _mesa_image_address( &ctx->Unpack, pixels,
                                                width, height, format, type,
                                                img, row, 0);
            assert(dest);

            if (format == GL_COLOR_INDEX) {
               GLuint indexRow[MAX_WIDTH];
               GLint col;
               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexel)(texImage, col, row, img,
                                          (GLvoid *) &indexRow[col]);
               }
               _mesa_pack_index_span(ctx, width, type, dest,
                                     indexRow, &ctx->Pack,
                                     0 /* no image transfer */);
            }
            else if (format == GL_DEPTH_COMPONENT) {
               GLfloat depthRow[MAX_WIDTH];
               GLint col;
               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexel)(texImage, col, row, img,
                                          (GLvoid *) &depthRow[col]);
               }
               _mesa_pack_depth_span(ctx, width, dest, type,
                                     depthRow, &ctx->Pack);
            }
            else {
               /* general case:  convert row to RGBA format */
               GLchan rgba[MAX_WIDTH][4];
               GLint col;
               for (col = 0; col < width; col++) {
                  (*texImage->FetchTexel)(texImage, col, row, img,
                                          (GLvoid *) rgba[col]);
               }
               _mesa_pack_rgba_span(ctx, width, (const GLchan (*)[4])rgba,
                                    format, type, dest, &ctx->Pack,
                                    0 /* no image transfer */);
            } /* format */
         } /* row */
      } /* img */
   }
}



/*
 * Called from the API.  Note that width includes the border.
 */
void
_mesa_TexImage1D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLint border, GLenum format,
                  GLenum type, const GLvoid *pixels )
{
   GLsizei postConvWidth = width;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   if (target == GL_TEXTURE_1D) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 1, postConvWidth, 1, 1, border)) {
         return;   /* error was recorded */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = _mesa_select_tex_object(ctx, texUnit, target);
      texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

      if (!texImage) {
         texImage = _mesa_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage1D");
            return;
         }
      }
      else if (texImage->Data) {
         /* free the old texture data */
         MESA_PBUFFER_FREE(texImage->Data);
         texImage->Data = NULL;
      }
      clear_teximage_fields(texImage); /* not really needed, but helpful */
      _mesa_init_teximage_fields(ctx, texImage, postConvWidth, 1, 1,
                                 border, internalFormat);

      if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
         _mesa_update_state(ctx);

      ASSERT(ctx->Driver.TexImage1D);

#if 0 /* don't make default teximage anymore */
      if (pixels) {
         (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                   width, border, format, type, pixels,
                                   &ctx->Unpack, texObj, texImage);
      }
      else {
         GLubyte *dummy = make_null_texture(width, 1, 1, format);
         if (dummy) {
            (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                      width, border,
                                      format, GL_UNSIGNED_BYTE, dummy,
                                      &_mesa_native_packing, texObj, texImage);
            FREE(dummy);
         }
      }
#else
      /* <pixels> may be null! */
      (*ctx->Driver.TexImage1D)(ctx, target, level, internalFormat,
                                width, border, format, type, pixels,
                                &ctx->Unpack, texObj, texImage);
#endif

      ASSERT(texImage->TexFormat);
      if (!texImage->FetchTexel) {
         /* If driver didn't explicitly set this, use the default */
         texImage->FetchTexel = texImage->TexFormat->FetchTexel1D;
      }
      ASSERT(texImage->FetchTexel);

      if (texImage->IsCompressed) {
         ASSERT(texImage->CompressedSize > 0);
      }

      /* state update */
      texObj->Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   else if (target == GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = texture_error_check(ctx, target, level, internalFormat,
                                         format, type, 1,
                                         postConvWidth, 1, 1, border);
      if (!error) {
         struct gl_texture_unit *texUnit;
         struct gl_texture_image *texImage;
         texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         _mesa_init_teximage_fields(ctx, texImage, postConvWidth, 1, 1,
                                    border, internalFormat);
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                                  internalFormat, format, type,
                                                  postConvWidth, 1, 1, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         if (level >= 0 && level < ctx->Const.MaxTextureLevels) {
            clear_teximage_fields(ctx->Texture.Proxy1D->Image[level]);
         }
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage1D(target)" );
      return;
   }
}


void
_mesa_TexImage2D( GLenum target, GLint level, GLint internalFormat,
                  GLsizei width, GLsizei height, GLint border,
                  GLenum format, GLenum type,
                  const GLvoid *pixels )
{
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
					 &postConvHeight);
   }

   if (target == GL_TEXTURE_2D ||
       (ctx->Extensions.ARB_texture_cube_map &&
        target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
        target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)) {
      /* non-proxy target */
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check(ctx, target, level, internalFormat,
                              format, type, 2, postConvWidth, postConvHeight,
                              1, border)) {
         return;   /* error was recorded */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = _mesa_select_tex_object(ctx, texUnit, target);
      texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

      if (!texImage) {
         texImage = _mesa_alloc_texture_image();
         _mesa_set_tex_image(texObj, target, level, texImage);
         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage2D");
            return;
         }
      }
      else if (texImage->Data) {
         /* free the old texture data */
         MESA_PBUFFER_FREE(texImage->Data);
         texImage->Data = NULL;
      }
      clear_teximage_fields(texImage); /* not really needed, but helpful */
      _mesa_init_teximage_fields(ctx, texImage, postConvWidth, postConvHeight,
                                 1, border, internalFormat);

      if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
         _mesa_update_state(ctx);

      ASSERT(ctx->Driver.TexImage2D);

#if 0 /* don't make default teximage anymore */
      if (pixels) {
         (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                   width, height, border, format, type, pixels,
                                   &ctx->Unpack, texObj, texImage);
      }
      else {
         GLubyte *dummy = make_null_texture(width, height, 1, format);
         if (dummy) {
            (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                      width, height, border,
                                      format, GL_UNSIGNED_BYTE, dummy,
                                      &_mesa_native_packing, texObj, texImage);
            FREE(dummy);
         }
      }
#else
      /* <pixels> may be null! */
      (*ctx->Driver.TexImage2D)(ctx, target, level, internalFormat,
                                width, height, border, format, type, pixels,
                                &ctx->Unpack, texObj, texImage);
#endif

      ASSERT(texImage->TexFormat);
      if (!texImage->FetchTexel) {
         /* If driver didn't explicitly set this, use the default */
         texImage->FetchTexel = texImage->TexFormat->FetchTexel2D;
      }
      ASSERT(texImage->FetchTexel);

      if (texImage->IsCompressed) {
         ASSERT(texImage->CompressedSize > 0);
      }

      /* state update */
      texObj->Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   else if (target == GL_PROXY_TEXTURE_2D ||
            (target == GL_PROXY_TEXTURE_CUBE_MAP_ARB &&
             ctx->Extensions.ARB_texture_cube_map)) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = texture_error_check(ctx, target, level, internalFormat,
				format, type, 2,
				postConvWidth, postConvHeight, 1, border);
      if (!error) {
         struct gl_texture_unit *texUnit;
         struct gl_texture_image *texImage;
         texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         _mesa_init_teximage_fields(ctx, texImage, postConvWidth,
                                    postConvHeight, 1, border, internalFormat);
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                    internalFormat, format, type,
                                    postConvWidth, postConvHeight, 1, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         const GLint maxLevels = (target == GL_PROXY_TEXTURE_2D) ?
            ctx->Const.MaxTextureLevels : ctx->Const.MaxCubeTextureLevels;
         if (level >= 0 && level < maxLevels) {
            clear_teximage_fields(ctx->Texture.Proxy2D->Image[level]);
         }
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage2D(target)" );
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
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (target == GL_TEXTURE_3D) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check(ctx, target, level, (GLint) internalFormat,
                              format, type, 3, width, height, depth, border)) {
         return;   /* error was recorded */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = _mesa_select_tex_object(ctx, texUnit, target);
      texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

      if (!texImage) {
         texImage = _mesa_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage3D");
            return;
         }
      }
      else if (texImage->Data) {
         MESA_PBUFFER_FREE(texImage->Data);
         texImage->Data = NULL;
      }
      clear_teximage_fields(texImage); /* not really needed, but helpful */
      _mesa_init_teximage_fields(ctx, texImage, width, height, depth, border,
                                 internalFormat);

      if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
         _mesa_update_state(ctx);

      ASSERT(ctx->Driver.TexImage3D);

#if 0 /* don't make default teximage anymore */
      if (pixels) {
         (*ctx->Driver.TexImage3D)(ctx, target, level, (GLint) internalFormat,
                                   width, height, depth, border,
                                   format, type, pixels,
                                   &ctx->Unpack, texObj, texImage);
      }
      else {
         GLubyte *dummy = make_null_texture(width, height, depth, format);
         if (dummy) {
            (*ctx->Driver.TexImage3D)(ctx, target, level,
                                      (GLint) internalFormat,
                                      width, height, depth, border,
                                      format, GL_UNSIGNED_BYTE, dummy,
                                      &_mesa_native_packing, texObj, texImage);
            FREE(dummy);
         }
      }
#else
      /* <pixels> may be null! */
      (*ctx->Driver.TexImage3D)(ctx, target, level, internalFormat,
                                width, height, depth, border, format, type,
                                pixels, &ctx->Unpack, texObj, texImage);
#endif

      ASSERT(texImage->TexFormat);
      if (!texImage->FetchTexel) {
         /* If driver didn't explicitly set this, use the default */
         texImage->FetchTexel = texImage->TexFormat->FetchTexel3D;
      }
      ASSERT(texImage->FetchTexel);

      if (texImage->IsCompressed) {
         ASSERT(texImage->CompressedSize > 0);
      }

      /* state update */
      texObj->Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   else if (target == GL_PROXY_TEXTURE_3D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = texture_error_check(ctx, target, level, internalFormat,
                                format, type, 3, width, height, depth, border);
      if (!error) {
         struct gl_texture_unit *texUnit;
         struct gl_texture_image *texImage;
         texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         _mesa_init_teximage_fields(ctx, texImage, width, height, 1,
                                    border, internalFormat);
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                                 internalFormat, format, type,
                                                 width, height, depth, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         if (level >= 0 && level < ctx->Const.Max3DTextureLevels) {
            clear_teximage_fields(ctx->Texture.Proxy3D->Image[level]);
         }
      }
   }
   else {
      _mesa_error( ctx, GL_INVALID_ENUM, "glTexImage3D(target)" );
      return;
   }
}


void
_mesa_TexImage3DEXT( GLenum target, GLint level, GLenum internalFormat,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLint border, GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   _mesa_TexImage3D(target, level, internalFormat, width, height,
                    depth, border, format, type, pixels);
}



void
_mesa_TexSubImage1D( GLenum target, GLint level,
                     GLint xoffset, GLsizei width,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GLsizei postConvWidth = width;
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   /* XXX should test internal format */
   if (is_color_format(format)) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   if (subtexture_error_check(ctx, 1, target, level, xoffset, 0, 0,
                              postConvWidth, 1, 1, format, type)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   assert(texImage);

   if (width == 0 || !pixels)
      return;  /* no-op, not an error */

   /* If we have a border, xoffset=-1 is legal.  Bias by border width */
   xoffset += texImage->Border;

   ASSERT(ctx->Driver.TexSubImage1D);
   (*ctx->Driver.TexSubImage1D)(ctx, target, level, xoffset, width,
                                format, type, pixels, &ctx->Unpack,
                                texObj, texImage);
   ctx->NewState |= _NEW_TEXTURE;
}


void
_mesa_TexSubImage2D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   GLsizei postConvWidth = width, postConvHeight = height;
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   /* XXX should test internal format */
   if (is_color_format(format)) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }

   if (subtexture_error_check(ctx, 2, target, level, xoffset, yoffset, 0,
                             postConvWidth, postConvHeight, 1, format, type)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   assert(texImage);

   if (width == 0 || height == 0 || !pixels)
      return;  /* no-op, not an error */

   /* If we have a border, xoffset=-1 is legal.  Bias by border width */
   xoffset += texImage->Border;
   yoffset += texImage->Border;

   ASSERT(ctx->Driver.TexSubImage2D);
   (*ctx->Driver.TexSubImage2D)(ctx, target, level, xoffset, yoffset,
                                width, height, format, type, pixels,
                                &ctx->Unpack, texObj, texImage);
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_TexSubImage3D( GLenum target, GLint level,
                     GLint xoffset, GLint yoffset, GLint zoffset,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLenum format, GLenum type,
                     const GLvoid *pixels )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   if (subtexture_error_check(ctx, 3, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, type)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   assert(texImage);

   if (width == 0 || height == 0 || height == 0 || !pixels)
      return;  /* no-op, not an error */

   /* If we have a border, xoffset=-1 is legal.  Bias by border width */
   xoffset += texImage->Border;
   yoffset += texImage->Border;
   zoffset += texImage->Border;

   ASSERT(ctx->Driver.TexSubImage3D);
   (*ctx->Driver.TexSubImage3D)(ctx, target, level,
                                xoffset, yoffset, zoffset,
                                width, height, depth,
                                format, type, pixels,
                                &ctx->Unpack, texObj, texImage );
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_CopyTexImage1D( GLenum target, GLint level,
                      GLenum internalFormat,
                      GLint x, GLint y,
                      GLsizei width, GLint border )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   if (is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);
   }

   if (copytexture_error_check(ctx, 1, target, level, internalFormat,
                               postConvWidth, 1, border))
      return;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!texImage) {
      texImage = _mesa_alloc_texture_image();
      _mesa_set_tex_image(texObj, target, level, texImage);
      if (!texImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage1D");
         return;
      }
   }
   else if (texImage->Data) {
      /* free the old texture data */
      MESA_PBUFFER_FREE(texImage->Data);
      texImage->Data = NULL;
   }

   clear_teximage_fields(texImage); /* not really needed, but helpful */
   _mesa_init_teximage_fields(ctx, texImage, postConvWidth, 1, 1,
                              border, internalFormat);


   ASSERT(ctx->Driver.CopyTexImage1D);
   (*ctx->Driver.CopyTexImage1D)(ctx, target, level, internalFormat,
                                 x, y, width, border);

   ASSERT(texImage->TexFormat);
   if (!texImage->FetchTexel) {
      /* If driver didn't explicitly set this, use the default */
      texImage->FetchTexel = texImage->TexFormat->FetchTexel1D;
   }
   ASSERT(texImage->FetchTexel);

   /* state update */
   texObj->Complete = GL_FALSE;
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_CopyTexImage2D( GLenum target, GLint level, GLenum internalFormat,
                      GLint x, GLint y, GLsizei width, GLsizei height,
                      GLint border )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   if (is_color_format(internalFormat)) {
      _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth,
                                         &postConvHeight);
   }

   if (copytexture_error_check(ctx, 2, target, level, internalFormat,
                               postConvWidth, postConvHeight, border))
      return;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!texImage) {
      texImage = _mesa_alloc_texture_image();
      _mesa_set_tex_image(texObj, target, level, texImage);
      if (!texImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexImage2D");
         return;
      }
   }
   else if (texImage->Data) {
      /* free the old texture data */
      MESA_PBUFFER_FREE(texImage->Data);
      texImage->Data = NULL;
   }

   clear_teximage_fields(texImage); /* not really needed, but helpful */
   _mesa_init_teximage_fields(ctx, texImage, postConvWidth, postConvHeight, 1,
                              border, internalFormat);

   ASSERT(ctx->Driver.CopyTexImage2D);
   (*ctx->Driver.CopyTexImage2D)(ctx, target, level, internalFormat,
                                 x, y, width, height, border);

   ASSERT(texImage->TexFormat);
   if (!texImage->FetchTexel) {
      /* If driver didn't explicitly set this, use the default */
      texImage->FetchTexel = texImage->TexFormat->FetchTexel2D;
   }
   ASSERT(texImage->FetchTexel);

   /* state update */
   texObj->Complete = GL_FALSE;
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_CopyTexSubImage1D( GLenum target, GLint level,
                         GLint xoffset, GLint x, GLint y, GLsizei width )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   /* XXX should test internal format */
   _mesa_adjust_image_for_convolution(ctx, 1, &postConvWidth, NULL);

   if (copytexsubimage_error_check(ctx, 1, target, level,
                                   xoffset, 0, 0, postConvWidth, 1))
      return;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

   /* If we have a border, xoffset=-1 is legal.  Bias by border width */
   xoffset += texImage->Border;

   ASSERT(ctx->Driver.CopyTexSubImage1D);
   (*ctx->Driver.CopyTexSubImage1D)(ctx, target, level, xoffset, x, y, width);
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_CopyTexSubImage2D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   /* XXX should test internal format */
   _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth, &postConvHeight);

   if (copytexsubimage_error_check(ctx, 2, target, level, xoffset, yoffset, 0,
                                   postConvWidth, postConvHeight))
      return;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

   /* If we have a border, xoffset=-1 is legal.  Bias by border width */
   xoffset += texImage->Border;
   yoffset += texImage->Border;

   ASSERT(ctx->Driver.CopyTexSubImage2D);
   (*ctx->Driver.CopyTexSubImage2D)(ctx, target, level,
                                    xoffset, yoffset, x, y, width, height);
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_CopyTexSubImage3D( GLenum target, GLint level,
                         GLint xoffset, GLint yoffset, GLint zoffset,
                         GLint x, GLint y, GLsizei width, GLsizei height )
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLsizei postConvWidth = width, postConvHeight = height;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->NewState & _IMAGE_NEW_TRANSFER_STATE)
      _mesa_update_state(ctx);

   /* XXX should test internal format */
   _mesa_adjust_image_for_convolution(ctx, 2, &postConvWidth, &postConvHeight);

   if (copytexsubimage_error_check(ctx, 3, target, level, xoffset, yoffset,
                                   zoffset, postConvWidth, postConvHeight))
      return;

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

   /* If we have a border, xoffset=-1 is legal.  Bias by border width */
   xoffset += texImage->Border;
   yoffset += texImage->Border;
   zoffset += texImage->Border;

   ASSERT(ctx->Driver.CopyTexSubImage3D);
   (*ctx->Driver.CopyTexSubImage3D)(ctx, target, level,
                                    xoffset, yoffset, zoffset,
                                    x, y, width, height);
   ctx->NewState |= _NEW_TEXTURE;
}



void
_mesa_CompressedTexImage1DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (internalFormat) {
      case GL_COMPRESSED_ALPHA_ARB:
      case GL_COMPRESSED_LUMINANCE_ARB:
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      case GL_COMPRESSED_INTENSITY_ARB:
      case GL_COMPRESSED_RGB_ARB:
      case GL_COMPRESSED_RGBA_ARB:
         _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage1DARB");
         return;
      default:
         /* silence compiler warning */
         ;
   }

   if (target == GL_TEXTURE_1D) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check(ctx, target, level, internalFormat,
                              GL_NONE, GL_NONE, 1, width, 1, 1, border)) {
         return;   /* error in texture image was detected */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = _mesa_select_tex_object(ctx, texUnit, target);
      texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

      if (!texImage) {
         texImage = _mesa_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage1DARB");
            return;
         }
      }
      else if (texImage->Data) {
         MESA_PBUFFER_FREE(texImage->Data);
         texImage->Data = NULL;
      }

      _mesa_init_teximage_fields(ctx, texImage, width, 1, 1,
                                 border, internalFormat);

      if (ctx->Extensions.ARB_texture_compression) {
         ASSERT(ctx->Driver.CompressedTexImage1D);
         (*ctx->Driver.CompressedTexImage1D)(ctx, target, level,
                                             internalFormat, width, border,
                                             imageSize, data,
                                             texObj, texImage);
         ASSERT(texImage->CompressedSize > 0); /* sanity */
      }

      /* state update */
      texObj->Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   else if (target == GL_PROXY_TEXTURE_1D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = texture_error_check(ctx, target, level, internalFormat,
                                    GL_NONE, GL_NONE, 1, width, 1, 1, border);
      if (!error) {
         struct gl_texture_unit *texUnit;
         struct gl_texture_image *texImage;
         texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         _mesa_init_teximage_fields(ctx, texImage, width, 1, 1,
                                    border, internalFormat);
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                             internalFormat, GL_NONE, GL_NONE,
                                             width, 1, 1, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         if (level >= 0 && level < ctx->Const.MaxTextureLevels) {
            clear_teximage_fields(ctx->Texture.Proxy1D->Image[level]);
         }
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage1DARB(target)");
      return;
   }
}


void
_mesa_CompressedTexImage2DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLint border, GLsizei imageSize,
                              const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (internalFormat) {
      case GL_COMPRESSED_ALPHA_ARB:
      case GL_COMPRESSED_LUMINANCE_ARB:
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      case GL_COMPRESSED_INTENSITY_ARB:
      case GL_COMPRESSED_RGB_ARB:
      case GL_COMPRESSED_RGBA_ARB:
         _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage2DARB");
         return;
      default:
         /* silence compiler warning */
         ;
   }

   if (target == GL_TEXTURE_2D ||
       (ctx->Extensions.ARB_texture_cube_map &&
        target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
        target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB)) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check(ctx, target, level, internalFormat,
                              GL_NONE, GL_NONE, 1, width, height, 1, border)) {
         return;   /* error in texture image was detected */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = _mesa_select_tex_object(ctx, texUnit, target);
      texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

      if (!texImage) {
         texImage = _mesa_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage2DARB");
            return;
         }
      }
      else if (texImage->Data) {
         MESA_PBUFFER_FREE(texImage->Data);
         texImage->Data = NULL;
      }

      _mesa_init_teximage_fields(ctx, texImage, width, height, 1, border,
                                 internalFormat);

      if (ctx->Extensions.ARB_texture_compression) {
         ASSERT(ctx->Driver.CompressedTexImage2D);
         (*ctx->Driver.CompressedTexImage2D)(ctx, target, level,
                                             internalFormat, width, height,
                                             border, imageSize, data,
                                             texObj, texImage);
         ASSERT(texImage->CompressedSize > 0); /* sanity */
      }

      /* state update */
      texObj->Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   else if (target == GL_PROXY_TEXTURE_2D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = texture_error_check(ctx, target, level, internalFormat,
                                GL_NONE, GL_NONE, 2, width, height, 1, border);
      if (!error) {
         struct gl_texture_unit *texUnit;
         struct gl_texture_image *texImage;
         texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         _mesa_init_teximage_fields(ctx, texImage, width, height, 1,
                                    border, internalFormat);
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                              internalFormat, GL_NONE, GL_NONE,
                                              width, height, 1, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         const GLint maxLevels = (target == GL_PROXY_TEXTURE_2D) ?
            ctx->Const.MaxTextureLevels : ctx->Const.MaxCubeTextureLevels;
         if (level >= 0 && level < maxLevels) {
            clear_teximage_fields(ctx->Texture.Proxy2D->Image[level]);
         }
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage2DARB(target)");
      return;
   }
}


void
_mesa_CompressedTexImage3DARB(GLenum target, GLint level,
                              GLenum internalFormat, GLsizei width,
                              GLsizei height, GLsizei depth, GLint border,
                              GLsizei imageSize, const GLvoid *data)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   switch (internalFormat) {
      case GL_COMPRESSED_ALPHA_ARB:
      case GL_COMPRESSED_LUMINANCE_ARB:
      case GL_COMPRESSED_LUMINANCE_ALPHA_ARB:
      case GL_COMPRESSED_INTENSITY_ARB:
      case GL_COMPRESSED_RGB_ARB:
      case GL_COMPRESSED_RGBA_ARB:
         _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage3DARB");
         return;
      default:
         /* silence compiler warning */
         ;
   }

   if (target == GL_TEXTURE_3D) {
      struct gl_texture_unit *texUnit;
      struct gl_texture_object *texObj;
      struct gl_texture_image *texImage;

      if (texture_error_check(ctx, target, level, internalFormat,
                          GL_NONE, GL_NONE, 1, width, height, depth, border)) {
         return;   /* error in texture image was detected */
      }

      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      texObj = _mesa_select_tex_object(ctx, texUnit, target);
      texImage = _mesa_select_tex_image(ctx, texUnit, target, level);

      if (!texImage) {
         texImage = _mesa_alloc_texture_image();
         texObj->Image[level] = texImage;
         if (!texImage) {
            _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage3DARB");
            return;
         }
      }
      else if (texImage->Data) {
         MESA_PBUFFER_FREE(texImage->Data);
         texImage->Data = NULL;
      }

      _mesa_init_teximage_fields(ctx, texImage, width, height, depth, border,
                                 internalFormat);

      if (ctx->Extensions.ARB_texture_compression) {
         ASSERT(ctx->Driver.CompressedTexImage3D);
         (*ctx->Driver.CompressedTexImage3D)(ctx, target, level,
                                             internalFormat,
                                             width, height, depth,
                                             border, imageSize, data,
                                             texObj, texImage);
         ASSERT(texImage->CompressedSize > 0); /* sanity */
      }

      /* state update */
      texObj->Complete = GL_FALSE;
      ctx->NewState |= _NEW_TEXTURE;
   }
   else if (target == GL_PROXY_TEXTURE_3D) {
      /* Proxy texture: check for errors and update proxy state */
      GLenum error = texture_error_check(ctx, target, level, internalFormat,
                            GL_NONE, GL_NONE, 1, width, height, depth, border);
      if (!error) {
         struct gl_texture_unit *texUnit;
         struct gl_texture_image *texImage;
         texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
         texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
         _mesa_init_teximage_fields(ctx, texImage, width, height, depth,
                                    border, internalFormat);
         ASSERT(ctx->Driver.TestProxyTexImage);
         error = !(*ctx->Driver.TestProxyTexImage)(ctx, target, level,
                                             internalFormat, GL_NONE, GL_NONE,
                                             width, height, depth, border);
      }
      if (error) {
         /* if error, clear all proxy texture image parameters */
         if (level >= 0 && level < ctx->Const.Max3DTextureLevels) {
            clear_teximage_fields(ctx->Texture.Proxy3D->Image[level]);
         }
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glCompressedTexImage3DARB(target)");
      return;
   }
}


void
_mesa_CompressedTexSubImage1DARB(GLenum target, GLint level, GLint xoffset,
                                 GLsizei width, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (subtexture_error_check(ctx, 1, target, level, xoffset, 0, 0,
                              width, 1, 1, format, GL_NONE)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   assert(texImage);

   if (width == 0 || !data)
      return;  /* no-op, not an error */

   if (ctx->Driver.CompressedTexSubImage1D) {
      (*ctx->Driver.CompressedTexSubImage1D)(ctx, target, level,
                                             xoffset, width,
                                             format, imageSize, data,
                                             texObj, texImage);
   }
   ctx->NewState |= _NEW_TEXTURE;
}


void
_mesa_CompressedTexSubImage2DARB(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLsizei width, GLsizei height,
                                 GLenum format, GLsizei imageSize,
                                 const GLvoid *data)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (subtexture_error_check(ctx, 2, target, level, xoffset, yoffset, 0,
                              width, height, 1, format, GL_NONE)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   assert(texImage);

   if (width == 0 || height == 0 || !data)
      return;  /* no-op, not an error */

   if (ctx->Driver.CompressedTexSubImage2D) {
      (*ctx->Driver.CompressedTexSubImage2D)(ctx, target, level,
                                             xoffset, yoffset, width, height,
                                             format, imageSize, data,
                                             texObj, texImage);
   }
   ctx->NewState |= _NEW_TEXTURE;
}


void
_mesa_CompressedTexSubImage3DARB(GLenum target, GLint level, GLint xoffset,
                                 GLint yoffset, GLint zoffset, GLsizei width,
                                 GLsizei height, GLsizei depth, GLenum format,
                                 GLsizei imageSize, const GLvoid *data)
{
   struct gl_texture_unit *texUnit;
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (subtexture_error_check(ctx, 3, target, level, xoffset, yoffset, zoffset,
                              width, height, depth, format, GL_NONE)) {
      return;   /* error was detected */
   }

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   assert(texImage);

   if (width == 0 || height == 0 || depth == 0 || !data)
      return;  /* no-op, not an error */

   if (ctx->Driver.CompressedTexSubImage3D) {
      (*ctx->Driver.CompressedTexSubImage3D)(ctx, target, level,
                                             xoffset, yoffset, zoffset,
                                             width, height, depth,
                                             format, imageSize, data,
                                             texObj, texImage);
   }
   ctx->NewState |= _NEW_TEXTURE;
}


void
_mesa_GetCompressedTexImageARB(GLenum target, GLint level, GLvoid *img)
{
   const struct gl_texture_unit *texUnit;
   const struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   GLint maxLevels;
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   if (!texObj) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetCompressedTexImageARB");
      return;
   }

   if (target == GL_TEXTURE_1D || target == GL_TEXTURE_2D) {
      maxLevels = ctx->Const.MaxTextureLevels;
   }
   else if (target == GL_TEXTURE_3D) {
      maxLevels = ctx->Const.Max3DTextureLevels;
   }
   else {
      maxLevels = ctx->Const.MaxCubeTextureLevels;
   }

   ASSERT(maxLevels > 0);

   if (level < 0 || level >= maxLevels) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetCompressedTexImageARB(level)");
      return;
   }

   if (is_proxy_target(target)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetCompressedTexImageARB(target)");
      return;
   }

   texImage = _mesa_select_tex_image(ctx, texUnit, target, level);
   if (!texImage) {
      /* probably invalid mipmap level */
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetCompressedTexImageARB(level)");
      return;
   }

   if (!texImage->IsCompressed) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetCompressedTexImageARB");
      return;
   }

   if (!img)
      return;

   if (ctx->Extensions.ARB_texture_compression) {
      ASSERT(ctx->Driver.GetCompressedTexImage);
      (*ctx->Driver.GetCompressedTexImage)(ctx, target, level, img, texObj,
                                           texImage);
   }
}
