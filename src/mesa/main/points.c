/* $Id: points.c,v 1.22 2000/11/15 16:38:40 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.4
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
#include "feedback.h"
#include "macros.h"
#include "mmath.h"
#include "points.h"
#include "texstate.h"
#include "types.h"
#include "vb.h"
#endif



void
_mesa_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPointSize");

   if (size <= 0.0) {
      gl_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }

   if (ctx->Point.Size != size) {
      ctx->Point.Size = size;
      ctx->Point._Size = CLAMP(size, ctx->Const.MinPointSize, ctx->Const.MaxPointSize);
      ctx->_TriangleCaps &= ~DD_POINT_SIZE;
      if (size != 1.0)
         ctx->_TriangleCaps |= DD_POINT_SIZE;
      ctx->NewState |= _NEW_POINT;
   }
}



void
_mesa_PointParameterfEXT( GLenum pname, GLfloat param)
{
   _mesa_PointParameterfvEXT(pname, &param);
}


void
_mesa_PointParameterfvEXT( GLenum pname, const GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glPointParameterfvEXT");

   switch (pname) {
      case GL_DISTANCE_ATTENUATION_EXT:
         {
            const GLboolean tmp = ctx->Point._Attenuated;
            COPY_3V(ctx->Point.Params, params);
            ctx->Point._Attenuated = (params[0] != 1.0 ||
				      params[1] != 0.0 ||
				      params[2] != 0.0);

            if (tmp != ctx->Point._Attenuated) {
               ctx->_Enabled ^= ENABLE_POINT_ATTEN;
               ctx->_TriangleCaps ^= DD_POINT_ATTEN;
	       ctx->_NeedEyeCoords ^= NEED_EYE_POINT_ATTEN;
            }
         }
         break;
      case GL_POINT_SIZE_MIN_EXT:
         if (*params < 0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         ctx->Point.MinSize = *params;
         break;
      case GL_POINT_SIZE_MAX_EXT:
         if (*params < 0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         ctx->Point.MaxSize = *params;
         break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
         if (*params < 0.0F) {
            gl_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         ctx->Point.Threshold = *params;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glPointParameterfvEXT" );
         return;
   }

   ctx->NewState |= _NEW_POINT;
}

