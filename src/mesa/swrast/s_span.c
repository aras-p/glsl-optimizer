/* $Id: s_span.c,v 1.19 2001/11/19 01:18:28 brianp Exp $ */

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
 */


/*
 * pixel span rasterization:
 * These functions implement the rasterization pipeline.
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "mem.h"

#include "s_alpha.h"
#include "s_alphabuf.h"
#include "s_blend.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_fog.h"
#include "s_logic.h"
#include "s_masking.h"
#include "s_scissor.h"
#include "s_span.h"
#include "s_stencil.h"
#include "s_texture.h"



/*
 * Apply the current polygon stipple pattern to a span of pixels.
 */
static void
stipple_polygon_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                      GLubyte mask[] )
{
   const GLuint highbit = 0x80000000;
   GLuint i, m, stipple;

   stipple = ctx->PolygonStipple[y % 32];
   m = highbit >> (GLuint) (x % 32);

   for (i = 0; i < n; i++) {
      if ((m & stipple) == 0) {
         mask[i] = 0;
      }
      m = m >> 1;
      if (m == 0) {
         m = highbit;
      }
   }
}



/*
 * Clip a pixel span to the current buffer/window boundaries.
 * Return:  'n' such that pixel 'n', 'n+1' etc. are clipped,
 *           as a special case:
 *           0 = all pixels clipped
 */
static GLuint
clip_span( GLcontext *ctx, GLint n, GLint x, GLint y, GLubyte mask[] )
{
   /* Clip to top and bottom */
   if (y < 0 || y >= ctx->DrawBuffer->Height) {
      return 0;
   }

   /* Clip to the left */
   if (x < 0) {
      if (x + n <= 0) {
         /* completely off left side */
         return 0;
      }
      else {
         /* partially off left side */
         BZERO(mask, -x * sizeof(GLubyte));
      }
   }

   /* Clip to right */
   if (x + n > ctx->DrawBuffer->Width) {
      if (x >= ctx->DrawBuffer->Width) {
         /* completely off right side */
         return 0;
      }
      else {
         /* partially off right side */
         return ctx->DrawBuffer->Width - x;
      }
   }

   return n;
}



/*
 * Draw to more than one color buffer (or none).
 */
static void
multi_write_index_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLuint indexes[], const GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint bufferBit;

   if (ctx->Color.DrawBuffer == GL_NONE)
      return;

   /* loop over four possible dest color buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit = bufferBit << 1) {
      if (bufferBit & ctx->Color.DrawDestMask) {
         GLuint indexTmp[MAX_WIDTH];
         ASSERT(n < MAX_WIDTH);

         if (bufferBit == FRONT_LEFT_BIT)
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_LEFT);
         else if (bufferBit == FRONT_RIGHT_BIT)
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_RIGHT);
         else if (bufferBit == BACK_LEFT_BIT)
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_LEFT);
         else
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_RIGHT);

         /* make copy of incoming indexes */
         MEMCPY( indexTmp, indexes, n * sizeof(GLuint) );
         if (ctx->Color.IndexLogicOpEnabled) {
            _mesa_logicop_ci_span( ctx, n, x, y, indexTmp, mask );
         }
         if (ctx->Color.IndexMask == 0) {
            break;
         }
         else if (ctx->Color.IndexMask != 0xffffffff) {
            _mesa_mask_index_span( ctx, n, x, y, indexTmp );
         }
         (*swrast->Driver.WriteCI32Span)( ctx, n, x, y, indexTmp, mask );
      }
   }

   /* restore default dest buffer */
   (void) (*ctx->Driver.SetDrawBuffer)( ctx, ctx->Color.DriverDrawBuffer);
}



/*
 * Write a horizontal span of color index pixels to the frame buffer.
 * Stenciling, Depth-testing, etc. are done as needed.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         fog - array of fog factor values in [0,1]
 *         index - array of [n] color indexes
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON, or GL_BITMAP
 */
void
_mesa_write_index_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLdepth z[], const GLfloat fog[],
                        GLuint indexIn[], const GLint coverage[],
                        GLenum primitive )
{
   const GLuint modBits = FOG_BIT | BLEND_BIT | MASKING_BIT | LOGIC_OP_BIT;
   GLubyte mask[MAX_WIDTH];
   GLuint indexBackup[MAX_WIDTH];
   GLuint *index;  /* points to indexIn or indexBackup */
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if ((n = clip_span(ctx,n,x,y,mask)) == 0) {
         return;
      }
   }

   if ((primitive==GL_BITMAP && (swrast->_RasterMask & modBits))
       || (swrast->_RasterMask & MULTI_DRAW_BIT)) {
      /* Make copy of color indexes */
      MEMCPY( indexBackup, indexIn, n * sizeof(GLuint) );
      index = indexBackup;
   }
   else {
      index = indexIn;
   }


   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((n = _mesa_scissor_span( ctx, n, x, y, mask )) == 0) {
         return;
      }
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
         return;
      }
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      if (_mesa_depth_test_span( ctx, n, x, y, z, mask ) == 0)
         return;
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if (fog && !swrast->_PreferPixelFog)
         _mesa_fog_ci_pixels( ctx, n, fog, index );
      else
         _mesa_depth_fog_ci_pixels( ctx, n, z, index );
   }

   /* Antialias coverage application */
   if (coverage) {
      GLuint i;
      for (i = 0; i < n; i++) {
         ASSERT(coverage[i] < 16);
         index[i] = (index[i] & ~0xf) | coverage[i];
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      /* draw to zero or two or more buffers */
      multi_write_index_span( ctx, n, x, y, index, mask );
   }
   else {
      /* normal situation: draw to exactly one buffer */
      if (ctx->Color.IndexLogicOpEnabled) {
         _mesa_logicop_ci_span( ctx, n, x, y, index, mask );
      }

      if (ctx->Color.IndexMask == 0) {
         return;
      }
      else if (ctx->Color.IndexMask != 0xffffffff) {
         _mesa_mask_index_span( ctx, n, x, y, index );
      }

      /* write pixels */
      (*swrast->Driver.WriteCI32Span)( ctx, n, x, y, index, mask );
   }
}




void
_mesa_write_monoindex_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLdepth z[], const GLfloat fog[],
                            GLuint index, const GLint coverage[],
                            GLenum primitive )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLubyte mask[MAX_WIDTH];
   GLuint i;

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if ((n = clip_span( ctx, n, x, y, mask)) == 0) {
	 return;
      }
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((n = _mesa_scissor_span( ctx, n, x, y, mask )) == 0) {
         return;
      }
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	 return;
      }
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      if (_mesa_depth_test_span( ctx, n, x, y, z, mask ) == 0)
         return;
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   if (ctx->Color.DrawBuffer == GL_NONE) {
      /* write no pixels */
      return;
   }

   if (ctx->Fog.Enabled
       || ctx->Color.IndexLogicOpEnabled
       || ctx->Color.IndexMask != 0xffffffff
       || coverage) {
      /* different index per pixel */
      GLuint indexes[MAX_WIDTH];
      for (i = 0; i < n; i++) {
	 indexes[i] = index;
      }

      if (ctx->Fog.Enabled) {
 	 if (fog && !swrast->_PreferPixelFog)
 	    _mesa_fog_ci_pixels( ctx, n, fog, indexes );
 	 else
 	    _mesa_depth_fog_ci_pixels( ctx, n, z, indexes );
      }

      /* Antialias coverage application */
      if (coverage) {
         GLuint i;
         for (i = 0; i < n; i++) {
            ASSERT(coverage[i] < 16);
            indexes[i] = (indexes[i] & ~0xf) | coverage[i];
         }
      }

      if (swrast->_RasterMask & MULTI_DRAW_BIT) {
         /* draw to zero or two or more buffers */
         multi_write_index_span( ctx, n, x, y, indexes, mask );
      }
      else {
         /* normal situation: draw to exactly one buffer */
         if (ctx->Color.IndexLogicOpEnabled) {
            _mesa_logicop_ci_span( ctx, n, x, y, indexes, mask );
         }
         if (ctx->Color.IndexMask == 0) {
            return;
         }
         else if (ctx->Color.IndexMask != 0xffffffff) {
            _mesa_mask_index_span( ctx, n, x, y, indexes );
         }
         (*swrast->Driver.WriteCI32Span)( ctx, n, x, y, indexes, mask );
      }
   }
   else {
      /* same color index for all pixels */
      ASSERT(!ctx->Color.IndexLogicOpEnabled);
      ASSERT(ctx->Color.IndexMask == 0xffffffff);
      if (swrast->_RasterMask & MULTI_DRAW_BIT) {
         /* draw to zero or two or more buffers */
         GLuint indexes[MAX_WIDTH];
         for (i = 0; i < n; i++)
            indexes[i] = index;
         multi_write_index_span( ctx, n, x, y, indexes, mask );
      }
      else {
         /* normal situation: draw to exactly one buffer */
         (*swrast->Driver.WriteMonoCISpan)( ctx, n, x, y, index, mask );
      }
   }
}



/*
 * Draw to more than one RGBA color buffer (or none).
 */
static void
multi_write_rgba_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       CONST GLchan rgba[][4], const GLubyte mask[] )
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   GLuint bufferBit;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   if (ctx->Color.DrawBuffer == GL_NONE)
      return;

   /* loop over four possible dest color buffers */
   for (bufferBit = 1; bufferBit <= 8; bufferBit = bufferBit << 1) {
      if (bufferBit & ctx->Color.DrawDestMask) {
         GLchan rgbaTmp[MAX_WIDTH][4];
         ASSERT(n < MAX_WIDTH);

         if (bufferBit == FRONT_LEFT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_LEFT);
            ctx->DrawBuffer->Alpha = ctx->DrawBuffer->FrontLeftAlpha;
         }
         else if (bufferBit == FRONT_RIGHT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_FRONT_RIGHT);
            ctx->DrawBuffer->Alpha = ctx->DrawBuffer->FrontRightAlpha;
         }
         else if (bufferBit == BACK_LEFT_BIT) {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_LEFT);
            ctx->DrawBuffer->Alpha = ctx->DrawBuffer->BackLeftAlpha;
         }
         else {
            (void) (*ctx->Driver.SetDrawBuffer)( ctx, GL_BACK_RIGHT);
            ctx->DrawBuffer->Alpha = ctx->DrawBuffer->BackRightAlpha;
         }

         /* make copy of incoming colors */
         MEMCPY( rgbaTmp, rgba, 4 * n * sizeof(GLchan) );

         if (ctx->Color.ColorLogicOpEnabled) {
            _mesa_logicop_rgba_span( ctx, n, x, y, rgbaTmp, mask );
         }
         else if (ctx->Color.BlendEnabled) {
            _mesa_blend_span( ctx, n, x, y, rgbaTmp, mask );
         }
         if (colorMask == 0x0) {
            break;
         }
         else if (colorMask != 0xffffffff) {
            _mesa_mask_rgba_span( ctx, n, x, y, rgbaTmp );
         }

         (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y,
				       (const GLchan (*)[4]) rgbaTmp, mask );
         if (swrast->_RasterMask & ALPHABUF_BIT) {
            _mesa_write_alpha_span( ctx, n, x, y,
                                    (const GLchan (*)[4])rgbaTmp, mask );
         }
      }
   }

   /* restore default dest buffer */
   (void) (*ctx->Driver.SetDrawBuffer)( ctx, ctx->Color.DriverDrawBuffer );
}



/*
 * Apply fragment processing to a span of RGBA fragments.
 * Input:
 *         n - number of fragments in the span
 *         x,y - location of first (left) fragment
 *         fog - array of fog factor values in [0,1]
 */
void
_mesa_write_rgba_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLdepth z[], const GLfloat fog[],
                       GLchan rgbaIn[][4], const GLfloat coverage[],
                       GLenum primitive )
{
   const GLuint modBits = FOG_BIT | BLEND_BIT | MASKING_BIT |
                          LOGIC_OP_BIT | TEXTURE_BIT;
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLchan rgbaBackup[MAX_WIDTH][4];
   GLchan (*rgba)[4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if ((n = clip_span( ctx,n,x,y,mask)) == 0) {
         return;
      }
      if (mask[0] == 0)
         write_all = GL_FALSE;
   }

   if ((primitive==GL_BITMAP && (swrast->_RasterMask & modBits))
       || (swrast->_RasterMask & MULTI_DRAW_BIT)) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY( rgbaBackup, rgbaIn, 4 * n * sizeof(GLchan) );
      rgba = rgbaBackup;
   }
   else {
      rgba = rgbaIn;
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((n = _mesa_scissor_span( ctx, n, x, y, mask )) == 0) {
         return;
      }
      if (mask[0] == 0)
	write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (_mesa_alpha_test( ctx, n, (const GLchan (*)[4]) rgba, mask ) == 0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	 return;
      }
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = _mesa_depth_test_span( ctx, n, x, y, z, mask );
      if (m == 0) {
         return;
      }
      if (m < n) {
         write_all = GL_FALSE;
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if (fog && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels( ctx, n, fog, rgba );
      else
	 _mesa_depth_fog_rgba_pixels( ctx, n, z, rgba );
   }

   /* Antialias coverage application */
   if (coverage) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, n, x, y, (const GLchan (*)[4]) rgba, mask );
   }
   else {
      /* normal: write to exactly one buffer */
      /* logic op or blending */
      const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);

      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, n, x, y, rgba, mask );
      }
      else if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, n, x, y, rgba, mask );
      }

      /* Color component masking */
      if (colorMask == 0x0) {
         return;
      }
      else if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, n, x, y, rgba );
      }

      /* write pixels */
      (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y,
                                 (const GLchan (*)[4]) rgba,
                                 write_all ? ((const GLubyte *) NULL) : mask );

      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, n, x, y,
                                 (const GLchan (*)[4]) rgba,
                                 write_all ? ((const GLubyte *) NULL) : mask );
      }
   }
}



/*
 * Write a horizontal span of color pixels to the frame buffer.
 * The color is initially constant for the whole span.
 * Alpha-testing, stenciling, depth-testing, and blending are done as needed.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         fog - array of fog factor values in [0,1]
 *         r, g, b, a - the color of the pixels
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON or GL_BITMAP.
 */
void
_mesa_write_monocolor_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLdepth z[], const GLfloat fog[],
                            const GLchan color[4], const GLfloat coverage[],
                            GLenum primitive )
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   GLuint i;
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLchan rgba[MAX_WIDTH][4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if ((n = clip_span( ctx,n,x,y,mask)) == 0) {
	 return;
      }
      if (mask[0] == 0)
         write_all = GL_FALSE;
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((n = _mesa_scissor_span( ctx, n, x, y, mask )) == 0) {
         return;
      }
      if (mask[0] == 0)
         write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = color[ACOMP];
      }
      if (_mesa_alpha_test( ctx, n, (const GLchan (*)[4])rgba, mask ) == 0) {
	 return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	 return;
      }
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = _mesa_depth_test_span( ctx, n, x, y, z, mask );
      if (m == 0) {
         return;
      }
      if (m < n) {
         write_all = GL_FALSE;
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   if (ctx->Color.DrawBuffer == GL_NONE) {
      /* write no pixels */
      return;
   }

   if (ctx->Color.ColorLogicOpEnabled || colorMask != 0xffffffff ||
       (swrast->_RasterMask & (BLEND_BIT | FOG_BIT)) || coverage) {
      /* assign same color to each pixel */
      for (i = 0; i < n; i++) {
	 if (mask[i]) {
            COPY_CHAN4(rgba[i], color);
	 }
      }

      /* Per-pixel fog */
      if (ctx->Fog.Enabled) {
	 if (fog && !swrast->_PreferPixelFog)
	    _mesa_fog_rgba_pixels( ctx, n, fog, rgba );
	 else
	    _mesa_depth_fog_rgba_pixels( ctx, n, z, rgba );
      }

      /* Antialias coverage application */
      if (coverage) {
         GLuint i;
         for (i = 0; i < n; i++) {
            rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
         }
      }

      if (swrast->_RasterMask & MULTI_DRAW_BIT) {
         multi_write_rgba_span( ctx, n, x, y,
                                (const GLchan (*)[4]) rgba, mask );
      }
      else {
         /* normal: write to exactly one buffer */
         if (ctx->Color.ColorLogicOpEnabled) {
            _mesa_logicop_rgba_span( ctx, n, x, y, rgba, mask );
         }
         else if (ctx->Color.BlendEnabled) {
            _mesa_blend_span( ctx, n, x, y, rgba, mask );
         }

         /* Color component masking */
         if (colorMask == 0x0) {
            return;
         }
         else if (colorMask != 0xffffffff) {
            _mesa_mask_rgba_span( ctx, n, x, y, rgba );
         }

         /* write pixels */
         (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y,
                                (const GLchan (*)[4]) rgba,
                                 write_all ? ((const GLubyte *) NULL) : mask );
         if (swrast->_RasterMask & ALPHABUF_BIT) {
            _mesa_write_alpha_span( ctx, n, x, y,
                                 (const GLchan (*)[4]) rgba,
                                 write_all ? ((const GLubyte *) NULL) : mask );
         }
      }
   }
   else {
      /* same color for all pixels */
      ASSERT(!ctx->Color.BlendEnabled);
      ASSERT(!ctx->Color.ColorLogicOpEnabled);

      if (swrast->_RasterMask & MULTI_DRAW_BIT) {
         for (i = 0; i < n; i++) {
            if (mask[i]) {
               COPY_CHAN4(rgba[i], color);
            }
         }
         multi_write_rgba_span( ctx, n, x, y,
				(const GLchan (*)[4]) rgba, mask );
      }
      else {
         (*swrast->Driver.WriteMonoRGBASpan)( ctx, n, x, y, color, mask );
         if (swrast->_RasterMask & ALPHABUF_BIT) {
            _mesa_write_mono_alpha_span( ctx, n, x, y, (GLchan) color[ACOMP],
                                 write_all ? ((const GLubyte *) NULL) : mask );
         }
      }
   }
}



/*
 * Add specular color to base color.  This is used only when
 * GL_LIGHT_MODEL_COLOR_CONTROL = GL_SEPARATE_SPECULAR_COLOR.
 */
static void
add_colors(GLuint n, GLchan rgba[][4], CONST GLchan specular[][4] )
{
   GLuint i;
   for (i = 0; i < n; i++) {
#if CHAN_TYPE == GL_FLOAT
      /* no clamping */
      rgba[i][RCOMP] += specular[i][RCOMP];
      rgba[i][GCOMP] += specular[i][GCOMP];
      rgba[i][BCOMP] += specular[i][BCOMP];
#else
      GLint r = rgba[i][RCOMP] + specular[i][RCOMP];
      GLint g = rgba[i][GCOMP] + specular[i][GCOMP];
      GLint b = rgba[i][BCOMP] + specular[i][BCOMP];
      rgba[i][RCOMP] = (GLchan) MIN2(r, CHAN_MAX);
      rgba[i][GCOMP] = (GLchan) MIN2(g, CHAN_MAX);
      rgba[i][BCOMP] = (GLchan) MIN2(b, CHAN_MAX);
#endif
   }
}


/*
 * Write a horizontal span of textured pixels to the frame buffer.
 * The color of each pixel is different.
 * Depth-testing, stenciling, scissor-testing etc. should already 
 * have been done,
 * only if alpha-testing is used, depth-testing is still done in this
 * function.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         s, t - array of (s,t) texture coordinates for each pixel
 *         lambda - array of texture lambda values
 *         rgba - array of [n] color components
 *         mask - masked pixels
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON or GL_BITMAP.
 * Contributed by Klaus Niederkrueger.
 */
static void
masked_texture_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                     const GLdepth z[], const GLfloat fog[],
                     const GLfloat s[], const GLfloat t[],
                     const GLfloat u[], GLfloat lambda[],
                     GLchan rgbaIn[][4], CONST GLchan spec[][4],
                     const GLfloat coverage[], GLubyte mask[],
                     GLboolean write_all )
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   GLchan rgbaBackup[MAX_WIDTH][4];
   GLchan (*rgba)[4];   /* points to either rgbaIn or rgbaBackup */
   SWcontext *swrast = SWRAST_CONTEXT(ctx);


   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY(rgbaBackup, rgbaIn, 4 * n * sizeof(GLchan));
      rgba = rgbaBackup;
   }
   else {
      rgba = rgbaIn;
   }


   ASSERT(ctx->Texture._ReallyEnabled);
   _swrast_texture_fragments( ctx, 0, n, s, t, u, lambda,
			      (CONST GLchan (*)[4]) rgba, rgba );


   /* Texture with alpha test */
   if (ctx->Color.AlphaEnabled) {
      /* Do the alpha test */
      if (_mesa_alpha_test( ctx, n, (const GLchan (*)[4]) rgba, mask ) == 0) {
         return;
      }
      write_all = GL_FALSE;
      
      /* Depth test usually in 'rasterize_span' but if alpha test
         needed, we have to wait for that test before depth test can
         be done. */
      if (ctx->Stencil.Enabled) {
	/* first stencil test */
	if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	  return;
	}
	write_all = GL_FALSE;
      }
      else if (ctx->Depth.Test) {
	/* regular depth testing */
	GLuint m = _mesa_depth_test_span( ctx, n, x, y, z, mask );
	if (m == 0) {
	  return;
	}
	if (m < n) {
	  write_all = GL_FALSE;
	}
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;


   /* Add base and specular colors */
   if (spec &&
       (ctx->Fog.ColorSumEnabled ||
	(ctx->Light.Enabled &&
         ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)))
      add_colors( n, rgba, spec );   /* rgba = rgba + spec */

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if (fog && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels( ctx, n, fog, rgba );
      else
	 _mesa_depth_fog_rgba_pixels( ctx, n, z, rgba );
   }

   /* Antialias coverage application */
   if (coverage) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, n, x, y, (const GLchan (*)[4]) rgba, mask );
   }
   else {
      /* normal: write to exactly one buffer */
      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, n, x, y, rgba, mask );
      }
      else  if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, n, x, y, rgba, mask );
      }
      if (colorMask == 0x0) {
         return;
      }
      else if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, n, x, y, rgba );
      }

      (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y, (const GLchan (*)[4])rgba,
				    write_all ? NULL : mask );
      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, n, x, y, (const GLchan (*)[4]) rgba,
                                 write_all ? NULL : mask );
      }
   }
}


/*
 * As above but perform multiple stages of texture application.
 * Contributed by Klaus Niederkrueger.
 */
static void
masked_multitexture_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLdepth z[], const GLfloat fog[],
                          CONST GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH],
                          CONST GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH],
                          CONST GLfloat u[MAX_TEXTURE_UNITS][MAX_WIDTH],
                          GLfloat lambda[][MAX_WIDTH],
                          GLchan rgbaIn[MAX_TEXTURE_UNITS][4],
                          CONST GLchan spec[MAX_TEXTURE_UNITS][4],
                          const GLfloat coverage[], GLubyte mask[],
                          GLboolean write_all )
{
   GLchan rgbaBackup[MAX_WIDTH][4];
   GLchan (*rgba)[4];   /* points to either rgbaIn or rgbaBackup */
   GLuint i;
   const GLuint texUnits = ctx->Const.MaxTextureUnits;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);


   if ( (swrast->_RasterMask & MULTI_DRAW_BIT) || texUnits > 1) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY(rgbaBackup, rgbaIn, 4 * n * sizeof(GLchan));
      rgba = rgbaBackup;
   }
   else {
      rgba = rgbaIn;
   }


   ASSERT(ctx->Texture._ReallyEnabled);
   for (i = 0; i < texUnits; i++)
     _swrast_texture_fragments( ctx, i, n, s[i], t[i], u[i], lambda[i],
				(CONST GLchan (*)[4]) rgbaIn, rgba );

   /* Texture with alpha test */
   if (ctx->Color.AlphaEnabled) {
      /* Do the alpha test */
      if (_mesa_alpha_test( ctx, n, (const GLchan (*)[4])rgba, mask ) == 0) {
         return;
      }
      write_all = GL_FALSE;
      /* Depth test usually in 'rasterize_span' but if alpha test
         needed, we have to wait for that test before depth test can
         be done. */
      if (ctx->Stencil.Enabled) {
	/* first stencil test */
	if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	  return;
	}
	write_all = GL_FALSE;
      }
      else if (ctx->Depth.Test) {
	/* regular depth testing */
	GLuint m = _mesa_depth_test_span( ctx, n, x, y, z, mask );
	if (m == 0) {
	  return;
	}
	if (m < n) {
	  write_all = GL_FALSE;
	}
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;


   /* Add base and specular colors */
   if (spec &&
       (ctx->Fog.ColorSumEnabled ||
	(ctx->Light.Enabled &&
	 ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)))
      add_colors( n, rgba, spec );   /* rgba = rgba + spec */

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if (fog && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels( ctx, n, fog, rgba );
      else
	 _mesa_depth_fog_rgba_pixels( ctx, n, z, rgba );
   }

   /* Antialias coverage application */
   if (coverage) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, n, x, y, (const GLchan (*)[4]) rgba, mask );
   }
   else {
      /* normal: write to exactly one buffer */
      const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);

      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, n, x, y, rgba, mask );
      }
      else  if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, n, x, y, rgba, mask );
      }

      if (colorMask == 0x0) {
         return;
      }
      else if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, n, x, y, rgba );
      }

      (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y, (const GLchan (*)[4])rgba,
                                    write_all ? NULL : mask );
      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, n, x, y, (const GLchan (*)[4])rgba,
                                 write_all ? NULL : mask );
      }
   }
}


/*
 * Generate arrays of fragment colors, z, fog, texcoords, etc from a
 * triangle span object.  Then call the span/fragment processsing
 * functions in s_span.[ch].  This is used by a bunch of the textured
 * triangle functions.
 * Contributed by Klaus Niederkrueger.
 */
void
_mesa_rasterize_span(GLcontext *ctx, struct triangle_span *span)
{
   DEFARRAY(GLubyte, mask, MAX_WIDTH);
   DEFMARRAY(GLchan, rgba, MAX_WIDTH, 4);
   DEFMARRAY(GLchan, spec, MAX_WIDTH, 4);
   DEFARRAY(GLuint, index, MAX_WIDTH);
   DEFARRAY(GLuint, z, MAX_WIDTH);
   DEFARRAY(GLfloat, fog, MAX_WIDTH);
   DEFARRAY(GLfloat, sTex, MAX_WIDTH);
   DEFARRAY(GLfloat, tTex, MAX_WIDTH);
   DEFARRAY(GLfloat, rTex, MAX_WIDTH);
   DEFARRAY(GLfloat, lambda, MAX_WIDTH);
   DEFMARRAY(GLfloat, msTex, MAX_TEXTURE_UNITS, MAX_WIDTH);
   DEFMARRAY(GLfloat, mtTex, MAX_TEXTURE_UNITS, MAX_WIDTH);
   DEFMARRAY(GLfloat, mrTex, MAX_TEXTURE_UNITS, MAX_WIDTH);
   DEFMARRAY(GLfloat, mLambda, MAX_TEXTURE_UNITS, MAX_WIDTH);

   GLboolean write_all = GL_TRUE;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   CHECKARRAY(mask, return);
   CHECKARRAY(rgba, return);
   CHECKARRAY(spec, return);
   CHECKARRAY(index, return);
   CHECKARRAY(z, return);
   CHECKARRAY(fog, return);
   CHECKARRAY(sTex, return);
   CHECKARRAY(tTex, return);
   CHECKARRAY(rTex, return);
   CHECKARRAY(lambda, return);
   CHECKARRAY(msTex, return);
   CHECKARRAY(mtTex, return);
   CHECKARRAY(mrTex, return);
   CHECKARRAY(mLambda, return);

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, span->count);

   if (swrast->_RasterMask & WINCLIP_BIT) {
      if ((span->count = clip_span(ctx, span->count, span->x, span->y, mask))
	  == 0) {
	 return;
      }
      if (mask[0] == 0)
	write_all = GL_FALSE;
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((span->count = _mesa_scissor_span(ctx, span->count, span->x, span->y, mask )) == 0) {
         return;
      }
      if (mask[0] == 0)
         write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag) {
      stipple_polygon_span( ctx, span->count, span->x, span->y, mask );
      write_all = GL_FALSE;
   }



   if (span->activeMask & SPAN_Z) {
      if (ctx->Visual.depthBits <= 16) {
         GLuint i;
         GLfixed zval = span->z;
         for (i = 0; i < span->count; i++) {
            z[i] = FixedToInt(zval);
            zval += span->zStep;
         }
      }
      else {
         /* Deep Z buffer, no fixed->int shift */
         GLuint i;
         GLfixed zval = span->z;
         for (i = 0; i < span->count; i++) {
            z[i] = zval;
            zval += span->zStep;
         }
      }
   }

   /* Correct order: texturing --> alpha test --> depth test.  But if
      no alpha test needed, we can do here the depth test and
      potentially avoid some of the texturing (otherwise alpha test,
      depth test etc.  happens in masked_texture_span(). */
   if (!ctx->Color.AlphaEnabled) {
     if (ctx->Stencil.Enabled) {
       /* first stencil test */
       if (_mesa_stencil_and_ztest_span(ctx, span->count, span->x,
					span->y, z, mask) == GL_FALSE) {
	 return;
       }
       write_all = GL_FALSE;
     }
     else if (ctx->Depth.Test) {
       /* regular depth testing */
       GLuint m = _mesa_depth_test_span( ctx, span->count, span->x,
					 span->y, z, mask );
       if (m == 0) {
         return;
       }
       if (m < span->count) {
         write_all = GL_FALSE;
       }
     }
   }

   if (span->activeMask & SPAN_RGBA) {
      if (span->activeMask & SPAN_FLAT) {
         GLuint i;
         GLchan color[4];
         color[RCOMP] = FixedToChan(span->red);
         color[GCOMP] = FixedToChan(span->green);
         color[BCOMP] = FixedToChan(span->blue);
         color[ACOMP] = FixedToChan(span->alpha);
         for (i = 0; i < span->count; i++) {
            COPY_CHAN4(rgba[i], color);
         }
      }
      else {
         /* smooth interpolation */
#if CHAN_TYPE == GL_FLOAT
         GLfloat r = span->red;
         GLfloat g = span->green;
         GLfloat b = span->blue;
         GLfloat a = span->alpha;
#else
         GLfixed r = span->red;
         GLfixed g = span->green;
         GLfixed b = span->blue;
         GLfixed a = span->alpha;
#endif
         GLuint i;
         for (i = 0; i < span->count; i++) {
            rgba[i][RCOMP] = FixedToChan(r);
            rgba[i][GCOMP] = FixedToChan(g);
            rgba[i][BCOMP] = FixedToChan(b);
            rgba[i][ACOMP] = FixedToChan(a);
            r += span->redStep;
            g += span->greenStep;
            b += span->blueStep;
            a += span->alphaStep;
         }
      }
   }

   if (span->activeMask & SPAN_SPEC) {
      if (span->activeMask & SPAN_FLAT) {
         const GLchan r = FixedToChan(span->specRed);
         const GLchan g = FixedToChan(span->specGreen);
         const GLchan b = FixedToChan(span->specBlue);
         GLuint i;
         for (i = 0; i < span->count; i++) {
            spec[i][RCOMP] = r;
            spec[i][GCOMP] = g;
            spec[i][BCOMP] = b;
         }
      }
      else {
         /* smooth interpolation */
#if CHAN_TYPE == GL_FLOAT
         GLfloat r = span->specRed;
         GLfloat g = span->specGreen;
         GLfloat b = span->specBlue;
#else
         GLfixed r = span->specRed;
         GLfixed g = span->specGreen;
         GLfixed b = span->specBlue;
#endif
         GLuint i;
         for (i = 0; i < span->count; i++) {
            spec[i][RCOMP] = FixedToChan(r);
            spec[i][GCOMP] = FixedToChan(g);
            spec[i][BCOMP] = FixedToChan(b);
            r += span->specRedStep;
            g += span->specGreenStep;
            b += span->specBlueStep;
         }
      }
   }

   if (span->activeMask & SPAN_INDEX) {
      if (span->activeMask & SPAN_FLAT) {
         GLuint i;
         const GLint indx = FixedToInt(span->index);
         for (i = 0; i < span->count; i++) {
            index[i] = indx;
         }
      }
      else {
         /* smooth interpolation */
         GLuint i;
         GLfixed ind = span->index;
         for (i = 0; i < span->count; i++) {
            index[i] = FixedToInt(ind);
            ind += span->indexStep;
         }
      }
   }

   if (span->activeMask & SPAN_FOG) {
      GLuint i;
      GLfloat f = span->fog;
      for (i = 0; i < span->count; i++) {
         fog[i] = f;
         f += span->fogStep;
      }
   }
   if (span->activeMask & SPAN_TEXTURE) {
      if (ctx->Texture._ReallyEnabled & ~TEXTURE0_ANY) {
         /* multitexture */
         if (span->activeMask & SPAN_LAMBDA) {
            /* with lambda */
            GLuint u;
	    /* multitexture, lambda */
	    for (u = 0; u < MAX_TEXTURE_UNITS; u++) {
		if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  GLfloat s = span->tex[u][0];
                  GLfloat t = span->tex[u][1];
                  GLfloat r = span->tex[u][2];
                  GLfloat q = span->tex[u][3];
                  GLuint i;
                  for (i = 0; i < span->count; i++) {
		    const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
		    msTex[u][i] = s * invQ;
		    mtTex[u][i] = t * invQ;
		    mrTex[u][i] = r * invQ;
		    mLambda[u][i] = (GLfloat) 
		      (log(span->rho[u] * invQ * invQ) * 1.442695F * 0.5F);
		    s += span->texStep[u][0];
		    t += span->texStep[u][1];
		    r += span->texStep[u][2];
		    q += span->texStep[u][3];
                  }
		}
	      }
         }
         else {
	    /* without lambda */
	    GLuint u;
	    /* multitexture, no lambda */
	    for (u = 0; u < MAX_TEXTURE_UNITS; u++) {
		if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  GLfloat s = span->tex[u][0];
                  GLfloat t = span->tex[u][1];
                  GLfloat r = span->tex[u][2];
                  GLfloat q = span->tex[u][3];
                  GLuint i;
                  for (i = 0; i < span->count; i++) {
		    const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
		    msTex[u][i] = s * invQ;
		    mtTex[u][i] = t * invQ;
		    mrTex[u][i] = r * invQ;
		    s += span->texStep[u][0];
		    t += span->texStep[u][1];
		    r += span->texStep[u][2];
		    q += span->texStep[u][3];
                  }
		}
	      }
	 }
      }
      else {
         /* just texture unit 0 */
         if (span->activeMask & SPAN_LAMBDA) {
            /* with lambda */
            GLfloat s = span->tex[0][0];
            GLfloat t = span->tex[0][1];
            GLfloat r = span->tex[0][2];
            GLfloat q = span->tex[0][3];
            GLuint i;
	    /* single texture, lambda */
	    for (i = 0; i < span->count; i++) {
		const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
		sTex[i] = s * invQ;
		tTex[i] = t * invQ;
		rTex[i] = r * invQ;
		lambda[i] = (GLfloat)
		  (log(span->rho[0] * invQ * invQ) * 1.442695F * 0.5F);
		s += span->texStep[0][0];
		t += span->texStep[0][1];
		r += span->texStep[0][2];
		q += span->texStep[0][3];
	      }
         }
         else {
            /* without lambda */
            GLfloat s = span->tex[0][0];
            GLfloat t = span->tex[0][1];
            GLfloat r = span->tex[0][2];
            GLfloat q = span->tex[0][3];
            GLuint i;
	    /* single texture, no lambda */
	    for (i = 0; i < span->count; i++) {
		const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
		sTex[i] = s * invQ;
		tTex[i] = t * invQ;
		rTex[i] = r * invQ;
		s += span->texStep[0][0];
		t += span->texStep[0][1];
		r += span->texStep[0][2];
		q += span->texStep[0][3];
	    }
         }
      }
   }
   /* XXX keep this? */
   if (span->activeMask & SPAN_INT_TEXTURE) {
      GLint intTexcoord[MAX_WIDTH][2];
      GLfixed s = span->intTex[0];
      GLfixed t = span->intTex[1];
      GLuint i;
      for (i = 0; i < span->count; i++) {
         intTexcoord[i][0] = FixedToInt(s);
         intTexcoord[i][1] = FixedToInt(t);
         s += span->intTexStep[0];
         t += span->intTexStep[1];
      }
   }

   /* examine activeMask and call a s_span.c function */
   if (span->activeMask & SPAN_TEXTURE) {
      const GLfloat *fogPtr;
      if (span->activeMask & SPAN_FOG)
         fogPtr = fog;
      else
         fogPtr = NULL;

      if (ctx->Texture._ReallyEnabled & ~TEXTURE0_ANY) {
         if (span->activeMask & SPAN_SPEC) {
            masked_multitexture_span(ctx, span->count, span->x, span->y,
                                     z, fogPtr,
                                     (const GLfloat (*)[MAX_WIDTH]) msTex,
                                     (const GLfloat (*)[MAX_WIDTH]) mtTex,
                                     (const GLfloat (*)[MAX_WIDTH]) mrTex,
                                     (GLfloat (*)[MAX_WIDTH]) mLambda,
                                     rgba, (CONST GLchan (*)[4]) spec,
                                     NULL, mask, write_all );
         }
         else {
            masked_multitexture_span(ctx, span->count, span->x, span->y,
                                     z, fogPtr,
                                     (const GLfloat (*)[MAX_WIDTH]) msTex,
                                     (const GLfloat (*)[MAX_WIDTH]) mtTex,
                                     (const GLfloat (*)[MAX_WIDTH]) mrTex,
                                     (GLfloat (*)[MAX_WIDTH]) mLambda,
                                     rgba, NULL, NULL, mask, write_all );
         }
      }
      else {
         /* single texture */
         if (span->activeMask & SPAN_SPEC) {
            masked_texture_span(ctx, span->count, span->x, span->y,
                                z, fogPtr, sTex, tTex, rTex,
                                lambda, rgba,
                                (CONST GLchan (*)[4]) spec,
                                NULL, mask, write_all);
         }
         else {
            masked_texture_span(ctx, span->count, span->x, span->y,
                                z, fogPtr, sTex, tTex, rTex, 
                                lambda, rgba, NULL, NULL,
                                mask, write_all);
         }
      }
   }
   else {
      _mesa_problem(ctx, "rasterize_span() should only be used for texturing");
   }

   UNDEFARRAY(mask);
   UNDEFARRAY(rgba);
   UNDEFARRAY(spec);
   UNDEFARRAY(index);
   UNDEFARRAY(z);
   UNDEFARRAY(fog);
   UNDEFARRAY(sTex);
   UNDEFARRAY(tTex);
   UNDEFARRAY(rTex);
   UNDEFARRAY(lambda);
   UNDEFARRAY(msTex);
   UNDEFARRAY(mtTex);
   UNDEFARRAY(mrTex);
   UNDEFARRAY(mLambda);
}


/*
 * Write a horizontal span of textured pixels to the frame buffer.
 * The color of each pixel is different.
 * Alpha-testing, stenciling, depth-testing, and blending are done
 * as needed.
 * Input:  n - number of pixels in the span
 *         x, y - location of leftmost pixel in the span
 *         z - array of [n] z-values
 *         fog - array of fog factor values in [0,1]
 *         s, t - array of (s,t) texture coordinates for each pixel
 *         lambda - array of texture lambda values
 *         rgba - array of [n] color components
 *         primitive - either GL_POINT, GL_LINE, GL_POLYGON or GL_BITMAP.
 */
void
_mesa_write_texture_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                          const GLdepth z[], const GLfloat fog[],
                          const GLfloat s[], const GLfloat t[],
                          const GLfloat u[], GLfloat lambda[],
                          GLchan rgbaIn[][4], CONST GLchan spec[][4],
                          const GLfloat coverage[], GLenum primitive )
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLchan rgbaBackup[MAX_WIDTH][4];
   GLchan (*rgba)[4];   /* points to either rgbaIn or rgbaBackup */
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if ((n=clip_span(ctx, n, x, y, mask)) == 0) {
	 return;
      }
      if (mask[0] == 0)
	write_all = GL_FALSE;
   }


   if (primitive==GL_BITMAP || (swrast->_RasterMask & MULTI_DRAW_BIT)) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY(rgbaBackup, rgbaIn, 4 * n * sizeof(GLchan));
      rgba = rgbaBackup;
   }
   else {
      rgba = rgbaIn;
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((n = _mesa_scissor_span( ctx, n, x, y, mask )) == 0) {
         return;
      }
      if (mask[0] == 0)
         write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Texture with alpha test */
   if (ctx->Color.AlphaEnabled) {
      /* Texturing without alpha is done after depth-testing which
         gives a potential speed-up. */
      ASSERT(ctx->Texture._ReallyEnabled);
      _swrast_texture_fragments( ctx, 0, n, s, t, u, lambda,
                                 (CONST GLchan (*)[4]) rgba, rgba );

      /* Do the alpha test */
      if (_mesa_alpha_test( ctx, n, (const GLchan (*)[4]) rgba, mask ) == 0) {
         return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	 return;
      }
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = _mesa_depth_test_span( ctx, n, x, y, z, mask );
      if (m == 0) {
         return;
      }
      if (m < n) {
         write_all = GL_FALSE;
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* Texture without alpha test */
   if (! ctx->Color.AlphaEnabled) {
      ASSERT(ctx->Texture._ReallyEnabled);
      _swrast_texture_fragments( ctx, 0, n, s, t, u, lambda,
                                 (CONST GLchan (*)[4]) rgba, rgba );
   }

   /* Add base and specular colors */
   if (spec &&
       (ctx->Fog.ColorSumEnabled ||
	(ctx->Light.Enabled &&
         ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)))
      add_colors( n, rgba, spec );   /* rgba = rgba + spec */

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if (fog && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels( ctx, n, fog, rgba );
      else
	 _mesa_depth_fog_rgba_pixels( ctx, n, z, rgba );
   }

   /* Antialias coverage application */
   if (coverage) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, n, x, y, (const GLchan (*)[4]) rgba, mask );
   }
   else {
      /* normal: write to exactly one buffer */
      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, n, x, y, rgba, mask );
      }
      else  if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, n, x, y, rgba, mask );
      }
      if (colorMask == 0x0) {
         return;
      }
      else if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, n, x, y, rgba );
      }

      (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y, (const GLchan (*)[4])rgba,
                                 write_all ? ((const GLubyte *) NULL) : mask );
      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, n, x, y, (const GLchan (*)[4]) rgba,
                                 write_all ? ((const GLubyte *) NULL) : mask );
      }
   }
}



/*
 * As above but perform multiple stages of texture application.
 */
void
_mesa_write_multitexture_span( GLcontext *ctx, GLuint n, GLint x, GLint y,
                               const GLdepth z[], const GLfloat fog[],
                               CONST GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH],
                               CONST GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH],
                               CONST GLfloat u[MAX_TEXTURE_UNITS][MAX_WIDTH],
                               GLfloat lambda[][MAX_WIDTH],
                               GLchan rgbaIn[MAX_TEXTURE_UNITS][4],
                               CONST GLchan spec[MAX_TEXTURE_UNITS][4],
                               const GLfloat coverage[],
                               GLenum primitive )
{
   GLubyte mask[MAX_WIDTH];
   GLboolean write_all = GL_TRUE;
   GLchan rgbaBackup[MAX_WIDTH][4];
   GLchan (*rgba)[4];   /* points to either rgbaIn or rgbaBackup */
   GLuint i;
   const GLubyte *Null = 0;
   const GLuint texUnits = ctx->Const.MaxTextureUnits;
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* init mask to 1's (all pixels are to be written) */
   MEMSET(mask, 1, n);

   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if ((n=clip_span(ctx, n, x, y, mask)) == 0) {
	 return;
      }
      if (mask[0] == 0)
	write_all = GL_FALSE;
   }


   if (primitive==GL_BITMAP || (swrast->_RasterMask & MULTI_DRAW_BIT)
                            || texUnits > 1) {
      /* must make a copy of the colors since they may be modified */
      MEMCPY(rgbaBackup, rgbaIn, 4 * n * sizeof(GLchan));
      rgba = rgbaBackup;
   }
   else {
      rgba = rgbaIn;
   }

   /* Do the scissor test */
   if (ctx->Scissor.Enabled) {
      if ((n = _mesa_scissor_span( ctx, n, x, y, mask )) == 0) {
         return;
      }
      if (mask[0] == 0)
         write_all = GL_FALSE;
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, n, x, y, mask );
      write_all = GL_FALSE;
   }

   /* Texture with alpha test */
   if (ctx->Color.AlphaEnabled) {
      /* Texturing without alpha is done after depth-testing which
       * gives a potential speed-up.
       */
      ASSERT(ctx->Texture._ReallyEnabled);
      for (i = 0; i < texUnits; i++)
         _swrast_texture_fragments( ctx, i, n, s[i], t[i], u[i], lambda[i],
                                    (CONST GLchan (*)[4]) rgbaIn, rgba );

      /* Do the alpha test */
      if (_mesa_alpha_test( ctx, n, (const GLchan (*)[4])rgba, mask ) == 0) {
         return;
      }
      write_all = GL_FALSE;
   }

   if (ctx->Stencil.Enabled) {
      /* first stencil test */
      if (_mesa_stencil_and_ztest_span(ctx, n, x, y, z, mask) == GL_FALSE) {
	 return;
      }
      write_all = GL_FALSE;
   }
   else if (ctx->Depth.Test) {
      /* regular depth testing */
      GLuint m = _mesa_depth_test_span( ctx, n, x, y, z, mask );
      if (m == 0) {
         return;
      }
      if (m < n) {
         write_all = GL_FALSE;
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* Texture without alpha test */
   if (! ctx->Color.AlphaEnabled) {
      ASSERT(ctx->Texture._ReallyEnabled);
      for (i = 0; i < texUnits; i++)
         _swrast_texture_fragments( ctx, i, n, s[i], t[i], u[i], lambda[i],
                                    (CONST GLchan (*)[4]) rgbaIn, rgba );
   }

   /* Add base and specular colors */
   if (spec &&
       (ctx->Fog.ColorSumEnabled ||
	(ctx->Light.Enabled &&
	 ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)))
      add_colors( n, rgba, spec );   /* rgba = rgba + spec */

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if (fog && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels( ctx, n, fog, rgba );
      else
	 _mesa_depth_fog_rgba_pixels( ctx, n, z, rgba );
   }

   /* Antialias coverage application */
   if (coverage) {
      GLuint i;
      for (i = 0; i < n; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, n, x, y, (const GLchan (*)[4]) rgba, mask );
   }
   else {
      /* normal: write to exactly one buffer */
      const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);

      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, n, x, y, rgba, mask );
      }
      else  if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, n, x, y, rgba, mask );
      }

      if (colorMask == 0x0) {
         return;
      }
      else if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, n, x, y, rgba );
      }

      (*swrast->Driver.WriteRGBASpan)( ctx, n, x, y, (const GLchan (*)[4])rgba,
                                    write_all ? Null : mask );
      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, n, x, y, (const GLchan (*)[4])rgba,
                                 write_all ? Null : mask );
      }
   }
}


/*
 * Read RGBA pixels from frame buffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void
_mesa_read_rgba_span( GLcontext *ctx, GLframebuffer *buffer,
                      GLuint n, GLint x, GLint y, GLchan rgba[][4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (y < 0 || y >= buffer->Height
       || x + (GLint) n < 0 || x >= buffer->Width) {
      /* completely above, below, or right */
      /* XXX maybe leave undefined? */
      BZERO(rgba, 4 * n * sizeof(GLchan));
   }
   else {
      GLint skip, length;
      if (x < 0) {
         /* left edge clippping */
         skip = -x;
         length = (GLint) n - skip;
         if (length < 0) {
            /* completely left of window */
            return;
         }
         if (length > buffer->Width) {
            length = buffer->Width;
         }
      }
      else if ((GLint) (x + n) > buffer->Width) {
         /* right edge clipping */
         skip = 0;
         length = buffer->Width - x;
         if (length < 0) {
            /* completely to right of window */
            return;
         }
      }
      else {
         /* no clipping */
         skip = 0;
         length = (GLint) n;
      }

      (*swrast->Driver.ReadRGBASpan)( ctx, length, x + skip, y, rgba + skip );
      if (buffer->UseSoftwareAlphaBuffers) {
         _mesa_read_alpha_span( ctx, length, x + skip, y, rgba + skip );
      }
   }
}




/*
 * Read CI pixels from frame buffer.  Clipping will be done to prevent
 * reading ouside the buffer's boundaries.
 */
void
_mesa_read_index_span( GLcontext *ctx, GLframebuffer *buffer,
                       GLuint n, GLint x, GLint y, GLuint indx[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   if (y < 0 || y >= buffer->Height
       || x + (GLint) n < 0 || x >= buffer->Width) {
      /* completely above, below, or right */
      BZERO(indx, n * sizeof(GLuint));
   }
   else {
      GLint skip, length;
      if (x < 0) {
         /* left edge clippping */
         skip = -x;
         length = (GLint) n - skip;
         if (length < 0) {
            /* completely left of window */
            return;
         }
         if (length > buffer->Width) {
            length = buffer->Width;
         }
      }
      else if ((GLint) (x + n) > buffer->Width) {
         /* right edge clipping */
         skip = 0;
         length = buffer->Width - x;
         if (length < 0) {
            /* completely to right of window */
            return;
         }
      }
      else {
         /* no clipping */
         skip = 0;
         length = (GLint) n;
      }

      (*swrast->Driver.ReadCI32Span)( ctx, length, skip + x, y, indx + skip );
   }
}
