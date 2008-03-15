#ifndef __NV10_SCREEN_H__
#define __NV10_SCREEN_H__

#include "pipe/p_screen.h"

struct nv10_screen {
	struct pipe_screen screen;

	struct nouveau_winsys *nvws;
	unsigned chipset;
};

static INLINE struct nv10_screen *
nv10_screen(struct pipe_screen *screen)
{
	return (struct nv10_screen *)screen;
}

#endif
