/* $Id: lines.c,v 1.16 2000/10/20 19:54:49 brianp Exp $ */

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
#include "context.h"
#include "depth.h"
#include "feedback.h"
#include "lines.h"
#include "macros.h"
#include "mmath.h"
#include "pb.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#endif



void
_mesa_LineWidth( GLfloat width )
{
   GET_CURRENT_CONTEXT(ctx);
   if (width<=0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glLineWidth" );
      return;
   }
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLineWidth");
   
   if (ctx->Line.Width != width) {
      ctx->Line.Width = width;
      ctx->TriangleCaps &= ~DD_LINE_WIDTH;
      if (width != 1.0) ctx->TriangleCaps |= DD_LINE_WIDTH;
      ctx->NewState |= NEW_RASTER_OPS;
      if (ctx->Driver.LineWidth)
         (*ctx->Driver.LineWidth)(ctx, width);
   }
}



void
_mesa_LineStipple( GLint factor, GLushort pattern )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLineStipple");
   ctx->Line.StippleFactor = CLAMP( factor, 1, 256 );
   ctx->Line.StipplePattern = pattern;
   ctx->NewState |= NEW_RASTER_OPS;

   if (ctx->Driver.LineStipple)
      ctx->Driver.LineStipple( ctx, factor, pattern );
}



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


/*
 * All line drawing functions have the same arguments:
 * v1, v2 - indexes of first and second endpoints into vertex buffer arrays
 * pv     - provoking vertex: which vertex color/index to use for flat shading.
 */






#if MAX_WIDTH > MAX_HEIGHT
#  define MAXPOINTS MAX_WIDTH
#else
#  define MAXPOINTS MAX_HEIGHT
#endif


/* Flat, color index line */
static void flat_ci_line( GLcontext *ctx,
                          GLuint vert0, GLuint vert1, GLuint pvert )
{
   PB_SET_INDEX( ctx->PB, ctx->VB->IndexPtr->data[pvert] );

#define INTERP_XY 1
#define PLOT(X,Y)  PB_WRITE_PIXEL(ctx->PB, X, Y, 0);

#include "linetemp.h"

   gl_flush_pb(ctx);
}



/* Flat, color index line with Z interpolation/testing */
static void flat_ci_z_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   PB_SET_INDEX( ctx->PB, ctx->VB->IndexPtr->data[pvert] );

#define INTERP_XY 1
#define INTERP_Z 1
#define PLOT(X,Y)  PB_WRITE_PIXEL(ctx->PB, X, Y, Z);

#include "linetemp.h"

   gl_flush_pb(ctx);
}



/* Flat-shaded, RGBA line */
static void flat_rgba_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   const GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   PB_SET_COLOR( ctx->PB, color[0], color[1], color[2], color[3] );

#define INTERP_XY 1
#define PLOT(X,Y)   PB_WRITE_PIXEL(ctx->PB, X, Y, 0);

#include "linetemp.h"

   gl_flush_pb(ctx);
}



/* Flat-shaded, RGBA line with Z interpolation/testing */
static void flat_rgba_z_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
   const GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   PB_SET_COLOR( ctx->PB, color[0], color[1], color[2], color[3] );

#define INTERP_XY 1
#define INTERP_Z 1
#define PLOT(X,Y)   PB_WRITE_PIXEL(ctx->PB, X, Y, Z);

#include "linetemp.h"

   gl_flush_pb(ctx);
}



/* Smooth shaded, color index line */
static void smooth_ci_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLuint *pbi = ctx->PB->index;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbi[count] = I;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}



/* Smooth shaded, color index line with Z interpolation/testing */
static void smooth_ci_z_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLuint *pbi = ctx->PB->index;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}



/* Smooth-shaded, RGBA line */
static void smooth_rgba_line( GLcontext *ctx,
                       	      GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

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

#include "linetemp.h"

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}



/* Smooth-shaded, RGBA line with Z interpolation/testing */
static void smooth_rgba_z_line( GLcontext *ctx,
                       	        GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1

#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}


#define CHECK_FULL(count)			\
	if (count >= PB_SIZE-MAX_WIDTH) {	\
	   ctx->PB->count = count;		\
	   gl_flush_pb(ctx);			\
	   count = ctx->PB->count;		\
	}



/* Smooth shaded, color index, any width, maybe stippled */
static void general_smooth_ci_line( GLcontext *ctx,
                           	    GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLuint *pbi = ctx->PB->index;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1
#define XMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X;	\
	pby[count] = Y;  pby[count+1] = Y+1;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	pbi[count] = I;  pbi[count+1] = I;	\
	count += 2;				\
	CHECK_FULL(count);
#define YMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X+1;	\
	pby[count] = Y;  pby[count+1] = Y;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	pbi[count] = I;  pbi[count+1] = I;	\
	count += 2;				\
	CHECK_FULL(count);
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_INDEX 1
#define WIDE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	pbi[count] = I;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}


/* Flat shaded, color index, any width, maybe stippled */
static void general_flat_ci_line( GLcontext *ctx,
                                  GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   PB_SET_INDEX( ctx->PB, ctx->VB->IndexPtr->data[pvert] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define XMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X;	\
	pby[count] = Y;  pby[count+1] = Y+1;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	count += 2;				\
	CHECK_FULL(count);
#define YMAJOR_PLOT(X,Y)			\
	pbx[count] = X;  pbx[count+1] = X+1;	\
	pby[count] = Y;  pby[count+1] = Y;	\
	pbz[count] = Z;  pbz[count+1] = Z;	\
	count += 2;				\
	CHECK_FULL(count);
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	count++;		\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}



static void general_smooth_rgba_line( GLcontext *ctx,
                                      GLuint vert0, GLuint vert1, GLuint pvert)
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)				\
	pbx[count] = X;				\
	pby[count] = Y;				\
	pbz[count] = Z;				\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;				\
	CHECK_FULL(count);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define XMAJOR_PLOT(X,Y)				\
	pbx[count] = X;  pbx[count+1] = X;		\
	pby[count] = Y;  pby[count+1] = Y+1;		\
	pbz[count] = Z;  pbz[count+1] = Z;		\
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
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define WIDE 1
#define PLOT(X,Y)				\
	pbx[count] = X;				\
	pby[count] = Y;				\
	pbz[count] = Z;				\
	pbrgba[count][RCOMP] = FixedToInt(r0);	\
	pbrgba[count][GCOMP] = FixedToInt(g0);	\
	pbrgba[count][BCOMP] = FixedToInt(b0);	\
	pbrgba[count][ACOMP] = FixedToInt(a0);	\
	count++;				\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}


static void general_flat_rgba_line( GLcontext *ctx,
                                    GLuint vert0, GLuint vert1, GLuint pvert )
{
   const GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   PB_SET_COLOR( ctx->PB, color[0], color[1], color[2], color[3] );

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)  PB_WRITE_PIXEL(ctx->PB, X, Y, Z);
#include "linetemp.h"
   }
   else {
      /* unstippled */
      if (ctx->Line.Width==2.0F) {
         /* special case: unstippled and width=2 */
#define INTERP_XY 1
#define INTERP_Z 1
#define XMAJOR_PLOT(X,Y) PB_WRITE_PIXEL(ctx->PB, X, Y, Z); \
                         PB_WRITE_PIXEL(ctx->PB, X, Y+1, Z);
#define YMAJOR_PLOT(X,Y)  PB_WRITE_PIXEL(ctx->PB, X, Y, Z); \
                          PB_WRITE_PIXEL(ctx->PB, X+1, Y, Z);
#include "linetemp.h"
      }
      else {
         /* unstippled, any width */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define PLOT(X,Y) PB_WRITE_PIXEL(ctx->PB, X, Y, Z);
#include "linetemp.h"
      }
   }

   gl_flush_pb(ctx);
}


/* Flat-shaded, textured, any width, maybe stippled */
static void flat_textured_line( GLcontext *ctx,
                                GLuint vert0, GLuint vert1, GLuint pv )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLfloat *pbs = ctx->PB->s[0];
   GLfloat *pbt = ctx->PB->t[0];
   GLfloat *pbu = ctx->PB->u[0];
   GLubyte *color = ctx->VB->ColorPtr->data[pv];
   PB_SET_COLOR( ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_TEX 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	{				\
	   pbx[count] = X;		\
	   pby[count] = Y;		\
	   pbz[count] = Z;		\
	   pbs[count] = fragTexcoord[0];\
	   pbt[count] = fragTexcoord[1];\
	   pbu[count] = fragTexcoord[2];\
	   count++;			\
	   CHECK_FULL(count);		\
	}
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_TEX 1
#define WIDE 1
#define PLOT(X,Y)			\
	{				\
	   pbx[count] = X;		\
	   pby[count] = Y;		\
	   pbz[count] = Z;		\
	   pbs[count] = fragTexcoord[0];\
	   pbt[count] = fragTexcoord[1];\
	   pbu[count] = fragTexcoord[2];\
	   count++;			\
	   CHECK_FULL(count);		\
	}
#include "linetemp.h"
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}



/* Smooth-shaded, textured, any width, maybe stippled */
static void smooth_textured_line( GLcontext *ctx,
                                  GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLfloat *pbs = ctx->PB->s[0];
   GLfloat *pbt = ctx->PB->t[0];
   GLfloat *pbu = ctx->PB->u[0];
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
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
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_TEX 1
#define WIDE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
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
#include "linetemp.h"
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}


/* Smooth-shaded, multitextured, any width, maybe stippled, separate specular
 * color interpolation.
 */
static void smooth_multitextured_line( GLcontext *ctx,
                                   GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   GLubyte (*pbspec)[3] = ctx->PB->spec;
   (void) pvert;

   ctx->PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
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
	   pbrgba[count][RCOMP] = FixedToInt(r0);		\
	   pbrgba[count][GCOMP] = FixedToInt(g0);		\
	   pbrgba[count][BCOMP] = FixedToInt(b0);		\
	   pbrgba[count][ACOMP] = FixedToInt(a0);		\
	   pbspec[count][RCOMP] = FixedToInt(sr0);		\
	   pbspec[count][GCOMP] = FixedToInt(sg0);		\
	   pbspec[count][BCOMP] = FixedToInt(sb0);		\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u].ReallyEnabled) {		\
	         ctx->PB->s[u][0] = fragTexcoord[u][0];		\
	         ctx->PB->s[u][1] = fragTexcoord[u][1];		\
	         ctx->PB->s[u][2] = fragTexcoord[u][2];		\
	         ctx->PB->s[u][3] = fragTexcoord[u][3];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
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
	   pbrgba[count][RCOMP] = FixedToInt(r0);		\
	   pbrgba[count][GCOMP] = FixedToInt(g0);		\
	   pbrgba[count][BCOMP] = FixedToInt(b0);		\
	   pbrgba[count][ACOMP] = FixedToInt(a0);		\
	   pbspec[count][RCOMP] = FixedToInt(sr0);		\
	   pbspec[count][GCOMP] = FixedToInt(sg0);		\
	   pbspec[count][BCOMP] = FixedToInt(sb0);		\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u].ReallyEnabled) {		\
	         ctx->PB->s[u][0] = fragTexcoord[u][0];		\
	         ctx->PB->s[u][1] = fragTexcoord[u][1];		\
	         ctx->PB->s[u][2] = fragTexcoord[u][2];		\
	         ctx->PB->s[u][3] = fragTexcoord[u][3];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "linetemp.h"
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}


/* Flat-shaded, multitextured, any width, maybe stippled, separate specular
 * color interpolation.
 */
static void flat_multitextured_line( GLcontext *ctx,
                                     GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   GLubyte (*pbspec)[3] = ctx->PB->spec;
   GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   GLubyte sRed   = ctx->VB->Specular ? ctx->VB->Specular[pvert][0] : 0;
   GLubyte sGreen = ctx->VB->Specular ? ctx->VB->Specular[pvert][1] : 0;
   GLubyte sBlue  = ctx->VB->Specular ? ctx->VB->Specular[pvert][2] : 0;

   (void) pvert;

   ctx->PB->mono = GL_FALSE;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
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
	   pbrgba[count][RCOMP] = color[0];			\
	   pbrgba[count][GCOMP] = color[1];			\
	   pbrgba[count][BCOMP] = color[2];			\
	   pbrgba[count][ACOMP] = color[3];			\
	   pbspec[count][RCOMP] = sRed;				\
	   pbspec[count][GCOMP] = sGreen;			\
	   pbspec[count][BCOMP] = sBlue;			\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u].ReallyEnabled) {		\
	         ctx->PB->s[u][0] = fragTexcoord[u][0];		\
	         ctx->PB->s[u][1] = fragTexcoord[u][1];		\
	         ctx->PB->s[u][2] = fragTexcoord[u][2];		\
	         ctx->PB->s[u][3] = fragTexcoord[u][3];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_ALPHA 1
#define INTERP_MULTITEX 1
#define WIDE 1
#define PLOT(X,Y)						\
	{							\
	   GLuint u;						\
	   pbx[count] = X;					\
	   pby[count] = Y;					\
	   pbz[count] = Z;					\
	   pbrgba[count][RCOMP] = color[0];			\
	   pbrgba[count][GCOMP] = color[1];			\
	   pbrgba[count][BCOMP] = color[2];			\
	   pbrgba[count][ACOMP] = color[3];			\
	   pbspec[count][RCOMP] = sRed;				\
	   pbspec[count][GCOMP] = sGreen;			\
	   pbspec[count][BCOMP] = sBlue;			\
	   for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {	\
	      if (ctx->Texture.Unit[u].ReallyEnabled) {		\
	         ctx->PB->s[u][0] = fragTexcoord[u][0];		\
	         ctx->PB->s[u][1] = fragTexcoord[u][1];		\
	         ctx->PB->s[u][2] = fragTexcoord[u][2];		\
	         ctx->PB->s[u][3] = fragTexcoord[u][3];		\
	      }							\
	   }							\
	   count++;						\
	   CHECK_FULL(count);					\
	}
#include "linetemp.h"
   }

   ctx->PB->count = count;
   gl_flush_pb(ctx);
}




/*
 * Antialiased RGBA line
 *
 * This AA line function isn't terribly efficient but it's pretty
 * straight-forward to understand.  Also, it doesn't exactly conform
 * to the specification.
 */
static void aa_rgba_line( GLcontext *ctx,
                          GLuint vert0, GLuint vert1, GLuint pvert )
{
#define INTERP_RGBA 1
#define PLOT(x, y)						\
   {								\
      PB_WRITE_RGBA_PIXEL( pb, (x), (y), z,			\
			   red, green, blue, coverage );	\
   }
#include "lnaatemp.h"
}

/*
 * Antialiased Textured RGBA line
 *
 * This AA line function isn't terribly efficient but it's pretty
 * straight-forward to understand.  Also, it doesn't exactly conform
 * to the specification.
 */
static void aa_tex_rgba_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
#define INTERP_RGBA 1
#define INTERP_TEX 1
#define PLOT(x, y)							\
   {									\
      PB_WRITE_TEX_PIXEL( pb, (x), (y), z, red, green, blue, coverage,	\
                 fragTexcoord[0], fragTexcoord[1], fragTexcoord[2] );	\
   }
#include "lnaatemp.h"
}


/*
 * Antialiased Multitextured RGBA line
 *
 * This AA line function isn't terribly efficient but it's pretty
 * straight-forward to understand.  Also, it doesn't exactly conform
 * to the specification.
 */
static void aa_multitex_rgba_line( GLcontext *ctx,
                                   GLuint vert0, GLuint vert1, GLuint pvert )
{
#define INTERP_RGBA 1
#define INTERP_SPEC 1
#define INTERP_MULTITEX 1
#define PLOT(x, y)							\
   {									\
      PB_WRITE_MULTITEX_SPEC_PIXEL( pb, (x), (y), z,			\
            red, green, blue, coverage, specRed, specGreen, specBlue,	\
            fragTexcoord );						\
   }
#include "lnaatemp.h"
}


/*
 * Antialiased CI line.  Same comments for RGBA antialiased lines apply.
 */
static void aa_ci_line( GLcontext *ctx,
                        GLuint vert0, GLuint vert1, GLuint pvert )
{
#define INTERP_INDEX 1
#define PLOT(x, y)						\
   {								\
      PB_WRITE_CI_PIXEL( pb, (x), (y), z, index + coverage );	\
   }
#include "lnaatemp.h"
}


/*
 * Null rasterizer for measuring transformation speed.
 */
static void null_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
   (void) ctx;
   (void) v1;
   (void) v2;
   (void) pv;
}



#ifdef DEBUG
void
_mesa_print_line_function(GLcontext *ctx)
{
   printf("Line Func == ");
   if (ctx->Driver.LineFunc == flat_ci_line)
      printf("flat_ci_line\n");
   else if (ctx->Driver.LineFunc == flat_ci_z_line)
      printf("flat_ci_z_line\n");
   else if (ctx->Driver.LineFunc == flat_rgba_line)
      printf("flat_rgba_line\n");
   else if (ctx->Driver.LineFunc == flat_rgba_z_line)
      printf("flat_rgba_z_line\n");
   else if (ctx->Driver.LineFunc == smooth_ci_line)
      printf("smooth_ci_line\n");
   else if (ctx->Driver.LineFunc == smooth_ci_z_line)
      printf("smooth_ci_z_line\n");
   else if (ctx->Driver.LineFunc == smooth_rgba_line)
      printf("smooth_rgba_line\n");
   else if (ctx->Driver.LineFunc == smooth_rgba_z_line)
      printf("smooth_rgba_z_line\n");
   else if (ctx->Driver.LineFunc == general_smooth_ci_line)
      printf("general_smooth_ci_line\n");
   else if (ctx->Driver.LineFunc == general_flat_ci_line)
      printf("general_flat_ci_line\n");
   else if (ctx->Driver.LineFunc == general_smooth_rgba_line)
      printf("general_smooth_rgba_line\n");
   else if (ctx->Driver.LineFunc == general_flat_rgba_line)
      printf("general_flat_rgba_line\n");
   else if (ctx->Driver.LineFunc == flat_textured_line)
      printf("flat_textured_line\n");
   else if (ctx->Driver.LineFunc == smooth_textured_line)
      printf("smooth_textured_line\n");
   else if (ctx->Driver.LineFunc == smooth_multitextured_line)
      printf("smooth_multitextured_line\n");
   else if (ctx->Driver.LineFunc == flat_multitextured_line)
      printf("flat_multitextured_line\n");
   else if (ctx->Driver.LineFunc == aa_rgba_line)
      printf("aa_rgba_line\n");
   else if (ctx->Driver.LineFunc == aa_tex_rgba_line)
      printf("aa_tex_rgba_line\n");
   else if (ctx->Driver.LineFunc == aa_multitex_rgba_line)
      printf("aa_multitex_rgba_line\n");
   else if (ctx->Driver.LineFunc == aa_ci_line)
      printf("aa_ci_line\n");
   else if (ctx->Driver.LineFunc == null_line)
      printf("null_line\n");
   else
      printf("Driver func %p\n", ctx->Driver.PointsFunc);
}
#endif



/*
 * Determine which line drawing function to use given the current
 * rendering context.
 */
void gl_set_line_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual.RGBAflag;
   /* TODO: antialiased lines */

   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.LineFunc = null_line;
         return;
      }
      if (ctx->Driver.LineFunc) {
         /* Device driver will draw lines. */
	 return;
      }

      if (ctx->Line.SmoothFlag) {
         /* antialiased lines */
         if (rgbmode) {
            if (ctx->Texture.ReallyEnabled) {
               if (ctx->Texture.MultiTextureEnabled
                  || ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR)
                  /* Multitextured! */
                  ctx->Driver.LineFunc = aa_multitex_rgba_line;
               else
                  ctx->Driver.LineFunc = aa_tex_rgba_line;
            } else {
               ctx->Driver.LineFunc = aa_rgba_line;
            }
         }
         else {
            ctx->Driver.LineFunc = aa_ci_line;
         }
      }
      else if (ctx->Texture.ReallyEnabled) {
         if (ctx->Texture.MultiTextureEnabled
             || ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) {
            /* multi-texture and/or separate specular color */
            if (ctx->Light.ShadeModel==GL_SMOOTH)
               ctx->Driver.LineFunc = smooth_multitextured_line;
            else
               ctx->Driver.LineFunc = flat_multitextured_line;
         }
         else {
            if (ctx->Light.ShadeModel==GL_SMOOTH) {
                ctx->Driver.LineFunc = smooth_textured_line;
            }
            else {
                ctx->Driver.LineFunc = flat_textured_line;
            }
         }
      }
      else if (ctx->Line.Width!=1.0 || ctx->Line.StippleFlag
               || ctx->Line.SmoothFlag) {
         if (ctx->Light.ShadeModel==GL_SMOOTH) {
            if (rgbmode)
               ctx->Driver.LineFunc = general_smooth_rgba_line;
            else
               ctx->Driver.LineFunc = general_smooth_ci_line;
         }
         else {
            if (rgbmode)
               ctx->Driver.LineFunc = general_flat_rgba_line;
            else
               ctx->Driver.LineFunc = general_flat_ci_line;
         }
      }
      else {
	 if (ctx->Light.ShadeModel==GL_SMOOTH) {
	    /* Width==1, non-stippled, smooth-shaded */
            if (ctx->Depth.Test || ctx->FogMode == FOG_FRAGMENT) {
               if (rgbmode)
                  ctx->Driver.LineFunc = smooth_rgba_z_line;
               else
                  ctx->Driver.LineFunc = smooth_ci_z_line;
            }
            else {
               if (rgbmode)
                  ctx->Driver.LineFunc = smooth_rgba_line;
               else
                  ctx->Driver.LineFunc = smooth_ci_line;
            }
	 }
         else {
	    /* Width==1, non-stippled, flat-shaded */
            if (ctx->Depth.Test || ctx->FogMode == FOG_FRAGMENT) {
               if (rgbmode)
                  ctx->Driver.LineFunc = flat_rgba_z_line;
               else
                  ctx->Driver.LineFunc = flat_ci_z_line;
            }
            else {
               if (rgbmode)
                  ctx->Driver.LineFunc = flat_rgba_line;
               else
                  ctx->Driver.LineFunc = flat_ci_line;
            }
         }
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      ctx->Driver.LineFunc = gl_feedback_line;
   }
   else {
      /* GL_SELECT mode */
      ctx->Driver.LineFunc = gl_select_line;
   }

   /*_mesa_print_line_function(ctx);*/
}
