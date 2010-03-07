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

#define __NOUVEAU_PUSH_H__
#include <stdint.h>
#include "nouveau/nouveau_pushbuf.h"
#include "nv50_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_tile.h"

static INLINE int
nv50_format(enum pipe_format format)
{
	switch (format) {
	case PIPE_FORMAT_B8G8R8A8_UNORM:
		return NV50_2D_DST_FORMAT_A8R8G8B8_UNORM;
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		return NV50_2D_DST_FORMAT_X8R8G8B8_UNORM;
	case PIPE_FORMAT_B5G6R5_UNORM:
		return NV50_2D_DST_FORMAT_R5G6B5_UNORM;
	case PIPE_FORMAT_A8_UNORM:
		return NV50_2D_DST_FORMAT_R8_UNORM;
	default:
		return -1;
	}
}

static int
nv50_surface_set(struct nv50_screen *screen, struct pipe_surface *ps, int dst)
{
	struct nv50_miptree *mt = nv50_miptree(ps->texture);
	struct nouveau_channel *chan = screen->eng2d->channel;
	struct nouveau_grobj *eng2d = screen->eng2d;
	struct nouveau_bo *bo = nv50_miptree(ps->texture)->base.bo;
 	int format, mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
 	int flags = NOUVEAU_BO_VRAM | (dst ? NOUVEAU_BO_WR : NOUVEAU_BO_RD);

 	format = nv50_format(ps->format);
 	if (format < 0)
 		return 1;

 	if (!bo->tile_flags) {
		MARK_RING (chan, 9, 2); /* flush on lack of space or relocs */
 		BEGIN_RING(chan, eng2d, mthd, 2);
 		OUT_RING  (chan, format);
 		OUT_RING  (chan, 1);
 		BEGIN_RING(chan, eng2d, mthd + 0x14, 5);
		OUT_RING  (chan, mt->level[ps->level].pitch);
 		OUT_RING  (chan, ps->width);
 		OUT_RING  (chan, ps->height);
 		OUT_RELOCh(chan, bo, ps->offset, flags);
 		OUT_RELOCl(chan, bo, ps->offset, flags);
 	} else {
		MARK_RING (chan, 11, 2); /* flush on lack of space or relocs */
 		BEGIN_RING(chan, eng2d, mthd, 5);
 		OUT_RING  (chan, format);
 		OUT_RING  (chan, 0);
		OUT_RING  (chan, mt->level[ps->level].tile_mode << 4);
 		OUT_RING  (chan, 1);
 		OUT_RING  (chan, 0);
 		BEGIN_RING(chan, eng2d, mthd + 0x18, 4);
 		OUT_RING  (chan, ps->width);
 		OUT_RING  (chan, ps->height);
 		OUT_RELOCh(chan, bo, ps->offset, flags);
 		OUT_RELOCl(chan, bo, ps->offset, flags);
 	}
 
#if 0
 	if (dst) {
 		BEGIN_RING(chan, eng2d, NV50_2D_CLIP_X, 4);
 		OUT_RING  (chan, 0);
 		OUT_RING  (chan, 0);
 		OUT_RING  (chan, surf->width);
 		OUT_RING  (chan, surf->height);
 	}
#endif
  
 	return 0;
}

int
nv50_surface_do_copy(struct nv50_screen *screen, struct pipe_surface *dst,
		     int dx, int dy, struct pipe_surface *src, int sx, int sy,
		     int w, int h)
{
	struct nouveau_channel *chan = screen->eng2d->channel;
	struct nouveau_grobj *eng2d = screen->eng2d;
	int ret;

	WAIT_RING (chan, 32);

	ret = nv50_surface_set(screen, dst, 1);
	if (ret)
		return ret;

	ret = nv50_surface_set(screen, src, 0);
	if (ret)
		return ret;

	BEGIN_RING(chan, eng2d, 0x088c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, eng2d, NV50_2D_BLIT_DST_X, 4);
	OUT_RING  (chan, dx);
	OUT_RING  (chan, dy);
	OUT_RING  (chan, w);
	OUT_RING  (chan, h);
	BEGIN_RING(chan, eng2d, 0x08c0, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng2d, 0x08d0, 4);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, sx);
	OUT_RING  (chan, 0);
	OUT_RING  (chan, sy);

	return 0;
}

static void
nv50_surface_copy(struct pipe_context *pipe,
		  struct pipe_surface *dest, unsigned destx, unsigned desty,
		  struct pipe_surface *src, unsigned srcx, unsigned srcy,
		  unsigned width, unsigned height)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nv50_screen *screen = nv50->screen;

	assert(src->format == dest->format);

	nv50_surface_do_copy(screen, dest, destx, desty, src, srcx,
				     srcy, width, height);
}

static void
nv50_surface_fill(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, unsigned width,
		  unsigned height, unsigned value)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nv50_screen *screen = nv50->screen;
	struct nouveau_channel *chan = screen->eng2d->channel;
	struct nouveau_grobj *eng2d = screen->eng2d;
	int format, ret;

	format = nv50_format(dest->format);
	if (format < 0)
		return;

	WAIT_RING (chan, 32);

	ret = nv50_surface_set(screen, dest, 1);
	if (ret)
		return;

	BEGIN_RING(chan, eng2d, NV50_2D_DRAW_SHAPE, 3);
	OUT_RING  (chan, NV50_2D_DRAW_SHAPE_RECTANGLES);
	OUT_RING  (chan, format);
	OUT_RING  (chan, value);
	BEGIN_RING(chan, eng2d, NV50_2D_DRAW_POINT32_X(0), 4);
	OUT_RING  (chan, destx);
	OUT_RING  (chan, desty);
	OUT_RING  (chan, width);
	OUT_RING  (chan, height);
}

void
nv50_init_surface_functions(struct nv50_context *nv50)
{
	nv50->pipe.surface_copy = nv50_surface_copy;
	nv50->pipe.surface_fill = nv50_surface_fill;
}


