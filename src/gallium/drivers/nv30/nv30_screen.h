#ifndef __NV30_SCREEN_H__
#define __NV30_SCREEN_H__

#include "pipe/p_screen.h"

struct nv30_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;

	/* HW graphics objects */
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

#endif
