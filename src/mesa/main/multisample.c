/*
 * Mesa 3-D graphics library
 * Version:  7.1
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "main/glheader.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/multisample.h"
#include "main/mtypes.h"
#include "main/fbobject.h"


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

   switch (pname) {
   case GL_SAMPLE_POSITION: {
      if (index >= ctx->DrawBuffer->Visual.samples) {
         _mesa_error( ctx, GL_INVALID_VALUE, "glGetMultisamplefv(index)" );
         return;
      }

      ctx->Driver.GetSamplePosition(ctx, ctx->DrawBuffer, index, val);

      /* winsys FBOs are upside down */
      if (_mesa_is_winsys_fbo(ctx->DrawBuffer))
         val[1] = 1 - val[1];

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
