/*
 * Copyright 2012-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Artur Wyszynski, harakash@gmail.com
 *      Alexander von Gluck IV, kallisti5@unixzen.com
 */


#include "GalliumFramebuffer.h"

extern "C" {
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "pipe/p_format.h"
#include "state_tracker/st_manager.h"
}

#include "GalliumContext.h"


#ifdef DEBUG
#   define TRACE(x...) printf("GalliumFramebuffer: " x)
#   define CALLED() TRACE("CALLED: %s\n", __PRETTY_FUNCTION__)
#else
#   define TRACE(x...)
#   define CALLED()
#endif
#define ERROR(x...) printf("GalliumFramebuffer: " x)


static boolean
hgl_framebuffer_flush_front(struct st_context_iface *stctx,
	struct st_framebuffer_iface* stfb, enum st_attachment_type statt)
{
	CALLED();

	hgl_context* context = (hgl_context*)stfb->st_manager_private;

	if (!context) {
		ERROR("%s: Couldn't obtain valid hgl_context!\n", __func__);
		return FALSE;
	}

	#if 0
	struct stw_st_framebuffer *stwfb = stw_st_framebuffer(stfb);
	pipe_mutex_lock(stwfb->fb->mutex);

	struct pipe_resource* resource = textures[statt];
	if (resource)
		stw_framebuffer_present_locked(...);
	#endif

	return TRUE;
}


static boolean
hgl_framebuffer_validate(struct st_context_iface* stctx,
	struct st_framebuffer_iface* stfb,
	const enum st_attachment_type* statts, unsigned count,
	struct pipe_resource** out)
{
	CALLED();

	if (!stfb) {
		ERROR("%s: Invalid st framebuffer interface!\n", __func__);
		return FALSE;
	}

	hgl_context* context = (hgl_context*)stfb->st_manager_private;

	if (!context) {
		ERROR("%s: Couldn't obtain valid hgl_context!\n", __func__);
		return FALSE;
	}

	int32 width = 0;
	int32 height = 0;
	get_bitmap_size(context->bitmap, &width, &height);

	struct pipe_resource templat;
	memset(&templat, 0, sizeof(templat));
	templat.target = PIPE_TEXTURE_RECT;
	templat.width0 = width;
	templat.height0 = height;
	templat.depth0 = 1;
	templat.array_size = 1;
	templat.usage = PIPE_USAGE_DEFAULT;

	if (context->stVisual && context->manager && context->manager->screen) {
		TRACE("%s: Updating resources\n", __func__);
		int i;
		for (i = 0; i < count; i++) {
			enum pipe_format format = PIPE_FORMAT_NONE;
			unsigned bind = 0;
	
			switch(statts[i]) {
				case ST_ATTACHMENT_FRONT_LEFT:
					format = context->stVisual->color_format;
					bind = PIPE_BIND_RENDER_TARGET;
					break;
				case ST_ATTACHMENT_DEPTH_STENCIL:
					format = context->stVisual->depth_stencil_format;
					bind = PIPE_BIND_DEPTH_STENCIL;
					break;
				case ST_ATTACHMENT_ACCUM:
					format = context->stVisual->accum_format;
					bind = PIPE_BIND_RENDER_TARGET;
					break;
				default:
					ERROR("%s: Unexpected attachment type!\n", __func__);
			}
			templat.format = format;
			templat.bind = bind;

			struct pipe_screen* screen = context->manager->screen;
			context->textures[i] = screen->resource_create(screen, &templat);
			out[i] = context->textures[i];
		}
	}

	return TRUE;
}


GalliumFramebuffer::GalliumFramebuffer(struct st_visual* visual,
	void* privateContext)
	:
	fBuffer(NULL)
{
	CALLED();
	fBuffer = CALLOC_STRUCT(st_framebuffer_iface);
	if (!fBuffer) {
		ERROR("%s: Couldn't calloc framebuffer!\n", __func__);
		return;
	}
	fBuffer->visual = visual;
	fBuffer->flush_front = hgl_framebuffer_flush_front;
	fBuffer->validate = hgl_framebuffer_validate;
	fBuffer->st_manager_private = privateContext;

	pipe_mutex_init(fMutex);
}


GalliumFramebuffer::~GalliumFramebuffer()
{
	CALLED();
	// We lock and unlock to try and make sure we wait for anything
	// using the framebuffer to finish
	Lock();
	if (!fBuffer) {
		ERROR("%s: Strange, no Gallium Framebuffer to free?\n", __func__);
		return;
	}
	FREE(fBuffer);
	Unlock();

	pipe_mutex_destroy(fMutex);
}


status_t
GalliumFramebuffer::Lock()
{
	CALLED();
	pipe_mutex_lock(fMutex);
	return B_OK;
}


status_t
GalliumFramebuffer::Unlock()
{
	CALLED();
	pipe_mutex_unlock(fMutex);
	return B_OK;
}
