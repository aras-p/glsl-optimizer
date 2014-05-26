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
#include "util/u_inlines.h"
#include "util/u_format.h"

#include "fd3_texture.h"
#include "fd3_util.h"

static enum a3xx_tex_clamp
tex_clamp(unsigned wrap)
{
	/* hardware probably supports more, but we can't coax all the
	 * wrap/clamp modes out of the GLESv2 blob driver.
	 *
	 * TODO once we have basics working, go back and just try
	 * different values and see what happens
	 */
	switch (wrap) {
	case PIPE_TEX_WRAP_REPEAT:
		return A3XX_TEX_REPEAT;
	case PIPE_TEX_WRAP_CLAMP:
	case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
		return A3XX_TEX_CLAMP_TO_EDGE;
	case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
		return A3XX_TEX_CLAMP_TO_BORDER;
	case PIPE_TEX_WRAP_MIRROR_CLAMP:
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
	case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
		return A3XX_TEX_MIRROR_CLAMP;
	case PIPE_TEX_WRAP_MIRROR_REPEAT:
		return A3XX_TEX_MIRROR_REPEAT;
	default:
		DBG("invalid wrap: %u", wrap);
		return 0;
	}
}

static enum a3xx_tex_filter
tex_filter(unsigned filter)
{
	switch (filter) {
	case PIPE_TEX_FILTER_NEAREST:
		return A3XX_TEX_NEAREST;
	case PIPE_TEX_FILTER_LINEAR:
		return A3XX_TEX_LINEAR;
	default:
		DBG("invalid filter: %u", filter);
		return 0;
	}
}

static void *
fd3_sampler_state_create(struct pipe_context *pctx,
		const struct pipe_sampler_state *cso)
{
	struct fd3_sampler_stateobj *so = CALLOC_STRUCT(fd3_sampler_stateobj);
	bool miplinear = false;

	if (!so)
		return NULL;

	if (cso->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
		miplinear = true;

	so->base = *cso;

	so->texsamp0 =
			COND(!cso->normalized_coords, A3XX_TEX_SAMP_0_UNNORM_COORDS) |
			COND(miplinear, A3XX_TEX_SAMP_0_MIPFILTER_LINEAR) |
			A3XX_TEX_SAMP_0_XY_MAG(tex_filter(cso->mag_img_filter)) |
			A3XX_TEX_SAMP_0_XY_MIN(tex_filter(cso->min_img_filter)) |
			A3XX_TEX_SAMP_0_WRAP_S(tex_clamp(cso->wrap_s)) |
			A3XX_TEX_SAMP_0_WRAP_T(tex_clamp(cso->wrap_t)) |
			A3XX_TEX_SAMP_0_WRAP_R(tex_clamp(cso->wrap_r));

	if (cso->compare_mode)
		so->texsamp0 |= A3XX_TEX_SAMP_0_COMPARE_FUNC(cso->compare_func); /* maps 1:1 */

	if (cso->min_mip_filter != PIPE_TEX_MIPFILTER_NONE) {
		so->texsamp1 =
				A3XX_TEX_SAMP_1_MIN_LOD(cso->min_lod) |
				A3XX_TEX_SAMP_1_MAX_LOD(cso->max_lod);
	} else {
		so->texsamp1 = 0x00000000;
	}

	return so;
}

static enum a3xx_tex_type
tex_type(unsigned target)
{
	switch (target) {
	default:
		assert(0);
	case PIPE_BUFFER:
	case PIPE_TEXTURE_1D:
	case PIPE_TEXTURE_1D_ARRAY:
		return A3XX_TEX_1D;
	case PIPE_TEXTURE_RECT:
	case PIPE_TEXTURE_2D:
	case PIPE_TEXTURE_2D_ARRAY:
		return A3XX_TEX_2D;
	case PIPE_TEXTURE_3D:
		return A3XX_TEX_3D;
	case PIPE_TEXTURE_CUBE:
	case PIPE_TEXTURE_CUBE_ARRAY:
		return A3XX_TEX_CUBE;
	}
}

static struct pipe_sampler_view *
fd3_sampler_view_create(struct pipe_context *pctx, struct pipe_resource *prsc,
		const struct pipe_sampler_view *cso)
{
	struct fd3_pipe_sampler_view *so = CALLOC_STRUCT(fd3_pipe_sampler_view);
	struct fd_resource *rsc = fd_resource(prsc);
	unsigned miplevels = cso->u.tex.last_level - cso->u.tex.first_level;

	if (!so)
		return NULL;

	so->base = *cso;
	pipe_reference(NULL, &prsc->reference);
	so->base.texture = prsc;
	so->base.reference.count = 1;
	so->base.context = pctx;

	so->tex_resource =  rsc;
	so->mipaddrs = 1 + miplevels;

	so->texconst0 =
			A3XX_TEX_CONST_0_TYPE(tex_type(prsc->target)) |
			A3XX_TEX_CONST_0_FMT(fd3_pipe2tex(cso->format)) |
			A3XX_TEX_CONST_0_MIPLVLS(miplevels) |
			fd3_tex_swiz(cso->format, cso->swizzle_r, cso->swizzle_g,
						cso->swizzle_b, cso->swizzle_a);

	if (util_format_is_srgb(cso->format))
		so->texconst0 |= A3XX_TEX_CONST_0_SRGB;

	so->texconst1 =
			A3XX_TEX_CONST_1_FETCHSIZE(fd3_pipe2fetchsize(cso->format)) |
			A3XX_TEX_CONST_1_WIDTH(prsc->width0) |
			A3XX_TEX_CONST_1_HEIGHT(prsc->height0);
	/* when emitted, A3XX_TEX_CONST_2_INDX() must be OR'd in: */
	so->texconst2 =
			A3XX_TEX_CONST_2_PITCH(rsc->slices[0].pitch * rsc->cpp);
	so->texconst3 = 0x00000000;  /* ??? */

	return &so->base;
}

void
fd3_texture_init(struct pipe_context *pctx)
{
	pctx->create_sampler_state = fd3_sampler_state_create;
	pctx->create_sampler_view = fd3_sampler_view_create;
}
