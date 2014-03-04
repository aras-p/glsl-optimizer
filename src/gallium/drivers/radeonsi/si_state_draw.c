/*
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 *      Christian König <christian.koenig@amd.com>
 */

#include "si_pipe.h"
#include "si_shader.h"
#include "../radeon/r600_cs.h"
#include "sid.h"

#include "util/u_blitter.h"
#include "util/u_format.h"
#include "util/u_index_modify.h"
#include "util/u_memory.h"
#include "util/u_upload_mgr.h"

/*
 * Shaders
 */

static void si_pipe_shader_es(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_pm4_state *pm4;
	unsigned num_sgprs, num_user_sgprs;
	unsigned vgpr_comp_cnt;
	uint64_t va;

	si_pm4_delete_state(sctx, es, shader->pm4);
	pm4 = shader->pm4 = si_pm4_alloc_state(sctx);

	if (pm4 == NULL)
		return;

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ, RADEON_PRIO_SHADER_DATA);

	vgpr_comp_cnt = shader->shader.uses_instanceid ? 3 : 0;

	num_user_sgprs = SI_VS_NUM_USER_SGPR;
	num_sgprs = shader->num_sgprs;
	/* One SGPR after user SGPRs is pre-loaded with es2gs_offset */
	if ((num_user_sgprs + 1) > num_sgprs) {
		/* Last 2 reserved SGPRs are used for VCC */
		num_sgprs = num_user_sgprs + 1 + 2;
	}
	assert(num_sgprs <= 104);

	si_pm4_set_reg(pm4, R_00B320_SPI_SHADER_PGM_LO_ES, va >> 8);
	si_pm4_set_reg(pm4, R_00B324_SPI_SHADER_PGM_HI_ES, va >> 40);
	si_pm4_set_reg(pm4, R_00B328_SPI_SHADER_PGM_RSRC1_ES,
		       S_00B328_VGPRS((shader->num_vgprs - 1) / 4) |
		       S_00B328_SGPRS((num_sgprs - 1) / 8) |
		       S_00B328_VGPR_COMP_CNT(vgpr_comp_cnt));
	si_pm4_set_reg(pm4, R_00B32C_SPI_SHADER_PGM_RSRC2_ES,
		       S_00B32C_USER_SGPR(num_user_sgprs));

	sctx->b.flags |= R600_CONTEXT_INV_SHADER_CACHE;
}

static void si_pipe_shader_gs(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct si_context *sctx = (struct si_context *)ctx;
	unsigned gs_vert_itemsize = shader->shader.noutput * (16 >> 2);
	unsigned gs_max_vert_out = shader->shader.gs_max_out_vertices;
	unsigned gsvs_itemsize = gs_vert_itemsize * gs_max_vert_out;
	unsigned cut_mode;
	struct si_pm4_state *pm4;
	unsigned num_sgprs, num_user_sgprs;
	uint64_t va;

	/* The GSVS_RING_ITEMSIZE register takes 15 bits */
	assert(gsvs_itemsize < (1 << 15));

	si_pm4_delete_state(sctx, gs, shader->pm4);
	pm4 = shader->pm4 = si_pm4_alloc_state(sctx);

	if (pm4 == NULL)
		return;

	if (gs_max_vert_out <= 128) {
		cut_mode = V_028A40_GS_CUT_128;
	} else if (gs_max_vert_out <= 256) {
		cut_mode = V_028A40_GS_CUT_256;
	} else if (gs_max_vert_out <= 512) {
		cut_mode = V_028A40_GS_CUT_512;
	} else {
		assert(gs_max_vert_out <= 1024);
		cut_mode = V_028A40_GS_CUT_1024;
	}

	si_pm4_set_reg(pm4, R_028A40_VGT_GS_MODE,
		       S_028A40_MODE(V_028A40_GS_SCENARIO_G) |
		       S_028A40_CUT_MODE(cut_mode)|
		       S_028A40_ES_WRITE_OPTIMIZE(1) |
		       S_028A40_GS_WRITE_OPTIMIZE(1));

	si_pm4_set_reg(pm4, R_028A60_VGT_GSVS_RING_OFFSET_1, gsvs_itemsize);
	si_pm4_set_reg(pm4, R_028A64_VGT_GSVS_RING_OFFSET_2, gsvs_itemsize);
	si_pm4_set_reg(pm4, R_028A68_VGT_GSVS_RING_OFFSET_3, gsvs_itemsize);

	si_pm4_set_reg(pm4, R_028AAC_VGT_ESGS_RING_ITEMSIZE,
		       shader->shader.nparam * (16 >> 2));
	si_pm4_set_reg(pm4, R_028AB0_VGT_GSVS_RING_ITEMSIZE, gsvs_itemsize);

	si_pm4_set_reg(pm4, R_028B38_VGT_GS_MAX_VERT_OUT, gs_max_vert_out);

	si_pm4_set_reg(pm4, R_028B5C_VGT_GS_VERT_ITEMSIZE, gs_vert_itemsize);

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ, RADEON_PRIO_SHADER_DATA);
	si_pm4_set_reg(pm4, R_00B220_SPI_SHADER_PGM_LO_GS, va >> 8);
	si_pm4_set_reg(pm4, R_00B224_SPI_SHADER_PGM_HI_GS, va >> 40);

	num_user_sgprs = SI_GS_NUM_USER_SGPR;
	num_sgprs = shader->num_sgprs;
	/* Two SGPRs after user SGPRs are pre-loaded with gs2vs_offset, gs_wave_id */
	if ((num_user_sgprs + 2) > num_sgprs) {
		/* Last 2 reserved SGPRs are used for VCC */
		num_sgprs = num_user_sgprs + 2 + 2;
	}
	assert(num_sgprs <= 104);

	si_pm4_set_reg(pm4, R_00B228_SPI_SHADER_PGM_RSRC1_GS,
		       S_00B228_VGPRS((shader->num_vgprs - 1) / 4) |
		       S_00B228_SGPRS((num_sgprs - 1) / 8));
	si_pm4_set_reg(pm4, R_00B22C_SPI_SHADER_PGM_RSRC2_GS,
		       S_00B22C_USER_SGPR(num_user_sgprs));

	sctx->b.flags |= R600_CONTEXT_INV_SHADER_CACHE;
}

static void si_pipe_shader_vs(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_pm4_state *pm4;
	unsigned num_sgprs, num_user_sgprs;
	unsigned nparams, i, vgpr_comp_cnt;
	uint64_t va;

	si_pm4_delete_state(sctx, vs, shader->pm4);
	pm4 = shader->pm4 = si_pm4_alloc_state(sctx);

	if (pm4 == NULL)
		return;

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ, RADEON_PRIO_SHADER_DATA);

	vgpr_comp_cnt = shader->shader.uses_instanceid ? 3 : 0;

	num_user_sgprs = SI_VS_NUM_USER_SGPR;
	num_sgprs = shader->num_sgprs;
	if (num_user_sgprs > num_sgprs) {
		/* Last 2 reserved SGPRs are used for VCC */
		num_sgprs = num_user_sgprs + 2;
	}
	assert(num_sgprs <= 104);

	/* Certain attributes (position, psize, etc.) don't count as params.
	 * VS is required to export at least one param and r600_shader_from_tgsi()
	 * takes care of adding a dummy export.
	 */
	for (nparams = 0, i = 0 ; i < shader->shader.noutput; i++) {
		switch (shader->shader.output[i].name) {
		case TGSI_SEMANTIC_CLIPVERTEX:
		case TGSI_SEMANTIC_POSITION:
		case TGSI_SEMANTIC_PSIZE:
			break;
		default:
			nparams++;
		}
	}
	if (nparams < 1)
		nparams = 1;

	si_pm4_set_reg(pm4, R_0286C4_SPI_VS_OUT_CONFIG,
		       S_0286C4_VS_EXPORT_COUNT(nparams - 1));

	si_pm4_set_reg(pm4, R_02870C_SPI_SHADER_POS_FORMAT,
		       S_02870C_POS0_EXPORT_FORMAT(V_02870C_SPI_SHADER_4COMP) |
		       S_02870C_POS1_EXPORT_FORMAT(shader->shader.nr_pos_exports > 1 ?
						   V_02870C_SPI_SHADER_4COMP :
						   V_02870C_SPI_SHADER_NONE) |
		       S_02870C_POS2_EXPORT_FORMAT(shader->shader.nr_pos_exports > 2 ?
						   V_02870C_SPI_SHADER_4COMP :
						   V_02870C_SPI_SHADER_NONE) |
		       S_02870C_POS3_EXPORT_FORMAT(shader->shader.nr_pos_exports > 3 ?
						   V_02870C_SPI_SHADER_4COMP :
						   V_02870C_SPI_SHADER_NONE));

	si_pm4_set_reg(pm4, R_00B120_SPI_SHADER_PGM_LO_VS, va >> 8);
	si_pm4_set_reg(pm4, R_00B124_SPI_SHADER_PGM_HI_VS, va >> 40);
	si_pm4_set_reg(pm4, R_00B128_SPI_SHADER_PGM_RSRC1_VS,
		       S_00B128_VGPRS((shader->num_vgprs - 1) / 4) |
		       S_00B128_SGPRS((num_sgprs - 1) / 8) |
		       S_00B128_VGPR_COMP_CNT(vgpr_comp_cnt));
	si_pm4_set_reg(pm4, R_00B12C_SPI_SHADER_PGM_RSRC2_VS,
		       S_00B12C_USER_SGPR(num_user_sgprs) |
		       S_00B12C_SO_BASE0_EN(!!shader->selector->so.stride[0]) |
		       S_00B12C_SO_BASE1_EN(!!shader->selector->so.stride[1]) |
		       S_00B12C_SO_BASE2_EN(!!shader->selector->so.stride[2]) |
		       S_00B12C_SO_BASE3_EN(!!shader->selector->so.stride[3]) |
		       S_00B12C_SO_EN(!!shader->selector->so.num_outputs));

	sctx->b.flags |= R600_CONTEXT_INV_SHADER_CACHE;
}

static void si_pipe_shader_ps(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_pm4_state *pm4;
	unsigned i, exports_ps, spi_ps_in_control, db_shader_control;
	unsigned num_sgprs, num_user_sgprs;
	unsigned spi_baryc_cntl = 0, spi_ps_input_ena, spi_shader_z_format;
	uint64_t va;

	si_pm4_delete_state(sctx, ps, shader->pm4);
	pm4 = shader->pm4 = si_pm4_alloc_state(sctx);

	if (pm4 == NULL)
		return;

	db_shader_control = S_02880C_Z_ORDER(V_02880C_EARLY_Z_THEN_LATE_Z) |
			    S_02880C_ALPHA_TO_MASK_DISABLE(sctx->framebuffer.cb0_is_integer);

	for (i = 0; i < shader->shader.ninput; i++) {
		switch (shader->shader.input[i].name) {
		case TGSI_SEMANTIC_POSITION:
			if (shader->shader.input[i].centroid) {
				/* SPI_BARYC_CNTL.POS_FLOAT_LOCATION
				 * Possible vaules:
				 * 0 -> Position = pixel center (default)
				 * 1 -> Position = pixel centroid
				 * 2 -> Position = iterated sample number XXX:
				 *                        What does this mean?
			 	 */
				spi_baryc_cntl |= S_0286E0_POS_FLOAT_LOCATION(1);
			}
			/* Fall through */
		case TGSI_SEMANTIC_FACE:
			continue;
		}
	}

	for (i = 0; i < shader->shader.noutput; i++) {
		if (shader->shader.output[i].name == TGSI_SEMANTIC_POSITION)
			db_shader_control |= S_02880C_Z_EXPORT_ENABLE(1);
		if (shader->shader.output[i].name == TGSI_SEMANTIC_STENCIL)
			db_shader_control |= S_02880C_STENCIL_TEST_VAL_EXPORT_ENABLE(1);
	}
	if (shader->shader.uses_kill || shader->key.ps.alpha_func != PIPE_FUNC_ALWAYS)
		db_shader_control |= S_02880C_KILL_ENABLE(1);

	exports_ps = 0;
	for (i = 0; i < shader->shader.noutput; i++) {
		if (shader->shader.output[i].name == TGSI_SEMANTIC_POSITION ||
		    shader->shader.output[i].name == TGSI_SEMANTIC_STENCIL)
			exports_ps |= 1;
	}
	if (!exports_ps) {
		/* always at least export 1 component per pixel */
		exports_ps = 2;
	}

	spi_ps_in_control = S_0286D8_NUM_INTERP(shader->shader.nparam) |
		S_0286D8_BC_OPTIMIZE_DISABLE(1);

	si_pm4_set_reg(pm4, R_0286E0_SPI_BARYC_CNTL, spi_baryc_cntl);
	spi_ps_input_ena = shader->spi_ps_input_ena;
	/* we need to enable at least one of them, otherwise we hang the GPU */
	assert(G_0286CC_PERSP_SAMPLE_ENA(spi_ps_input_ena) ||
	    G_0286CC_PERSP_CENTER_ENA(spi_ps_input_ena) ||
	    G_0286CC_PERSP_CENTROID_ENA(spi_ps_input_ena) ||
	    G_0286CC_PERSP_PULL_MODEL_ENA(spi_ps_input_ena) ||
	    G_0286CC_LINEAR_SAMPLE_ENA(spi_ps_input_ena) ||
	    G_0286CC_LINEAR_CENTER_ENA(spi_ps_input_ena) ||
	    G_0286CC_LINEAR_CENTROID_ENA(spi_ps_input_ena) ||
	    G_0286CC_LINE_STIPPLE_TEX_ENA(spi_ps_input_ena));

	si_pm4_set_reg(pm4, R_0286CC_SPI_PS_INPUT_ENA, spi_ps_input_ena);
	si_pm4_set_reg(pm4, R_0286D0_SPI_PS_INPUT_ADDR, spi_ps_input_ena);
	si_pm4_set_reg(pm4, R_0286D8_SPI_PS_IN_CONTROL, spi_ps_in_control);

	if (G_02880C_STENCIL_TEST_VAL_EXPORT_ENABLE(db_shader_control))
		spi_shader_z_format = V_028710_SPI_SHADER_32_GR;
	else if (G_02880C_Z_EXPORT_ENABLE(db_shader_control))
		spi_shader_z_format = V_028710_SPI_SHADER_32_R;
	else
		spi_shader_z_format = 0;
	si_pm4_set_reg(pm4, R_028710_SPI_SHADER_Z_FORMAT, spi_shader_z_format);
	si_pm4_set_reg(pm4, R_028714_SPI_SHADER_COL_FORMAT,
		       shader->spi_shader_col_format);
	si_pm4_set_reg(pm4, R_02823C_CB_SHADER_MASK, shader->cb_shader_mask);

	va = r600_resource_va(ctx->screen, (void *)shader->bo);
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ, RADEON_PRIO_SHADER_DATA);
	si_pm4_set_reg(pm4, R_00B020_SPI_SHADER_PGM_LO_PS, va >> 8);
	si_pm4_set_reg(pm4, R_00B024_SPI_SHADER_PGM_HI_PS, va >> 40);

	num_user_sgprs = SI_PS_NUM_USER_SGPR;
	num_sgprs = shader->num_sgprs;
	/* One SGPR after user SGPRs is pre-loaded with {prim_mask, lds_offset} */
	if ((num_user_sgprs + 1) > num_sgprs) {
		/* Last 2 reserved SGPRs are used for VCC */
		num_sgprs = num_user_sgprs + 1 + 2;
	}
	assert(num_sgprs <= 104);

	si_pm4_set_reg(pm4, R_00B028_SPI_SHADER_PGM_RSRC1_PS,
		       S_00B028_VGPRS((shader->num_vgprs - 1) / 4) |
		       S_00B028_SGPRS((num_sgprs - 1) / 8));
	si_pm4_set_reg(pm4, R_00B02C_SPI_SHADER_PGM_RSRC2_PS,
		       S_00B02C_EXTRA_LDS_SIZE(shader->lds_size) |
		       S_00B02C_USER_SGPR(num_user_sgprs));

	si_pm4_set_reg(pm4, R_02880C_DB_SHADER_CONTROL, db_shader_control);

	shader->cb0_is_integer = sctx->framebuffer.cb0_is_integer;
	shader->sprite_coord_enable = sctx->sprite_coord_enable;
	sctx->b.flags |= R600_CONTEXT_INV_SHADER_CACHE;
}

/*
 * Drawing
 */

static unsigned si_conv_pipe_prim(unsigned pprim)
{
        static const unsigned prim_conv[] = {
		[PIPE_PRIM_POINTS]			= V_008958_DI_PT_POINTLIST,
		[PIPE_PRIM_LINES]			= V_008958_DI_PT_LINELIST,
		[PIPE_PRIM_LINE_LOOP]			= V_008958_DI_PT_LINELOOP,
		[PIPE_PRIM_LINE_STRIP]			= V_008958_DI_PT_LINESTRIP,
		[PIPE_PRIM_TRIANGLES]			= V_008958_DI_PT_TRILIST,
		[PIPE_PRIM_TRIANGLE_STRIP]		= V_008958_DI_PT_TRISTRIP,
		[PIPE_PRIM_TRIANGLE_FAN]		= V_008958_DI_PT_TRIFAN,
		[PIPE_PRIM_QUADS]			= V_008958_DI_PT_QUADLIST,
		[PIPE_PRIM_QUAD_STRIP]			= V_008958_DI_PT_QUADSTRIP,
		[PIPE_PRIM_POLYGON]			= V_008958_DI_PT_POLYGON,
		[PIPE_PRIM_LINES_ADJACENCY]		= V_008958_DI_PT_LINELIST_ADJ,
		[PIPE_PRIM_LINE_STRIP_ADJACENCY]	= V_008958_DI_PT_LINESTRIP_ADJ,
		[PIPE_PRIM_TRIANGLES_ADJACENCY]		= V_008958_DI_PT_TRILIST_ADJ,
		[PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY]	= V_008958_DI_PT_TRISTRIP_ADJ
        };
	unsigned result = prim_conv[pprim];
        if (result == ~0) {
		R600_ERR("unsupported primitive type %d\n", pprim);
        }
	return result;
}

static unsigned si_conv_prim_to_gs_out(unsigned mode)
{
	static const int prim_conv[] = {
		[PIPE_PRIM_POINTS]			= V_028A6C_OUTPRIM_TYPE_POINTLIST,
		[PIPE_PRIM_LINES]			= V_028A6C_OUTPRIM_TYPE_LINESTRIP,
		[PIPE_PRIM_LINE_LOOP]			= V_028A6C_OUTPRIM_TYPE_LINESTRIP,
		[PIPE_PRIM_LINE_STRIP]			= V_028A6C_OUTPRIM_TYPE_LINESTRIP,
		[PIPE_PRIM_TRIANGLES]			= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_TRIANGLE_STRIP]		= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_TRIANGLE_FAN]		= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_QUADS]			= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_QUAD_STRIP]			= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_POLYGON]			= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_LINES_ADJACENCY]		= V_028A6C_OUTPRIM_TYPE_LINESTRIP,
		[PIPE_PRIM_LINE_STRIP_ADJACENCY]	= V_028A6C_OUTPRIM_TYPE_LINESTRIP,
		[PIPE_PRIM_TRIANGLES_ADJACENCY]		= V_028A6C_OUTPRIM_TYPE_TRISTRIP,
		[PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY]	= V_028A6C_OUTPRIM_TYPE_TRISTRIP
	};
	assert(mode < Elements(prim_conv));

	return prim_conv[mode];
}

static bool si_update_draw_info_state(struct si_context *sctx,
				      const struct pipe_draw_info *info,
				      const struct pipe_index_buffer *ib)
{
	struct si_pm4_state *pm4 = si_pm4_alloc_state(sctx);
	struct si_shader *vs = si_get_vs_state(sctx);
	unsigned prim = si_conv_pipe_prim(info->mode);
	unsigned gs_out_prim =
		si_conv_prim_to_gs_out(sctx->gs_shader ?
				       sctx->gs_shader->current->shader.gs_output_prim :
				       info->mode);
	unsigned ls_mask = 0;

	if (pm4 == NULL)
		return false;

	if (prim == ~0) {
		FREE(pm4);
		return false;
	}

	if (sctx->b.chip_class >= CIK) {
		struct si_state_rasterizer *rs = sctx->queued.named.rasterizer;
		bool wd_switch_on_eop = prim == V_008958_DI_PT_POLYGON ||
					prim == V_008958_DI_PT_LINELOOP ||
					prim == V_008958_DI_PT_TRIFAN ||
					prim == V_008958_DI_PT_TRISTRIP_ADJ ||
					info->primitive_restart ||
					(rs ? rs->line_stipple_enable : false);
		/* If the WD switch is false, the IA switch must be false too. */
		bool ia_switch_on_eop = wd_switch_on_eop;

		si_pm4_set_reg(pm4, R_028AA8_IA_MULTI_VGT_PARAM,
			       S_028AA8_SWITCH_ON_EOP(ia_switch_on_eop) |
			       S_028AA8_PARTIAL_VS_WAVE_ON(1) |
			       S_028AA8_PRIMGROUP_SIZE(63) |
			       S_028AA8_WD_SWITCH_ON_EOP(wd_switch_on_eop));
		si_pm4_set_reg(pm4, R_028B74_VGT_DISPATCH_DRAW_INDEX,
			       ib->index_size == 4 ? 0xFC000000 : 0xFC00);

		si_pm4_set_reg(pm4, R_030908_VGT_PRIMITIVE_TYPE, prim);
	} else {
		si_pm4_set_reg(pm4, R_008958_VGT_PRIMITIVE_TYPE, prim);
	}

	si_pm4_set_reg(pm4, R_028A6C_VGT_GS_OUT_PRIM_TYPE, gs_out_prim);
	si_pm4_set_reg(pm4, R_028408_VGT_INDX_OFFSET,
		       info->indexed ? info->index_bias : info->start);
	si_pm4_set_reg(pm4, R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, info->restart_index);
	si_pm4_set_reg(pm4, R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, info->primitive_restart);
	si_pm4_set_reg(pm4, SI_SGPR_START_INSTANCE * 4 +
		       (sctx->gs_shader ? R_00B330_SPI_SHADER_USER_DATA_ES_0 :
			R_00B130_SPI_SHADER_USER_DATA_VS_0),
		       info->start_instance);

        if (prim == V_008958_DI_PT_LINELIST)
                ls_mask = 1;
        else if (prim == V_008958_DI_PT_LINESTRIP)
                ls_mask = 2;
	si_pm4_set_reg(pm4, R_028A0C_PA_SC_LINE_STIPPLE,
		       S_028A0C_AUTO_RESET_CNTL(ls_mask) |
		       sctx->pa_sc_line_stipple);

        if (info->mode == PIPE_PRIM_QUADS || info->mode == PIPE_PRIM_QUAD_STRIP || info->mode == PIPE_PRIM_POLYGON) {
		si_pm4_set_reg(pm4, R_028814_PA_SU_SC_MODE_CNTL,
			       S_028814_PROVOKING_VTX_LAST(1) | sctx->pa_su_sc_mode_cntl);
        } else {
		si_pm4_set_reg(pm4, R_028814_PA_SU_SC_MODE_CNTL, sctx->pa_su_sc_mode_cntl);
        }
	si_pm4_set_reg(pm4, R_02881C_PA_CL_VS_OUT_CNTL,
		       S_02881C_USE_VTX_POINT_SIZE(vs->vs_out_point_size) |
		       S_02881C_USE_VTX_EDGE_FLAG(vs->vs_out_edgeflag) |
		       S_02881C_USE_VTX_RENDER_TARGET_INDX(vs->vs_out_layer) |
		       S_02881C_VS_OUT_CCDIST0_VEC_ENA((vs->clip_dist_write & 0x0F) != 0) |
		       S_02881C_VS_OUT_CCDIST1_VEC_ENA((vs->clip_dist_write & 0xF0) != 0) |
		       S_02881C_VS_OUT_MISC_VEC_ENA(vs->vs_out_misc_write) |
		       (sctx->queued.named.rasterizer->clip_plane_enable &
			vs->clip_dist_write));
	si_pm4_set_reg(pm4, R_028810_PA_CL_CLIP_CNTL,
		       sctx->queued.named.rasterizer->pa_cl_clip_cntl |
		       (vs->clip_dist_write ? 0 :
			sctx->queued.named.rasterizer->clip_plane_enable & 0x3F));

	si_pm4_set_state(sctx, draw_info, pm4);
	return true;
}

static void si_update_spi_map(struct si_context *sctx)
{
	struct si_shader *ps = &sctx->ps_shader->current->shader;
	struct si_shader *vs = si_get_vs_state(sctx);
	struct si_pm4_state *pm4 = si_pm4_alloc_state(sctx);
	unsigned i, j, tmp;

	for (i = 0; i < ps->ninput; i++) {
		unsigned name = ps->input[i].name;
		unsigned param_offset = ps->input[i].param_offset;

		if (name == TGSI_SEMANTIC_POSITION)
			/* Read from preloaded VGPRs, not parameters */
			continue;

bcolor:
		tmp = 0;

		if (ps->input[i].interpolate == TGSI_INTERPOLATE_CONSTANT ||
		    (ps->input[i].interpolate == TGSI_INTERPOLATE_COLOR &&
		     sctx->ps_shader->current->key.ps.flatshade)) {
			tmp |= S_028644_FLAT_SHADE(1);
		}

		if (name == TGSI_SEMANTIC_GENERIC &&
		    sctx->sprite_coord_enable & (1 << ps->input[i].sid)) {
			tmp |= S_028644_PT_SPRITE_TEX(1);
		}

		for (j = 0; j < vs->noutput; j++) {
			if (name == vs->output[j].name &&
			    ps->input[i].sid == vs->output[j].sid) {
				tmp |= S_028644_OFFSET(vs->output[j].param_offset);
				break;
			}
		}

		if (j == vs->noutput) {
			/* No corresponding output found, load defaults into input */
			tmp |= S_028644_OFFSET(0x20);
		}

		si_pm4_set_reg(pm4,
			       R_028644_SPI_PS_INPUT_CNTL_0 + param_offset * 4,
			       tmp);

		if (name == TGSI_SEMANTIC_COLOR &&
		    sctx->ps_shader->current->key.ps.color_two_side) {
			name = TGSI_SEMANTIC_BCOLOR;
			param_offset++;
			goto bcolor;
		}
	}

	si_pm4_set_state(sctx, spi, pm4);
}

/* Initialize state related to ESGS / GSVS ring buffers */
static void si_init_gs_rings(struct si_context *sctx)
{
	unsigned size = 128 * 1024;

	assert(!sctx->gs_rings);
	sctx->gs_rings = si_pm4_alloc_state(sctx);

	sctx->esgs_ring.buffer =
		pipe_buffer_create(sctx->b.b.screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_DEFAULT, size);
	sctx->esgs_ring.buffer_size = size;

	size = 64 * 1024 * 1024;
	sctx->gsvs_ring.buffer =
		pipe_buffer_create(sctx->b.b.screen, PIPE_BIND_CUSTOM,
				   PIPE_USAGE_DEFAULT, size);
	sctx->gsvs_ring.buffer_size = size;

	if (sctx->b.chip_class >= CIK) {
		si_pm4_set_reg(sctx->gs_rings, R_030900_VGT_ESGS_RING_SIZE,
			       sctx->esgs_ring.buffer_size / 256);
		si_pm4_set_reg(sctx->gs_rings, R_030904_VGT_GSVS_RING_SIZE,
			       sctx->gsvs_ring.buffer_size / 256);
	} else {
		si_pm4_set_reg(sctx->gs_rings, R_0088C8_VGT_ESGS_RING_SIZE,
			       sctx->esgs_ring.buffer_size / 256);
		si_pm4_set_reg(sctx->gs_rings, R_0088CC_VGT_GSVS_RING_SIZE,
			       sctx->gsvs_ring.buffer_size / 256);
	}

	si_set_ring_buffer(&sctx->b.b, PIPE_SHADER_VERTEX, SI_RING_ESGS,
			   &sctx->esgs_ring, 0, sctx->esgs_ring.buffer_size,
			   true, true, 4, 64);
	si_set_ring_buffer(&sctx->b.b, PIPE_SHADER_GEOMETRY, SI_RING_ESGS,
			   &sctx->esgs_ring, 0, sctx->esgs_ring.buffer_size,
			   false, false, 0, 0);
	si_set_ring_buffer(&sctx->b.b, PIPE_SHADER_VERTEX, SI_RING_GSVS,
			   &sctx->gsvs_ring, 0, sctx->gsvs_ring.buffer_size,
			   false, false, 0, 0);
}

static void si_update_derived_state(struct si_context *sctx)
{
	struct pipe_context * ctx = (struct pipe_context*)sctx;

	if (!sctx->blitter->running) {
		/* Flush depth textures which need to be flushed. */
		for (int i = 0; i < SI_NUM_SHADERS; i++) {
			if (sctx->samplers[i].depth_texture_mask) {
				si_flush_depth_textures(sctx, &sctx->samplers[i]);
			}
			if (sctx->samplers[i].compressed_colortex_mask) {
				si_decompress_color_textures(sctx, &sctx->samplers[i]);
			}
		}
	}

	if (sctx->gs_shader) {
		si_shader_select(ctx, sctx->gs_shader);

		if (!sctx->gs_shader->current->pm4) {
			si_pipe_shader_gs(ctx, sctx->gs_shader->current);
			si_pipe_shader_vs(ctx,
					  sctx->gs_shader->current->gs_copy_shader);
		}

		si_pm4_bind_state(sctx, gs, sctx->gs_shader->current->pm4);
		si_pm4_bind_state(sctx, vs, sctx->gs_shader->current->gs_copy_shader->pm4);

		sctx->b.streamout.stride_in_dw = sctx->gs_shader->so.stride;

		si_shader_select(ctx, sctx->vs_shader);

		if (!sctx->vs_shader->current->pm4)
			si_pipe_shader_es(ctx, sctx->vs_shader->current);

		si_pm4_bind_state(sctx, es, sctx->vs_shader->current->pm4);

		if (!sctx->gs_rings)
			si_init_gs_rings(sctx);
		if (sctx->emitted.named.gs_rings != sctx->gs_rings)
			sctx->b.flags |= R600_CONTEXT_VGT_FLUSH;
		si_pm4_bind_state(sctx, gs_rings, sctx->gs_rings);

		si_set_ring_buffer(ctx, PIPE_SHADER_GEOMETRY, SI_RING_GSVS,
				   &sctx->gsvs_ring,
				   sctx->gs_shader->current->shader.gs_max_out_vertices *
				   sctx->gs_shader->current->shader.noutput * 16,
				   64, true, true, 4, 16);

		if (!sctx->gs_on) {
			sctx->gs_on = si_pm4_alloc_state(sctx);

			si_pm4_set_reg(sctx->gs_on, R_028B54_VGT_SHADER_STAGES_EN,
				       S_028B54_ES_EN(V_028B54_ES_STAGE_REAL) |
				       S_028B54_GS_EN(1) |
				       S_028B54_VS_EN(V_028B54_VS_STAGE_COPY_SHADER));
		}
		si_pm4_bind_state(sctx, gs_onoff, sctx->gs_on);
	} else {
		si_shader_select(ctx, sctx->vs_shader);

		if (!sctx->vs_shader->current->pm4)
			si_pipe_shader_vs(ctx, sctx->vs_shader->current);

		si_pm4_bind_state(sctx, vs, sctx->vs_shader->current->pm4);

		sctx->b.streamout.stride_in_dw = sctx->vs_shader->so.stride;

		if (!sctx->gs_off) {
			sctx->gs_off = si_pm4_alloc_state(sctx);

			si_pm4_set_reg(sctx->gs_off, R_028A40_VGT_GS_MODE, 0);
			si_pm4_set_reg(sctx->gs_off, R_028B54_VGT_SHADER_STAGES_EN, 0);
		}
		si_pm4_bind_state(sctx, gs_onoff, sctx->gs_off);
		si_pm4_bind_state(sctx, gs_rings, NULL);
		si_pm4_bind_state(sctx, gs, NULL);
		si_pm4_bind_state(sctx, es, NULL);
	}

	si_shader_select(ctx, sctx->ps_shader);

	if (!sctx->ps_shader->current->pm4 ||
	    sctx->ps_shader->current->cb0_is_integer != sctx->framebuffer.cb0_is_integer)
		si_pipe_shader_ps(ctx, sctx->ps_shader->current);

	si_pm4_bind_state(sctx, ps, sctx->ps_shader->current->pm4);

	if (si_pm4_state_changed(sctx, ps) || si_pm4_state_changed(sctx, vs)) {
		/* XXX: Emitting the PS state even when only the VS changed
		 * fixes random failures with piglit glsl-max-varyings.
		 * Not sure why...
		 */
		sctx->emitted.named.ps = NULL;
		si_update_spi_map(sctx);
	}
}

static void si_vertex_buffer_update(struct si_context *sctx)
{
	struct pipe_context *ctx = &sctx->b.b;
	struct si_pm4_state *pm4 = si_pm4_alloc_state(sctx);
	bool bound[PIPE_MAX_ATTRIBS] = {};
	unsigned i, count;
	uint64_t va;

	sctx->b.flags |= R600_CONTEXT_INV_TEX_CACHE;

	count = sctx->vertex_elements->count;
	assert(count <= 256 / 4);

	si_pm4_sh_data_begin(pm4);
	for (i = 0 ; i < count; i++) {
		struct pipe_vertex_element *ve = &sctx->vertex_elements->elements[i];
		struct pipe_vertex_buffer *vb;
		struct r600_resource *rbuffer;
		unsigned offset;

		if (ve->vertex_buffer_index >= sctx->nr_vertex_buffers)
			continue;

		vb = &sctx->vertex_buffer[ve->vertex_buffer_index];
		rbuffer = (struct r600_resource*)vb->buffer;
		if (rbuffer == NULL)
			continue;

		offset = 0;
		offset += vb->buffer_offset;
		offset += ve->src_offset;

		va = r600_resource_va(ctx->screen, (void*)rbuffer);
		va += offset;

		/* Fill in T# buffer resource description */
		si_pm4_sh_data_add(pm4, va & 0xFFFFFFFF);
		si_pm4_sh_data_add(pm4, (S_008F04_BASE_ADDRESS_HI(va >> 32) |
					 S_008F04_STRIDE(vb->stride)));
		if (vb->stride)
			/* Round up by rounding down and adding 1 */
			si_pm4_sh_data_add(pm4,
					   (vb->buffer->width0 - offset -
					    util_format_get_blocksize(ve->src_format)) /
					   vb->stride + 1);
		else
			si_pm4_sh_data_add(pm4, vb->buffer->width0 - offset);
		si_pm4_sh_data_add(pm4, sctx->vertex_elements->rsrc_word3[i]);

		if (!bound[ve->vertex_buffer_index]) {
			si_pm4_add_bo(pm4, rbuffer, RADEON_USAGE_READ,
				      RADEON_PRIO_SHADER_BUFFER_RO);
			bound[ve->vertex_buffer_index] = true;
		}
	}
	si_pm4_sh_data_end(pm4, sctx->gs_shader ?
			   R_00B330_SPI_SHADER_USER_DATA_ES_0 :
			   R_00B130_SPI_SHADER_USER_DATA_VS_0,
			   SI_SGPR_VERTEX_BUFFER);
	si_pm4_set_state(sctx, vertex_buffers, pm4);
}

static void si_state_draw(struct si_context *sctx,
			  const struct pipe_draw_info *info,
			  const struct pipe_index_buffer *ib)
{
	struct si_pm4_state *pm4 = si_pm4_alloc_state(sctx);

	if (pm4 == NULL)
		return;

	/* queries need some special values
	 * (this is non-zero if any query is active) */
	if (sctx->b.num_occlusion_queries > 0) {
		if (sctx->b.chip_class >= CIK) {
			si_pm4_set_reg(pm4, R_028004_DB_COUNT_CONTROL,
				       S_028004_PERFECT_ZPASS_COUNTS(1) |
				       S_028004_SAMPLE_RATE(sctx->framebuffer.log_samples) |
				       S_028004_ZPASS_ENABLE(1) |
				       S_028004_SLICE_EVEN_ENABLE(1) |
				       S_028004_SLICE_ODD_ENABLE(1));
		} else {
			si_pm4_set_reg(pm4, R_028004_DB_COUNT_CONTROL,
				       S_028004_PERFECT_ZPASS_COUNTS(1) |
				       S_028004_SAMPLE_RATE(sctx->framebuffer.log_samples));
		}
	}

	if (info->count_from_stream_output) {
		struct r600_so_target *t =
			(struct r600_so_target*)info->count_from_stream_output;
		uint64_t va = r600_resource_va(&sctx->screen->b.b,
					       &t->buf_filled_size->b.b);
		va += t->buf_filled_size_offset;

		si_pm4_set_reg(pm4, R_028B30_VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE,
			       t->stride_in_dw);

		si_pm4_cmd_begin(pm4, PKT3_COPY_DATA);
		si_pm4_cmd_add(pm4,
			       COPY_DATA_SRC_SEL(COPY_DATA_MEM) |
			       COPY_DATA_DST_SEL(COPY_DATA_REG) |
			       COPY_DATA_WR_CONFIRM);
		si_pm4_cmd_add(pm4, va);     /* src address lo */
		si_pm4_cmd_add(pm4, va >> 32UL); /* src address hi */
		si_pm4_cmd_add(pm4, R_028B2C_VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE >> 2);
		si_pm4_cmd_add(pm4, 0); /* unused */
		si_pm4_add_bo(pm4, t->buf_filled_size, RADEON_USAGE_READ,
			      RADEON_PRIO_MIN);
		si_pm4_cmd_end(pm4, true);
	}

	/* draw packet */
	si_pm4_cmd_begin(pm4, PKT3_INDEX_TYPE);
	if (ib->index_size == 4) {
		si_pm4_cmd_add(pm4, V_028A7C_VGT_INDEX_32 | (SI_BIG_ENDIAN ?
				V_028A7C_VGT_DMA_SWAP_32_BIT : 0));
	} else {
		si_pm4_cmd_add(pm4, V_028A7C_VGT_INDEX_16 | (SI_BIG_ENDIAN ?
				V_028A7C_VGT_DMA_SWAP_16_BIT : 0));
	}
	si_pm4_cmd_end(pm4, sctx->b.predicate_drawing);

	si_pm4_cmd_begin(pm4, PKT3_NUM_INSTANCES);
	si_pm4_cmd_add(pm4, info->instance_count);
	si_pm4_cmd_end(pm4, sctx->b.predicate_drawing);

	if (info->indexed) {
		uint32_t max_size = (ib->buffer->width0 - ib->offset) /
				 sctx->index_buffer.index_size;
		uint64_t va;
		va = r600_resource_va(&sctx->screen->b.b, ib->buffer);
		va += ib->offset;

		si_pm4_add_bo(pm4, (struct r600_resource *)ib->buffer, RADEON_USAGE_READ,
			      RADEON_PRIO_MIN);
		si_cmd_draw_index_2(pm4, max_size, va, info->count,
				    V_0287F0_DI_SRC_SEL_DMA,
				    sctx->b.predicate_drawing);
	} else {
		uint32_t initiator = V_0287F0_DI_SRC_SEL_AUTO_INDEX;
		initiator |= S_0287F0_USE_OPAQUE(!!info->count_from_stream_output);
		si_cmd_draw_index_auto(pm4, info->count, initiator, sctx->b.predicate_drawing);
	}

	si_pm4_set_state(sctx, draw, pm4);
}

void si_emit_cache_flush(struct r600_common_context *sctx, struct r600_atom *atom)
{
	struct radeon_winsys_cs *cs = sctx->rings.gfx.cs;
	uint32_t cp_coher_cntl = 0;

	/* XXX SI flushes both ICACHE and KCACHE if either flag is set.
	 * XXX CIK shouldn't have this issue. Test CIK before separating the flags
	 * XXX to ensure there is no regression. Also find out if there is another
	 * XXX way to flush either ICACHE or KCACHE but not both for SI. */
	if (sctx->flags & (R600_CONTEXT_INV_SHADER_CACHE |
			   R600_CONTEXT_INV_CONST_CACHE)) {
		cp_coher_cntl |= S_0085F0_SH_ICACHE_ACTION_ENA(1) |
				 S_0085F0_SH_KCACHE_ACTION_ENA(1);
	}
	if (sctx->flags & (R600_CONTEXT_INV_TEX_CACHE |
			   R600_CONTEXT_STREAMOUT_FLUSH)) {
		cp_coher_cntl |= S_0085F0_TC_ACTION_ENA(1) |
				 S_0085F0_TCL1_ACTION_ENA(1);
	}
	if (sctx->flags & R600_CONTEXT_FLUSH_AND_INV_CB) {
		cp_coher_cntl |= S_0085F0_CB_ACTION_ENA(1) |
				 S_0085F0_CB0_DEST_BASE_ENA(1) |
			         S_0085F0_CB1_DEST_BASE_ENA(1) |
			         S_0085F0_CB2_DEST_BASE_ENA(1) |
			         S_0085F0_CB3_DEST_BASE_ENA(1) |
			         S_0085F0_CB4_DEST_BASE_ENA(1) |
			         S_0085F0_CB5_DEST_BASE_ENA(1) |
			         S_0085F0_CB6_DEST_BASE_ENA(1) |
			         S_0085F0_CB7_DEST_BASE_ENA(1);
	}
	if (sctx->flags & R600_CONTEXT_FLUSH_AND_INV_DB) {
		cp_coher_cntl |= S_0085F0_DB_ACTION_ENA(1) |
				 S_0085F0_DB_DEST_BASE_ENA(1);
	}

	if (cp_coher_cntl) {
		if (sctx->chip_class >= CIK) {
			radeon_emit(cs, PKT3(PKT3_ACQUIRE_MEM, 5, 0));
			radeon_emit(cs, cp_coher_cntl);   /* CP_COHER_CNTL */
			radeon_emit(cs, 0xffffffff);      /* CP_COHER_SIZE */
			radeon_emit(cs, 0xff);            /* CP_COHER_SIZE_HI */
			radeon_emit(cs, 0);               /* CP_COHER_BASE */
			radeon_emit(cs, 0);               /* CP_COHER_BASE_HI */
			radeon_emit(cs, 0x0000000A);      /* POLL_INTERVAL */
		} else {
			radeon_emit(cs, PKT3(PKT3_SURFACE_SYNC, 3, 0));
			radeon_emit(cs, cp_coher_cntl);   /* CP_COHER_CNTL */
			radeon_emit(cs, 0xffffffff);      /* CP_COHER_SIZE */
			radeon_emit(cs, 0);               /* CP_COHER_BASE */
			radeon_emit(cs, 0x0000000A);      /* POLL_INTERVAL */
		}
	}

	if (sctx->flags & R600_CONTEXT_FLUSH_AND_INV_CB_META) {
		radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
		radeon_emit(cs, EVENT_TYPE(V_028A90_FLUSH_AND_INV_CB_META) | EVENT_INDEX(0));
	}
	if (sctx->flags & R600_CONTEXT_FLUSH_AND_INV_DB_META) {
		radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
		radeon_emit(cs, EVENT_TYPE(V_028A90_FLUSH_AND_INV_DB_META) | EVENT_INDEX(0));
	}

	if (sctx->flags & (R600_CONTEXT_WAIT_3D_IDLE |
			   R600_CONTEXT_PS_PARTIAL_FLUSH)) {
		radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
		radeon_emit(cs, EVENT_TYPE(V_028A90_PS_PARTIAL_FLUSH) | EVENT_INDEX(4));
	} else if (sctx->flags & R600_CONTEXT_STREAMOUT_FLUSH) {
		/* Needed if streamout buffers are going to be used as a source. */
		radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
		radeon_emit(cs, EVENT_TYPE(V_028A90_VS_PARTIAL_FLUSH) | EVENT_INDEX(4));
	}

	if (sctx->flags & R600_CONTEXT_VGT_FLUSH) {
		radeon_emit(cs, PKT3(PKT3_EVENT_WRITE, 0, 0));
		radeon_emit(cs, EVENT_TYPE(V_028A90_VGT_FLUSH) | EVENT_INDEX(0));
	}

	sctx->flags = 0;
}

const struct r600_atom si_atom_cache_flush = { si_emit_cache_flush, 13 }; /* number of CS dwords */

void si_draw_vbo(struct pipe_context *ctx, const struct pipe_draw_info *info)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct pipe_index_buffer ib = {};
	uint32_t i;

	if (!info->count && (info->indexed || !info->count_from_stream_output))
		return;

	if (!sctx->ps_shader || !sctx->vs_shader)
		return;

	si_update_derived_state(sctx);
	si_vertex_buffer_update(sctx);

	if (info->indexed) {
		/* Initialize the index buffer struct. */
		pipe_resource_reference(&ib.buffer, sctx->index_buffer.buffer);
		ib.user_buffer = sctx->index_buffer.user_buffer;
		ib.index_size = sctx->index_buffer.index_size;
		ib.offset = sctx->index_buffer.offset + info->start * ib.index_size;

		/* Translate or upload, if needed. */
		if (ib.index_size == 1) {
			struct pipe_resource *out_buffer = NULL;
			unsigned out_offset;
			void *ptr;

			u_upload_alloc(sctx->b.uploader, 0, info->count * 2,
				       &out_offset, &out_buffer, &ptr);

			util_shorten_ubyte_elts_to_userptr(
						&sctx->b.b, &ib, 0, ib.offset, info->count, ptr);

			pipe_resource_reference(&ib.buffer, NULL);
			ib.user_buffer = NULL;
			ib.buffer = out_buffer;
			ib.offset = out_offset;
			ib.index_size = 2;
		}

		if (ib.user_buffer && !ib.buffer) {
			u_upload_data(sctx->b.uploader, 0, info->count * ib.index_size,
				      ib.user_buffer, &ib.offset, &ib.buffer);
		}
	}

	if (!si_update_draw_info_state(sctx, info, &ib))
		return;

	si_state_draw(sctx, info, &ib);

	sctx->pm4_dirty_cdwords += si_pm4_dirty_dw(sctx);

	/* Check flush flags. */
	if (sctx->b.flags)
		sctx->atoms.cache_flush->dirty = true;

	si_need_cs_space(sctx, 0, TRUE);

	/* Emit states. */
	for (i = 0; i < SI_NUM_ATOMS(sctx); i++) {
		if (sctx->atoms.array[i]->dirty) {
			sctx->atoms.array[i]->emit(&sctx->b, sctx->atoms.array[i]);
			sctx->atoms.array[i]->dirty = false;
		}
	}

	si_pm4_emit_dirty(sctx);
	sctx->pm4_dirty_cdwords = 0;

#if SI_TRACE_CS
	if (sctx->screen->b.trace_bo) {
		si_trace_emit(sctx);
	}
#endif

	/* Set the depth buffer as dirty. */
	if (sctx->framebuffer.state.zsbuf) {
		struct pipe_surface *surf = sctx->framebuffer.state.zsbuf;
		struct r600_texture *rtex = (struct r600_texture *)surf->texture;

		rtex->dirty_level_mask |= 1 << surf->u.tex.level;
	}
	if (sctx->framebuffer.compressed_cb_mask) {
		struct pipe_surface *surf;
		struct r600_texture *rtex;
		unsigned mask = sctx->framebuffer.compressed_cb_mask;

		do {
			unsigned i = u_bit_scan(&mask);
			surf = sctx->framebuffer.state.cbufs[i];
			rtex = (struct r600_texture*)surf->texture;

			rtex->dirty_level_mask |= 1 << surf->u.tex.level;
		} while (mask);
	}

	pipe_resource_reference(&ib.buffer, NULL);
	sctx->b.num_draw_calls++;
}
