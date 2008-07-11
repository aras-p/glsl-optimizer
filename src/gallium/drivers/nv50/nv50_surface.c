/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_inlines.h"
#include "util/p_tile.h"

static void
nv50_surface_copy(struct pipe_context *pipe, boolean flip,
		  struct pipe_surface *dest, unsigned destx, unsigned desty,
		  struct pipe_surface *src, unsigned srcx, unsigned srcy,
		  unsigned width, unsigned height)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->screen->nvws;

	if (flip) {
		desty += height;
		while (height--) {
			nvws->surface_copy(nvws, dest, destx, desty--, src,
					   srcx, srcy++, width, 1);
		}
	} else {
		nvws->surface_copy(nvws, dest, destx, desty, src, srcx, srcy,
				   width, height);
	}
}

static void
nv50_surface_fill(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, unsigned width,
		  unsigned height, unsigned value)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->screen->nvws;

	nvws->surface_fill(nvws, dest, destx, desty, width, height, value);
}

static void *
nv50_surface_map(struct pipe_screen *screen, struct pipe_surface *ps,
		 unsigned flags )
{
	struct nouveau_winsys *nvws = nv50_screen(screen)->nvws;
	struct pipe_winsys *ws = screen->winsys;
	struct nv50_surface *s = nv50_surface(ps);
	struct nv50_surface m = *s;
	void *map;

	if (!s->untiled) {
		s->untiled = ws->buffer_create(ws, 0, 0, ps->buffer->size);

		m.base.buffer = s->untiled;
		nvws->surface_copy(nvws, &m.base, 0, 0, &s->base, 0, 0,
					 ps->width, ps->height);
	}

	/* Map original tiled surface to disallow it being validated while
	 * untiled mirror is mapped.
	 */
	ws->buffer_map(ws, ps->buffer, flags);

	map = ws->buffer_map(ws, s->untiled, flags);
	if (!map)
		return NULL;

	return map;
}

static void
nv50_surface_unmap(struct pipe_screen *screen, struct pipe_surface *ps)
{
	struct nouveau_winsys *nvws = nv50_screen(screen)->nvws;
	struct pipe_winsys *ws = screen->winsys;
	struct nv50_surface *s = nv50_surface(ps);
	struct nv50_surface m = *s;

	ws->buffer_unmap(ws, s->untiled);
	ws->buffer_unmap(ws, ps->buffer);

	m.base.buffer = s->untiled;
	nvws->surface_copy(nvws, &s->base, 0, 0, &m.base, 0, 0,
				 ps->width, ps->height);

	pipe_buffer_reference(ws, &s->untiled, NULL);
}

void
nv50_init_surface_functions(struct nv50_context *nv50)
{
	nv50->pipe.surface_copy = nv50_surface_copy;
	nv50->pipe.surface_fill = nv50_surface_fill;
}

void
nv50_surface_init_screen_functions(struct pipe_screen *pscreen)
{
	pscreen->surface_map = nv50_surface_map;
	pscreen->surface_unmap = nv50_surface_unmap;
}

