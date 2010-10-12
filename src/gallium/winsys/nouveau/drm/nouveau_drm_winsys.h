#ifndef __NOUVEAU_DRM_WINSYS_H__
#define __NOUVEAU_DRM_WINSYS_H__

#include "util/u_simple_screen.h"

#include "nouveau_dri.h"

struct nouveau_winsys {
	struct pipe_winsys base;

	struct pipe_screen *pscreen;

	struct pipe_surface *front;
};

static INLINE struct nouveau_winsys *
nouveau_winsys(struct pipe_winsys *ws)
{
	return (struct nouveau_winsys *)ws;
}

static INLINE struct nouveau_winsys *
nouveau_winsys_screen(struct pipe_screen *pscreen)
{
	return nouveau_winsys(pscreen->winsys);
}

#endif
