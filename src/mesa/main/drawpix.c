/* $Id: drawpix.c,v 1.44 2000/10/31 18:09:44 keithw Exp $ */

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
#include "colormac.h"
#include "context.h"
#include "feedback.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "types.h"
#include "swrast/swrast.h"
#endif





/*
 * Execute glDrawPixels
 */
void
_mesa_DrawPixels( GLsizei width, GLsizei height,
                  GLenum format, GLenum type, const GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glDrawPixels");

   if (ctx->RenderMode==GL_RENDER) {
      GLint x, y;
      if (!pixels || !ctx->Current.RasterPosValid) {
	 return;
      }

      if (ctx->NewState) {
         gl_update_state(ctx);
      }

      x = (GLint) (ctx->Current.RasterPos[0] + 0.5F);
      y = (GLint) (ctx->Current.RasterPos[1] + 0.5F);

      ctx->OcclusionResult = GL_TRUE;

      /* see if device driver can do the drawpix */
      RENDER_START(ctx);

      if (ctx->Driver.DrawPixels
          && (*ctx->Driver.DrawPixels)(ctx, x, y, width, height, format, type,
                                       &ctx->Unpack, pixels)) {
         /* finished */
      } else
	 _swrast_DrawPixels( ctx, x, y, width, height, format, type,
			     &ctx->Unpack, pixels );

      RENDER_FINISH(ctx);
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      if (ctx->Current.RasterPosValid) {
         GLfloat color[4];
	 GLfloat texcoord[4], invq;
         color[0] = CHAN_TO_FLOAT(ctx->Current.Color[0]);
         color[1] = CHAN_TO_FLOAT(ctx->Current.Color[1]);
         color[2] = CHAN_TO_FLOAT(ctx->Current.Color[2]);
         color[3] = CHAN_TO_FLOAT(ctx->Current.Color[3]);
         invq = 1.0F / ctx->Current.Texcoord[0][3];
         texcoord[0] = ctx->Current.Texcoord[0][0] * invq;
         texcoord[1] = ctx->Current.Texcoord[0][1] * invq;
         texcoord[2] = ctx->Current.Texcoord[0][2] * invq;
         texcoord[3] = ctx->Current.Texcoord[0][3];
         FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
         gl_feedback_vertex( ctx,
                             ctx->Current.RasterPos,
                             color, ctx->Current.Index, texcoord );
      }
   }
   else if (ctx->RenderMode==GL_SELECT) {
      if (ctx->Current.RasterPosValid) {
         gl_update_hitflag( ctx, ctx->Current.RasterPos[2] );
      }
   }
}

