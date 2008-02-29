#ifndef __NV40_SCREEN_H__
#define __NV40_SCREEN_H__

#include "pipe/p_screen.h"

struct nv40_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;
	unsigned chipset;
};

static INLINE struct nv40_screen *
nv40_screen(struct pipe_screen *screen)
{
	return (struct nv40_screen *)screen;
}

#endif
