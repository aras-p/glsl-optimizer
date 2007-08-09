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


#include "mtypes.h"
#include "context.h"
#include "enums.h"

#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_context.h"
#include "intel_reg.h"
#include "vblank.h"

#include "pipe/p_context.h"
#include "state_tracker/st_cb_fbo.h"


#define FILE_DEBUG_FLAG DEBUG_BLIT

/**
 * Copy the back color buffer to the front color buffer. 
 * Used for SwapBuffers().
 */
void
intelCopyBuffer(__DRIdrawablePrivate * dPriv,
                const drm_clip_rect_t * rect)
{

   struct intel_context *intel;
   const intelScreenPrivate *intelScreen;

   DBG("%s\n", __FUNCTION__);

   assert(dPriv);

   intel = intelScreenContext(dPriv->driScreenPriv->private);
   if (!intel)
      return;

   intelScreen = intel->intelScreen;

   if (intel->last_swap_fence) {
      driFenceFinish(intel->last_swap_fence, DRM_FENCE_TYPE_EXE, GL_TRUE);
      driFenceUnReference(intel->last_swap_fence);
      intel->last_swap_fence = NULL;
   }
   intel->last_swap_fence = intel->first_swap_fence;
   intel->first_swap_fence = NULL;

   /* The LOCK_HARDWARE is required for the cliprects.  Buffer offsets
    * should work regardless.
    */
   LOCK_HARDWARE(intel);
   /* if this drawable isn't currently bound the LOCK_HARDWARE done on the
      current context (which is what intelScreenContext should return) might
      not get a contended lock and thus cliprects not updated (tests/manywin) */
      if ((struct intel_context *)dPriv->driContextPriv->driverPrivate != intel)
         DRI_VALIDATE_DRAWABLE_INFO(intel->driScreen, dPriv);


   if (dPriv && dPriv->numClipRects) {
      struct intel_framebuffer *intel_fb = dPriv->driverPrivate;
#if 0
      const struct pipe_region *backRegion
	 = intel_fb->Base._ColorDrawBufferMask[0] == BUFFER_BIT_FRONT_LEFT ?
	   intel_get_rb_region(&intel_fb->Base, BUFFER_FRONT_LEFT) :
	   intel_get_rb_region(&intel_fb->Base, BUFFER_BACK_LEFT);
#endif
      const int backWidth = intel_fb->Base.Width;
      const int backHeight = intel_fb->Base.Height;
      const int nbox = dPriv->numClipRects;
      const drm_clip_rect_t *pbox = dPriv->pClipRects;
      const int pitch = intelScreen->front.pitch / intelScreen->front.cpp;
#if 0
      const int srcpitch = backRegion->pitch;
#endif
      const int cpp = intelScreen->front.cpp;
      int BR13, CMD;
      int i;

      const struct pipe_surface *backSurf;
      const struct pipe_region *backRegion;
      int srcpitch;
      struct st_renderbuffer *strb;

      /* blit from back color buffer if it exists, else front buffer */
      strb = st_renderbuffer(intel_fb->Base.Attachment[BUFFER_BACK_LEFT].Renderbuffer);
      if (strb) {
         backSurf = strb->surface;
      }
      else {
         strb = st_renderbuffer(intel_fb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
         backSurf = strb->surface;
      }

      backRegion = backSurf->region;
      srcpitch = backRegion->pitch;

      ASSERT(intel_fb);
      ASSERT(intel_fb->Base.Name == 0);    /* Not a user-created FBO */
      ASSERT(backRegion);
      ASSERT(backRegion->cpp == cpp);

      DBG("front pitch %d back pitch %d\n",
	 pitch, backRegion->pitch);

      if (cpp == 2) {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1 << 24);
	 CMD = XY_SRC_COPY_BLT_CMD;
      }
      else {
	 BR13 = (pitch * cpp) | (0xCC << 16) | (1 << 24) | (1 << 25);
	 CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
		XY_SRC_COPY_BLT_WRITE_RGB);
      }

      for (i = 0; i < nbox; i++, pbox++) {
	 drm_clip_rect_t box;
	 drm_clip_rect_t sbox;

	 if (pbox->x1 > pbox->x2 ||
	     pbox->y1 > pbox->y2 ||
	     pbox->x2 > intelScreen->front.width || 
	     pbox->y2 > intelScreen->front.height)
	    continue;

	 box = *pbox;

	 if (rect) {
	    drm_clip_rect_t rrect;

	    rrect.x1 = dPriv->x + rect->x1;
	    rrect.y1 = (dPriv->h - rect->y1 - rect->y2) + dPriv->y;
	    rrect.x2 = rect->x2 + rrect.x1;
	    rrect.y2 = rect->y2 + rrect.y1;
	    if (rrect.x1 > box.x1)
	       box.x1 = rrect.x1;
	    if (rrect.y1 > box.y1)
	       box.y1 = rrect.y1;
	    if (rrect.x2 < box.x2)
	       box.x2 = rrect.x2;
	    if (rrect.y2 < box.y2)
	       box.y2 = rrect.y2;

	    if (box.x1 > box.x2 || box.y1 > box.y2)
	       continue;
	 }

	 /* restrict blit to size of actually rendered area */
	 if (box.x2 - box.x1 > backWidth)
	    box.x2 = backWidth + box.x1;
	 if (box.y2 - box.y1 > backHeight)
	    box.y2 = backHeight + box.y1;

	 DBG("box x1 x2 y1 y2 %d %d %d %d\n",
	      box.x1, box.x2, box.y1, box.y2);

	 sbox.x1 = box.x1 - dPriv->x;
	 sbox.y1 = box.y1 - dPriv->y;

	 BEGIN_BATCH(8, INTEL_BATCH_NO_CLIPRECTS);
	 OUT_BATCH(CMD);
	 OUT_BATCH(BR13);
	 OUT_BATCH((box.y1 << 16) | box.x1);
	 OUT_BATCH((box.y2 << 16) | box.x2);

	 OUT_RELOC(intelScreen->front.buffer, 
		   DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
		   DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE, 0);
	 OUT_BATCH((sbox.y1 << 16) | sbox.x1);
	 OUT_BATCH((srcpitch * cpp) & 0xffff);
	 OUT_RELOC(backRegion->buffer, DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
		   DRM_BO_MASK_MEM | DRM_BO_FLAG_READ, 0);

	 ADVANCE_BATCH();
      }

      if (intel->first_swap_fence)
	 driFenceUnReference(intel->first_swap_fence);
      intel->first_swap_fence = intel_batchbuffer_flush(intel->batch);
      driFenceReference(intel->first_swap_fence);
   }

   UNLOCK_HARDWARE(intel);

   /* XXX this is bogus. The context here may not even be bound to this drawable! */
   if (intel->lastStamp != dPriv->lastStamp) {
      GET_CURRENT_CONTEXT(currctx);
      struct intel_context *intelcurrent = intel_context(currctx);
      if (intelcurrent == intel && intelcurrent->driDrawable == dPriv) {
         intelWindowMoved(intel);
         intel->lastStamp = dPriv->lastStamp;
      }
   }

}







