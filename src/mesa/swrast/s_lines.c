/* $Id: s_lines.c,v 1.19 2001/06/11 19:44:01 brianp Exp $ */

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


#include "glheader.h"
#include "colormac.h"
#include "macros.h"
#include "mmath.h"
#include "s_aaline.h"
#include "s_pb.h"
#include "s_context.h"
#include "s_depth.h"
#include "s_lines.h"
#include "s_feedback.h"



/**********************************************************************/
/*****                    Rasterization                           *****/
/**********************************************************************/


/*
 * There are 4 pairs (RGBA, CI) of line drawing functions:
 *   1. simple:  width=1 and no special rasterization functions (fastest)
 *   2. flat:  width=1, non-stippled, flat-shaded, any raster operations
 *   3. smooth:  width=1, non-stippled, smooth-shaded, any raster operations
 *   4. general:  any other kind of line (slowest)
 */


/* Flat, color index line */
static void flat_ci_line( GLcontext *ctx,
                          const SWvertex *vert0,
			  const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;

   PB_SET_INDEX( PB, vert0->index );

#define INTERP_XY 1
#define PLOT(X,Y)  PB_WRITE_PIXEL(PB, X, Y, 0, 0);

#include "s_linetemp.h"

   _mesa_flush_pb(ctx);
}



/* Flat, color index line with Z interpolation/testing */
static void flat_ci_z_line( GLcontext *ctx,
                            const SWvertex *vert0,
			    const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   PB_SET_INDEX( PB, vert0->index );

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define PLOT(X,Y)  PB_WRITE_PIXEL(PB, X, Y, Z, fog0);

#include "s_linetemp.h"

   _mesa_flush_pb(ctx);
}



/* Flat-shaded, RGBA line */
static void flat_rgba_line( GLcontext *ctx,
                            const SWvertex *vert0,
			    const SWvertex *vert1 )
{
   const GLchan *color = vert1->color;
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   PB_SET_COLOR( PB, color[0], color[1], color[2], color[3] );

#define INTERP_XY 1
#define PLOT(X,Y)   PB_WRITE_PIXEL(PB, X, Y, 0, 0);

#include "s_linetemp.h"

   _mesa_flush_pb(ctx);
}



/* Flat-shaded, RGBA line with Z interpolation/testing */
static void flat_rgba_z_line( GLcontext *ctx,
                              const SWvertex *vert0,
			      const SWvertex *vert1 )
{
   const GLchan *color = vert1->color;
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   PB_SET_COLOR( PB, color[0], color[1], color[2], color[3] );

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define PLOT(X,Y)   PB_WRITE_PIXEL(PB, X, Y, Z, fog0);

#include "s_linetemp.h"

   _mesa_flush_pb(ctx);
}



/* Smooth shaded, color index line */
static void smooth_ci_line( GLcontext *ctx,
                            const SWvertex *vert0,
			    const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLuint *pbi = PB->index;

   PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbi[count] = I;		\
	count++;

#include "s_linetemp.h"

   PB->count = count;
   _mesa_flush_pb(ctx);
}



/* Smooth shaded, color index line with Z interpolation/testing */
static void smooth_ci_z_line( GLcontext *ctx,
                              const SWvertex *vert0,
			      const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLuint *pbi = PB->index;

   PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;

#include "s_linetemp.h"

   PB->count = count;
   _mesa_flush_pb(ctx);
}



/* Smooth-shaded, RGBA line */
static void smooth_rgba_line( GLcontext *ctx,
                       	      const SWvertex *vert0,
			      const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLchan (*pbrgba)[4] = PB->rgba;

   PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;

#include "s_linetemp.h"

   PB->count = count;
   _mesa_flush_pb(ctx);
}



/* Smooth-shaded, RGBA line with Z interpolation/testing */
static void smooth_rgba_z_line( GLcontext *ctx,
                       	        const SWvertex *vert0,
				const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLchan (*pbrgba)[4] = PB->rgba;


   PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define PLOT(X,Y)				\
	pbx[count] = X;				\
	pby[count] = Y;				\
	pbz[count] = Z;				\
	pbfog[count] = fog0;			\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;

#include "s_linetemp.h"

   PB->count = count;
   _mesa_flush_pb(ctx);
}


#define CHECK_FULL(count)		\
   if (count >= PB_SIZE-MAX_WIDTH) {	\
      PB->count = count;		\
      _mesa_flush_pb(ctx);			\
      count = PB->count;		\
   }



/* Smooth shaded, color index, any width, maybe stippled */
static void general_smooth_ci_line( GLcontext *ctx,
                           	    const SWvertex *vert0,
				    const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLuint *pbi = PB->index;

   PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_INDEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbfog[count] = fog0;	\
	pbi[count] = I;		\
	count++;		\
	CHECK_FULL(count);
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_INDEX 1
#define XMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X;		\
	pby[count] = Y;  pby[count+1] = Y+1;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
	pbfog[count] = fog0;  pbfog[count+1] = fog0;	\
	pbi[count] = I;  pbi[count+1] = I;		\
	count += 2;					\
	CHECK_FULL(count);
#define YMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X+1;		\
	pby[count] = Y;  pby[count+1] = Y;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
	pbfog[count] = fog0;  pbfog[count+1] = fog0;	\
	pbi[count] = I;  pbi[count+1] = I;		\
	count += 2;					\
	CHECK_FULL(count);
#include "s_linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_INDEX 1
#define WIDE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	pbfog[count] = fog0;	\
	count++;		\
	CHECK_FULL(count);
#include "s_linetemp.h"
      }
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
}


/* Flat shaded, color index, any width, maybe stippled */
static void general_flat_ci_line( GLcontext *ctx,
                                  const SWvertex *vert0,
				  const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   PB_SET_INDEX( PB, vert0->index );
   count = PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbfog[count] = fog0;	\
	count++;		\
	CHECK_FULL(count);
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define XMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X;		\
	pby[count] = Y;  pby[count+1] = Y+1;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
	pbfog[count] = fog0;  pbfog[count+1] = fog0;	\
	count += 2;					\
	CHECK_FULL(count);
#define YMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X+1;		\
	pby[count] = Y;  pby[count+1] = Y;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
	pbfog[count] = fog0;  pbfog[count+1] = fog0;	\
	count += 2;					\
	CHECK_FULL(count);
#include "s_linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define WIDE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbfog[count] = fog0;	\
	count++;		\
	CHECK_FULL(count);
#include "s_linetemp.h"
      }
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
}



static void general_smooth_rgba_line( GLcontext *ctx,
                                      const SWvertex *vert0,
				      const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLchan (*pbrgba)[4] = PB->rgba;

   PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)				\
	pbx[count] = X;				\
	pby[count] = Y;				\
	pbz[count] = Z;				\
	pbfog[count] = fog0;			\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;				\
	CHECK_FULL(count);
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define XMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X;		\
	pby[count] = Y;  pby[count+1] = Y+1;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
	pbfog[count] = fog0;  pbfog[count+1] = fog0;	\
	pbrgba[count][RCOMP] = FixedToInt(r0);		\
	pbrgba[count][GCOMP] = FixedToInt(g0);		\
	pbrgba[count][BCOMP] = FixedToInt(b0);		\
	pbrgba[count][ACOMP] = FixedToInt(a0);		\
	pbrgba[count+1][RCOMP] = FixedToInt(r0);	\
	pbrgba[count+1][GCOMP] = FixedToInt(g0);	\
	pbrgba[count+1][BCOMP] = FixedToInt(b0);	\
	pbrgba[count+1][ACOMP] = FixedToInt(a0);	\
	count += 2;					\
	CHECK_FULL(count);
#define YMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X+1;		\
	pby[count] = Y;  pby[count+1] = Y;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
	pbfog[count] = fog0;  pbfog[count+1] = fog0;	\
	pbrgba[count][RCOMP] = FixedToInt(r0);		\
	pbrgba[count][GCOMP] = FixedToInt(g0);		\
	pbrgba[count][BCOMP] = FixedToInt(b0);		\
	pbrgba[count][ACOMP] = FixedToInt(a0);		\
	pbrgba[count+1][RCOMP] = FixedToInt(r0);	\
	pbrgba[count+1][GCOMP] = FixedToInt(g0);	\
	pbrgba[count+1][BCOMP] = FixedToInt(b0);	\
	pbrgba[count+1][ACOMP] = FixedToInt(a0);	\
	count += 2;					\
	CHECK_FULL(count);
#include "s_linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define WIDE 1
#define PLOT(X,Y)				\
	pbx[count] = X;				\
	pby[count] = Y;				\
	pbz[count] = Z;				\
	pbfog[count] = fog0;  			\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;				\
	CHECK_FULL(count);
#include "s_linetemp.h"
      }
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
}


static void general_flat_rgba_line( GLcontext *ctx,
                                    const SWvertex *vert0,
				    const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   const GLchan *color = vert1->color;
   GLuint count;
   PB_SET_COLOR( PB, color[0], color[1], color[2], color[3] );

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)                       \
    PB_WRITE_PIXEL(PB, X, Y, Z, fog0);  \
    count = PB->count;                  \
    CHECK_FULL(count);
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define XMAJOR_PLOT(X,Y) PB_WRITE_PIXEL(PB, X, Y, Z, fog0); \
                         PB_WRITE_PIXEL(PB, X, Y+1, Z, fog0);
#define YMAJOR_PLOT(X,Y)  PB_WRITE_PIXEL(PB, X, Y, Z, fog0); \
                          PB_WRITE_PIXEL(PB, X+1, Y, Z, fog0);
#include "s_linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define WIDE 1
#define PLOT(X,Y)                       \
    PB_WRITE_PIXEL(PB, X, Y, Z, fog0);  \
    count = PB->count;                  \
    CHECK_FULL(count);
#include "s_linetemp.h"
      }
   }

   _mesa_flush_pb(ctx);
}


/* Flat-shaded, textured, any width, maybe stippled */
static void flat_textured_line( GLcontext *ctx,
                                const SWvertex *vert0,
				const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLfloat *pbs = PB->s[0];
   GLfloat *pbt = PB->t[0];
   GLfloat *pbu = PB->u[0];
   GLchan *color = (GLchan*) vert1->color;
   PB_SET_COLOR( PB, color[0], color[1], color[2], color[3] );
   count = PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_TEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	{				\
	   pbx[count] = X;		\
	   pby[count] = Y;		\
	   pbz[count] = Z;		\
 	   pbfog[count] = fog0;		\
	   pbs[count] = fragTexcoord[0];\
	   pbt[count] = fragTexcoord[1];\
	   pbu[count] = fragTexcoord[2];\
	   count++;			\
	   CHECK_FULL(count);		\
	}
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_TEX 1
#define WIDE 1
#define PLOT(X,Y)			\
	{				\
	   pbx[count] = X;		\
	   pby[count] = Y;		\
	   pbz[count] = Z;		\
 	   pbfog[count] = fog0;		\
	   pbs[count] = fragTexcoord[0];\
	   pbt[count] = fragTexcoord[1];\
	   pbu[count] = fragTexcoord[2];\
	   count++;			\
	   CHECK_FULL(count);		\
	}
#include "s_linetemp.h"
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
}



/* Smooth-shaded, textured, any width, maybe stippled */
static void smooth_textured_line( GLcontext *ctx,
                                  const SWvertex *vert0,
				  const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLfloat *pbs = PB->s[0];
   GLfloat *pbt = PB->t[0];
   GLfloat *pbu = PB->u[0];
   GLchan (*pbrgba)[4] = PB->rgba;

   PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
 	   pbfog[count] = fog0;				\
	   pbs[count] = fragTexcoord[0];		\
	   pbt[count] = fragTexcoord[1];		\
	   pbu[count] = fragTexcoord[2];		\
	   pbrgba[count][RCOMP] = FixedToInt(r0);	\
	   pbrgba[count][GCOMP] = FixedToInt(g0);	\
	   pbrgba[count][BCOMP] = FixedToInt(b0);	\
	   pbrgba[count][ACOMP] = FixedToInt(a0);	\
	   count++;					\
	   CHECK_FULL(count);				\
	}
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define WIDE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
 	   pbfog[count] = fog0;				\
	   pbs[count] = fragTexcoord[0];		\
	   pbt[count] = fragTexcoord[1];		\
	   pbu[count] = fragTexcoord[2];		\
	   pbrgba[count][RCOMP] = FixedToInt(r0);	\
	   pbrgba[count][GCOMP] = FixedToInt(g0);	\
	   pbrgba[count][BCOMP] = FixedToInt(b0);	\
	   pbrgba[count][ACOMP] = FixedToInt(a0);	\
	   count++;					\
	   CHECK_FULL(count);				\
	}
#include "s_linetemp.h"
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
}


/* Smooth-shaded, multitextured, any width, maybe stippled, separate specular
 * color interpolation.
 */
static void smooth_multitextured_line( GLcontext *ctx,
				       const SWvertex *vert0,
				       const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLchan (*pbrgba)[4] = PB->rgba;
   GLchan (*pbspec)[3] = PB->spec;

   PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)						\
	{							\
	   GLuint u;						\
	   pbx[count] = X;					\
	   pby[count] = Y;					\
	   pbz[count] = Z;					\
 	   pbfog[count] = fog0;					\
	   pbrgba[count][RCOMP] = FixedToInt(r0);		\
	   pbrgba[count][GCOMP] = FixedToInt(g0);		\
	   pbrgba[count][BCOMP] = FixedToInt(b0);		\
	   pbrgba[count][ACOMP] = FixedToInt(a0);		\
	   pbspec[count][RCOMP] = FixedToInt(sr0);		\
	   pbspec[count][GCOMP] = FixedToInt(sg0);		\
	   pbspec[count][BCOMP] = FixedToInt(sb0);		\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u]._ReallyEnabled) {	\
	         PB->s[u][count] = fragTexcoord[u][0];		\
	         PB->t[u][count] = fragTexcoord[u][1];		\
	         PB->u[u][count] = fragTexcoord[u][2];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define WIDE 1
#define PLOT(X,Y)						\
	{							\
	   GLuint u;						\
	   pbx[count] = X;					\
	   pby[count] = Y;					\
	   pbz[count] = Z;					\
 	   pbfog[count] = fog0;					\
	   pbrgba[count][RCOMP] = FixedToInt(r0);		\
	   pbrgba[count][GCOMP] = FixedToInt(g0);		\
	   pbrgba[count][BCOMP] = FixedToInt(b0);		\
	   pbrgba[count][ACOMP] = FixedToInt(a0);		\
	   pbspec[count][RCOMP] = FixedToInt(sr0);		\
	   pbspec[count][GCOMP] = FixedToInt(sg0);		\
	   pbspec[count][BCOMP] = FixedToInt(sb0);		\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u]._ReallyEnabled) {	\
	         PB->s[u][count] = fragTexcoord[u][0];		\
	         PB->t[u][count] = fragTexcoord[u][1];		\
	         PB->u[u][count] = fragTexcoord[u][2];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "s_linetemp.h"
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
}


/* Flat-shaded, multitextured, any width, maybe stippled, separate specular
 * color interpolation.
 */
static void flat_multitextured_line( GLcontext *ctx,
                                     const SWvertex *vert0,
				     const SWvertex *vert1 )
{
   struct pixel_buffer *PB = SWRAST_CONTEXT(ctx)->PB;
   GLint count = PB->count;
   GLint *pbx = PB->x;
   GLint *pby = PB->y;
   GLdepth *pbz = PB->z;
   GLfloat *pbfog = PB->fog;
   GLchan (*pbrgba)[4] = PB->rgba;
   GLchan (*pbspec)[3] = PB->spec;
   GLchan *color = (GLchan*) vert1->color;
   GLchan sRed   = vert1->specular[0];
   GLchan sGreen = vert1->specular[1];
   GLchan sBlue  = vert1->specular[2];

   PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)						\
	{							\
	   GLuint u;						\
	   pbx[count] = X;					\
	   pby[count] = Y;					\
	   pbz[count] = Z;					\
 	   pbfog[count] = fog0;					\
	   pbrgba[count][RCOMP] = color[0];			\
	   pbrgba[count][GCOMP] = color[1];			\
	   pbrgba[count][BCOMP] = color[2];			\
	   pbrgba[count][ACOMP] = color[3];			\
	   pbspec[count][RCOMP] = sRed;				\
	   pbspec[count][GCOMP] = sGreen;			\
	   pbspec[count][BCOMP] = sBlue;			\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u]._ReallyEnabled) {	\
	         PB->s[u][count] = fragTexcoord[u][0];		\
	         PB->t[u][count] = fragTexcoord[u][1];		\
	         PB->u[u][count] = fragTexcoord[u][2];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "s_linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_FOG 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define WIDE 1
#define PLOT(X,Y)						\
	{							\
	   GLuint u;						\
	   pbx[count] = X;					\
	   pby[count] = Y;					\
	   pbz[count] = Z;					\
 	   pbfog[count] = fog0;					\
	   pbrgba[count][RCOMP] = color[0];			\
	   pbrgba[count][GCOMP] = color[1];			\
	   pbrgba[count][BCOMP] = color[2];			\
	   pbrgba[count][ACOMP] = color[3];			\
	   pbspec[count][RCOMP] = sRed;				\
	   pbspec[count][GCOMP] = sGreen;			\
	   pbspec[count][BCOMP] = sBlue;			\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u]._ReallyEnabled) {	\
	         PB->s[u][count] = fragTexcoord[u][0];		\
	         PB->t[u][count] = fragTexcoord[u][1];		\
	         PB->u[u][count] = fragTexcoord[u][2];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "s_linetemp.h"
   }

   PB->count = count;
   _mesa_flush_pb(ctx);
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
   else if (swrast->Line == flat_ci_z_line)
      printf("flat_ci_z_line\n");
   else if (swrast->Line == flat_rgba_line)
      printf("flat_rgba_line\n");
   else if (swrast->Line == flat_rgba_z_line)
      printf("flat_rgba_z_line\n");
   else if (swrast->Line == smooth_ci_line)
      printf("smooth_ci_line\n");
   else if (swrast->Line == smooth_ci_z_line)
      printf("smooth_ci_z_line\n");
   else if (swrast->Line == smooth_rgba_line)
      printf("smooth_rgba_line\n");
   else if (swrast->Line == smooth_rgba_z_line)
      printf("smooth_rgba_z_line\n");
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
      printf("Driver func %p\n", swrast->Line);
}
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

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->Line.SmoothFlag) {
         /* antialiased lines */
         _swrast_choose_aa_line_function(ctx);
         ASSERT(swrast->Triangle);
      }
      else if (ctx->Texture._ReallyEnabled) {
         if (ctx->Texture._ReallyEnabled > TEXTURE0_ANY ||	     
	     (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)) {
            /* multi-texture and/or separate specular color */
            if (ctx->Light.ShadeModel==GL_SMOOTH)
               swrast->Line = smooth_multitextured_line;
            else
               swrast->Line = flat_multitextured_line;
         }
         else {
            if (ctx->Light.ShadeModel==GL_SMOOTH) {
                swrast->Line = smooth_textured_line;
            }
            else {
                swrast->Line = flat_textured_line;
            }
         }
      }
      else if (ctx->Line.Width!=1.0 || ctx->Line.StippleFlag) {
         if (ctx->Light.ShadeModel==GL_SMOOTH) {
            if (rgbmode)
               swrast->Line = general_smooth_rgba_line;
            else
               swrast->Line = general_smooth_ci_line;
         }
         else {
            if (rgbmode)
               swrast->Line = general_flat_rgba_line;
            else
               swrast->Line = general_flat_ci_line;
         }
      }
      else {
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* Width==1, non-stippled, smooth-shaded */
            if (ctx->Depth.Test || ctx->Fog.Enabled) {
               if (rgbmode)
                  swrast->Line = smooth_rgba_z_line;
               else
                  swrast->Line = smooth_ci_z_line;
            }
            else {
               if (rgbmode)
                  swrast->Line = smooth_rgba_line;
               else
                  swrast->Line = smooth_ci_line;
            }
	 }
         else {
	    /* Width==1, non-stippled, flat-shaded */
            if (ctx->Depth.Test || ctx->Fog.Enabled) {
               if (rgbmode)
                  swrast->Line = flat_rgba_z_line;
               else
                  swrast->Line = flat_ci_z_line;
            }
            else {
               if (rgbmode)
                  swrast->Line = flat_rgba_line;
               else
                  swrast->Line = flat_ci_line;
            }
         }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      swrast->Line = _mesa_feedback_line;
   }
   else {
      /* GL_SELECT mode */
      swrast->Line = _mesa_select_line;
   }

   /*_mesa_print_line_function(ctx);*/
}
