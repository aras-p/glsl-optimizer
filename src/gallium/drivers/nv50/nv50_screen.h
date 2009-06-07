#ifndef __NV50_SCREEN_H__
#define __NV50_SCREEN_H__

#include "nouveau/nouveau_screen.h"

struct nv50_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;

	unsigned cur_pctx;

	struct nouveau_grobj *tesla;
	struct nouveau_grobj *eng2d;
	struct nouveau_grobj *m2mf;
	struct nouveau_notifier *sync;

	struct nouveau_bo *constbuf_misc[1];
	struct nouveau_bo *constbuf_parm[2];

	struct nouveau_resource *immd_heap[1];
	struct nouveau_resource *parm_heap[2];

	struct nouveau_bo *tic;
	struct nouveau_bo *tsc;

	struct nouveau_stateobj *static_init;
};

static INLINE struct nv50_screen *
nv50_screen(struct pipe_screen *screen)
{
	return (struct nv50_screen *)screen;
}

void nv50_transfer_init_screen_functions(struct pipe_screen *);

#endif
