#ifndef NOUVEAU_PIPE_WINSYS_H
#define NOUVEAU_PIPE_WINSYS_H

#include "pipe/p_context.h"
#include "pipe/p_winsys.h"
#include "nouveau_context.h"

struct nouveau_pipe_buffer {
	struct pipe_buffer base;
	struct nouveau_bo *bo;
};

static inline struct nouveau_pipe_buffer *
nouveau_buffer(struct pipe_buffer *buf)
{
	return (struct nouveau_pipe_buffer *)buf;
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

#endif
