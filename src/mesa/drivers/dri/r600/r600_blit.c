/*
 * Copyright (C) 2009 Advanced Micro Devices, Inc.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "radeon_common.h"
#include "r600_context.h"

#include "r600_blit.h"
#include "r600_cmdbuf.h"

static inline void
set_render_target(drm_radeon_private_t *dev_priv, int format, int w, int h, u64 gpu_addr)
{
    int id = 0;

    nPitchInPixel = rrb->pitch/rrb->cpp;
    SETfield(cb_color0_size, (nPitchInPixel / 8) - 1,
             PITCH_TILE_MAX_shift, PITCH_TILE_MAX_mask);
    SETfield(cb_color0_size, ((nPitchInPixel * height) / 64) - 1,
             SLICE_TILE_MAX_shift, SLICE_TILE_MAX_mask);
    SETfield(cb_color0_info, ENDIAN_NONE, ENDIAN_shift, ENDIAN_mask);
    SETfield(cb_color0_info, ARRAY_LINEAR_GENERAL,
             cb_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
    if(4 == rrb->cpp)
    {
        SETfield(cb_color0_info, COLOR_8_8_8_8,
                 CB_COLOR0_INFO__FORMAT_shift, CB_COLOR0_INFO__FORMAT_mask);
        SETfield(cb_color0_info, SWAP_ALT, COMP_SWAP_shift, COMP_SWAP_mask);
    }
    else
    {
        SETfield(cb_color0_info, COLOR_5_6_5,
                 CB_COLOR0_INFO__FORMAT_shift, CB_COLOR0_INFO__FORMAT_mask);
        SETfield(cb_color0_info, SWAP_ALT_REV,
                 COMP_SWAP_shift, COMP_SWAP_mask);
    }
    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
    SETbit(cb_color0_info, BLEND_CLAMP_bit);
    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(CB_COLOR0_BASE + (4 * id), 1);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH_RELOC(0,
			 bo,
			 0,
			 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    END_BATCH();

    if ((context->radeon.radeonScreen->chip_family > CHIP_FAMILY_R600) &&
	(context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770)) {
	    BEGIN_BATCH_NO_AUTOSTATE(2);
	    R600_OUT_BATCH(CP_PACKET3(R600_IT_SURFACE_BASE_UPDATE, 0));
	    R600_OUT_BATCH((2 << id));
	    END_BATCH();
    }

    BEGIN_BATCH_NO_AUTOSTATE(18);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_SIZE + (4 * id), cb_color0_size);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_VIEW + (4 * id), cb_color0_view);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_INFO + (4 * id), cb_color0_info);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_TILE + (4 * id), 0);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_FRAG + (4 * id), 0);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_MASK + (4 * id), 0);
    END_BATCH();

    COMMIT_BATCH();

}

static inline void
set_shaders(struct drm_device *dev)
{
    /* FS */
    pbo = (struct radeon_bo *)r700GetActiveVpShaderBo(GL_CONTEXT(context));
    r700->fs.SQ_PGM_START_FS.u32All = r700->vs.SQ_PGM_START_VS.u32All;
    r700->fs.SQ_PGM_RESOURCES_FS.u32All = 0;
    r700->fs.SQ_PGM_CF_OFFSET_FS.u32All = 0;
    /* XXX */

    if (!pbo)
	    return;

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_FS, 1);
    R600_OUT_BATCH(r700->fs.SQ_PGM_START_FS.u32All);
    R600_OUT_BATCH_RELOC(r700->fs.SQ_PGM_START_FS.u32All,
			 pbo,
			 r700->fs.SQ_PGM_START_FS.u32All,
			 RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_FS, r700->fs.SQ_PGM_RESOURCES_FS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_FS, r700->fs.SQ_PGM_CF_OFFSET_FS.u32All);
    END_BATCH();

    /* VS */
    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_VS, 1);
    R600_OUT_BATCH(r700->vs.SQ_PGM_START_VS.u32All);
    R600_OUT_BATCH_RELOC(r700->vs.SQ_PGM_START_VS.u32All,
		         pbo,
		         r700->vs.SQ_PGM_START_VS.u32All,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_VS, r700->vs.SQ_PGM_RESOURCES_VS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_VS, r700->vs.SQ_PGM_CF_OFFSET_VS.u32All);
    END_BATCH();

    /* PS */
    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_PS, 1);
    R600_OUT_BATCH(r700->ps.SQ_PGM_START_PS.u32All);
    R600_OUT_BATCH_RELOC(r700->ps.SQ_PGM_START_PS.u32All,
		         pbo,
		         r700->ps.SQ_PGM_START_PS.u32All,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(9);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_PS, r700->ps.SQ_PGM_RESOURCES_PS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_EXPORTS_PS, r700->ps.SQ_PGM_EXPORTS_PS.u32All);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_PS, r700->ps.SQ_PGM_CF_OFFSET_PS.u32All);
    END_BATCH();

    COMMIT_BATCH();

}

static inline void
set_vtx_resource(drm_radeon_private_t *dev_priv, u64 gpu_addr)
{

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_BASE_VTX_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_START_INST_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);
    END_BATCH();
    COMMIT_BATCH();

    if ((context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV610) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV620) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS780) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS880) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV710))
	    r700SyncSurf(context, bo, RADEON_GEM_DOMAIN_GTT, 0, TC_ACTION_ENA_bit);
    else
	    r700SyncSurf(context, bo, RADEON_GEM_DOMAIN_GTT, 0, VC_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(9 + 2);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
    R600_OUT_BATCH(SQ_FETCH_RESOURCE_VS_OFFSET * FETCH_RESOURCE_STRIDE);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(48 - 1);
    R600_OUT_BATCH(16 << SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift);
    R600_OUT_BATCH(1 << MEM_REQUEST_SIZE_shift);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(SQ_TEX_VTX_VALID_BUFFER << SQ_TEX_RESOURCE_WORD6_0__TYPE_shift);
    R600_OUT_BATCH_RELOC(uSQ_VTX_CONSTANT_WORD0_0,
                         paos->bo,
                         uSQ_VTX_CONSTANT_WORD0_0,
                         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
    COMMIT_BATCH();

}

static inline void
set_tex_resource(drm_radeon_private_t *dev_priv,
		 int format, int w, int h, int pitch, u64 gpu_addr)
{
	uint32_t sq_tex_resource_word0, sq_tex_resource_word1, sq_tex_resource_word4;
	RING_LOCALS;
	DRM_DEBUG("\n");

	r700SyncSurf(context, bo,
		     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
		     0, TC_ACTION_ENA_bit);

	BEGIN_BATCH_NO_AUTOSTATE(9 + 4);
	R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
	R600_OUT_BATCH(i * 7);

	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE0);
	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE1);
	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE2);
	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE3);
	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE4);
	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE5);
	R600_OUT_BATCH(r700->textures[i]->SQ_TEX_RESOURCE6);
	R600_OUT_BATCH_RELOC(r700->textures[i]->SQ_TEX_RESOURCE2,
			     bo,
			     offset,
			     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
	R600_OUT_BATCH_RELOC(r700->textures[i]->SQ_TEX_RESOURCE3,
			     bo,
			     r700->textures[i]->SQ_TEX_RESOURCE3,
			     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
	END_BATCH();
	COMMIT_BATCH();

	if (h < 1)
		h = 1;

	sq_tex_resource_word0 = (1 << 0);
	sq_tex_resource_word0 |= ((((pitch >> 3) - 1) << 8) |
				  ((w - 1) << 19));

	sq_tex_resource_word1 = (format << 26);
	sq_tex_resource_word1 |= ((h - 1) << 0);

	sq_tex_resource_word4 = ((1 << 14) |
				 (0 << 16) |
				 (1 << 19) |
				 (2 << 22) |
				 (3 << 25));

	BEGIN_RING(9);
	OUT_RING(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
	OUT_RING(0);
	OUT_RING(sq_tex_resource_word0);
	OUT_RING(sq_tex_resource_word1);
	OUT_RING(gpu_addr >> 8);
	OUT_RING(gpu_addr >> 8);
	OUT_RING(sq_tex_resource_word4);
	OUT_RING(0);
	OUT_RING(R600_SQ_TEX_VTX_VALID_TEXTURE << 30);
	ADVANCE_RING();

}

static inline void
set_scissors(drm_radeon_private_t *dev_priv, int x1, int y1, int x2, int y2)
{
	RING_LOCALS;
	DRM_DEBUG("\n");

	BEGIN_BATCH_NO_AUTOSTATE(22);
	R600_OUT_BATCH_REGSEQ(PA_SC_SCREEN_SCISSOR_TL, 2);
	R600_OUT_BATCH(r700->PA_SC_SCREEN_SCISSOR_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_SCREEN_SCISSOR_BR.u32All);

	R600_OUT_BATCH_REGSEQ(PA_SC_WINDOW_OFFSET, 12);
	R600_OUT_BATCH(r700->PA_SC_WINDOW_OFFSET.u32All);
	R600_OUT_BATCH(r700->PA_SC_WINDOW_SCISSOR_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_WINDOW_SCISSOR_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_RULE.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_0_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_0_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_1_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_1_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_2_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_2_BR.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_3_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_CLIPRECT_3_BR.u32All);

	R600_OUT_BATCH_REGSEQ(PA_SC_GENERIC_SCISSOR_TL, 2);
	R600_OUT_BATCH(r700->PA_SC_GENERIC_SCISSOR_TL.u32All);
	R600_OUT_BATCH(r700->PA_SC_GENERIC_SCISSOR_BR.u32All);
	END_BATCH();
	COMMIT_BATCH();


	BEGIN_RING(12);
	OUT_RING(CP_PACKET3(R600_IT_SET_CONTEXT_REG, 2));
	OUT_RING((R600_PA_SC_SCREEN_SCISSOR_TL - R600_SET_CONTEXT_REG_OFFSET) >> 2);
	OUT_RING((x1 << 0) | (y1 << 16));
	OUT_RING((x2 << 0) | (y2 << 16));

	OUT_RING(CP_PACKET3(R600_IT_SET_CONTEXT_REG, 2));
	OUT_RING((R600_PA_SC_GENERIC_SCISSOR_TL - R600_SET_CONTEXT_REG_OFFSET) >> 2);
	OUT_RING((x1 << 0) | (y1 << 16) | (1 << 31));
	OUT_RING((x2 << 0) | (y2 << 16));

	OUT_RING(CP_PACKET3(R600_IT_SET_CONTEXT_REG, 2));
	OUT_RING((R600_PA_SC_WINDOW_SCISSOR_TL - R600_SET_CONTEXT_REG_OFFSET) >> 2);
	OUT_RING((x1 << 0) | (y1 << 16) | (1 << 31));
	OUT_RING((x2 << 0) | (y2 << 16));
	ADVANCE_RING();
}

static inline void
draw_auto(drm_radeon_private_t *dev_priv)
{
	RING_LOCALS;
	DRM_DEBUG("\n");

	BEGIN_RING(10);
	OUT_RING(CP_PACKET3(R600_IT_SET_CONFIG_REG, 1));
	OUT_RING((R600_VGT_PRIMITIVE_TYPE - R600_SET_CONFIG_REG_OFFSET) >> 2);
	OUT_RING(DI_PT_RECTLIST);

	OUT_RING(CP_PACKET3(R600_IT_INDEX_TYPE, 0));
	OUT_RING(DI_INDEX_SIZE_16_BIT);

	OUT_RING(CP_PACKET3(R600_IT_NUM_INSTANCES, 0));
	OUT_RING(1);

	OUT_RING(CP_PACKET3(R600_IT_DRAW_INDEX_AUTO, 1));
	OUT_RING(3);
	OUT_RING(DI_SRC_SEL_AUTO_INDEX);

	ADVANCE_RING();
	COMMIT_RING();
}

static inline void
set_default_state(drm_radeon_private_t *dev_priv)
{
	int i;
	u32 sq_config, sq_gpr_resource_mgmt_1, sq_gpr_resource_mgmt_2;
	u32 sq_thread_resource_mgmt, sq_stack_resource_mgmt_1, sq_stack_resource_mgmt_2;
	int num_ps_gprs, num_vs_gprs, num_temp_gprs, num_gs_gprs, num_es_gprs;
	int num_ps_threads, num_vs_threads, num_gs_threads, num_es_threads;
	int num_ps_stack_entries, num_vs_stack_entries, num_gs_stack_entries, num_es_stack_entries;
	RING_LOCALS;

	switch ((dev_priv->flags & RADEON_FAMILY_MASK)) {
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

	if (((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV610) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV620) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS780) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RS880) ||
	    ((dev_priv->flags & RADEON_FAMILY_MASK) == CHIP_RV710))
		sq_config = 0;
	else
		sq_config = R600_VC_ENABLE;

	sq_config |= (R600_DX9_CONSTS |
		      R600_ALU_INST_PREFER_VECTOR |
		      R600_PS_PRIO(0) |
		      R600_VS_PRIO(1) |
		      R600_GS_PRIO(2) |
		      R600_ES_PRIO(3));

	sq_gpr_resource_mgmt_1 = (R600_NUM_PS_GPRS(num_ps_gprs) |
				  R600_NUM_VS_GPRS(num_vs_gprs) |
				  R600_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs));
	sq_gpr_resource_mgmt_2 = (R600_NUM_GS_GPRS(num_gs_gprs) |
				  R600_NUM_ES_GPRS(num_es_gprs));
	sq_thread_resource_mgmt = (R600_NUM_PS_THREADS(num_ps_threads) |
				   R600_NUM_VS_THREADS(num_vs_threads) |
				   R600_NUM_GS_THREADS(num_gs_threads) |
				   R600_NUM_ES_THREADS(num_es_threads));
	sq_stack_resource_mgmt_1 = (R600_NUM_PS_STACK_ENTRIES(num_ps_stack_entries) |
				    R600_NUM_VS_STACK_ENTRIES(num_vs_stack_entries));
	sq_stack_resource_mgmt_2 = (R600_NUM_GS_STACK_ENTRIES(num_gs_stack_entries) |
				    R600_NUM_ES_STACK_ENTRIES(num_es_stack_entries));

	if ((dev_priv->flags & RADEON_FAMILY_MASK) >= CHIP_RV770) {
		BEGIN_RING(r7xx_default_size + 10);
		for (i = 0; i < r7xx_default_size; i++)
			OUT_RING(r7xx_default_state[i]);
	} else {
		BEGIN_RING(r6xx_default_size + 10);
		for (i = 0; i < r6xx_default_size; i++)
			OUT_RING(r6xx_default_state[i]);
	}
	OUT_RING(CP_PACKET3(R600_IT_EVENT_WRITE, 0));
	OUT_RING(R600_CACHE_FLUSH_AND_INV_EVENT);
	/* SQ config */
	OUT_RING(CP_PACKET3(R600_IT_SET_CONFIG_REG, 6));
	OUT_RING((R600_SQ_CONFIG - R600_SET_CONFIG_REG_OFFSET) >> 2);
	OUT_RING(sq_config);
	OUT_RING(sq_gpr_resource_mgmt_1);
	OUT_RING(sq_gpr_resource_mgmt_2);
	OUT_RING(sq_thread_resource_mgmt);
	OUT_RING(sq_stack_resource_mgmt_1);
	OUT_RING(sq_stack_resource_mgmt_2);
	ADVANCE_RING();
}

static GLboolean validate_buffers(struct r300_context *r300,
                                  struct radeon_bo *src_bo,
                                  struct radeon_bo *dst_bo)
{
    int ret;
    radeon_cs_space_add_persistent_bo(r300->radeon.cmdbuf.cs,
                                      src_bo, RADEON_GEM_DOMAIN_VRAM, 0);

    radeon_cs_space_add_persistent_bo(r300->radeon.cmdbuf.cs,
                                      dst_bo, 0, RADEON_GEM_DOMAIN_VRAM);

    ret = radeon_cs_space_check_with_bo(r300->radeon.cmdbuf.cs,
                                        first_elem(&r300->radeon.dma.reserved)->bo,
                                        RADEON_GEM_DOMAIN_GTT, 0);
    if (ret)
        return GL_FALSE;

    return GL_TRUE;
}

GLboolean r600_blit(struct r300_context *r300,
                    struct radeon_bo *src_bo,
                    intptr_t src_offset,
                    gl_format src_mesaformat,
                    unsigned src_pitch,
                    unsigned src_width,
                    unsigned src_height,
                    struct radeon_bo *dst_bo,
                    intptr_t dst_offset,
                    gl_format dst_mesaformat,
                    unsigned dst_pitch,
                    unsigned dst_width,
                    unsigned dst_height)
{
    int id = 0;

    if (src_bo == dst_bo) {
        return GL_FALSE;
    }

    if (0) {
        fprintf(stderr, "src: width %d, height %d, pitch %d vs %d, format %s\n",
                src_width, src_height, src_pitch,
                _mesa_format_row_stride(src_mesaformat, src_width),
                _mesa_get_format_name(src_mesaformat));
        fprintf(stderr, "dst: width %d, height %d, pitch %d, format %s\n",
                dst_width, dst_height,
                _mesa_format_row_stride(dst_mesaformat, dst_width),
                _mesa_get_format_name(dst_mesaformat));
    }

    if (!validate_buffers(r300, src_bo, dst_bo))
        return GL_FALSE;

    rcommonEnsureCmdBufSpace(&r300->radeon, 200, __FUNCTION__);

    /* src */
    set_tex_resource(dev_priv, tex_format,
		     src_pitch / cpp,
		     sy2, src_pitch / cpp,
		     src_gpu_addr);

    r700SyncSurf(context, src_bo,
		 RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
		 0, TC_ACTION_ENA_bit);

    /* dst */
    set_render_target(dev_priv, cb_format,
		      dst_pitch / cpp, dy2,
		      dst_gpu_addr);

    /* scissors */
    set_scissors(dev_priv, 0, 0, width, height);

    /* Vertex buffer setup */
    set_vtx_resource(dev_priv, vb_addr);

    /* draw */
    draw_auto(dev_priv);

    r700SyncSurf(context, dst_bo, 0, RADEON_GEM_DOMAIN_VRAM,
		 CB_ACTION_ENA_bit | (1 << (id + 6)));

    radeonFlush(r300->radeon.glCtx);

    return GL_TRUE;
}
