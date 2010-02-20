
/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "nvfx_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_tile.h"

static void
nvfx_surface_copy(struct pipe_context *pipe,
		  struct pipe_surface *dest, unsigned destx, unsigned desty,
		  struct pipe_surface *src, unsigned srcx, unsigned srcy,
		  unsigned width, unsigned height)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nv04_surface_2d *eng2d = nvfx->screen->eng2d;

	eng2d->copy(eng2d, dest, destx, desty, src, srcx, srcy, width, height);
}

static void
nvfx_surface_fill(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, unsigned width,
		  unsigned height, unsigned value)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nv04_surface_2d *eng2d = nvfx->screen->eng2d;

	eng2d->fill(eng2d, dest, destx, desty, width, height, value);
}

void
nvfx_init_surface_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.surface_copy = nvfx_surface_copy;
	nvfx->pipe.surface_fill = nvfx_surface_fill;
}
