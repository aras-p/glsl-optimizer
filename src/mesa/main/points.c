/* $Id: points.c,v 1.31 2001/03/29 21:16:25 keithw Exp $ */

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
#include "context.h"
#include "macros.h"
#include "mmath.h"
#include "points.h"
#include "texstate.h"
#include "mtypes.h"
#endif



void
_mesa_PointSize( GLfloat size )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (size <= 0.0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPointSize" );
      return;
   }

   if (ctx->Point.Size == size)
      return;

   FLUSH_VERTICES(ctx, _NEW_POINT);
   ctx->Point.Size = size;
   ctx->Point._Size = CLAMP(size,
			    ctx->Const.MinPointSize,
			    ctx->Const.MaxPointSize);

   if (ctx->Point._Size == 1.0F)
      ctx->_TriangleCaps &= ~DD_POINT_SIZE;
   else
      ctx->_TriangleCaps |= DD_POINT_SIZE;

   if (ctx->Driver.PointSize)
      (*ctx->Driver.PointSize)(ctx, size);
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
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (pname) {
      case GL_DISTANCE_ATTENUATION_EXT:
         {
            const GLboolean tmp = ctx->Point._Attenuated;
            if (TEST_EQ_3V(ctx->Point.Params, params))
	       return;

	    FLUSH_VERTICES(ctx, _NEW_POINT);
            COPY_3V(ctx->Point.Params, params);

	    /* Update several derived values now.  This likely to be
	     * more efficient than trying to catch this statechange in
	     * state.c.
	     */
            ctx->Point._Attenuated = (params[0] != 1.0 ||
				      params[1] != 0.0 ||
				      params[2] != 0.0);

            if (tmp != ctx->Point._Attenuated) {
               ctx->_TriangleCaps ^= DD_POINT_ATTEN;
	       ctx->_NeedEyeCoords ^= NEED_EYE_POINT_ATTEN;
            }
         }
         break;
      case GL_POINT_SIZE_MIN_EXT:
         if (*params < 0.0F) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         if (ctx->Point.MinSize == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.MinSize = *params;
         break;
      case GL_POINT_SIZE_MAX_EXT:
         if (*params < 0.0F) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         if (ctx->Point.MaxSize == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.MaxSize = *params;
         break;
      case GL_POINT_FADE_THRESHOLD_SIZE_EXT:
         if (*params < 0.0F) {
            _mesa_error( ctx, GL_INVALID_VALUE, "glPointParameterfvEXT" );
            return;
         }
         if (ctx->Point.Threshold == *params)
	    return;
	 FLUSH_VERTICES(ctx, _NEW_POINT);
         ctx->Point.Threshold = *params;
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glPointParameterfvEXT" );
         return;
   }

   if (ctx->Driver.PointParameterfv)
      (*ctx->Driver.PointParameterfv)(ctx, pname, params);
}
