/* $Id: colortab.c,v 1.3 1999/11/08 07:36:43 brianp Exp $ */

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
/* $XFree86: xc/lib/GL/mesa/src/colortab.c,v 1.2 1999/04/04 00:20:21 dawes Exp $ */





#ifdef PC_HEADER
#include "all.h"
#else
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#include "colortab.h"
#include "context.h"
#include "macros.h"
#endif



/*
 * Return GL_TRUE if k is a power of two, else return GL_FALSE.
 */
static GLboolean power_of_two( GLint k )
{
   GLint i, m = 1;
   for (i=0; i<32; i++) {
      if (k == m)
         return GL_TRUE;
      m = m << 1;
   }
   return GL_FALSE;
}


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
      default:
         return -1;  /* error */
   }
}


void gl_ColorTable( GLcontext *ctx, GLenum target,
                    GLenum internalFormat, struct gl_image *table )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj;
   GLboolean proxy = GL_FALSE;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glColorTable");

   if (decode_internal_format(internalFormat) < 0) {
      gl_error( ctx, GL_INVALID_ENUM, "glColorTable(internalFormat)" );
      return;
   }

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->CurrentD[1];
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->CurrentD[2];
         break;
      case GL_TEXTURE_3D_EXT:
         texObj = texUnit->CurrentD[3];
         break;
      case GL_PROXY_TEXTURE_1D:
         texObj = ctx->Texture.Proxy1D;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_2D:
         texObj = ctx->Texture.Proxy2D;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_3D_EXT:
         texObj = ctx->Texture.Proxy3D;
         proxy = GL_TRUE;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         texObj = NULL;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorTableEXT(target)");
         return;
   }

   /* internalformat = just like glTexImage */

   if (table->Width < 1 || table->Width > MAX_TEXTURE_PALETTE_SIZE
       || !power_of_two(table->Width)) {
      gl_error(ctx, GL_INVALID_VALUE, "glColorTableEXT(width)");
      if (proxy) {
         texObj->PaletteSize = 0;
         texObj->PaletteIntFormat = (GLenum) 0;
         texObj->PaletteFormat = (GLenum) 0;
      }
      return;
   }

   if (texObj) {
      /* per-texture object palette */
      texObj->PaletteSize = table->Width;
      texObj->PaletteIntFormat = internalFormat;
      texObj->PaletteFormat = (GLenum) decode_internal_format(internalFormat);
      if (!proxy) {
         MEMCPY(texObj->Palette, table->Data, table->Width*table->Components);
         if (ctx->Driver.UpdateTexturePalette) {
            (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
         }
      }
   }
   else {
      /* shared texture palette */
      ctx->Texture.PaletteSize = table->Width;
      ctx->Texture.PaletteIntFormat = internalFormat;
      ctx->Texture.PaletteFormat = (GLenum) decode_internal_format(internalFormat);
      MEMCPY(ctx->Texture.Palette, table->Data, table->Width*table->Components);
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, NULL );
      }
   }
}



void gl_ColorSubTable( GLcontext *ctx, GLenum target,
                       GLsizei start, struct gl_image *data )
{
   /* XXX TODO */
   gl_problem(ctx, "glColorSubTableEXT not implemented");
   (void) target;
   (void) start;
   (void) data;
}



void gl_GetColorTable( GLcontext *ctx, GLenum target, GLenum format,
                       GLenum type, GLvoid *table )
{
   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetBooleanv");

   switch (target) {
      case GL_TEXTURE_1D:
         break;
      case GL_TEXTURE_2D:
         break;
      case GL_TEXTURE_3D_EXT:
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableEXT(target)");
         return;
   }

   gl_problem(ctx, "glGetColorTableEXT not implemented!");
   (void) format;
   (void) type;
   (void) table;
}



void gl_GetColorTableParameterfv( GLcontext *ctx, GLenum target,
                                  GLenum pname, GLfloat *params )
{
   GLint iparams[10];

   gl_GetColorTableParameteriv( ctx, target, pname, iparams );
   *params = (GLfloat) iparams[0];
}



void gl_GetColorTableParameteriv( GLcontext *ctx, GLenum target,
                                  GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetColorTableParameter");

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->CurrentD[1];
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->CurrentD[2];
         break;
      case GL_TEXTURE_3D_EXT:
         texObj = texUnit->CurrentD[3];
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         texObj = NULL;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
         return;
   }

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT_EXT:
         if (texObj)
            *params = texObj->PaletteIntFormat;
         else
            *params = ctx->Texture.PaletteIntFormat;
         break;
      case GL_COLOR_TABLE_WIDTH_EXT:
         if (texObj)
            *params = texObj->PaletteSize;
         else
            *params = ctx->Texture.PaletteSize;
         break;
      case GL_COLOR_TABLE_RED_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE_EXT:
         *params = 8;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE_EXT:
         *params = 8;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter" );
         return;
   }
}


