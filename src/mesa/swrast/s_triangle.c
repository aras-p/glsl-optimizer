/* $Id: s_triangle.c,v 1.29 2001/05/17 09:32:17 keithw Exp $ */

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
 * When the device driver doesn't implement triangle rasterization it
 * can hook in _swrast_Triangle, which eventually calls one of these
 * functions to draw triangles.
 */

#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "texformat.h"
#include "teximage.h"
#include "texstate.h"

#include "s_aatriangle.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_feedback.h"
#include "s_span.h"
#include "s_triangle.h"
#include "s_trispan.h"



GLboolean _mesa_cull_triangle( GLcontext *ctx,
			    const SWvertex *v0,
			    const SWvertex *v1,
			    const SWvertex *v2 )
{
   GLfloat ex = v1->win[0] - v0->win[0];
   GLfloat ey = v1->win[1] - v0->win[1];
   GLfloat fx = v2->win[0] - v0->win[0];
   GLfloat fy = v2->win[1] - v0->win[1];
   GLfloat c = ex*fy-ey*fx;

   if (c * SWRAST_CONTEXT(ctx)->_backface_sign > 0)
      return 0;

   return 1;
}



/*
 * Render a flat-shaded color index triangle.
 */
static void flat_ci_triangle( GLcontext *ctx,
			      const SWvertex *v0,
			      const SWvertex *v1,
			      const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1

#define RENDER_SPAN( span )						\
   GLdepth zSpan[MAX_WIDTH];						\
   GLfloat fogSpan[MAX_WIDTH];						\
   GLuint i;								\
   for (i = 0; i < span.count; i++) {					\
      zSpan[i] = FixedToDepth(span.z);					\
      span.z += span.zStep;						\
      fogSpan[i] = span.fog;						\
      span.fog += span.fogStep;						\
   }									\
   _mesa_write_monoindex_span(ctx, span.count, span.x, span.y,		\
	                      zSpan, fogSpan, v0->index, NULL, GL_POLYGON );

#include "s_tritemp.h"
}



/*
 * Render a smooth-shaded color index triangle.
 */
static void smooth_ci_triangle( GLcontext *ctx,
				const SWvertex *v0,
				const SWvertex *v1,
				const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_INDEX 1

#define RENDER_SPAN( span )						\
   GLdepth zSpan[MAX_WIDTH];						\
   GLfloat fogSpan[MAX_WIDTH];						\
   GLuint indexSpan[MAX_WIDTH];						\
   GLuint i;						                \
   for (i = 0; i < span.count; i++) {					\
      zSpan[i] = FixedToDepth(span.z);					\
      span.z += span.zStep;						\
      indexSpan[i] = FixedToInt(span.index);				\
      span.index += span.indexStep;					\
      fogSpan[i] = span.fog;						\
      span.fog += span.fogStep;						\
   }									\
   _mesa_write_index_span(ctx, span.count, span.x, span.y,		\
                          zSpan, fogSpan, indexSpan, NULL, GL_POLYGON);

#include "s_tritemp.h"
}



/*
 * Render a flat-shaded RGBA triangle.
 */
static void flat_rgba_triangle( GLcontext *ctx,
				const SWvertex *v0,
				const SWvertex *v1,
				const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE

#define RENDER_SPAN( span )						\
   GLdepth zSpan[MAX_WIDTH];						\
   GLfloat fogSpan[MAX_WIDTH];						\
   GLuint i;								\
   for (i = 0; i < span.count; i++) {					\
      zSpan[i] = FixedToDepth(span.z);					\
      span.z += span.zStep;						\
      fogSpan[i] = span.fog;						\
      span.fog += span.fogStep;						\
   }									\
   _mesa_write_monocolor_span(ctx, span.count, span.x, span.y, zSpan,	\
                              fogSpan, v2->color, NULL, GL_POLYGON );

#include "s_tritemp.h"

   ASSERT(!ctx->Texture._ReallyEnabled);  /* texturing must be off */
   ASSERT(ctx->Light.ShadeModel==GL_FLAT);
}



/*
 * Render a smooth-shaded RGBA triangle.
 */
static void smooth_rgba_triangle( GLcontext *ctx,
				  const SWvertex *v0,
				  const SWvertex *v1,
				  const SWvertex *v2 )
{

#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define RENDER_SPAN( span )					\
   GLdepth zSpan[MAX_WIDTH];					\
   GLchan rgbaSpan[MAX_WIDTH][4];				\
   GLfloat fogSpan[MAX_WIDTH];					\
   GLuint i;				        	        \
   for (i = 0; i < span.count; i++) {				\
      rgbaSpan[i][RCOMP] = FixedToInt(span.red);		\
      rgbaSpan[i][GCOMP] = FixedToInt(span.green);		\
      rgbaSpan[i][BCOMP] = FixedToInt(span.blue);		\
      rgbaSpan[i][ACOMP] = FixedToInt(span.alpha);		\
      span.red += span.redStep;					\
      span.green += span.greenStep;				\
      span.blue += span.blueStep;				\
      span.alpha += span.alphaStep;				\
      zSpan[i] = FixedToDepth(span.z);				\
      span.z += span.zStep;					\
      fogSpan[i] = span.fog;					\
      span.fog += span.fogStep;					\
   }								\
   _mesa_write_rgba_span(ctx, span.count, span.x, span.y,	\
                         (CONST GLdepth *) zSpan,		\
                         fogSpan, rgbaSpan, NULL, GL_POLYGON);

#include "s_tritemp.h"

   ASSERT(!ctx->Texture._ReallyEnabled);  /* texturing must be off */
   ASSERT(ctx->Light.ShadeModel==GL_SMOOTH);
}


/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T only w/out mipmapping or perspective correction.
 *
 * No fog.
 */
static void simple_textured_triangle( GLcontext *ctx,
				      const SWvertex *v0,
				      const SWvertex *v1,
				      const SWvertex *v2 )
{
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   SWcontext *swrast = SWRAST_CONTEXT(ctx);                             \
   struct gl_texture_object *obj = ctx->Texture.Unit[0].Current2D;	\
   GLint b = obj->BaseLevel;						\
   const GLfloat twidth = (GLfloat) obj->Image[b]->Width;		\
   const GLfloat theight = (GLfloat) obj->Image[b]->Height;		\
   const GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   const GLchan *texture = (const GLchan *) obj->Image[b]->Data;	\
   const GLint smask = obj->Image[b]->Width - 1;			\
   const GLint tmask = obj->Image[b]->Height - 1;			\
   if (!texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }

#define RENDER_SPAN( span  )						\
   GLchan rgbSpan[MAX_WIDTH][3];					\
   GLuint i;								\
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		\
   span.intTex[1] -= FIXED_HALF;					\
   for (i = 0; i < span.count; i++) {					\
      GLint s = FixedToInt(span.intTex[0]) & smask;			\
      GLint t = FixedToInt(span.intTex[1]) & tmask;			\
      GLint pos = (t << twidth_log2) + s;				\
      pos = pos + pos + pos;  /* multiply by 3 */			\
      rgbSpan[i][RCOMP] = texture[pos];					\
      rgbSpan[i][GCOMP] = texture[pos+1];				\
      rgbSpan[i][BCOMP] = texture[pos+2];				\
      span.intTex[0] += span.intTexStep[0];				\
      span.intTex[1] += span.intTexStep[1];				\
   }									\
   (*swrast->Driver.WriteRGBSpan)(ctx, span.count, span.x, span.y,	\
                                  (CONST GLchan (*)[3]) rgbSpan, NULL );

#include "s_tritemp.h"
}


/*
 * Render an RGB, GL_DECAL, textured triangle.
 * Interpolate S,T, GL_LESS depth test, w/out mipmapping or
 * perspective correction.
 *
 * No fog.
 */
static void simple_z_textured_triangle( GLcontext *ctx,
					const SWvertex *v0,
					const SWvertex *v1,
					const SWvertex *v2 )
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE							\
   SWcontext *swrast = SWRAST_CONTEXT(ctx);                             \
   struct gl_texture_object *obj = ctx->Texture.Unit[0].Current2D;	\
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   const GLchan *texture = (const GLchan *) obj->Image[b]->Data;	\
   GLint smask = obj->Image[b]->Width - 1;				\
   GLint tmask = obj->Image[b]->Height - 1;				\
   if (!texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }

#define RENDER_SPAN( span )						\
   GLchan rgbSpan[MAX_WIDTH][3];					\
   GLubyte mask[MAX_WIDTH];						\
   GLuint i;				    				\
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		\
   span.intTex[1] -= FIXED_HALF;					\
   for (i = 0; i < span.count; i++) {					\
      const GLdepth z = FixedToDepth(span.z);				\
      if (z < zRow[i]) {						\
         GLint s = FixedToInt(span.intTex[0]) & smask;			\
         GLint t = FixedToInt(span.intTex[1]) & tmask;			\
         GLint pos = (t << twidth_log2) + s;				\
         pos = pos + pos + pos;  /* multiply by 3 */			\
         rgbSpan[i][RCOMP] = texture[pos];				\
         rgbSpan[i][GCOMP] = texture[pos+1];				\
         rgbSpan[i][BCOMP] = texture[pos+2];				\
         zRow[i] = z;							\
         mask[i] = 1;							\
      }									\
      else {								\
         mask[i] = 0;							\
      }									\
      span.intTex[0] += span.intTexStep[0];				\
      span.intTex[1] += span.intTexStep[1];				\
      span.z += span.zStep;						\
   }									\
   (*swrast->Driver.WriteRGBSpan)(ctx, span.count, span.x, span.y,	\
                                  (CONST GLchan (*)[3]) rgbSpan, mask );

#include "s_tritemp.h"
}



struct affine_info
{
   GLenum filter;
   GLenum format;
   GLenum envmode;
   GLint smask, tmask;
   GLint twidth_log2;
   const GLchan *texture;
   GLchan er, eg, eb, ea;
   GLint tbytesline, tsize;
   GLint fixedToDepthShift;
};

static void
affine_span(GLcontext *ctx, struct triangle_span *span,
            struct affine_info *info)
{
   GLint tr, tg, tb, ta;

  /* Instead of defining a function for each mode, a test is done
   * between the outer and inner loops. This is to reduce code size
   * and complexity. Observe that an optimizing compiler kills
   * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
   */

#define NEAREST_RGB		\
        tr = tex00[RCOMP];	\
        tg = tex00[GCOMP];	\
        tb = tex00[BCOMP];	\
        ta = 0xff

#define LINEAR_RGB							\
	tr = (ti * (si * tex00[0] + sf * tex01[0]) +			\
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT;	\
	tg = (ti * (si * tex00[1] + sf * tex01[1]) +			\
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT;	\
	tb = (ti * (si * tex00[2] + sf * tex01[2]) +			\
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT;	\
	ta = 0xff

#define NEAREST_RGBA		\
        tr = tex00[RCOMP];	\
        tg = tex00[GCOMP];	\
        tb = tex00[BCOMP];	\
        ta = tex00[ACOMP]

#define LINEAR_RGBA							\
	tr = (ti * (si * tex00[0] + sf * tex01[0]) +			\
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT;	\
	tg = (ti * (si * tex00[1] + sf * tex01[1]) +			\
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT;	\
	tb = (ti * (si * tex00[2] + sf * tex01[2]) +			\
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT;	\
	ta = (ti * (si * tex00[3] + sf * tex01[3]) +			\
              tf * (si * tex10[3] + sf * tex11[3])) >> 2 * FIXED_SHIFT

#define MODULATE							\
        dest[RCOMP] = span->red * (tr + 1) >> (FIXED_SHIFT + 8);	\
        dest[GCOMP] = span->green * (tg + 1) >> (FIXED_SHIFT + 8);	\
        dest[BCOMP] = span->blue * (tb + 1) >> (FIXED_SHIFT + 8);	\
        dest[ACOMP] = span->alpha * (ta + 1) >> (FIXED_SHIFT + 8)

#define DECAL								\
	dest[RCOMP] = ((0xff - ta) * span->red				\
           + ((ta + 1) * tr << FIXED_SHIFT)) >> (FIXED_SHIFT + 8);	\
	dest[GCOMP] = ((0xff - ta) * span->green			\
           + ((ta + 1) * tg << FIXED_SHIFT)) >> (FIXED_SHIFT + 8);	\
	dest[BCOMP] = ((0xff - ta) * span->blue				\
           + ((ta + 1) * tb << FIXED_SHIFT)) >> (FIXED_SHIFT + 8);	\
	dest[ACOMP] = FixedToInt(span->alpha)

#define BLEND								\
        dest[RCOMP] = ((0xff - tr) * span->red				\
           + (tr + 1) * info->er) >> (FIXED_SHIFT + 8);			\
        dest[GCOMP] = ((0xff - tg) * span->green			\
           + (tg + 1) * info->eg) >> (FIXED_SHIFT + 8);			\
        dest[BCOMP] = ((0xff - tb) * span->blue				\
           + (tb + 1) * info->eb) >> (FIXED_SHIFT + 8);			\
        dest[ACOMP] = span->alpha * (ta + 1) >> (FIXED_SHIFT + 8)

#define REPLACE			\
        dest[RCOMP] = tr;	\
        dest[GCOMP] = tg;	\
        dest[BCOMP] = tb;	\
        dest[ACOMP] = ta

#define ADD								\
        dest[RCOMP] = ((span->red << 8)					\
           + (tr + 1) * info->er) >> (FIXED_SHIFT + 8);			\
        dest[GCOMP] = ((span->green << 8)				\
           + (tg + 1) * info->eg) >> (FIXED_SHIFT + 8);			\
        dest[BCOMP] = ((span->blue << 8)				\
           + (tb + 1) * info->eb) >> (FIXED_SHIFT + 8);			\
        dest[ACOMP] = span->alpha * (ta + 1) >> (FIXED_SHIFT + 8)

/* shortcuts */

#define NEAREST_RGB_REPLACE  NEAREST_RGB;REPLACE

#define NEAREST_RGBA_REPLACE  *(GLint *)dest = *(GLint *)tex00

#define SPAN1(DO_TEX,COMP)						\
	for (i = 0; i < span->count; i++) {				\
           GLint s = FixedToInt(span->intTex[0]) & info->smask;		\
           GLint t = FixedToInt(span->intTex[1]) & info->tmask;		\
           GLint pos = (t << info->twidth_log2) + s;			\
           const GLchan *tex00 = info->texture + COMP * pos;		\
	   zspan[i] = FixedToDepth(span->z);				\
	   fogspan[i] = span->fog;					\
           DO_TEX;							\
	   span->fog += span->fogStep;					\
	   span->z += span->zStep;					\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   span->intTex[0] += span->intTexStep[0];			\
	   span->intTex[1] += span->intTexStep[1];			\
           dest += 4;							\
	}

#define SPAN2(DO_TEX,COMP)						\
	for (i = 0; i < span->count; i++) {				\
           GLint s = FixedToInt(span->intTex[0]) & info->smask;		\
           GLint t = FixedToInt(span->intTex[1]) & info->tmask;		\
           GLint sf = span->intTex[0] & FIXED_FRAC_MASK;		\
           GLint tf = span->intTex[1] & FIXED_FRAC_MASK;		\
           GLint si = FIXED_FRAC_MASK - sf;				\
           GLint ti = FIXED_FRAC_MASK - tf;				\
           GLint pos = (t << info->twidth_log2) + s;			\
           const GLchan *tex00 = info->texture + COMP * pos;		\
           const GLchan *tex10 = tex00 + info->tbytesline;		\
           const GLchan *tex01 = tex00 + COMP;				\
           const GLchan *tex11 = tex10 + COMP;				\
           (void) ti;							\
           (void) si;							\
           if (t == info->tmask) {					\
              tex10 -= info->tsize;					\
              tex11 -= info->tsize;					\
           }								\
           if (s == info->smask) {					\
              tex01 -= info->tbytesline;				\
              tex11 -= info->tbytesline;				\
           }								\
	   zspan[i] = FixedToDepth(span->z);				\
	   fogspan[i] = span->fog;					\
           DO_TEX;							\
	   span->fog += span->fogStep;					\
	   span->z += span->zStep;					\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   span->intTex[0] += span->intTexStep[0];			\
	   span->intTex[1] += span->intTexStep[1];			\
           dest += 4;							\
	}

#define FixedToDepth(F)  ((F) >> fixedToDepthShift)

   GLuint i;
   GLdepth zspan[MAX_WIDTH];
   GLfloat fogspan[MAX_WIDTH];
   GLchan rgba[MAX_WIDTH][4];
   GLchan *dest = rgba[0];
   const GLint fixedToDepthShift = info->fixedToDepthShift;

   span->intTex[0] -= FIXED_HALF;
   span->intTex[1] -= FIXED_HALF;
   switch (info->filter) {
   case GL_NEAREST:
      switch (info->format) {
      case GL_RGB:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN1(NEAREST_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN1(NEAREST_RGB_REPLACE,3);
            break;
         case GL_BLEND:
            SPAN1(NEAREST_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN1(NEAREST_RGB;ADD,3);
            break;
         default:
            abort();
         }
         break;
      case GL_RGBA:
         switch(info->envmode) {
         case GL_MODULATE:
            SPAN1(NEAREST_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN1(NEAREST_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN1(NEAREST_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN1(NEAREST_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN1(NEAREST_RGBA_REPLACE,4);
            break;
         default:
            abort();
         }
         break;
      }
      break;

   case GL_LINEAR:
      span->intTex[0] -= FIXED_HALF;
      span->intTex[1] -= FIXED_HALF;
      switch (info->format) {
      case GL_RGB:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN2(LINEAR_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN2(LINEAR_RGB;REPLACE,3);
            break;
         case GL_BLEND:
            SPAN2(LINEAR_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN2(LINEAR_RGB;ADD,3);
            break;
         default:
            abort();
         }
         break;
      case GL_RGBA:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN2(LINEAR_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN2(LINEAR_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN2(LINEAR_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN2(LINEAR_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN2(LINEAR_RGBA;REPLACE,4);
            break;
         default:
            abort();
         }		    break;
      }
      break;
   }
   _mesa_write_rgba_span(ctx, span->count, span->x, span->y,
                         zspan, fogspan, rgba, NULL, GL_POLYGON);

#undef SPAN1
#undef SPAN2
#undef FixedToDepth
}



/*
 * Render an RGB/RGBA textured triangle without perspective correction.
 */
static void affine_textured_triangle( GLcontext *ctx,
				      const SWvertex *v0,
				      const SWvertex *v1,
				      const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight

#define SETUP_CODE
   struct affine_info info;
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;			\
   struct gl_texture_object *obj = unit->Current2D;			\
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   info.fixedToDepthShift = ctx->Visual.depthBits <= 16 ? FIXED_SHIFT : 0;\
   info.texture = (const GLchan *) obj->Image[b]->Data;			\
   info.twidth_log2 = obj->Image[b]->WidthLog2;				\
   info.smask = obj->Image[b]->Width - 1;				\
   info.tmask = obj->Image[b]->Height - 1;				\
   info.format = obj->Image[b]->Format;					\
   info.filter = obj->MinFilter;					\
   info.envmode = unit->EnvMode;					\
									\
   if (info.envmode == GL_BLEND) {					\
      /* potential off-by-one error here? (1.0f -> 2048 -> 0) */	\
      info.er = FloatToFixed(unit->EnvColor[RCOMP]);			\
      info.eg = FloatToFixed(unit->EnvColor[GCOMP]);			\
      info.eb = FloatToFixed(unit->EnvColor[BCOMP]);			\
      info.ea = FloatToFixed(unit->EnvColor[ACOMP]);			\
   }									\
   if (!info.texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }									\
									\
   switch (info.format) {						\
   case GL_ALPHA:							\
   case GL_LUMINANCE:							\
   case GL_INTENSITY:							\
      info.tbytesline = obj->Image[b]->Width;				\
      break;								\
   case GL_LUMINANCE_ALPHA:						\
      info.tbytesline = obj->Image[b]->Width * 2;			\
      break;								\
   case GL_RGB:								\
      info.tbytesline = obj->Image[b]->Width * 3;			\
      break;								\
   case GL_RGBA:							\
      info.tbytesline = obj->Image[b]->Width * 4;			\
      break;								\
   default:								\
      _mesa_problem(NULL, "Bad texture format in affine_texture_triangle");\
      return;								\
   }									\
   info.tsize = obj->Image[b]->Height * info.tbytesline;

#define RENDER_SPAN( span )		\
   affine_span(ctx, &span, &info);

#include "s_tritemp.h"

}


#if 0 /* XXX disabled because of texcoord interpolation errors */
/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 *
 * This function only renders textured triangles that use GL_NEAREST.
 * Perspective correction works right.
 *
 * This function written by Klaus Niederkrueger <klaus@math.leidenuniv.nl>
 * Send all questions and bug reports to him.
 */
static void near_persp_textured_triangle(GLcontext *ctx,
					 const SWvertex *v0,
					 const SWvertex *v1,
					 const SWvertex *v2 )
{
/* The BIAS value is used to shift negative values into positive values.
 * Without this, negative texture values don't GL_REPEAT correctly at just
 * below zero, because (int)-0.5 = 0 = (int)0.5. We're not going to worry
 * about texture coords less than -BIAS. This could be fixed by using
 * FLOORF etc. instead, but this is slower...
 */
#define BIAS 4096.0F

#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1

#define SETUP_CODE							\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;			\
   struct gl_texture_object *obj = unit->Current2D;			\
   const GLint b = obj->BaseLevel;					\
   const GLfloat twidth = (GLfloat) obj->Image[b]->Width;	      	\
   const GLfloat theight = (GLfloat) obj->Image[b]->Height;		\
   const GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   const GLchan *texture = (const GLchan *) obj->Image[b]->Data;	\
   const GLint smask = (obj->Image[b]->Width - 1);                      \
   const GLint tmask = (obj->Image[b]->Height - 1);                     \
   const GLint format = obj->Image[b]->Format;                          \
   const GLint envmode = unit->EnvMode;                                 \
   GLfloat sscale, tscale;                                              \
   GLfixed er, eg, eb, ea;                                              \
   GLint tr, tg, tb, ta;                                                \
   if (!texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }									\
   if (envmode == GL_BLEND || envmode == GL_ADD) {                      \
      er = FloatToFixed(unit->EnvColor[RCOMP]);                         \
      eg = FloatToFixed(unit->EnvColor[GCOMP]);                         \
      eb = FloatToFixed(unit->EnvColor[BCOMP]);                         \
      ea = FloatToFixed(unit->EnvColor[ACOMP]);                         \
   }                                                                    \
   sscale = twidth;                                                     \
   tscale = theight;                                                    \


#define OLD_SPAN(DO_TEX,COMP)                         \
   for (i=0;i<n;i++) {                                \
      GLfloat invQ = 1.0f / vv;                       \
      GLint s = (int)(SS * invQ + BIAS) & smask;      \
      GLint t = (int)(TT * invQ + BIAS) & tmask;      \
      GLint pos = COMP * ((t << twidth_log2) + s);    \
      const GLchan *tex00 = texture + pos;            \
      zspan[i] = FixedToDepth(span.z);                   \
      fogspan[i] = span.fog;                              \
      DO_TEX;                                         \
      span.fog += span.fogStep;                                 \
      span.z += span.zStep;                                   \
      span.red += span.redStep;                                   \
      span.green += span.greenStep;                                   \
      span.blue += span.blueStep;                                   \
      span.alpha += span.alphaStep;                                   \
      SS += dSdx;                                     \
      TT += dTdx;                                     \
      vv += dvdx;                                     \
      dest += 4;                                      \
   }

#define X_Y_TEX_COORD(X, Y) ((((int)(X) & tmask) << twidth_log2) + ((int)(Y) & smask))
#define Y_X_TEX_COORD(X, Y) ((((int)(Y) & tmask) << twidth_log2) + ((int)(X) & smask))

#define SPAN1(DO_TEX, COMP, TEX_COORD) {                          \
   GLfloat x_max = CEILF(x_tex);                                  \
   GLfloat y_max = y_tex + (x_max - x_tex) * dy_dx;               \
   GLint j, x_m = (int)x_max;                                     \
   GLint pos;                                                     \
   if ((int)y_max != (int)y_tex) {                                \
      GLfloat x_mid = x_tex + (CEILF(y_tex)-y_tex) * dx_dy;       \
      j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);    \
      pos = COMP * TEX_COORD(x_tex, y_tex);                       \
      DRAW_LINE (DO_TEX);                                         \
      y_tex = y_max;                                              \
   }                                                              \
   nominator += vv * x_max;                                       \
   denominator -= dvdx * x_max;                                   \
   j = nominator / denominator;                                   \
   pos = COMP * TEX_COORD(x_tex, y_tex);                          \
   DRAW_LINE (DO_TEX);                                            \
   while (i<n)  {                                                 \
      y_tex = y_max;                                              \
      y_max += dy_dx;                                             \
      if ((int)y_max != (int)y_tex) {                             \
         GLfloat x_mid = (CEILF(y_tex)-y_tex) * dx_dy;            \
         j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid); \
         pos = COMP * TEX_COORD(x_m, y_tex);                      \
         DRAW_LINE (DO_TEX);                                      \
         y_tex = y_max;                                           \
      }                                                           \
      nominator += vv;                                            \
      denominator -= dvdx;                                        \
      j = nominator/denominator;                                  \
      pos = COMP * TEX_COORD(x_m, y_tex);                         \
      DRAW_LINE (DO_TEX);                                         \
      x_m ++;                                                     \
   }                                                              \
}

#define SPAN2(DO_TEX, COMP, TEX_COORD)  {			\
   GLfloat x_max = CEILF (x_tex);				\
   GLfloat y_max = y_tex + (x_max - x_tex) * dy_dx;		\
   GLint j, x_m = (int) x_max;					\
   GLint pos;							\
   if ((int)y_max != (int)y_tex) {				\
      GLfloat x_mid = x_tex + (FLOORF(y_tex)-y_tex) * dx_dy;	\
      j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);	\
      pos = COMP * TEX_COORD(x_tex, y_tex);			\
      DRAW_LINE (DO_TEX);					\
      y_tex = y_max;						\
   }								\
   nominator += vv * x_max;					\
   denominator -= dvdx * x_max;					\
   j = nominator / denominator;					\
   pos = COMP * TEX_COORD(x_tex, y_tex);			\
   DRAW_LINE (DO_TEX);						\
   while (i<n)  {						\
      y_tex = y_max;						\
      y_max += dy_dx;						\
      if ((int)y_max != (int)y_tex) {				\
         GLfloat x_mid = (FLOORF(y_tex)-y_tex) * dx_dy;		\
         j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);\
         pos = COMP * TEX_COORD(x_m, y_tex);			\
         DRAW_LINE (DO_TEX);					\
         y_tex = y_max;						\
      }								\
      nominator += vv;						\
      denominator -= dvdx;					\
      j = nominator/denominator;				\
      pos = COMP * TEX_COORD(x_m, y_tex);			\
      DRAW_LINE (DO_TEX);					\
      x_m ++;							\
   }								\
}

#define SPAN3(DO_TEX, COMP, TEX_COORD) {				\
   GLfloat x_min = FLOORF (x_tex);					\
   GLfloat y_min = y_tex + (x_min - x_tex) * dy_dx;			\
   GLint j, x_m = (int)x_min;						\
   GLint pos;								\
   if ((int)y_min != (int)y_tex) {					\
      GLfloat x_mid = x_tex + (CEILF(y_tex)-y_tex) * dx_dy;		\
      j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);		\
      pos = COMP * TEX_COORD(x_m, y_tex);				\
      DRAW_LINE (DO_TEX);						\
      y_tex = y_min;							\
   }									\
   nominator += vv*x_min;						\
   denominator -= dvdx*x_min;						\
   j = nominator / denominator;						\
   pos = COMP * TEX_COORD(x_m, y_tex);					\
   DRAW_LINE (DO_TEX);							\
   while (i<n) {							\
      x_m --;								\
      y_tex = y_min;							\
      y_min -= dy_dx;							\
      if ((int)y_min != (int)y_tex) {					\
         GLfloat x_mid = (CEILF(y_tex)-y_tex) * dx_dy;			\
         j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);	\
         pos = COMP * TEX_COORD(x_m, y_tex);				\
         DRAW_LINE (DO_TEX);						\
         y_tex = y_min;							\
      }									\
      nominator -= vv;							\
      denominator += dvdx;						\
      j = nominator/denominator;					\
      pos = COMP * TEX_COORD(x_m, y_tex);				\
      DRAW_LINE (DO_TEX);						\
   }									\
}

#define SPAN4(DO_TEX, COMP, TEX_COORD)					\
{									\
   GLfloat x_min = FLOORF(x_tex);					\
   GLint x_m = (int)x_min;						\
   GLfloat y_min = y_tex + (x_min - x_tex) * dy_dx;			\
   GLint j;								\
   GLint pos;								\
   if ((int)y_min != (int)y_tex) {					\
      GLfloat x_mid = x_tex + (FLOORF(y_tex)-y_tex) * dx_dy;		\
      j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);		\
      pos = COMP * TEX_COORD(x_m, y_tex);				\
      DRAW_LINE (DO_TEX);						\
      y_tex = y_min;							\
   }									\
   nominator += vv * x_min;						\
   denominator -= dvdx * x_min;						\
   j = nominator / denominator;						\
   pos = COMP * TEX_COORD(x_m, y_tex);					\
   DRAW_LINE (DO_TEX);							\
   while (i<n)  {							\
      x_m --;								\
      y_tex = y_min;							\
      y_min -= dy_dx;							\
      if ((int)y_min != (int)y_tex) {					\
         GLfloat x_mid = (FLOORF(y_tex)-y_tex) * dx_dy;			\
         j = (nominator + vv * x_mid)/(denominator - dvdx*x_mid);	\
         pos = COMP * TEX_COORD(x_m, (y_tex));				\
         DRAW_LINE (DO_TEX);						\
         y_tex = y_min;							\
      }									\
      nominator -= vv;							\
      denominator += dvdx;						\
      j = nominator/denominator;					\
      pos = COMP * TEX_COORD(x_m, y_tex);				\
      DRAW_LINE (DO_TEX);						\
   }									\
}

#define DRAW_LINE(DO_TEX)		\
   {					\
      GLchan *tex00 = texture + pos;	\
      if (j>n || j<-100000)		\
         j = n;				\
      while (i<j) {			\
         zspan[i] = FixedToDepth(span.z);	\
         fogspan[i] = span.fog;             \
         DO_TEX;			\
         span.fog += span.fogStep;                \
         span.z += span.zStep;			\
         span.red += span.redStep;			\
         span.green += span.greenStep;			\
         span.blue += span.blueStep;			\
         span.alpha += span.alphaStep;			\
         dest += 4;			\
         i++;				\
      }					\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
   {									\
      GLint i = 0;							\
      const GLint n = RIGHT-LEFT;					\
      GLdepth zspan[MAX_WIDTH];						\
      GLfloat fogspan[MAX_WIDTH];					\
      GLchan rgba[MAX_WIDTH][4];					\
      (void)uu; /* please GCC */					\
      if (n > 0) {							\
         GLchan *dest = rgba[0];					\
         GLfloat SS = ss * sscale;					\
         GLfloat TT = tt * tscale;					\
         GLfloat dSdx = dsdx * sscale;					\
         GLfloat dTdx = dtdx * tscale;					\
         GLfloat x_tex;					                \
         GLfloat y_tex;					                \
         GLfloat dx_tex;	                                        \
         GLfloat dy_tex;	                                        \
         if (n<5) /* When line very short, setup-time > speed-gain. */	\
            goto old_span;  /* So: take old method */			\
         x_tex = SS / vv,					        \
            y_tex = TT / vv;					        \
         dx_tex = (SS + n * dSdx) / (vv + n * dvdx) - x_tex,	        \
            dy_tex = (TT + n * dTdx) / (vv + n * dvdx) - y_tex;	        \
	 /* Choose between walking over texture or over pixelline: */	\
	 /* If there are few texels, walk over texture otherwise   */	\
	 /* walk over pixelarray. The quotient on the right side   */	\
	 /* should give the timeratio needed to draw one texel in  */	\
	 /* comparison to one pixel. Depends on CPU.	 	   */	\
         if (dx_tex*dx_tex + dy_tex*dy_tex < (n*n)/16) {		\
            x_tex += BIAS;						\
            y_tex += BIAS;						\
            if (dx_tex*dx_tex > dy_tex*dy_tex) {			\
	    /* if (FABSF(dx_tex) > FABSF(dy_tex)) */			\
               GLfloat nominator = - SS - vv * BIAS;			\
               GLfloat denominator = dvdx * BIAS + dSdx;		\
               GLfloat dy_dx;						\
               GLfloat dx_dy;						\
               if (dy_tex != 0.0f) {					\
                  dy_dx = dy_tex / dx_tex;                              \
                  dx_dy = 1.0f/dy_dx;					\
               }                                                        \
               else                                                     \
                  dy_dx = 0.0f;                                         \
               if (dx_tex > 0.0f) {					\
                  if (dy_tex > 0.0f) {					\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN1(NEAREST_RGB;MODULATE,3, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                        case GL_REPLACE:				\
                           SPAN1(NEAREST_RGB_REPLACE,3, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN1(NEAREST_RGB;BLEND,3, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN1(NEAREST_RGB;ADD,3, Y_X_TEX_COORD);     \
		           break;                                       \
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                        case GL_MODULATE:				\
                           SPAN1(NEAREST_RGBA;MODULATE,4, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN1(NEAREST_RGBA;DECAL,4, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN1(NEAREST_RGBA;BLEND,4, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN1(NEAREST_RGBA;ADD,4, Y_X_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN1(NEAREST_RGBA_REPLACE,4, Y_X_TEX_COORD);\
                           break;					\
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
                  else {  /* dy_tex <= 0.0f */				\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN2(NEAREST_RGB;MODULATE,3, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                        case GL_REPLACE:				\
                           SPAN2(NEAREST_RGB_REPLACE,3, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN2(NEAREST_RGB;BLEND,3, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN2(NEAREST_RGB;ADD,3, Y_X_TEX_COORD);     \
		           break;                                       \
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                        case GL_MODULATE:				\
                           SPAN2(NEAREST_RGBA;MODULATE,4, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN2(NEAREST_RGBA;DECAL,4, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN2(NEAREST_RGBA;BLEND,4, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN2(NEAREST_RGBA;ADD,4, Y_X_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN2(NEAREST_RGBA_REPLACE,4, Y_X_TEX_COORD);\
                           break;					\
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
               }							\
               else { /* dx_tex < 0.0f */				\
                  if (dy_tex > 0.0f) {					\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN3(NEAREST_RGB;MODULATE,3, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                        case GL_REPLACE:				\
                           SPAN3(NEAREST_RGB_REPLACE,3, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN3(NEAREST_RGB;BLEND,3, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN3(NEAREST_RGB;ADD,3, Y_X_TEX_COORD);     \
		           break;                                       \
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                        case GL_MODULATE:				\
                           SPAN3(NEAREST_RGBA;MODULATE,4, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN3(NEAREST_RGBA;DECAL,4, Y_X_TEX_COORD);	\
                           break;					\
                          case GL_BLEND:				\
                           SPAN3(NEAREST_RGBA;BLEND,4, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN3(NEAREST_RGBA;ADD,4, Y_X_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN3(NEAREST_RGBA_REPLACE,4, Y_X_TEX_COORD);\
                           break;					\
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
                  else {  /* dy_tex <= 0.0f */				\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN4(NEAREST_RGB;MODULATE,3, Y_X_TEX_COORD);\
                           break;					\
                          case GL_DECAL:				\
                        case GL_REPLACE:				\
                           SPAN4(NEAREST_RGB_REPLACE,3, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN4(NEAREST_RGB;BLEND,3, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN4(NEAREST_RGB;ADD,3, Y_X_TEX_COORD);     \
		           break;                                       \
                        default:					\
                           abort();					\
                        }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                            case GL_MODULATE:				\
                           SPAN4(NEAREST_RGBA;MODULATE,4, Y_X_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN4(NEAREST_RGBA;DECAL,4, Y_X_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN4(NEAREST_RGBA;BLEND,4, Y_X_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN4(NEAREST_RGBA;ADD,4, Y_X_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN4(NEAREST_RGBA_REPLACE,4, Y_X_TEX_COORD);\
                           break;					\
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
               }							\
            }								\
            else {  /* FABSF(dx_tex) > FABSF(dy_tex) */		        \
               GLfloat swap;						\
               GLfloat dy_dx;						\
               GLfloat dx_dy;						\
               GLfloat nominator, denominator;				\
               if (dx_tex == 0.0f /* &&  dy_tex == 0.0f*/)		\
                  goto old_span; /* case so special, that use old */    \
               /* swap some x-values and y-values */			\
               SS = TT;							\
               dSdx = dTdx;						\
               swap = x_tex, x_tex = y_tex, y_tex = swap;		\
               swap = dx_tex, dx_tex = dy_tex, dy_tex = swap;		\
               nominator = - SS - vv * BIAS;				\
               denominator = dvdx * BIAS + dSdx;			\
               if (dy_tex != 0.0f) {					\
                  dy_dx = dy_tex / dx_tex;                              \
                  dx_dy = 1.0f/dy_dx;					\
               }                                                        \
               else                                                     \
                  dy_dx = 0.0f;                                         \
               if (dx_tex > 0.0f) {					\
                  if (dy_tex > 0.0f) {					\
		     switch (format) {					\
		     case GL_RGB:					\
			switch (envmode) {				\
			case GL_MODULATE:				\
                           SPAN1(NEAREST_RGB;MODULATE,3, X_Y_TEX_COORD);\
			   break;					\
                        case GL_DECAL:					\
                        case GL_REPLACE:                       		\
                           SPAN1(NEAREST_RGB_REPLACE,3, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN1(NEAREST_RGB;BLEND,3, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN1(NEAREST_RGB;ADD,3, X_Y_TEX_COORD);     \
		           break;                                       \
                        default: /* unexpected env mode */		\
                           abort();					\
                        }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                        case GL_MODULATE:				\
                           SPAN1(NEAREST_RGBA;MODULATE,4, X_Y_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN1(NEAREST_RGBA;DECAL,4, X_Y_TEX_COORD);  \
                           break;					\
                        case GL_BLEND:					\
                           SPAN1(NEAREST_RGBA;BLEND,4, X_Y_TEX_COORD);  \
                           break;					\
		        case GL_ADD:                                    \
		           SPAN1(NEAREST_RGBA;ADD,4, X_Y_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN1(NEAREST_RGBA_REPLACE,4, X_Y_TEX_COORD);\
                           break;					\
                        default:					\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
                  else { /* dy_tex <= 0.0f */				\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN2(NEAREST_RGB;MODULATE,3, X_Y_TEX_COORD);\
                           break;					\
                          case GL_DECAL:				\
                        case GL_REPLACE:				\
                           SPAN2(NEAREST_RGB_REPLACE,3, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN2(NEAREST_RGB;BLEND,3, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN2(NEAREST_RGB;ADD,3, X_Y_TEX_COORD);     \
		           break;                                       \
                        default:					\
                           abort();					\
                            }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                        case GL_MODULATE:				\
                           SPAN2(NEAREST_RGBA;MODULATE,4, X_Y_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN2(NEAREST_RGBA;DECAL,4, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:				        \
                           SPAN2(NEAREST_RGBA;BLEND,4, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN2(NEAREST_RGBA;ADD,4, X_Y_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN2(NEAREST_RGBA_REPLACE,4, X_Y_TEX_COORD);\
                           break;					\
                        default:					\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
               }							\
               else { /* dx_tex < 0.0f */				\
                  if (dy_tex > 0.0f) {					\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN3(NEAREST_RGB;MODULATE,3, X_Y_TEX_COORD);\
                           break;					\
                        case GL_DECAL:				\
                        case GL_REPLACE:				\
                           SPAN3(NEAREST_RGB_REPLACE,3, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN3(NEAREST_RGB;BLEND,3, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN3(NEAREST_RGB;ADD,3, X_Y_TEX_COORD);     \
		           break;                                       \
                        default:					\
                           abort();					\
                        }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {				\
                           case GL_MODULATE:				\
                           SPAN3(NEAREST_RGBA;MODULATE,4, X_Y_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN3(NEAREST_RGBA;DECAL,4, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN3(NEAREST_RGBA;BLEND,4, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN3(NEAREST_RGBA;ADD,4, X_Y_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN3(NEAREST_RGBA_REPLACE,4, X_Y_TEX_COORD);\
                           break;					\
                        default:					\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
                  else { /* dy_tex <= 0.0f */				\
                     switch (format) {					\
                     case GL_RGB:					\
                        switch (envmode) {				\
                        case GL_MODULATE:				\
                           SPAN4(NEAREST_RGB;MODULATE,3, X_Y_TEX_COORD);\
                           break;					\
                          case GL_DECAL:				\
                        case GL_REPLACE:				\
                           SPAN4(NEAREST_RGB_REPLACE,3, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:					\
                           SPAN4(NEAREST_RGB;BLEND,3, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN4(NEAREST_RGB;ADD,3, X_Y_TEX_COORD);     \
		           break;                                       \
                        default:					\
                           abort();					\
                            }						\
                        break;						\
                     case GL_RGBA:					\
                        switch(envmode) {                      		\
                        case GL_MODULATE:				\
                           SPAN4(NEAREST_RGBA;MODULATE,4, X_Y_TEX_COORD);\
                           break;					\
                        case GL_DECAL:					\
                           SPAN4(NEAREST_RGBA;DECAL,4, X_Y_TEX_COORD);	\
                           break;					\
                        case GL_BLEND:				        \
                           SPAN4(NEAREST_RGBA;BLEND,4, X_Y_TEX_COORD);	\
                           break;					\
		        case GL_ADD:                                    \
		           SPAN4(NEAREST_RGBA;ADD,4, X_Y_TEX_COORD);    \
		           break;                                       \
                        case GL_REPLACE:				\
                           SPAN4(NEAREST_RGBA_REPLACE,4, X_Y_TEX_COORD);\
                           break;					\
                        default:					\
                           abort();					\
                        }						\
                        break;						\
                     }							\
                  }							\
               }							\
            }								\
         }								\
         else {                                                		\
            old_span:                                                   \
            switch (format) {                                  		\
            case GL_RGB:						\
               switch (envmode) {                              		\
               case GL_MODULATE:					\
                  OLD_SPAN(NEAREST_RGB;MODULATE,3);			\
                  break;						\
               case GL_DECAL:						\
               case GL_REPLACE:						\
                  OLD_SPAN(NEAREST_RGB_REPLACE,3);			\
                  break;						\
               case GL_BLEND:						\
                  OLD_SPAN(NEAREST_RGB;BLEND,3);			\
                  break;						\
	       case GL_ADD:                                             \
		  OLD_SPAN(NEAREST_RGB;ADD,3);                          \
		  break;                                                \
               default:							\
                  abort();						\
               }							\
               break;							\
            case GL_RGBA:						\
               switch(envmode) {                               		\
               case GL_MODULATE:					\
                  OLD_SPAN(NEAREST_RGBA;MODULATE,4);			\
                  break;						\
               case GL_DECAL:						\
                  OLD_SPAN(NEAREST_RGBA;DECAL,4);			\
                  break;						\
               case GL_BLEND:						\
                  OLD_SPAN(NEAREST_RGBA;BLEND,4);			\
                  break;						\
	       case GL_ADD:                                             \
		  OLD_SPAN(NEAREST_RGBA;ADD,4);                         \
		  break;                                                \
               case GL_REPLACE:						\
                  OLD_SPAN(NEAREST_RGBA_REPLACE,4);			\
                  break;						\
               default:							\
                  abort();						\
               }							\
               break;							\
            }								\
         }								\
         _mesa_write_rgba_span( ctx, n, LEFT, Y, zspan,			\
                             fogspan, rgba, NULL, GL_POLYGON);		\
         span.red = span.green = span.blue = span.alpha = 0;		\
      }									\
   }									\

#include "s_tritemp.h"
#undef OLD_SPAN
#undef SPAN1
#undef SPAN2
#undef SPAN3
#undef SPAN4
#undef X_Y_TEX_COORD
#undef Y_X_TEX_COORD
#undef DRAW_LINE
#undef BIAS
}
#endif


#if 0 /* XXX disabled because of texcoord interpolation errors */
/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 *
 * This function written by Klaus Niederkrueger <klaus@math.leidenuniv.nl>
 * Send all questions and bug reports to him.
 */
static void lin_persp_textured_triangle( GLcontext *ctx,
					 const SWvertex *v0,
					 const SWvertex *v1,
					 const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1

#define SETUP_CODE							\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;			\
   struct gl_texture_object *obj = unit->Current2D;			\
   const GLint b = obj->BaseLevel;					\
   const GLfloat twidth = (GLfloat) obj->Image[b]->Width;	        \
   const GLfloat theight = (GLfloat) obj->Image[b]->Height;		\
   const GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   GLchan *texture = obj->Image[b]->Data;				\
   const GLint smask = (obj->Image[b]->Width - 1);                      \
   const GLint tmask = (obj->Image[b]->Height - 1);                     \
   const GLint format = obj->Image[b]->Format;                          \
   const GLint envmode = unit->EnvMode;                                 \
   GLfloat sscale, tscale;                                              \
   GLint comp, tbytesline, tsize;                                       \
   GLfixed er, eg, eb, ea;                                              \
   GLint tr, tg, tb, ta;                                                \
   if (!texture) {							\
      return;								\
   }									\
   if (envmode == GL_BLEND || envmode == GL_ADD) {                      \
      er = FloatToFixed(unit->EnvColor[RCOMP]);                         \
      eg = FloatToFixed(unit->EnvColor[GCOMP]);                         \
      eb = FloatToFixed(unit->EnvColor[BCOMP]);                         \
      ea = FloatToFixed(unit->EnvColor[ACOMP]);                         \
   }                                                                    \
   switch (format) {                                                    \
   case GL_ALPHA:							\
   case GL_LUMINANCE:							\
   case GL_INTENSITY:							\
      comp = 1;                                                         \
      break;                                                            \
   case GL_LUMINANCE_ALPHA:						\
      comp = 2;                                                         \
      break;                                                            \
   case GL_RGB:								\
      comp = 3;                                                         \
      break;                                                            \
   case GL_RGBA:							\
      comp = 4;                                                         \
      break;                                                            \
   default:								\
      _mesa_problem(NULL, "Bad texture format in lin_persp_texture_triangle"); \
      return;                                                           \
   }                                                                    \
   sscale = FIXED_SCALE * twidth;                                       \
   tscale = FIXED_SCALE * theight;                                      \
   tbytesline = obj->Image[b]->Width * comp;                            \
   tsize = theight * tbytesline;


#define SPAN(DO_TEX,COMP)					\
        for (i=0;i<n;i++) {					\
           GLfloat invQ = 1.0f / vv;				\
           GLfixed span.intTex[0] = (int)(SS * invQ);		\
           GLfixed span.intTex[1] = (int)(TT * invQ);		\
	   GLint s = FixedToInt(span.intTex[0]) & smask;	\
	   GLint t = FixedToInt(span.intTex[1]) & tmask;	\
           GLint sf = span.intTex[0] & FIXED_FRAC_MASK;		\
           GLint tf = span.intTex[1] & FIXED_FRAC_MASK;		\
           GLint si = FIXED_FRAC_MASK - sf;			\
           GLint ti = FIXED_FRAC_MASK - tf;			\
           GLint pos = COMP * ((t << twidth_log2) + s);		\
           GLchan *tex00 = texture + pos;			\
           GLchan *tex10 = tex00 + tbytesline;			\
           GLchan *tex01 = tex00 + COMP;			\
           GLchan *tex11 = tex10 + COMP;			\
           if (t == tmask) {					\
              tex10 -= tsize;					\
              tex11 -= tsize;					\
           }							\
           if (s == smask) {					\
              tex01 -= tbytesline;				\
              tex11 -= tbytesline;				\
           }							\
	   zspan[i] = FixedToDepth(span.z);			\
	   fogspan[i] = span.fog;				\
           DO_TEX;						\
	   span.fog += span.fogStep;				\
	   span.z += span.zStep;				\
           span.red += span.redStep;				\
	   span.green += span.greenStep;			\
           span.blue += span.blueStep;				\
	   span.alpha += span.alphaStep;			\
           SS += dSdx;						\
           TT += dTdx;						\
	   vv += dvdx;						\
           dest += 4;						\
	}

#define INNER_LOOP( LEFT, RIGHT, Y )			\
   {							\
      GLint i;						\
      const GLint n = RIGHT-LEFT;			\
      GLdepth zspan[MAX_WIDTH];				\
      GLfloat fogspan[MAX_WIDTH];			\
      GLchan rgba[MAX_WIDTH][4];			\
      (void) uu; /* please GCC */			\
      if (n > 0) {					\
         GLfloat SS = ss * sscale;			\
         GLfloat TT = tt * tscale;			\
         GLfloat dSdx = dsdx * sscale;			\
         GLfloat dTdx = dtdx * tscale;			\
         GLchan *dest = rgba[0];			\
         SS -= 0.5f * FIXED_SCALE * vv;			\
         TT -= 0.5f * FIXED_SCALE * vv;			\
         switch (format) {				\
         case GL_RGB:					\
            switch (envmode) {				\
            case GL_MODULATE:				\
               SPAN(LINEAR_RGB;MODULATE,3);		\
               break;					\
            case GL_DECAL:				\
            case GL_REPLACE:				\
               SPAN(LINEAR_RGB;REPLACE,3);		\
               break;					\
            case GL_BLEND:				\
               SPAN(LINEAR_RGB;BLEND,3);		\
               break;					\
            case GL_ADD:                                \
	       SPAN(LINEAR_RGB;ADD,3);                  \
	       break;                                   \
            default:					\
               abort();					\
            }						\
            break;					\
         case GL_RGBA:					\
            switch (envmode) {				\
            case GL_MODULATE:				\
               SPAN(LINEAR_RGBA;MODULATE,4);		\
               break;					\
            case GL_DECAL:				\
               SPAN(LINEAR_RGBA;DECAL,4);		\
               break;					\
            case GL_BLEND:				\
               SPAN(LINEAR_RGBA;BLEND,4);		\
               break;					\
            case GL_REPLACE:				\
               SPAN(LINEAR_RGBA;REPLACE,4);		\
               break;					\
            case GL_ADD:                                \
               SPAN(LINEAR_RGBA;ADD,4);                 \
               break;                                   \
            default: /* unexpected env mode */		\
               abort();					\
            }						\
         }						\
         _mesa_write_rgba_span(ctx, n, LEFT, Y, zspan,	\
                               fogspan, rgba, NULL,     \
                               GL_POLYGON);	        \
      }							\
   }

#include "s_tritemp.h"
#undef SPAN
}
#endif


/*
 * Generate arrays of fragment colors, z, fog, texcoords, etc from a
 * triangle span object.  Then call the span/fragment processsing
 * functions in s_span.[ch].
 */
static void
rasterize_span(GLcontext *ctx, const struct triangle_span *span)
{
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

   if (span->activeMask & SPAN_RGBA) {
      GLfixed r = span->red;
      GLfixed g = span->green;
      GLfixed b = span->blue;
      GLfixed a = span->alpha;
      GLuint i;
      for (i = 0; i < span->count; i++) {
         rgba[i][RCOMP] = FixedToInt(r);
         rgba[i][GCOMP] = FixedToInt(g);
         rgba[i][BCOMP] = FixedToInt(b);
         rgba[i][ACOMP] = FixedToInt(a);
         r += span->redStep;
         g += span->greenStep;
         b += span->blueStep;
         a += span->alphaStep;
      }
   }
   if (span->activeMask & SPAN_SPEC) {
      GLfixed r = span->specRed;
      GLfixed g = span->specGreen;
      GLfixed b = span->specBlue;
      GLuint i;
      for (i = 0; i < span->count; i++) {
         spec[i][RCOMP] = FixedToInt(r);
         spec[i][GCOMP] = FixedToInt(g);
         spec[i][BCOMP] = FixedToInt(b);
         r += span->specRedStep;
         g += span->specGreenStep;
         b += span->specBlueStep;
      }
   }
   if (span->activeMask & SPAN_INDEX) {
      GLuint i;
      GLfixed ind = span->index;
      for (i = 0; i < span->count; i++) {
         index[i] = FixedToInt(ind);
         ind += span->indexStep;
      }
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
            for (u = 0; u < MAX_TEXTURE_UNITS; u++) {
               if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  GLfloat s = span->tex[u][0];
                  GLfloat t = span->tex[u][1];
                  GLfloat r = span->tex[u][2];
                  GLfloat q = span->tex[u][3];
                  GLuint i;
                  for (i = 0; i < span->count; i++) {
                     const GLfloat invQ = (q == 0.0F) ? 1.0 : (1.0F / q);
                     msTex[u][i] = s * invQ;
                     mtTex[u][i] = t * invQ;
                     mrTex[u][i] = r * invQ;
                     mLambda[u][i] = log(span->rho[u] * invQ * invQ) * 1.442695F * 0.5F;
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
            for (u = 0; u < MAX_TEXTURE_UNITS; u++) {
               if (ctx->Texture.Unit[u]._ReallyEnabled) {
                  GLfloat s = span->tex[u][0];
                  GLfloat t = span->tex[u][1];
                  GLfloat r = span->tex[u][2];
                  GLfloat q = span->tex[u][3];
                  GLuint i;
                  for (i = 0; i < span->count; i++) {
                     const GLfloat invQ = (q == 0.0F) ? 1.0 : (1.0F / q);
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
            for (i = 0; i < span->count; i++) {
               const GLfloat invQ = (q == 0.0F) ? 1.0 : (1.0F / q);
               sTex[i] = s * invQ;
               tTex[i] = t * invQ;
               rTex[i] = r * invQ;
               lambda[i] = log(span->rho[0] * invQ * invQ) * 1.442695F * 0.5F;
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
            for (i = 0; i < span->count; i++) {
               const GLfloat invQ = (q == 0.0F) ? 1.0 : (1.0F / q);
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
            _mesa_write_multitexture_span(ctx, span->count, span->x, span->y,
                                          z, fogPtr,
                                          (const GLfloat (*)[MAX_WIDTH]) msTex,
                                          (const GLfloat (*)[MAX_WIDTH]) mtTex,
                                          (const GLfloat (*)[MAX_WIDTH]) mrTex,
                                          (GLfloat (*)[MAX_WIDTH]) mLambda,
                                          rgba, (CONST GLchan (*)[4]) spec,
                                          NULL, GL_POLYGON );
         }
         else {
            _mesa_write_multitexture_span(ctx, span->count, span->x, span->y,
                                          z, fogPtr,
                                          (const GLfloat (*)[MAX_WIDTH]) msTex,
                                          (const GLfloat (*)[MAX_WIDTH]) mtTex,
                                          (const GLfloat (*)[MAX_WIDTH]) mrTex,
                                          (GLfloat (*)[MAX_WIDTH]) mLambda,
                                          rgba, NULL, NULL, GL_POLYGON);
         }
      }
      else {
         /* single texture */
         if (span->activeMask & SPAN_SPEC) {
            _mesa_write_texture_span(ctx, span->count, span->x, span->y,
                                     z, fogPtr, sTex, tTex, rTex, lambda,
                                     rgba, (CONST GLchan (*)[4]) spec,
                                     NULL, GL_POLYGON);
         }
         else {
            _mesa_write_texture_span(ctx, span->count, span->x, span->y,
                                     z, fogPtr, sTex, tTex, rTex, lambda,
                                     rgba, NULL, NULL, GL_POLYGON);
         }
      }
   }
   else {
      _mesa_problem(ctx, "rasterize_span() should only be used for texturing");
   }

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
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,R with perspective correction, w/out mipmapping.
 */
static void general_textured_triangle( GLcontext *ctx,
				       const SWvertex *v0,
				       const SWvertex *v1,
				       const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;	\
   const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];\
   const GLboolean flatShade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLfixed rFlat, gFlat, bFlat, aFlat;					\
   DEFARRAY(GLfloat, sSpan, MAX_WIDTH);  /* mac 32k limitation */	\
   DEFARRAY(GLfloat, tSpan, MAX_WIDTH);  /* mac 32k limitation */	\
   DEFARRAY(GLfloat, uSpan, MAX_WIDTH);  /* mac 32k limitation */	\
   CHECKARRAY(sSpan, return);  /* mac 32k limitation */			\
   CHECKARRAY(tSpan, return);  /* mac 32k limitation */			\
   CHECKARRAY(uSpan, return);  /* mac 32k limitation */			\
   if (flatShade) {							\
      rFlat = IntToFixed(v2->color[RCOMP]);				\
      gFlat = IntToFixed(v2->color[GCOMP]);				\
      bFlat = IntToFixed(v2->color[BCOMP]);				\
      aFlat = IntToFixed(v2->color[ACOMP]);				\
   }									\
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )						\
   GLdepth zSpan[MAX_WIDTH];						\
   GLfloat fogSpan[MAX_WIDTH];						\
   GLchan rgbaSpan[MAX_WIDTH][4];					\
   GLuint i;								\
   if (flatShade) {							\
      span.red = rFlat;    span.redStep = 0;				\
      span.green = gFlat;  span.greenStep = 0;				\
      span.blue = bFlat;   span.blueStep = 0;				\
      span.alpha = aFlat;  span.alphaStep = 0;				\
   }									\
   /* NOTE: we could just call rasterize_span() here instead */		\
   for (i = 0; i < span.count; i++) {					\
      GLdouble invQ = span.tex[0][3] ? (1.0 / span.tex[0][3]) : 1.0;	\
      zSpan[i] = FixedToDepth(span.z);					\
      span.z += span.zStep;						\
      fogSpan[i] = span.fog;			          		\
      span.fog += span.fogStep;						\
      rgbaSpan[i][RCOMP] = FixedToInt(span.red);			\
      rgbaSpan[i][GCOMP] = FixedToInt(span.green);			\
      rgbaSpan[i][BCOMP] = FixedToInt(span.blue);			\
      rgbaSpan[i][ACOMP] = FixedToInt(span.alpha);			\
      span.red += span.redStep;						\
      span.green += span.greenStep;					\
      span.blue += span.blueStep;					\
      span.alpha += span.alphaStep;					\
      sSpan[i] = span.tex[0][0] * invQ;					\
      tSpan[i] = span.tex[0][1] * invQ;					\
      uSpan[i] = span.tex[0][2] * invQ;					\
      span.tex[0][0] += span.texStep[0][0];				\
      span.tex[0][1] += span.texStep[0][1];				\
      span.tex[0][2] += span.texStep[0][2];				\
      span.tex[0][3] += span.texStep[0][3];				\
   }									\
   _mesa_write_texture_span(ctx, span.count, span.x, span.y,		\
                            zSpan, fogSpan, sSpan, tSpan, uSpan,	\
                            NULL, rgbaSpan, NULL, NULL, GL_POLYGON );

#define CLEANUP_CODE				\
   UNDEFARRAY(sSpan);  /* mac 32k limitation */	\
   UNDEFARRAY(tSpan);				\
   UNDEFARRAY(uSpan);

#include "s_tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * color interpolation.
 * Interpolate texcoords with perspective correction, w/out mipmapping.
 */
static void general_textured_spec_triangle( GLcontext *ctx,
					    const SWvertex *v0,
					    const SWvertex *v1,
					    const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;	\
   const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];\
   const GLboolean flatShade = (ctx->Light.ShadeModel == GL_FLAT);	\
   GLfixed rFlat, gFlat, bFlat, aFlat;					\
   GLfixed srFlat, sgFlat, sbFlat;					\
   if (flatShade) {							\
      rFlat = IntToFixed(v2->color[RCOMP]);				\
      gFlat = IntToFixed(v2->color[GCOMP]);				\
      bFlat = IntToFixed(v2->color[BCOMP]);				\
      aFlat = IntToFixed(v2->color[ACOMP]);				\
      srFlat = IntToFixed(v2->specular[RCOMP]);				\
      sgFlat = IntToFixed(v2->specular[GCOMP]);				\
      sbFlat = IntToFixed(v2->specular[BCOMP]);				\
   }									\
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )					\
   if (flatShade) {						\
      span.red = rFlat;    span.redStep = 0;			\
      span.green = gFlat;  span.greenStep = 0;			\
      span.blue = bFlat;   span.blueStep = 0;			\
      span.alpha = aFlat;  span.alphaStep = 0;			\
      span.specRed = srFlat;    span.specRedStep = 0;		\
      span.specGreen = sgFlat;  span.specGreenStep = 0;		\
      span.specBlue = sbFlat;   span.specBlueStep = 0;		\
   }								\
   rasterize_span(ctx, &span);

#include "s_tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,R with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_triangle( GLcontext *ctx,
				      const SWvertex *v0,
				      const SWvertex *v1,
				      const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define INTERP_LAMBDA 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;	\
   const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];\
   const GLboolean flatShade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLfixed rFlat, gFlat, bFlat, aFlat;					\
   GLfixed srFlat, sgFlat, sbFlat;					\
   if (flatShade) {							\
      rFlat = IntToFixed(v2->color[RCOMP]);				\
      gFlat = IntToFixed(v2->color[GCOMP]);				\
      bFlat = IntToFixed(v2->color[BCOMP]);				\
      aFlat = IntToFixed(v2->color[ACOMP]);				\
      srFlat = IntToFixed(v2->specular[RCOMP]);				\
      sgFlat = IntToFixed(v2->specular[GCOMP]);				\
      sbFlat = IntToFixed(v2->specular[BCOMP]);				\
   }									\
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )					\
   if (flatShade) {						\
      span.red = rFlat;    span.redStep = 0;			\
      span.green = gFlat;  span.greenStep = 0;			\
      span.blue = bFlat;   span.blueStep = 0;			\
      span.alpha = aFlat;  span.alphaStep = 0;			\
      span.specRed = srFlat;    span.specRedStep = 0;		\
      span.specGreen = sgFlat;  span.specGreenStep = 0;		\
      span.specBlue = sbFlat;   span.specBlueStep = 0;		\
   }								\
   rasterize_span(ctx, &span);

#include "s_tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * interpolation.
 * Interpolate S,T,R with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_spec_triangle( GLcontext *ctx,
					   const SWvertex *v0,
					   const SWvertex *v1,
					   const SWvertex *v2 )
{
#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define INTERP_LAMBDA 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;	\
   const struct gl_texture_image *texImage = obj->Image[obj->BaseLevel];\
   const GLboolean flatShade = (ctx->Light.ShadeModel == GL_FLAT);	\
   GLfixed rFlat, gFlat, bFlat, aFlat;					\
   GLfixed srFlat, sgFlat, sbFlat;					\
   if (flatShade) {							\
      rFlat = IntToFixed(v2->color[RCOMP]);				\
      gFlat = IntToFixed(v2->color[GCOMP]);				\
      bFlat = IntToFixed(v2->color[BCOMP]);				\
      aFlat = IntToFixed(v2->color[ACOMP]);				\
      srFlat = IntToFixed(v2->specular[RCOMP]);				\
      sgFlat = IntToFixed(v2->specular[GCOMP]);				\
      sbFlat = IntToFixed(v2->specular[BCOMP]);				\
   }									\
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )					\
   if (flatShade) {						\
      span.red = rFlat;    span.redStep = 0;			\
      span.green = gFlat;  span.greenStep = 0;			\
      span.blue = bFlat;   span.blueStep = 0;			\
      span.alpha = aFlat;  span.alphaStep = 0;			\
      span.specRed = srFlat;    span.specRedStep = 0;		\
      span.specGreen = sgFlat;  span.specGreenStep = 0;		\
      span.specBlue = sbFlat;   span.specBlueStep = 0;		\
   }								\
   rasterize_span(ctx, &span);

#include "s_tritemp.h"
}


/*
 * This is the big one!
 * Interpolate Z, RGB, Alpha, specular, fog, and N sets of texture coordinates
 * with lambda (LOD).
 * Yup, it's slow.
 */
static void
lambda_multitextured_triangle( GLcontext *ctx,
                               const SWvertex *v0,
                               const SWvertex *v1,
                               const SWvertex *v2 )
{

#define INTERP_Z 1
#define INTERP_FOG 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_SPEC 1
#define INTERP_MULTITEX 1
#define INTERP_LAMBDA 1

#define SETUP_CODE							\
   const GLboolean flatShade = (ctx->Light.ShadeModel == GL_FLAT);	\
   GLfixed rFlat, gFlat, bFlat, aFlat;					\
   GLfixed srFlat, sgFlat, sbFlat;					\
   GLuint u;								\
   if (flatShade) {							\
      rFlat = IntToFixed(v2->color[RCOMP]);				\
      gFlat = IntToFixed(v2->color[GCOMP]);				\
      bFlat = IntToFixed(v2->color[BCOMP]);				\
      aFlat = IntToFixed(v2->color[ACOMP]);				\
      srFlat = IntToFixed(v2->specular[RCOMP]);				\
      sgFlat = IntToFixed(v2->specular[GCOMP]);				\
      sbFlat = IntToFixed(v2->specular[BCOMP]);				\
   }									\
   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {			\
      if (ctx->Texture.Unit[u]._ReallyEnabled) {			\
         const struct gl_texture_object *texObj;			\
         const struct gl_texture_image *texImage;			\
         texObj = ctx->Texture.Unit[u]._Current;			\
         texImage = texObj->Image[texObj->BaseLevel];			\
         span.texWidth[u] = (GLfloat) texImage->Width;			\
         span.texHeight[u] = (GLfloat) texImage->Height;		\
      }									\
   }									\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )					\
   if (flatShade) {						\
      span.red = rFlat;    span.redStep = 0;			\
      span.green = gFlat;  span.greenStep = 0;			\
      span.blue = bFlat;   span.blueStep = 0;			\
      span.alpha = aFlat;  span.alphaStep = 0;			\
      span.specRed = srFlat;    span.specRedStep = 0;		\
      span.specGreen = sgFlat;  span.specGreenStep = 0;		\
      span.specBlue = sbFlat;   span.specBlueStep = 0;		\
   }								\
   rasterize_span(ctx, &span);

#include "s_tritemp.h"

}


static void occlusion_zless_triangle( GLcontext *ctx,
				      const SWvertex *v0,
				      const SWvertex *v1,
				      const SWvertex *v2 )
{
   if (ctx->OcclusionResult) {
      return;
   }

#define DO_OCCLUSION_TEST
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE

#define RENDER_SPAN( span )				\
   GLuint i;						\
   for (i = 0; i < span.count; i++) {			\
      GLdepth z = FixedToDepth(span.z);			\
      if (z < zRow[i]) {				\
         ctx->OcclusionResult = GL_TRUE;		\
         return;					\
      }							\
      span.z += span.zStep;				\
   }

#include "s_tritemp.h"
}

static void nodraw_triangle( GLcontext *ctx,
			     const SWvertex *v0,
			     const SWvertex *v1,
			     const SWvertex *v2 )
{
   (void) (ctx && v0 && v1 && v2);
}

void _swrast_add_spec_terms_triangle( GLcontext *ctx,
				      const SWvertex *v0,
				      const SWvertex *v1,
				      const SWvertex *v2 )
{
   SWvertex *ncv0 = (SWvertex *)v0; /* drop const qualifier */
   SWvertex *ncv1 = (SWvertex *)v1;
   SWvertex *ncv2 = (SWvertex *)v2;
   GLchan c[3][4];
   COPY_CHAN4( c[0], ncv0->color );
   COPY_CHAN4( c[1], ncv1->color );
   COPY_CHAN4( c[2], ncv2->color );
   ACC_3V( ncv0->color, ncv0->specular );
   ACC_3V( ncv1->color, ncv1->specular );
   ACC_3V( ncv2->color, ncv2->specular );
   SWRAST_CONTEXT(ctx)->SpecTriangle( ctx, ncv0, ncv1, ncv2 );
   COPY_CHAN4( ncv0->color, c[0] );
   COPY_CHAN4( ncv1->color, c[1] );
   COPY_CHAN4( ncv2->color, c[2] );
}



#ifdef DEBUG

/* record the current triangle function name */
static const char *triFuncName = NULL;

#define USE(triFunc)                   \
do {                                   \
    triFuncName = #triFunc;            \
    /*printf("%s\n", triFuncName);*/   \
    swrast->Triangle = triFunc;        \
} while (0)

#else

#define USE(triFunc)  swrast->Triangle = triFunc;

#endif




/*
 * Determine which triangle rendering function to use given the current
 * rendering context.
 *
 * Please update the summary flag _SWRAST_NEW_TRIANGLE if you add or
 * remove tests to this code.
 */
void
_swrast_choose_triangle( GLcontext *ctx )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const GLboolean rgbmode = ctx->Visual.rgbMode;

   if (ctx->Polygon.CullFlag &&
       ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) {
      USE(nodraw_triangle);
      return;
   }

   if (ctx->RenderMode==GL_RENDER) {

      if (ctx->Polygon.SmoothFlag) {
         _mesa_set_aa_triangle_function(ctx);
         ASSERT(swrast->Triangle);
         return;
      }

      if (ctx->Depth.OcclusionTest &&
          ctx->Depth.Test &&
          ctx->Depth.Mask == GL_FALSE &&
          ctx->Depth.Func == GL_LESS &&
          !ctx->Stencil.Enabled) {
         if ((rgbmode &&
              ctx->Color.ColorMask[0] == 0 &&
              ctx->Color.ColorMask[1] == 0 &&
              ctx->Color.ColorMask[2] == 0 &&
              ctx->Color.ColorMask[3] == 0)
             ||
             (!rgbmode && ctx->Color.IndexMask == 0)) {
            USE(occlusion_zless_triangle);
            return;
         }
      }

      if (ctx->Texture._ReallyEnabled) {
         /* Ugh, we do a _lot_ of tests to pick the best textured tri func */
	 const struct gl_texture_object *texObj2D;
         const struct gl_texture_image *texImg;
         GLenum minFilter, magFilter, envMode;
         GLint format;
         texObj2D = ctx->Texture.Unit[0].Current2D;
         texImg = texObj2D ? texObj2D->Image[texObj2D->BaseLevel] : NULL;
         format = texImg ? texImg->TexFormat->MesaFormat : -1;
         minFilter = texObj2D ? texObj2D->MinFilter : (GLenum) 0;
         magFilter = texObj2D ? texObj2D->MagFilter : (GLenum) 0;
         envMode = ctx->Texture.Unit[0].EnvMode;

         /* First see if we can used an optimized 2-D texture function */
         if (ctx->Texture._ReallyEnabled==TEXTURE0_2D
             && texObj2D->WrapS==GL_REPEAT
	     && texObj2D->WrapT==GL_REPEAT
             && texImg->Border==0
             && (format == MESA_FORMAT_RGB || format == MESA_FORMAT_RGBA)
	     && minFilter == magFilter
	     && ctx->Light.Model.ColorControl == GL_SINGLE_COLOR
	     && ctx->Texture.Unit[0].EnvMode != GL_COMBINE_EXT) {
	    if (ctx->Hint.PerspectiveCorrection==GL_FASTEST) {
	       if (minFilter == GL_NEAREST
		   && format == MESA_FORMAT_RGB
		   && (envMode == GL_REPLACE || envMode == GL_DECAL)
		   && ((swrast->_RasterMask == (DEPTH_BIT | TEXTURE_BIT)
			&& ctx->Depth.Func == GL_LESS
			&& ctx->Depth.Mask == GL_TRUE)
		       || swrast->_RasterMask == TEXTURE_BIT)
		   && ctx->Polygon.StippleFlag == GL_FALSE) {
		  if (swrast->_RasterMask == (DEPTH_BIT | TEXTURE_BIT)) {
		     USE(simple_z_textured_triangle);
		  }
		  else {
		     USE(simple_textured_triangle);
		  }
	       }
	       else {
                  if (ctx->Texture.Unit[0].EnvMode==GL_ADD) {
                     USE(general_textured_triangle);
                  }
                  else {
                     USE(affine_textured_triangle);
                  }
	       }
	    }
	    else {
#if 00 /* XXX these function have problems with texture coord interpolation */
	       if (filter==GL_NEAREST) {
                  USE(near_persp_textured_triangle);
	       }
	       else {
                  USE(lin_persp_textured_triangle);
	       }
#endif
               USE(general_textured_triangle);
	    }
	 }
         else {
            /* More complicated textures (mipmap, multi-tex, sep specular) */
            GLboolean needLambda;
            /* if mag filter != min filter we need to compute lambda */
            const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;
            if (obj && obj->MinFilter != obj->MagFilter)
               needLambda = GL_TRUE;
            else
               needLambda = GL_FALSE;
            if (ctx->Texture._ReallyEnabled > TEXTURE0_ANY) {
               USE(lambda_multitextured_triangle);
            }
            else if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
               /* separate specular color interpolation */
               if (needLambda) {
                  USE(lambda_textured_spec_triangle);
	       }
               else {
                  USE(general_textured_spec_triangle);
	       }
            }
            else {
               if (needLambda) {
		  USE(lambda_textured_triangle);
	       }
               else {
                  USE(general_textured_triangle);
	       }
            }
         }
      }
      else {
         ASSERT(!ctx->Texture._ReallyEnabled);
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* smooth shaded, no texturing, stippled or some raster ops */
            if (rgbmode) {
	       USE(smooth_rgba_triangle);
            }
            else {
               USE(smooth_ci_triangle);
            }
	 }
	 else {
	    /* flat shaded, no texturing, stippled or some raster ops */
            if (rgbmode) {
	       USE(flat_rgba_triangle);
            }
            else {
               USE(flat_ci_triangle);
            }
	 }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      USE(_mesa_feedback_triangle);
   }
   else {
      /* GL_SELECT mode */
      USE(_mesa_select_triangle);
   }
}
