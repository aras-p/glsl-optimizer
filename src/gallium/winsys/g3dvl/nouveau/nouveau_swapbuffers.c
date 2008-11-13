#include "pipe/p_context.h"
#include "nouveau_context.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_swapbuffers.h"

void
nouveau_copy_buffer(dri_drawable_t *dri_drawable, struct pipe_surface *surf,
		    const drm_clip_rect_t *rect)
{
	struct nouveau_context	*nv = dri_drawable->private;
	drm_clip_rect_t		*pbox;
	int			nbox, i;

	LOCK_HARDWARE(nv);
	if (!dri_drawable->num_cliprects) {
		UNLOCK_HARDWARE(nv);
		return;
	}
	pbox = dri_drawable->cliprects;
	nbox = dri_drawable->num_cliprects;

	nv->surface_copy_prep(nv, nv->frontbuffer, surf);
	for (i = 0; i < nbox; i++, pbox++) {
		int sx, sy, dx, dy, w, h;

		sx = pbox->x1 - dri_drawable->x;
		sy = pbox->y1 - dri_drawable->y;
		dx = pbox->x1;
		dy = pbox->y1;
		w  = pbox->x2 - pbox->x1;
		h  = pbox->y2 - pbox->y1;

		nv->surface_copy(nv, dx, dy, sx, sy, w, h);
	}

	FIRE_RING(nv->nvc->channel);
	UNLOCK_HARDWARE(nv);

	//if (nv->last_stamp != dri_drawable->last_sarea_stamp)
		//nv->last_stamp = dri_drawable->last_sarea_stamp;
}

void
nouveau_copy_sub_buffer(dri_drawable_t *dri_drawable, struct pipe_surface *surf, int x, int y, int w, int h)
{
	if (surf) {
		drm_clip_rect_t rect;
		rect.x1 = x;
		rect.y1 = y;
		rect.x2 = x + w;
		rect.y2 = y + h;

		nouveau_copy_buffer(dri_drawable, surf, &rect);
	}
}

void
nouveau_swap_buffers(dri_drawable_t *dri_drawable, struct pipe_surface *surf)
{
	if (surf)
		nouveau_copy_buffer(dri_drawable, surf, NULL);
}

