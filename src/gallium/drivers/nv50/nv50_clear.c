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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "nv50_context.h"

void
nv50_clear(struct pipe_context *pipe, struct pipe_surface *ps,
	   unsigned clearValue)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_framebuffer_state fb, s_fb = nv50->framebuffer;
	struct pipe_scissor_state sc, s_sc = nv50->scissor;
	unsigned dirty = nv50->dirty;

	nv50->dirty = 0;

	if (ps->format == PIPE_FORMAT_Z24S8_UNORM ||
	    ps->format == PIPE_FORMAT_Z16_UNORM) {
		fb.nr_cbufs = 0;
		fb.zsbuf = ps;
	} else {
		fb.nr_cbufs = 1;
		fb.cbufs[0] = ps;
		fb.zsbuf = NULL;
	}
	fb.width = ps->width;
	fb.height = ps->height;
	pipe->set_framebuffer_state(pipe, &fb);

	sc.minx = sc.miny = 0;
	sc.maxx = fb.width;
	sc.maxy = fb.height;
	pipe->set_scissor_state(pipe, &sc);

	nv50_state_validate(nv50);

	switch (ps->format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
		BEGIN_RING(chan, tesla, 0x0d80, 4);
		OUT_RINGf (chan, ubyte_to_float((clearValue >> 16) & 0xff));
		OUT_RINGf (chan, ubyte_to_float((clearValue >>  8) & 0xff));
		OUT_RINGf (chan, ubyte_to_float((clearValue >>  0) & 0xff));
		OUT_RINGf (chan, ubyte_to_float((clearValue >> 24) & 0xff));
		BEGIN_RING(chan, tesla, 0x19d0, 1);
		OUT_RING  (chan, 0x3c);
		break;
	case PIPE_FORMAT_Z24S8_UNORM:
		BEGIN_RING(chan, tesla, 0x0d90, 1);
		OUT_RINGf (chan, (float)(clearValue >> 8) * (1.0 / 16777215.0));
		BEGIN_RING(chan, tesla, 0x0da0, 1);
		OUT_RING  (chan, clearValue & 0xff);
		BEGIN_RING(chan, tesla, 0x19d0, 1);
		OUT_RING  (chan, 0x03);
		break;
	default:
		pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height,
				   clearValue);
		break;
	}

	pipe->set_framebuffer_state(pipe, &s_fb);
	pipe->set_scissor_state(pipe, &s_sc);
	nv50->dirty |= dirty;
}

