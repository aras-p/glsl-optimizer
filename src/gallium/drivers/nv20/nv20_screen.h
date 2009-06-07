#ifndef __NV20_SCREEN_H__
#define __NV20_SCREEN_H__

#include "nouveau/nouveau_screen.h"
#include "nv04/nv04_surface_2d.h"

struct nv20_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;

	/* HW graphics objects */
	struct nv04_surface_2d *eng2d;
	struct nouveau_grobj *kelvin;
	struct nouveau_notifier *sync;
};

static INLINE struct nv20_screen *
nv20_screen(struct pipe_screen *screen)
{
	return (struct nv20_screen *)screen;
}


void
nv20_screen_init_transfer_functions(struct pipe_screen *pscreen);

#endif
