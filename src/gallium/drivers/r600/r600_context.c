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
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_resource.h"
#include "r600d.h"


static void r600_destroy_context(struct pipe_context *context)
{
	struct r600_context *rctx = r600_context(context);

	FREE(rctx);
}

void r600_flush(struct pipe_context *ctx, unsigned flags,
			struct pipe_fence_handle **fence)
{
	struct r600_context *rctx = r600_context(ctx);
	struct r600_query *rquery;
	static int dc = 0;
	char dname[256];

	/* suspend queries */
	r600_queries_suspend(ctx);
	/* FIXME dumping should be removed once shader support instructions
	 * without throwing bad code
	 */
	if (!rctx->ctx.cdwords)
		goto out;
#if 0
	sprintf(dname, "gallium-%08d.bof", dc);
	if (dc < 2) {
		radeon_ctx_dump_bof(&rctx->ctx, dname);
		R600_ERR("dumped %s\n", dname);
	}
#endif
#if 1
	radeon_ctx_submit(&rctx->ctx);
#endif
	LIST_FOR_EACH_ENTRY(rquery, &rctx->query_list, list) {
		rquery->flushed = true;
	}
	dc++;
out:
	radeon_ctx_clear(&rctx->ctx);
	/* resume queries */
	r600_queries_resume(ctx);
}

static void r600_init_config(struct r600_context *rctx)
{
	int ps_prio;
	int vs_prio;
	int gs_prio;
	int es_prio;
	int num_ps_gprs;
	int num_vs_gprs;
	int num_gs_gprs;
	int num_es_gprs;
	int num_temp_gprs;
	int num_ps_threads;
	int num_vs_threads;
	int num_gs_threads;
	int num_es_threads;
	int num_ps_stack_entries;
	int num_vs_stack_entries;
	int num_gs_stack_entries;
	int num_es_stack_entries;
	enum radeon_family family;

	family = radeon_get_family(rctx->rw);
	ps_prio = 0;
	vs_prio = 1;
	gs_prio = 2;
	es_prio = 3;
	switch (family) {
	case CHIP_R600:
		num_ps_gprs = 192;
		num_vs_gprs = 56;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 136;
		num_vs_threads = 48;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 128;
		num_vs_stack_entries = 128;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	case CHIP_RV630:
	case CHIP_RV635:
		num_ps_gprs = 84;
		num_vs_gprs = 36;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 144;
		num_vs_threads = 40;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 40;
		num_vs_stack_entries = 40;
		num_gs_stack_entries = 32;
		num_es_stack_entries = 16;
		break;
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
	default:
		num_ps_gprs = 84;
		num_vs_gprs = 36;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 136;
		num_vs_threads = 48;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 40;
		num_vs_stack_entries = 40;
		num_gs_stack_entries = 32;
		num_es_stack_entries = 16;
		break;
	case CHIP_RV670:
		num_ps_gprs = 144;
		num_vs_gprs = 40;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 136;
		num_vs_threads = 48;
		num_gs_threads = 4;
		num_es_threads = 4;
		num_ps_stack_entries = 40;
		num_vs_stack_entries = 40;
		num_gs_stack_entries = 32;
		num_es_stack_entries = 16;
		break;
	case CHIP_RV770:
		num_ps_gprs = 192;
		num_vs_gprs = 56;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 188;
		num_vs_threads = 60;
		num_gs_threads = 0;
		num_es_threads = 0;
		num_ps_stack_entries = 256;
		num_vs_stack_entries = 256;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	case CHIP_RV730:
	case CHIP_RV740:
		num_ps_gprs = 84;
		num_vs_gprs = 36;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 188;
		num_vs_threads = 60;
		num_gs_threads = 0;
		num_es_threads = 0;
		num_ps_stack_entries = 128;
		num_vs_stack_entries = 128;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	case CHIP_RV710:
		num_ps_gprs = 192;
		num_vs_gprs = 56;
		num_temp_gprs = 4;
		num_gs_gprs = 0;
		num_es_gprs = 0;
		num_ps_threads = 144;
		num_vs_threads = 48;
		num_gs_threads = 0;
		num_es_threads = 0;
		num_ps_stack_entries = 128;
		num_vs_stack_entries = 128;
		num_gs_stack_entries = 0;
		num_es_stack_entries = 0;
		break;
	}
	radeon_state_init(&rctx->config, rctx->rw, R600_STATE_CONFIG, 0, 0);

	rctx->config.states[R600_CONFIG__SQ_CONFIG] = 0x00000000;
	switch (family) {
	case CHIP_RV610:
	case CHIP_RV620:
	case CHIP_RS780:
	case CHIP_RS880:
	case CHIP_RV710:
		break;
	default:
		rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_VC_ENABLE(1);
		break;
	}
	rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_DX9_CONSTS(1);
	rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_ALU_INST_PREFER_VECTOR(1);
	rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_PS_PRIO(ps_prio);
	rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_VS_PRIO(vs_prio);
	rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_GS_PRIO(gs_prio);
	rctx->config.states[R600_CONFIG__SQ_CONFIG] |= S_008C00_ES_PRIO(es_prio);

	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_1] = 0;
	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_1] |= S_008C04_NUM_PS_GPRS(num_ps_gprs);
	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_1] |= S_008C04_NUM_VS_GPRS(num_vs_gprs);
	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_1] |= S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs);

	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_2] = 0;
	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_2] |= S_008C08_NUM_GS_GPRS(num_gs_gprs);
	rctx->config.states[R600_CONFIG__SQ_GPR_RESOURCE_MGMT_2] |= S_008C08_NUM_GS_GPRS(num_es_gprs);

	rctx->config.states[R600_CONFIG__SQ_THREAD_RESOURCE_MGMT] = 0;
	rctx->config.states[R600_CONFIG__SQ_THREAD_RESOURCE_MGMT] |= S_008C0C_NUM_PS_THREADS(num_ps_threads);
	rctx->config.states[R600_CONFIG__SQ_THREAD_RESOURCE_MGMT] |= S_008C0C_NUM_VS_THREADS(num_vs_threads);
	rctx->config.states[R600_CONFIG__SQ_THREAD_RESOURCE_MGMT] |= S_008C0C_NUM_GS_THREADS(num_gs_threads);
	rctx->config.states[R600_CONFIG__SQ_THREAD_RESOURCE_MGMT] |= S_008C0C_NUM_ES_THREADS(num_es_threads);

	rctx->config.states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_1] = 0;
	rctx->config.states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_1] |= S_008C10_NUM_PS_STACK_ENTRIES(num_ps_stack_entries);
	rctx->config.states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_1] |= S_008C10_NUM_VS_STACK_ENTRIES(num_vs_stack_entries);

	rctx->config.states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_2] = 0;
	rctx->config.states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_2] |= S_008C14_NUM_GS_STACK_ENTRIES(num_gs_stack_entries);
	rctx->config.states[R600_CONFIG__SQ_STACK_RESOURCE_MGMT_2] |= S_008C14_NUM_ES_STACK_ENTRIES(num_es_stack_entries);

	rctx->config.states[R600_CONFIG__VC_ENHANCE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SX_MISC] = 0x00000000;

	if (family >= CHIP_RV770) {
		rctx->config.states[R600_CONFIG__SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0x00004000;
		rctx->config.states[R600_CONFIG__TA_CNTL_AUX] = 0x07000002;
		rctx->config.states[R600_CONFIG__DB_DEBUG] = 0x00000000;
		rctx->config.states[R600_CONFIG__DB_WATERMARKS] = 0x00420204;
		rctx->config.states[R600_CONFIG__SPI_THREAD_GROUPING] = 0x00000000;
		rctx->config.states[R600_CONFIG__PA_SC_MODE_CNTL] = 0x00514000;
	} else {
		rctx->config.states[R600_CONFIG__SQ_DYN_GPR_CNTL_PS_FLUSH_REQ] = 0x00000000;
		rctx->config.states[R600_CONFIG__TA_CNTL_AUX] = 0x07000003;
		rctx->config.states[R600_CONFIG__DB_DEBUG] = 0x82000000;
		rctx->config.states[R600_CONFIG__DB_WATERMARKS] = 0x01020204;
		rctx->config.states[R600_CONFIG__SPI_THREAD_GROUPING] = 0x00000001;
		rctx->config.states[R600_CONFIG__PA_SC_MODE_CNTL] = 0x00004010;
	}
	rctx->config.states[R600_CONFIG__CB_SHADER_CONTROL] = 0x00000003;
	rctx->config.states[R600_CONFIG__SQ_ESGS_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_GSVS_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_ESTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_GSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_VSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_PSTMP_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_FBUF_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_REDUC_RING_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__SQ_GS_VERT_ITEMSIZE] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_OUTPUT_PATH_CNTL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_HOS_CNTL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_HOS_MAX_TESS_LEVEL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_HOS_MIN_TESS_LEVEL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_HOS_REUSE_DEPTH] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_PRIM_TYPE] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_FIRST_DECR] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_DECR] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_VECT_0_CNTL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_VECT_1_CNTL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_VECT_0_FMT_CNTL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GROUP_VECT_1_FMT_CNTL] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_GS_MODE] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_STRMOUT_EN] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_REUSE_OFF] = 0x00000001;
	rctx->config.states[R600_CONFIG__VGT_VTX_CNT_EN] = 0x00000000;
	rctx->config.states[R600_CONFIG__VGT_STRMOUT_BUFFER_EN] = 0x00000000;
	radeon_state_pm4(&rctx->config);
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
	rctx->context.draw_vbo = r600_draw_vbo;
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

	r600_init_config(rctx);

	radeon_ctx_init(&rctx->ctx, rscreen->rw);
	radeon_draw_init(&rctx->draw, rscreen->rw);
	return &rctx->context;
}
