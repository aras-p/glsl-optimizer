#ifndef __NOUVEAU_SWAPBUFFERS_H__
#define __NOUVEAU_SWAPBUFFERS_H__

extern void nouveau_copy_buffer(dri_drawable_t *, struct pipe_surface *,
				const drm_clip_rect_t *);
extern void nouveau_copy_sub_buffer(dri_drawable_t *, struct pipe_surface *,
				    int x, int y, int w, int h);
extern void nouveau_swap_buffers(dri_drawable_t *, struct pipe_surface *);

#endif
