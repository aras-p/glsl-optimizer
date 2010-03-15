/*
 * Copyright (C) 2009-2010 Francisco Jerez.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "nouveau_driver.h"
#include "nouveau_context.h"
#include "nouveau_class.h"
#include "nv10_driver.h"
#include "nv20_driver.h"

void
nv20_emit_tex_env(GLcontext *ctx, int emit)
{
	const int i = emit - NOUVEAU_STATE_TEX_ENV0;
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	uint32_t a_in, a_out, c_in, c_out, k;

	nv10_get_general_combiner(ctx, i, &a_in, &a_out, &c_in, &c_out, &k);

	BEGIN_RING(chan, kelvin, NV20TCL_RC_IN_ALPHA(i), 1);
	OUT_RING(chan, a_in);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_OUT_ALPHA(i), 1);
	OUT_RING(chan, a_out);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_IN_RGB(i), 1);
	OUT_RING(chan, c_in);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_OUT_RGB(i), 1);
	OUT_RING(chan, c_out);
	BEGIN_RING(chan, kelvin, NV20TCL_RC_CONSTANT_COLOR0(i), 1);
	OUT_RING(chan, k);

	context_dirty(ctx, FRAG);
}

void
nv20_emit_frag(GLcontext *ctx, int emit)
{
	struct nouveau_channel *chan = context_chan(ctx);
	struct nouveau_grobj *kelvin = context_eng3d(ctx);
	uint64_t in;
	int n;

	nv10_get_final_combiner(ctx, &in, &n);

	BEGIN_RING(chan, kelvin, NV20TCL_RC_FINAL0, 2);
	OUT_RING(chan, in);
	OUT_RING(chan, in >> 32);

	BEGIN_RING(chan, kelvin, NV20TCL_RC_ENABLE, 1);
	OUT_RING(chan, n);
}
