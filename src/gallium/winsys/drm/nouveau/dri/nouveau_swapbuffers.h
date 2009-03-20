#ifndef __NOUVEAU_SWAPBUFFERS_H__
#define __NOUVEAU_SWAPBUFFERS_H__

void nouveau_copy_buffer(__DRIdrawablePrivate *, struct pipe_surface *,
			 const drm_clip_rect_t *);
void nouveau_copy_sub_buffer(__DRIdrawablePrivate *, int x, int y, int w, int h);
void nouveau_swap_buffers(__DRIdrawablePrivate *);
void nouveau_flush_frontbuffer(struct pipe_screen *, struct pipe_surface *,
			       void *context_private);

#endif
