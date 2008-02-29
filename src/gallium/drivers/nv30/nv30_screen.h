#ifndef __NV30_SCREEN_H__
#define __NV30_SCREEN_H__

#include "pipe/p_screen.h"

struct nv30_screen {
	struct pipe_screen screen;
	unsigned chipset;
};

static INLINE struct nv30_screen *
nv30_screen(struct pipe_screen *screen)
{
	return (struct nv30_screen *)screen;
}

extern struct pipe_screen *
nv30_screen_create(struct pipe_winsys *winsys, unsigned chipset);

#endif
