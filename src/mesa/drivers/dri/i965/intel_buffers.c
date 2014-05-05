/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "brw_context.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"


bool
brw_is_front_buffer_reading(struct gl_framebuffer *fb)
{
   if (!fb || _mesa_is_user_fbo(fb))
      return false;

   return fb->_ColorReadBufferIndex == BUFFER_FRONT_LEFT;
}

bool
brw_is_front_buffer_drawing(struct gl_framebuffer *fb)
{
   if (!fb || _mesa_is_user_fbo(fb))
      return false;

   return (fb->_NumColorDrawBuffers >= 1 &&
           fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT);
}

static void
intelDrawBuffer(struct gl_context * ctx, GLenum mode)
{
   if (brw_is_front_buffer_drawing(ctx->DrawBuffer)) {
      struct brw_context *const brw = brw_context(ctx);

      /* If we might be front-buffer rendering on this buffer for the first
       * time, invalidate our DRI drawable so we'll ask for new buffers
       * (including the fake front) before we start rendering again.
       */
      dri2InvalidateDrawable(brw->driContext->driDrawablePriv);
      intel_prepare_render(brw);
   }
}


static void
intelReadBuffer(struct gl_context * ctx, GLenum mode)
{
   if (brw_is_front_buffer_reading(ctx->ReadBuffer)) {
      struct brw_context *const brw = brw_context(ctx);

      /* If we might be front-buffer reading on this buffer for the first
       * time, invalidate our DRI drawable so we'll ask for new buffers
       * (including the fake front) before we start reading again.
       */
      dri2InvalidateDrawable(brw->driContext->driReadablePriv);
      intel_prepare_render(brw);
   }
}


void
intelInitBufferFuncs(struct dd_function_table *functions)
{
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}
