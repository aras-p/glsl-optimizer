/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson
 */
#include <stdio.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include <util/u_memory.h>
#include <util/u_blitter.h>
#include "r600_resource.h"
#include "r600_screen.h"
#include "r600_context.h"

static void r600_destroy_context(struct pipe_context *context)
{
	struct r600_context *rctx = r600_context(context);

	FREE(rctx);
}

static void r600_flush(struct pipe_context *ctx, unsigned flags,
			struct pipe_fence_handle **fence)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_screen *rscreen = rctx->screen;
	static int dc = 0;

	if (radeon_ctx_pm4(rctx->ctx))
		return;
	/* FIXME dumping should be removed once shader support instructions
	 * without throwing bad code
	 */
	if (!dc)
		radeon_ctx_dump_bof(rctx->ctx, "gallium.bof");
#if 0
	radeon_ctx_submit(rctx->ctx);
#endif
	rctx->ctx = radeon_ctx_decref(rctx->ctx);
	rctx->ctx = radeon_ctx(rscreen->rw);
	dc++;
}

struct pipe_context *r600_create_context(struct pipe_screen *screen, void *priv)
{
	struct r600_context *rctx = CALLOC_STRUCT(r600_context);
	struct r600_screen* rscreen = r600_screen(screen);

	if (rctx == NULL)
		return NULL;
	rctx->context.winsys = rscreen->screen.winsys;
	rctx->context.screen = screen;
	rctx->context.priv = priv;
	rctx->context.destroy = r600_destroy_context;
	rctx->context.draw_arrays = r600_draw_arrays;
	rctx->context.draw_elements = r600_draw_elements;
	rctx->context.draw_range_elements = r600_draw_range_elements;
	rctx->context.flush = r600_flush;

	/* Easy accessing of screen/winsys. */
	rctx->screen = rscreen;
	rctx->rw = rscreen->rw;

	r600_init_blit_functions(rctx);
	r600_init_query_functions(rctx);
	r600_init_state_functions(rctx);
	r600_init_context_resource_functions(rctx);

	rctx->blitter = util_blitter_create(&rctx->context);
	if (rctx->blitter == NULL) {
		FREE(rctx);
		return NULL;
	}

	rctx->cb_cntl = radeon_state(rscreen->rw, R600_CB_CNTL_TYPE, R600_CB_CNTL);
	rctx->cb_cntl->states[R600_CB_CNTL__CB_SHADER_MASK] = 0x0000000F;
	rctx->cb_cntl->states[R600_CB_CNTL__CB_TARGET_MASK] = 0x0000000F;
	rctx->cb_cntl->states[R600_CB_CNTL__CB_COLOR_CONTROL] = 0x00CC0000;
	rctx->cb_cntl->states[R600_CB_CNTL__PA_SC_AA_CONFIG] = 0x00000000;
	rctx->cb_cntl->states[R600_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_MCTX] = 0x00000000;
	rctx->cb_cntl->states[R600_CB_CNTL__PA_SC_AA_SAMPLE_LOCS_8S_WD1_MCTX] = 0x00000000;
	rctx->cb_cntl->states[R600_CB_CNTL__CB_CLRCMP_CONTROL] = 0x01000000;
	rctx->cb_cntl->states[R600_CB_CNTL__CB_CLRCMP_SRC] = 0x00000000;
	rctx->cb_cntl->states[R600_CB_CNTL__CB_CLRCMP_DST] = 0x000000FF;
	rctx->cb_cntl->states[R600_CB_CNTL__CB_CLRCMP_MSK] = 0xFFFFFFFF;
	rctx->cb_cntl->states[R600_CB_CNTL__PA_SC_AA_MASK] = 0xFFFFFFFF;
	radeon_state_pm4(rctx->cb_cntl);

	rctx->config = radeon_state(rscreen->rw, R600_CONFIG_TYPE, R600_CONFIG);
	rctx->config->states[R600_CONFIG__SQ_CONFIG] = 0xE400000C;
	rctx->config->states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_1] = 0x403800C0;
	rctx->config->states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_2] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_THREAD_RESOURCE_MGMT] = 0x00003090;
	rctx->config->states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_1] = 0x00800080;
	rctx->config->states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_2] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0x00004000;
	rctx->config->states[R600_CONFIG__TA_CNTL_AUX] = 0x07000002;
	rctx->config->states[R600_CONFIG__VC_ENHANCE] = 0x00000000;
	rctx->config->states[R600_CONFIG__DB_DEBUG] = 0x00000000;
	rctx->config->states[R600_CONFIG__DB_WATERMARKS] = 0x00420204;
	rctx->config->states[R600_CONFIG__SX_MISC] = 0x00000000;
	rctx->config->states[R600_CONFIG__SPI_THREAD_GROUPING] = 0x00000001;
	rctx->config->states[R600_CONFIG__CB_SHADER_CONTROL] = 0x00000003;
	rctx->config->states[R600_CONFIG__SQ_ESGS_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_GSVS_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_ESTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_GSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_VSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_PSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_FBUF_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_REDUC_RING_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__SQ_GS_VERT_ITEMSIZE] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_OUTPUT_PATH_CNTL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_HOS_CNTL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_HOS_MAX_TESS_LEVEL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_HOS_MIN_TESS_LEVEL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_HOS_REUSE_DEPTH] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_PRIM_TYPE] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_FIRST_DECR] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_DECR] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_VECT_0_CNTL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_VECT_1_CNTL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_VECT_0_FMT_CNTL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GROUP_VECT_1_FMT_CNTL] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_GS_MODE] = 0x00000000;
	rctx->config->states[R600_CONFIG__PA_SC_MODE_CNTL] = 0x00514000;
	rctx->config->states[R600_CONFIG__VGT_STRMOUT_EN] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_REUSE_OFF] = 0x00000001;
	rctx->config->states[R600_CONFIG__VGT_VTX_CNT_EN] = 0x00000000;
	rctx->config->states[R600_CONFIG__VGT_STRMOUT_BUFFER_EN] = 0x00000000;
	radeon_state_pm4(rctx->config);

	rctx->ctx = radeon_ctx(rscreen->rw);
	rctx->draw = radeon_draw(rscreen->rw);
	return &rctx->context;
}
