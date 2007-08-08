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

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_depthstencil.h"
#include "intel_fbo.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"
#include "context.h"
#include "utils.h"
#include "drirenderbuffer.h"
#include "framebuffer.h"
#include "vblank.h"

#include "pipe/p_context.h"



/**
 * Update the following fields for rendering:
 *   intel->numClipRects
 *   intel->pClipRects
 */
static void
intelSetRenderbufferClipRects(struct intel_context *intel)
{
   /* zero-sized buffers might be legal? */
   assert(intel->ctx.DrawBuffer->Width > 0);
   assert(intel->ctx.DrawBuffer->Height > 0);
   intel->fboRect.x1 = 0;
   intel->fboRect.y1 = 0;
   intel->fboRect.x2 = intel->ctx.DrawBuffer->Width;
   intel->fboRect.y2 = intel->ctx.DrawBuffer->Height;
   intel->numClipRects = 1;
   intel->pClipRects = &intel->fboRect;
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
   struct pipe_region *colorRegion, *depthRegion = NULL;
   struct intel_renderbuffer *irbDepth = NULL, *irbStencil = NULL;

   if (!fb) {
      /* this can happen during the initial context initialization */
      return;
   }

   /* Do this here, not core Mesa, since this function is called from
    * many places within the driver.
    */
   if (ctx->NewState & (_NEW_BUFFERS | _NEW_COLOR | _NEW_PIXEL)) {
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

   if (fb->Name)
      intel_validate_paired_depth_stencil(ctx, fb);

   /*
    * How many color buffers are we drawing into?
    */
   if (fb->_NumColorDrawBuffers[0] != 1) {
      /* writing to 0 or 2 or 4 color buffers */
      /*_mesa_debug(ctx, "Software rendering\n");*/
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_TRUE);
   }
   else {
      /* draw to exactly one color buffer */
      /*_mesa_debug(ctx, "Hardware rendering\n");*/
      FALLBACK(intel, INTEL_FALLBACK_DRAW_BUFFER, GL_FALSE);
   }

   /*
    * Get the intel_renderbuffer for the colorbuffer we're drawing into.
    * And set up cliprects.
    */
   {
      struct intel_renderbuffer *irb;
      intelSetRenderbufferClipRects(intel);
      irb = intel_renderbuffer(fb->_ColorDrawBuffers[0][0]);
      colorRegion = (irb && irb->region) ? irb->region : NULL;
   }

   /* Update culling direction which changes depending on the
    * orientation of the buffer:
    */
   if (ctx->Driver.FrontFace)
      ctx->Driver.FrontFace(ctx, ctx->Polygon.FrontFace);
   else
      ctx->NewState |= _NEW_POLYGON;

   if (!colorRegion) {
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
    *** combined DEPTH_STENCIL buffer (for now anyway).
    ***/
   if (fb->_StencilBuffer && fb->_StencilBuffer->Wrapped) {
      irbStencil = intel_renderbuffer(fb->_StencilBuffer->Wrapped);
      if (irbStencil && irbStencil->region) {
         ASSERT(irbStencil->Base._ActualFormat == GL_DEPTH24_STENCIL8_EXT);
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
         /* need to re-compute stencil hw state */
//         ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
         if (!depthRegion)
            depthRegion = irbStencil->region;
      }
      else {
         FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_TRUE);
      }
   }
   else {
      /* XXX FBO: instead of FALSE, pass ctx->Stencil.Enabled ??? */
      FALLBACK(intel, INTEL_FALLBACK_STENCIL_BUFFER, GL_FALSE);
      /* need to re-compute stencil hw state */
//      ctx->Driver.Enable(ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled);
   }


   /**
    ** Release old regions, reference new regions
    **/

   //  intel->vtbl.set_draw_region(intel, colorRegion, depthRegion);

   /* update viewport since it depends on window size */
//   ctx->Driver.Viewport(ctx, ctx->Viewport.X, ctx->Viewport.Y,
//                        ctx->Viewport.Width, ctx->Viewport.Height);

   /* Update hardware scissor */
//   ctx->Driver.Scissor(ctx, ctx->Scissor.X, ctx->Scissor.Y,
//                       ctx->Scissor.Width, ctx->Scissor.Height);
}


static void
intelDrawBuffer(GLcontext * ctx, GLenum mode)
{
   intel_draw_buffer(ctx, ctx->DrawBuffer);
}


static void
intelReadBuffer(GLcontext * ctx, GLenum mode)
{
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
