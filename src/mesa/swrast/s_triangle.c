/* $Id: s_triangle.c,v 1.42 2001/12/17 04:47:57 brianp Exp $ */

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
   _mesa_write_monoindex_span(ctx, &span, v2->index, GL_POLYGON );

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
   GLuint i;						                \
   SW_SPAN_SET_FLAG(span.filledColor);			                \
   for (i = 0; i < span.end; i++) {					\
      span.color.index[i] = FixedToInt(span.index);			\
      span.index += span.indexStep;					\
   }									\
   _mesa_write_index_span(ctx, &span, GL_POLYGON);

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
   _mesa_write_monocolor_span(ctx, &span, v2->color, GL_POLYGON );

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
   GLuint i;				        	        \
   SW_SPAN_SET_FLAG(span.filledColor);			        \
   for (i = 0; i < span.end; i++) {				\
      span.color.rgba[i][RCOMP] = FixedToChan(span.red);	\
      span.color.rgba[i][GCOMP] = FixedToChan(span.green);	\
      span.color.rgba[i][BCOMP] = FixedToChan(span.blue);	\
      span.color.rgba[i][ACOMP] = FixedToChan(span.alpha);	\
      span.red += span.redStep;					\
      span.green += span.greenStep;				\
      span.blue += span.blueStep;				\
      span.alpha += span.alphaStep;				\
   }								\
   _mesa_write_rgba_span(ctx, &span, GL_POLYGON);

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
   GLuint i;								\
   SW_SPAN_SET_FLAG(span.filledColor);					\
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		\
   span.intTex[1] -= FIXED_HALF;					\
   for (i = 0; i < span.end; i++) {					\
      GLint s = FixedToInt(span.intTex[0]) & smask;			\
      GLint t = FixedToInt(span.intTex[1]) & tmask;			\
      GLint pos = (t << twidth_log2) + s;				\
      pos = pos + pos + pos;  /* multiply by 3 */			\
      span.color.rgb[i][RCOMP] = texture[pos];				\
      span.color.rgb[i][GCOMP] = texture[pos+1];			\
      span.color.rgb[i][BCOMP] = texture[pos+2];			\
      span.intTex[0] += span.intTexStep[0];				\
      span.intTex[1] += span.intTexStep[1];				\
   }									\
   (*swrast->Driver.WriteRGBSpan)(ctx, span.end, span.x, span.y,	\
                                  (CONST GLchan (*)[3]) span.color.rgb, NULL );

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
   GLuint i;				    				\
   span.intTex[0] -= FIXED_HALF; /* off-by-one error? */		\
   span.intTex[1] -= FIXED_HALF;					\
   SW_SPAN_SET_FLAG(span.filledColor);					\
   for (i = 0; i < span.end; i++) {					\
      const GLdepth z = FixedToDepth(span.z);				\
      if (z < zRow[i]) {						\
         GLint s = FixedToInt(span.intTex[0]) & smask;			\
         GLint t = FixedToInt(span.intTex[1]) & tmask;			\
         GLint pos = (t << twidth_log2) + s;				\
         pos = pos + pos + pos;  /* multiply by 3 */			\
         span.color.rgb[i][RCOMP] = texture[pos];			\
         span.color.rgb[i][GCOMP] = texture[pos+1];			\
         span.color.rgb[i][BCOMP] = texture[pos+2];			\
         zRow[i] = z;							\
         span.mask[i] = 1;						\
      }									\
      else {								\
         span.mask[i] = 0;						\
      }									\
      span.intTex[0] += span.intTexStep[0];				\
      span.intTex[1] += span.intTexStep[1];				\
      span.z += span.zStep;						\
   }									\
   (*swrast->Driver.WriteRGBSpan)(ctx, span.end, span.x, span.y,	\
                                  (CONST GLchan (*)[3]) span.color.rgb, span.mask );

#include "s_tritemp.h"
}


#if CHAN_TYPE != GL_FLOAT

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
};


/* This function can handle GL_NEAREST or GL_LINEAR sampling of 2D RGB or RGBA
 * textures with GL_REPLACE, GL_MODULATE, GL_BLEND, GL_DECAL or GL_ADD
 * texture env modes.
 */
static INLINE void
affine_span(GLcontext *ctx, struct sw_span *span,
            struct affine_info *info)
{
   GLchan sample[4];  /* the filtered texture sample */

   /* Instead of defining a function for each mode, a test is done
    * between the outer and inner loops. This is to reduce code size
    * and complexity. Observe that an optimizing compiler kills
    * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
    */

#define NEAREST_RGB			\
   sample[RCOMP] = tex00[RCOMP];	\
   sample[GCOMP] = tex00[GCOMP];	\
   sample[BCOMP] = tex00[BCOMP];	\
   sample[ACOMP] = CHAN_MAX

#define LINEAR_RGB							\
   sample[RCOMP] = (ti * (si * tex00[0] + sf * tex01[0]) +		\
             tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT;	\
   sample[GCOMP] = (ti * (si * tex00[1] + sf * tex01[1]) +		\
             tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT;	\
   sample[BCOMP] = (ti * (si * tex00[2] + sf * tex01[2]) +		\
             tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT;	\
   sample[ACOMP] = CHAN_MAX

#define NEAREST_RGBA  COPY_CHAN4(sample, tex00)

#define LINEAR_RGBA							\
   sample[RCOMP] = (ti * (si * tex00[0] + sf * tex01[0]) +		\
               tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT;\
   sample[GCOMP] = (ti * (si * tex00[1] + sf * tex01[1]) +		\
               tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT;\
   sample[BCOMP] = (ti * (si * tex00[2] + sf * tex01[2]) +		\
               tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT;\
   sample[ACOMP] = (ti * (si * tex00[3] + sf * tex01[3]) +		\
               tf * (si * tex10[3] + sf * tex11[3])) >> 2 * FIXED_SHIFT

#define MODULATE							   \
   dest[RCOMP] = span->red   * (sample[RCOMP] + 1u) >> (FIXED_SHIFT + 8); \
   dest[GCOMP] = span->green * (sample[GCOMP] + 1u) >> (FIXED_SHIFT + 8); \
   dest[BCOMP] = span->blue  * (sample[BCOMP] + 1u) >> (FIXED_SHIFT + 8); \
   dest[ACOMP] = span->alpha * (sample[ACOMP] + 1u) >> (FIXED_SHIFT + 8)

#define DECAL								\
   dest[RCOMP] = ((CHAN_MAX - sample[ACOMP]) * span->red +		\
               ((sample[ACOMP] + 1) * sample[RCOMP] << FIXED_SHIFT))	\
               >> (FIXED_SHIFT + 8);					\
   dest[GCOMP] = ((CHAN_MAX - sample[ACOMP]) * span->green +		\
               ((sample[ACOMP] + 1) * sample[GCOMP] << FIXED_SHIFT))	\
               >> (FIXED_SHIFT + 8);					\
   dest[BCOMP] = ((CHAN_MAX - sample[ACOMP]) * span->blue +		\
               ((sample[ACOMP] + 1) * sample[BCOMP] << FIXED_SHIFT))	\
               >> (FIXED_SHIFT + 8);					\
   dest[ACOMP] = FixedToInt(span->alpha)

#define BLEND								\
   dest[RCOMP] = ((CHAN_MAX - sample[RCOMP]) * span->red		\
               + (sample[RCOMP] + 1) * info->er) >> (FIXED_SHIFT + 8);	\
   dest[GCOMP] = ((CHAN_MAX - sample[GCOMP]) * span->green		\
               + (sample[GCOMP] + 1) * info->eg) >> (FIXED_SHIFT + 8);	\
   dest[BCOMP] = ((CHAN_MAX - sample[BCOMP]) * span->blue		\
               + (sample[BCOMP] + 1) * info->eb) >> (FIXED_SHIFT + 8);	\
   dest[ACOMP] = span->alpha * (sample[ACOMP] + 1) >> (FIXED_SHIFT + 8)

#define REPLACE  COPY_CHAN4(dest, sample)

#define ADD								\
   {									\
      GLint rSum = FixedToInt(span->red)   + (GLint) sample[RCOMP];	\
      GLint gSum = FixedToInt(span->green) + (GLint) sample[GCOMP];	\
      GLint bSum = FixedToInt(span->blue)  + (GLint) sample[BCOMP];	\
      dest[RCOMP] = MIN2(rSum, CHAN_MAX);				\
      dest[GCOMP] = MIN2(gSum, CHAN_MAX);				\
      dest[BCOMP] = MIN2(bSum, CHAN_MAX);				\
      dest[ACOMP] = span->alpha * (sample[ACOMP] + 1) >> (FIXED_SHIFT + 8); \
  }

/* shortcuts */

#define NEAREST_RGB_REPLACE  NEAREST_RGB;REPLACE

#define NEAREST_RGBA_REPLACE  COPY_CHAN4(dest, tex00)

#define SPAN_NEAREST(DO_TEX,COMP)					\
	for (i = 0; i < span->end; i++) {				\
           /* Isn't it necessary to use FixedFloor below?? */		\
           GLint s = FixedToInt(span->intTex[0]) & info->smask;		\
           GLint t = FixedToInt(span->intTex[1]) & info->tmask;		\
           GLint pos = (t << info->twidth_log2) + s;			\
           const GLchan *tex00 = info->texture + COMP * pos;		\
           DO_TEX;							\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   span->intTex[0] += span->intTexStep[0];			\
	   span->intTex[1] += span->intTexStep[1];			\
           dest += 4;							\
	}

#define SPAN_LINEAR(DO_TEX,COMP)					\
	for (i = 0; i < span->end; i++) {				\
           /* Isn't it necessary to use FixedFloor below?? */		\
           GLint s = FixedToInt(span->intTex[0]) & info->smask;		\
           GLint t = FixedToInt(span->intTex[1]) & info->tmask;		\
           GLfixed sf = span->intTex[0] & FIXED_FRAC_MASK;		\
           GLfixed tf = span->intTex[1] & FIXED_FRAC_MASK;		\
           GLfixed si = FIXED_FRAC_MASK - sf;				\
           GLfixed ti = FIXED_FRAC_MASK - tf;				\
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
           DO_TEX;							\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   span->intTex[0] += span->intTexStep[0];			\
	   span->intTex[1] += span->intTexStep[1];			\
           dest += 4;							\
	}


   GLuint i;
   GLchan *dest = span->color.rgba[0];

   SW_SPAN_SET_FLAG(span->filledColor);

   span->intTex[0] -= FIXED_HALF;
   span->intTex[1] -= FIXED_HALF;
   switch (info->filter) {
   case GL_NEAREST:
      switch (info->format) {
      case GL_RGB:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGB_REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGB;ADD,3);
            break;
         default:
            abort();
         }
         break;
      case GL_RGBA:
         switch(info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_NEAREST(NEAREST_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGBA_REPLACE,4);
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
            SPAN_LINEAR(LINEAR_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGB;REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGB;ADD,3);
            break;
         default:
            abort();
         }
         break;
      case GL_RGBA:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_LINEAR(LINEAR_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGBA;REPLACE,4);
            break;
         default:
            abort();
         }		    break;
      }
      break;
   }
   _mesa_write_rgba_span(ctx, span, GL_POLYGON);

#undef SPAN_NEAREST
#undef SPAN_LINEAR
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

#define SETUP_CODE							\
   struct affine_info info;						\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;			\
   struct gl_texture_object *obj = unit->Current2D;			\
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
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

#define RENDER_SPAN( span )   affine_span(ctx, &span, &info);

#include "s_tritemp.h"

}



struct persp_info
{
   GLenum filter;
   GLenum format;
   GLenum envmode;
   GLint smask, tmask;
   GLint twidth_log2;
   const GLchan *texture;
   GLchan er, eg, eb, ea;   /* texture env color */
   GLint tbytesline, tsize;
};


static INLINE void
fast_persp_span(GLcontext *ctx, struct sw_span *span,
		struct persp_info *info)
{
   GLchan sample[4];  /* the filtered texture sample */

  /* Instead of defining a function for each mode, a test is done
   * between the outer and inner loops. This is to reduce code size
   * and complexity. Observe that an optimizing compiler kills
   * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
   */
#define SPAN_NEAREST(DO_TEX,COMP)					\
	for (i = 0; i < span->end; i++) {				\
           GLdouble invQ = tex_coord[2] ?				\
                                 (1.0 / tex_coord[2]) : 1.0;            \
           GLfloat s_tmp = (GLfloat) (tex_coord[0] * invQ);		\
           GLfloat t_tmp = (GLfloat) (tex_coord[1] * invQ);		\
           GLint s = IFLOOR(s_tmp) & info->smask;	        	\
           GLint t = IFLOOR(t_tmp) & info->tmask;	        	\
           GLint pos = (t << info->twidth_log2) + s;			\
           const GLchan *tex00 = info->texture + COMP * pos;		\
           DO_TEX;							\
           span->red += span->redStep;					\
	   span->green += span->greenStep;				\
           span->blue += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   tex_coord[0] += tex_step[0];					\
	   tex_coord[1] += tex_step[1];					\
	   tex_coord[2] += tex_step[2];					\
           dest += 4;							\
	}

#define SPAN_LINEAR(DO_TEX,COMP)					\
	for (i = 0; i < span->end; i++) {				\
           GLdouble invQ = tex_coord[2] ?				\
                                 (1.0 / tex_coord[2]) : 1.0;            \
           GLfloat s_tmp = (GLfloat) (tex_coord[0] * invQ);		\
           GLfloat t_tmp = (GLfloat) (tex_coord[1] * invQ);		\
           GLfixed s_fix = FloatToFixed(s_tmp) - FIXED_HALF;		\
           GLfixed t_fix = FloatToFixed(t_tmp) - FIXED_HALF;        	\
           GLint s = FixedToInt(FixedFloor(s_fix)) & info->smask;	\
           GLint t = FixedToInt(FixedFloor(t_fix)) & info->tmask;	\
           GLfixed sf = s_fix & FIXED_FRAC_MASK;			\
           GLfixed tf = t_fix & FIXED_FRAC_MASK;			\
           GLfixed si = FIXED_FRAC_MASK - sf;				\
           GLfixed ti = FIXED_FRAC_MASK - tf;				\
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
           DO_TEX;							\
           span->red   += span->redStep;				\
	   span->green += span->greenStep;				\
           span->blue  += span->blueStep;				\
	   span->alpha += span->alphaStep;				\
	   tex_coord[0] += tex_step[0];					\
	   tex_coord[1] += tex_step[1];					\
	   tex_coord[2] += tex_step[2];					\
           dest += 4;							\
	}

   GLuint i;
   GLfloat tex_coord[3], tex_step[3];
   GLchan *dest = span->color.rgba[0];

   SW_SPAN_SET_FLAG(span->filledColor);

   tex_coord[0] = span->tex[0][0]  * (info->smask + 1),
     tex_step[0] = span->texStep[0][0] * (info->smask + 1);
   tex_coord[1] = span->tex[0][1] * (info->tmask + 1),
     tex_step[1] = span->texStep[0][1] * (info->tmask + 1);
   /* span->tex[0][2] only if 3D-texturing, here only 2D */
   tex_coord[2] = span->tex[0][3],
     tex_step[2] = span->texStep[0][3];

   switch (info->filter) {
   case GL_NEAREST:
      switch (info->format) {
      case GL_RGB:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGB_REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGB;ADD,3);
            break;
         default:
            abort();
         }
         break;
      case GL_RGBA:
         switch(info->envmode) {
         case GL_MODULATE:
            SPAN_NEAREST(NEAREST_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_NEAREST(NEAREST_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_NEAREST(NEAREST_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_NEAREST(NEAREST_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_NEAREST(NEAREST_RGBA_REPLACE,4);
            break;
         default:
            abort();
         }
         break;
      }
      break;

   case GL_LINEAR:
      switch (info->format) {
      case GL_RGB:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGB;MODULATE,3);
            break;
         case GL_DECAL:
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGB;REPLACE,3);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGB;BLEND,3);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGB;ADD,3);
            break;
         default:
            abort();
         }
         break;
      case GL_RGBA:
         switch (info->envmode) {
         case GL_MODULATE:
            SPAN_LINEAR(LINEAR_RGBA;MODULATE,4);
            break;
         case GL_DECAL:
            SPAN_LINEAR(LINEAR_RGBA;DECAL,4);
            break;
         case GL_BLEND:
            SPAN_LINEAR(LINEAR_RGBA;BLEND,4);
            break;
         case GL_ADD:
            SPAN_LINEAR(LINEAR_RGBA;ADD,4);
            break;
         case GL_REPLACE:
            SPAN_LINEAR(LINEAR_RGBA;REPLACE,4);
            break;
         default:
            abort();
         }
         break;
      }
      break;
   }
   
   _mesa_write_rgba_span(ctx, span, GL_POLYGON);


#undef SPAN_NEAREST
#undef SPAN_LINEAR
}


/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 *
 */
static void persp_textured_triangle( GLcontext *ctx,
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
   struct persp_info info;						\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;			\
   struct gl_texture_object *obj = unit->Current2D;			\
   GLint b = obj->BaseLevel;						\
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
      _mesa_problem(NULL, "Bad texture format in persp_textured_triangle");\
      return;								\
   }									\
   info.tsize = obj->Image[b]->Height * info.tbytesline;

#define RENDER_SPAN( span )   fast_persp_span(ctx, &span, &info);

#include "s_tritemp.h"

}


#endif /* CHAN_BITS != GL_FLOAT */

                


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
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )						\
   GLfloat fogSpan[MAX_WIDTH];						\
   GLuint i;								\
   SW_SPAN_SET_FLAG(span.filledColor);					\
   SW_SPAN_SET_FLAG(span.filledTex[0]);					\
   /* NOTE: we could just call rasterize_span() here instead */		\
   for (i = 0; i < span.end; i++) {					\
      GLdouble invQ = span.tex[0][3] ? (1.0 / span.tex[0][3]) : 1.0;	\
      span.depth[i] = FixedToDepth(span.z);				\
      span.z += span.zStep;						\
      fogSpan[i] = span.fog;			          		\
      span.fog += span.fogStep;					\
      span.color.rgba[i][RCOMP] = FixedToChan(span.red);		\
      span.color.rgba[i][GCOMP] = FixedToChan(span.green);		\
      span.color.rgba[i][BCOMP] = FixedToChan(span.blue);		\
      span.color.rgba[i][ACOMP] = FixedToChan(span.alpha);		\
      span.red += span.redStep;					\
      span.green += span.greenStep;					\
      span.blue += span.blueStep;					\
      span.alpha += span.alphaStep;					\
      span.texcoords[0][i][0] = (GLfloat) (span.tex[0][0] * invQ);	\
      span.texcoords[0][i][1] = (GLfloat) (span.tex[0][1] * invQ);	\
      span.texcoords[0][i][2] = (GLfloat) (span.tex[0][2] * invQ);	\
      span.tex[0][0] += span.texStep[0][0];				\
      span.tex[0][1] += span.texStep[0][1];				\
      span.tex[0][2] += span.texStep[0][2];				\
      span.tex[0][3] += span.texStep[0][3];				\
   }									\
   _old_write_texture_span( ctx, span.end, span.x, span.y,		\
                            span.depth, fogSpan,			\
			    span.texcoords[0],				\
                            NULL, span.color.rgba, NULL, NULL, GL_POLYGON );

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
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )   _mesa_rasterize_span(ctx, &span);

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
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )   _mesa_rasterize_span(ctx, &span);

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
   span.texWidth[0] = (GLfloat) texImage->Width;			\
   span.texHeight[0] = (GLfloat) texImage->Height;			\
   (void) fixedToDepthShift;

#define RENDER_SPAN( span )   _mesa_rasterize_span(ctx, &span);

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
   GLuint u;								\
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

#define RENDER_SPAN( span )   _mesa_rasterize_span(ctx, &span);

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
   for (i = 0; i < span.end; i++) {			\
      GLdepth z = FixedToDepth(span.z);		\
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
#if CHAN_TYPE == GL_FLOAT
                  USE(general_textured_triangle);
#else
                  USE(affine_textured_triangle);
#endif
	       }
	    }
	    else {
#if CHAN_TYPE == GL_FLOAT
               USE(general_textured_triangle);
#else
               USE(persp_textured_triangle);
#endif
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
