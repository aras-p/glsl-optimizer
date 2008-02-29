#ifndef __NV50_SCREEN_H__
#define __NV50_SCREEN_H__

#include "pipe/p_screen.h"

struct nv50_screen {
	struct pipe_screen pipe;
	unsigned chipset;
};

static INLINE struct nv50_screen *
nv50_screen(struct pipe_screen *screen)
{
	return (struct nv50_screen *)screen;
}

extern struct pipe_screen *
nv50_screen_create(struct pipe_winsys *winsys, unsigned chipset);

#endif
