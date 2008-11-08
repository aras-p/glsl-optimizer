#ifndef __NV20_SCREEN_H__
#define __NV20_SCREEN_H__

#include "pipe/p_screen.h"

struct nv20_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;

	/* HW graphics objects */
	struct nouveau_grobj *kelvin;
	struct nouveau_notifier *sync;
};

static INLINE struct nv20_screen *
nv20_screen(struct pipe_screen *screen)
{
	return (struct nv20_screen *)screen;
}

#endif
