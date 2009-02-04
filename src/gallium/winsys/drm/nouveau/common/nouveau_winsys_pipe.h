#ifndef NOUVEAU_PIPE_WINSYS_H
#define NOUVEAU_PIPE_WINSYS_H

#include "pipe/p_context.h"
#include "pipe/internal/p_winsys_screen.h"
#include "nouveau_context.h"

struct nouveau_pipe_buffer {
	struct pipe_buffer base;
	struct nouveau_bo *bo;
};

/* This is so horrible I should be shot - I promise I'll fix it properly
 * tomorrow.  Just to make the winsys build again however... The TG guys
 * don't like to make life easy :)
 */
static inline struct nouveau_pipe_buffer *
nouveau_buffer(struct pipe_surface *ps)
{
	return *(struct nouveau_pipe_buffer **)
		((void *)ps->texture + sizeof(struct pipe_texture));
}

struct nouveau_pipe_winsys {
	struct pipe_winsys pws;

	struct nouveau_context *nv;
};

extern struct pipe_winsys *
nouveau_create_pipe_winsys(struct nouveau_context *nv);

struct pipe_context *
nouveau_create_softpipe(struct nouveau_context *nv);

struct pipe_context *
nouveau_pipe_create(struct nouveau_context *nv);

/* Must be provided by clients of common code */
extern void
nouveau_flush_frontbuffer(struct pipe_winsys *pws, struct pipe_surface *surf,
			  void *context_private);

struct pipe_surface *
nouveau_surface_buffer_ref(struct nouveau_context *nv, struct pipe_buffer *pb,
			   enum pipe_format format, int w, int h,
			   unsigned pitch, struct pipe_texture **ppt);

#endif
