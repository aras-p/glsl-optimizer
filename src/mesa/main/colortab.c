/* $Id: colortab.c,v 1.12 2000/04/11 20:54:49 brianp Exp $ */

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
#include "colortab.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#endif



/*
 * Return GL_TRUE if k is a power of two, else return GL_FALSE.
 */
static GLboolean
power_of_two( GLint k )
{
   GLint i, m = 1;
   for (i=0; i<32; i++) {
      if (k == m)
         return GL_TRUE;
      m = m << 1;
   }
   return GL_FALSE;
}


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
      default:
         return -1;  /* error */
   }
}


void 
_mesa_ColorTable( GLenum target, GLenum internalFormat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj;
   struct gl_color_table *table;
   GLboolean proxy = GL_FALSE;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glColorTable");

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->CurrentD[1];
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->CurrentD[2];
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->CurrentD[3];
         table = &texObj->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         texObj = ctx->Texture.Proxy1D;
         table = &texObj->Palette;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_2D:
         texObj = ctx->Texture.Proxy2D;
         table = &texObj->Palette;
         proxy = GL_TRUE;
         break;
      case GL_PROXY_TEXTURE_3D:
         texObj = ctx->Texture.Proxy3D;
         table = &texObj->Palette;
         proxy = GL_TRUE;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         texObj = NULL;
         table = &ctx->Texture.Palette;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         texObj = NULL;
         table = &ctx->PostColorMatrixColorTable;
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         texObj = NULL;
         table = &ctx->ProxyPostColorMatrixColorTable;
         proxy = GL_TRUE;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
         return;
   }

   assert(table);

   if (!_mesa_is_legal_format_and_type(format, type)) {
      gl_error(ctx, GL_INVALID_ENUM, "glColorTable(format or type)");
      return;
   }

   if (decode_internal_format(internalFormat) < 0) {
      gl_error(ctx, GL_INVALID_ENUM, "glColorTable(internalFormat)");
      return;
   }

   if (width < 1 || width > MAX_TEXTURE_PALETTE_SIZE || !power_of_two(width)) {
      if (width > MAX_TEXTURE_PALETTE_SIZE)
         gl_error(ctx, GL_TABLE_TOO_LARGE, "glColorTable(width)");
      else
         gl_error(ctx, GL_INVALID_VALUE, "glColorTable(width)");
      if (proxy) {
         table->Size = 0;
         table->IntFormat = (GLenum) 0;
         table->Format = (GLenum) 0;
      }
      return;
   }


   table->Size = width;
   table->IntFormat = internalFormat;
   table->Format = (GLenum) decode_internal_format(internalFormat);
   if (!proxy) {
      _mesa_unpack_ubyte_color_span(ctx, width, table->Format,
                                    table->Table,  /* dest */
                                    format, type, data,
                                    &ctx->Unpack, GL_FALSE);
   }
   if (texObj) {
      /* per-texture object palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
      }
   }
   else {
      /* shared texture palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, NULL );
      }
   }
}



void
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *table )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj;
   struct gl_color_table *palette;
   GLint comps;
   GLubyte *dest;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glColorSubTable");

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->CurrentD[1];
         palette = &texObj->Palette;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->CurrentD[2];
         palette = &texObj->Palette;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->CurrentD[3];
         palette = &texObj->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         texObj = NULL;
         palette = &ctx->Texture.Palette;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
         return;
   }

   assert(palette);

   if (!_mesa_is_legal_format_and_type(format, type)) {
      gl_error(ctx, GL_INVALID_ENUM, "glColorSubTable(format or type)");
      return;
   }

   if (count < 1) {
      gl_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }

   comps = _mesa_components_in_format(format);
   assert(comps > 0);  /* error should be caught sooner */

   if (start + count > palette->Size) {
      gl_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }
   dest = palette->Table + start * comps * sizeof(GLubyte);
   _mesa_unpack_ubyte_color_span(ctx, count, palette->Format, dest,
                                 format, type, table,
                                 &ctx->Unpack, GL_FALSE);

   if (texObj) {
      /* per-texture object palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
      }
   }
   else {
      /* shared texture palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, NULL );
      }
   }
}



void
_mesa_GetColorTable( GLenum target, GLenum format,
                     GLenum type, GLvoid *table )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *palette;
   GLubyte rgba[MAX_TEXTURE_PALETTE_SIZE][4];
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetColorTable");

   switch (target) {
      case GL_TEXTURE_1D:
         palette = &texUnit->CurrentD[1]->Palette;
         break;
      case GL_TEXTURE_2D:
         palette = &texUnit->CurrentD[2]->Palette;
         break;
      case GL_TEXTURE_3D:
         palette = &texUnit->CurrentD[3]->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         palette = &ctx->Texture.Palette;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
         return;
   }

   assert(palette);

   switch (palette->Format) {
      case GL_ALPHA:
         for (i = 0; i < palette->Size; i++) {
            rgba[i][RCOMP] = 0;
            rgba[i][GCOMP] = 0;
            rgba[i][BCOMP] = 0;
            rgba[i][ACOMP] = palette->Table[i];
         }
         break;
      case GL_LUMINANCE:
         for (i = 0; i < palette->Size; i++) {
            rgba[i][RCOMP] = palette->Table[i];
            rgba[i][GCOMP] = palette->Table[i];
            rgba[i][BCOMP] = palette->Table[i];
            rgba[i][ACOMP] = 255;
         }
         break;
      case GL_LUMINANCE_ALPHA:
         for (i = 0; i < palette->Size; i++) {
            rgba[i][RCOMP] = palette->Table[i*2+0];
            rgba[i][GCOMP] = palette->Table[i*2+0];
            rgba[i][BCOMP] = palette->Table[i*2+0];
            rgba[i][ACOMP] = palette->Table[i*2+1];
         }
         break;
      case GL_INTENSITY:
         for (i = 0; i < palette->Size; i++) {
            rgba[i][RCOMP] = palette->Table[i];
            rgba[i][GCOMP] = palette->Table[i];
            rgba[i][BCOMP] = palette->Table[i];
            rgba[i][ACOMP] = 255;
         }
         break;
      case GL_RGB:
         for (i = 0; i < palette->Size; i++) {
            rgba[i][RCOMP] = palette->Table[i*3+0];
            rgba[i][GCOMP] = palette->Table[i*3+1];
            rgba[i][BCOMP] = palette->Table[i*3+2];
            rgba[i][ACOMP] = 255;
         }
         break;
      case GL_RGBA:
         for (i = 0; i < palette->Size; i++) {
            rgba[i][RCOMP] = palette->Table[i*4+0];
            rgba[i][GCOMP] = palette->Table[i*4+1];
            rgba[i][BCOMP] = palette->Table[i*4+2];
            rgba[i][ACOMP] = palette->Table[i*4+3];
         }
         break;
      default:
         gl_problem(ctx, "bad palette format in glGetColorTable");
         return;
   }

   _mesa_pack_rgba_span(ctx, palette->Size, (const GLubyte (*)[]) rgba,
                        format, type, table, &ctx->Pack, GL_FALSE);

   (void) format;
   (void) type;
   (void) table;
}



void
_mesa_GetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GLint iparams[10];
   _mesa_GetColorTableParameteriv( target, pname, iparams );
   *params = (GLfloat) iparams[0];
}



void
_mesa_GetColorTableParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *palette;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetColorTableParameter");

   switch (target) {
      case GL_TEXTURE_1D:
         palette = &texUnit->CurrentD[1]->Palette;
         break;
      case GL_TEXTURE_2D:
         palette = &texUnit->CurrentD[2]->Palette;
         break;
      case GL_TEXTURE_3D:
         palette = &texUnit->CurrentD[3]->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         palette = &ctx->Texture.Proxy1D->Palette;
         break;
      case GL_PROXY_TEXTURE_2D:
         palette = &ctx->Texture.Proxy2D->Palette;
         break;
      case GL_PROXY_TEXTURE_3D:
         palette = &ctx->Texture.Proxy3D->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         palette = &ctx->Texture.Palette;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter(target)");
         return;
   }

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = palette->IntFormat;
         break;
      case GL_COLOR_TABLE_WIDTH:
         *params = palette->Size;
         break;
      case GL_COLOR_TABLE_RED_SIZE:
         *params = 8;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE:
         *params = 8;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE:
         *params = 8;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE:
         *params = 8;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE:
         *params = 8;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE:
         *params = 8;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameter" );
         return;
   }
}


