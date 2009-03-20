#include <main/glheader.h>
#include <glapi/glthread.h>
#include <GL/internal/glcore.h>

#include <pipe/p_context.h>
#include <state_tracker/st_public.h>
#include <state_tracker/st_context.h>
#include <state_tracker/st_cb_fbo.h>

#include "nouveau_context.h"
#include "nouveau_screen.h"
#include "nouveau_swapbuffers.h"

#include "nouveau_pushbuf.h"

void
nouveau_copy_buffer(__DRIdrawablePrivate *dPriv, struct pipe_surface *surf,
		    const drm_clip_rect_t *rect)
{
	struct nouveau_context *nv = dPriv->driContextPriv->driverPrivate;
	struct nouveau_screen *nv_screen = nv->dri_screen->private;
	struct pipe_context *pipe = nv->st->pipe;
	drm_clip_rect_t *pbox;
	int nbox, i;

	LOCK_HARDWARE(nv);
	if (!dPriv->numClipRects) {
		UNLOCK_HARDWARE(nv);
		return;
	}
	pbox = dPriv->pClipRects;
	nbox = dPriv->numClipRects;

	for (i = 0; i < nbox; i++, pbox++) {
		int sx, sy, dx, dy, w, h;

		sx = pbox->x1 - dPriv->x;
		sy = pbox->y1 - dPriv->y;
		dx = pbox->x1;
		dy = pbox->y1;
		w  = pbox->x2 - pbox->x1;
		h  = pbox->y2 - pbox->y1;

		pipe->surface_copy(pipe, nv_screen->fb, dx, dy, surf,
				   sx, sy, w, h);
	}

	pipe->flush(pipe, 0, NULL);
	UNLOCK_HARDWARE(nv);

	if (nv->last_stamp != dPriv->lastStamp) {
		struct nouveau_framebuffer *nvfb = dPriv->driverPrivate;
		st_resize_framebuffer(nvfb->stfb, dPriv->w, dPriv->h);
		nv->last_stamp = dPriv->lastStamp;
	}
}

void
nouveau_copy_sub_buffer(__DRIdrawablePrivate *dPriv, int x, int y, int w, int h)
{
	struct nouveau_framebuffer *nvfb = dPriv->driverPrivate;
	struct pipe_surface *surf;

	st_get_framebuffer_surface(nvfb->stfb, ST_SURFACE_BACK_LEFT, &surf);
	if (surf) {
		drm_clip_rect_t rect;
		rect.x1 = x;
		rect.y1 = y;
		rect.x2 = x + w;
		rect.y2 = y + h;

		st_notify_swapbuffers(nvfb->stfb);
		nouveau_copy_buffer(dPriv, surf, &rect);
	}
}

void
nouveau_swap_buffers(__DRIdrawablePrivate *dPriv)
{
	struct nouveau_framebuffer *nvfb = dPriv->driverPrivate;
	struct pipe_surface *surf;

	st_get_framebuffer_surface(nvfb->stfb, ST_SURFACE_BACK_LEFT, &surf);
	if (surf) {
		st_notify_swapbuffers(nvfb->stfb);
		nouveau_copy_buffer(dPriv, surf, NULL);
	}
}

void
nouveau_flush_frontbuffer(struct pipe_screen *pscreen, struct pipe_surface *ps,
			  void *context_private)
{
	struct nouveau_context *nv = context_private;
	__DRIdrawablePrivate *dPriv = nv->dri_drawable;

	nouveau_copy_buffer(dPriv, ps, NULL);
}

void
nouveau_contended_lock(struct nouveau_context *nv)
{
	struct nouveau_context *nv_sub = (struct nouveau_context*)nv;
	__DRIdrawablePrivate *dPriv = nv_sub->dri_drawable;
	__DRIscreenPrivate *sPriv = nv_sub->dri_screen;

	/* If the window moved, may need to set a new cliprect now.
	 *
	 * NOTE: This releases and regains the hw lock, so all state
	 * checking must be done *after* this call:
	 */
	if (dPriv)
		DRI_VALIDATE_DRAWABLE_INFO(sPriv, dPriv);
}

