#ifndef __NV04_SCREEN_H__
#define __NV04_SCREEN_H__

#include "nouveau/nouveau_screen.h"
#include "nv04_surface_2d.h"

struct nv04_screen {
	struct nouveau_screen base;

	struct nouveau_winsys *nvws;
	unsigned chipset;

	/* HW graphics objects */
	struct nv04_surface_2d *eng2d;
	struct nouveau_grobj *fahrenheit;
	struct nouveau_grobj *context_surfaces_3d;
	struct nouveau_notifier *sync;

};

static INLINE struct nv04_screen *
nv04_screen(struct pipe_screen *screen)
{
	return (struct nv04_screen *)screen;
}

void
nv04_screen_init_transfer_functions(struct pipe_screen *pscreen);

#endif
