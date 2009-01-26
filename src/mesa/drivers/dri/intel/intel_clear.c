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

#include "intel_clear.h"
#include "intel_context.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_chipset.h"
#include "intel_fbo.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "main/framebuffer.h"
#include "swrast/swrast.h"
#include "drirenderbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

/* A true meta version of this would be very simple and additionally
 * machine independent.  Maybe we'll get there one day.
 */
static void
intelClearWithTris(struct intel_context *intel, GLbitfield mask)
{
   GLcontext *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint buf;

   intel->vtbl.install_meta_state(intel);

   /* Back and stencil cliprects are the same.  Try and do both
    * buffers at once:
    */
   if (mask & (BUFFER_BIT_BACK_LEFT | BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH)) {
      struct intel_region *backRegion =
	 intel_get_rb_region(fb, BUFFER_BACK_LEFT);
      struct intel_region *depthRegion =
	 intel_get_rb_region(fb, BUFFER_DEPTH);

      intel->vtbl.meta_draw_region(intel, backRegion, depthRegion);

      if (mask & BUFFER_BIT_BACK_LEFT)
	 intel->vtbl.meta_color_mask(intel, GL_TRUE);
      else
	 intel->vtbl.meta_color_mask(intel, GL_FALSE);

      if (mask & BUFFER_BIT_STENCIL)
	 intel->vtbl.meta_stencil_replace(intel,
					  intel->ctx.Stencil.WriteMask[0],
					  intel->ctx.Stencil.Clear);
      else
	 intel->vtbl.meta_no_stencil_write(intel);

      if (mask & BUFFER_BIT_DEPTH)
	 intel->vtbl.meta_depth_replace(intel);
      else
	 intel->vtbl.meta_no_depth_write(intel);

      intel->vtbl.meta_draw_quad(intel,
				 fb->_Xmin,
				 fb->_Xmax,
				 fb->_Ymin,
				 fb->_Ymax,
				 intel->ctx.Depth.Clear,
				 intel->ClearColor8888,
				 0, 0, 0, 0);   /* texcoords */

      mask &= ~(BUFFER_BIT_BACK_LEFT | BUFFER_BIT_STENCIL | BUFFER_BIT_DEPTH);
   }

   /* clear the remaining (color) renderbuffers */
   for (buf = 0; buf < BUFFER_COUNT && mask; buf++) {
      const GLuint bufBit = 1 << buf;
      if (mask & bufBit) {
	 struct intel_renderbuffer *irbColor =
	    intel_renderbuffer(fb->Attachment[buf].Renderbuffer);

	 ASSERT(irbColor);

	 intel->vtbl.meta_no_depth_write(intel);
	 intel->vtbl.meta_no_stencil_write(intel);
	 intel->vtbl.meta_color_mask(intel, GL_TRUE);
	 intel->vtbl.meta_draw_region(intel, irbColor->region, NULL);

	 intel->vtbl.meta_draw_quad(intel,
				    fb->_Xmin,
				    fb->_Xmax,
				    fb->_Ymin,
				    fb->_Ymax,
				    0, intel->ClearColor8888,
				    0, 0, 0, 0);   /* texcoords */

	 mask &= ~bufBit;
      }
   }

   intel->vtbl.leave_meta_state(intel);
}

static const char *buffer_names[] = {
   [BUFFER_FRONT_LEFT] = "front",
   [BUFFER_BACK_LEFT] = "back",
   [BUFFER_FRONT_RIGHT] = "front right",
   [BUFFER_BACK_RIGHT] = "back right",
   [BUFFER_AUX0] = "aux0",
   [BUFFER_AUX1] = "aux1",
   [BUFFER_AUX2] = "aux2",
   [BUFFER_AUX3] = "aux3",
   [BUFFER_DEPTH] = "depth",
   [BUFFER_STENCIL] = "stencil",
   [BUFFER_ACCUM] = "accum",
   [BUFFER_COLOR0] = "color0",
   [BUFFER_COLOR1] = "color1",
   [BUFFER_COLOR2] = "color2",
   [BUFFER_COLOR3] = "color3",
   [BUFFER_COLOR4] = "color4",
   [BUFFER_COLOR5] = "color5",
   [BUFFER_COLOR6] = "color6",
   [BUFFER_COLOR7] = "color7",
};

/**
 * Called by ctx->Driver.Clear.
 */
static void
intelClear(GLcontext *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   const GLuint colorMask = *((GLuint *) & ctx->Color.ColorMask);
   GLbitfield tri_mask = 0;
   GLbitfield blit_mask = 0;
   GLbitfield swrast_mask = 0;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint i;

   if (0)
      fprintf(stderr, "%s\n", __FUNCTION__);

   /* HW color buffers (front, back, aux, generic FBO, etc) */
   if (colorMask == ~0) {
      /* clear all R,G,B,A */
      /* XXX FBO: need to check if colorbuffers are software RBOs! */
      blit_mask |= (mask & BUFFER_BITS_COLOR);
   }
   else {
      /* glColorMask in effect */
      tri_mask |= (mask & BUFFER_BITS_COLOR);
   }

   /* HW stencil */
   if (mask & BUFFER_BIT_STENCIL) {
      const struct intel_region *stencilRegion
         = intel_get_rb_region(fb, BUFFER_STENCIL);
      if (stencilRegion) {
         /* have hw stencil */
         if (IS_965(intel->intelScreen->deviceID) ||
	     (ctx->Stencil.WriteMask[0] & 0xff) != 0xff) {
	    /* We have to use the 3D engine if we're clearing a partial mask
	     * of the stencil buffer, or if we're on a 965 which has a tiled
	     * depth/stencil buffer in a layout we can't blit to.
	     */
            tri_mask |= BUFFER_BIT_STENCIL;
         }
         else {
            /* clearing all stencil bits, use blitting */
            blit_mask |= BUFFER_BIT_STENCIL;
         }
      }
   }

   /* HW depth */
   if (mask & BUFFER_BIT_DEPTH) {
      /* clear depth with whatever method is used for stencil (see above) */
      if (IS_965(intel->intelScreen->deviceID) ||
	  tri_mask & BUFFER_BIT_STENCIL)
         tri_mask |= BUFFER_BIT_DEPTH;
      else
         blit_mask |= BUFFER_BIT_DEPTH;
   }

   /* SW fallback clearing */
   swrast_mask = mask & ~tri_mask & ~blit_mask;

   for (i = 0; i < BUFFER_COUNT; i++) {
      GLuint bufBit = 1 << i;
      if ((blit_mask | tri_mask) & bufBit) {
         if (!fb->Attachment[i].Renderbuffer->ClassID) {
            blit_mask &= ~bufBit;
            tri_mask &= ~bufBit;
            swrast_mask |= bufBit;
         }
      }
   }

   if (blit_mask) {
      if (INTEL_DEBUG & DEBUG_BLIT) {
	 DBG("blit clear:");
	 for (i = 0; i < BUFFER_COUNT; i++) {
	    if (blit_mask & (1 << i))
	       DBG(" %s", buffer_names[i]);
	 }
	 DBG("\n");
      }
      intelClearWithBlit(ctx, blit_mask);
   }

   if (tri_mask) {
      if (INTEL_DEBUG & DEBUG_BLIT) {
	 DBG("tri clear:");
	 for (i = 0; i < BUFFER_COUNT; i++) {
	    if (tri_mask & (1 << i))
	       DBG(" %s", buffer_names[i]);
	 }
	 DBG("\n");
      }
      intelClearWithTris(intel, tri_mask);
   }

   if (swrast_mask) {
      if (INTEL_DEBUG & DEBUG_BLIT) {
	 DBG("swrast clear:");
	 for (i = 0; i < BUFFER_COUNT; i++) {
	    if (swrast_mask & (1 << i))
	       DBG(" %s", buffer_names[i]);
	 }
	 DBG("\n");
      }
      _swrast_Clear(ctx, swrast_mask);
   }
}


void
intelInitClearFuncs(struct dd_function_table *functions)
{
   functions->Clear = intelClear;
}
