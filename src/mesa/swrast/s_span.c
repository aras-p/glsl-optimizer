/* $Id: s_span.c,v 1.26 2002/01/28 03:42:28 brianp Exp $ */

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
 * Init span's Z interpolation values to the RasterPos Z.
 * Used during setup for glDraw/CopyPixels.
 */
void
_mesa_span_default_z( GLcontext *ctx, struct sw_span *span )
{
   if (ctx->Visual.depthBits <= 16)
      span->z = FloatToFixed(ctx->Current.RasterPos[2] * ctx->DepthMax);
   else
      span->z = (GLint) (ctx->Current.RasterPos[2] * ctx->DepthMax);
   span->zStep = 0;
   span->interpMask |= SPAN_Z;
}


/*
 * Init span's fog interpolation values to the RasterPos fog.
 * Used during setup for glDraw/CopyPixels.
 */
void
_mesa_span_default_fog( GLcontext *ctx, struct sw_span *span )
{
   if (ctx->Fog.FogCoordinateSource == GL_FOG_COORDINATE_EXT)
      span->fog = _mesa_z_to_fogfactor(ctx, ctx->Current.RasterFogCoord);
   else
      span->fog = _mesa_z_to_fogfactor(ctx, ctx->Current.RasterDistance);
   span->fogStep = 0;
   span->interpMask |= SPAN_FOG;
}


/*
 * Init span's color or index interpolation values to the RasterPos color.
 * Used during setup for glDraw/CopyPixels.
 */
void
_mesa_span_default_color( GLcontext *ctx, struct sw_span *span )
{
   if (ctx->Visual.rgbMode) {
      GLchan r, g, b, a;
      UNCLAMPED_FLOAT_TO_CHAN(r, ctx->Current.RasterColor[0]);
      UNCLAMPED_FLOAT_TO_CHAN(g, ctx->Current.RasterColor[1]);
      UNCLAMPED_FLOAT_TO_CHAN(b, ctx->Current.RasterColor[2]);
      UNCLAMPED_FLOAT_TO_CHAN(a, ctx->Current.RasterColor[3]);
#if CHAN_TYPE == GL_FLOAT
      span->red = r;
      span->green = g;
      span->blue = b;
      span->alpha = a;
#else
      span->red   = IntToFixed(r);
      span->green = IntToFixed(g);
      span->blue  = IntToFixed(b);
      span->alpha = IntToFixed(a);
#endif
      span->redStep = 0;
      span->greenStep = 0;
      span->blueStep = 0;
      span->alphaStep = 0;
      span->interpMask |= SPAN_RGBA;
   }
   else {
      span->index = IntToFixed(ctx->Current.RasterIndex);
      span->indexStep = 0;
      span->interpMask |= SPAN_INDEX;
   }
}


/* Fill in the span.color.rgba array from the interpolation values */
static void
interpolate_colors(GLcontext *ctx, struct sw_span *span)
{
   GLfixed r = span->red;
   GLfixed g = span->green;
   GLfixed b = span->blue;
   GLfixed a = span->alpha;
   const GLint dr = span->redStep;
   const GLint dg = span->greenStep;
   const GLint db = span->blueStep;
   const GLint da = span->alphaStep;
   const GLuint n = span->end;
   GLchan (*rgba)[4] = span->color.rgba;
   GLuint i;

   ASSERT(span->interpMask & SPAN_RGBA);

   if (span->interpMask & SPAN_FLAT) {
      /* constant color */
      GLchan color[4];
      color[RCOMP] = FixedToChan(r);
      color[GCOMP] = FixedToChan(g);
      color[BCOMP] = FixedToChan(b);
      color[ACOMP] = FixedToChan(a);
      for (i = 0; i < n; i++) {
         COPY_CHAN4(span->color.rgba[i], color);
      }
   }
   else {
      /* interpolate */
      for (i = 0; i < n; i++) {
         rgba[i][RCOMP] = FixedToChan(r);
         rgba[i][GCOMP] = FixedToChan(g);
         rgba[i][BCOMP] = FixedToChan(b);
         rgba[i][ACOMP] = FixedToChan(a);
         r += dr;
         g += dg;
         b += db;
         a += da;
      }
   }
   span->arrayMask |= SPAN_RGBA;
}


/* Fill in the span.color.index array from the interpolation values */
static void
interpolate_indexes(GLcontext *ctx, struct sw_span *span)
{
   GLfixed index = span->index;
   const GLint indexStep = span->indexStep;
   const GLuint n = span->end;
   GLuint *indexes = span->color.index;
   GLuint i;
   ASSERT(span->interpMask & SPAN_INDEX);

   if ((span->interpMask & SPAN_FLAT) || (indexStep == 0)) {
      /* constant color */
      index = FixedToInt(index);
      for (i = 0; i < n; i++) {
         indexes[i] = index;
      }
   }
   else {
      /* interpolate */
      for (i = 0; i < n; i++) {
         indexes[i] = FixedToInt(index);
         index += indexStep;
      }
   }
   span->arrayMask |= SPAN_INDEX;
}


/* Fill in the span.specArray array from the interpolation values */
static void
interpolate_specular(GLcontext *ctx, struct sw_span *span)
{
   if (span->interpMask & SPAN_FLAT) {
      /* constant color */
      const GLchan r = FixedToChan(span->specRed);
      const GLchan g = FixedToChan(span->specGreen);
      const GLchan b = FixedToChan(span->specBlue);
      GLuint i;
      for (i = 0; i < span->end; i++) {
         span->specArray[i][RCOMP] = r;
         span->specArray[i][GCOMP] = g;
         span->specArray[i][BCOMP] = b;
      }
   }
   else {
      /* interpolate */
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
      for (i = 0; i < span->end; i++) {
         span->specArray[i][RCOMP] = FixedToChan(r);
         span->specArray[i][GCOMP] = FixedToChan(g);
         span->specArray[i][BCOMP] = FixedToChan(b);
         r += span->specRedStep;
         g += span->specGreenStep;
         b += span->specBlueStep;
      }
   }
   span->arrayMask |= SPAN_SPEC;
}


/* Fill in the span.zArray array from the interpolation values */
static void
interpolate_z(GLcontext *ctx, struct sw_span *span)
{
   const GLuint n = span->end;
   GLuint i;

   ASSERT(span->interpMask & SPAN_Z);

   if (ctx->Visual.depthBits <= 16) {
      GLfixed zval = span->z;
      for (i = 0; i < n; i++) {
         span->zArray[i] = FixedToInt(zval);
         zval += span->zStep;
      }
   }
   else {
      /* Deep Z buffer, no fixed->int shift */
      GLfixed zval = span->z;
      for (i = 0; i < n; i++) {
         span->zArray[i] = zval;
         zval += span->zStep;
      }
   }
   span->arrayMask |= SPAN_Z;
}



/* Fill in the span.texcoords array from the interpolation values */
static void
interpolate_texcoords(GLcontext *ctx, struct sw_span *span)
{
   ASSERT(span->interpMask & SPAN_TEXTURE);

   if (ctx->Texture._ReallyEnabled & ~TEXTURE0_ANY) {
      if (span->interpMask & SPAN_LAMBDA) {
         /* multitexture, lambda */
         GLuint u;
         for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
            if (ctx->Texture.Unit[u]._ReallyEnabled) {
               const GLfloat ds = span->texStep[u][0];
               const GLfloat dt = span->texStep[u][1];
               const GLfloat dr = span->texStep[u][2];
               const GLfloat dq = span->texStep[u][3];
               GLfloat s = span->tex[u][0];
               GLfloat t = span->tex[u][1];
               GLfloat r = span->tex[u][2];
               GLfloat q = span->tex[u][3];
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
                  span->texcoords[u][i][0] = s * invQ;
                  span->texcoords[u][i][1] = t * invQ;
                  span->texcoords[u][i][2] = r * invQ;
                  span->lambda[u][i] = (GLfloat) 
                     (log(span->rho[u] * invQ * invQ) * 1.442695F * 0.5F);
                  s += ds;
                  t += dt;
                  r += dr;
                  q += dq;
               }
            }
         }
         span->arrayMask |= SPAN_LAMBDA;
      }
      else {
         /* multitexture, no lambda */
         GLuint u;
         for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
            if (ctx->Texture.Unit[u]._ReallyEnabled) {
               const GLfloat ds = span->texStep[u][0];
               const GLfloat dt = span->texStep[u][1];
               const GLfloat dr = span->texStep[u][2];
               const GLfloat dq = span->texStep[u][3];
               GLfloat s = span->tex[u][0];
               GLfloat t = span->tex[u][1];
               GLfloat r = span->tex[u][2];
               GLfloat q = span->tex[u][3];
               GLuint i;
               for (i = 0; i < span->end; i++) {
                  const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
                  span->texcoords[u][i][0] = s * invQ;
                  span->texcoords[u][i][1] = t * invQ;
                  span->texcoords[u][i][2] = r * invQ;
                  s += ds;
                  t += dt;
                  r += dr;
                  q += dq;
               }
            }
         }
      }
   }
   else {
      if (span->interpMask & SPAN_LAMBDA) {
         /* just texture unit 0, with lambda */
         const GLfloat ds = span->texStep[0][0];
         const GLfloat dt = span->texStep[0][1];
         const GLfloat dr = span->texStep[0][2];
         const GLfloat dq = span->texStep[0][3];
         GLfloat s = span->tex[0][0];
         GLfloat t = span->tex[0][1];
         GLfloat r = span->tex[0][2];
         GLfloat q = span->tex[0][3];
         GLuint i;
         for (i = 0; i < span->end; i++) {
            const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
            span->texcoords[0][i][0] = s * invQ;
            span->texcoords[0][i][1] = t * invQ;
            span->texcoords[0][i][2] = r * invQ;
            span->lambda[0][i] = (GLfloat)
               (log(span->rho[0] * invQ * invQ) * 1.442695F * 0.5F);
            s += ds;
            t += dt;
            r += dr;
            q += dq;
         }
         span->arrayMask |= SPAN_LAMBDA;
      }
      else {
         /* just texture 0, witout lambda */
         const GLfloat ds = span->texStep[0][0];
         const GLfloat dt = span->texStep[0][1];
         const GLfloat dr = span->texStep[0][2];
         const GLfloat dq = span->texStep[0][3];
         GLfloat s = span->tex[0][0];
         GLfloat t = span->tex[0][1];
         GLfloat r = span->tex[0][2];
         GLfloat q = span->tex[0][3];
         GLuint i;
         for (i = 0; i < span->end; i++) {
            const GLfloat invQ = (q == 0.0F) ? 1.0F : (1.0F / q);
            span->texcoords[0][i][0] = s * invQ;
            span->texcoords[0][i][1] = t * invQ;
            span->texcoords[0][i][2] = r * invQ;
            s += ds;
            t += dt;
            r += dr;
            q += dq;
         }
      }
   }
}


/*
 * Apply the current polygon stipple pattern to a span of pixels.
 */
static void
stipple_polygon_span( GLcontext *ctx, struct sw_span *span)
{
   const GLuint highbit = 0x80000000;
   GLuint i, m, stipple;

   stipple = ctx->PolygonStipple[span->y % 32];
   m = highbit >> (GLuint) (span->x % 32);

   for (i = 0; i < span->end; i++) {
      if ((m & stipple) == 0) {
	 span->mask[i] = 0;
      }
      m = m >> 1;
      if (m == 0) {
         m = highbit;
      }
   }
   span->writeAll = GL_FALSE;
}


/*
 * Clip a pixel span to the current buffer/window boundaries.
 * Return:   GL_TRUE   some pixel still visible
 *           GL_FALSE  nothing visible
 */
static GLuint
clip_span( GLcontext *ctx, struct sw_span *span)
{
   GLint x = span->x, y = span->y, n = span->end;

   /* Clip to top and bottom */
   if (y < 0 || y >= ctx->DrawBuffer->Height) {
      span->end = 0;
      return GL_FALSE;
   }

   /* Clip to the left */
   if (x < 0) {
      if (x + n <= 0) {
         /* completely off left side */
	 span->end = 0;
         return GL_FALSE;
      }
      else {
         /* partially off left side */
	 span->writeAll = GL_FALSE;
         BZERO(span->mask, -x * sizeof(GLubyte));
	 return GL_TRUE;
      }
   }

   /* Clip to right */
   if (x + n > ctx->DrawBuffer->Width) {
      if (x >= ctx->DrawBuffer->Width) {
         /* completely off right side */
	 span->end = 0;
         return GL_FALSE;
      }
      else {
         /* partially off right side */
         span->end = ctx->DrawBuffer->Width - x;
	 return GL_TRUE;
      }
   }

   return GL_TRUE;
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
         if (ctx->Color.IndexMask != 0xffffffff) {
            _mesa_mask_index_span( ctx, n, x, y, indexTmp );
         }
         (*swrast->Driver.WriteCI32Span)( ctx, n, x, y, indexTmp, mask );
      }
   }

   /* restore default dest buffer */
   (void) (*ctx->Driver.SetDrawBuffer)( ctx, ctx->Color.DriverDrawBuffer);
}


/*
 * Draw to more than one RGBA color buffer (or none).
 * All fragment operations, up to (but not) blending/logicop should
 * have been done first.
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
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_mesa_write_index_span( GLcontext *ctx, struct sw_span *span,
			GLenum primitive)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;

   ASSERT((span->interpMask & span->arrayMask) == 0);

   MEMSET(span->mask, 1, span->end);
   span->writeAll = GL_TRUE;

   /* Window clipping */
   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if (clip_span(ctx,span) == GL_FALSE) {
         return;
      }
   }

   /* Scissor test */
   if (ctx->Scissor.Enabled) {
      if (_mesa_scissor_span( ctx, span ) == GL_FALSE) {
         return;
      }
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span(ctx, span);
   }

   /* Depth test and stencil */
   if (ctx->Depth.Test || ctx->Stencil.Enabled) {
      if (span->interpMask & SPAN_Z)
         interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled) {
         if (_mesa_stencil_and_ztest_span(ctx, span) == GL_FALSE) {
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else {
         ASSERT(ctx->Depth.Test);
         if (_mesa_depth_test_span(ctx, span) == 0) {
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* we have to wait until after occlusion to do this test */
   if (ctx->Color.DrawBuffer == GL_NONE || ctx->Color.IndexMask == 0) {
      /* write no pixels */
      span->arrayMask = origArrayMask;
      return;
   }

   /* Interpolate the color indexes if needed */
   if (span->interpMask & SPAN_INDEX) {
      interpolate_indexes(ctx, span);
      /* clear the bit - this allows the WriteMonoCISpan optimization below */
      span->interpMask &= ~SPAN_INDEX;
   }

   /* Fog */
   if (ctx->Fog.Enabled) {
      if ((span->arrayMask & SPAN_FOG) && !swrast->_PreferPixelFog)
         _mesa_fog_ci_pixels_with_array( ctx, span, span->fogArray,
                                         span->color.index);
      else if ((span->interpMask & SPAN_FOG) && !swrast->_PreferPixelFog)
         _mesa_fog_ci_pixels( ctx, span, span->color.index);
      else
         _mesa_depth_fog_ci_pixels( ctx, span, span->color.index);
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      GLuint i;
      GLuint *index = span->color.index;
      for (i = 0; i < span->end; i++) {
         ASSERT(span->coverage[i] < 16);
         index[i] = (index[i] & ~0xf) | ((GLuint) (span->coverage[i]));
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      /* draw to zero or two or more buffers */
      multi_write_index_span( ctx, span->end, span->x, span->y,
			      span->color.index, span->mask );
   }
   else {
      /* normal situation: draw to exactly one buffer */
      if (ctx->Color.IndexLogicOpEnabled) {
         _mesa_logicop_ci_span( ctx, span->end, span->x, span->y,
				span->color.index, span->mask );
      }

      if (ctx->Color.IndexMask != 0xffffffff) {
         _mesa_mask_index_span( ctx, span->end, span->x, span->y,
                                span->color.index );
      }

      /* write pixels */
      if ((span->interpMask & SPAN_INDEX) && span->indexStep == 0) {
         /* all pixels have same color index */
         (*swrast->Driver.WriteMonoCISpan)( ctx, span->end, span->x, span->y,
                                            FixedToInt(span->index),
                                            span->mask );
      }
      else {
         (*swrast->Driver.WriteCI32Span)( ctx, span->end, span->x, span->y,
                                          span->color.index, span->mask );
      }
   }

   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
}


/*
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_mesa_write_rgba_span( GLcontext *ctx, struct sw_span *span,
		       GLenum primitive)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   const GLuint origInterpMask = span->interpMask;
   const GLuint origArrayMask = span->arrayMask;
   GLboolean monoColor;

   ASSERT((span->interpMask & span->arrayMask) == 0);

   MEMSET(span->mask, 1, span->end);
   span->writeAll = GL_TRUE;

   /* Determine if we have mono-chromatic colors */
   monoColor = (span->interpMask & SPAN_RGBA) &&
      span->redStep == 0 && span->greenStep == 0 &&
      span->blueStep == 0 && span->alphaStep == 0;

   /* Window clipping */
   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive == GL_BITMAP) {
      if (clip_span(ctx, span) == GL_FALSE) {
         return;
      }
   }

   /* Scissor test */
   if (ctx->Scissor.Enabled) {
      if (!_mesa_scissor_span(ctx, span)) {
         return;
      }
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive == GL_POLYGON) {
      stipple_polygon_span(ctx, span);
   }

   /* Now we may need to interpolate the colors */
   if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0) {
      interpolate_colors(ctx, span);
      /* clear the bit - this allows the WriteMonoCISpan optimization below */
      span->interpMask &= ~SPAN_RGBA;
   }

   /* Do the alpha test */
   if (ctx->Color.AlphaEnabled) {
      if (!_mesa_alpha_test(ctx, span,
                            (const GLchan (*)[4]) span->color.rgba)) {
         span->interpMask = origInterpMask;
         span->arrayMask = origArrayMask;
	 return;
      }
   }

   /* Stencil and Z testing */
   if (ctx->Stencil.Enabled || ctx->Depth.Test) {
      if (span->interpMask & SPAN_Z)
         interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled) {
         if (!_mesa_stencil_and_ztest_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else {
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         /* regular depth testing */
         if (!_mesa_depth_test_span(ctx, span)) {
            span->interpMask = origInterpMask;
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, something passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* can't abort span-writing until after occlusion testing */
   if (colorMask == 0x0) {
      span->interpMask = origInterpMask;
      span->arrayMask = origArrayMask;
      return;
   }

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
      if ((span->arrayMask & SPAN_FOG) && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels_with_array(ctx, span, span->fogArray,
                                          span->color.rgba);
      else if ((span->interpMask & SPAN_FOG) && !swrast->_PreferPixelFog)
         _mesa_fog_rgba_pixels(ctx, span, span->color.rgba);
      else {
         if ((span->interpMask & SPAN_Z) && (span->arrayMask & SPAN_Z) == 0)
            interpolate_z(ctx, span);
         _mesa_depth_fog_rgba_pixels(ctx, span, span->color.rgba);
      }
      monoColor = GL_FALSE;
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      GLchan (*rgba)[4] = span->color.rgba;
      GLuint i;
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * span->coverage[i]);
      }
      monoColor = GL_FALSE;
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, span->end, span->x, span->y,
                             (const GLchan (*)[4]) span->color.rgba,
                             span->mask );
   }
   else {
      /* normal: write to exactly one buffer */
      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, span->end, span->x, span->y,
                                  span->color.rgba, span->mask );
         monoColor = GL_FALSE;
      }
      else if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, span->end, span->x, span->y,
                           span->color.rgba, span->mask );
         monoColor = GL_FALSE;
      }

      /* Color component masking */
      if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, span->end, span->x, span->y,
                               span->color.rgba );
         monoColor = GL_FALSE;
      }

      /* write pixels */
      if (monoColor) {
         /* all pixels have same color */
         GLchan color[4];
         color[RCOMP] = FixedToChan(span->red);
         color[GCOMP] = FixedToChan(span->green);
         color[BCOMP] = FixedToChan(span->blue);
         color[ACOMP] = FixedToChan(span->alpha);
         (*swrast->Driver.WriteMonoRGBASpan)( ctx, span->end, span->x, span->y,
                                              color, span->mask);
      }
      else {
         (*swrast->Driver.WriteRGBASpan)( ctx, span->end, span->x, span->y,
                      (const GLchan (*)[4]) span->color.rgba,
                      span->writeAll ? ((const GLubyte *) NULL) : span->mask );
      }

      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, span->end, span->x, span->y,
                      (const GLchan (*)[4]) span->color.rgba,
                      span->writeAll ? ((const GLubyte *) NULL) : span->mask );
      }
   }

   span->interpMask = origInterpMask;
   span->arrayMask = origArrayMask;
}


/*
 * Add specular color to base color.  This is used only when
 * GL_LIGHT_MODEL_COLOR_CONTROL = GL_SEPARATE_SPECULAR_COLOR.
 */
static void
add_colors(GLuint n, GLchan rgba[][4], GLchan specular[][4] )
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
 * This function may modify any of the array values in the span.
 * span->interpMask and span->arrayMask may be changed but will be restored
 * to their original values before returning.
 */
void
_mesa_write_texture_span( GLcontext *ctx, struct sw_span *span,
                          GLenum primitive )
{
   const GLuint colorMask = *((GLuint *) ctx->Color.ColorMask);
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLuint origArrayMask = span->arrayMask;

   ASSERT((span->interpMask & span->arrayMask) == 0);

   /*   printf("%s()  interp 0x%x  array 0x%x\n", __FUNCTION__, span->interpMask, span->arrayMask);*/

   ASSERT(ctx->Texture._ReallyEnabled);

   MEMSET(span->mask, 1, span->end);
   span->writeAll = GL_TRUE;

   /* clip against window bounds */
   if ((swrast->_RasterMask & WINCLIP_BIT) || primitive==GL_BITMAP) {
      if (clip_span(ctx,span) == GL_FALSE) {
	 return;
      }
   }

   /* Scissor test */
   if (ctx->Scissor.Enabled) {
      if (_mesa_scissor_span( ctx, span ) == GL_FALSE) {
         return;
      }
   }

   /* Polygon Stippling */
   if (ctx->Polygon.StippleFlag && primitive==GL_POLYGON) {
      stipple_polygon_span( ctx, span);
   }

   /* Need texture coordinates now */
   if ((span->interpMask & SPAN_TEXTURE)
       && (span->arrayMask & SPAN_TEXTURE) == 0)
      interpolate_texcoords(ctx, span);

   /* Texture with alpha test */
   if (ctx->Color.AlphaEnabled) {

      /* Now we need the rgba array, fill it in if needed */
      if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0)
         interpolate_colors(ctx, span);

      /* Texturing without alpha is done after depth-testing which
       * gives a potential speed-up.
       */
      _swrast_multitexture_fragments( ctx, span );

      /* Do the alpha test */
      if (!_old_alpha_test(ctx, span->end,
                           (CONST GLchan (*)[4]) span->color.rgba,
                           span->mask)) {
         span->arrayMask = origArrayMask;
         return;
      }
   }

   /* Stencil and Z testing */
   if (ctx->Stencil.Enabled || ctx->Depth.Test) {
      if (span->interpMask & SPAN_Z)
         interpolate_z(ctx, span);

      if (ctx->Stencil.Enabled) {
         if (!_mesa_stencil_and_ztest_span(ctx, span)) {
            span->arrayMask = origArrayMask;
            return;
         }
      }
      else {
         ASSERT(ctx->Depth.Test);
         ASSERT(span->arrayMask & SPAN_Z);
         /* regular depth testing */
         if (!_mesa_depth_test_span(ctx, span)) {
            span->arrayMask = origArrayMask;
            return;
         }
      }
   }

   /* if we get here, some fragments passed the depth test */
   ctx->OcclusionResult = GL_TRUE;

   /* We had to wait until now to check for glColorMask(F,F,F,F) because of
    * the occlusion test.
    */
   if (colorMask == 0x0) {
      span->arrayMask = origArrayMask;
      return;
   }

   /* Texture without alpha test */
   if (!ctx->Color.AlphaEnabled) {

      /* Now we need the rgba array, fill it in if needed */
      if ((span->interpMask & SPAN_RGBA) && (span->arrayMask & SPAN_RGBA) == 0)
         interpolate_colors(ctx, span);

      _swrast_multitexture_fragments( ctx, span );
   }

   ASSERT(span->arrayMask & SPAN_RGBA);

   /* Add base and specular colors */
   if (ctx->Fog.ColorSumEnabled ||
       (ctx->Light.Enabled &&
        ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR)) {
      if (span->interpMask & SPAN_SPEC) {
         interpolate_specular(ctx, span);
      }
      ASSERT(span->arrayMask & SPAN_SPEC);
      add_colors( span->end, span->color.rgba, span->specArray );
   }

   /* Per-pixel fog */
   if (ctx->Fog.Enabled) {
#if 0
      if ((span->interpMask & SPAN_FOG) && (span->arrayMask & SPAN_FOG) == 0)
         interpolate_fog(ctx, span);
#endif
      if ((span->arrayMask & SPAN_FOG) && !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels_with_array( ctx, span, span->fogArray,
                                           span->color.rgba);
      else if ((span->interpMask & SPAN_FOG)  &&  !swrast->_PreferPixelFog)
	 _mesa_fog_rgba_pixels( ctx, span, span->color.rgba );
      else {
         if ((span->interpMask & SPAN_Z) && (span->arrayMask & SPAN_Z) == 0)
            interpolate_z(ctx, span);
	 _mesa_depth_fog_rgba_pixels(ctx, span, span->color.rgba);
      }
   }

   /* Antialias coverage application */
   if (span->arrayMask & SPAN_COVERAGE) {
      GLchan (*rgba)[4] = span->color.rgba;
      GLuint i;
      for (i = 0; i < span->end; i++) {
         rgba[i][ACOMP] = (GLchan) (rgba[i][ACOMP] * span->coverage[i]);
      }
   }

   if (swrast->_RasterMask & MULTI_DRAW_BIT) {
      multi_write_rgba_span( ctx, span->end, span->x, span->y,
                             (const GLchan (*)[4]) span->color.rgba,
                             span->mask );
   }
   else {
      /* normal: write to exactly one buffer */
      if (ctx->Color.ColorLogicOpEnabled) {
         _mesa_logicop_rgba_span( ctx, span->end, span->x, span->y,
                                  span->color.rgba, span->mask );
      }
      else  if (ctx->Color.BlendEnabled) {
         _mesa_blend_span( ctx, span->end, span->x, span->y,
                           span->color.rgba, span->mask);
      }

      if (colorMask != 0xffffffff) {
         _mesa_mask_rgba_span( ctx, span->end, span->x, span->y,
                               span->color.rgba );
      }

      (*swrast->Driver.WriteRGBASpan)( ctx, span->end, span->x, span->y,
                                       (const GLchan (*)[4]) span->color.rgba,
                                       span->writeAll ? NULL : span->mask );
      if (swrast->_RasterMask & ALPHABUF_BIT) {
         _mesa_write_alpha_span( ctx, span->end, span->x, span->y,
                                 (const GLchan (*)[4]) span->color.rgba,
                                 span->writeAll ? NULL : span->mask );
      }
   }

   span->arrayMask = origArrayMask;
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
