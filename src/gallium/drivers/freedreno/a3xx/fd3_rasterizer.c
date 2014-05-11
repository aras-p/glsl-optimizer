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

#include "fd3_rasterizer.h"
#include "fd3_context.h"
#include "fd3_util.h"

void *
fd3_rasterizer_state_create(struct pipe_context *pctx,
		const struct pipe_rasterizer_state *cso)
{
	struct fd3_rasterizer_stateobj *so;
	float psize_min, psize_max;

	so = CALLOC_STRUCT(fd3_rasterizer_stateobj);
	if (!so)
		return NULL;

	so->base = *cso;

	if (cso->point_size_per_vertex) {
		psize_min = util_get_min_point_size(cso);
		psize_max = 8192;
	} else {
		/* Force the point size to be as if the vertex output was disabled. */
		psize_min = cso->point_size;
		psize_max = cso->point_size;
	}

/*
	if (cso->line_stipple_enable) {
		??? TODO line stipple
	}
	TODO cso->half_pixel_center
	if (cso->multisample)
		TODO
*/
	so->gras_cl_clip_cntl = A3XX_GRAS_CL_CLIP_CNTL_IJ_PERSP_CENTER; /* ??? */
	so->gras_su_point_minmax =
			A3XX_GRAS_SU_POINT_MINMAX_MIN(psize_min/2) |
			A3XX_GRAS_SU_POINT_MINMAX_MAX(psize_max/2);
	so->gras_su_point_size   = A3XX_GRAS_SU_POINT_SIZE(cso->point_size/2);
	so->gras_su_poly_offset_scale =
			A3XX_GRAS_SU_POLY_OFFSET_SCALE_VAL(cso->offset_scale);
	so->gras_su_poly_offset_offset =
			A3XX_GRAS_SU_POLY_OFFSET_OFFSET(cso->offset_units);

	so->gras_su_mode_control =
			A3XX_GRAS_SU_MODE_CONTROL_LINEHALFWIDTH(cso->line_width/2.0);

	so->pc_prim_vtx_cntl =
		A3XX_PC_PRIM_VTX_CNTL_POLYMODE_FRONT_PTYPE(fd_polygon_mode(cso->fill_front)) |
		A3XX_PC_PRIM_VTX_CNTL_POLYMODE_BACK_PTYPE(fd_polygon_mode(cso->fill_back));

	if (cso->cull_face & PIPE_FACE_FRONT)
		so->gras_su_mode_control |= A3XX_GRAS_SU_MODE_CONTROL_CULL_FRONT;
	if (cso->cull_face & PIPE_FACE_BACK)
		so->gras_su_mode_control |= A3XX_GRAS_SU_MODE_CONTROL_CULL_BACK;
	if (!cso->front_ccw)
		so->gras_su_mode_control |= A3XX_GRAS_SU_MODE_CONTROL_FRONT_CW;
	if (!cso->flatshade_first)
		so->pc_prim_vtx_cntl |= A3XX_PC_PRIM_VTX_CNTL_PROVOKING_VTX_LAST;

	if (cso->offset_tri)
		so->gras_su_mode_control |= A3XX_GRAS_SU_MODE_CONTROL_POLY_OFFSET;

	return so;
}
