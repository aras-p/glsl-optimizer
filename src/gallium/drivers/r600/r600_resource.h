/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef R600_RESOURCE_H
#define R600_RESOURCE_H

#include "../../winsys/radeon/drm/radeon_winsys.h"
#include "../radeon/r600_pipe_common.h"

struct r600_screen;
struct compute_memory_item;

struct r600_resource_global {
	struct r600_resource base;
	struct compute_memory_item *chunk;
};

struct r600_surface {
	struct pipe_surface		base;

	bool color_initialized;
	bool depth_initialized;

	/* Misc. color flags. */
	bool alphatest_bypass;
	bool export_16bpc;

	/* Color registers. */
	unsigned cb_color_info;
	unsigned cb_color_base;
	unsigned cb_color_view;
	unsigned cb_color_size;		/* R600 only */
	unsigned cb_color_dim;		/* EG only */
	unsigned cb_color_pitch;	/* EG only */
	unsigned cb_color_slice;	/* EG only */
	unsigned cb_color_attrib;	/* EG only */
	unsigned cb_color_fmask;	/* CB_COLORn_FMASK (EG) or CB_COLORn_FRAG (r600) */
	unsigned cb_color_fmask_slice;	/* EG only */
	unsigned cb_color_cmask;	/* CB_COLORn_CMASK (EG) or CB_COLORn_TILE (r600) */
	unsigned cb_color_cmask_slice;	/* EG only */
	unsigned cb_color_mask;		/* R600 only */
	struct r600_resource *cb_buffer_fmask; /* Used for FMASK relocations. R600 only */
	struct r600_resource *cb_buffer_cmask; /* Used for CMASK relocations. R600 only */

	/* DB registers. */
	unsigned db_depth_info;		/* DB_Z_INFO (EG) or DB_DEPTH_INFO (r600) */
	unsigned db_depth_base;		/* DB_Z_READ/WRITE_BASE (EG) or DB_DEPTH_BASE (r600) */
	unsigned db_depth_view;
	unsigned db_depth_size;
	unsigned db_depth_slice;	/* EG only */
	unsigned db_stencil_base;	/* EG only */
	unsigned db_stencil_info;	/* EG only */
	unsigned db_prefetch_limit;	/* R600 only */
	unsigned pa_su_poly_offset_db_fmt_cntl;

	unsigned			htile_enabled;
	unsigned			db_htile_surface;
	unsigned			db_htile_data_base;
	unsigned			db_preload_control;
};

/* Return if the depth format can be read without the DB->CB copy on r6xx-r7xx. */
static INLINE bool r600_can_read_depth(struct r600_texture *rtex)
{
	return rtex->resource.b.b.nr_samples <= 1 &&
	       (rtex->resource.b.b.format == PIPE_FORMAT_Z16_UNORM ||
		rtex->resource.b.b.format == PIPE_FORMAT_Z32_FLOAT);
}

void r600_resource_destroy(struct pipe_screen *screen, struct pipe_resource *res);
void r600_init_screen_resource_functions(struct pipe_screen *screen);

#endif
