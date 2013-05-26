/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#include "pipe/p_state.h"
#include "util/u_string.h"
#include "util/u_memory.h"

#include "fd3_blend.h"
#include "fd3_context.h"
#include "fd3_util.h"

void *
fd3_blend_state_create(struct pipe_context *pctx,
		const struct pipe_blend_state *cso)
{
	struct fd3_blend_stateobj *so;
	int i;

	if (cso->logicop_enable) {
		DBG("Unsupported! logicop");
		return NULL;
	}

	if (cso->independent_blend_enable) {
		DBG("Unsupported! independent blend state");
		return NULL;
	}

	so = CALLOC_STRUCT(fd3_blend_stateobj);
	if (!so)
		return NULL;

	so->base = *cso;

	for (i = 0; i < ARRAY_SIZE(so->rb_mrt); i++) {
		const struct pipe_rt_blend_state *rt = &cso->rt[i];

		so->rb_mrt[i].blend_control =
				A3XX_RB_MRT_BLEND_CONTROL_RGB_SRC_FACTOR(fd_blend_factor(rt->rgb_src_factor)) |
				A3XX_RB_MRT_BLEND_CONTROL_RGB_BLEND_OPCODE(fd_blend_func(rt->rgb_func)) |
				A3XX_RB_MRT_BLEND_CONTROL_RGB_DEST_FACTOR(fd_blend_factor(rt->rgb_dst_factor)) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_SRC_FACTOR(fd_blend_factor(rt->alpha_src_factor)) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_BLEND_OPCODE(fd_blend_func(rt->alpha_func)) |
				A3XX_RB_MRT_BLEND_CONTROL_ALPHA_DEST_FACTOR(fd_blend_factor(rt->alpha_dst_factor)) |
				A3XX_RB_MRT_BLEND_CONTROL_CLAMP_ENABLE;

		so->rb_mrt[i].control =
				A3XX_RB_MRT_CONTROL_ROP_CODE(12) |
				A3XX_RB_MRT_CONTROL_COMPONENT_ENABLE(rt->colormask);

		if (rt->blend_enable)
			so->rb_mrt[i].control |=
					A3XX_RB_MRT_CONTROL_READ_DEST_ENABLE |
					A3XX_RB_MRT_CONTROL_BLEND |
					A3XX_RB_MRT_CONTROL_BLEND2;

		if (cso->dither)
			so->rb_mrt[i].control |= A3XX_RB_MRT_CONTROL_DITHER_MODE(DITHER_ALWAYS);
	}

	return so;
}
