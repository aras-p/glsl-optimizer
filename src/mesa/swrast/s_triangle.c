/* $Id: s_triangle.c,v 1.14 2001/03/03 00:37:27 brianp Exp $ */

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
 * Triangle rasterizers
 * When the device driver doesn't implement triangle rasterization Mesa
 * will use these functions to draw triangles.
 */


#include "glheader.h"
#include "context.h"
#include "colormac.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "teximage.h"
#include "texstate.h"

#include "s_aatriangle.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_feedback.h"
#include "s_span.h"
#include "s_triangle.h"
 
GLboolean gl_cull_triangle( GLcontext *ctx,
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

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   const GLint n = RIGHT-LEFT;				\
	   GLint i;				                \
	   GLdepth zspan[MAX_WIDTH];				\
	   GLfixed fogspan[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 ffz += fdzdx;					\
		 fogspan[i] = fffog / 256;			\
		 fffog += fdfogdx;				\
	      }							\
	      gl_write_monoindex_span( ctx, n, LEFT, Y, zspan,	\
                         fogspan, v0->index, GL_POLYGON );	\
	   }							\
	}

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
#define INTERP_INDEX 1

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   const GLint n = RIGHT-LEFT;				\
	   GLint i;				                \
	   GLdepth zspan[MAX_WIDTH];				\
           GLfixed fogspan[MAX_WIDTH];				\
           GLuint index[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 ffz += fdzdx;					\
                 index[i] = FixedToInt(ffi);			\
		 ffi += fdidx;					\
		 fogspan[i] = fffog / 256;			\
		 fffog += fdfogdx;				\
	      }							\
	      gl_write_index_span( ctx, n, LEFT, Y, zspan, fogspan,	\
	                           index, GL_POLYGON );		\
	   }							\
	}

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
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   const GLint n = RIGHT-LEFT;				\
	   GLint i;						\
	   GLdepth zspan[MAX_WIDTH];				\
	   GLfixed fogspan[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 ffz += fdzdx;					\
		 fogspan[i] = fffog / 256;			\
		 fffog += fdfogdx;				\
	      }							\
              gl_write_monocolor_span( ctx, n, LEFT, Y, zspan,	\
                                       fogspan, v2->color,	\
			               GL_POLYGON );		\
	   }							\
	}

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
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   const GLint n = RIGHT-LEFT;                          \
	   GLint i;				                \
	   GLdepth zspan[MAX_WIDTH];				\
	   GLchan rgba[MAX_WIDTH][4];				\
	   GLfixed fogspan[MAX_WIDTH];				\
	   if (n>0) {						\
	      for (i=0;i<n;i++) {				\
		 zspan[i] = FixedToDepth(ffz);			\
		 rgba[i][RCOMP] = FixedToInt(ffr);		\
		 rgba[i][GCOMP] = FixedToInt(ffg);		\
		 rgba[i][BCOMP] = FixedToInt(ffb);		\
		 rgba[i][ACOMP] = FixedToInt(ffa);		\
		 fogspan[i] = fffog / 256;			\
		 fffog += fdfogdx;				\
		 ffz += fdzdx;					\
		 ffr += fdrdx;					\
		 ffg += fdgdx;					\
		 ffb += fdbdx;					\
		 ffa += fdadx;					\
	      }							\
	      gl_write_rgba_span( ctx, n, LEFT, Y,		\
                                  (CONST GLdepth *) zspan,	\
                                  fogspan,                      \
	                          rgba, GL_POLYGON );		\
	   }							\
	}

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

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   CONST GLint n = RIGHT-LEFT;				\
	   GLint i;				                \
	   GLchan rgb[MAX_WIDTH][3];				\
	   if (n>0) {						\
              ffs -= FIXED_HALF; /* off-by-one error? */        \
              fft -= FIXED_HALF;                                \
	      for (i=0;i<n;i++) {				\
                 GLint s = FixedToInt(ffs) & smask;		\
                 GLint t = FixedToInt(fft) & tmask;		\
                 GLint pos = (t << twidth_log2) + s;		\
                 pos = pos + pos + pos;  /* multiply by 3 */	\
                 rgb[i][RCOMP] = texture[pos];			\
                 rgb[i][GCOMP] = texture[pos+1];		\
                 rgb[i][BCOMP] = texture[pos+2];		\
		 ffs += fdsdx;					\
		 fft += fdtdx;					\
	      }							\
              (*ctx->Driver.WriteRGBSpan)( ctx, n, LEFT, Y,	\
                           (CONST GLchan (*)[3]) rgb, NULL );	\
	   }							\
	}

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

#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   CONST GLint n = RIGHT-LEFT;                          \
	   GLint i;				                \
	   GLchan rgb[MAX_WIDTH][3];				\
           GLubyte mask[MAX_WIDTH];				\
           (void) fffog;                                        \
	   if (n>0) {						\
              ffs -= FIXED_HALF; /* off-by-one error? */        \
              fft -= FIXED_HALF;                                \
	      for (i=0;i<n;i++) {				\
                 GLdepth z = FixedToDepth(ffz);			\
                 if (z < zRow[i]) {				\
                    GLint s = FixedToInt(ffs) & smask;		\
                    GLint t = FixedToInt(fft) & tmask;		\
                    GLint pos = (t << twidth_log2) + s;		\
                    pos = pos + pos + pos;  /* multiply by 3 */	\
                    rgb[i][RCOMP] = texture[pos];		\
                    rgb[i][GCOMP] = texture[pos+1];		\
                    rgb[i][BCOMP] = texture[pos+2];		\
		    zRow[i] = z;				\
                    mask[i] = 1;				\
                 }						\
                 else {						\
                    mask[i] = 0;				\
                 }						\
		 ffz += fdzdx;					\
		 ffs += fdsdx;					\
		 fft += fdtdx;					\
	      }							\
              (*ctx->Driver.WriteRGBSpan)( ctx, n, LEFT, Y,	\
                           (CONST GLchan (*)[3]) rgb, mask );	\
	   }							\
	}

#include "s_tritemp.h"
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
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_INT_TEX 1
#define S_SCALE twidth
#define T_SCALE theight
#define SETUP_CODE							\
   struct gl_texture_unit *unit = ctx->Texture.Unit+0;			\
   struct gl_texture_object *obj = unit->Current2D;			\
   GLint b = obj->BaseLevel;						\
   GLfloat twidth = (GLfloat) obj->Image[b]->Width;			\
   GLfloat theight = (GLfloat) obj->Image[b]->Height;			\
   GLint twidth_log2 = obj->Image[b]->WidthLog2;			\
   const GLchan *texture = (const GLchan *) obj->Image[b]->Data;	\
   GLint smask = obj->Image[b]->Width - 1;				\
   GLint tmask = obj->Image[b]->Height - 1;                             \
   GLint format = obj->Image[b]->Format;                                \
   GLint filter = obj->MinFilter;                                       \
   GLint envmode = unit->EnvMode;                                       \
   GLint comp, tbytesline, tsize;                                       \
   GLfixed er, eg, eb, ea;                                              \
   GLint tr, tg, tb, ta;                                                \
   if (!texture) {							\
      /* this shouldn't happen */					\
      return;								\
   }									\
   if (envmode == GL_BLEND || envmode == GL_ADD) {                      \
      /* potential off-by-one error here? (1.0f -> 2048 -> 0) */        \
      er = FloatToFixed(unit->EnvColor[RCOMP]);                         \
      eg = FloatToFixed(unit->EnvColor[GCOMP]);                         \
      eb = FloatToFixed(unit->EnvColor[BCOMP]);                         \
      ea = FloatToFixed(unit->EnvColor[ACOMP]);                         \
   }                                                                    \
   switch (format) {                                                    \
   case GL_ALPHA:                                                       \
   case GL_LUMINANCE:                                                   \
   case GL_INTENSITY:                                                   \
      comp = 1;                                                         \
      break;                                                            \
   case GL_LUMINANCE_ALPHA:                                             \
      comp = 2;                                                         \
      break;                                                            \
   case GL_RGB:                                                         \
      comp = 3;                                                         \
      break;                                                            \
   case GL_RGBA:                                                        \
      comp = 4;                                                         \
      break;                                                            \
   default:                                                             \
      gl_problem(NULL, "Bad texture format in affine_texture_triangle");\
      return;                                                           \
   }                                                                    \
   tbytesline = obj->Image[b]->Width * comp;                            \
   tsize = theight * tbytesline;


  /* Instead of defining a function for each mode, a test is done 
   * between the outer and inner loops. This is to reduce code size
   * and complexity. Observe that an optimizing compiler kills 
   * unused variables (for instance tf,sf,ti,si in case of GL_NEAREST).
   */ 

#define NEAREST_RGB    \
        tr = tex00[RCOMP]; \
        tg = tex00[GCOMP]; \
        tb = tex00[BCOMP]; \
        ta = 0xff

#define LINEAR_RGB                                                      \
	tr = (ti * (si * tex00[0] + sf * tex01[0]) +                    \
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT; \
	tg = (ti * (si * tex00[1] + sf * tex01[1]) +                    \
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT; \
	tb = (ti * (si * tex00[2] + sf * tex01[2]) +                    \
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT; \
	ta = 0xff

#define NEAREST_RGBA   \
        tr = tex00[RCOMP]; \
        tg = tex00[GCOMP]; \
        tb = tex00[BCOMP]; \
        ta = tex00[ACOMP]

#define LINEAR_RGBA                                                     \
	tr = (ti * (si * tex00[0] + sf * tex01[0]) +                    \
              tf * (si * tex10[0] + sf * tex11[0])) >> 2 * FIXED_SHIFT; \
	tg = (ti * (si * tex00[1] + sf * tex01[1]) +                    \
              tf * (si * tex10[1] + sf * tex11[1])) >> 2 * FIXED_SHIFT; \
	tb = (ti * (si * tex00[2] + sf * tex01[2]) +                    \
              tf * (si * tex10[2] + sf * tex11[2])) >> 2 * FIXED_SHIFT; \
	ta = (ti * (si * tex00[3] + sf * tex01[3]) +                    \
              tf * (si * tex10[3] + sf * tex11[3])) >> 2 * FIXED_SHIFT

#define MODULATE                                       \
        dest[RCOMP] = ffr * (tr + 1) >> (FIXED_SHIFT + 8); \
        dest[GCOMP] = ffg * (tg + 1) >> (FIXED_SHIFT + 8); \
        dest[BCOMP] = ffb * (tb + 1) >> (FIXED_SHIFT + 8); \
        dest[ACOMP] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

#define DECAL                                                                \
	dest[RCOMP] = ((0xff - ta) * ffr + ((ta + 1) * tr << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
	dest[GCOMP] = ((0xff - ta) * ffg + ((ta + 1) * tg << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
	dest[BCOMP] = ((0xff - ta) * ffb + ((ta + 1) * tb << FIXED_SHIFT)) >> (FIXED_SHIFT + 8); \
	dest[ACOMP] = FixedToInt(ffa)

#define BLEND                                                               \
        dest[RCOMP] = ((0xff - tr) * ffr + (tr + 1) * er) >> (FIXED_SHIFT + 8); \
        dest[GCOMP] = ((0xff - tg) * ffg + (tg + 1) * eg) >> (FIXED_SHIFT + 8); \
        dest[BCOMP] = ((0xff - tb) * ffb + (tb + 1) * eb) >> (FIXED_SHIFT + 8); \
        dest[ACOMP] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

#define REPLACE       \
        dest[RCOMP] = tr; \
        dest[GCOMP] = tg; \
        dest[BCOMP] = tb; \
        dest[ACOMP] = ta

#define ADD                                                          \
        dest[RCOMP] = ((ffr << 8) + (tr + 1) * er) >> (FIXED_SHIFT + 8); \
        dest[GCOMP] = ((ffg << 8) + (tg + 1) * eg) >> (FIXED_SHIFT + 8); \
        dest[BCOMP] = ((ffb << 8) + (tb + 1) * eb) >> (FIXED_SHIFT + 8); \
        dest[ACOMP] = ffa * (ta + 1) >> (FIXED_SHIFT + 8)

/* shortcuts */

#define NEAREST_RGB_REPLACE  NEAREST_RGB;REPLACE

#define NEAREST_RGBA_REPLACE  *(GLint *)dest = *(GLint *)tex00

#define SPAN1(DO_TEX,COMP)                                 \
	for (i=0;i<n;i++) {                                \
           GLint s = FixedToInt(ffs) & smask;              \
           GLint t = FixedToInt(fft) & tmask;              \
           GLint pos = (t << twidth_log2) + s;             \
           const GLchan *tex00 = texture + COMP * pos;     \
	   zspan[i] = FixedToDepth(ffz);                   \
	   fogspan[i] = fffog / 256;                       \
           DO_TEX;                                         \
	   fffog += fdfogdx;                               \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
	   ffs += fdsdx;                                   \
	   fft += fdtdx;                                   \
           dest += 4;                                      \
	}

#define SPAN2(DO_TEX,COMP)                                 \
	for (i=0;i<n;i++) {                                \
           GLint s = FixedToInt(ffs) & smask;              \
           GLint t = FixedToInt(fft) & tmask;              \
           GLint sf = ffs & FIXED_FRAC_MASK;               \
           GLint tf = fft & FIXED_FRAC_MASK;               \
           GLint si = FIXED_FRAC_MASK - sf;                \
           GLint ti = FIXED_FRAC_MASK - tf;                \
           GLint pos = (t << twidth_log2) + s;             \
           const GLchan *tex00 = texture + COMP * pos;     \
           const GLchan *tex10 = tex00 + tbytesline;       \
           const GLchan *tex01 = tex00 + COMP;             \
           const GLchan *tex11 = tex10 + COMP;             \
           if (t == tmask) {                               \
              tex10 -= tsize;                              \
              tex11 -= tsize;                              \
           }                                               \
           if (s == smask) {                               \
              tex01 -= tbytesline;                         \
              tex11 -= tbytesline;                         \
           }                                               \
	   zspan[i] = FixedToDepth(ffz);                   \
	   fogspan[i] = fffog / 256;                       \
           DO_TEX;                                         \
	   fffog += fdfogdx;                               \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
	   ffs += fdsdx;                                   \
	   fft += fdtdx;                                   \
           dest += 4;                                      \
	}

/* here comes the heavy part.. (something for the compiler to chew on) */
#define INNER_LOOP( LEFT, RIGHT, Y )	                   \
	{				                   \
           CONST GLint n = RIGHT-LEFT;	                   \
	   GLint i;                                        \
	   GLdepth zspan[MAX_WIDTH];	                   \
	   GLfixed fogspan[MAX_WIDTH];	                   \
	   GLchan rgba[MAX_WIDTH][4];                      \
	   if (n>0) {                                      \
              GLchan *dest = rgba[0];                      \
              ffs -= FIXED_HALF; /* off-by-one error? */   \
              fft -= FIXED_HALF;                           \
              switch (filter) {                            \
   	      case GL_NEAREST:                             \
		 switch (format) {                         \
                 case GL_RGB:                              \
	            switch (envmode) {                     \
	            case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGB;MODULATE,3);      \
                       break;                              \
	            case GL_DECAL:                         \
                    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGB_REPLACE,3);       \
                       break;                              \
                    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGB;BLEND,3);         \
                       break;                              \
		    case GL_ADD:                           \
		       SPAN1(NEAREST_RGB;ADD,3);           \
		       break;                              \
                    default: /* unexpected env mode */     \
                       abort();                            \
	            }                                      \
                    break;                                 \
		 case GL_RGBA:                             \
		    switch(envmode) {                      \
		    case GL_MODULATE:                      \
                       SPAN1(NEAREST_RGBA;MODULATE,4);     \
                       break;                              \
		    case GL_DECAL:                         \
                       SPAN1(NEAREST_RGBA;DECAL,4);        \
                       break;                              \
		    case GL_BLEND:                         \
                       SPAN1(NEAREST_RGBA;BLEND,4);        \
                       break;                              \
		    case GL_ADD:                           \
		       SPAN1(NEAREST_RGBA;ADD,4);          \
		       break;                              \
		    case GL_REPLACE:                       \
                       SPAN1(NEAREST_RGBA_REPLACE,4);      \
                       break;                              \
                    default: /* unexpected env mode */     \
                       abort();                            \
		    }                                      \
                    break;                                 \
	         }                                         \
                 break;                                    \
	      case GL_LINEAR:                              \
                 ffs -= FIXED_HALF;                        \
                 fft -= FIXED_HALF;                        \
		 switch (format) {                         \
		 case GL_RGB:                              \
		    switch (envmode) {                     \
		    case GL_MODULATE:                      \
		       SPAN2(LINEAR_RGB;MODULATE,3);       \
                       break;                              \
		    case GL_DECAL:                         \
		    case GL_REPLACE:                       \
                       SPAN2(LINEAR_RGB;REPLACE,3);        \
                       break;                              \
		    case GL_BLEND:                         \
		       SPAN2(LINEAR_RGB;BLEND,3);          \
		       break;                              \
		    case GL_ADD:                           \
		       SPAN2(LINEAR_RGB;ADD,3);            \
		       break;                              \
                    default: /* unexpected env mode */     \
                       abort();                            \
		    }                                      \
		    break;                                 \
		 case GL_RGBA:                             \
		    switch (envmode) {                     \
		    case GL_MODULATE:                      \
		       SPAN2(LINEAR_RGBA;MODULATE,4);      \
		       break;                              \
		    case GL_DECAL:                         \
		       SPAN2(LINEAR_RGBA;DECAL,4);         \
		       break;                              \
		    case GL_BLEND:                         \
		       SPAN2(LINEAR_RGBA;BLEND,4);         \
		       break;                              \
		    case GL_ADD:                           \
		       SPAN2(LINEAR_RGBA;ADD,4);           \
		       break;                              \
		    case GL_REPLACE:                       \
		       SPAN2(LINEAR_RGBA;REPLACE,4);       \
		       break;                              \
                    default: /* unexpected env mode */     \
                       abort();                            \
		    }                                      \
		    break;                                 \
	         }                                         \
                 break;                                    \
	      }                                            \
              gl_write_rgba_span(ctx, n, LEFT, Y, zspan,   \
                                 fogspan,                  \
                                 rgba, GL_POLYGON);        \
              /* explicit kill of variables: */            \
              ffr = ffg = ffb = ffa = 0;                   \
           }                                               \
	}

#include "s_tritemp.h"
#undef SPAN1
#undef SPAN2
}



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
#if 0 /* XXX disabled because of texcoord interpolation errors */
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
      zspan[i] = FixedToDepth(ffz);                   \
      fogspan[i] = fffog / 256;                       \
      DO_TEX;                                         \
      fffog += fdfogdx;                               \
      ffz += fdzdx;                                   \
      ffr += fdrdx;                                   \
      ffg += fdgdx;                                   \
      ffb += fdbdx;                                   \
      ffa += fdadx;                                   \
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
         zspan[i] = FixedToDepth(ffz);	\
         fogspan[i] = fffog / 256;      \
         DO_TEX;			\
         fffog += fdfogdx;              \
         ffz += fdzdx;			\
         ffr += fdrdx;			\
         ffg += fdgdx;			\
         ffb += fdbdx;			\
         ffa += fdadx;			\
         dest += 4;			\
         i++;				\
      }					\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
   {									\
      GLint i = 0;							\
      const GLint n = RIGHT-LEFT;					\
      GLdepth zspan[MAX_WIDTH];						\
      GLfixed fogspan[MAX_WIDTH];					\
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
         gl_write_rgba_span( ctx, n, LEFT, Y, zspan,			\
                             fogspan, rgba, GL_POLYGON);		\
         ffr = ffg = ffb = ffa = 0;					\
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


/*
 * Render an perspective corrected RGB/RGBA textured triangle.
 * The Q (aka V in Mesa) coordinate must be zero such that the divide
 * by interpolated Q/W comes out right.
 *
 * This function written by Klaus Niederkrueger <klaus@math.leidenuniv.nl>
 * Send all questions and bug reports to him.
 */
#if 0 /* XXX disabled because of texcoord interpolation errors */
static void lin_persp_textured_triangle( GLcontext *ctx,
					 const SWvertex *v0,
					 const SWvertex *v1,
					 const SWvertex *v2 )
{
#define INTERP_Z 1
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
      gl_problem(NULL, "Bad texture format in lin_persp_texture_triangle"); \
      return;                                                           \
   }                                                                    \
   sscale = FIXED_SCALE * twidth;                                       \
   tscale = FIXED_SCALE * theight;                                      \
   tbytesline = obj->Image[b]->Width * comp;                            \
   tsize = theight * tbytesline;


#define SPAN(DO_TEX,COMP)                                  \
        for (i=0;i<n;i++) {                                \
           GLfloat invQ = 1.0f / vv;                       \
           GLfixed ffs = (int)(SS * invQ);                 \
           GLfixed fft = (int)(TT * invQ);                 \
	   GLint s = FixedToInt(ffs) & smask;              \
	   GLint t = FixedToInt(fft) & tmask;              \
           GLint sf = ffs & FIXED_FRAC_MASK;               \
           GLint tf = fft & FIXED_FRAC_MASK;               \
           GLint si = FIXED_FRAC_MASK - sf;                \
           GLint ti = FIXED_FRAC_MASK - tf;                \
           GLint pos = COMP * ((t << twidth_log2) + s);    \
           GLchan *tex00 = texture + pos;                  \
           GLchan *tex10 = tex00 + tbytesline;             \
           GLchan *tex01 = tex00 + COMP;                   \
           GLchan *tex11 = tex10 + COMP;                   \
           if (t == tmask) {                               \
              tex10 -= tsize;                              \
              tex11 -= tsize;                              \
           }                                               \
           if (s == smask) {                               \
              tex01 -= tbytesline;                         \
              tex11 -= tbytesline;                         \
           }                                               \
	   zspan[i] = FixedToDepth(ffz);                   \
	   fogspan[i] = fffog / 256;                       \
           DO_TEX;                                         \
	   fffog += fdfogdx;                               \
	   ffz += fdzdx;                                   \
           ffr += fdrdx;                                   \
	   ffg += fdgdx;                                   \
           ffb += fdbdx;                                   \
	   ffa += fdadx;                                   \
           SS += dSdx;                                     \
           TT += dTdx;                                     \
	   vv += dvdx;                                     \
           dest += 4;                                      \
	}

#define INNER_LOOP( LEFT, RIGHT, Y )			\
   {							\
      GLint i;						\
      const GLint n = RIGHT-LEFT;			\
      GLdepth zspan[MAX_WIDTH];				\
      GLfixed fogspan[MAX_WIDTH];			\
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
         gl_write_rgba_span( ctx, n, LEFT, Y, zspan,	\
                             fogspan,                   \
                             rgba, GL_POLYGON );	\
         ffr = ffg = ffb = ffa = 0;			\
      }							\
   }

#include "s_tritemp.h"
#undef SPAN
}
#endif


/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,U with perspective correction, w/out mipmapping.
 * Note: we use texture coordinates S,T,U,V instead of S,T,R,Q because
 * R is already used for red.
 */
static void general_textured_triangle( GLcontext *ctx,
				       const SWvertex *v0,
				       const SWvertex *v1,
				       const SWvertex *v2 )
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define SETUP_CODE						\
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a;						\
   if (flat_shade) {						\
      r = v2->color[RCOMP];					\
      g = v2->color[GCOMP];					\
      b = v2->color[BCOMP];					\
      a = v2->color[ACOMP];					\
   }
#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i;                                             \
           const GLint n = RIGHT-LEFT;				\
	   GLdepth zspan[MAX_WIDTH];				\
	   GLfixed fogspan[MAX_WIDTH];				\
	   GLchan rgba[MAX_WIDTH][4];				\
           GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];	\
	   if (n>0) {						\
              if (flat_shade) {					\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;	\
		    zspan[i] = FixedToDepth(ffz);		\
		    fogspan[i] = fffog / 256;          		\
		    rgba[i][RCOMP] = r;				\
		    rgba[i][GCOMP] = g;				\
		    rgba[i][BCOMP] = b;				\
		    rgba[i][ACOMP] = a;				\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
	 	    u[i] = uu*invQ;				\
 		    fffog += fdfogdx;				\
		    ffz += fdzdx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
              else {						\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;	\
		    zspan[i] = FixedToDepth(ffz);		\
		    rgba[i][RCOMP] = FixedToInt(ffr);		\
		    rgba[i][GCOMP] = FixedToInt(ffg);		\
		    rgba[i][BCOMP] = FixedToInt(ffb);		\
		    rgba[i][ACOMP] = FixedToInt(ffa);		\
		    fogspan[i] = fffog / 256;         		\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    fffog += fdfogdx;				\
		    ffz += fdzdx;				\
		    ffr += fdrdx;				\
		    ffg += fdgdx;				\
		    ffb += fdbdx;				\
		    ffa += fdadx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan, fogspan,	\
                                     s, t, u, NULL, 		\
	                             rgba, \
                                     NULL, GL_POLYGON );	\
	   }							\
	}

#include "s_tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * color interpolation.
 * Interpolate S,T,U with perspective correction, w/out mipmapping.
 * Note: we use texture coordinates S,T,U,V instead of S,T,R,Q because
 * R is already used for red.
 */
static void general_textured_spec_triangle1( GLcontext *ctx,
					     const SWvertex *v0,
					     const SWvertex *v1,
					     const SWvertex *v2,
                                             GLdepth zspan[MAX_WIDTH],
                                             GLfixed fogspan[MAX_WIDTH],
                                             GLchan rgba[MAX_WIDTH][4],
                                             GLchan spec[MAX_WIDTH][4] )
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define SETUP_CODE						\
   GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a, sr, sg, sb;				\
   if (flat_shade) {						\
      r = v2->color[RCOMP];					\
      g = v2->color[GCOMP];					\
      b = v2->color[BCOMP];					\
      a = v2->color[ACOMP];					\
      sr = v2->specular[RCOMP];					\
      sg = v2->specular[GCOMP];					\
      sb = v2->specular[BCOMP];					\
   }
#define INNER_LOOP( LEFT, RIGHT, Y )				\
	{							\
	   GLint i;                                             \
           const GLint n = RIGHT-LEFT;				\
           GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];	\
	   if (n>0) {						\
              if (flat_shade) {					\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;	\
		    zspan[i] = FixedToDepth(ffz);		\
		    fogspan[i] = fffog / 256;			\
		    rgba[i][RCOMP] = r;				\
		    rgba[i][GCOMP] = g;				\
		    rgba[i][BCOMP] = b;				\
		    rgba[i][ACOMP] = a;				\
		    spec[i][RCOMP] = sr;			\
		    spec[i][GCOMP] = sg;			\
		    spec[i][BCOMP] = sb;			\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    fffog += fdfogdx;				\
		    ffz += fdzdx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
              else {						\
                 for (i=0;i<n;i++) {				\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;	\
		    zspan[i] = FixedToDepth(ffz);		\
		    fogspan[i] = fffog / 256;			\
		    rgba[i][RCOMP] = FixedToInt(ffr);		\
		    rgba[i][GCOMP] = FixedToInt(ffg);		\
		    rgba[i][BCOMP] = FixedToInt(ffb);		\
		    rgba[i][ACOMP] = FixedToInt(ffa);		\
		    spec[i][RCOMP] = FixedToInt(ffsr);		\
		    spec[i][GCOMP] = FixedToInt(ffsg);		\
		    spec[i][BCOMP] = FixedToInt(ffsb);		\
		    s[i] = ss*invQ;				\
		    t[i] = tt*invQ;				\
		    u[i] = uu*invQ;				\
		    fffog += fdfogdx;				\
		    ffz += fdzdx;				\
		    ffr += fdrdx;				\
		    ffg += fdgdx;				\
		    ffb += fdbdx;				\
		    ffa += fdadx;				\
		    ffsr += fdsrdx;				\
		    ffsg += fdsgdx;				\
		    ffsb += fdsbdx;				\
		    ss += dsdx;					\
		    tt += dtdx;					\
		    uu += dudx;					\
		    vv += dvdx;					\
		 }						\
              }							\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan,	\
                                   fogspan,                     \
                                   s, t, u, NULL, rgba,		\
                                   (CONST GLchan (*)[4]) spec,	\
	                           GL_POLYGON );		\
	   }							\
	}

#include "s_tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle.
 * Interpolate S,T,U with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_triangle1( GLcontext *ctx,
				       const SWvertex *v0,
				       const SWvertex *v1,
				       const SWvertex *v2, 
                                       GLfloat s[MAX_WIDTH],
                                       GLfloat t[MAX_WIDTH],
                                       GLfloat u[MAX_WIDTH] )
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_LAMBDA 1
#define INTERP_TEX 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;	\
   const GLint baseLevel = obj->BaseLevel;				\
   const struct gl_texture_image *texImage = obj->Image[baseLevel];	\
   const GLfloat twidth = (GLfloat) texImage->Width;			\
   const GLfloat theight = (GLfloat) texImage->Height;			\
   const GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a;							\
   if (flat_shade) {							\
      r = v2->color[RCOMP];						\
      g = v2->color[GCOMP];						\
      b = v2->color[BCOMP];						\
      a = v2->color[ACOMP];						\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
	{								\
	   GLint i;                                                     \
           const GLint n = RIGHT-LEFT;					\
	   GLdepth zspan[MAX_WIDTH];					\
	   GLfixed fogspan[MAX_WIDTH];					\
	   GLchan rgba[MAX_WIDTH][4];					\
	   GLfloat lambda[MAX_WIDTH];					\
	   if (n>0) {							\
	      if (flat_shade) {						\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;		\
		    zspan[i] = FixedToDepth(ffz);			\
		    fogspan[i] = fffog / 256;          			\
		    rgba[i][RCOMP] = r;					\
		    rgba[i][GCOMP] = g;					\
		    rgba[i][BCOMP] = b;					\
		    rgba[i][ACOMP] = a;					\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
                    COMPUTE_LAMBDA(lambda[i], invQ);			\
		    ffz += fdzdx;					\
		    fffog += fdfogdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
              else {							\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;		\
		    zspan[i] = FixedToDepth(ffz);			\
		    fogspan[i] = fffog / 256;          			\
		    rgba[i][RCOMP] = FixedToInt(ffr);			\
		    rgba[i][GCOMP] = FixedToInt(ffg);			\
		    rgba[i][BCOMP] = FixedToInt(ffb);			\
		    rgba[i][ACOMP] = FixedToInt(ffa);			\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
                    COMPUTE_LAMBDA(lambda[i], invQ);			\
		    ffz += fdzdx;					\
		    fffog += fdfogdx;					\
		    ffr += fdrdx;					\
		    ffg += fdgdx;					\
		    ffb += fdbdx;					\
		    ffa += fdadx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan, fogspan,	\
                                     s, t, u, lambda,	 		\
	                             rgba, NULL, GL_POLYGON );		\
	   }								\
	}

#include "s_tritemp.h"
}


/*
 * Render a smooth-shaded, textured, RGBA triangle with separate specular
 * interpolation.
 * Interpolate S,T,U with perspective correction and compute lambda for
 * each fragment.  Lambda is used to determine whether to use the
 * minification or magnification filter.  If minification and using
 * mipmaps, lambda is also used to select the texture level of detail.
 */
static void lambda_textured_spec_triangle1( GLcontext *ctx,
					    const SWvertex *v0,
					    const SWvertex *v1,
					    const SWvertex *v2,
                                            GLfloat s[MAX_WIDTH],
                                            GLfloat t[MAX_WIDTH],
                                            GLfloat u[MAX_WIDTH] )
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define INTERP_LAMBDA 1

#define SETUP_CODE							\
   const struct gl_texture_object *obj = ctx->Texture.Unit[0]._Current;	\
   const GLint baseLevel = obj->BaseLevel;				\
   const struct gl_texture_image *texImage = obj->Image[baseLevel];	\
   const GLfloat twidth = (GLfloat) texImage->Width;			\
   const GLfloat theight = (GLfloat) texImage->Height;			\
   const GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLint r, g, b, a, sr, sg, sb;					\
   if (flat_shade) {							\
      r = v2->color[RCOMP];						\
      g = v2->color[GCOMP];						\
      b = v2->color[BCOMP];						\
      a = v2->color[ACOMP];						\
      sr = v2->specular[RCOMP];						\
      sg = v2->specular[GCOMP];						\
      sb = v2->specular[BCOMP];						\
   }

#define INNER_LOOP( LEFT, RIGHT, Y )					\
	{								\
	   GLint i;                                                     \
           const GLint n = RIGHT-LEFT;					\
	   GLdepth zspan[MAX_WIDTH];					\
	   GLfixed fogspan[MAX_WIDTH];					\
	   GLchan spec[MAX_WIDTH][4];					\
           GLchan rgba[MAX_WIDTH][4];					\
	   GLfloat lambda[MAX_WIDTH];					\
	   if (n>0) {							\
	      if (flat_shade) {						\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;		\
		    zspan[i] = FixedToDepth(ffz);			\
		    fogspan[i] = fffog / 256;				\
		    rgba[i][RCOMP] = r;					\
		    rgba[i][GCOMP] = g;					\
		    rgba[i][BCOMP] = b;					\
		    rgba[i][ACOMP] = a;					\
		    spec[i][RCOMP] = sr;				\
		    spec[i][GCOMP] = sg;				\
		    spec[i][BCOMP] = sb;				\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
                    COMPUTE_LAMBDA(lambda[i], invQ);			\
		    fffog += fdfogdx;					\
		    ffz += fdzdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
              else {							\
		 for (i=0;i<n;i++) {					\
		    GLdouble invQ = vv ? (1.0 / vv) : 1.0;		\
		    zspan[i] = FixedToDepth(ffz);			\
		    fogspan[i] = fffog / 256;				\
		    rgba[i][RCOMP] = FixedToInt(ffr);			\
		    rgba[i][GCOMP] = FixedToInt(ffg);			\
		    rgba[i][BCOMP] = FixedToInt(ffb);			\
		    rgba[i][ACOMP] = FixedToInt(ffa);			\
		    spec[i][RCOMP] = FixedToInt(ffsr);			\
		    spec[i][GCOMP] = FixedToInt(ffsg);			\
		    spec[i][BCOMP] = FixedToInt(ffsb);			\
		    s[i] = ss*invQ;					\
		    t[i] = tt*invQ;					\
		    u[i] = uu*invQ;					\
                    COMPUTE_LAMBDA(lambda[i], invQ);			\
		    fffog += fdfogdx;					\
		    ffz += fdzdx;					\
		    ffr += fdrdx;					\
		    ffg += fdgdx;					\
		    ffb += fdbdx;					\
		    ffa += fdadx;					\
		    ffsr += fdsrdx;					\
		    ffsg += fdsgdx;					\
		    ffsb += fdsbdx;					\
		    ss += dsdx;						\
		    tt += dtdx;						\
		    uu += dudx;						\
		    vv += dvdx;						\
		 }							\
              }								\
	      gl_write_texture_span( ctx, n, LEFT, Y, zspan, fogspan,	\
                                     s, t, u, lambda,	 		\
	                             rgba, (CONST GLchan (*)[4]) spec,	\
                                     GL_POLYGON );			\
	   }								\
	}

#include "s_tritemp.h"
}


/*
 * This is the big one!
 * Interpolate Z, RGB, Alpha, and two sets of texture coordinates.
 * Yup, it's slow.
 */
static void 
lambda_multitextured_triangle1( GLcontext *ctx,
				const SWvertex *v0,
				const SWvertex *v1,
				const SWvertex *v2,
				GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH],
				GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH],
				GLfloat u[MAX_TEXTURE_UNITS][MAX_WIDTH])
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define INTERP_MULTILAMBDA 1

#define SETUP_CODE							\
   GLchan rgba[MAX_WIDTH][4];						\
   const GLboolean flat_shade = (ctx->Light.ShadeModel==GL_FLAT);	\
   GLfloat twidth[MAX_TEXTURE_UNITS], theight[MAX_TEXTURE_UNITS];	\
   GLint r, g, b, a;							\
   if (flat_shade) {							\
      r = v2->color[0];							\
      g = v2->color[1];							\
      b = v2->color[2];							\
      a = v2->color[3];							\
   }									\
   {									\
      GLuint unit;							\
      for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {	\
         if (ctx->Texture.Unit[unit]._ReallyEnabled) {			\
            const struct gl_texture_object *obj = ctx->Texture.Unit[unit]._Current; \
            const GLint baseLevel = obj->BaseLevel;			\
            const struct gl_texture_image *texImage = obj->Image[baseLevel];\
            twidth[unit] = (GLfloat) texImage->Width;			\
            theight[unit] = (GLfloat) texImage->Height;			\
         }								\
      }									\
   }



#define INNER_LOOP( LEFT, RIGHT, Y )					\
   {									\
      GLint i;								\
      const GLint n = RIGHT-LEFT;					\
      GLdepth zspan[MAX_WIDTH];						\
      GLfixed fogspan[MAX_WIDTH];					\
      GLfloat lambda[MAX_TEXTURE_UNITS][MAX_WIDTH];			\
      if (n > 0) {							\
         if (flat_shade) {						\
	    for (i=0;i<n;i++) {						\
	       zspan[i] = FixedToDepth(ffz);				\
	       fogspan[i] = fffog / 256;				\
               fffog += fdfogdx;					\
	       ffz += fdzdx;						\
	       rgba[i][RCOMP] = r;					\
	       rgba[i][GCOMP] = g;					\
	       rgba[i][BCOMP] = b;					\
	       rgba[i][ACOMP] = a;					\
	       {							\
		  GLuint unit;						\
		  for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {\
		     if (ctx->Texture.Unit[unit]._ReallyEnabled) {	\
			GLdouble invQ = 1.0 / vv[unit];			\
			s[unit][i] = ss[unit] * invQ;			\
			t[unit][i] = tt[unit] * invQ;			\
			u[unit][i] = uu[unit] * invQ;			\
                        COMPUTE_MULTILAMBDA(lambda[unit][i], invQ, unit);\
			ss[unit] += dsdx[unit];				\
			tt[unit] += dtdx[unit];				\
			uu[unit] += dudx[unit];				\
			vv[unit] += dvdx[unit];				\
		     }							\
		  }							\
	       }							\
	    }								\
	 }								\
	 else { /* smooth shade */					\
	    for (i=0;i<n;i++) {						\
	       zspan[i] = FixedToDepth(ffz);				\
	       fogspan[i] = fffog / 256;				\
	       ffz += fdzdx;						\
	       fffog += fdfogdx;					\
	       rgba[i][RCOMP] = FixedToInt(ffr);			\
	       rgba[i][GCOMP] = FixedToInt(ffg);			\
	       rgba[i][BCOMP] = FixedToInt(ffb);			\
	       rgba[i][ACOMP] = FixedToInt(ffa);			\
	       ffr += fdrdx;						\
	       ffg += fdgdx;						\
	       ffb += fdbdx;						\
	       ffa += fdadx;						\
	       {							\
		  GLuint unit;						\
		  for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {\
		     if (ctx->Texture.Unit[unit]._ReallyEnabled) {	\
			GLdouble invQ = 1.0 / vv[unit];			\
			s[unit][i] = ss[unit] * invQ;			\
			t[unit][i] = tt[unit] * invQ;			\
			u[unit][i] = uu[unit] * invQ;			\
                        COMPUTE_MULTILAMBDA(lambda[unit][i], invQ, unit);  \
			ss[unit] += dsdx[unit];				\
			tt[unit] += dtdx[unit];				\
			uu[unit] += dudx[unit];				\
			vv[unit] += dvdx[unit];				\
		     }							\
		  }							\
	       }							\
	    }								\
	 }								\
	 gl_write_multitexture_span( ctx, n, LEFT, Y, zspan, fogspan,	\
				     (const GLfloat (*)[MAX_WIDTH]) s,	\
				     (const GLfloat (*)[MAX_WIDTH]) t,	\
				     (const GLfloat (*)[MAX_WIDTH]) u,	\
				     (GLfloat (*)[MAX_WIDTH]) lambda,	\
				     rgba, NULL, GL_POLYGON );		\
      }									\
   }
#include "s_tritemp.h"
}


/*
 * These wrappers are needed to deal with the 32KB / stack frame limit
 * on Mac / PowerPC systems.
 */

static void general_textured_spec_triangle(GLcontext *ctx,
					   const SWvertex *v0,
					   const SWvertex *v1,
					   const SWvertex *v2 )
{
   GLdepth zspan[MAX_WIDTH];
   GLfixed fogspan[MAX_WIDTH];	                   
   GLchan rgba[MAX_WIDTH][4], spec[MAX_WIDTH][4];
   general_textured_spec_triangle1(ctx,v0,v1,v2,zspan,fogspan,rgba,spec);
}

static void lambda_textured_triangle( GLcontext *ctx,
				      const SWvertex *v0,
				      const SWvertex *v1,
				      const SWvertex *v2 )
{
   GLfloat s[MAX_WIDTH], t[MAX_WIDTH], u[MAX_WIDTH];
   lambda_textured_triangle1(ctx,v0,v1,v2,s,t,u);
}

static void lambda_textured_spec_triangle( GLcontext *ctx,
					   const SWvertex *v0,
					   const SWvertex *v1,
					   const SWvertex *v2 )
{
   GLfloat s[MAX_WIDTH];
   GLfloat t[MAX_WIDTH];
   GLfloat u[MAX_WIDTH];
   lambda_textured_spec_triangle1(ctx,v0,v1,v2,s,t,u);
}


static void lambda_multitextured_triangle( GLcontext *ctx,
					   const SWvertex *v0,
					   const SWvertex *v1,
					   const SWvertex *v2 )
{

   GLfloat s[MAX_TEXTURE_UNITS][MAX_WIDTH];
   GLfloat t[MAX_TEXTURE_UNITS][MAX_WIDTH];
   DEFMARRAY(GLfloat,u,MAX_TEXTURE_UNITS,MAX_WIDTH);
   CHECKARRAY(u,return);
   
   lambda_multitextured_triangle1(ctx,v0,v1,v2,s,t,u);
   
   UNDEFARRAY(u);
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
#define INNER_LOOP( LEFT, RIGHT, Y )		\
   {						\
      GLint i;                                  \
      const GLint len = RIGHT-LEFT;		\
      for (i=0;i<len;i++) {			\
	 GLdepth z = FixedToDepth(ffz);		\
         (void) fffog;                          \
	 if (z < zRow[i]) {			\
	    ctx->OcclusionResult = GL_TRUE;	\
	    return;				\
	 }					\
	 ffz += fdzdx;				\
      }						\
   }
#include "s_tritemp.h"
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



#if 0
# define dputs(s) puts(s)
#else
# define dputs(s)
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
            dputs("occlusion_test_triangle");
            swrast->Triangle = occlusion_zless_triangle;
            return;
         }
      }

      if (ctx->Texture._ReallyEnabled) {
         /* Ugh, we do a _lot_ of tests to pick the best textured tri func */
	 GLint format, filter;
	 const struct gl_texture_object *current2Dtex = ctx->Texture.Unit[0].Current2D;
         const struct gl_texture_image *image;
         /* First see if we can used an optimized 2-D texture function */
         if (ctx->Texture._ReallyEnabled==TEXTURE0_2D
             && current2Dtex->WrapS==GL_REPEAT
	     && current2Dtex->WrapT==GL_REPEAT
             && ((image = current2Dtex->Image[current2Dtex->BaseLevel]) != 0)  /* correct! */
             && image->Border==0
             && ((format = image->Format)==GL_RGB || format==GL_RGBA)
             && image->Type == CHAN_TYPE
	     && (filter = current2Dtex->MinFilter)==current2Dtex->MagFilter
	     /* ==> current2Dtex->MinFilter != GL_XXX_MIPMAP_XXX */
	     && ctx->Light.Model.ColorControl==GL_SINGLE_COLOR
	     && ctx->Texture.Unit[0].EnvMode!=GL_COMBINE_EXT) {

	    if (ctx->Hint.PerspectiveCorrection==GL_FASTEST) {
	     
	       if (filter==GL_NEAREST
		   && format==GL_RGB
		   && (ctx->Texture.Unit[0].EnvMode==GL_REPLACE
		       || ctx->Texture.Unit[0].EnvMode==GL_DECAL)
		   && ((swrast->_RasterMask==DEPTH_BIT
			&& ctx->Depth.Func==GL_LESS
			&& ctx->Depth.Mask==GL_TRUE)
		       || swrast->_RasterMask==0)
		   && ctx->Polygon.StippleFlag==GL_FALSE) {

		  if (swrast->_RasterMask==DEPTH_BIT) {
		     swrast->Triangle = simple_z_textured_triangle;
		     dputs("simple_z_textured_triangle");
		  }
		  else {
		     swrast->Triangle = simple_textured_triangle;
		     dputs("simple_textured_triangle");
		  }
	       }
	       else {
                  if (ctx->Texture.Unit[0].EnvMode==GL_ADD) {
                     swrast->Triangle = general_textured_triangle;
                     dputs("general_textured_triangle");
                  }
                  else {
                     swrast->Triangle = affine_textured_triangle;
                     dputs("affine_textured_triangle");
                  }
	       }
	    }
	    else {
#if 00 /* XXX these function have problems with texture coord interpolation */
	       if (filter==GL_NEAREST) {
		 swrast->Triangle = near_persp_textured_triangle;
		 dputs("near_persp_textured_triangle");
	       }
	       else {
		 swrast->Triangle = lin_persp_textured_triangle;
		 dputs("lin_persp_textured_triangle");
	       }
#endif
               swrast->Triangle = general_textured_triangle;
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
            if (swrast->_MultiTextureEnabled) {
               swrast->Triangle = lambda_multitextured_triangle;
	       dputs("lambda_multitextured_triangle");
            }
            else if (ctx->_TriangleCaps & DD_SEPERATE_SPECULAR) {
               /* separate specular color interpolation */
               if (needLambda) {
                  swrast->Triangle = lambda_textured_spec_triangle;
		  dputs("lambda_textured_spec_triangle");
	       }
               else {
                  swrast->Triangle = general_textured_spec_triangle;
		  dputs("general_textured_spec_triangle");
	       }
            }
            else {
               if (needLambda) {
		  swrast->Triangle = lambda_textured_triangle;
		  dputs("lambda_textured_triangle");
	       }
               else {
                  swrast->Triangle = general_textured_triangle;
		  dputs("general_textured_triangle");
	       }
            }
         }
      }
      else {
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* smooth shaded, no texturing, stippled or some raster ops */
            if (rgbmode) {
	       dputs("smooth_rgba_triangle");
	       swrast->Triangle = smooth_rgba_triangle;
            }
            else {
               dputs("smooth_ci_triangle");
               swrast->Triangle = smooth_ci_triangle;
            }
	 }
	 else {
	    /* flat shaded, no texturing, stippled or some raster ops */
            if (rgbmode) {
	       dputs("flat_rgba_triangle");
	       swrast->Triangle = flat_rgba_triangle;
            }
            else {
               dputs("flat_ci_triangle");
               swrast->Triangle = flat_ci_triangle;
            }
	 }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      swrast->Triangle = gl_feedback_triangle;
   }
   else {
      /* GL_SELECT mode */
      swrast->Triangle = gl_select_triangle;
   }
}
