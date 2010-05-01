#ifndef __NV04_SURFACE_2D_H__
#define __NV04_SURFACE_2D_H__

#include "pipe/p_state.h"

struct nouveau_screen;

struct nv04_surface {
	struct pipe_surface base;
	unsigned pitch;
	struct nv04_surface* backing;
};

struct nv04_surface_2d {
	struct nouveau_notifier *ntfy;
	struct nouveau_grobj *surf2d;
	struct nouveau_grobj *swzsurf;
	struct nouveau_grobj *m2mf;
	struct nouveau_grobj *rect;
	struct nouveau_grobj *blit;
	struct nouveau_grobj *sifm;

	struct nouveau_bo *(*buf)(struct pipe_surface *);

	void (*copy)(struct nv04_surface_2d *, struct pipe_surface *dst,
		     int dx, int dy, struct pipe_surface *src, int sx, int sy,
		     int w, int h);
	void (*fill)(struct nv04_surface_2d *, struct pipe_surface *dst,
		     int dx, int dy, int w, int h, unsigned value);
};

struct nv04_surface_2d *
nv04_surface_2d_init(struct nouveau_screen *screen);

void
nv04_surface_2d_takedown(struct nv04_surface_2d **);

struct nv04_surface*
nv04_surface_wrap_for_render(struct pipe_screen *pscreen, struct nv04_surface_2d* eng2d, struct nv04_surface* ns);

#define NVFX_RESOURCE_FLAG_LINEAR (PIPE_RESOURCE_FLAG_DRV_PRIV << 0)

#endif
