/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 1999-2013  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * glBlitFramebuffer functions.
 */

#include <stdbool.h>

#include "context.h"
#include "enums.h"
#include "blit.h"
#include "fbobject.h"
#include "glformats.h"
#include "mtypes.h"
#include "state.h"


/** Set this to 1 to debug/log glBlitFramebuffer() calls */
#define DEBUG_BLIT 0



static const struct gl_renderbuffer_attachment *
find_attachment(const struct gl_framebuffer *fb,
                const struct gl_renderbuffer *rb)
{
   GLuint i;
   for (i = 0; i < Elements(fb->Attachment); i++) {
      if (fb->Attachment[i].Renderbuffer == rb)
         return &fb->Attachment[i];
   }
   return NULL;
}


/**
 * Helper function for checking if the datatypes of color buffers are
 * compatible for glBlitFramebuffer.  From the 3.1 spec, page 198:
 *
 * "GL_INVALID_OPERATION is generated if mask contains GL_COLOR_BUFFER_BIT
 *  and any of the following conditions hold:
 *   - The read buffer contains fixed-point or floating-point values and any
 *     draw buffer contains neither fixed-point nor floating-point values.
 *   - The read buffer contains unsigned integer values and any draw buffer
 *     does not contain unsigned integer values.
 *   - The read buffer contains signed integer values and any draw buffer
 *     does not contain signed integer values."
 */
static GLboolean
compatible_color_datatypes(mesa_format srcFormat, mesa_format dstFormat)
{
   GLenum srcType = _mesa_get_format_datatype(srcFormat);
   GLenum dstType = _mesa_get_format_datatype(dstFormat);

   if (srcType != GL_INT && srcType != GL_UNSIGNED_INT) {
      assert(srcType == GL_UNSIGNED_NORMALIZED ||
             srcType == GL_SIGNED_NORMALIZED ||
             srcType == GL_FLOAT);
      /* Boil any of those types down to GL_FLOAT */
      srcType = GL_FLOAT;
   }

   if (dstType != GL_INT && dstType != GL_UNSIGNED_INT) {
      assert(dstType == GL_UNSIGNED_NORMALIZED ||
             dstType == GL_SIGNED_NORMALIZED ||
             dstType == GL_FLOAT);
      /* Boil any of those types down to GL_FLOAT */
      dstType = GL_FLOAT;
   }

   return srcType == dstType;
}


static GLboolean
compatible_resolve_formats(const struct gl_renderbuffer *readRb,
                           const struct gl_renderbuffer *drawRb)
{
   GLenum readFormat, drawFormat;

   /* The simple case where we know the backing Mesa formats are the same.
    */
   if (_mesa_get_srgb_format_linear(readRb->Format) ==
       _mesa_get_srgb_format_linear(drawRb->Format)) {
      return GL_TRUE;
   }

   /* The Mesa formats are different, so we must check whether the internal
    * formats are compatible.
    *
    * Under some circumstances, the user may request e.g. two GL_RGBA8
    * textures and get two entirely different Mesa formats like RGBA8888 and
    * ARGB8888. Drivers behaving like that should be able to cope with
    * non-matching formats by themselves, because it's not the user's fault.
    *
    * Blits between linear and sRGB formats are also allowed.
    */
   readFormat = _mesa_get_nongeneric_internalformat(readRb->InternalFormat);
   drawFormat = _mesa_get_nongeneric_internalformat(drawRb->InternalFormat);
   readFormat = _mesa_get_linear_internalformat(readFormat);
   drawFormat = _mesa_get_linear_internalformat(drawFormat);

   if (readFormat == drawFormat) {
      return GL_TRUE;
   }

   return GL_FALSE;
}


static GLboolean
is_valid_blit_filter(const struct gl_context *ctx, GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_LINEAR:
      return true;
   case GL_SCALED_RESOLVE_FASTEST_EXT:
   case GL_SCALED_RESOLVE_NICEST_EXT:
      return ctx->Extensions.EXT_framebuffer_multisample_blit_scaled;
   default:
      return false;
   }
}


/**
 * Blit rectangular region, optionally from one framebuffer to another.
 *
 * Note, if the src buffer is multisampled and the dest is not, this is
 * when the samples must be resolved to a single color.
 */
void GLAPIENTRY
_mesa_BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                         GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                         GLbitfield mask, GLenum filter)
{
   const GLbitfield legalMaskBits = (GL_COLOR_BUFFER_BIT |
                                     GL_DEPTH_BUFFER_BIT |
                                     GL_STENCIL_BUFFER_BIT);
   const struct gl_framebuffer *readFb, *drawFb;
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
                  "glBlitFramebuffer(%d, %d, %d, %d,  %d, %d, %d, %d, 0x%x, %s)\n",
                  srcX0, srcY0, srcX1, srcY1,
                  dstX0, dstY0, dstX1, dstY1,
                  mask, _mesa_lookup_enum_by_nr(filter));

   if (ctx->NewState) {
      _mesa_update_state(ctx);
   }

   readFb = ctx->ReadBuffer;
   drawFb = ctx->DrawBuffer;

   if (!readFb || !drawFb) {
      /* This will normally never happen but someday we may want to
       * support MakeCurrent() with no drawables.
       */
      return;
   }

   /* check for complete framebuffers */
   if (drawFb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT ||
       readFb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                  "glBlitFramebufferEXT(incomplete draw/read buffers)");
      return;
   }

   if (!is_valid_blit_filter(ctx, filter)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBlitFramebufferEXT(%s)",
                  _mesa_lookup_enum_by_nr(filter));
      return;
   }

   if ((filter == GL_SCALED_RESOLVE_FASTEST_EXT ||
        filter == GL_SCALED_RESOLVE_NICEST_EXT) &&
        (readFb->Visual.samples == 0 || drawFb->Visual.samples > 0)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBlitFramebufferEXT(%s)",
                  _mesa_lookup_enum_by_nr(filter));
      return;
   }

   if (mask & ~legalMaskBits) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glBlitFramebufferEXT(mask)");
      return;
   }

   /* depth/stencil must be blitted with nearest filtering */
   if ((mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
        && filter != GL_NEAREST) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
             "glBlitFramebufferEXT(depth/stencil requires GL_NEAREST filter)");
      return;
   }

   /* get color read/draw renderbuffers */
   if (mask & GL_COLOR_BUFFER_BIT) {
      const GLuint numColorDrawBuffers = ctx->DrawBuffer->_NumColorDrawBuffers;
      const struct gl_renderbuffer *colorReadRb = readFb->_ColorReadBuffer;
      const struct gl_renderbuffer *colorDrawRb = NULL;
      GLuint i;

      /* From the EXT_framebuffer_object spec:
       *
       *     "If a buffer is specified in <mask> and does not exist in both
       *     the read and draw framebuffers, the corresponding bit is silently
       *     ignored."
       */
      if (!colorReadRb || numColorDrawBuffers == 0) {
         mask &= ~GL_COLOR_BUFFER_BIT;
      }
      else {
         for (i = 0; i < numColorDrawBuffers; i++) {
            colorDrawRb = ctx->DrawBuffer->_ColorDrawBuffers[i];
            if (!colorDrawRb)
               continue;

            /* Page 193 (page 205 of the PDF) in section 4.3.2 of the OpenGL
             * ES 3.0.1 spec says:
             *
             *     "If the source and destination buffers are identical, an
             *     INVALID_OPERATION error is generated. Different mipmap
             *     levels of a texture, different layers of a three-
             *     dimensional texture or two-dimensional array texture, and
             *     different faces of a cube map texture do not constitute
             *     identical buffers."
             */
            if (_mesa_is_gles3(ctx) && (colorDrawRb == colorReadRb)) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glBlitFramebuffer(source and destination color "
                           "buffer cannot be the same)");
               return;
            }

            if (!compatible_color_datatypes(colorReadRb->Format,
                                            colorDrawRb->Format)) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glBlitFramebufferEXT(color buffer datatypes mismatch)");
               return;
            }
            /* extra checks for multisample copies... */
            if (readFb->Visual.samples > 0 || drawFb->Visual.samples > 0) {
               /* color formats must match */
               if (!compatible_resolve_formats(colorReadRb, colorDrawRb)) {
                  _mesa_error(ctx, GL_INVALID_OPERATION,
                         "glBlitFramebufferEXT(bad src/dst multisample pixel formats)");
                  return;
               }
            }
         }
         if (filter != GL_NEAREST) {
            /* From EXT_framebuffer_multisample_blit_scaled specification:
             * "Calling BlitFramebuffer will result in an INVALID_OPERATION error
             * if filter is not NEAREST and read buffer contains integer data."
             */
            GLenum type = _mesa_get_format_datatype(colorReadRb->Format);
            if (type == GL_INT || type == GL_UNSIGNED_INT) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
                           "glBlitFramebufferEXT(integer color type)");
               return;
            }
         }
      }
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_renderbuffer *readRb =
         readFb->Attachment[BUFFER_STENCIL].Renderbuffer;
      struct gl_renderbuffer *drawRb =
         drawFb->Attachment[BUFFER_STENCIL].Renderbuffer;

      /* From the EXT_framebuffer_object spec:
       *
       *     "If a buffer is specified in <mask> and does not exist in both
       *     the read and draw framebuffers, the corresponding bit is silently
       *     ignored."
       */
      if ((readRb == NULL) || (drawRb == NULL)) {
	 mask &= ~GL_STENCIL_BUFFER_BIT;
      }
      else {
         int read_z_bits, draw_z_bits;

         if (_mesa_is_gles3(ctx) && (drawRb == readRb)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBlitFramebuffer(source and destination stencil "
                        "buffer cannot be the same)");
            return;
         }

         if (_mesa_get_format_bits(readRb->Format, GL_STENCIL_BITS) !=
             _mesa_get_format_bits(drawRb->Format, GL_STENCIL_BITS)) {
            /* There is no need to check the stencil datatype here, because
             * there is only one: GL_UNSIGNED_INT.
             */
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBlitFramebuffer(stencil attachment format mismatch)");
            return;
         }

         read_z_bits = _mesa_get_format_bits(readRb->Format, GL_DEPTH_BITS);
         draw_z_bits = _mesa_get_format_bits(drawRb->Format, GL_DEPTH_BITS);

         /* If both buffers also have depth data, the depth formats must match
          * as well.  If one doesn't have depth, it's not blitted, so we should
          * ignore the depth format check.
          */
         if (read_z_bits > 0 && draw_z_bits > 0 &&
             (read_z_bits != draw_z_bits ||
              _mesa_get_format_datatype(readRb->Format) !=
              _mesa_get_format_datatype(drawRb->Format))) {

            _mesa_error(ctx, GL_INVALID_OPERATION, "glBlitFramebuffer"
                        "(stencil attachment depth format mismatch)");
            return;
         }
      }
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_renderbuffer *readRb =
         readFb->Attachment[BUFFER_DEPTH].Renderbuffer;
      struct gl_renderbuffer *drawRb =
         drawFb->Attachment[BUFFER_DEPTH].Renderbuffer;

      /* From the EXT_framebuffer_object spec:
       *
       *     "If a buffer is specified in <mask> and does not exist in both
       *     the read and draw framebuffers, the corresponding bit is silently
       *     ignored."
       */
      if ((readRb == NULL) || (drawRb == NULL)) {
	 mask &= ~GL_DEPTH_BUFFER_BIT;
      }
      else {
         int read_s_bit, draw_s_bit;

         if (_mesa_is_gles3(ctx) && (drawRb == readRb)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBlitFramebuffer(source and destination depth "
                        "buffer cannot be the same)");
            return;
         }

         if ((_mesa_get_format_bits(readRb->Format, GL_DEPTH_BITS) !=
              _mesa_get_format_bits(drawRb->Format, GL_DEPTH_BITS)) ||
             (_mesa_get_format_datatype(readRb->Format) !=
              _mesa_get_format_datatype(drawRb->Format))) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBlitFramebuffer(depth attachment format mismatch)");
            return;
         }

         read_s_bit = _mesa_get_format_bits(readRb->Format, GL_STENCIL_BITS);
         draw_s_bit = _mesa_get_format_bits(drawRb->Format, GL_STENCIL_BITS);

         /* If both buffers also have stencil data, the stencil formats must
          * match as well.  If one doesn't have stencil, it's not blitted, so
          * we should ignore the stencil format check.
          */
         if (read_s_bit > 0 && draw_s_bit > 0 && read_s_bit != draw_s_bit) {
            _mesa_error(ctx, GL_INVALID_OPERATION, "glBlitFramebuffer"
                        "(depth attachment stencil bits mismatch)");
            return;
         }
      }
   }


   if (_mesa_is_gles3(ctx)) {
      /* Page 194 (page 206 of the PDF) in section 4.3.2 of the OpenGL ES
       * 3.0.1 spec says:
       *
       *     "If SAMPLE_BUFFERS for the draw framebuffer is greater than zero,
       *     an INVALID_OPERATION error is generated."
       */
      if (drawFb->Visual.samples > 0) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBlitFramebuffer(destination samples must be 0)");
         return;
      }

      /* Page 194 (page 206 of the PDF) in section 4.3.2 of the OpenGL ES
       * 3.0.1 spec says:
       *
       *     "If SAMPLE_BUFFERS for the read framebuffer is greater than zero,
       *     no copy is performed and an INVALID_OPERATION error is generated
       *     if the formats of the read and draw framebuffers are not
       *     identical or if the source and destination rectangles are not
       *     defined with the same (X0, Y0) and (X1, Y1) bounds."
       *
       * The format check was made above because desktop OpenGL has the same
       * requirement.
       */
      if (readFb->Visual.samples > 0
          && (srcX0 != dstX0 || srcY0 != dstY0
              || srcX1 != dstX1 || srcY1 != dstY1)) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBlitFramebuffer(bad src/dst multisample region)");
         return;
      }
   } else {
      if (readFb->Visual.samples > 0 &&
          drawFb->Visual.samples > 0 &&
          readFb->Visual.samples != drawFb->Visual.samples) {
         _mesa_error(ctx, GL_INVALID_OPERATION,
                     "glBlitFramebufferEXT(mismatched samples)");
         return;
      }

      /* extra checks for multisample copies... */
      if ((readFb->Visual.samples > 0 || drawFb->Visual.samples > 0) &&
          (filter == GL_NEAREST || filter == GL_LINEAR)) {
         /* src and dest region sizes must be the same */
         if (abs(srcX1 - srcX0) != abs(dstX1 - dstX0) ||
             abs(srcY1 - srcY0) != abs(dstY1 - dstY0)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBlitFramebufferEXT(bad src/dst multisample region sizes)");
            return;
         }
      }
   }

   /* Debug code */
   if (DEBUG_BLIT) {
      const struct gl_renderbuffer *colorReadRb = readFb->_ColorReadBuffer;
      const struct gl_renderbuffer *colorDrawRb = NULL;
      GLuint i = 0;

      printf("glBlitFramebuffer(%d, %d, %d, %d,  %d, %d, %d, %d,"
	     " 0x%x, 0x%x)\n",
	     srcX0, srcY0, srcX1, srcY1,
	     dstX0, dstY0, dstX1, dstY1,
	     mask, filter);
      if (colorReadRb) {
         const struct gl_renderbuffer_attachment *att;

         att = find_attachment(readFb, colorReadRb);
         printf("  Src FBO %u  RB %u (%dx%d)  ",
		readFb->Name, colorReadRb->Name,
		colorReadRb->Width, colorReadRb->Height);
         if (att && att->Texture) {
            printf("Tex %u  tgt 0x%x  level %u  face %u",
		   att->Texture->Name,
		   att->Texture->Target,
		   att->TextureLevel,
		   att->CubeMapFace);
         }
         printf("\n");

         /* Print all active color render buffers */
         for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
            colorDrawRb = ctx->DrawBuffer->_ColorDrawBuffers[i];
            if (!colorDrawRb)
               continue;

            att = find_attachment(drawFb, colorDrawRb);
            printf("  Dst FBO %u  RB %u (%dx%d)  ",
		   drawFb->Name, colorDrawRb->Name,
		   colorDrawRb->Width, colorDrawRb->Height);
            if (att && att->Texture) {
               printf("Tex %u  tgt 0x%x  level %u  face %u",
		      att->Texture->Name,
		      att->Texture->Target,
		      att->TextureLevel,
		      att->CubeMapFace);
            }
            printf("\n");
         }
      }
   }

   if (!mask ||
       (srcX1 - srcX0) == 0 || (srcY1 - srcY0) == 0 ||
       (dstX1 - dstX0) == 0 || (dstY1 - dstY0) == 0) {
      return;
   }

   ASSERT(ctx->Driver.BlitFramebuffer);
   ctx->Driver.BlitFramebuffer(ctx,
                               srcX0, srcY0, srcX1, srcY1,
                               dstX0, dstY0, dstX1, dstY1,
                               mask, filter);
}
