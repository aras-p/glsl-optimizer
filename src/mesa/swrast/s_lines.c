/* $Id: s_lines.c,v 1.28 2002/04/19 00:38:27 brianp Exp $ */

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


#include "glheader.h"
#include "colormac.h"
#include "macros.h"
#include "mmath.h"
#include "s_aaline.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_feedback.h"
#include "s_lines.h"
#include "s_span.h"


/*
 * Init the mask[] array to implement a line stipple.
 */
static void
compute_stipple_mask( GLcontext *ctx, GLuint len, GLubyte mask[] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   GLuint i;

   for (i = 0; i < len; i++) {
      GLuint bit = (swrast->StippleCounter / ctx->Line.StippleFactor) & 0xf;
      if ((1 << bit) & ctx->Line.StipplePattern) {
         mask[i] = GL_TRUE;
      }
      else {
         mask[i] = GL_FALSE;
      }
      swrast->StippleCounter++;
   }
}


/*
 * To draw a wide line we can simply redraw the span N times, side by side.
 */
static void
draw_wide_line( GLcontext *ctx, struct sw_span *span, GLboolean xMajor )
{
   GLint width, start;

   ASSERT(span->end < MAX_WIDTH);

   width = (GLint) CLAMP( ctx->Line.Width, MIN_LINE_WIDTH, MAX_LINE_WIDTH );

   if (width & 1)
      start = width / 2;
   else
      start = width / 2 - 1;

   if (xMajor) {
      GLuint i;
      GLint w;
      for (w = 0; w < width; w++) {
         if (w == 0) {
            for (i = 0; i < span->end; i++)
               span->yArray[i] -= start;
         }
         else {
            for (i = 0; i < span->end; i++)
               span->yArray[i]++;
         }
         if ((span->interpMask | span->arrayMask) & SPAN_TEXTURE)
            _mesa_write_texture_span(ctx, span);
         else if ((span->interpMask | span->arrayMask) & SPAN_RGBA)
            _mesa_write_rgba_span(ctx, span);
         else
            _mesa_write_index_span(ctx, span);
      }
   }
   else {
      GLuint i;
      GLint w;
      for (w = 0; w < width; w++) {
         if (w == 0) {
            for (i = 0; i < span->end; i++)
               span->xArray[i] -= start;
         }
         else {
            for (i = 0; i < span->end; i++)
               span->xArray[i]++;
         }
         if ((span->interpMask | span->arrayMask) & SPAN_TEXTURE)
            _mesa_write_texture_span(ctx, span);
         else if ((span->interpMask | span->arrayMask) & SPAN_RGBA)
            _mesa_write_rgba_span(ctx, span);
         else
            _mesa_write_index_span(ctx, span);
      }
   }
}



/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/


/* Flat, color index line */
static void flat_ci_line( GLcontext *ctx,
                          const SWvertex *vert0,
			  const SWvertex *vert1 )
{
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_FLAT);
   ASSERT(!ctx->Line.StippleFlag);
   ASSERT(ctx->Line.Width == 1.0F);

   INIT_SPAN(span, GL_LINE, 0, SPAN_INDEX, SPAN_XY);
   /*span.arrayMask |= SPAN_XY;
     span.interpMask |= SPAN_INDEX;*/
   span.index = IntToFixed(vert1->index);
   span.indexStep = 0;

#define INTERP_XY 1
#define PLOT(X,Y)			\
   {					\
      span.xArray[span.end] = X;	\
      span.yArray[span.end] = Y;	\
      span.end++;			\
   }

#include "s_linetemp.h"

   _mesa_write_index_span(ctx, &span);
}


/* Flat-shaded, RGBA line */
static void flat_rgba_line( GLcontext *ctx,
                            const SWvertex *vert0,
			    const SWvertex *vert1 )
{
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_FLAT);
   ASSERT(!ctx->Line.StippleFlag);
   ASSERT(ctx->Line.Width == 1.0F);

   INIT_SPAN(span, GL_LINE, 0, SPAN_RGBA, SPAN_XY);
   /*span.arrayMask |= SPAN_XY;
     span.interpMask |= SPAN_RGBA;*/
   span.red = ChanToFixed(vert1->color[0]);
   span.green = ChanToFixed(vert1->color[1]);
   span.blue = ChanToFixed(vert1->color[2]);
   span.alpha = ChanToFixed(vert1->color[3]);
   span.redStep = 0;
   span.greenStep = 0;
   span.blueStep = 0;
   span.alphaStep = 0;

#define INTERP_XY 1
#define PLOT(X,Y)			\
   {					\
      span.xArray[span.end] = X;	\
      span.yArray[span.end] = Y;	\
      span.end++;			\
   }

#include "s_linetemp.h"

   _mesa_write_rgba_span(ctx, &span);
}


/* Smooth shaded, color index line */
static void smooth_ci_line( GLcontext *ctx,
                            const SWvertex *vert0,
			    const SWvertex *vert1 )
{
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_SMOOTH);
   ASSERT(!ctx->Line.StippleFlag);
   ASSERT(ctx->Line.Width == 1.0F);

   INIT_SPAN(span, GL_LINE, 0, 0, SPAN_XY | SPAN_INDEX);
   /*span.arrayMask |= (SPAN_XY | SPAN_INDEX);*/

#define INTERP_XY 1
#define INTERP_INDEX 1
#define PLOT(X,Y)			\
   {					\
      span.xArray[span.end] = X;	\
      span.yArray[span.end] = Y;	\
      span.color.index[span.end] = I;	\
      span.end++;			\
   }

#include "s_linetemp.h"

   _mesa_write_index_span(ctx, &span);
}


/* Smooth-shaded, RGBA line */
static void smooth_rgba_line( GLcontext *ctx,
                       	      const SWvertex *vert0,
			      const SWvertex *vert1 )
{
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_SMOOTH);
   ASSERT(!ctx->Line.StippleFlag);
   ASSERT(ctx->Line.Width == 1.0F);

   INIT_SPAN(span, GL_LINE, 0, 0, SPAN_XY | SPAN_RGBA);
   /*span.arrayMask |= (SPAN_XY | SPAN_RGBA);*/

#define INTERP_XY 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define PLOT(X,Y)						\
   {								\
      span.xArray[span.end] = X;				\
      span.yArray[span.end] = Y;				\
      span.color.rgba[span.end][RCOMP] = FixedToInt(r0);	\
      span.color.rgba[span.end][GCOMP] = FixedToInt(g0);	\
      span.color.rgba[span.end][BCOMP] = FixedToInt(b0);	\
      span.color.rgba[span.end][ACOMP] = FixedToInt(a0);	\
      span.end++;						\
   }

#include "s_linetemp.h"

   _mesa_write_rgba_span(ctx, &span);
}


/* Smooth shaded, color index, any width, maybe stippled */
static void general_smooth_ci_line( GLcontext *ctx,
                           	    const SWvertex *vert0,
				    const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_SMOOTH);

   INIT_SPAN(span, GL_LINE, 0, 0,
	     SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_INDEX);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_INDEX);*/

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_INDEX 1
#define PLOT(X,Y)			\
   {					\
      span.xArray[span.end] = X;	\
      span.yArray[span.end] = Y;	\
      span.zArray[span.end] = Z;	\
      span.fogArray[span.end] = fog0;	\
      span.color.index[span.end] = I;	\
      span.end++;			\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_index_span(ctx, &span);
   }
}


/* Flat shaded, color index, any width, maybe stippled */
static void general_flat_ci_line( GLcontext *ctx,
                                  const SWvertex *vert0,
				  const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_FLAT);

   INIT_SPAN(span, GL_LINE, 0, SPAN_INDEX,
	     SPAN_XY | SPAN_Z | SPAN_FOG);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG);
     span.interpMask |= SPAN_INDEX;*/
   span.index = IntToFixed(vert1->index);
   span.indexStep = 0;

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define PLOT(X,Y)			\
   {					\
      span.xArray[span.end] = X;	\
      span.yArray[span.end] = Y;	\
      span.zArray[span.end] = Z;	\
      span.fogArray[span.end] = fog0;	\
      span.end++;			\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_index_span(ctx, &span);
   }
}



static void general_smooth_rgba_line( GLcontext *ctx,
                                      const SWvertex *vert0,
				      const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_SMOOTH);

   INIT_SPAN(span, GL_LINE, 0, 0,
	     SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_RGBA);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_RGBA);*/

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define PLOT(X,Y)						\
   {								\
      span.xArray[span.end] = X;				\
      span.yArray[span.end] = Y;				\
      span.zArray[span.end] = Z;				\
      span.color.rgba[span.end][RCOMP] = FixedToInt(r0);	\
      span.color.rgba[span.end][GCOMP] = FixedToInt(g0);	\
      span.color.rgba[span.end][BCOMP] = FixedToInt(b0);	\
      span.color.rgba[span.end][ACOMP] = FixedToInt(a0);	\
      span.fogArray[span.end] = fog0;				\
      span.end++;						\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_rgba_span(ctx, &span);
   }
}


static void general_flat_rgba_line( GLcontext *ctx,
                                    const SWvertex *vert0,
				    const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_FLAT);

   INIT_SPAN(span, GL_LINE, 0, SPAN_RGBA,
	     SPAN_XY | SPAN_Z | SPAN_FOG);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG);
     span.interpMask |= SPAN_RGBA;*/
   span.red = ChanToFixed(vert1->color[0]);
   span.green = ChanToFixed(vert1->color[1]);
   span.blue = ChanToFixed(vert1->color[2]);
   span.alpha = ChanToFixed(vert1->color[3]);
   span.redStep = 0;
   span.greenStep = 0;
   span.blueStep = 0;
   span.alphaStep = 0;

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define PLOT(X,Y)						\
   {								\
      span.xArray[span.end] = X;				\
      span.yArray[span.end] = Y;				\
      span.zArray[span.end] = Z;				\
      span.fogArray[span.end] = fog0;				\
      span.end++;						\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_rgba_span(ctx, &span);
   }
}


/* Flat-shaded, textured, any width, maybe stippled */
static void flat_textured_line( GLcontext *ctx,
                                const SWvertex *vert0,
				const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_FLAT);

   INIT_SPAN(span, GL_LINE, 0, SPAN_RGBA | SPAN_SPEC,
	     SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_TEXTURE | SPAN_RGBA);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_TEXTURE | SPAN_LAMBDA);
     span.interpMask |= (SPAN_RGBA | SPAN_SPEC);*/
   span.red = ChanToFixed(vert1->color[0]);
   span.green = ChanToFixed(vert1->color[1]);
   span.blue = ChanToFixed(vert1->color[2]);
   span.alpha = ChanToFixed(vert1->color[3]);
   span.redStep = 0;
   span.greenStep = 0;
   span.blueStep = 0;
   span.alphaStep = 0;
   span.specRed = ChanToFixed(vert1->specular[0]);
   span.specGreen = ChanToFixed(vert1->specular[1]);
   span.specBlue = ChanToFixed(vert1->specular[2]);
   span.specRedStep = 0;
   span.specGreenStep = 0;
   span.specBlueStep = 0;

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_TEX 1
#define PLOT(X,Y)						\
   {								\
      span.xArray[span.end] = X;				\
      span.yArray[span.end] = Y;				\
      span.zArray[span.end] = Z;				\
      span.fogArray[span.end] = fog0;				\
      span.texcoords[0][span.end][0] = fragTexcoord[0];		\
      span.texcoords[0][span.end][1] = fragTexcoord[1];		\
      span.texcoords[0][span.end][2] = fragTexcoord[2];		\
      span.lambda[0][span.end] = 0.0;				\
      span.end++;						\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_texture_span(ctx, &span);
   }
}



/* Smooth-shaded, textured, any width, maybe stippled */
static void smooth_textured_line( GLcontext *ctx,
                                  const SWvertex *vert0,
				  const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;

   ASSERT(ctx->Light.ShadeModel == GL_SMOOTH);

   INIT_SPAN(span, GL_LINE, 0, 0,
	     SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_RGBA | SPAN_TEXTURE | SPAN_LAMBDA);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_RGBA | SPAN_TEXTURE | SPAN_LAMBDA);*/

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define PLOT(X,Y)						\
   {								\
      span.xArray[span.end] = X;				\
      span.yArray[span.end] = Y;				\
      span.zArray[span.end] = Z;				\
      span.fogArray[span.end] = fog0;				\
      span.color.rgba[span.end][RCOMP] = FixedToInt(r0);	\
      span.color.rgba[span.end][GCOMP] = FixedToInt(g0);	\
      span.color.rgba[span.end][BCOMP] = FixedToInt(b0);	\
      span.color.rgba[span.end][ACOMP] = FixedToInt(a0);	\
      span.texcoords[0][span.end][0] = fragTexcoord[0];		\
      span.texcoords[0][span.end][1] = fragTexcoord[1];		\
      span.texcoords[0][span.end][2] = fragTexcoord[2];		\
      span.lambda[0][span.end] = 0.0;				\
      span.end++;						\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_texture_span(ctx, &span);
   }
}


/* Smooth-shaded, multitextured, any width, maybe stippled, separate specular
 * color interpolation.
 */
static void smooth_multitextured_line( GLcontext *ctx,
				       const SWvertex *vert0,
				       const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;
   GLuint u;

   ASSERT(ctx->Light.ShadeModel == GL_SMOOTH);

   INIT_SPAN(span, GL_LINE, 0, 0,
	     SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_RGBA | SPAN_SPEC | SPAN_TEXTURE | SPAN_LAMBDA);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_RGBA | SPAN_SPEC | SPAN_TEXTURE | SPAN_LAMBDA);*/

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define PLOT(X,Y)							\
   {									\
      span.xArray[span.end] = X;					\
      span.yArray[span.end] = Y;					\
      span.zArray[span.end] = Z;					\
      span.fogArray[span.end] = fog0;					\
      span.color.rgba[span.end][RCOMP] = FixedToInt(r0);		\
      span.color.rgba[span.end][GCOMP] = FixedToInt(g0);		\
      span.color.rgba[span.end][BCOMP] = FixedToInt(b0);		\
      span.color.rgba[span.end][ACOMP] = FixedToInt(a0);		\
      span.specArray[span.end][RCOMP] = FixedToInt(sr0);		\
      span.specArray[span.end][GCOMP] = FixedToInt(sb0);		\
      span.specArray[span.end][BCOMP] = FixedToInt(sb0);		\
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {		\
         if (ctx->Texture.Unit[u]._ReallyEnabled) {			\
            span.texcoords[u][span.end][0] = fragTexcoord[u][0];	\
            span.texcoords[u][span.end][1] = fragTexcoord[u][1];	\
            span.texcoords[u][span.end][2] = fragTexcoord[u][2];	\
            span.lambda[u][span.end] = 0.0;				\
         }								\
      }									\
      span.end++;							\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_texture_span(ctx, &span);
   }
}


/* Flat-shaded, multitextured, any width, maybe stippled, separate specular
 * color interpolation.
 */
static void flat_multitextured_line( GLcontext *ctx,
                                     const SWvertex *vert0,
				     const SWvertex *vert1 )
{
   GLboolean xMajor = GL_FALSE;
   struct sw_span span;
   GLuint u;

   ASSERT(ctx->Light.ShadeModel == GL_FLAT);

   INIT_SPAN(span, GL_LINE, 0, SPAN_RGBA | SPAN_SPEC,
	     SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_TEXTURE | SPAN_LAMBDA);
   /*span.arrayMask |= (SPAN_XY | SPAN_Z | SPAN_FOG | SPAN_TEXTURE | SPAN_LAMBDA);
     span.interpMask |= (SPAN_RGBA | SPAN_SPEC);*/
   span.red = ChanToFixed(vert1->color[0]);
   span.green = ChanToFixed(vert1->color[1]);
   span.blue = ChanToFixed(vert1->color[2]);
   span.alpha = ChanToFixed(vert1->color[3]);
   span.redStep = 0;
   span.greenStep = 0;
   span.blueStep = 0;
   span.alphaStep = 0;
   span.specRed = ChanToFixed(vert1->specular[0]);
   span.specGreen = ChanToFixed(vert1->specular[1]);
   span.specBlue = ChanToFixed(vert1->specular[2]);
   span.specRedStep = 0;
   span.specGreenStep = 0;
   span.specBlueStep = 0;

#define SET_XMAJOR 1
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_MULTITEX 1
#define PLOT(X,Y)							\
   {									\
      span.xArray[span.end] = X;					\
      span.yArray[span.end] = Y;					\
      span.zArray[span.end] = Z;					\
      span.fogArray[span.end] = fog0;					\
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {		\
         if (ctx->Texture.Unit[u]._ReallyEnabled) {			\
            span.texcoords[u][span.end][0] = fragTexcoord[u][0];	\
            span.texcoords[u][span.end][1] = fragTexcoord[u][1];	\
            span.texcoords[u][span.end][2] = fragTexcoord[u][2];	\
            span.lambda[u][span.end] = 0.0;				\
         }								\
      }									\
      span.end++;							\
   }
#include "s_linetemp.h"

   if (ctx->Line.StippleFlag) {
      span.arrayMask |= SPAN_MASK;
      compute_stipple_mask(ctx, span.end, span.mask);
   }

   if (ctx->Line.Width > 1.0) {
      draw_wide_line(ctx, &span, xMajor);
   }
   else {
      _mesa_write_texture_span(ctx, &span);
   }
}


void _swrast_add_spec_terms_line( GLcontext *ctx,
				  const SWvertex *v0,
				  const SWvertex *v1 )
{
   SWvertex *ncv0 = (SWvertex *)v0;
   SWvertex *ncv1 = (SWvertex *)v1;
   GLchan c[2][4];
   COPY_CHAN4( c[0], ncv0->color );
   COPY_CHAN4( c[1], ncv1->color );
   ACC_3V( ncv0->color, ncv0->specular );
   ACC_3V( ncv1->color, ncv1->specular );
   SWRAST_CONTEXT(ctx)->SpecLine( ctx, ncv0, ncv1 );
   COPY_CHAN4( ncv0->color, c[0] );
   COPY_CHAN4( ncv1->color, c[1] );
}


#ifdef DEBUG
extern void
_mesa_print_line_function(GLcontext *ctx);  /* silence compiler warning */
void
_mesa_print_line_function(GLcontext *ctx)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   printf("Line Func == ");
   if (swrast->Line == flat_ci_line)
      printf("flat_ci_line\n");
   else if (swrast->Line == flat_rgba_line)
      printf("flat_rgba_line\n");
   else if (swrast->Line == smooth_ci_line)
      printf("smooth_ci_line\n");
   else if (swrast->Line == smooth_rgba_line)
      printf("smooth_rgba_line\n");
   else if (swrast->Line == general_smooth_ci_line)
      printf("general_smooth_ci_line\n");
   else if (swrast->Line == general_flat_ci_line)
      printf("general_flat_ci_line\n");
   else if (swrast->Line == general_smooth_rgba_line)
      printf("general_smooth_rgba_line\n");
   else if (swrast->Line == general_flat_rgba_line)
      printf("general_flat_rgba_line\n");
   else if (swrast->Line == flat_textured_line)
      printf("flat_textured_line\n");
   else if (swrast->Line == smooth_textured_line)
      printf("smooth_textured_line\n");
   else if (swrast->Line == smooth_multitextured_line)
      printf("smooth_multitextured_line\n");
   else if (swrast->Line == flat_multitextured_line)
      printf("flat_multitextured_line\n");
   else
      printf("Driver func %p\n", (void *) swrast->Line);
}
#endif



#ifdef DEBUG

/* record the current line function name */
static const char *lineFuncName = NULL;

#define USE(lineFunc)                   \
do {                                    \
    lineFuncName = #lineFunc;           \
    /*printf("%s\n", lineFuncName);*/   \
    swrast->Line = lineFunc;            \
} while (0)

#else

#define USE(lineFunc)  swrast->Line = lineFunc

#endif



/*
 * Determine which line drawing function to use given the current
 * rendering context.
 *
 * Please update the summary flag _SWRAST_NEW_LINE if you add or remove
 * tests to this code.
 */
void
_swrast_choose_line( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLboolean rgbmode = ctx->Visual.rgbMode;

   if (ctx->RenderMode == GL_RENDER) {
      if (ctx->Line.SmoothFlag) {
         /* antialiased lines */
         _swrast_choose_aa_line_function(ctx);
         ASSERT(swrast->Triangle);
      }
      else if (ctx->Texture._ReallyEnabled) {
         if (ctx->Texture._ReallyEnabled > TEXTURE0_ANY ||	     
	     (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)) {
            /* multi-texture and/or separate specular color */
            if (ctx->Light.ShadeModel == GL_SMOOTH)
               USE(smooth_multitextured_line);
            else
               USE(flat_multitextured_line);
         }
         else {
            if (ctx->Light.ShadeModel == GL_SMOOTH) {
                USE(smooth_textured_line);
            }
            else {
                USE(flat_textured_line);
            }
         }
      }
      else {
	 if (ctx->Light.ShadeModel == GL_SMOOTH) {
            if (ctx->Depth.Test || ctx->Fog.Enabled || ctx->Line.Width != 1.0
                || ctx->Line.StippleFlag) {
               if (rgbmode)
                  USE(general_smooth_rgba_line);
               else
                  USE(general_smooth_ci_line);
            }
            else {
               if (rgbmode)
                  USE(smooth_rgba_line);
               else
                  USE(smooth_ci_line);
            }
	 }
         else {
            if (ctx->Depth.Test || ctx->Fog.Enabled || ctx->Line.Width != 1.0
                || ctx->Line.StippleFlag) {
               if (rgbmode)
                  USE(general_flat_rgba_line);
               else
                  USE(general_flat_ci_line);
            }
            else {
               if (rgbmode)
                  USE(flat_rgba_line);
               else
                  USE(flat_ci_line);
            }
         }
      }
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      USE(_mesa_feedback_line);
   }
   else {
      ASSERT(ctx->RenderMode == GL_SELECT);
      USE(_mesa_select_line);
   }

   /*_mesa_print_line_function(ctx);*/
}
