/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * glGet functions
 *
 * \author Brian Paul
 */

#include "main/imports.h"
#include "main/context.h"

#include "pipe/p_defines.h"

#include "st_cb_fbo.h"
#include "st_cb_get.h"



/**
 * Examine the current color read buffer format to determine
 * which GL pixel format/type combo is the best match.
 */
static void
get_preferred_read_format_type(GLcontext *ctx, GLint *format, GLint *type)
{
   struct gl_framebuffer *fb = ctx->ReadBuffer;
   struct st_renderbuffer *strb = st_renderbuffer(fb->_ColorReadBuffer);

   /* defaults */
   *format = ctx->Const.ColorReadFormat;
   *type = ctx->Const.ColorReadType;

   if (strb) {
      /* XXX could add more cases here... */
      if (strb->format == PIPE_FORMAT_A8R8G8B8_UNORM) {
         *format = GL_BGRA;
         if (_mesa_little_endian())
            *type = GL_UNSIGNED_INT_8_8_8_8_REV;
         else
            *type = GL_UNSIGNED_INT_8_8_8_8;
      }
   }
}


/**
 * We only intercept the OES preferred ReadPixels format/type.
 * Everything else goes to the default _mesa_GetIntegerv.
 */
static GLboolean 
st_GetIntegerv(GLcontext *ctx, GLenum pname, GLint *params)
{
   GLint dummy;

   switch (pname) {
   case GL_IMPLEMENTATION_COLOR_READ_TYPE_OES:
      get_preferred_read_format_type(ctx, &dummy, params);
      return GL_TRUE;
   case GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES:
      get_preferred_read_format_type(ctx, params, &dummy);
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


void st_init_get_functions(struct dd_function_table *functions)
{
   functions->GetIntegerv = st_GetIntegerv;
}
