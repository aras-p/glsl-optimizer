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
 */
#include "r600.h"
#include "r600_hw_context_priv.h"
#include "radeonsi_pipe.h"
#include "sid.h"
#include "util/u_memory.h"
#include <errno.h>

#define GROUP_FORCE_NEW_BLOCK	0

static const struct r600_reg si_config_reg_list[] = {
	{R_0088B0_VGT_VTX_VECT_EJECT_REG, REG_FLAG_FLUSH_CHANGE},
	{R_0088C8_VGT_ESGS_RING_SIZE, REG_FLAG_FLUSH_CHANGE},
	{R_0088CC_VGT_GSVS_RING_SIZE, REG_FLAG_FLUSH_CHANGE},
	{R_008958_VGT_PRIMITIVE_TYPE, 0},
	{R_008A14_PA_CL_ENHANCE, REG_FLAG_FLUSH_CHANGE},
	{R_009100_SPI_CONFIG_CNTL, REG_FLAG_ENABLE_ALWAYS | REG_FLAG_FLUSH_CHANGE},
	{R_00913C_SPI_CONFIG_CNTL_1, REG_FLAG_ENABLE_ALWAYS | REG_FLAG_FLUSH_CHANGE},
};

static const struct r600_reg si_context_reg_list[] = {
	{R_028000_DB_RENDER_CONTROL, 0},
	{R_028004_DB_COUNT_CONTROL, 0},
	{R_02800C_DB_RENDER_OVERRIDE, 0},
	{R_028010_DB_RENDER_OVERRIDE2, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_028014_DB_HTILE_DATA_BASE, REG_FLAG_NEED_BO},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_028020_DB_DEPTH_BOUNDS_MIN, 0},
	{R_028024_DB_DEPTH_BOUNDS_MAX, 0},
	{R_028028_DB_STENCIL_CLEAR, 0},
	{R_02802C_DB_DEPTH_CLEAR, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_028080_TA_BC_BASE_ADDR, REG_FLAG_NEED_BO},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_02820C_PA_SC_CLIPRECT_RULE, 0},
	{R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0},
	{R_028238_CB_TARGET_MASK, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_028400_VGT_MAX_VTX_INDX, 0},
	{R_028404_VGT_MIN_VTX_INDX, 0},
	{R_028408_VGT_INDX_OFFSET, 0},
	{R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, 0},
	{R_028A94_VGT_MULTI_PRIM_IB_RESET_EN, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_028430_DB_STENCILREFMASK, 0},
	{R_028434_DB_STENCILREFMASK_BF, 0},
	{R_028644_SPI_PS_INPUT_CNTL_0, 0},
	{R_028648_SPI_PS_INPUT_CNTL_1, 0},
	{R_02864C_SPI_PS_INPUT_CNTL_2, 0},
	{R_028650_SPI_PS_INPUT_CNTL_3, 0},
	{R_028654_SPI_PS_INPUT_CNTL_4, 0},
	{R_028658_SPI_PS_INPUT_CNTL_5, 0},
	{R_02865C_SPI_PS_INPUT_CNTL_6, 0},
	{R_028660_SPI_PS_INPUT_CNTL_7, 0},
	{R_028664_SPI_PS_INPUT_CNTL_8, 0},
	{R_028668_SPI_PS_INPUT_CNTL_9, 0},
	{R_02866C_SPI_PS_INPUT_CNTL_10, 0},
	{R_028670_SPI_PS_INPUT_CNTL_11, 0},
	{R_028674_SPI_PS_INPUT_CNTL_12, 0},
	{R_028678_SPI_PS_INPUT_CNTL_13, 0},
	{R_02867C_SPI_PS_INPUT_CNTL_14, 0},
	{R_028680_SPI_PS_INPUT_CNTL_15, 0},
	{R_028684_SPI_PS_INPUT_CNTL_16, 0},
	{R_028688_SPI_PS_INPUT_CNTL_17, 0},
	{R_02868C_SPI_PS_INPUT_CNTL_18, 0},
	{R_028690_SPI_PS_INPUT_CNTL_19, 0},
	{R_028694_SPI_PS_INPUT_CNTL_20, 0},
	{R_028698_SPI_PS_INPUT_CNTL_21, 0},
	{R_02869C_SPI_PS_INPUT_CNTL_22, 0},
	{R_0286A0_SPI_PS_INPUT_CNTL_23, 0},
	{R_0286A4_SPI_PS_INPUT_CNTL_24, 0},
	{R_0286A8_SPI_PS_INPUT_CNTL_25, 0},
	{R_0286AC_SPI_PS_INPUT_CNTL_26, 0},
	{R_0286B0_SPI_PS_INPUT_CNTL_27, 0},
	{R_0286B4_SPI_PS_INPUT_CNTL_28, 0},
	{R_0286B8_SPI_PS_INPUT_CNTL_29, 0},
	{R_0286BC_SPI_PS_INPUT_CNTL_30, 0},
	{R_0286C0_SPI_PS_INPUT_CNTL_31, 0},
	{R_0286C4_SPI_VS_OUT_CONFIG, 0},
	{R_0286CC_SPI_PS_INPUT_ENA, 0},
	{R_0286D0_SPI_PS_INPUT_ADDR, 0},
	{R_0286D4_SPI_INTERP_CONTROL_0, 0},
	{R_0286D8_SPI_PS_IN_CONTROL, 0},
	{R_0286E0_SPI_BARYC_CNTL, 0},
	{R_02870C_SPI_SHADER_POS_FORMAT, 0},
	{R_028710_SPI_SHADER_Z_FORMAT, 0},
	{R_028714_SPI_SHADER_COL_FORMAT, 0},
	{R_0287D4_PA_CL_POINT_X_RAD, 0},
	{R_0287D8_PA_CL_POINT_Y_RAD, 0},
	{R_0287DC_PA_CL_POINT_SIZE, 0},
	{R_0287E0_PA_CL_POINT_CULL_RAD, 0},
	{R_028800_DB_DEPTH_CONTROL, 0},
	{R_028804_DB_EQAA, 0},
	{R_02880C_DB_SHADER_CONTROL, 0},
	{R_028810_PA_CL_CLIP_CNTL, 0},
	{R_028814_PA_SU_SC_MODE_CNTL, 0},
	{R_02881C_PA_CL_VS_OUT_CNTL, 0},
	{R_028820_PA_CL_NANINF_CNTL, 0},
	{R_028824_PA_SU_LINE_STIPPLE_CNTL, 0},
	{R_028828_PA_SU_LINE_STIPPLE_SCALE, 0},
	{R_02882C_PA_SU_PRIM_FILTER_CNTL, 0},
	{R_028A00_PA_SU_POINT_SIZE, 0},
	{R_028A04_PA_SU_POINT_MINMAX, 0},
	{R_028A08_PA_SU_LINE_CNTL, 0},
	{R_028A0C_PA_SC_LINE_STIPPLE, 0},
	{R_028A10_VGT_OUTPUT_PATH_CNTL, 0},
	{R_028A14_VGT_HOS_CNTL, 0},
	{R_028A18_VGT_HOS_MAX_TESS_LEVEL, 0},
	{R_028A1C_VGT_HOS_MIN_TESS_LEVEL, 0},
	{R_028A20_VGT_HOS_REUSE_DEPTH, 0},
	{R_028A24_VGT_GROUP_PRIM_TYPE, 0},
	{R_028A28_VGT_GROUP_FIRST_DECR, 0},
	{R_028A2C_VGT_GROUP_DECR, 0},
	{R_028A30_VGT_GROUP_VECT_0_CNTL, 0},
	{R_028A34_VGT_GROUP_VECT_1_CNTL, 0},
	{R_028A38_VGT_GROUP_VECT_0_FMT_CNTL, 0},
	{R_028A3C_VGT_GROUP_VECT_1_FMT_CNTL, 0},
	{R_028A40_VGT_GS_MODE, 0},
	{R_028A48_PA_SC_MODE_CNTL_0, 0},
	{R_028A4C_PA_SC_MODE_CNTL_1, 0},
	{R_028A50_VGT_ENHANCE, 0},
	{R_028A54_VGT_GS_PER_ES, 0},
	{R_028A58_VGT_ES_PER_GS, 0},
	{R_028A5C_VGT_GS_PER_VS, 0},
	{R_028A60_VGT_GSVS_RING_OFFSET_1, 0},
	{R_028A64_VGT_GSVS_RING_OFFSET_2, 0},
	{R_028A68_VGT_GSVS_RING_OFFSET_3, 0},
	{R_028A6C_VGT_GS_OUT_PRIM_TYPE, 0},
	{R_028A70_IA_ENHANCE, 0},
	{R_028A84_VGT_PRIMITIVEID_EN, 0},
	{R_028A8C_VGT_PRIMITIVEID_RESET, 0},
	{R_028AA0_VGT_INSTANCE_STEP_RATE_0, 0},
	{R_028AA4_VGT_INSTANCE_STEP_RATE_1, 0},
	{R_028AA8_IA_MULTI_VGT_PARAM, 0},
	{R_028AAC_VGT_ESGS_RING_ITEMSIZE, 0},
	{R_028AB0_VGT_GSVS_RING_ITEMSIZE, 0},
	{R_028AB4_VGT_REUSE_OFF, 0},
	{R_028AB8_VGT_VTX_CNT_EN, 0},
	{R_028ABC_DB_HTILE_SURFACE, 0},
	{R_028AC0_DB_SRESULTS_COMPARE_STATE0, 0},
	{R_028AC4_DB_SRESULTS_COMPARE_STATE1, 0},
	{R_028AC8_DB_PRELOAD_CONTROL, 0},
	{R_028B54_VGT_SHADER_STAGES_EN, 0},
	{R_028B70_DB_ALPHA_TO_MASK, 0},
	{R_028B78_PA_SU_POLY_OFFSET_DB_FMT_CNTL, 0},
	{R_028B7C_PA_SU_POLY_OFFSET_CLAMP, 0},
	{R_028B80_PA_SU_POLY_OFFSET_FRONT_SCALE, 0},
	{R_028B84_PA_SU_POLY_OFFSET_FRONT_OFFSET, 0},
	{R_028B88_PA_SU_POLY_OFFSET_BACK_SCALE, 0},
	{R_028B8C_PA_SU_POLY_OFFSET_BACK_OFFSET, 0},
	{R_028B94_VGT_STRMOUT_CONFIG, 0},
	{R_028B98_VGT_STRMOUT_BUFFER_CONFIG, 0},
	{R_028BD4_PA_SC_CENTROID_PRIORITY_0, 0},
	{R_028BD8_PA_SC_CENTROID_PRIORITY_1, 0},
	{R_028BDC_PA_SC_LINE_CNTL, 0},
	{R_028BE4_PA_SU_VTX_CNTL, 0},
	{R_028BE8_PA_CL_GB_VERT_CLIP_ADJ, 0},
	{R_028BEC_PA_CL_GB_VERT_DISC_ADJ, 0},
	{R_028BF0_PA_CL_GB_HORZ_CLIP_ADJ, 0},
	{R_028BF4_PA_CL_GB_HORZ_DISC_ADJ, 0},
	{R_028BF8_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_0, 0},
	{R_028BFC_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_1, 0},
	{R_028C00_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_2, 0},
	{R_028C04_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y0_3, 0},
	{R_028C08_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_0, 0},
	{R_028C0C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_1, 0},
	{R_028C10_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_2, 0},
	{R_028C14_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y0_3, 0},
	{R_028C18_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_0, 0},
	{R_028C1C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_1, 0},
	{R_028C20_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_2, 0},
	{R_028C24_PA_SC_AA_SAMPLE_LOCS_PIXEL_X0Y1_3, 0},
	{R_028C28_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_0, 0},
	{R_028C2C_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_1, 0},
	{R_028C30_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_2, 0},
	{R_028C34_PA_SC_AA_SAMPLE_LOCS_PIXEL_X1Y1_3, 0},
};

static const struct r600_reg si_sh_reg_list[] = {
	{R_00B020_SPI_SHADER_PGM_LO_PS, REG_FLAG_NEED_BO},
	{R_00B024_SPI_SHADER_PGM_HI_PS, REG_FLAG_NEED_BO},
	{R_00B028_SPI_SHADER_PGM_RSRC1_PS, 0},
	{R_00B02C_SPI_SHADER_PGM_RSRC2_PS, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B030_SPI_SHADER_USER_DATA_PS_0, REG_FLAG_NEED_BO},
	{R_00B034_SPI_SHADER_USER_DATA_PS_1, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B038_SPI_SHADER_USER_DATA_PS_2, REG_FLAG_NEED_BO},
	{R_00B03C_SPI_SHADER_USER_DATA_PS_3, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B040_SPI_SHADER_USER_DATA_PS_4, REG_FLAG_NEED_BO},
	{R_00B044_SPI_SHADER_USER_DATA_PS_5, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B048_SPI_SHADER_USER_DATA_PS_6, REG_FLAG_NEED_BO},
	{R_00B04C_SPI_SHADER_USER_DATA_PS_7, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B050_SPI_SHADER_USER_DATA_PS_8, REG_FLAG_NEED_BO},
	{R_00B054_SPI_SHADER_USER_DATA_PS_9, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B058_SPI_SHADER_USER_DATA_PS_10, REG_FLAG_NEED_BO},
	{R_00B05C_SPI_SHADER_USER_DATA_PS_11, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B060_SPI_SHADER_USER_DATA_PS_12, REG_FLAG_NEED_BO},
	{R_00B064_SPI_SHADER_USER_DATA_PS_13, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B068_SPI_SHADER_USER_DATA_PS_14, REG_FLAG_NEED_BO},
	{R_00B06C_SPI_SHADER_USER_DATA_PS_15, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B120_SPI_SHADER_PGM_LO_VS, REG_FLAG_NEED_BO},
	{R_00B124_SPI_SHADER_PGM_HI_VS, REG_FLAG_NEED_BO},
	{R_00B128_SPI_SHADER_PGM_RSRC1_VS, 0},
	{R_00B12C_SPI_SHADER_PGM_RSRC2_VS, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B130_SPI_SHADER_USER_DATA_VS_0, REG_FLAG_NEED_BO},
	{R_00B134_SPI_SHADER_USER_DATA_VS_1, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B138_SPI_SHADER_USER_DATA_VS_2, REG_FLAG_NEED_BO},
	{R_00B13C_SPI_SHADER_USER_DATA_VS_3, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B140_SPI_SHADER_USER_DATA_VS_4, REG_FLAG_NEED_BO},
	{R_00B144_SPI_SHADER_USER_DATA_VS_5, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B148_SPI_SHADER_USER_DATA_VS_6, REG_FLAG_NEED_BO},
	{R_00B14C_SPI_SHADER_USER_DATA_VS_7, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B150_SPI_SHADER_USER_DATA_VS_8, REG_FLAG_NEED_BO},
	{R_00B154_SPI_SHADER_USER_DATA_VS_9, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B158_SPI_SHADER_USER_DATA_VS_10, REG_FLAG_NEED_BO},
	{R_00B15C_SPI_SHADER_USER_DATA_VS_11, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B160_SPI_SHADER_USER_DATA_VS_12, REG_FLAG_NEED_BO},
	{R_00B164_SPI_SHADER_USER_DATA_VS_13, 0},
	{GROUP_FORCE_NEW_BLOCK, 0},
	{R_00B168_SPI_SHADER_USER_DATA_VS_14, REG_FLAG_NEED_BO},
	{R_00B16C_SPI_SHADER_USER_DATA_VS_15, 0},
};

int si_context_init(struct r600_context *ctx)
{
	int r;

	LIST_INITHEAD(&ctx->active_query_list);

	/* init dirty list */
	LIST_INITHEAD(&ctx->dirty);
	LIST_INITHEAD(&ctx->enable_list);

	ctx->range = calloc(NUM_RANGES, sizeof(struct r600_range));
	if (!ctx->range) {
		r = -ENOMEM;
		goto out_err;
	}

	/* add blocks */
	r = r600_context_add_block(ctx, si_config_reg_list,
				   Elements(si_config_reg_list), PKT3_SET_CONFIG_REG, SI_CONFIG_REG_OFFSET);
	if (r)
		goto out_err;
	r = r600_context_add_block(ctx, si_context_reg_list,
				   Elements(si_context_reg_list), PKT3_SET_CONTEXT_REG, SI_CONTEXT_REG_OFFSET);
	if (r)
		goto out_err;
	r = r600_context_add_block(ctx, si_sh_reg_list,
				   Elements(si_sh_reg_list), PKT3_SET_SH_REG, SI_SH_REG_OFFSET);
	if (r)
		goto out_err;


	/* PS SAMPLER */
	/* VS SAMPLER */

	/* PS SAMPLER BORDER */
	/* VS SAMPLER BORDER */

	/* PS RESOURCES */
	/* VS RESOURCES */

	ctx->cs = ctx->ws->cs_create(ctx->ws);

	r600_init_cs(ctx);
	ctx->max_db = 8;
	return 0;
out_err:
	r600_context_fini(ctx);
	return r;
}

static inline void evergreen_context_ps_partial_flush(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	if (!(ctx->flags & R600_CONTEXT_DRAW_PENDING))
		return;

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_PS_PARTIAL_FLUSH) | EVENT_INDEX(4);

	ctx->flags &= ~R600_CONTEXT_DRAW_PENDING;
}

void si_context_draw(struct r600_context *ctx, const struct r600_draw *draw)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	unsigned ndwords = 7;
	uint32_t *pm4;
	uint64_t va;

	if (draw->indices) {
		ndwords = 12;
	}
	if (ctx->num_cs_dw_queries_suspend)
		ndwords += 6;

	/* when increasing ndwords, bump the max limit too */
	assert(ndwords <= SI_MAX_DRAW_CS_DWORDS);

	/* queries need some special values
	 * (this is non-zero if any query is active) */
	if (ctx->num_cs_dw_queries_suspend) {
		pm4 = &cs->buf[cs->cdw];
		pm4[0] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		pm4[1] = (R_028004_DB_COUNT_CONTROL - SI_CONTEXT_REG_OFFSET) >> 2;
		pm4[2] = S_028004_PERFECT_ZPASS_COUNTS(1);
		pm4[3] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		pm4[4] = (R_02800C_DB_RENDER_OVERRIDE - SI_CONTEXT_REG_OFFSET) >> 2;
		pm4[5] = draw->db_render_override | S_02800C_NOOP_CULL_DISABLE(1);
		cs->cdw += 6;
		ndwords -= 6;
	}

	/* draw packet */
	pm4 = &cs->buf[cs->cdw];
	pm4[0] = PKT3(PKT3_INDEX_TYPE, 0, ctx->predicate_drawing);
	pm4[1] = draw->vgt_index_type;
	pm4[2] = PKT3(PKT3_NUM_INSTANCES, 0, ctx->predicate_drawing);
	pm4[3] = draw->vgt_num_instances;
	if (draw->indices) {
		va = r600_resource_va(&ctx->screen->screen, (void*)draw->indices);
		va += draw->indices_bo_offset;
		pm4[4] = PKT3(PKT3_DRAW_INDEX_2, 4, ctx->predicate_drawing);
		pm4[5] = (draw->indices->b.b.width0 - draw->indices_bo_offset) /
			ctx->index_buffer.index_size;
		pm4[6] = va;
		pm4[7] = (va >> 32UL) & 0xFF;
		pm4[8] = draw->vgt_num_indices;
		pm4[9] = draw->vgt_draw_initiator;
		pm4[10] = PKT3(PKT3_NOP, 0, ctx->predicate_drawing);
		pm4[11] = r600_context_bo_reloc(ctx, draw->indices, RADEON_USAGE_READ);
	} else {
		pm4[4] = PKT3(PKT3_DRAW_INDEX_AUTO, 1, ctx->predicate_drawing);
		pm4[5] = draw->vgt_num_indices;
		pm4[6] = draw->vgt_draw_initiator;
	}
	cs->cdw += ndwords;
}

void evergreen_flush_vgt_streamout(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONFIG_REG, 1, 0);
	cs->buf[cs->cdw++] = (R_0084FC_CP_STRMOUT_CNTL - SI_CONFIG_REG_OFFSET) >> 2;
	cs->buf[cs->cdw++] = 0;

	cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
	cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_SO_VGTSTREAMOUT_FLUSH) | EVENT_INDEX(0);

	cs->buf[cs->cdw++] = PKT3(PKT3_WAIT_REG_MEM, 5, 0);
	cs->buf[cs->cdw++] = WAIT_REG_MEM_EQUAL; /* wait until the register is equal to the reference value */
	cs->buf[cs->cdw++] = R_0084FC_CP_STRMOUT_CNTL >> 2;  /* register */
	cs->buf[cs->cdw++] = 0;
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* reference value */
	cs->buf[cs->cdw++] = S_0084FC_OFFSET_UPDATE_DONE(1); /* mask */
	cs->buf[cs->cdw++] = 4; /* poll interval */
}

void evergreen_set_streamout_enable(struct r600_context *ctx, unsigned buffer_enable_bit)
{
	struct radeon_winsys_cs *cs = ctx->cs;

	if (buffer_enable_bit) {
		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B94_VGT_STRMOUT_CONFIG - SI_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B94_STREAMOUT_0_EN(1);

		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B98_VGT_STRMOUT_BUFFER_CONFIG - SI_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B98_STREAM_0_BUFFER_EN(buffer_enable_bit);
	} else {
		cs->buf[cs->cdw++] = PKT3(PKT3_SET_CONTEXT_REG, 1, 0);
		cs->buf[cs->cdw++] = (R_028B94_VGT_STRMOUT_CONFIG - SI_CONTEXT_REG_OFFSET) >> 2;
		cs->buf[cs->cdw++] = S_028B94_STREAMOUT_0_EN(0);
	}
}
