#ifndef NOUVEAU_PIPE_WINSYS_H
#define NOUVEAU_PIPE_WINSYS_H

#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_context.h"

#include "nouveau/nouveau_winsys.h"

#include "nouveau_device.h"

struct nouveau_pipe_buffer {
	struct pipe_buffer base;
	struct nouveau_bo *bo;
};

static INLINE struct nouveau_pipe_buffer *
nouveau_pipe_buffer(struct pipe_buffer *buf)
{
	return (struct nouveau_pipe_buffer *)buf;
}

struct nouveau_pipe_winsys {
	struct pipe_winsys base;

	struct pipe_screen *pscreen;

	struct nouveau_channel *channel;
	uint32_t next_handle;

	unsigned nr_pctx;
	struct pipe_context **pctx;
};

static INLINE struct nouveau_pipe_winsys *
nouveau_pipe_winsys(struct pipe_winsys *ws)
{
	return (struct nouveau_pipe_winsys *)ws;
}

static INLINE struct nouveau_pipe_winsys *
nouveau_screen(struct pipe_screen *pscreen)
{
	return nouveau_pipe_winsys(pscreen->winsys);
}

struct pipe_winsys *
nouveau_pipe_winsys_new(struct nouveau_device *);

struct nouveau_winsys *
nouveau_winsys_new(struct pipe_winsys *ws);

#endif
