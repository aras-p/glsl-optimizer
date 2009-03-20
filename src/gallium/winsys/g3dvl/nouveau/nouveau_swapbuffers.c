#include <driclient.h>
#include <common/nouveau_local.h>
#include <common/nouveau_screen.h>
#include "nouveau_context_vl.h"
#include "nouveau_swapbuffers.h"

void
nouveau_copy_buffer(dri_drawable_t *dri_drawable, struct pipe_surface *surf,
		    const drm_clip_rect_t *rect)
{
	struct nouveau_context_vl	*nv = dri_drawable->private;
	struct pipe_context		*pipe = nv->base.nvc->pctx[nv->base.pctx_id];
	drm_clip_rect_t			*pbox;
	int				nbox, i;

	LOCK_HARDWARE(&nv->base);
	if (!dri_drawable->num_cliprects) {
		UNLOCK_HARDWARE(&nv->base);
		return;
	}
	pbox = dri_drawable->cliprects;
	nbox = dri_drawable->num_cliprects;

	for (i = 0; i < nbox; i++, pbox++) {
		int sx, sy, dx, dy, w, h;

		sx = pbox->x1 - dri_drawable->x;
		sy = pbox->y1 - dri_drawable->y;
		dx = pbox->x1;
		dy = pbox->y1;
		w  = pbox->x2 - pbox->x1;
		h  = pbox->y2 - pbox->y1;

		pipe->surface_copy(pipe, nv->base.frontbuffer,
				   dx, dy, surf, sx, sy, w, h);
	}

	FIRE_RING(nv->base.nvc->channel);
	UNLOCK_HARDWARE(&nv->base);
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

void
nouveau_flush_frontbuffer(struct pipe_winsys *pws, struct pipe_surface *surf,
			  void *context_private)
{
	struct nouveau_context_vl	*nv;
	dri_drawable_t			*dri_drawable;

	assert(pws);
	assert(surf);
	assert(context_private);

	nv = context_private;
	dri_drawable = nv->dri_drawable;

	nouveau_copy_buffer(dri_drawable, surf, NULL);
}

void
nouveau_contended_lock(struct nouveau_context *nv)
{
	struct nouveau_context_vl	*nv_vl = (struct nouveau_context_vl*)nv;
	dri_drawable_t			*dri_drawable = nv_vl->dri_drawable;
	dri_screen_t			*dri_screen = nv_vl->dri_context->dri_screen;

	/* If the window moved, may need to set a new cliprect now.
	 *
	 * NOTE: This releases and regains the hw lock, so all state
	 * checking must be done *after* this call:
	 */
	if (dri_drawable)
		DRI_VALIDATE_DRAWABLE_INFO(dri_screen, dri_drawable);
}
