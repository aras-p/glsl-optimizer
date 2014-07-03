/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/multisample.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/glformats.h"
#include "main/state.h"


/**
 * Called via glSampleCoverageARB
 */
void GLAPIENTRY
_mesa_SampleCoverage(GLclampf value, GLboolean invert)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   ctx->Multisample.SampleCoverageValue = (GLfloat) CLAMP(value, 0.0, 1.0);
   ctx->Multisample.SampleCoverageInvert = invert;
   ctx->NewState |= _NEW_MULTISAMPLE;
}


/**
 * Initialize the context's multisample state.
 * \param ctx  the GL context.
 */
void
_mesa_init_multisample(struct gl_context *ctx)
{
   ctx->Multisample.Enabled = GL_TRUE;
   ctx->Multisample.SampleAlphaToCoverage = GL_FALSE;
   ctx->Multisample.SampleAlphaToOne = GL_FALSE;
   ctx->Multisample.SampleCoverage = GL_FALSE;
   ctx->Multisample.SampleCoverageValue = 1.0;
   ctx->Multisample.SampleCoverageInvert = GL_FALSE;

   /* ARB_texture_multisample / GL3.2 additions */
   ctx->Multisample.SampleMask = GL_FALSE;
   ctx->Multisample.SampleMaskValue = ~(GLbitfield)0;
}


void GLAPIENTRY
_mesa_GetMultisamplefv(GLenum pname, GLuint index, GLfloat * val)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx->NewState & _NEW_BUFFERS) {
      _mesa_update_state(ctx);
   }

   switch (pname) {
   case GL_SAMPLE_POSITION: {
      if ((int) index >= ctx->DrawBuffer->Visual.samples) {
         _mesa_error( ctx, GL_INVALID_VALUE, "glGetMultisamplefv(index)" );
         return;
      }

      ctx->Driver.GetSamplePosition(ctx, ctx->DrawBuffer, index, val);

      /* winsys FBOs are upside down */
      if (_mesa_is_winsys_fbo(ctx->DrawBuffer))
         val[1] = 1.0f - val[1];

      return;
   }

   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetMultisamplefv(pname)" );
      return;
   }
}

void GLAPIENTRY
_mesa_SampleMaski(GLuint index, GLbitfield mask)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.ARB_texture_multisample) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glSampleMaski");
      return;
   }

   if (index != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glSampleMaski(index)");
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_MULTISAMPLE);
   ctx->Multisample.SampleMaskValue = mask;
}

/**
 * Called via glMinSampleShadingARB
 */
void GLAPIENTRY
_mesa_MinSampleShading(GLclampf value)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.ARB_sample_shading || !_mesa_is_desktop_gl(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glMinSampleShading");
      return;
   }

   FLUSH_VERTICES(ctx, 0);

   ctx->Multisample.MinSampleShadingValue = CLAMP(value, 0.0, 1.0);
   ctx->NewState |= _NEW_MULTISAMPLE;
}

/**
 * Helper for checking a requested sample count against the limit
 * for a particular (target, internalFormat) pair. The limit imposed,
 * and the error generated, both depend on which extensions are supported.
 *
 * Returns a GL error enum, or GL_NO_ERROR if the requested sample count is
 * acceptable.
 */
GLenum
_mesa_check_sample_count(struct gl_context *ctx, GLenum target,
                         GLenum internalFormat, GLsizei samples)
{
   /* If ARB_internalformat_query is supported, then treat its highest
    * returned sample count as the absolute maximum for this format; it is
    * allowed to exceed MAX_SAMPLES.
    *
    * From the ARB_internalformat_query spec:
    *
    * "If <samples is greater than the maximum number of samples supported
    * for <internalformat> then the error INVALID_OPERATION is generated."
    */
   if (ctx->Extensions.ARB_internalformat_query) {
      GLint buffer[16];
      int count = ctx->Driver.QuerySamplesForFormat(ctx, target,
                                                    internalFormat, buffer);
      int limit = count ? buffer[0] : -1;

      return samples > limit ? GL_INVALID_OPERATION : GL_NO_ERROR;
   }

   /* If ARB_texture_multisample is supported, we have separate limits,
    * which may be lower than MAX_SAMPLES:
    *
    * From the ARB_texture_multisample spec, when describing the operation
    * of RenderbufferStorageMultisample:
    *
    * "If <internalformat> is a signed or unsigned integer format and
    * <samples> is greater than the value of MAX_INTEGER_SAMPLES, then the
    * error INVALID_OPERATION is generated"
    *
    * And when describing the operation of TexImage*Multisample:
    *
    * "The error INVALID_OPERATION may be generated if any of the following
    * are true:
    *
    * * <internalformat> is a depth/stencil-renderable format and <samples>
    *   is greater than the value of MAX_DEPTH_TEXTURE_SAMPLES
    * * <internalformat> is a color-renderable format and <samples> is
    *   grater than the value of MAX_COLOR_TEXTURE_SAMPLES
    * * <internalformat> is a signed or unsigned integer format and
    *   <samples> is greater than the value of MAX_INTEGER_SAMPLES
    */

   if (ctx->Extensions.ARB_texture_multisample) {
      if (_mesa_is_enum_format_integer(internalFormat))
         return samples > ctx->Const.MaxIntegerSamples
            ? GL_INVALID_OPERATION : GL_NO_ERROR;

      if (target == GL_TEXTURE_2D_MULTISAMPLE ||
          target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY) {

         if (_mesa_is_depth_or_stencil_format(internalFormat))
            return samples > ctx->Const.MaxDepthTextureSamples
               ? GL_INVALID_OPERATION : GL_NO_ERROR;
         else
            return samples > ctx->Const.MaxColorTextureSamples
               ? GL_INVALID_OPERATION : GL_NO_ERROR;
      }
   }

   /* No more specific limit is available, so just use MAX_SAMPLES:
    *
    * On p205 of the GL3.1 spec:
    *
    * "... or if samples is greater than MAX_SAMPLES, then the error
    * INVALID_VALUE is generated"
    */
   return (GLuint) samples > ctx->Const.MaxSamples
      ? GL_INVALID_VALUE : GL_NO_ERROR;
}
