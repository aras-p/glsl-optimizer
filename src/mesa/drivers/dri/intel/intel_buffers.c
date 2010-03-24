/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "intel_context.h"
#include "intel_buffers.h"
#include "intel_fbo.h"
#include "main/framebuffer.h"

/**
 * Return pointer to current color drawing region, or NULL.
 */
struct intel_region *
intel_drawbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irbColor =
      intel_renderbuffer(intel->ctx.DrawBuffer->_ColorDrawBuffers[0]);
   if (irbColor)
      return irbColor->region;
   else
      return NULL;
}

/**
 * Return pointer to current color reading region, or NULL.
 */
struct intel_region *
intel_readbuf_region(struct intel_context *intel)
{
   struct intel_renderbuffer *irb
      = intel_renderbuffer(intel->ctx.ReadBuffer->_ColorReadBuffer);
   if (irb)
      return irb->region;
   else
      return NULL;
}

/**
 * Check if we're about to draw into the front color buffer.
 * If so, set the intel->front_buffer_dirty field to true.
 */
void
intel_check_front_buffer_rendering(struct intel_context *intel)
{
   const struct gl_framebuffer *fb = intel->ctx.DrawBuffer;
   if (fb->Name == 0) {
      /* drawing to window system buffer */
      if (fb->_NumColorDrawBuffers > 0) {
         if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
	    intel->front_buffer_dirty = GL_TRUE;
	 }
      }
   }
}


/**
 * Update the hardware state for drawing into a window or framebuffer object.
 *
 * Called by glDrawBuffer, glBindFramebufferEXT, MakeCurrent, and other
 * places within the driver.
 *
 * Basically, this needs to be called any time the current framebuffer
 * changes, the renderbuffers change, or we need to draw into different
 * color buffers.
 */
void
intel_draw_buffer(GLcontext * ctx, struct gl_framebuffer *fb)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *colorRegions[MAX_DRAW_BUFFERS], *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /* Do this here, not core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & _NEW_BUFFERS) {
      /* this updates the DrawBuffer->_NumColorDrawBuffers fields, etc */
      _mesa_update_framebuffer(ctx);
      /* this updates the DrawBuffer's Width/Height if it's a FBO */
      _mesa_update_draw_buffer_bounds(ctx);
   }

   if (fb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      /* this may occur when we're called by glBindFrameBuffer() during
       * the process of someone setting up renderbuffers, etc.
       */
      /*_mesa_debug(ctx, "DrawBuffer: incomplete user FBO\n");*/
      return;
   }

   /* How many color buffers are we drawing into?
    *
    * If there are zero buffers or the buffer is too big, don't configure any
    * regions for hardware drawing.  We'll fallback to software below.  Not
    * having regions set makes some of the software fallback paths faster.
    */
   if ((fb->Width > ctx->Const.MaxRenderbufferSize)
       || (fb->Height > ctx->Const.MaxRenderbufferSize)
       || (fb->_NumColorDrawBuffers == 0)) {
      /* writing to 0  */
      colorRegions[0] = NULL;
   }
   else if (fb->_NumColorDrawBuffers > 1) {
       int i;
       struct intel_renderbuffer *irb;

       for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
           irb = intel_renderbuffer(fb->_ColorDrawBuffers[i]);
           colorRegions[i] = irb ? irb->region : NULL;
       }
   }
   else {
      /* Get the intel_renderbuffer for the single colorbuffer we're drawing
       * into.
       */
      if (fb->Name == 0) {
	 /* drawing to window system buffer */
	 if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT)
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_FRONT_LEFT);
	 else
	    colorRegions[0] = intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      }
      else {
	 /* drawing to user-created FBO */
	 struct intel_renderbuffer *irb;
	 irb = intel_renderbuffer(fb->_ColorDrawBuffers[0]);
	 colorRegions[0] = (irb && irb->region) ? irb->region : NULL;
      }
   }

   if (!colorRegions[0]) {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
   }
   else {
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
   }

   /***
    *** Get depth buffer region and check if we need a software fallback.
    *** Note that the depth buffer is usually a DEPTH_STENCIL buffer.
    ***/
   if (fb->_DepthBuffer && fb->_DepthBuffer->Wrapped) {
      irbDepth = intel_renderbuffer(fb->_DepthBuffer->Wrapped);
      if (irbDepth && irbDepth->region) {
         FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
         depthRegion = irbDepth->region;
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_TRUE);
         depthRegion = NULL;
      }
   }
   else {
      /* not using depth buffer */
      FALLBACK(intel, INTEL_FALLBACK_DEPTH_BUFFER, GL_FALSE);
      depthRegion = NULL;
   }

   /***
    *** Stencil buffer
    *** This can only be hardware accelerated if we're using a
    *** combined DEPTH_STENCIL buffer.
    ***/
   if (fb->_StencilBuffer && fb->_StencilBuffer->Wrapped) {
      irbStencil = intel_renderbuffer(fb->_StencilBuffer->Wrapped);
      if (irbStencil && irbStencil->region) {
         ASSERT(irbStencil->Base.Format == MESA_FORMAT_S8_Z24);
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_TRUE);
      }
   }
   else {
      /* XXX FBO: instead of FALSE, pass ctx->Stencil._Enabled ??? */
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
   }

   /* If we have a (packed) stencil buffer attached but no depth buffer,
    * we still need to set up the shared depth/stencil state so we can use it.
    */
   if (depthRegion == NULL && irbStencil && irbStencil->region)
      depthRegion = irbStencil->region;

   /*
    * Update depth and stencil test state
    */
   if (ctx->Driver.Enable) {
      ctx->Driver.Enable(ctx, GL_DEPTH_TEST,
                         (ctx->Depth.Test && fb->Visual.depthBits > 0));
      ctx->Driver.Enable(ctx, GL_STENCIL_TEST,
                         (ctx->Stencil.Enabled && fb->Visual.stencilBits > 0));
   }
   else {
      /* Mesa's Stencil._Enabled field is updated when
       * _NEW_BUFFERS | _NEW_STENCIL, but i965 code assumes that the value
       * only changes with _NEW_STENCIL (which seems sensible).  So flag it
       * here since this is the _NEW_BUFFERS path.
       */
      intel->NewGLState |= (_NEW_DEPTH | _NEW_STENCIL);
   }

   intel->vtbl.set_draw_region(intel, colorRegions, depthRegion, 
                               fb->_NumColorDrawBuffers);

   /* update viewport since it depends on window size */
#ifdef I915
   intelCalcViewport(ctx);
#else
   intel->NewGLState |= _NEW_VIEWPORT;
#endif
   /* Set state we know depends on drawable parameters:
    */
   if (ctx->Driver.Scissor)
      ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
			  ctx->Scissor.Width, ctx->Scissor.Height);
   intel->NewGLState |= _NEW_SCISSOR;

   if (ctx->Driver.DepthRange)
      ctx->Driver.DepthRange(ctx,
			     ctx->Viewport.Near,
			     ctx->Viewport.Far);

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
   else
      intel->NewGLState |= _NEW_POLYGON;
}


static void
intelDrawBuffer(GLcontext * ctx, GLenum mode)
{
   if ((ctx->DrawBuffer != NULL) && (ctx->DrawBuffer->Name == 0)) {
      struct intel_context *const intel = intel_context(ctx);
      const GLboolean was_front_buffer_rendering =
	intel->is_front_buffer_rendering;

      intel->is_front_buffer_rendering = (mode == GL_FRONT_LEFT)
	|| (mode == GL_FRONT);

      /* If we weren't front-buffer rendering before but we are now,
       * invalidate our DRI drawable so we'll ask for new buffers
       * (including the fake front) before we start rendering again.
       */
      if (!was_front_buffer_rendering && intel->is_front_buffer_rendering)
	 dri2InvalidateDrawable(intel->driContext->driDrawablePriv);
   }

   intel_draw_buffer(ctx, ctx->DrawBuffer);
}


static void
intelReadBuffer(GLcontext * ctx, GLenum mode)
{
   if ((ctx->DrawBuffer != NULL) && (ctx->DrawBuffer->Name == 0)) {
      struct intel_context *const intel = intel_context(ctx);
      const GLboolean was_front_buffer_reading =
	intel->is_front_buffer_reading;

      intel->is_front_buffer_reading = (mode == GL_FRONT_LEFT)
	|| (mode == GL_FRONT);

      /* If we weren't front-buffer reading before but we are now,
       * invalidate our DRI drawable so we'll ask for new buffers
       * (including the fake front) before we start reading again.
       */
      if (!was_front_buffer_reading && intel->is_front_buffer_reading)
	 dri2InvalidateDrawable(intel->driContext->driReadablePriv);
   }

   if (ctx->ReadBuffer == ctx->DrawBuffer) {
      /* This will update FBO completeness status.
       * A framebuffer will be incomplete if the GL_READ_BUFFER setting
       * refers to a missing renderbuffer.  Calling glReadBuffer can set
       * that straight and can make the drawing buffer complete.
       */
      intel_draw_buffer(ctx, ctx->DrawBuffer);
   }
   /* Generally, functions which read pixels (glReadPixels, glCopyPixels, etc)
    * reference ctx->ReadBuffer and do appropriate state checks.
    */
}


void
intelInitBufferFuncs(struct dd_function_table *functions)
{
   functions->DrawBuffer = intelDrawBuffer;
   functions->ReadBuffer = intelReadBuffer;
}
