/* $Id: feedback.c,v 1.5 1999/11/08 07:36:44 brianp Exp $ */

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
#ifndef XFree86Server
#include <assert.h>
#include <stdio.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "enums.h"
#include "feedback.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#include "triangle.h"
#endif



#define FB_3D		0x01
#define FB_4D		0x02
#define FB_INDEX	0x04
#define FB_COLOR	0x08
#define FB_TEXTURE	0X10



void
gl_FeedbackBuffer( GLcontext *ctx, GLsizei size, GLenum type, GLfloat *buffer )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH( ctx, "glFeedbackBuffer" );

   if (ctx->RenderMode==GL_FEEDBACK) {
      gl_error( ctx, GL_INVALID_OPERATION, "glFeedbackBuffer" );
      return;
   }

   if (size<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(size<0)" );
      return;
   }
   if (!buffer) {
      gl_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(buffer==NULL)" );
      ctx->Feedback.BufferSize = 0;
      return;
   }

   switch (type) {
      case GL_2D:
	 ctx->Feedback.Mask = 0;
         ctx->Feedback.Type = type;
	 break;
      case GL_3D:
	 ctx->Feedback.Mask = FB_3D;
         ctx->Feedback.Type = type;
	 break;
      case GL_3D_COLOR:
	 ctx->Feedback.Mask = FB_3D
                           | (ctx->Visual->RGBAflag ? FB_COLOR : FB_INDEX);
         ctx->Feedback.Type = type;
	 break;
      case GL_3D_COLOR_TEXTURE:
	 ctx->Feedback.Mask = FB_3D
                           | (ctx->Visual->RGBAflag ? FB_COLOR : FB_INDEX)
	                   | FB_TEXTURE;
         ctx->Feedback.Type = type;
	 break;
      case GL_4D_COLOR_TEXTURE:
	 ctx->Feedback.Mask = FB_3D | FB_4D
                           | (ctx->Visual->RGBAflag ? FB_COLOR : FB_INDEX)
	                   | FB_TEXTURE;
         ctx->Feedback.Type = type;
	 break;
      default:
	 ctx->Feedback.Mask = 0;
         gl_error( ctx, GL_INVALID_ENUM, "glFeedbackBuffer" );
   }

   ctx->Feedback.BufferSize = size;
   ctx->Feedback.Buffer = buffer;
   ctx->Feedback.Count = 0;
}



void gl_PassThrough( GLcontext *ctx, GLfloat token )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPassThrough");

   if (ctx->RenderMode==GL_FEEDBACK) {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_PASS_THROUGH_TOKEN );
      FEEDBACK_TOKEN( ctx, token );
   }
}



/*
 * Put a vertex into the feedback buffer.
 */
void gl_feedback_vertex( GLcontext *ctx,
                         const GLfloat win[4],
			 const GLfloat color[4], 
			 GLuint index,
			 const GLfloat texcoord[4] )
{
   FEEDBACK_TOKEN( ctx, win[0] );
   FEEDBACK_TOKEN( ctx, win[1] );
   if (ctx->Feedback.Mask & FB_3D) {
      FEEDBACK_TOKEN( ctx, win[2] );
   }
   if (ctx->Feedback.Mask & FB_4D) {
      FEEDBACK_TOKEN( ctx, win[3] );
   }
   if (ctx->Feedback.Mask & FB_INDEX) {
      FEEDBACK_TOKEN( ctx, (GLfloat) index );
   }
   if (ctx->Feedback.Mask & FB_COLOR) {
      FEEDBACK_TOKEN( ctx, color[0] );
      FEEDBACK_TOKEN( ctx, color[1] );
      FEEDBACK_TOKEN( ctx, color[2] );
      FEEDBACK_TOKEN( ctx, color[3] );
   }
   if (ctx->Feedback.Mask & FB_TEXTURE) {
      FEEDBACK_TOKEN( ctx, texcoord[0] );
      FEEDBACK_TOKEN( ctx, texcoord[1] );
      FEEDBACK_TOKEN( ctx, texcoord[2] );
      FEEDBACK_TOKEN( ctx, texcoord[3] );
   }
}



static void gl_do_feedback_vertex( GLcontext *ctx, GLuint v, GLuint pv )
{
   GLfloat win[4];
   GLfloat color[4];
   GLfloat tc[4];
   GLuint texUnit = ctx->Texture.CurrentTransformUnit;
   struct vertex_buffer *VB = ctx->VB;

   win[0] = VB->Win.data[v][0];
   win[1] = VB->Win.data[v][1];
   win[2] = VB->Win.data[v][2] / DEPTH_SCALE;
   win[3] = 1.0 / VB->Win.data[v][3];

   if (ctx->Light.ShadeModel==GL_SMOOTH) pv = v;

   UBYTE_RGBA_TO_FLOAT_RGBA( color, VB->ColorPtr->data[pv] );

   if (VB->TexCoordPtr[texUnit]->size == 4 &&     
       VB->TexCoordPtr[texUnit]->data[v][3]!=0.0)
   {
      GLfloat invq = 1.0F / VB->TexCoordPtr[texUnit]->data[v][3];
      tc[0] = VB->TexCoordPtr[texUnit]->data[v][0] * invq;
      tc[1] = VB->TexCoordPtr[texUnit]->data[v][1] * invq;
      tc[2] = VB->TexCoordPtr[texUnit]->data[v][2] * invq;
      tc[3] = VB->TexCoordPtr[texUnit]->data[v][3];
   } else {
      ASSIGN_4V(tc, 0,0,0,1);
      COPY_SZ_4V(tc, 
		 VB->TexCoordPtr[texUnit]->size,
		 VB->TexCoordPtr[texUnit]->data[v]);
   }

   gl_feedback_vertex( ctx, win, color, VB->IndexPtr->data[v], tc );
}



/*
 * Put triangle in feedback buffer.
 */
void gl_feedback_triangle( GLcontext *ctx,
			   GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   if (gl_cull_triangle( ctx, v0, v1, v2, 0 )) {
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POLYGON_TOKEN );
      FEEDBACK_TOKEN( ctx, (GLfloat) 3 );        /* three vertices */
      
      gl_do_feedback_vertex( ctx, v0, pv );
      gl_do_feedback_vertex( ctx, v1, pv );
      gl_do_feedback_vertex( ctx, v2, pv );
   }
}


void gl_feedback_line( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv )
{
   GLenum token = GL_LINE_TOKEN;

   if (ctx->StippleCounter==0) 
      token = GL_LINE_RESET_TOKEN;

   FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) token );

   gl_do_feedback_vertex( ctx, v1, pv );
   gl_do_feedback_vertex( ctx, v2, pv );

   ctx->StippleCounter++;
}


void gl_feedback_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint i;

   for (i=first;i<=last;i++) 
      if (VB->ClipMask[i]==0) {
         FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_POINT_TOKEN );
	 gl_do_feedback_vertex( ctx, i, i );
      }
}





/**********************************************************************/
/*                              Selection                             */
/**********************************************************************/


/*
 * NOTE: this function can't be put in a display list.
 */
void gl_SelectBuffer( GLcontext *ctx, GLsizei size, GLuint *buffer )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glSelectBuffer");
   if (ctx->RenderMode==GL_SELECT) {
      gl_error( ctx, GL_INVALID_OPERATION, "glSelectBuffer" );
   }
   ctx->Select.Buffer = buffer;
   ctx->Select.BufferSize = size;
   ctx->Select.BufferCount = 0;

   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = 0.0;
}


#define WRITE_RECORD( CTX, V )					\
	if (CTX->Select.BufferCount < CTX->Select.BufferSize) {	\
	   CTX->Select.Buffer[CTX->Select.BufferCount] = (V);	\
	}							\
	CTX->Select.BufferCount++;



void gl_update_hitflag( GLcontext *ctx, GLfloat z )
{
   ctx->Select.HitFlag = GL_TRUE;
   if (z < ctx->Select.HitMinZ) {
      ctx->Select.HitMinZ = z;
   }
   if (z > ctx->Select.HitMaxZ) {
      ctx->Select.HitMaxZ = z;
   }
}

void gl_select_triangle( GLcontext *ctx,
			 GLuint v0, GLuint v1, GLuint v2, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;

   if (gl_cull_triangle( ctx, v0, v1, v2, 0 )) {
      gl_update_hitflag( ctx, VB->Win.data[v0][3] / DEPTH_SCALE );
      gl_update_hitflag( ctx, VB->Win.data[v1][3] / DEPTH_SCALE );
      gl_update_hitflag( ctx, VB->Win.data[v2][3] / DEPTH_SCALE );
   }
}


void gl_select_line( GLcontext *ctx,
		     GLuint v0, GLuint v1, GLuint pv )
{
   struct vertex_buffer *VB = ctx->VB;

   gl_update_hitflag( ctx, VB->Win.data[v0][3] / DEPTH_SCALE );
   gl_update_hitflag( ctx, VB->Win.data[v1][3] / DEPTH_SCALE );
}

void gl_select_points( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint i;

   for (i=first;i<=last;i++) 
      if (VB->ClipMask[i]==0) 
         gl_update_hitflag( ctx, VB->Win.data[i][3] / DEPTH_SCALE);
}


static void write_hit_record( GLcontext *ctx )
{
   GLuint i;
   GLuint zmin, zmax, zscale = (~0u);

   /* HitMinZ and HitMaxZ are in [0,1].  Multiply these values by */
   /* 2^32-1 and round to nearest unsigned integer. */

   assert( ctx != NULL ); /* this line magically fixes a SunOS 5.x/gcc bug */
   zmin = (GLuint) ((GLfloat) zscale * ctx->Select.HitMinZ);
   zmax = (GLuint) ((GLfloat) zscale * ctx->Select.HitMaxZ);

   WRITE_RECORD( ctx, ctx->Select.NameStackDepth );
   WRITE_RECORD( ctx, zmin );
   WRITE_RECORD( ctx, zmax );
   for (i=0;i<ctx->Select.NameStackDepth;i++) {
      WRITE_RECORD( ctx, ctx->Select.NameStack[i] );
   }

   ctx->Select.Hits++;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = -1.0;
}



void gl_InitNames( GLcontext *ctx )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glInitNames");
   /* Record the hit before the HitFlag is wiped out again. */
   if (ctx->RenderMode==GL_SELECT) {
      if (ctx->Select.HitFlag) {
         write_hit_record( ctx );
      }
   }
   ctx->Select.NameStackDepth = 0;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = 0.0;
}



void gl_LoadName( GLcontext *ctx, GLuint name )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glLoadName");
   if (ctx->RenderMode!=GL_SELECT) {
      return;
   }
   if (ctx->Select.NameStackDepth==0) {
      gl_error( ctx, GL_INVALID_OPERATION, "glLoadName" );
      return;
   }
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth<MAX_NAME_STACK_DEPTH) {
      ctx->Select.NameStack[ctx->Select.NameStackDepth-1] = name;
   }
   else {
      ctx->Select.NameStack[MAX_NAME_STACK_DEPTH-1] = name;
   }
}


void gl_PushName( GLcontext *ctx, GLuint name )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPushName");
   if (ctx->RenderMode!=GL_SELECT) {
      return;
   }
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth<MAX_NAME_STACK_DEPTH) {
      ctx->Select.NameStack[ctx->Select.NameStackDepth++] = name;
   }
   else {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushName" );
   }
}



void gl_PopName( GLcontext *ctx )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPopName");
   if (ctx->RenderMode!=GL_SELECT) {
      return;
   }
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth>0) {
      ctx->Select.NameStackDepth--;
   }
   else {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopName" );
   }
}



/**********************************************************************/
/*                           Render Mode                              */
/**********************************************************************/



/*
 * NOTE: this function can't be put in a display list.
 */
GLint gl_RenderMode( GLcontext *ctx, GLenum mode )
{
   GLint result;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH_WITH_RETVAL(ctx, "glRenderMode", 0);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glRenderMode %s\n", gl_lookup_enum_by_nr(mode));

   ctx->TriangleCaps &= ~(DD_FEEDBACK|DD_SELECT);

   switch (ctx->RenderMode) {
      case GL_RENDER:
	 result = 0;
	 break;
      case GL_SELECT:
	 if (ctx->Select.HitFlag) {
	    write_hit_record( ctx );
	 }
	 if (ctx->Select.BufferCount > ctx->Select.BufferSize) {
	    /* overflow */
#ifdef DEBUG
            gl_warning(ctx, "Feedback buffer overflow");
#endif
	    result = -1;
	 }
	 else {
	    result = ctx->Select.Hits;
	 }
	 ctx->Select.BufferCount = 0;
	 ctx->Select.Hits = 0;
	 ctx->Select.NameStackDepth = 0;
	 break;
      case GL_FEEDBACK:
	 if (ctx->Feedback.Count > ctx->Feedback.BufferSize) {
	    /* overflow */
	    result = -1;
	 }
	 else {
	    result = ctx->Feedback.Count;
	 }
	 ctx->Feedback.Count = 0;
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
	 return 0;
   }

   switch (mode) {
      case GL_RENDER:
         break;
      case GL_SELECT:
	 ctx->TriangleCaps |= DD_SELECT;
	 if (ctx->Select.BufferSize==0) {
	    /* haven't called glSelectBuffer yet */
	    gl_error( ctx, GL_INVALID_OPERATION, "glRenderMode" );
	 }
	 break;
      case GL_FEEDBACK:
	 ctx->TriangleCaps |= DD_FEEDBACK;
	 if (ctx->Feedback.BufferSize==0) {
	    /* haven't called glFeedbackBuffer yet */
	    gl_error( ctx, GL_INVALID_OPERATION, "glRenderMode" );
	 }
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
	 return 0;
   }


   ctx->RenderMode = mode;
   ctx->NewState |= NEW_ALL;

   return result;
}

