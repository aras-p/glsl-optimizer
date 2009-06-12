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
nv50_clear(struct pipe_context *pipe, unsigned buffers,
	   const float *rgba, double depth, unsigned stencil)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_framebuffer_state *fb = &nv50->framebuffer;
	unsigned mode = 0, i;

	if (!nv50_state_validate(nv50))
		return;

	if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
		BEGIN_RING(chan, tesla, NV50TCL_CLEAR_COLOR(0), 4);
		OUT_RING  (chan, fui(rgba[0]));
		OUT_RING  (chan, fui(rgba[1]));
		OUT_RING  (chan, fui(rgba[2]));
		OUT_RING  (chan, fui(rgba[3]));
		mode |= 0x3c;
	}

	if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
		BEGIN_RING(chan, tesla, NV50TCL_CLEAR_DEPTH, 1);
		OUT_RING  (chan, fui(depth));
		BEGIN_RING(chan, tesla, NV50TCL_CLEAR_STENCIL, 1);
		OUT_RING  (chan, stencil & 0xff);

		mode |= 0x03;
	}

	BEGIN_RING(chan, tesla, NV50TCL_CLEAR_BUFFERS, 1);
	OUT_RING  (chan, mode);

	for (i = 1; i < fb->nr_cbufs; i++) {
		BEGIN_RING(chan, tesla, NV50TCL_CLEAR_BUFFERS, 1);
		OUT_RING  (chan, (i << 6) | 0x3c);
	}
}

