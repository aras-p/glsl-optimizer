/* $Id: feedback.c,v 1.24 2001/03/29 17:08:26 keithw Exp $ */

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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "enums.h"
#include "feedback.h"
#include "macros.h"
#include "mmath.h"
#include "mtypes.h"
#endif



#define FB_3D		0x01
#define FB_4D		0x02
#define FB_INDEX	0x04
#define FB_COLOR	0x08
#define FB_TEXTURE	0X10



void
_mesa_FeedbackBuffer( GLsizei size, GLenum type, GLfloat *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode==GL_FEEDBACK) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glFeedbackBuffer" );
      return;
   }
   if (size<0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(size<0)" );
      return;
   }
   if (!buffer) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glFeedbackBuffer(buffer==NULL)" );
      ctx->Feedback.BufferSize = 0;
      return;
   }

   switch (type) {
      case GL_2D:
	 ctx->Feedback._Mask = 0;
	 break;
      case GL_3D:
	 ctx->Feedback._Mask = FB_3D;
	 break;
      case GL_3D_COLOR:
	 ctx->Feedback._Mask = (FB_3D |
				(ctx->Visual.rgbMode ? FB_COLOR : FB_INDEX));
	 break;
      case GL_3D_COLOR_TEXTURE:
	 ctx->Feedback._Mask = (FB_3D |
				(ctx->Visual.rgbMode ? FB_COLOR : FB_INDEX) |
				FB_TEXTURE);
	 break;
      case GL_4D_COLOR_TEXTURE:
	 ctx->Feedback._Mask = (FB_3D | FB_4D |
				(ctx->Visual.rgbMode ? FB_COLOR : FB_INDEX) |
				FB_TEXTURE);
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glFeedbackBuffer" );
	 return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE); /* Always flush */
   ctx->Feedback.Type = type;
   ctx->Feedback.BufferSize = size;
   ctx->Feedback.Buffer = buffer;
   ctx->Feedback.Count = 0;	              /* Becaues of this. */
}


void
_mesa_PassThrough( GLfloat token )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode==GL_FEEDBACK) {
      FLUSH_VERTICES(ctx, 0);
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_PASS_THROUGH_TOKEN );
      FEEDBACK_TOKEN( ctx, token );
   }
}



/*
 * Put a vertex into the feedback buffer.
 */
void _mesa_feedback_vertex( GLcontext *ctx,
                         const GLfloat win[4],
			 const GLfloat color[4],
			 GLuint index,
			 const GLfloat texcoord[4] )
{
   FEEDBACK_TOKEN( ctx, win[0] );
   FEEDBACK_TOKEN( ctx, win[1] );
   if (ctx->Feedback._Mask & FB_3D) {
      FEEDBACK_TOKEN( ctx, win[2] );
   }
   if (ctx->Feedback._Mask & FB_4D) {
      FEEDBACK_TOKEN( ctx, win[3] );
   }
   if (ctx->Feedback._Mask & FB_INDEX) {
      FEEDBACK_TOKEN( ctx, (GLfloat) index );
   }
   if (ctx->Feedback._Mask & FB_COLOR) {
      FEEDBACK_TOKEN( ctx, color[0] );
      FEEDBACK_TOKEN( ctx, color[1] );
      FEEDBACK_TOKEN( ctx, color[2] );
      FEEDBACK_TOKEN( ctx, color[3] );
   }
   if (ctx->Feedback._Mask & FB_TEXTURE) {
      FEEDBACK_TOKEN( ctx, texcoord[0] );
      FEEDBACK_TOKEN( ctx, texcoord[1] );
      FEEDBACK_TOKEN( ctx, texcoord[2] );
      FEEDBACK_TOKEN( ctx, texcoord[3] );
   }
}


/**********************************************************************/
/*                              Selection                             */
/**********************************************************************/


/*
 * NOTE: this function can't be put in a display list.
 */
void
_mesa_SelectBuffer( GLsizei size, GLuint *buffer )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode==GL_SELECT) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glSelectBuffer" );
      return;			/* KW: added return */
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE); /* why bother? */
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



void _mesa_update_hitflag( GLcontext *ctx, GLfloat z )
{
   ctx->Select.HitFlag = GL_TRUE;
   if (z < ctx->Select.HitMinZ) {
      ctx->Select.HitMinZ = z;
   }
   if (z > ctx->Select.HitMaxZ) {
      ctx->Select.HitMaxZ = z;
   }
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
   for (i = 0; i < ctx->Select.NameStackDepth; i++) {
      WRITE_RECORD( ctx, ctx->Select.NameStack[i] );
   }

   ctx->Select.Hits++;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = -1.0;
}



void
_mesa_InitNames( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   /* Record the hit before the HitFlag is wiped out again. */
   if (ctx->RenderMode == GL_SELECT) {
      if (ctx->Select.HitFlag) {
         write_hit_record( ctx );
      }
   }
   ctx->Select.NameStackDepth = 0;
   ctx->Select.HitFlag = GL_FALSE;
   ctx->Select.HitMinZ = 1.0;
   ctx->Select.HitMaxZ = 0.0;
   ctx->NewState |= _NEW_RENDERMODE;
}



void
_mesa_LoadName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode != GL_SELECT) {
      return;
   }
   if (ctx->Select.NameStackDepth == 0) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glLoadName" );
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);

   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth < MAX_NAME_STACK_DEPTH) {
      ctx->Select.NameStack[ctx->Select.NameStackDepth-1] = name;
   }
   else {
      ctx->Select.NameStack[MAX_NAME_STACK_DEPTH-1] = name;
   }
}


void
_mesa_PushName( GLuint name )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode != GL_SELECT) {
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth >= MAX_NAME_STACK_DEPTH) {
      _mesa_error( ctx, GL_STACK_OVERFLOW, "glPushName" );
   }
   else
      ctx->Select.NameStack[ctx->Select.NameStackDepth++] = name;
}



void
_mesa_PopName( void )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (ctx->RenderMode != GL_SELECT) {
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);
   if (ctx->Select.HitFlag) {
      write_hit_record( ctx );
   }
   if (ctx->Select.NameStackDepth == 0) {
      _mesa_error( ctx, GL_STACK_UNDERFLOW, "glPopName" );
   }
   else
      ctx->Select.NameStackDepth--;
}



/**********************************************************************/
/*                           Render Mode                              */
/**********************************************************************/



/*
 * NOTE: this function can't be put in a display list.
 */
GLint
_mesa_RenderMode( GLenum mode )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint result;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      fprintf(stderr, "glRenderMode %s\n", _mesa_lookup_enum_by_nr(mode));

   FLUSH_VERTICES(ctx, _NEW_RENDERMODE);

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
            _mesa_warning(ctx, "Feedback buffer overflow");
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
	 _mesa_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
	 return 0;
   }

   switch (mode) {
      case GL_RENDER:
         break;
      case GL_SELECT:
	 if (ctx->Select.BufferSize==0) {
	    /* haven't called glSelectBuffer yet */
	    _mesa_error( ctx, GL_INVALID_OPERATION, "glRenderMode" );
	 }
	 break;
      case GL_FEEDBACK:
	 if (ctx->Feedback.BufferSize==0) {
	    /* haven't called glFeedbackBuffer yet */
	    _mesa_error( ctx, GL_INVALID_OPERATION, "glRenderMode" );
	 }
	 break;
      default:
	 _mesa_error( ctx, GL_INVALID_ENUM, "glRenderMode" );
	 return 0;
   }

   ctx->RenderMode = mode;
   if (ctx->Driver.RenderMode)
      ctx->Driver.RenderMode( ctx, mode );

   return result;
}
