/* $Id: api_validate.c,v 1.5 2001/03/12 00:48:37 gareth Exp $ */

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
#include "api_validate.h"
#include "context.h"
#include "mtypes.h"
#include "state.h"


GLboolean
_mesa_validate_DrawElements(GLcontext *ctx,
			    GLenum mode, GLsizei count, GLenum type,
			    const GLvoid *indices)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx,  GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE, "glDrawElements(count)" );
      return GL_FALSE;
   }

   if (mode < 0 ||
       mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (type != GL_UNSIGNED_INT &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_UNSIGNED_SHORT)
   {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (!ctx->Array.Vertex.Enabled)
      return GL_FALSE;

   return GL_TRUE;
}


GLboolean
_mesa_validate_DrawRangeElements(GLcontext *ctx, GLenum mode,
				 GLuint start, GLuint end,
				 GLsizei count, GLenum type,
				 const GLvoid *indices)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count <= 0) {
      if (count < 0)
	 _mesa_error(ctx, GL_INVALID_VALUE, "glDrawElements(count)" );
      return GL_FALSE;
   }

   if (mode < 0 || mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (end < start) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawRangeElements(end<start)");
      return GL_FALSE;
   }

   if (type != GL_UNSIGNED_INT &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_UNSIGNED_SHORT)
   {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (!ctx->Array.Vertex.Enabled)
      return GL_FALSE;

   return GL_TRUE;
}



GLboolean
_mesa_validate_DrawArrays(GLcontext *ctx,
			  GLenum mode, GLint start, GLsizei count)
{
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count<0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return GL_FALSE;
   }

   if (mode < 0 || mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (!ctx->Array.Vertex.Enabled)
      return GL_FALSE;

   return GL_TRUE;
}
