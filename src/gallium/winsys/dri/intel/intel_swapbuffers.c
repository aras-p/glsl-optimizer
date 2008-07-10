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
#include "intel_swapbuffers.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "intel_drm/ws_dri_bufmgr.h"


/**
 * Display a colorbuffer surface in an X window.
 * Used for SwapBuffers and flushing front buffer rendering.
 *
 * \param dPriv  the window/drawable to display into
 * \param surf  the surface to display
 * \param rect  optional subrect of surface to display (may be NULL).
 */
void
intelDisplaySurface(__DRIdrawablePrivate *dPriv,
		struct pipe_texture *tex,
		const drm_clip_rect_t *rect)
{
	struct intel_screen *intelScreen = intel_screen(dPriv->driScreenPriv);
	struct intel_context *intel = intelScreen->dummyContext;

	DBG(SWAP, "%s\n", __FUNCTION__);

	if (!intel) {
		/* XXX this is where some kind of extra/meta context could be useful */
		return;
	} else if (!intel->blit) {
		intel->blit = util_create_blit(intel->st->pipe, intel->st->cso_context);
	}

	if (intel->last_swap_fence) {
		driFenceFinish(intel->last_swap_fence, DRM_FENCE_TYPE_EXE, TRUE);
		driFenceUnReference(&intel->last_swap_fence);
		intel->last_swap_fence = NULL;
	}
	intel->last_swap_fence = intel->first_swap_fence;
	intel->first_swap_fence = NULL;

	/* The LOCK_HARDWARE is required for the cliprects.  Buffer offsets
	 * should work regardless.
	 */
	LOCK_HARDWARE(intel);
	/* if this drawable isn't currently bound the LOCK_HARDWARE done on the
	 * current context (which is what intelScreenContext should return) might
	 * not get a contended lock and thus cliprects not updated (tests/manywin)
	 */
	if (intel_context(dPriv->driContextPriv) != intel)
		DRI_VALIDATE_DRAWABLE_INFO(intel->driScreen, dPriv);

	if (dPriv && dPriv->numClipRects) {
		const int srcWidth = tex->width[0];
		const int srcHeight = tex->height[0];
		const int nbox = dPriv->numClipRects;
		const drm_clip_rect_t *pbox = dPriv->pClipRects;
		int i;

		assert(surf->buffer);
		assert(surf->cpp == cpp);


		for (i = 0; i < nbox; i++, pbox++) {
			drm_clip_rect_t box;
			drm_clip_rect_t sbox;

			if (pbox->x1 > pbox->x2 ||
					pbox->y1 > pbox->y2 ||
					pbox->x2 > intelScreen->front.width ||
					pbox->y2 > intelScreen->front.height) {
				/* invalid cliprect, skip it */
				continue;
			}

			box = *pbox;

			if (rect) {
				/* intersect cliprect with user-provided src rect */
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
			if (box.x2 - box.x1 > srcWidth)
				box.x2 = srcWidth + box.x1;
			if (box.y2 - box.y1 > srcHeight)
				box.y2 = srcHeight + box.y1;

			DBG(SWAP, "\tdbox x1 x2 y1 y2 %d %d %d %d\n",
					box.x1, box.x2, box.y1, box.y2);

			sbox.x1 = box.x1 - dPriv->x;
			sbox.y1 = box.y1 - dPriv->y;
			sbox.x2 = box.x2 - dPriv->x;
			sbox.y2 = box.y2 - dPriv->y;
			DBG(SWAP, "\tsbox x1 x2 y1 y2 %d %d %d %d\n",
					sbox.x1, sbox.x2, sbox.y1, sbox.y2);

			assert(box.x1 < box.x2);
			assert(box.y1 < box.y2);

			util_blit_pixels_tex(intel->blit,
					tex,
					sbox.x1, sbox.y1,
					sbox.x2, sbox.y2,
					intelScreen->front.surface,
					box.x1, box.y1,
					box.x2, box.y2,
					0.0,
					PIPE_TEX_MIPFILTER_NEAREST);
		}

		if (intel->first_swap_fence)
			driFenceUnReference(&intel->first_swap_fence);
		intel->first_swap_fence = intel_be_batchbuffer_flush(intel->base.batch);
	}


	UNLOCK_HARDWARE(intel);

	if (intel->lastStamp != dPriv->lastStamp) {
		intelUpdateWindowSize(dPriv);
		intel->lastStamp = dPriv->lastStamp;
	}
}

/**
 * This will be called whenever the currently bound window is moved/resized.
 */
void
intelUpdateWindowSize(__DRIdrawablePrivate *dPriv)
{
   struct intel_framebuffer *intelfb = intel_framebuffer(dPriv);
   assert(intelfb->stfb);
   st_resize_framebuffer(intelfb->stfb, dPriv->w, dPriv->h);
}

/**
 * Called from glx code
 */
void
intelSwapBuffers(__DRIdrawablePrivate * dPriv)
{
   struct intel_framebuffer *intel_fb = intel_framebuffer(dPriv);
   struct pipe_texture *back_tex;

   assert(intel_fb);
   assert(intel_fb->stfb);

   back_tex = st_get_framebuffer_texture(intel_fb->stfb,
                                         ST_SURFACE_BACK_LEFT);
   if (back_tex) {
      st_notify_swapbuffers(intel_fb->stfb);
      intelDisplaySurface(dPriv, back_tex, NULL);
      st_notify_swapbuffers_complete(intel_fb->stfb);
   }
}

/**
 * Called via glXCopySubBufferMESA() to copy a subrect of the back
 * buffer to the front buffer/screen.
 */
void
intelCopySubBuffer(__DRIdrawablePrivate * dPriv, int x, int y, int w, int h)
{
   struct intel_framebuffer *intel_fb = intel_framebuffer(dPriv);
   struct pipe_texture *back_tex;

   assert(intel_fb);
   assert(intel_fb->stfb);

   back_tex = st_get_framebuffer_texture(intel_fb->stfb,
                                         ST_SURFACE_BACK_LEFT);
   if (back_tex) {
      drm_clip_rect_t rect;
      rect.x1 = x;
      rect.y1 = y;
      rect.x2 = w;
      rect.y2 = h;

      st_notify_swapbuffers(intel_fb->stfb);
      intelDisplaySurface(dPriv, back_tex, &rect);
   }
}
