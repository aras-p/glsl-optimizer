#ifndef __NV04_SCREEN_H__
#define __NV04_SCREEN_H__

#include "pipe/p_screen.h"

struct nv04_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;
	unsigned chipset;

	/* HW graphics objects */
	struct nouveau_grobj *fahrenheit;
	struct nouveau_grobj *context_surfaces_3d;
	struct nouveau_notifier *sync;

};

static INLINE struct nv04_screen *
nv04_screen(struct pipe_screen *screen)
{
	return (struct nv04_screen *)screen;
}

#endif
