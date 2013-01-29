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
#include "r600_hw_context_priv.h"
#include "evergreend.h"
#include "util/u_memory.h"
#include "util/u_math.h"

static const struct r600_reg cayman_config_reg_list[] = {
	{R_009100_SPI_CONFIG_CNTL, REG_FLAG_ENABLE_ALWAYS | REG_FLAG_FLUSH_CHANGE, 0},
	{R_00913C_SPI_CONFIG_CNTL_1, REG_FLAG_ENABLE_ALWAYS | REG_FLAG_FLUSH_CHANGE, 0},
};

static const struct r600_reg evergreen_context_reg_list[] = {
	{R_028010_DB_RENDER_OVERRIDE2, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028014_DB_HTILE_DATA_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_02861C_SPI_VS_OUT_ID_0, 0, 0},
	{R_028620_SPI_VS_OUT_ID_1, 0, 0},
	{R_028624_SPI_VS_OUT_ID_2, 0, 0},
	{R_028628_SPI_VS_OUT_ID_3, 0, 0},
	{R_02862C_SPI_VS_OUT_ID_4, 0, 0},
	{R_028630_SPI_VS_OUT_ID_5, 0, 0},
	{R_028634_SPI_VS_OUT_ID_6, 0, 0},
	{R_028638_SPI_VS_OUT_ID_7, 0, 0},
	{R_02863C_SPI_VS_OUT_ID_8, 0, 0},
	{R_028640_SPI_VS_OUT_ID_9, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028644_SPI_PS_INPUT_CNTL_0, 0, 0},
	{R_028648_SPI_PS_INPUT_CNTL_1, 0, 0},
	{R_02864C_SPI_PS_INPUT_CNTL_2, 0, 0},
	{R_028650_SPI_PS_INPUT_CNTL_3, 0, 0},
	{R_028654_SPI_PS_INPUT_CNTL_4, 0, 0},
	{R_028658_SPI_PS_INPUT_CNTL_5, 0, 0},
	{R_02865C_SPI_PS_INPUT_CNTL_6, 0, 0},
	{R_028660_SPI_PS_INPUT_CNTL_7, 0, 0},
	{R_028664_SPI_PS_INPUT_CNTL_8, 0, 0},
	{R_028668_SPI_PS_INPUT_CNTL_9, 0, 0},
	{R_02866C_SPI_PS_INPUT_CNTL_10, 0, 0},
	{R_028670_SPI_PS_INPUT_CNTL_11, 0, 0},
	{R_028674_SPI_PS_INPUT_CNTL_12, 0, 0},
	{R_028678_SPI_PS_INPUT_CNTL_13, 0, 0},
	{R_02867C_SPI_PS_INPUT_CNTL_14, 0, 0},
	{R_028680_SPI_PS_INPUT_CNTL_15, 0, 0},
	{R_028684_SPI_PS_INPUT_CNTL_16, 0, 0},
	{R_028688_SPI_PS_INPUT_CNTL_17, 0, 0},
	{R_02868C_SPI_PS_INPUT_CNTL_18, 0, 0},
	{R_028690_SPI_PS_INPUT_CNTL_19, 0, 0},
	{R_028694_SPI_PS_INPUT_CNTL_20, 0, 0},
	{R_028698_SPI_PS_INPUT_CNTL_21, 0, 0},
	{R_02869C_SPI_PS_INPUT_CNTL_22, 0, 0},
	{R_0286A0_SPI_PS_INPUT_CNTL_23, 0, 0},
	{R_0286A4_SPI_PS_INPUT_CNTL_24, 0, 0},
	{R_0286A8_SPI_PS_INPUT_CNTL_25, 0, 0},
	{R_0286AC_SPI_PS_INPUT_CNTL_26, 0, 0},
	{R_0286B0_SPI_PS_INPUT_CNTL_27, 0, 0},
	{R_0286B4_SPI_PS_INPUT_CNTL_28, 0, 0},
	{R_0286B8_SPI_PS_INPUT_CNTL_29, 0, 0},
	{R_0286BC_SPI_PS_INPUT_CNTL_30, 0, 0},
	{R_0286C0_SPI_PS_INPUT_CNTL_31, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_0286C4_SPI_VS_OUT_CONFIG, 0, 0},
	{R_0286C8_SPI_THREAD_GROUPING, 0, 0},
	{R_0286CC_SPI_PS_IN_CONTROL_0, 0, 0},
	{R_0286D0_SPI_PS_IN_CONTROL_1, 0, 0},
	{R_0286D8_SPI_INPUT_Z, 0, 0},
	{R_0286E0_SPI_BARYC_CNTL, 0, 0},
	{R_0286E4_SPI_PS_IN_CONTROL_2, 0, 0},
	{R_0286E8_SPI_COMPUTE_INPUT_CNTL, 0, 0},
	{R_028840_SQ_PGM_START_PS, REG_FLAG_NEED_BO, 0},
	{R_028844_SQ_PGM_RESOURCES_PS, 0, 0},
	{R_02884C_SQ_PGM_EXPORTS_PS, 0, 0},
	{R_02885C_SQ_PGM_START_VS, REG_FLAG_NEED_BO, 0},
	{R_028860_SQ_PGM_RESOURCES_VS, 0, 0},
	{R_0288EC_SQ_LDS_ALLOC_PS, 0, 0},
	{R_028ABC_DB_HTILE_SURFACE, 0, 0},
	{R_028B54_VGT_SHADER_STAGES_EN, 0, 0},
};

static const struct r600_reg cayman_context_reg_list[] = {
	{R_028010_DB_RENDER_OVERRIDE2, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028014_DB_HTILE_DATA_BASE, REG_FLAG_NEED_BO, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_028234_PA_SU_HARDWARE_SCREEN_OFFSET, 0, 0},
	{GROUP_FORCE_NEW_BLOCK, 0, 0},
	{R_02861C_SPI_VS_OUT_ID_0, 0, 0},
	{R_028620_SPI_VS_OUT_ID_1, 0, 0},
	{R_028624_SPI_VS_OUT_ID_2, 0, 0},
	{R_028628_SPI_VS_OUT_ID_3, 0, 0},
	{R_02862C_SPI_VS_OUT_ID_4, 0, 0},
	{R_028630_SPI_VS_OUT_ID_5, 0, 0},
	{R_028634_SPI_VS_OUT_ID_6, 0, 0},
	{R_028638_SPI_VS_OUT_ID_7, 0, 0},
	{R_02863C_SPI_VS_OUT_ID_8, 0, 0},
	{R_028640_SPI_VS_OUT_ID_9, 0, 0},
	{R_028644_SPI_PS_INPUT_CNTL_0, 0, 0},
	{R_028648_SPI_PS_INPUT_CNTL_1, 0, 0},
	{R_02864C_SPI_PS_INPUT_CNTL_2, 0, 0},
	{R_028650_SPI_PS_INPUT_CNTL_3, 0, 0},
	{R_028654_SPI_PS_INPUT_CNTL_4, 0, 0},
	{R_028658_SPI_PS_INPUT_CNTL_5, 0, 0},
	{R_02865C_SPI_PS_INPUT_CNTL_6, 0, 0},
	{R_028660_SPI_PS_INPUT_CNTL_7, 0, 0},
	{R_028664_SPI_PS_INPUT_CNTL_8, 0, 0},
	{R_028668_SPI_PS_INPUT_CNTL_9, 0, 0},
	{R_02866C_SPI_PS_INPUT_CNTL_10, 0, 0},
	{R_028670_SPI_PS_INPUT_CNTL_11, 0, 0},
	{R_028674_SPI_PS_INPUT_CNTL_12, 0, 0},
	{R_028678_SPI_PS_INPUT_CNTL_13, 0, 0},
	{R_02867C_SPI_PS_INPUT_CNTL_14, 0, 0},
	{R_028680_SPI_PS_INPUT_CNTL_15, 0, 0},
	{R_028684_SPI_PS_INPUT_CNTL_16, 0, 0},
	{R_028688_SPI_PS_INPUT_CNTL_17, 0, 0},
	{R_02868C_SPI_PS_INPUT_CNTL_18, 0, 0},
	{R_028690_SPI_PS_INPUT_CNTL_19, 0, 0},
	{R_028694_SPI_PS_INPUT_CNTL_20, 0, 0},
	{R_028698_SPI_PS_INPUT_CNTL_21, 0, 0},
	{R_02869C_SPI_PS_INPUT_CNTL_22, 0, 0},
	{R_0286A0_SPI_PS_INPUT_CNTL_23, 0, 0},
	{R_0286A4_SPI_PS_INPUT_CNTL_24, 0, 0},
	{R_0286A8_SPI_PS_INPUT_CNTL_25, 0, 0},
	{R_0286AC_SPI_PS_INPUT_CNTL_26, 0, 0},
	{R_0286B0_SPI_PS_INPUT_CNTL_27, 0, 0},
	{R_0286B4_SPI_PS_INPUT_CNTL_28, 0, 0},
	{R_0286B8_SPI_PS_INPUT_CNTL_29, 0, 0},
	{R_0286BC_SPI_PS_INPUT_CNTL_30, 0, 0},
	{R_0286C0_SPI_PS_INPUT_CNTL_31, 0, 0},
	{R_0286C4_SPI_VS_OUT_CONFIG, 0, 0},
	{R_0286C8_SPI_THREAD_GROUPING, 0, 0},
	{R_0286CC_SPI_PS_IN_CONTROL_0, 0, 0},
	{R_0286D0_SPI_PS_IN_CONTROL_1, 0, 0},
	{R_0286D8_SPI_INPUT_Z, 0, 0},
	{R_0286E0_SPI_BARYC_CNTL, 0, 0},
	{R_0286E4_SPI_PS_IN_CONTROL_2, 0, 0},
	{R_0286E8_SPI_COMPUTE_INPUT_CNTL, 0, 0},
	{R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1, 0, 0},
	{R_028840_SQ_PGM_START_PS, REG_FLAG_NEED_BO, 0},
	{R_028844_SQ_PGM_RESOURCES_PS, 0, 0},
	{R_02884C_SQ_PGM_EXPORTS_PS, 0, 0},
	{R_02885C_SQ_PGM_START_VS, REG_FLAG_NEED_BO, 0},
	{R_028860_SQ_PGM_RESOURCES_VS, 0, 0},
	{R_028900_SQ_ESGS_RING_ITEMSIZE, 0, 0},
	{R_028904_SQ_GSVS_RING_ITEMSIZE, 0, 0},
	{R_028908_SQ_ESTMP_RING_ITEMSIZE, 0, 0},
	{R_02890C_SQ_GSTMP_RING_ITEMSIZE, 0, 0},
	{R_028910_SQ_VSTMP_RING_ITEMSIZE, 0, 0},
	{R_028914_SQ_PSTMP_RING_ITEMSIZE, 0, 0},
	{R_02891C_SQ_GS_VERT_ITEMSIZE, 0, 0},
	{R_028920_SQ_GS_VERT_ITEMSIZE_1, 0, 0},
	{R_028924_SQ_GS_VERT_ITEMSIZE_2, 0, 0},
	{R_028928_SQ_GS_VERT_ITEMSIZE_3, 0, 0},
	{R_028ABC_DB_HTILE_SURFACE, 0, 0},
	{R_028B54_VGT_SHADER_STAGES_EN, 0, 0},
};

int evergreen_context_init(struct r600_context *ctx)
{
	int r = 0;

	/* add blocks */
	if (ctx->family >= CHIP_CAYMAN)
		r = r600_context_add_block(ctx, cayman_config_reg_list,
					   Elements(cayman_config_reg_list), PKT3_SET_CONFIG_REG, EVERGREEN_CONFIG_REG_OFFSET);
	if (r)
		goto out_err;
	if (ctx->family >= CHIP_CAYMAN)
		r = r600_context_add_block(ctx, cayman_context_reg_list,
					   Elements(cayman_context_reg_list), PKT3_SET_CONTEXT_REG, EVERGREEN_CONTEXT_REG_OFFSET);
	else
		r = r600_context_add_block(ctx, evergreen_context_reg_list,
					   Elements(evergreen_context_reg_list), PKT3_SET_CONTEXT_REG, EVERGREEN_CONTEXT_REG_OFFSET);
	if (r)
		goto out_err;

	r = r600_setup_block_table(ctx);
	if (r)
		goto out_err;

	ctx->max_db = 8;
	return 0;
out_err:
	r600_context_fini(ctx);
	return r;
}

void evergreen_flush_vgt_streamout(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	r600_write_config_reg(cs, R_0084FC_CP_STRMOUT_CNTL, 0);

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
	struct radeon_winsys_cs *cs = ctx->rings.gfx.cs;

	if (buffer_enable_bit) {
		r600_write_context_reg_seq(cs, R_028B94_VGT_STRMOUT_CONFIG, 2);
		r600_write_value(cs, S_028B94_STREAMOUT_0_EN(1)); /* R_028B94_VGT_STRMOUT_CONFIG */
		r600_write_value(cs, S_028B98_STREAM_0_BUFFER_EN(buffer_enable_bit)); /* R_028B98_VGT_STRMOUT_BUFFER_CONFIG */
	} else {
		r600_write_context_reg(cs, R_028B94_VGT_STRMOUT_CONFIG, S_028B94_STREAMOUT_0_EN(0));
	}
}

void evergreen_dma_copy(struct r600_context *rctx,
		struct pipe_resource *dst,
		struct pipe_resource *src,
		uint64_t dst_offset,
		uint64_t src_offset,
		uint64_t size)
{
	struct radeon_winsys_cs *cs = rctx->rings.dma.cs;
	unsigned i, ncopy, csize, sub_cmd, shift;
	struct r600_resource *rdst = (struct r600_resource*)dst;
	struct r600_resource *rsrc = (struct r600_resource*)src;

	/* make sure that the dma ring is only one active */
	rctx->rings.gfx.flush(rctx, RADEON_FLUSH_ASYNC);
	dst_offset += r600_resource_va(&rctx->screen->screen, dst);
	src_offset += r600_resource_va(&rctx->screen->screen, src);

	/* see if we use dword or byte copy */
	if (!(dst_offset & 0x3) && !(src_offset & 0x3) && !(size & 0x3)) {
		size >>= 2;
		sub_cmd = 0x00;
		shift = 2;
	} else {
		sub_cmd = 0x40;
		shift = 0;
	}
	ncopy = (size / 0x000fffff) + !!(size % 0x000fffff);

	r600_need_dma_space(rctx, ncopy * 5);
	for (i = 0; i < ncopy; i++) {
		csize = size < 0x000fffff ? size : 0x000fffff;
		/* emit reloc before writting cs so that cs is always in consistent state */
		r600_context_bo_reloc(rctx, &rctx->rings.dma, rsrc, RADEON_USAGE_READ);
		r600_context_bo_reloc(rctx, &rctx->rings.dma, rdst, RADEON_USAGE_WRITE);
		cs->buf[cs->cdw++] = DMA_PACKET(DMA_PACKET_COPY, sub_cmd, csize);
		cs->buf[cs->cdw++] = dst_offset & 0xffffffff;
		cs->buf[cs->cdw++] = src_offset & 0xffffffff;
		cs->buf[cs->cdw++] = (dst_offset >> 32UL) & 0xff;
		cs->buf[cs->cdw++] = (src_offset >> 32UL) & 0xff;
		dst_offset += csize << shift;
		src_offset += csize << shift;
		size -= csize;
	}
}
