#ifndef __NV50_SCREEN_H__
#define __NV50_SCREEN_H__

#include "pipe/p_screen.h"

struct nv50_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;

	unsigned cur_pctx;

	struct nouveau_grobj *tesla;
	struct nouveau_grobj *eng2d;
	struct nouveau_grobj *m2mf;
	struct nouveau_notifier *sync;

	struct pipe_buffer *constbuf;
	struct nouveau_resource *vp_data_heap;

	struct pipe_buffer *tic;
	struct pipe_buffer *tsc;

	struct nouveau_stateobj *static_init;
};

static INLINE struct nv50_screen *
nv50_screen(struct pipe_screen *screen)
{
	return (struct nv50_screen *)screen;
}

void nv50_transfer_init_screen_functions(struct pipe_screen *);

#endif
