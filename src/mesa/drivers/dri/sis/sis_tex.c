/**************************************************************************

Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86$ */

/*
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_alloc.h"
#include "sis_tex.h"

#include "swrast/swrast.h"
#include "imports.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"

#define ALIGN(value, align) (char *)((long)(value + align - 1) & ~(align - 1))

#define TEXTURE_HW_ALIGNMENT 4
#define TEXTURE_HW_PLUS (4 + 4)

static sisTexObjPtr
sisAllocTexObj( struct gl_texture_object *texObj )
{
   sisTexObjPtr t;

   t = (sisTexObjPtr) CALLOC_STRUCT( sis_tex_obj );
   texObj->DriverData = t;
   return t;
}

static void
sisAllocTexImage( sisContextPtr smesa, sisTexObjPtr t, int level,
		  const struct gl_texture_image *image )
{
   char *addr;
   int size, texel_size;

   if (t->format == 0) {
      t->format = image->Format;
      switch (t->format)
      {
      case GL_RGBA:
         t->hwformat = TEXEL_ARGB_8888_32;
         break;
      case GL_INTENSITY:
         t->hwformat = TEXEL_I8;
         break;
      case GL_ALPHA:
         t->hwformat = TEXEL_A8;
         break;
      case GL_LUMINANCE:
         t->hwformat = TEXEL_L8;
         break;
      case GL_LUMINANCE_ALPHA:
         t->hwformat = TEXEL_AL88;
         break;
      case GL_RGB:
         t->hwformat = TEXEL_ARGB_0888_32;
         break;
      default:
         assert(0);
      }
   }
   assert(t->format == image->Format);

   texel_size = image->TexFormat->TexelBytes;
   size = image->Width * image->Height * texel_size + TEXTURE_HW_PLUS;

   addr = sisAllocFB( smesa, size, &t->image[level].handle );
   if (addr == NULL) {
      addr = sisAllocAGP( smesa, size, &t->image[level].handle );
      if (addr == NULL) {
         fprintf (stderr, "SIS driver : out of video/agp memory\n");
         sis_fatal_error();
      }
      t->image[level].memType = AGP_TYPE;
   }
   else
      t->image[level].memType = VIDEO_TYPE;
   
   t->image[level].Data = ALIGN(addr, TEXTURE_HW_ALIGNMENT);
   t->image[level].pitch = image->Width * texel_size;
   t->image[level].size = image->Width * image->Height * texel_size;
   t->numImages++;
}

static void
sisFreeTexImage( sisContextPtr smesa, sisTexObjPtr t, int level )
{
   if (t->image[level].Data == NULL)
      return;

   switch (t->image[level].memType)
   {
   case VIDEO_TYPE:
      sisFreeFB( smesa, t->image[level].handle );
      break;
   case AGP_TYPE:
      sisFreeAGP( smesa, t->image[level].handle );
      break;
   }
   t->image[level].Data = NULL;
   t->image[level].handle = NULL;
   /* If there are no textures loaded any more, reset the hw format so the 
    * object can be reused for new formats
    */
   t->numImages--;
   if (t->numImages == 0) {
      t->format = 0;
      t->hwformat = 0;
   }
}

static void 
sisDDTexEnv( GLcontext *ctx, GLenum target, GLenum pname, const GLfloat *param )
{
  sisContextPtr smesa = SIS_CONTEXT(ctx);

  smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
}

static void
sisDDTexParameter( GLcontext *ctx, GLenum target,
		   struct gl_texture_object *texObj, GLenum pname,
		   const GLfloat *params )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void
sisDDBindTexture( GLcontext *ctx, GLenum target,
		  struct gl_texture_object *texObj )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;

   if ( target == GL_TEXTURE_2D || target == GL_TEXTURE_1D ) {
      if ( texObj->DriverData == NULL ) {
	 sisAllocTexObj( texObj );
      }
   }

   t = texObj->DriverData;

   if (t == NULL)
      return;

   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format) {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void
sisDDDeleteTexture( GLcontext * ctx, struct gl_texture_object *texObj )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;
   int i;

   smesa->clearTexCache = GL_TRUE;

   t = texObj->DriverData;
   if (t == NULL) {
      /* 
       * this shows the texture is default object and never be a 
       * argument of sisDDTexImage*
       */
      return;
   }
   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      sisFreeTexImage( smesa, t, i );
   }

   FREE(t);
   texObj->DriverData = NULL;
}

static GLboolean sisDDIsTextureResident( GLcontext * ctx,
			 		 struct gl_texture_object *texObj )
{
  return (texObj->DriverData != NULL);
}

static const struct gl_texture_format *
sisDDChooseTextureFormat( GLcontext *ctx, GLint internalFormat,
			  GLenum format, GLenum type )
{
   /* XXX 16-bit internal texture formats? */
   switch ( internalFormat ) {
   case GL_ALPHA:
   case GL_ALPHA4:
   case GL_ALPHA8:
   case GL_ALPHA12:
   case GL_ALPHA16:
      return &_mesa_texformat_a8;
   case 1:
   case GL_LUMINANCE:
   case GL_LUMINANCE4:
   case GL_LUMINANCE8:
   case GL_LUMINANCE12:
   case GL_LUMINANCE16:
      return &_mesa_texformat_l8;
   case 2:
   case GL_LUMINANCE_ALPHA:
   case GL_LUMINANCE4_ALPHA4:
   case GL_LUMINANCE6_ALPHA2:
   case GL_LUMINANCE8_ALPHA8:
   case GL_LUMINANCE12_ALPHA4:
   case GL_LUMINANCE12_ALPHA12:
   case GL_LUMINANCE16_ALPHA16:
      return &_mesa_texformat_al88;
   case GL_INTENSITY:
   case GL_INTENSITY4:
   case GL_INTENSITY8:
   case GL_INTENSITY12:
   case GL_INTENSITY16:
      return &_mesa_texformat_i8;
   case GL_R3_G3_B2:
   case GL_RGB4:
   case GL_RGB5:
   case 3:
   case GL_RGB:
   case GL_RGB8:
   case GL_RGB10:
   case GL_RGB12:
   case GL_RGB16:
      return &_mesa_texformat_argb8888 /*_mesa_texformat_rgb888*/; /* XXX */
   case GL_RGBA2:
   case GL_RGBA4:
   case GL_RGB5_A1:
   case 4:
   case GL_RGBA:
   case GL_RGBA8:
   case GL_RGB10_A2:
   case GL_RGBA12:
   case GL_RGBA16:
      return &_mesa_texformat_argb8888;
   default:
      _mesa_problem(ctx, "unexpected format in tdfxDDChooseTextureFormat: %d",
         internalFormat);
      return NULL;
   }
}

static void sisDDTexImage1D( GLcontext *ctx, GLenum target, GLint level,
			     GLint internalFormat,
			     GLint width, GLint border,
			     GLenum format, GLenum type, const GLvoid *pixels,
			     const struct gl_pixelstore_attrib *packing,
			     struct gl_texture_object *texObj,
			     struct gl_texture_image *texImage )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   /* Note, this will call sisDDChooseTextureFormat */
   _mesa_store_teximage1d( ctx, target, level, internalFormat,
			   width, border, format, type,
			   pixels, packing, texObj, texImage );

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   memcpy(t->image[level].Data, texImage->Data, t->image[level].size);
   
   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}


static void sisDDTexSubImage1D( GLcontext *ctx,
				GLenum target,
				GLint level,
				GLint xoffset,
				GLsizei width,
				GLenum format, GLenum type,
				const GLvoid *pixels,
				const struct gl_pixelstore_attrib *packing,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;
   GLuint copySize;
   GLint texelBytes;
   char *src, *dst;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   _mesa_store_texsubimage1d(ctx, target, level, xoffset, width,
			     format, type, pixels, packing, texObj,
			     texImage);

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   texelBytes = texImage->TexFormat->TexelBytes;

   copySize = width * texelBytes;
   src = (char *)texImage->Data + xoffset * texelBytes;
   dst = t->image[level].Data + xoffset * texelBytes;

   memcpy( dst, src, copySize );

   smesa->clearTexCache = GL_TRUE;

   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void sisDDTexImage2D( GLcontext *ctx, GLenum target, GLint level,
			     GLint internalFormat,
			     GLint width, GLint height, GLint border,
			     GLenum format, GLenum type, const GLvoid *pixels,
			     const struct gl_pixelstore_attrib *packing,
			     struct gl_texture_object *texObj,
			     struct gl_texture_image *texImage )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   /* Note, this will call sisDDChooseTextureFormat */
   _mesa_store_teximage2d(ctx, target, level, internalFormat,
                          width, height, border, format, type, pixels,
                          &ctx->Unpack, texObj, texImage);

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   memcpy(t->image[level].Data, texImage->Data, t->image[level].size);
   
   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;
}

static void sisDDTexSubImage2D( GLcontext *ctx,
				GLenum target,
				GLint level,
				GLint xoffset, GLint yoffset,
				GLsizei width, GLsizei height,
				GLenum format, GLenum type,
				const GLvoid *pixels,
				const struct gl_pixelstore_attrib *packing,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   sisTexObjPtr t;
   GLuint copySize;
   GLint texelBytes;
   char *src, *dst;
   int j;
   GLuint soffset;

   if ( texObj->DriverData == NULL )
      sisAllocTexObj( texObj );
   t = texObj->DriverData;

   _mesa_store_texsubimage2d(ctx, target, level, xoffset, yoffset, width,
			     height, format, type, pixels, packing, texObj,
			     texImage);

   /* Allocate offscreen space for the texture */
   sisFreeTexImage(smesa, t, level);
   sisAllocTexImage(smesa, t, level, texImage);

   /* Upload the texture */
   texelBytes = texImage->TexFormat->TexelBytes;

   copySize = width * texelBytes;
   src = (char *)texImage->Data + (xoffset + yoffset * texImage->Width) *
      texelBytes;
   dst = t->image[level].Data + (xoffset + yoffset * texImage->Width) *
      texelBytes;
   soffset = texImage->Width * texelBytes;

   for (j = yoffset; j < yoffset + height; j++) {
      memcpy( dst, src, copySize );
      src += soffset;
      dst += soffset;
   }

   smesa->clearTexCache = GL_TRUE;

   if (smesa->PrevTexFormat[ctx->Texture.CurrentUnit] != t->format)
   {
      smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURE_ENV;
      smesa->PrevTexFormat[ctx->Texture.CurrentUnit] = t->format;
   }
   smesa->TexStates[ctx->Texture.CurrentUnit] |= NEW_TEXTURING;

}

void sisDDInitTextureFuncs( GLcontext *ctx )
{
   ctx->Driver.TexEnv			= sisDDTexEnv;
   ctx->Driver.ChooseTextureFormat	= sisDDChooseTextureFormat;
   ctx->Driver.TexImage1D		= sisDDTexImage1D;
   ctx->Driver.TexSubImage1D		= sisDDTexSubImage1D;
   ctx->Driver.TexImage2D		= sisDDTexImage2D;
   ctx->Driver.TexSubImage2D		= sisDDTexSubImage2D;
   ctx->Driver.TexImage3D               = _mesa_store_teximage3d;
   ctx->Driver.TexSubImage3D            = _mesa_store_texsubimage3d;
   ctx->Driver.CopyTexImage1D           = _swrast_copy_teximage1d;
   ctx->Driver.CopyTexImage2D           = _swrast_copy_teximage2d;
   ctx->Driver.CopyTexSubImage1D        = _swrast_copy_texsubimage1d;
   ctx->Driver.CopyTexSubImage2D        = _swrast_copy_texsubimage2d;
   ctx->Driver.CopyTexSubImage3D        = _swrast_copy_texsubimage3d;
   ctx->Driver.TestProxyTexImage        = _mesa_test_proxy_teximage;
   ctx->Driver.TexParameter		= sisDDTexParameter;
   ctx->Driver.BindTexture		= sisDDBindTexture;
   ctx->Driver.DeleteTexture		= sisDDDeleteTexture;
   ctx->Driver.IsTextureResident	= sisDDIsTextureResident;
   ctx->Driver.PrioritizeTexture	= NULL;
}
