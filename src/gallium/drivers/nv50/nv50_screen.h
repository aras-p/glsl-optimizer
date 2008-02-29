#ifndef __NV50_SCREEN_H__
#define __NV50_SCREEN_H__

#include "pipe/p_screen.h"

struct nv50_screen {
	struct pipe_screen pipe;

	struct nouveau_winsys *nvws;
	unsigned chipset;
};

static INLINE struct nv50_screen *
nv50_screen(struct pipe_screen *screen)
{
	return (struct nv50_screen *)screen;
}

#endif
