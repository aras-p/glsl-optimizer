/* $Id: lines.c,v 1.1 1999/08/19 00:55:41 jtg Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
#include <assert.h>
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
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_LineWidth( GLcontext *ctx, GLfloat width )
{
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
   }
}



void gl_LineStipple( GLcontext *ctx, GLint factor, GLushort pattern )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLineStipple");
   ctx->Line.StippleFactor = CLAMP( factor, 1, 256 );
   ctx->Line.StipplePattern = pattern;
   ctx->NewState |= NEW_RASTER_OPS;
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



static void feedback_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;
   GLfloat x1, y1, z1, w1;
   GLfloat x2, y2, z2, w2;
   GLfloat tex1[4], tex2[4], invq;
   GLuint texUnit = ctx->Texture.CurrentTransformUnit;

   x1 = VB->Win.data[v1][0];
   y1 = VB->Win.data[v1][1];
   z1 = VB->Win.data[v1][2] / DEPTH_SCALE;
   w1 = (VB->ClipPtr->size == 4 ? VEC_ELT(VB->ClipPtr, GLfloat, v1)[3] : 1.0);

   x2 = VB->Win.data[v2][0];
   y2 = VB->Win.data[v2][1];
   z2 = VB->Win.data[v2][2] / DEPTH_SCALE;
   w2 = (VB->ClipPtr->size == 4 ? VEC_ELT(VB->ClipPtr, GLfloat, v2)[3] : 1.0);


   if (VB->TexCoordPtr[texUnit]->size == 4) {      
      invq = (VB->TexCoordPtr[texUnit]->data[v1][3]==0.0
	      ? 1.0
	      : 1.0F / VB->TexCoordPtr[texUnit]->data[v1][3]);

      tex1[0] = VB->TexCoordPtr[texUnit]->data[v1][0] * invq;
      tex1[1] = VB->TexCoordPtr[texUnit]->data[v1][1] * invq;
      tex1[2] = VB->TexCoordPtr[texUnit]->data[v1][2] * invq;
      tex1[3] = VB->TexCoordPtr[texUnit]->data[v1][3];

      invq = (VB->TexCoordPtr[texUnit]->data[v2][3]==0.0
	      ? 1.0
	      : 1.0F / VB->TexCoordPtr[texUnit]->data[v2][3]);

      tex2[0] = VB->TexCoordPtr[texUnit]->data[v2][0] * invq;
      tex2[1] = VB->TexCoordPtr[texUnit]->data[v2][1] * invq;
      tex2[2] = VB->TexCoordPtr[texUnit]->data[v2][2] * invq;
      tex2[3] = VB->TexCoordPtr[texUnit]->data[v2][3];
   } else {
      ASSIGN_4V(tex1, 0,0,0,1);
      ASSIGN_4V(tex2, 0,0,0,1);
      COPY_SZ_4V(tex1, 
		 VB->TexCoordPtr[texUnit]->size,
		 VB->TexCoordPtr[texUnit]->data[v1]);
      COPY_SZ_4V(tex2, 
		 VB->TexCoordPtr[texUnit]->size,
		 VB->TexCoordPtr[texUnit]->data[v2]);
   }


   if (ctx->StippleCounter==0) {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_LINE_RESET_TOKEN );
   }
   else {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_LINE_TOKEN );
   }

   {
      GLfloat color[4];
      GLubyte *ubc = VB->ColorPtr->data[pv];
      GLuint index = VB->IndexPtr->data[pv];

      UBYTE_RGBA_TO_FLOAT_RGBA( color, ubc );
      gl_feedback_vertex( ctx, x1,y1,z1,w1, color, (GLfloat) index, tex1 );
      gl_feedback_vertex( ctx, x2,y2,z2,w2, color, (GLfloat) index, tex2 );
   }

   ctx->StippleCounter++;
}



static void select_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
   (void) pv;
   gl_update_hitflag( ctx, ctx->VB->Win.data[v1][2] / DEPTH_SCALE );
   gl_update_hitflag( ctx, ctx->VB->Win.data[v2][2] / DEPTH_SCALE );
}



#if MAX_WIDTH > MAX_HEIGHT
#  define MAXPOINTS MAX_WIDTH
#else
#  define MAXPOINTS MAX_HEIGHT
#endif


/* Flat, color index line */
static void flat_ci_line( GLcontext *ctx,
                          GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   PB_SET_INDEX( ctx, ctx->PB, ctx->VB->IndexPtr->data[pvert] );
   count = ctx->PB->count;

#define INTERP_XY 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat, color index line with Z interpolation/testing */
static void flat_ci_z_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   PB_SET_INDEX( ctx, ctx->PB, ctx->VB->IndexPtr->data[pvert] );
   count = ctx->PB->count;

#define INTERP_XY 1
#define INTERP_Z 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbz[count] = Z;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat-shaded, RGBA line */
static void flat_rgba_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

#define INTERP_XY 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Flat-shaded, RGBA line with Z interpolation/testing */
static void flat_rgba_z_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

#define INTERP_XY 1
#define INTERP_Z 1

#define PLOT(X,Y)	\
	pbx[count] = X;	\
	pby[count] = Y;	\
	pbz[count] = Z;	\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth shaded, color index line */
static void smooth_ci_line( GLcontext *ctx,
                            GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLuint *pbi = ctx->PB->i;
   (void) pvert;

#define INTERP_XY 1
#define INTERP_INDEX 1

#define PLOT(X,Y)		\
	pbx[count] = X;		\
	pby[count] = Y;		\
	pbi[count] = I;		\
	count++;

#include "linetemp.h"

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
}



/* Smooth shaded, color index line with Z interpolation/testing */
static void smooth_ci_z_line( GLcontext *ctx,
                              GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count = ctx->PB->count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLuint *pbi = ctx->PB->i;
   (void) pvert;

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
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
   GLuint *pbi = ctx->PB->i;
   (void) pvert;

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
   PB_CHECK_FLUSH( ctx, ctx->PB );
}


/* Flat shaded, color index, any width, maybe stippled */
static void general_flat_ci_line( GLcontext *ctx,
                                  GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   PB_SET_INDEX( ctx, ctx->PB, ctx->VB->IndexPtr->data[pvert] );
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
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
   PB_CHECK_FLUSH( ctx, ctx->PB );
}


static void general_flat_rgba_line( GLcontext *ctx,
                                    GLuint vert0, GLuint vert1, GLuint pvert )
{
   GLint count;
   GLint *pbx = ctx->PB->x;
   GLint *pby = ctx->PB->y;
   GLdepth *pbz = ctx->PB->z;
   GLubyte *color = ctx->VB->ColorPtr->data[pvert];
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	count++;			\
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
#define PLOT(X,Y)			\
	pbx[count] = X;			\
	pby[count] = Y;			\
	pbz[count] = Z;			\
	count++;			\
	CHECK_FULL(count);
#include "linetemp.h"
      }
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
   PB_SET_COLOR( ctx, ctx->PB, color[0], color[1], color[2], color[3] );
   count = ctx->PB->count;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_STUV0 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)			\
	{				\
	   pbx[count] = X;		\
	   pby[count] = Y;		\
	   pbz[count] = Z;		\
	   pbs[count] = s;		\
	   pbt[count] = t;		\
	   pbu[count] = u;		\
	   count++;			\
	   CHECK_FULL(count);		\
	}
#include "linetemp.h"
   }
   else {
      /* unstippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_STUV0 1
#define WIDE 1
#define PLOT(X,Y)			\
	{				\
	   pbx[count] = X;		\
	   pby[count] = Y;		\
	   pbz[count] = Z;		\
	   pbs[count] = s;		\
	   pbt[count] = t;		\
	   pbu[count] = u;		\
	   count++;			\
	   CHECK_FULL(count);		\
	}
#include "linetemp.h"
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
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

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_ALPHA 1
#define INTERP_STUV0 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
	   pbs[count] = s;				\
	   pbt[count] = t;				\
	   pbu[count] = u;				\
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
#define INTERP_STUV0 1
#define WIDE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
	   pbs[count] = s;				\
	   pbt[count] = t;				\
	   pbu[count] = u;				\
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
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
   GLfloat *pbs = ctx->PB->s[0];
   GLfloat *pbt = ctx->PB->t[0];
   GLfloat *pbu = ctx->PB->u[0];
   GLfloat *pbs1 = ctx->PB->s[1];
   GLfloat *pbt1 = ctx->PB->t[1];
   GLfloat *pbu1 = ctx->PB->u[1];
   GLubyte (*pbrgba)[4] = ctx->PB->rgba;
   GLubyte (*pbspec)[3] = ctx->PB->spec;
   (void) pvert;

   if (ctx->Line.StippleFlag) {
      /* stippled */
#define INTERP_XY 1
#define INTERP_Z 1
#define INTERP_RGB 1
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_STUV0 1
#define INTERP_STUV1 1
#define WIDE 1
#define STIPPLE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
	   pbs[count] = s;				\
	   pbt[count] = t;				\
	   pbu[count] = u;				\
	   pbs1[count] = s1;				\
	   pbt1[count] = t1;				\
	   pbu1[count] = u1;				\
	   pbrgba[count][RCOMP] = FixedToInt(r0);	\
	   pbrgba[count][GCOMP] = FixedToInt(g0);	\
	   pbrgba[count][BCOMP] = FixedToInt(b0);	\
	   pbrgba[count][ACOMP] = FixedToInt(a0);	\
	   pbspec[count][RCOMP] = FixedToInt(sr0);	\
	   pbspec[count][GCOMP] = FixedToInt(sg0);	\
	   pbspec[count][BCOMP] = FixedToInt(sb0);	\
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
#define INTERP_SPEC 1
#define INTERP_ALPHA 1
#define INTERP_STUV0 1
#define INTERP_STUV1 1
#define WIDE 1
#define PLOT(X,Y)					\
	{						\
	   pbx[count] = X;				\
	   pby[count] = Y;				\
	   pbz[count] = Z;				\
	   pbs[count] = s;				\
	   pbt[count] = t;				\
	   pbu[count] = u;				\
	   pbs1[count] = s1;				\
	   pbt1[count] = t1;				\
	   pbu1[count] = u1;				\
	   pbrgba[count][RCOMP] = FixedToInt(r0);	\
	   pbrgba[count][GCOMP] = FixedToInt(g0);	\
	   pbrgba[count][BCOMP] = FixedToInt(b0);	\
	   pbrgba[count][ACOMP] = FixedToInt(a0);	\
	   pbspec[count][RCOMP] = FixedToInt(sr0);	\
	   pbspec[count][GCOMP] = FixedToInt(sg0);	\
	   pbspec[count][BCOMP] = FixedToInt(sb0);	\
	   count++;					\
	   CHECK_FULL(count);				\
	}
#include "linetemp.h"
   }

   ctx->PB->count = count;
   PB_CHECK_FLUSH( ctx, ctx->PB );
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
#define PLOT(x, y)  { PB_WRITE_RGBA_PIXEL( pb, (x), (y), z, red, green, blue, coverage ); }
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
#define INTERP_STUV0 1
#define PLOT(x, y)							\
   {									\
      PB_WRITE_TEX_PIXEL( pb, (x), (y), z, red, green, blue, coverage,	\
                          s, t, u );					\
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
#define INTERP_STUV0 1
#define INTERP_STUV1 1
#define PLOT(x, y)							\
   {									\
      PB_WRITE_MULTITEX_SPEC_PIXEL( pb, (x), (y), z,			\
            red, green, blue, coverage, specRed, specGreen, specBlue,	\
            s, t, u, s1, t1, u1 );					\
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


/*
 * Determine which line drawing function to use given the current
 * rendering context.
 */
void gl_set_line_function( GLcontext *ctx )
{
   GLboolean rgbmode = ctx->Visual->RGBAflag;
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
               if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D
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
         if (ctx->Texture.ReallyEnabled >= TEXTURE1_1D
             || ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) {
            /* multi-texture and/or separate specular color */
            ctx->Driver.LineFunc = smooth_multitextured_line;
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
            if (ctx->Depth.Test
	        || (ctx->Fog.Enabled && ctx->Hint.Fog==GL_NICEST)) {
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
            if (ctx->Depth.Test
                || (ctx->Fog.Enabled && ctx->Hint.Fog==GL_NICEST)) {
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
      ctx->Driver.LineFunc = feedback_line;
   }
   else {
      /* GL_SELECT mode */
      ctx->Driver.LineFunc = select_line;
   }
}

