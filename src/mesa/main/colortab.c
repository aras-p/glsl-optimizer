/* $Id: colortab.c,v 1.31 2000/12/09 20:51:12 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "swrast/s_span.h" /* XXX SWRAST hack */
#endif



/*
 * Given an internalFormat token passed to glColorTable,
 * return the corresponding base format.
 * Return -1 if invalid token.
 */
static GLint
base_colortab_format( GLenum format )
{
   switch (format) {
      case GL_ALPHA:
      case GL_ALPHA4:
      case GL_ALPHA8:
      case GL_ALPHA12:
      case GL_ALPHA16:
         return GL_ALPHA;
      case GL_LUMINANCE:
      case GL_LUMINANCE4:
      case GL_LUMINANCE8:
      case GL_LUMINANCE12:
      case GL_LUMINANCE16:
         return GL_LUMINANCE;
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
      case GL_RGB:
      case GL_R3_G3_B2:
      case GL_RGB4:
      case GL_RGB5:
      case GL_RGB8:
      case GL_RGB10:
      case GL_RGB12:
      case GL_RGB16:
         return GL_RGB;
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
_mesa_init_colortable( struct gl_color_table *p )
{
   p->FloatTable = GL_FALSE;
   /* allocate a width=1 table by default */
   p->Table = CALLOC(4 * sizeof(GLchan));
   if (p->Table) {
      GLchan *t = (GLchan *) p->Table;
      t[0] = CHAN_MAX;
      t[1] = CHAN_MAX;
      t[2] = CHAN_MAX;
      t[3] = CHAN_MAX;
   }
   p->Size = 0;
   p->IntFormat = GL_RGBA;
   p->Format = GL_RGBA;
   p->RedSize = CHAN_BITS;
   p->GreenSize = CHAN_BITS;
   p->BlueSize = CHAN_BITS;
   p->AlphaSize = CHAN_BITS;
   p->IntensitySize = 0;
   p->LuminanceSize = 0;
}



void
_mesa_free_colortable_data( struct gl_color_table *p )
{
   if (p->Table) {
      FREE(p->Table);
      p->Table = NULL;
   }
}


/*
 * Examine table's format and set the component sizes accordingly.
 */
static void
set_component_sizes( struct gl_color_table *table )
{
   switch (table->Format) {
      case GL_ALPHA:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = CHAN_BITS;
         table->IntensitySize = 0;
         table->LuminanceSize = 0;
         break;
      case GL_LUMINANCE:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = 0;
         table->IntensitySize = 0;
         table->LuminanceSize = CHAN_BITS;
         break;
      case GL_LUMINANCE_ALPHA:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = CHAN_BITS;
         table->IntensitySize = 0;
         table->LuminanceSize = CHAN_BITS;
         break;
      case GL_INTENSITY:
         table->RedSize = 0;
         table->GreenSize = 0;
         table->BlueSize = 0;
         table->AlphaSize = 0;
         table->IntensitySize = CHAN_BITS;
         table->LuminanceSize = 0;
         break;
      case GL_RGB:
         table->RedSize = CHAN_BITS;
         table->GreenSize = CHAN_BITS;
         table->BlueSize = CHAN_BITS;
         table->AlphaSize = 0;
         table->IntensitySize = 0;
         table->LuminanceSize = 0;
         break;
      case GL_RGBA:
         table->RedSize = CHAN_BITS;
         table->GreenSize = CHAN_BITS;
         table->BlueSize = CHAN_BITS;
         table->AlphaSize = CHAN_BITS;
         table->IntensitySize = 0;
         table->LuminanceSize = 0;
         break;
      default:
         gl_problem(NULL, "unexpected format in set_component_sizes");
   }
}



void 
_mesa_ColorTable( GLenum target, GLenum internalFormat,
                  GLsizei width, GLenum format, GLenum type,
                  const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj = NULL;
   struct gl_color_table *table = NULL;
   GLboolean proxy = GL_FALSE;
   GLint baseFormat;
   GLfloat rScale = 1.0, gScale = 1.0, bScale = 1.0, aScale = 1.0;
   GLfloat rBias  = 0.0, gBias  = 0.0, bBias  = 0.0, aBias  = 0.0;
   GLboolean floatTable = GL_FALSE;
   GLint comps;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glColorTable");

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
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
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable;
         floatTable = GL_TRUE;
         rScale = ctx->Pixel.ColorTableScale[0];
         gScale = ctx->Pixel.ColorTableScale[1];
         bScale = ctx->Pixel.ColorTableScale[2];
         aScale = ctx->Pixel.ColorTableScale[3];
         rBias = ctx->Pixel.ColorTableBias[0];
         gBias = ctx->Pixel.ColorTableBias[1];
         bBias = ctx->Pixel.ColorTableBias[2];
         aBias = ctx->Pixel.ColorTableBias[3];
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable;
         proxy = GL_TRUE;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         floatTable = GL_TRUE;
         rScale = ctx->Pixel.PCCTscale[0];
         gScale = ctx->Pixel.PCCTscale[1];
         bScale = ctx->Pixel.PCCTscale[2];
         aScale = ctx->Pixel.PCCTscale[3];
         rBias = ctx->Pixel.PCCTbias[0];
         gBias = ctx->Pixel.PCCTbias[1];
         bBias = ctx->Pixel.PCCTbias[2];
         aBias = ctx->Pixel.PCCTbias[3];
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyPostConvolutionColorTable;
         proxy = GL_TRUE;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         floatTable = GL_TRUE;
         rScale = ctx->Pixel.PCMCTscale[0];
         gScale = ctx->Pixel.PCMCTscale[1];
         bScale = ctx->Pixel.PCMCTscale[2];
         aScale = ctx->Pixel.PCMCTscale[3];
         rBias = ctx->Pixel.PCMCTbias[0];
         gBias = ctx->Pixel.PCMCTbias[1];
         bBias = ctx->Pixel.PCMCTbias[2];
         aBias = ctx->Pixel.PCMCTbias[3];
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyPostColorMatrixColorTable;
         proxy = GL_TRUE;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorTable(target)");
         return;
   }

   assert(table);

   if (!_mesa_is_legal_format_and_type(format, type) ||
       format == GL_INTENSITY) {
      gl_error(ctx, GL_INVALID_ENUM, "glColorTable(format or type)");
      return;
   }

   baseFormat = base_colortab_format(internalFormat);
   if (baseFormat < 0 || baseFormat == GL_COLOR_INDEX) {
      gl_error(ctx, GL_INVALID_ENUM, "glColorTable(internalFormat)");
      return;
   }

   if (width < 0 || (width != 0 && _mesa_bitcount(width) != 1)) {
      /* error */
      if (proxy) {
         table->Size = 0;
         table->IntFormat = (GLenum) 0;
         table->Format = (GLenum) 0;
      }
      else {
         char msg[100];
         sprintf(msg, "glColorTable(width=%d)", width);
         gl_error(ctx, GL_INVALID_VALUE, msg);
      }
      return;
   }

   if (width > ctx->Const.MaxColorTableSize) {
      if (proxy) {
         table->Size = 0;
         table->IntFormat = (GLenum) 0;
         table->Format = (GLenum) 0;
      }
      else {
         if (width > ctx->Const.MaxColorTableSize)
            gl_error(ctx, GL_TABLE_TOO_LARGE, "glColorTable(width)");
         else {
            char msg[100];
            sprintf(msg, "glColorTable(width=%d)", width);
            gl_error(ctx, GL_INVALID_VALUE, msg);
         }
      }
      return;
   }

   table->Size = width;
   table->IntFormat = internalFormat;
   table->Format = (GLenum) baseFormat;
   set_component_sizes(table);

   comps = _mesa_components_in_format(table->Format);
   assert(comps > 0);  /* error should have been caught sooner */

   if (!proxy) {
      /* free old table, if any */
      if (table->Table) {
         FREE(table->Table);
      }
      if (floatTable) {
         GLfloat tempTab[MAX_COLOR_TABLE_SIZE * 4];
         GLfloat *tableF;
         GLuint i;

         _mesa_unpack_float_color_span(ctx, width, table->Format,
                                       tempTab,  /* dest */
                                       format, type, data, &ctx->Unpack,
                                       0, GL_FALSE);

         table->FloatTable = GL_TRUE;
         table->Table = MALLOC(comps * width * sizeof(GLfloat));
         if (!table->Table) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glColorTable");
            return;
         }

         tableF = (GLfloat *) table->Table;

         switch (table->Format) {
            case GL_INTENSITY:
               for (i = 0; i < width; i++) {
                  tableF[i] = CLAMP(tempTab[i] * rScale + rBias, 0.0F, 1.0F);
               }
               break;
            case GL_LUMINANCE:
               for (i = 0; i < width; i++) {
                  tableF[i] = CLAMP(tempTab[i] * rScale + rBias, 0.0F, 1.0F);
               }
               break;
            case GL_ALPHA:
               for (i = 0; i < width; i++) {
                  tableF[i] = CLAMP(tempTab[i] * aScale + aBias, 0.0F, 1.0F);
               }
               break;
            case GL_LUMINANCE_ALPHA:
               for (i = 0; i < width; i++) {
                  tableF[i*2+0] = CLAMP(tempTab[i*2+0] * rScale + rBias, 0.0F, 1.0F);
                  tableF[i*2+1] = CLAMP(tempTab[i*2+1] * aScale + aBias, 0.0F, 1.0F);
               }
               break;
            case GL_RGB:
               for (i = 0; i < width; i++) {
                  tableF[i*3+0] = CLAMP(tempTab[i*3+0] * rScale + rBias, 0.0F, 1.0F);
                  tableF[i*3+1] = CLAMP(tempTab[i*3+1] * gScale + gBias, 0.0F, 1.0F);
                  tableF[i*3+2] = CLAMP(tempTab[i*3+2] * bScale + bBias, 0.0F, 1.0F);
               }
               break;
            case GL_RGBA:
               for (i = 0; i < width; i++) {
                  tableF[i*4+0] = CLAMP(tempTab[i*4+0] * rScale + rBias, 0.0F, 1.0F);
                  tableF[i*4+1] = CLAMP(tempTab[i*4+1] * gScale + gBias, 0.0F, 1.0F);
                  tableF[i*4+2] = CLAMP(tempTab[i*4+2] * bScale + bBias, 0.0F, 1.0F);
                  tableF[i*4+3] = CLAMP(tempTab[i*4+3] * aScale + aBias, 0.0F, 1.0F);
               }
               break;
            default:
               gl_problem(ctx, "Bad format in _mesa_ColorTable");
               return;
         }
      }
      else {
         /* store GLchan table */
         table->FloatTable = GL_FALSE;
         table->Table = MALLOC(comps * width * sizeof(GLchan));
         if (!table->Table) {
            gl_error(ctx, GL_OUT_OF_MEMORY, "glColorTable");
            return;
         }
         _mesa_unpack_chan_color_span(ctx, width, table->Format,
                                      table->Table,  /* dest */
                                      format, type, data,
                                      &ctx->Unpack, 0);
      } /* floatTable */
   } /* proxy */

   if (texObj || target == GL_SHARED_TEXTURE_PALETTE_EXT) {
      /* texture object palette, texObj==NULL means the shared palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
      }
   }
   
   ctx->NewState |= _NEW_PIXEL;
}



void
_mesa_ColorSubTable( GLenum target, GLsizei start,
                     GLsizei count, GLenum format, GLenum type,
                     const GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *texObj = NULL;
   struct gl_color_table *table = NULL;
   GLfloat rScale = 1.0, gScale = 1.0, bScale = 1.0, aScale = 1.0;
   GLfloat rBias  = 0.0, gBias  = 0.0, bBias  = 0.0, aBias  = 0.0;
   GLint comps;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glColorSubTable");

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         table = &texObj->Palette;
         break;
      case GL_TEXTURE_3D:
         texObj = texUnit->Current3D;
         table = &texObj->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable;
         rScale = ctx->Pixel.ColorTableScale[0];
         gScale = ctx->Pixel.ColorTableScale[1];
         bScale = ctx->Pixel.ColorTableScale[2];
         aScale = ctx->Pixel.ColorTableScale[3];
         rBias = ctx->Pixel.ColorTableBias[0];
         gBias = ctx->Pixel.ColorTableBias[1];
         bBias = ctx->Pixel.ColorTableBias[2];
         aBias = ctx->Pixel.ColorTableBias[3];
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         rScale = ctx->Pixel.PCCTscale[0];
         gScale = ctx->Pixel.PCCTscale[1];
         bScale = ctx->Pixel.PCCTscale[2];
         aScale = ctx->Pixel.PCCTscale[3];
         rBias = ctx->Pixel.PCCTbias[0];
         gBias = ctx->Pixel.PCCTbias[1];
         bBias = ctx->Pixel.PCCTbias[2];
         aBias = ctx->Pixel.PCCTbias[3];
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         rScale = ctx->Pixel.PCMCTscale[0];
         gScale = ctx->Pixel.PCMCTscale[1];
         bScale = ctx->Pixel.PCMCTscale[2];
         aScale = ctx->Pixel.PCMCTscale[3];
         rBias = ctx->Pixel.PCMCTbias[0];
         gBias = ctx->Pixel.PCMCTbias[1];
         bBias = ctx->Pixel.PCMCTbias[2];
         aBias = ctx->Pixel.PCMCTbias[3];
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorSubTable(target)");
         return;
   }

   assert(table);

   if (!_mesa_is_legal_format_and_type(format, type) ||
       format == GL_INTENSITY) {
      gl_error(ctx, GL_INVALID_ENUM, "glColorSubTable(format or type)");
      return;
   }

   if (count < 1) {
      gl_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }

   comps = _mesa_components_in_format(table->Format);
   assert(comps > 0);  /* error should have been caught sooner */

   if (start + count > table->Size) {
      gl_error(ctx, GL_INVALID_VALUE, "glColorSubTable(count)");
      return;
   }

   if (!table->Table) {
      gl_error(ctx, GL_OUT_OF_MEMORY, "glColorSubTable");
      return;
   }

   if (!table->FloatTable) {
      GLchan *dest = (GLchan *) table->Table + start * comps * sizeof(GLchan);
      _mesa_unpack_chan_color_span(ctx, count, table->Format, dest,
                                   format, type, data, &ctx->Unpack, 0);
   }
   else {
      GLfloat tempTab[MAX_COLOR_TABLE_SIZE * 4];
      GLfloat *tableF;
      GLuint i;

      ASSERT(table->FloatTable);

      _mesa_unpack_float_color_span(ctx, count, table->Format,
                                    tempTab,  /* dest */
                                    format, type, data, &ctx->Unpack,
                                    0, GL_FALSE);

      tableF = (GLfloat *) table->Table;

      switch (table->Format) {
         case GL_INTENSITY:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j] = CLAMP(tempTab[i] * rScale + rBias, 0.0F, 1.0F);
            }
            break;
         case GL_LUMINANCE:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j] = CLAMP(tempTab[i] * rScale + rBias, 0.0F, 1.0F);
            }
            break;
         case GL_ALPHA:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j] = CLAMP(tempTab[i] * aScale + aBias, 0.0F, 1.0F);
            }
            break;
         case GL_LUMINANCE_ALPHA:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j*2+0] = CLAMP(tempTab[i*2+0] * rScale + rBias, 0.0F, 1.0F);
               tableF[j*2+1] = CLAMP(tempTab[i*2+1] * aScale + aBias, 0.0F, 1.0F);
            }
            break;
         case GL_RGB:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j*3+0] = CLAMP(tempTab[i*3+0] * rScale + rBias, 0.0F, 1.0F);
               tableF[j*3+1] = CLAMP(tempTab[i*3+1] * gScale + gBias, 0.0F, 1.0F);
               tableF[j*3+2] = CLAMP(tempTab[i*3+2] * bScale + bBias, 0.0F, 1.0F);
            }
            break;
         case GL_RGBA:
            for (i = 0; i < count; i++) {
               GLuint j = start + i;
               tableF[j*4+0] = CLAMP(tempTab[i*4+0] * rScale + rBias, 0.0F, 1.0F);
               tableF[j*4+1] = CLAMP(tempTab[i*4+1] * gScale + gBias, 0.0F, 1.0F);
               tableF[j*4+2] = CLAMP(tempTab[i*4+2] * bScale + bBias, 0.0F, 1.0F);
               tableF[j*4+3] = CLAMP(tempTab[i*4+3] * aScale + aBias, 0.0F, 1.0F);
            }
            break;
         default:
            gl_problem(ctx, "Bad format in _mesa_ColorSubTable");
            return;
         }
   }

   if (texObj || target == GL_SHARED_TEXTURE_PALETTE_EXT) {
      /* per-texture object palette */
      if (ctx->Driver.UpdateTexturePalette) {
         (*ctx->Driver.UpdateTexturePalette)( ctx, texObj );
      }
   }
   
   ctx->NewState |= _NEW_PIXEL;
}



/* XXX not tested */
void
_mesa_CopyColorTable(GLenum target, GLenum internalformat,
                     GLint x, GLint y, GLsizei width)
{
   GLchan data[MAX_WIDTH][4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyColorTable");

   /* Select buffer to read from */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   if (width > MAX_WIDTH)
      width = MAX_WIDTH;

   /* read the data from framebuffer */
   RENDER_START(ctx);
   gl_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y, data );
   RENDER_FINISH(ctx);

   /* Restore reading from draw buffer (the default) */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );

   _mesa_ColorTable(target, internalformat, width,
                    GL_RGBA, GL_UNSIGNED_BYTE, data);
}



/* XXX not tested */
void
_mesa_CopyColorSubTable(GLenum target, GLsizei start,
                        GLint x, GLint y, GLsizei width)
{
   GLchan data[MAX_WIDTH][4];
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glCopyColorSubTable");

   /* Select buffer to read from */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->ReadBuffer,
                                 ctx->Pixel.DriverReadBuffer );

   if (width > MAX_WIDTH)
      width = MAX_WIDTH;

   /* read the data from framebuffer */
   RENDER_START(ctx);
   gl_read_rgba_span( ctx, ctx->ReadBuffer, width, x, y, data );
   RENDER_FINISH(ctx);

   /* Restore reading from draw buffer (the default) */
   (*ctx->Driver.SetReadBuffer)( ctx, ctx->DrawBuffer,
                                 ctx->Color.DriverDrawBuffer );

   _mesa_ColorSubTable(target, start, width, GL_RGBA, GL_UNSIGNED_BYTE, data);
}



void
_mesa_GetColorTable( GLenum target, GLenum format,
                     GLenum type, GLvoid *data )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *table = NULL;
   GLchan rgba[MAX_COLOR_TABLE_SIZE][4];
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetColorTable");

   if (ctx->NewState) {
      gl_update_state(ctx);
   }

   switch (target) {
      case GL_TEXTURE_1D:
         table = &texUnit->Current1D->Palette;
         break;
      case GL_TEXTURE_2D:
         table = &texUnit->Current2D->Palette;
         break;
      case GL_TEXTURE_3D:
         table = &texUnit->Current3D->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTable(target)");
         return;
   }

   assert(table);

   switch (table->Format) {
      case GL_ALPHA:
         if (table->FloatTable) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = 0;
               rgba[i][GCOMP] = 0;
               rgba[i][BCOMP] = 0;
               rgba[i][ACOMP] = (GLint) (tableF[i] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = 0;
               rgba[i][GCOMP] = 0;
               rgba[i][BCOMP] = 0;
               rgba[i][ACOMP] = tableUB[i];
            }
         }
         break;
      case GL_LUMINANCE:
         if (table->FloatTable) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = (GLint) (tableF[i] * CHAN_MAXF);
               rgba[i][GCOMP] = (GLint) (tableF[i] * CHAN_MAXF);
               rgba[i][BCOMP] = (GLint) (tableF[i] * CHAN_MAXF);
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i];
               rgba[i][GCOMP] = tableUB[i];
               rgba[i][BCOMP] = tableUB[i];
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         break;
      case GL_LUMINANCE_ALPHA:
         if (table->FloatTable) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = (GLint) (tableF[i*2+0] * CHAN_MAXF);
               rgba[i][GCOMP] = (GLint) (tableF[i*2+0] * CHAN_MAXF);
               rgba[i][BCOMP] = (GLint) (tableF[i*2+0] * CHAN_MAXF);
               rgba[i][ACOMP] = (GLint) (tableF[i*2+1] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i*2+0];
               rgba[i][GCOMP] = tableUB[i*2+0];
               rgba[i][BCOMP] = tableUB[i*2+0];
               rgba[i][ACOMP] = tableUB[i*2+1];
            }
         }
         break;
      case GL_INTENSITY:
         if (table->FloatTable) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = (GLint) (tableF[i] * CHAN_MAXF);
               rgba[i][GCOMP] = (GLint) (tableF[i] * CHAN_MAXF);
               rgba[i][BCOMP] = (GLint) (tableF[i] * CHAN_MAXF);
               rgba[i][ACOMP] = (GLint) (tableF[i] * CHAN_MAXF);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i];
               rgba[i][GCOMP] = tableUB[i];
               rgba[i][BCOMP] = tableUB[i];
               rgba[i][ACOMP] = tableUB[i];
            }
         }
         break;
      case GL_RGB:
         if (table->FloatTable) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = (GLint) (tableF[i*3+0] * CHAN_MAXF);
               rgba[i][GCOMP] = (GLint) (tableF[i*3+1] * CHAN_MAXF);
               rgba[i][BCOMP] = (GLint) (tableF[i*3+2] * CHAN_MAXF);
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i*3+0];
               rgba[i][GCOMP] = tableUB[i*3+1];
               rgba[i][BCOMP] = tableUB[i*3+2];
               rgba[i][ACOMP] = CHAN_MAX;
            }
         }
         break;
      case GL_RGBA:
         if (table->FloatTable) {
            const GLfloat *tableF = (const GLfloat *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = (GLint) (tableF[i*4+0] * CHAN_MAXF + 0.5F);
               rgba[i][GCOMP] = (GLint) (tableF[i*4+1] * CHAN_MAXF + 0.5F);
               rgba[i][BCOMP] = (GLint) (tableF[i*4+2] * CHAN_MAXF + 0.5F);
               rgba[i][ACOMP] = (GLint) (tableF[i*4+3] * CHAN_MAXF + 0.5F);
            }
         }
         else {
            const GLchan *tableUB = (const GLchan *) table->Table;
            for (i = 0; i < table->Size; i++) {
               rgba[i][RCOMP] = tableUB[i*4+0];
               rgba[i][GCOMP] = tableUB[i*4+1];
               rgba[i][BCOMP] = tableUB[i*4+2];
               rgba[i][ACOMP] = tableUB[i*4+3];
            }
         }
         break;
      default:
         gl_problem(ctx, "bad table format in glGetColorTable");
         return;
   }

   _mesa_pack_rgba_span(ctx, table->Size, (const GLchan (*)[]) rgba,
                        format, type, data, &ctx->Pack, GL_FALSE);
}



void
_mesa_ColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx, "glColorTableParameterfv");

   switch (target) {
      case GL_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            ctx->Pixel.ColorTableScale[0] = params[0];
            ctx->Pixel.ColorTableScale[1] = params[1];
            ctx->Pixel.ColorTableScale[2] = params[2];
            ctx->Pixel.ColorTableScale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.ColorTableBias[0] = params[0];
            ctx->Pixel.ColorTableBias[1] = params[1];
            ctx->Pixel.ColorTableBias[2] = params[2];
            ctx->Pixel.ColorTableBias[3] = params[3];
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            ctx->Pixel.PCCTscale[0] = params[0];
            ctx->Pixel.PCCTscale[1] = params[1];
            ctx->Pixel.PCCTscale[2] = params[2];
            ctx->Pixel.PCCTscale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.PCCTbias[0] = params[0];
            ctx->Pixel.PCCTbias[1] = params[1];
            ctx->Pixel.PCCTbias[2] = params[2];
            ctx->Pixel.PCCTbias[3] = params[3];
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI:
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            ctx->Pixel.PCMCTscale[0] = params[0];
            ctx->Pixel.PCMCTscale[1] = params[1];
            ctx->Pixel.PCMCTscale[2] = params[2];
            ctx->Pixel.PCMCTscale[3] = params[3];
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            ctx->Pixel.PCMCTbias[0] = params[0];
            ctx->Pixel.PCMCTbias[1] = params[1];
            ctx->Pixel.PCMCTbias[2] = params[2];
            ctx->Pixel.PCMCTbias[3] = params[3];
         }
         else {
            gl_error(ctx, GL_INVALID_ENUM, "glColorTableParameterfv(pname)");
            return;
         }
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glColorTableParameter(target)");
         return;
   }

   ctx->NewState |= _NEW_PIXEL;
}



void
_mesa_ColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   GLfloat fparams[4];
   if (pname == GL_COLOR_TABLE_SGI ||
       pname == GL_POST_CONVOLUTION_COLOR_TABLE_SGI ||
       pname == GL_POST_COLOR_MATRIX_COLOR_TABLE_SGI) {
      /* four values */
      fparams[0] = (GLfloat) params[0];
      fparams[1] = (GLfloat) params[1];
      fparams[2] = (GLfloat) params[2];
      fparams[3] = (GLfloat) params[3];
   }
   else {
      /* one values */
      fparams[0] = (GLfloat) params[0];
   }
   _mesa_ColorTableParameterfv(target, pname, fparams);
}



void
_mesa_GetColorTableParameterfv( GLenum target, GLenum pname, GLfloat *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *table = NULL;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetColorTableParameterfv");

   switch (target) {
      case GL_TEXTURE_1D:
         table = &texUnit->Current1D->Palette;
         break;
      case GL_TEXTURE_2D:
         table = &texUnit->Current2D->Palette;
         break;
      case GL_TEXTURE_3D:
         table = &texUnit->Current3D->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         table = &ctx->Texture.Proxy1D->Palette;
         break;
      case GL_PROXY_TEXTURE_2D:
         table = &ctx->Texture.Proxy2D->Palette;
         break;
      case GL_PROXY_TEXTURE_3D:
         table = &ctx->Texture.Proxy3D->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.ColorTableScale[0];
            params[1] = ctx->Pixel.ColorTableScale[1];
            params[2] = ctx->Pixel.ColorTableScale[2];
            params[3] = ctx->Pixel.ColorTableScale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.ColorTableBias[0];
            params[1] = ctx->Pixel.ColorTableBias[1];
            params[2] = ctx->Pixel.ColorTableBias[2];
            params[3] = ctx->Pixel.ColorTableBias[3];
            return;
         }
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.PCCTscale[0];
            params[1] = ctx->Pixel.PCCTscale[1];
            params[2] = ctx->Pixel.PCCTscale[2];
            params[3] = ctx->Pixel.PCCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.PCCTbias[0];
            params[1] = ctx->Pixel.PCCTbias[1];
            params[2] = ctx->Pixel.PCCTbias[2];
            params[3] = ctx->Pixel.PCCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyPostConvolutionColorTable;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = ctx->Pixel.PCMCTscale[0];
            params[1] = ctx->Pixel.PCMCTscale[1];
            params[2] = ctx->Pixel.PCMCTscale[2];
            params[3] = ctx->Pixel.PCMCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = ctx->Pixel.PCMCTbias[0];
            params[1] = ctx->Pixel.PCMCTbias[1];
            params[2] = ctx->Pixel.PCMCTbias[2];
            params[3] = ctx->Pixel.PCMCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyPostColorMatrixColorTable;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(target)");
         return;
   }

   assert(table);

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = table->IntFormat;
         break;
      case GL_COLOR_TABLE_WIDTH:
         *params = table->Size;
         break;
      case GL_COLOR_TABLE_RED_SIZE:
         *params = table->RedSize;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE:
         *params = table->GreenSize;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE:
         *params = table->BlueSize;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE:
         *params = table->AlphaSize;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE:
         *params = table->LuminanceSize;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE:
         *params = table->IntensitySize;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameterfv(pname)" );
         return;
   }
}



void
_mesa_GetColorTableParameteriv( GLenum target, GLenum pname, GLint *params )
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_color_table *table = NULL;

   ASSERT_OUTSIDE_BEGIN_END(ctx, "glGetColorTableParameteriv");

   switch (target) {
      case GL_TEXTURE_1D:
         table = &texUnit->Current1D->Palette;
         break;
      case GL_TEXTURE_2D:
         table = &texUnit->Current2D->Palette;
         break;
      case GL_TEXTURE_3D:
         table = &texUnit->Current3D->Palette;
         break;
      case GL_PROXY_TEXTURE_1D:
         table = &ctx->Texture.Proxy1D->Palette;
         break;
      case GL_PROXY_TEXTURE_2D:
         table = &ctx->Texture.Proxy2D->Palette;
         break;
      case GL_PROXY_TEXTURE_3D:
         table = &ctx->Texture.Proxy3D->Palette;
         break;
      case GL_SHARED_TEXTURE_PALETTE_EXT:
         table = &ctx->Texture.Palette;
         break;
      case GL_COLOR_TABLE:
         table = &ctx->ColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.ColorTableScale[0];
            params[1] = (GLint) ctx->Pixel.ColorTableScale[1];
            params[2] = (GLint) ctx->Pixel.ColorTableScale[2];
            params[3] = (GLint) ctx->Pixel.ColorTableScale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.ColorTableBias[0];
            params[1] = (GLint) ctx->Pixel.ColorTableBias[1];
            params[2] = (GLint) ctx->Pixel.ColorTableBias[2];
            params[3] = (GLint) ctx->Pixel.ColorTableBias[3];
            return;
         }
         break;
      case GL_PROXY_COLOR_TABLE:
         table = &ctx->ProxyColorTable;
         break;
      case GL_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->PostConvolutionColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.PCCTscale[0];
            params[1] = (GLint) ctx->Pixel.PCCTscale[1];
            params[2] = (GLint) ctx->Pixel.PCCTscale[2];
            params[3] = (GLint) ctx->Pixel.PCCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.PCCTbias[0];
            params[1] = (GLint) ctx->Pixel.PCCTbias[1];
            params[2] = (GLint) ctx->Pixel.PCCTbias[2];
            params[3] = (GLint) ctx->Pixel.PCCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_CONVOLUTION_COLOR_TABLE:
         table = &ctx->ProxyPostConvolutionColorTable;
         break;
      case GL_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->PostColorMatrixColorTable;
         if (pname == GL_COLOR_TABLE_SCALE_SGI) {
            params[0] = (GLint) ctx->Pixel.PCMCTscale[0];
            params[1] = (GLint) ctx->Pixel.PCMCTscale[1];
            params[2] = (GLint) ctx->Pixel.PCMCTscale[2];
            params[3] = (GLint) ctx->Pixel.PCMCTscale[3];
            return;
         }
         else if (pname == GL_COLOR_TABLE_BIAS_SGI) {
            params[0] = (GLint) ctx->Pixel.PCMCTbias[0];
            params[1] = (GLint) ctx->Pixel.PCMCTbias[1];
            params[2] = (GLint) ctx->Pixel.PCMCTbias[2];
            params[3] = (GLint) ctx->Pixel.PCMCTbias[3];
            return;
         }
         break;
      case GL_PROXY_POST_COLOR_MATRIX_COLOR_TABLE:
         table = &ctx->ProxyPostColorMatrixColorTable;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(target)");
         return;
   }

   assert(table);

   switch (pname) {
      case GL_COLOR_TABLE_FORMAT:
         *params = table->IntFormat;
         break;
      case GL_COLOR_TABLE_WIDTH:
         *params = table->Size;
         break;
      case GL_COLOR_TABLE_RED_SIZE:
         *params = table->RedSize;
         break;
      case GL_COLOR_TABLE_GREEN_SIZE:
         *params = table->GreenSize;
         break;
      case GL_COLOR_TABLE_BLUE_SIZE:
         *params = table->BlueSize;
         break;
      case GL_COLOR_TABLE_ALPHA_SIZE:
         *params = table->AlphaSize;
         break;
      case GL_COLOR_TABLE_LUMINANCE_SIZE:
         *params = table->LuminanceSize;
         break;
      case GL_COLOR_TABLE_INTENSITY_SIZE:
         *params = table->IntensitySize;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetColorTableParameteriv(pname)" );
         return;
   }
}
