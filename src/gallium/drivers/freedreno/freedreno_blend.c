/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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

#include "freedreno_blend.h"
#include "freedreno_context.h"
#include "freedreno_util.h"

static enum adreno_rb_blend_factor
blend_factor(unsigned factor)
{
	switch (factor) {
	case PIPE_BLENDFACTOR_ONE:
		return FACTOR_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return FACTOR_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return FACTOR_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return FACTOR_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return FACTOR_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return FACTOR_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return FACTOR_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return FACTOR_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
	case 0:
		return FACTOR_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return FACTOR_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return FACTOR_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return FACTOR_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return FACTOR_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return FACTOR_ONE_MINUS_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return FACTOR_ONE_MINUS_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
	case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
	case PIPE_BLENDFACTOR_SRC1_COLOR:
	case PIPE_BLENDFACTOR_SRC1_ALPHA:
		/* I don't think these are supported */
	default:
		DBG("invalid blend factor: %x", factor);
		return 0;
	}
}

static enum adreno_rb_blend_opcode
blend_func(unsigned func)
{
	switch (func) {
	case PIPE_BLEND_ADD:
		return BLEND_DST_PLUS_SRC;
	case PIPE_BLEND_MIN:
		return BLEND_MIN_DST_SRC;
	case PIPE_BLEND_MAX:
		return BLEND_MAX_DST_SRC;
	case PIPE_BLEND_SUBTRACT:
		return BLEND_SRC_MINUS_DST;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return BLEND_DST_MINUS_SRC;
	default:
		DBG("invalid blend func: %x", func);
		return 0;
	}
}

static void *
fd_blend_state_create(struct pipe_context *pctx,
		const struct pipe_blend_state *cso)
{
	const struct pipe_rt_blend_state *rt = &cso->rt[0];
	struct fd_blend_stateobj *so;

	if (cso->logicop_enable) {
		DBG("Unsupported! logicop");
		return NULL;
	}

	if (cso->independent_blend_enable) {
		DBG("Unsupported! independent blend state");
		return NULL;
	}

	so = CALLOC_STRUCT(fd_blend_stateobj);
	if (!so)
		return NULL;

	so->base = *cso;

	so->rb_colorcontrol = A2XX_RB_COLORCONTROL_ROP_CODE(12);

	so->rb_blendcontrol =
		A2XX_RB_BLEND_CONTROL_COLOR_SRCBLEND(blend_factor(rt->rgb_src_factor)) |
		A2XX_RB_BLEND_CONTROL_COLOR_COMB_FCN(blend_func(rt->rgb_func)) |
		A2XX_RB_BLEND_CONTROL_COLOR_DESTBLEND(blend_factor(rt->rgb_dst_factor)) |
		A2XX_RB_BLEND_CONTROL_ALPHA_SRCBLEND(blend_factor(rt->alpha_src_factor)) |
		A2XX_RB_BLEND_CONTROL_ALPHA_COMB_FCN(blend_func(rt->alpha_func)) |
		A2XX_RB_BLEND_CONTROL_ALPHA_DESTBLEND(blend_factor(rt->alpha_dst_factor));

	if (rt->colormask & PIPE_MASK_R)
		so->rb_colormask |= A2XX_RB_COLOR_MASK_WRITE_RED;
	if (rt->colormask & PIPE_MASK_G)
		so->rb_colormask |= A2XX_RB_COLOR_MASK_WRITE_GREEN;
	if (rt->colormask & PIPE_MASK_B)
		so->rb_colormask |= A2XX_RB_COLOR_MASK_WRITE_BLUE;
	if (rt->colormask & PIPE_MASK_A)
		so->rb_colormask |= A2XX_RB_COLOR_MASK_WRITE_ALPHA;

	if (!rt->blend_enable)
		so->rb_colorcontrol |= A2XX_RB_COLORCONTROL_BLEND_DISABLE;

	if (cso->dither)
		so->rb_colorcontrol |= A2XX_RB_COLORCONTROL_DITHER_MODE(DITHER_ALWAYS);

	return so;
}

static void
fd_blend_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->blend = hwcso;
	ctx->dirty |= FD_DIRTY_BLEND;
}

static void
fd_blend_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

void
fd_blend_init(struct pipe_context *pctx)
{
	pctx->create_blend_state = fd_blend_state_create;
	pctx->bind_blend_state = fd_blend_state_bind;
	pctx->delete_blend_state = fd_blend_state_delete;
}

