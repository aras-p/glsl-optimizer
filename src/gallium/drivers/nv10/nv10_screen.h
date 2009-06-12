#ifndef __NV10_SCREEN_H__
#define __NV10_SCREEN_H__

#include "nouveau/nouveau_screen.h"
#include "nv04/nv04_surface_2d.h"

struct nv10_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;

	/* HW graphics objects */
	struct nv04_surface_2d *eng2d;
	struct nouveau_grobj *celsius;
	struct nouveau_notifier *sync;
};

static INLINE struct nv10_screen *
nv10_screen(struct pipe_screen *screen)
{
	return (struct nv10_screen *)screen;
}


void
nv10_screen_init_transfer_functions(struct pipe_screen *pscreen);

#endif
