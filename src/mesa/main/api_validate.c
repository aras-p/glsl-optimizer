
/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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
#include "image.h"  /* for _mesa_sizeof_type() */
#include "imports.h"
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

   if (mode > GL_POLYGON) {
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

   if (ctx->Array.Vertex.Enabled
       || (ctx->VertexProgram.Enabled && ctx->Array.VertexAttrib[0].Enabled))
      return GL_TRUE;
   else
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

   if (mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (end < start) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawRangeElements(end<start)");
      return GL_FALSE;
   }

   if (type != GL_UNSIGNED_INT &&
       type != GL_UNSIGNED_BYTE &&
       type != GL_UNSIGNED_SHORT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state( ctx );

   if (ctx->Array.Vertex.Enabled
       || (ctx->VertexProgram.Enabled && ctx->Array.VertexAttrib[0].Enabled))
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 * Helper routine for validating vertex array data to be sure the given
 * element lies within the legal range (i.e. vertex buffer object).
 */
static INLINE GLboolean
validate(GLcontext *ctx, GLint attribArray,
         const struct gl_client_array *array, GLint element)
{
   if (ctx->VertexProgram.Enabled
       && attribArray >= 0
       && ctx->Array.VertexAttrib[attribArray].Enabled) {
      if (element >= ctx->Array.VertexAttrib[attribArray]._MaxElement)
         return GL_FALSE;
   }
   else if (array && array->Enabled) {
      if (element >= array->_MaxElement)
         return GL_FALSE;
   }
   return GL_TRUE;
}


/**
 * Called from the tnl module to error check the function parameters and
 * verify that we really can draw something.
 */
GLboolean
_mesa_validate_DrawArrays(GLcontext *ctx,
			  GLenum mode, GLint start, GLsizei count)
{
   GLint i;
   ASSERT_OUTSIDE_BEGIN_END_WITH_RETVAL(ctx, GL_FALSE);

   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawArrays(count)" );
      return GL_FALSE;
   }

   if (mode > GL_POLYGON) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glDrawArrays(mode)" );
      return GL_FALSE;
   }

   if (ctx->NewState)
      _mesa_update_state( ctx );

   /* Either the conventional vertex position array, or the 0th
    * generic vertex attribute array is required to be enabled.
    */
   if (ctx->VertexProgram.Enabled
       && ctx->Array.VertexAttrib[VERT_ATTRIB_POS].Enabled) {
      if (start + count >= ctx->Array.VertexAttrib[VERT_ATTRIB_POS]._MaxElement)
         return GL_FALSE;
   }
   else if (ctx->Array.Vertex.Enabled) {
      if (start + count >= ctx->Array.Vertex._MaxElement)
         return GL_FALSE;
   }
   else {
      /* no vertex position array! */
      return GL_FALSE;
   }

   /*
    * OK, now check all the other enabled arrays to be sure the elements
    * are in bounds.
    */
   if (!validate(ctx, VERT_ATTRIB_WEIGHT, NULL, start + count))
      return GL_FALSE;

   if (!validate(ctx, VERT_ATTRIB_NORMAL, &ctx->Array.Normal, start + count))
      return GL_FALSE;

   if (!validate(ctx, VERT_ATTRIB_COLOR0, &ctx->Array.Color, start + count))
      return GL_FALSE;

   if (!validate(ctx, VERT_ATTRIB_COLOR1, &ctx->Array.SecondaryColor, start + count))
      return GL_FALSE;

   if (!validate(ctx, VERT_ATTRIB_FOG, &ctx->Array.FogCoord, start + count))
      return GL_FALSE;

   if (!validate(ctx, VERT_ATTRIB_SIX, NULL, start + count))
      return GL_FALSE;

   if (!validate(ctx, VERT_ATTRIB_SEVEN, &ctx->Array.FogCoord, start + count))
      return GL_FALSE;

   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++)
      if (!validate(ctx, VERT_ATTRIB_TEX0 + i, &ctx->Array.TexCoord[i], start + count))
         return GL_FALSE;

   if (!validate(ctx, -1, &ctx->Array.Index, start + count))
      return GL_FALSE;

   if (!validate(ctx, -1, &ctx->Array.EdgeFlag, start + count))
      return GL_FALSE;

   return GL_TRUE;
}
