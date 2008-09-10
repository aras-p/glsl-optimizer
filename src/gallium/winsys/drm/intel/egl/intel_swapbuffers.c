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

#include "intel_device.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

#include "pipe/p_context.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_cb_fbo.h"
#include "intel_egl.h"


static void
intel_display_surface(struct egl_drm_drawable *draw,
                      struct pipe_surface *surf);

void intel_swap_buffers(struct egl_drm_drawable *draw)
{
	struct intel_framebuffer *intel_fb = (struct intel_framebuffer *)draw->priv;
	struct pipe_surface *back_surf;

	assert(intel_fb);
	assert(intel_fb->stfb);

	back_surf = st_get_framebuffer_surface(intel_fb->stfb, ST_SURFACE_BACK_LEFT);
	if (back_surf) {
		st_notify_swapbuffers(intel_fb->stfb);
		if (intel_fb->front)
			intel_display_surface(draw, back_surf);
		st_notify_swapbuffers_complete(intel_fb->stfb);
	}
}

static void
intel_display_surface(struct egl_drm_drawable *draw,
                      struct pipe_surface *surf)
{
	struct intel_context *intel = NULL;
	struct intel_framebuffer *intel_fb = (struct intel_framebuffer *)draw->priv;
	struct _DriFenceObject *fence;

	//const int srcWidth = surf->width;
	//const int srcHeight = surf->height;

	intel = intel_fb->device->dummy;
	if (!intel) {
		printf("No dummy context\n");
		return;
	}

	const int dstWidth = intel_fb->front->width;
	const int dstHeight = intel_fb->front->height;
	const int dstPitch = intel_fb->front->pitch / 4;//draw->front.cpp;

	const int cpp = 4;//intel_fb->front->cpp;
	const int srcPitch = surf->stride / cpp;

	int BR13, CMD;
	//int i;

	BR13 = (dstPitch * cpp) | (0xCC << 16) | (1 << 24) | (1 << 25);
	CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
			XY_SRC_COPY_BLT_WRITE_RGB);

	BEGIN_BATCH(8, 2);
	OUT_BATCH(CMD);
	OUT_BATCH(BR13);
	OUT_BATCH((0 << 16) | 0);
	OUT_BATCH((dstHeight << 16) | dstWidth);

	OUT_RELOC(intel_fb->front_buffer,
		DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_WRITE,
		DRM_BO_MASK_MEM | DRM_BO_FLAG_WRITE, 0);

	OUT_BATCH((0 << 16) | 0);
	OUT_BATCH((srcPitch * cpp) & 0xffff);
	OUT_RELOC(dri_bo(surf->buffer),
			DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
			DRM_BO_MASK_MEM | DRM_BO_FLAG_READ, 0);

	fence = intel_be_batchbuffer_flush(intel->base.batch);
	driFenceUnReference(&fence);
	intel_be_batchbuffer_finish(intel->base.batch);
}
