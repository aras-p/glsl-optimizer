/* $Id: drawpix.c,v 1.55 2001/06/26 01:32:48 brianp Exp $ */

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
#include "drawpix.h"
#include "feedback.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "mtypes.h"
#endif





/*
 * Execute glDrawPixels
 */
void
_mesa_DrawPixels( GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (ctx->RenderMode==GL_RENDER) {
      GLint x, y;
      if (!pixels || !ctx->Current.RasterPosValid) {
	 return;
      }

      if (ctx->NewState) {
         _mesa_update_state(ctx);
      }

#if 1
      x = IROUND(ctx->Current.RasterPos[0]);
      y = IROUND(ctx->Current.RasterPos[1]);
#else
      x = IFLOOR(ctx->Current.RasterPos[0]);
      y = IFLOOR(ctx->Current.RasterPos[1]);
#endif
      ctx->OcclusionResult = GL_TRUE;
      ctx->Driver.DrawPixels(ctx, x, y, width, height, format, type,
			     &ctx->Unpack, pixels);
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      if (ctx->Current.RasterPosValid) {
	 GLfloat texcoord[4], invq;

	 FLUSH_CURRENT(ctx, 0);
         invq = 1.0F / ctx->Current.Texcoord[0][3];
         texcoord[0] = ctx->Current.Texcoord[0][0] * invq;
         texcoord[1] = ctx->Current.Texcoord[0][1] * invq;
         texcoord[2] = ctx->Current.Texcoord[0][2] * invq;
         texcoord[3] = ctx->Current.Texcoord[0][3];
         FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
         _mesa_feedback_vertex( ctx,
				ctx->Current.RasterPos,
				ctx->Current.Color,
				ctx->Current.Index, 
				texcoord );
      }
   }
   else if (ctx->RenderMode==GL_SELECT) {
      if (ctx->Current.RasterPosValid) {
         _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
      }
   }
}



void
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (!pixels) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glReadPixels(pixels)" );
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, pixels);

}



void
_mesa_CopyPixels( GLint srcx, GLint srcy, GLsizei width, GLsizei height,
                  GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint destx, desty;
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glCopyPixels" );
      return;
   }

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   if (ctx->RenderMode==GL_RENDER) {
      /* Destination of copy: */
      if (!ctx->Current.RasterPosValid) {
	 return;
      }
#if 1
      destx = IROUND(ctx->Current.RasterPos[0]);
      desty = IROUND(ctx->Current.RasterPos[1]);
#else
      destx = IFLOOR(ctx->Current.RasterPos[0]);
      desty = IFLOOR(ctx->Current.RasterPos[1]);
#endif
      ctx->OcclusionResult = GL_TRUE;

      ctx->Driver.CopyPixels( ctx, srcx, srcy, width, height, destx, desty,
			      type );
   }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT( ctx, 0 );
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
      _mesa_feedback_vertex( ctx, 
			     ctx->Current.RasterPos,
			     ctx->Current.Color, 
			     ctx->Current.Index,
			     ctx->Current.Texcoord[0] );
   }
   else if (ctx->RenderMode == GL_SELECT) {
      _mesa_update_hitflag( ctx, ctx->Current.RasterPos[2] );
   }
}



void
_mesa_Bitmap( GLsizei width, GLsizei height,
              GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove,
              const GLubyte *bitmap )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);


   /* Error checking */
   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glBitmap" );
      return;
   }

   if (ctx->Current.RasterPosValid == GL_FALSE) {
      return;    /* do nothing */
   }

   if (ctx->RenderMode==GL_RENDER) {
      if (bitmap) {
#if 0
         GLint x = (GLint) ( (ctx->Current.RasterPos[0] - xorig) + 0.0F );
         GLint y = (GLint) ( (ctx->Current.RasterPos[1] - yorig) + 0.0F );
#else
         GLint x = IFLOOR(ctx->Current.RasterPos[0] - xorig);
         GLint y = IFLOOR(ctx->Current.RasterPos[1] - yorig);
#endif
         if (ctx->NewState) {
            _mesa_update_state(ctx);
         }

         ctx->OcclusionResult = GL_TRUE;
	 ctx->Driver.Bitmap( ctx, x, y, width, height, &ctx->Unpack, bitmap );
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      GLfloat color[4], texcoord[4], invq;

      color[0] = ctx->Current.RasterColor[0];
      color[1] = ctx->Current.RasterColor[1];
      color[2] = ctx->Current.RasterColor[2];
      color[3] = ctx->Current.RasterColor[3];
      if (ctx->Current.Texcoord[0][3] == 0.0)
         invq = 1.0F;
      else
         invq = 1.0F / ctx->Current.RasterTexCoord[3];
      texcoord[0] = ctx->Current.RasterTexCoord[0] * invq;
      texcoord[1] = ctx->Current.RasterTexCoord[1] * invq;
      texcoord[2] = ctx->Current.RasterTexCoord[2] * invq;
      texcoord[3] = ctx->Current.RasterTexCoord[3];
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_BITMAP_TOKEN );
      _mesa_feedback_vertex( ctx,
                          ctx->Current.RasterPos,
			  color, ctx->Current.RasterIndex, texcoord );
   }
   else if (ctx->RenderMode==GL_SELECT) {
      /* Bitmaps don't generate selection hits.  See appendix B of 1.1 spec. */
   }

   /* update raster position */
   ctx->Current.RasterPos[0] += xmove;
   ctx->Current.RasterPos[1] += ymove;
}
