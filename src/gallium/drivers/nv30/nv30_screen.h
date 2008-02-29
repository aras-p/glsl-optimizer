#ifndef __NV30_SCREEN_H__
#define __NV30_SCREEN_H__

#include "pipe/p_screen.h"

struct nv30_screen {
	struct pipe_screen screen;

	struct nouveau_winsys *nvws;
	unsigned chipset;
};

static INLINE struct nv30_screen *
nv30_screen(struct pipe_screen *screen)
{
	return (struct nv30_screen *)screen;
}

#endif
