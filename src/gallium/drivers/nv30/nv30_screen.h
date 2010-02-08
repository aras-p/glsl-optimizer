#ifndef __NV30_SCREEN_H__
#define __NV30_SCREEN_H__

#include "nouveau/nouveau_screen.h"

#include "nouveau/nv04_surface_2d.h"

struct nv30_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;

	struct nv30_context *cur_ctx;

	/* HW graphics objects */
	struct nv04_surface_2d *eng2d;
	struct nouveau_grobj *rankine;
	struct nouveau_notifier *sync;

	/* Query object resources */
	struct nouveau_notifier *query;
	struct nouveau_resource *query_heap;

	/* Vtxprog resources */
	struct nouveau_resource *vp_exec_heap;
	struct nouveau_resource *vp_data_heap;

	/* Current 3D state of channel */
	struct nouveau_stateobj *state[NV30_STATE_MAX];
};

static INLINE struct nv30_screen *
nv30_screen(struct pipe_screen *screen)
{
	return (struct nv30_screen *)screen;
}

void
nv30_screen_init_transfer_functions(struct pipe_screen *pscreen);

#endif
