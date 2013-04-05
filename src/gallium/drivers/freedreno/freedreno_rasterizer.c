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

#include "freedreno_rasterizer.h"
#include "freedreno_context.h"
#include "freedreno_util.h"


static enum adreno_pa_su_sc_draw
polygon_mode(unsigned mode)
{
	switch (mode) {
	case PIPE_POLYGON_MODE_POINT:
		return PC_DRAW_POINTS;
	case PIPE_POLYGON_MODE_LINE:
		return PC_DRAW_LINES;
	case PIPE_POLYGON_MODE_FILL:
		return PC_DRAW_TRIANGLES;
	default:
		DBG("invalid polygon mode: %u", mode);
		return 0;
	}
}

static void *
fd_rasterizer_state_create(struct pipe_context *pctx,
		const struct pipe_rasterizer_state *cso)
{
	struct fd_rasterizer_stateobj *so;
	float psize_min, psize_max;

	so = CALLOC_STRUCT(fd_rasterizer_stateobj);
	if (!so)
		return NULL;

	if (cso->point_size_per_vertex) {
		psize_min = util_get_min_point_size(cso);
		psize_max = 8192;
	} else {
		/* Force the point size to be as if the vertex output was disabled. */
		psize_min = cso->point_size;
		psize_max = cso->point_size;
	}

	so->base = *cso;

	so->pa_sc_line_stipple = cso->line_stipple_enable ?
		A2XX_PA_SC_LINE_STIPPLE_LINE_PATTERN(cso->line_stipple_pattern) |
		A2XX_PA_SC_LINE_STIPPLE_REPEAT_COUNT(cso->line_stipple_factor) : 0;

	so->pa_cl_clip_cntl = 0; // TODO

	so->pa_su_vtx_cntl =
		A2XX_PA_SU_VTX_CNTL_PIX_CENTER(cso->gl_rasterization_rules ? PIXCENTER_OGL : PIXCENTER_D3D) |
		A2XX_PA_SU_VTX_CNTL_QUANT_MODE(ONE_SIXTEENTH);

	so->pa_su_point_size =
		A2XX_PA_SU_POINT_SIZE_HEIGHT(cso->point_size/2) |
		A2XX_PA_SU_POINT_SIZE_WIDTH(cso->point_size/2);

	so->pa_su_point_minmax =
		A2XX_PA_SU_POINT_MINMAX_MIN(psize_min/2) |
		A2XX_PA_SU_POINT_MINMAX_MAX(psize_max/2);

	so->pa_su_line_cntl =
		A2XX_PA_SU_LINE_CNTL_WIDTH(cso->line_width/2);

	so->pa_su_sc_mode_cntl =
		A2XX_PA_SU_SC_MODE_CNTL_VTX_WINDOW_OFFSET_ENABLE |
		A2XX_PA_SU_SC_MODE_CNTL_FRONT_PTYPE(polygon_mode(cso->fill_front)) |
		A2XX_PA_SU_SC_MODE_CNTL_BACK_PTYPE(polygon_mode(cso->fill_back));

	if (cso->cull_face & PIPE_FACE_FRONT)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_CULL_FRONT;
	if (cso->cull_face & PIPE_FACE_BACK)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_CULL_BACK;
	if (!cso->flatshade_first)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST;
	if (!cso->front_ccw)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_FACE;
	if (cso->line_stipple_enable)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_LINE_STIPPLE_ENABLE;
	if (cso->multisample)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_MSAA_ENABLE;

	if (cso->fill_front != PIPE_POLYGON_MODE_FILL ||
			cso->fill_back != PIPE_POLYGON_MODE_FILL)
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_POLYMODE(POLY_DUALMODE);
	else
		so->pa_su_sc_mode_cntl |= A2XX_PA_SU_SC_MODE_CNTL_POLYMODE(POLY_DISABLED);

	if (cso->offset_tri)
		so->pa_su_sc_mode_cntl |=
			A2XX_PA_SU_SC_MODE_CNTL_POLY_OFFSET_FRONT_ENABLE |
			A2XX_PA_SU_SC_MODE_CNTL_POLY_OFFSET_BACK_ENABLE |
			A2XX_PA_SU_SC_MODE_CNTL_POLY_OFFSET_PARA_ENABLE;

	return so;
}

static void
fd_rasterizer_state_bind(struct pipe_context *pctx, void *hwcso)
{
	struct fd_context *ctx = fd_context(pctx);
	ctx->rasterizer = hwcso;
	ctx->dirty |= FD_DIRTY_RASTERIZER;
}

static void
fd_rasterizer_state_delete(struct pipe_context *pctx, void *hwcso)
{
	FREE(hwcso);
}

void
fd_rasterizer_init(struct pipe_context *pctx)
{
	pctx->create_rasterizer_state = fd_rasterizer_state_create;
	pctx->bind_rasterizer_state = fd_rasterizer_state_bind;
	pctx->delete_rasterizer_state = fd_rasterizer_state_delete;
}
