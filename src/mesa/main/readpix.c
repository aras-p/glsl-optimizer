/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
#include "imports.h"
#include "bufferobj.h"
#include "context.h"
#include "enums.h"
#include "readpix.h"
#include "framebuffer.h"
#include "formats.h"
#include "image.h"
#include "mtypes.h"
#include "pbo.h"
#include "state.h"


/**
 * Do error checking of the format/type parameters to glReadPixels and
 * glDrawPixels.
 * \param drawing if GL_TRUE do checking for DrawPixels, else do checking
 *                for ReadPixels.
 * \return GL_TRUE if error detected, GL_FALSE if no errors
 */
GLboolean
_mesa_error_check_format_type(struct gl_context *ctx, GLenum format,
                              GLenum type, GLboolean drawing)
{
   const char *readDraw = drawing ? "Draw" : "Read";
   const GLboolean reading = !drawing;

   /* state validation should have already been done */
   ASSERT(ctx->NewState == 0x0);

   if (ctx->Extensions.EXT_packed_depth_stencil
       && type == GL_UNSIGNED_INT_24_8_EXT
       && format != GL_DEPTH_STENCIL_EXT) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "gl%sPixels(format is not GL_DEPTH_STENCIL_EXT)", readDraw);
      return GL_TRUE;
   }

   /* basic combinations test */
   if (!_mesa_is_legal_format_and_type(ctx, format, type)) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  "gl%sPixels(format or type)", readDraw);
      return GL_TRUE;
   }

   /* additional checks */
   switch (format) {
   case GL_RG:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_ALPHA:
   case GL_LUMINANCE:
   case GL_LUMINANCE_ALPHA:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
   case GL_RED_INTEGER_EXT:
   case GL_GREEN_INTEGER_EXT:
   case GL_BLUE_INTEGER_EXT:
   case GL_ALPHA_INTEGER_EXT:
   case GL_RGB_INTEGER_EXT:
   case GL_RGBA_INTEGER_EXT:
   case GL_BGR_INTEGER_EXT:
   case GL_BGRA_INTEGER_EXT:
   case GL_LUMINANCE_INTEGER_EXT:
   case GL_LUMINANCE_ALPHA_INTEGER_EXT:
      if (!drawing) {
         /* reading */
         if (!_mesa_source_buffer_exists(ctx, GL_COLOR)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glReadPixels(no color buffer)");
            return GL_TRUE;
         }
      }
      break;
   case GL_COLOR_INDEX:
      if (drawing) {
         if (ctx->PixelMaps.ItoR.Size == 0 ||
	     ctx->PixelMaps.ItoG.Size == 0 ||
	     ctx->PixelMaps.ItoB.Size == 0) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                   "glDrawPixels(drawing color index pixels into RGB buffer)");
            return GL_TRUE;
         }
      }
      else {
         /* reading */
         if (!_mesa_source_buffer_exists(ctx, GL_COLOR)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glReadPixels(no color buffer)");
            return GL_TRUE;
         }
         /* We no longer support CI-mode color buffers so trying to read
          * GL_COLOR_INDEX pixels is always an error.
          */
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(color buffer is RGB)");
         return GL_TRUE;
      }
      break;
   case GL_STENCIL_INDEX:
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format)) ||
          (reading && !_mesa_source_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no stencil buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   case GL_DEPTH_COMPONENT:
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no depth buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   case GL_DEPTH_STENCIL_EXT:
      if (!ctx->Extensions.EXT_packed_depth_stencil ||
          type != GL_UNSIGNED_INT_24_8_EXT) {
         _mesa_error(ctx, GL_INVALID_ENUM, "gl%sPixels(type)", readDraw);
         return GL_TRUE;
      }
      if ((drawing && !_mesa_dest_buffer_exists(ctx, format)) ||
          (reading && !_mesa_source_buffer_exists(ctx, format))) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "gl%sPixels(no depth or stencil buffer)", readDraw);
         return GL_TRUE;
      }
      break;
   default:
      /* this should have been caught in _mesa_is_legal_format_type() */
      _mesa_problem(ctx, "unexpected format in _mesa_%sPixels", readDraw);
      return GL_TRUE;
   }

   /* no errors */
   return GL_FALSE;
}
      


void GLAPIENTRY
_mesa_ReadnPixelsARB( GLint x, GLint y, GLsizei width, GLsizei height,
		      GLenum format, GLenum type, GLsizei bufSize,
                      GLvoid *pixels )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   FLUSH_CURRENT(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glReadPixels(%d, %d, %s, %s, %p)\n",
                  width, height,
                  _mesa_lookup_enum_by_nr(format),
                  _mesa_lookup_enum_by_nr(type),
                  pixels);

   if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE,
                   "glReadPixels(width=%d height=%d)", width, height );
      return;
   }

   if (ctx->NewState)
      _mesa_update_state(ctx);

   if (_mesa_error_check_format_type(ctx, format, type, GL_FALSE)) {
      /* found an error */
      return;
   }

   /* Check that the destination format and source buffer are both
    * integer-valued or both non-integer-valued.
    */
   if (ctx->Extensions.EXT_texture_integer && _mesa_is_color_format(format)) {
      const struct gl_renderbuffer *rb = ctx->ReadBuffer->_ColorReadBuffer;
      const GLboolean srcInteger = _mesa_is_format_integer_color(rb->Format);
      const GLboolean dstInteger = _mesa_is_integer_format(format);
      if (dstInteger != srcInteger) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(integer / non-integer format mismatch");
         return;
      }
   }

   if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glReadPixels(incomplete framebuffer)" );
      return;
   }

   if (!_mesa_source_buffer_exists(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(no readbuffer)");
      return;
   }

   if (width == 0 || height == 0)
      return; /* nothing to do */

   if (!_mesa_validate_pbo_access(2, &ctx->Pack, width, height, 1,
                                  format, type, bufSize, pixels)) {
      if (_mesa_is_bufferobj(ctx->Pack.BufferObj)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadPixels(out of bounds PBO access)");
      } else {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glReadnPixelsARB(out of bounds access:"
                     " bufSize (%d) is too small)", bufSize);
      }
      return;
   }

   if (_mesa_is_bufferobj(ctx->Pack.BufferObj) &&
       _mesa_bufferobj_mapped(ctx->Pack.BufferObj)) {
      /* buffer is mapped - that's an error */
      _mesa_error(ctx, GL_INVALID_OPERATION, "glReadPixels(PBO is mapped)");
      return;
   }

   ctx->Driver.ReadPixels(ctx, x, y, width, height,
			  format, type, &ctx->Pack, pixels);
}

void GLAPIENTRY
_mesa_ReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		  GLenum format, GLenum type, GLvoid *pixels )
{
   _mesa_ReadnPixelsARB(x, y, width, height, format, type, INT_MAX, pixels);
}
