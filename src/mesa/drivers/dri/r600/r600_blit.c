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
#include "r600_blit_shaders.h"
#include "r600_cmdbuf.h"

/* common formats supported as both textures and render targets */
unsigned r600_check_blit(gl_format mesa_format)
{
    switch (mesa_format) {
    case MESA_FORMAT_RGBA8888:
    case MESA_FORMAT_SIGNED_RGBA8888:
    case MESA_FORMAT_RGBA8888_REV:
    case MESA_FORMAT_SIGNED_RGBA8888_REV:
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
    case MESA_FORMAT_ARGB8888_REV:
    case MESA_FORMAT_XRGB8888_REV:
    case MESA_FORMAT_RGB565:
    case MESA_FORMAT_RGB565_REV:
    case MESA_FORMAT_ARGB4444:
    case MESA_FORMAT_ARGB4444_REV:
    case MESA_FORMAT_ARGB1555:
    case MESA_FORMAT_ARGB1555_REV:
    case MESA_FORMAT_AL88:
    case MESA_FORMAT_AL88_REV:
    case MESA_FORMAT_RGB332:
    case MESA_FORMAT_A8:
    case MESA_FORMAT_I8:
    case MESA_FORMAT_CI8:
    case MESA_FORMAT_L8:
    case MESA_FORMAT_RGBA_FLOAT32:
    case MESA_FORMAT_RGBA_FLOAT16:
    case MESA_FORMAT_ALPHA_FLOAT32:
    case MESA_FORMAT_ALPHA_FLOAT16:
    case MESA_FORMAT_LUMINANCE_FLOAT32:
    case MESA_FORMAT_LUMINANCE_FLOAT16:
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
    case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
    case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
    case MESA_FORMAT_X8_Z24:
    case MESA_FORMAT_S8_Z24:
    case MESA_FORMAT_Z24_S8:
    case MESA_FORMAT_Z16:
    case MESA_FORMAT_Z32:
    case MESA_FORMAT_SRGBA8:
    case MESA_FORMAT_SLA8:
    case MESA_FORMAT_SL8:
	    break;
    default:
	    return 0;
    }

    /* ??? */
    /* not sure blit to depth works or not yet */
    if (_mesa_get_format_bits(mesa_format, GL_DEPTH_BITS) > 0)
	    return 0;

    return 1;
}

static inline void
set_render_target(context_t *context, struct radeon_bo *bo, gl_format mesa_format,
                  int nPitchInPixel, int w, int h, intptr_t dst_offset)
{
    uint32_t cb_color0_base, cb_color0_size = 0, cb_color0_info = 0, cb_color0_view = 0;
    int id = 0;
    uint32_t comp_swap, format;
    BATCH_LOCALS(&context->radeon);

    cb_color0_base = dst_offset / 256;

    SETfield(cb_color0_size, (nPitchInPixel / 8) - 1,
             PITCH_TILE_MAX_shift, PITCH_TILE_MAX_mask);
    SETfield(cb_color0_size, ((nPitchInPixel * h) / 64) - 1,
             SLICE_TILE_MAX_shift, SLICE_TILE_MAX_mask);

    SETfield(cb_color0_info, ENDIAN_NONE, ENDIAN_shift, ENDIAN_mask);
    SETfield(cb_color0_info, ARRAY_LINEAR_GENERAL,
             CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);

    SETbit(cb_color0_info, BLEND_BYPASS_bit);

    switch(mesa_format) {
    case MESA_FORMAT_RGBA8888:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_SIGNED_RGBA8888:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_SNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_RGBA8888_REV:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_SIGNED_RGBA8888_REV:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_SNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ARGB8888_REV:
    case MESA_FORMAT_XRGB8888_REV:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_RGB565:
            format = COLOR_5_6_5;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_RGB565_REV:
            format = COLOR_5_6_5;
            comp_swap = SWAP_STD;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ARGB4444:
            format = COLOR_4_4_4_4;
            comp_swap = SWAP_ALT;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ARGB4444_REV:
            format = COLOR_4_4_4_4;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ARGB1555:
            format = COLOR_1_5_5_5;
            comp_swap = SWAP_ALT;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ARGB1555_REV:
            format = COLOR_1_5_5_5;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_AL88:
            format = COLOR_8_8;
            comp_swap = SWAP_STD;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_AL88_REV:
            format = COLOR_8_8;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_RGB332:
            format = COLOR_3_3_2;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_A8:
            format = COLOR_8;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_I8:
    case MESA_FORMAT_CI8:
            format = COLOR_8;
            comp_swap = SWAP_STD;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_L8:
            format = COLOR_8;
            comp_swap = SWAP_ALT;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_RGBA_FLOAT32:
            format = COLOR_32_32_32_32_FLOAT;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, BLEND_FLOAT32_bit);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_RGBA_FLOAT16:
            format = COLOR_16_16_16_16_FLOAT;
            comp_swap = SWAP_STD_REV;
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ALPHA_FLOAT32:
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, BLEND_FLOAT32_bit);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_ALPHA_FLOAT16:
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_LUMINANCE_FLOAT32:
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_ALT;
	    SETbit(cb_color0_info, BLEND_FLOAT32_bit);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_LUMINANCE_FLOAT16:
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_ALT;
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
            format = COLOR_32_32_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, BLEND_FLOAT32_bit);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
            format = COLOR_16_16_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_STD;
	    SETbit(cb_color0_info, BLEND_FLOAT32_bit);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_STD;
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_FLOAT, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_X8_Z24:
    case MESA_FORMAT_S8_Z24:
            format = COLOR_8_24;
            comp_swap = SWAP_STD;
	    SETfield(cb_color0_info, ARRAY_1D_TILED_THIN1,
		     CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_Z24_S8:
            format = COLOR_24_8;
            comp_swap = SWAP_STD;
	    SETfield(cb_color0_info, ARRAY_1D_TILED_THIN1,
		     CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_Z16:
            format = COLOR_16;
            comp_swap = SWAP_STD;
	    SETfield(cb_color0_info, ARRAY_1D_TILED_THIN1,
		     CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_Z32:
            format = COLOR_32;
            comp_swap = SWAP_STD;
	    SETfield(cb_color0_info, ARRAY_1D_TILED_THIN1,
		     CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
	    CLEARbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_SRGBA8:
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_SRGB, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_SLA8:
            format = COLOR_8_8;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_SRGB, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    case MESA_FORMAT_SL8:
            format = COLOR_8;
            comp_swap = SWAP_ALT_REV;
	    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
	    SETfield(cb_color0_info, NUMBER_SRGB, NUMBER_TYPE_shift, NUMBER_TYPE_mask);
            break;
    default:
            fprintf(stderr,"Invalid format for copy %s\n",_mesa_get_format_name(mesa_format));
            assert("Invalid format for US output\n");
            return;
    }

    SETfield(cb_color0_info, format, CB_COLOR0_INFO__FORMAT_shift,
             CB_COLOR0_INFO__FORMAT_mask);
    SETfield(cb_color0_info, comp_swap, COMP_SWAP_shift, COMP_SWAP_mask);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(CB_COLOR0_BASE + (4 * id), 1);
    R600_OUT_BATCH(cb_color0_base);
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

    /* Set CMASK & TILE buffer to the offset of color buffer as
     * we don't use those this shouldn't cause any issue and we
     * then have a valid cmd stream
     */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(CB_COLOR0_TILE + (4 * id), 1);
    R600_OUT_BATCH(cb_color0_base);
    R600_OUT_BATCH_RELOC(0,
			 bo,
			 0,
			 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    END_BATCH();
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(CB_COLOR0_FRAG + (4 * id), 1);
    R600_OUT_BATCH(cb_color0_base);
    R600_OUT_BATCH_RELOC(0,
			 bo,
			 0,
			 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(12);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_SIZE + (4 * id), cb_color0_size);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_VIEW + (4 * id), cb_color0_view);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_INFO + (4 * id), cb_color0_info);
    R600_OUT_BATCH_REGVAL(CB_COLOR0_MASK + (4 * id), 0);
    END_BATCH();

    COMMIT_BATCH();

}

static inline void load_shaders(GLcontext * ctx)
{

    radeonContextPtr radeonctx = RADEON_CONTEXT(ctx);
    context_t *context = R700_CONTEXT(ctx);
    int i, size;
    uint32_t *shader;

    if (context->blit_bo_loaded == 1)
        return;

    size = 4096;
    context->blit_bo = radeon_bo_open(radeonctx->radeonScreen->bom, 0,
                                      size, 256, RADEON_GEM_DOMAIN_GTT, 0);
    radeon_bo_map(context->blit_bo, 1);
    shader = context->blit_bo->ptr;

    for(i=0; i<sizeof(r6xx_vs)/4; i++) {
        shader[128+i] = r6xx_vs[i];
    }
    for(i=0; i<sizeof(r6xx_ps)/4; i++) {
        shader[256+i] = r6xx_ps[i];
    }

    radeon_bo_unmap(context->blit_bo);
    context->blit_bo_loaded = 1;

}

static inline void
set_shaders(context_t *context)
{
    struct radeon_bo * pbo = context->blit_bo;
    BATCH_LOCALS(&context->radeon);

    uint32_t sq_pgm_start_fs = (512 >> 8);
    uint32_t sq_pgm_resources_fs = 0;
    uint32_t sq_pgm_cf_offset_fs = 0;

    uint32_t sq_pgm_start_vs = (512 >> 8);
    uint32_t sq_pgm_resources_vs = (1 << NUM_GPRS_shift);
    uint32_t sq_pgm_cf_offset_vs = 0;

    uint32_t sq_pgm_start_ps = (1024 >> 8);
    uint32_t sq_pgm_resources_ps = (1 << NUM_GPRS_shift);
    uint32_t sq_pgm_cf_offset_ps = 0;
    uint32_t sq_pgm_exports_ps = (1 << 1);

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    /* FS */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_FS, 1);
    R600_OUT_BATCH(sq_pgm_start_fs);
    R600_OUT_BATCH_RELOC(sq_pgm_start_fs,
			 pbo,
			 sq_pgm_start_fs,
			 RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_FS, sq_pgm_resources_fs);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_FS, sq_pgm_cf_offset_fs);
    END_BATCH();

    /* VS */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_VS, 1);
    R600_OUT_BATCH(sq_pgm_start_vs);
    R600_OUT_BATCH_RELOC(sq_pgm_start_vs,
		         pbo,
		         sq_pgm_start_vs,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_VS, sq_pgm_resources_vs);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_VS, sq_pgm_cf_offset_vs);
    END_BATCH();

    /* PS */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    R600_OUT_BATCH_REGSEQ(SQ_PGM_START_PS, 1);
    R600_OUT_BATCH(sq_pgm_start_ps);
    R600_OUT_BATCH_RELOC(sq_pgm_start_ps,
		         pbo,
		         sq_pgm_start_ps,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(9);
    R600_OUT_BATCH_REGVAL(SQ_PGM_RESOURCES_PS, sq_pgm_resources_ps);
    R600_OUT_BATCH_REGVAL(SQ_PGM_EXPORTS_PS, sq_pgm_exports_ps);
    R600_OUT_BATCH_REGVAL(SQ_PGM_CF_OFFSET_PS, sq_pgm_cf_offset_ps);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(18);
    R600_OUT_BATCH_REGVAL(SPI_VS_OUT_CONFIG, 0); //EXPORT_COUNT is - 1
    R600_OUT_BATCH_REGVAL(SPI_VS_OUT_ID_0, 0);
    R600_OUT_BATCH_REGVAL(SPI_PS_INPUT_CNTL_0, SEL_CENTROID_bit);
    R600_OUT_BATCH_REGVAL(SPI_PS_IN_CONTROL_0, (1 << NUM_INTERP_shift));
    R600_OUT_BATCH_REGVAL(SPI_PS_IN_CONTROL_1, 0);
    R600_OUT_BATCH_REGVAL(SPI_INTERP_CONTROL_0, 0);
    END_BATCH();

    COMMIT_BATCH();

}

static inline void
set_vtx_resource(context_t *context)
{
    struct radeon_bo *bo = context->blit_bo;
    BATCH_LOCALS(&context->radeon);

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
    R600_OUT_BATCH_RELOC(SQ_VTX_CONSTANT_WORD0_0,
                         bo,
                         SQ_VTX_CONSTANT_WORD0_0,
                         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
    COMMIT_BATCH();

}

static inline void
set_tex_resource(context_t * context,
		 gl_format mesa_format, struct radeon_bo *bo, int w, int h,
		 int TexelPitch, intptr_t src_offset)
{
    uint32_t sq_tex_resource0, sq_tex_resource1, sq_tex_resource2, sq_tex_resource4, sq_tex_resource6;

    sq_tex_resource0 = sq_tex_resource1 = sq_tex_resource2 = sq_tex_resource4 = sq_tex_resource6 = 0;
    BATCH_LOCALS(&context->radeon);

    SETfield(sq_tex_resource0, SQ_TEX_DIM_2D, DIM_shift, DIM_mask);
    SETfield(sq_tex_resource0, ARRAY_LINEAR_GENERAL,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);

    switch (mesa_format) {
    case MESA_FORMAT_RGBA8888:
    case MESA_FORMAT_SIGNED_RGBA8888:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    if (mesa_format == MESA_FORMAT_SIGNED_RGBA8888) {
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);
	    }
	    break;
    case MESA_FORMAT_RGBA8888_REV:
    case MESA_FORMAT_SIGNED_RGBA8888_REV:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    if (mesa_format == MESA_FORMAT_SIGNED_RGBA8888_REV) {
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_X_shift, FORMAT_COMP_X_mask);
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_Y_shift, FORMAT_COMP_Y_mask);
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_Z_shift, FORMAT_COMP_Z_mask);
		    SETfield(sq_tex_resource4, SQ_FORMAT_COMP_SIGNED,
			     FORMAT_COMP_W_shift, FORMAT_COMP_W_mask);
	    }
	    break;
    case MESA_FORMAT_ARGB8888:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_XRGB8888:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ARGB8888_REV:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_XRGB8888_REV:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_RGB565:
	    SETfield(sq_tex_resource1, FMT_5_6_5,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_RGB565_REV:
	    SETfield(sq_tex_resource1, FMT_5_6_5,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ARGB4444:
	    SETfield(sq_tex_resource1, FMT_4_4_4_4,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ARGB4444_REV:
	    SETfield(sq_tex_resource1, FMT_4_4_4_4,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ARGB1555:
	    SETfield(sq_tex_resource1, FMT_1_5_5_5,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ARGB1555_REV:
	    SETfield(sq_tex_resource1, FMT_1_5_5_5,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_AL88:
    case MESA_FORMAT_AL88_REV: /* TODO : Check this. */
	    SETfield(sq_tex_resource1, FMT_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_RGB332:
	    SETfield(sq_tex_resource1, FMT_3_3_2,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_A8: /* ZERO, ZERO, ZERO, X */
	    SETfield(sq_tex_resource1, FMT_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_L8: /* X, X, X, ONE */
	    SETfield(sq_tex_resource1, FMT_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_I8: /* X, X, X, X */
    case MESA_FORMAT_CI8:
	    SETfield(sq_tex_resource1, FMT_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_RGBA_FLOAT32:
	    SETfield(sq_tex_resource1, FMT_32_32_32_32_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_RGBA_FLOAT16:
	    SETfield(sq_tex_resource1, FMT_16_16_16_16_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ALPHA_FLOAT32: /* ZERO, ZERO, ZERO, X */
	    SETfield(sq_tex_resource1, FMT_32_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_ALPHA_FLOAT16: /* ZERO, ZERO, ZERO, X */
	    SETfield(sq_tex_resource1, FMT_16_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_LUMINANCE_FLOAT32: /* X, X, X, ONE */
	    SETfield(sq_tex_resource1, FMT_32_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_LUMINANCE_FLOAT16: /* X, X, X, ONE */
	    SETfield(sq_tex_resource1, FMT_16_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
	    SETfield(sq_tex_resource1, FMT_32_32_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
	    SETfield(sq_tex_resource1, FMT_16_16_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
	    SETfield(sq_tex_resource1, FMT_32_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
	    SETfield(sq_tex_resource1, FMT_16_FLOAT,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_Z16:
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource1, FMT_16,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_X8_Z24:
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource1, FMT_8_24,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_S8_Z24:
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource1, FMT_8_24,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_Z24_S8:
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource1, FMT_24_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_0,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_Z32:
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource1, FMT_32,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_S8:
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource1, FMT_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_SRGBA8:
	    SETfield(sq_tex_resource1, FMT_8_8_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    SETbit(sq_tex_resource4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
	    break;
    case MESA_FORMAT_SLA8:
	    SETfield(sq_tex_resource1, FMT_8_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    SETbit(sq_tex_resource4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
	    break;
    case MESA_FORMAT_SL8: /* X, X, X, ONE */
	    SETfield(sq_tex_resource1, FMT_8,
		     SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift, SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_1,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    SETbit(sq_tex_resource4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
	    break;
    default:
            fprintf(stderr,"Invalid format for copy %s\n",_mesa_get_format_name(mesa_format));
            assert("Invalid format for US output\n");
            return;
    };

    SETfield(sq_tex_resource0, (TexelPitch/8)-1, PITCH_shift, PITCH_mask);
    SETfield(sq_tex_resource0, w - 1, TEX_WIDTH_shift, TEX_WIDTH_mask);
    SETfield(sq_tex_resource1, h - 1, TEX_HEIGHT_shift, TEX_HEIGHT_mask);

    sq_tex_resource2 = src_offset / 256;

    SETfield(sq_tex_resource6, SQ_TEX_VTX_VALID_TEXTURE,
             SQ_TEX_RESOURCE_WORD6_0__TYPE_shift,
             SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);

    r700SyncSurf(context, bo,
                 RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
		 0, TC_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(9 + 4);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 7));
    R600_OUT_BATCH(0 * 7);

    R600_OUT_BATCH(sq_tex_resource0);
    R600_OUT_BATCH(sq_tex_resource1);
    R600_OUT_BATCH(sq_tex_resource2);
    R600_OUT_BATCH(0); //SQ_TEX_RESOURCE3
    R600_OUT_BATCH(sq_tex_resource4);
    R600_OUT_BATCH(0); //SQ_TEX_RESOURCE5
    R600_OUT_BATCH(sq_tex_resource6);
    R600_OUT_BATCH_RELOC(0,
		     bo,
		     0,
		     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
    R600_OUT_BATCH_RELOC(0,
		     bo,
		     0,
		     RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);
    END_BATCH();
    COMMIT_BATCH();
}

static inline void
set_tex_sampler(context_t * context)
{
    uint32_t sq_tex_sampler_word0 = 0, sq_tex_sampler_word1 = 0, sq_tex_sampler_word2 = 0;
    int i = 0;

    SETbit(sq_tex_sampler_word2, SQ_TEX_SAMPLER_WORD2_0__TYPE_bit);

    BATCH_LOCALS(&context->radeon);

    BEGIN_BATCH_NO_AUTOSTATE(5);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_SAMPLER, 3));
    R600_OUT_BATCH(i * 3);
    R600_OUT_BATCH(sq_tex_sampler_word0);
    R600_OUT_BATCH(sq_tex_sampler_word1);
    R600_OUT_BATCH(sq_tex_sampler_word2);
    END_BATCH();

}

static inline void
set_scissors(context_t *context, int x1, int y1, int x2, int y2)
{
    BATCH_LOCALS(&context->radeon);

    BEGIN_BATCH_NO_AUTOSTATE(17);
    R600_OUT_BATCH_REGSEQ(PA_SC_SCREEN_SCISSOR_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    R600_OUT_BATCH_REGSEQ(PA_SC_WINDOW_OFFSET, 3);
    R600_OUT_BATCH(0); //PA_SC_WINDOW_OFFSET
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit)); //PA_SC_WINDOW_SCISSOR_TL
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    R600_OUT_BATCH_REGSEQ(PA_SC_GENERIC_SCISSOR_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    /* XXX 16 of these PA_SC_VPORT_SCISSOR_0_TL_num ... */
    R600_OUT_BATCH_REGSEQ(PA_SC_VPORT_SCISSOR_0_TL, 2 );
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));
    END_BATCH();

    COMMIT_BATCH();

}

static inline void
set_vb_data(context_t * context, int src_x, int src_y, int dst_x, int dst_y,
            int w, int h, int src_h, unsigned flip_y)
{
    float *vb;
    radeon_bo_map(context->blit_bo, 1);
    vb = context->blit_bo->ptr;

    vb[0] = (float)(dst_x);
    vb[1] = (float)(dst_y);
    vb[2] = (float)(src_x);
    vb[3] = (flip_y) ? (float)(src_h - src_y) : (float)src_y;

    vb[4] = (float)(dst_x);
    vb[5] = (float)(dst_y + h);
    vb[6] = (float)(src_x);
    vb[7] = (flip_y) ? (float)(src_h - (src_y + h)) : (float)(src_y + h);

    vb[8] = (float)(dst_x + w);
    vb[9] = (float)(dst_y + h);
    vb[10] = (float)(src_x + w);
    vb[11] = (flip_y) ? (float)(src_h - (src_y + h)) : (float)(src_y + h);

    radeon_bo_unmap(context->blit_bo);

}

static inline void
draw_auto(context_t *context)
{
    BATCH_LOCALS(&context->radeon);
    uint32_t vgt_primitive_type = 0, vgt_index_type = 0, vgt_draw_initiator = 0, vgt_num_indices;

    SETfield(vgt_primitive_type, DI_PT_RECTLIST,
             VGT_PRIMITIVE_TYPE__PRIM_TYPE_shift,
             VGT_PRIMITIVE_TYPE__PRIM_TYPE_mask);
    SETfield(vgt_index_type, DI_INDEX_SIZE_16_BIT, INDEX_TYPE_shift,
             INDEX_TYPE_mask);
    SETfield(vgt_draw_initiator, DI_MAJOR_MODE_0, MAJOR_MODE_shift,
             MAJOR_MODE_mask);
    SETfield(vgt_draw_initiator, DI_SRC_SEL_AUTO_INDEX, SOURCE_SELECT_shift,
             SOURCE_SELECT_mask);

    vgt_num_indices = 3;

    BEGIN_BATCH_NO_AUTOSTATE(10);
    // prim
    R600_OUT_BATCH_REGSEQ(VGT_PRIMITIVE_TYPE, 1);
    R600_OUT_BATCH(vgt_primitive_type);
    // index type
    R600_OUT_BATCH(CP_PACKET3(R600_IT_INDEX_TYPE, 0));
    R600_OUT_BATCH(vgt_index_type);
    // num instances
    R600_OUT_BATCH(CP_PACKET3(R600_IT_NUM_INSTANCES, 0));
    R600_OUT_BATCH(1);
    //
    R600_OUT_BATCH(CP_PACKET3(R600_IT_DRAW_INDEX_AUTO, 1));
    R600_OUT_BATCH(vgt_num_indices);
    R600_OUT_BATCH(vgt_draw_initiator);

    END_BATCH();
    COMMIT_BATCH();
}

static inline void
set_default_state(context_t *context)
{
    int ps_prio = 0;
    int vs_prio = 1;
    int gs_prio = 2;
    int es_prio = 3;
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
    uint32_t sq_config, sq_gpr_resource_mgmt_1, sq_gpr_resource_mgmt_2;
    uint32_t sq_thread_resource_mgmt, sq_stack_resource_mgmt_1, sq_stack_resource_mgmt_2;
    uint32_t ta_cntl_aux, db_watermarks, sq_dyn_gpr_cntl_ps_flush_req, db_debug;
    BATCH_LOCALS(&context->radeon);

    switch (context->radeon.radeonScreen->chip_family) {
    case CHIP_FAMILY_R600:
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
    case CHIP_FAMILY_RV630:
    case CHIP_FAMILY_RV635:
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
    case CHIP_FAMILY_RV610:
    case CHIP_FAMILY_RV620:
    case CHIP_FAMILY_RS780:
    case CHIP_FAMILY_RS880:
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
    case CHIP_FAMILY_RV670:
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
    case CHIP_FAMILY_RV770:
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
    case CHIP_FAMILY_RV730:
    case CHIP_FAMILY_RV740:
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
    case CHIP_FAMILY_RV710:
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

    sq_config = 0;
    if ((context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV610) ||
        (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV620) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS780) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RS880) ||
        (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_RV710))
	    CLEARbit(sq_config, VC_ENABLE_bit);
    else
	    SETbit(sq_config, VC_ENABLE_bit);
    SETbit(sq_config, DX9_CONSTS_bit);
    SETbit(sq_config, ALU_INST_PREFER_VECTOR_bit);
    SETfield(sq_config, ps_prio, PS_PRIO_shift, PS_PRIO_mask);
    SETfield(sq_config, vs_prio, VS_PRIO_shift, VS_PRIO_mask);
    SETfield(sq_config, gs_prio, GS_PRIO_shift, GS_PRIO_mask);
    SETfield(sq_config, es_prio, ES_PRIO_shift, ES_PRIO_mask);

    sq_gpr_resource_mgmt_1 = 0;
    SETfield(sq_gpr_resource_mgmt_1, num_ps_gprs, NUM_PS_GPRS_shift, NUM_PS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_1, num_vs_gprs, NUM_VS_GPRS_shift, NUM_VS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_1, num_temp_gprs,
	     NUM_CLAUSE_TEMP_GPRS_shift, NUM_CLAUSE_TEMP_GPRS_mask);

    sq_gpr_resource_mgmt_2 = 0;
    SETfield(sq_gpr_resource_mgmt_2, num_gs_gprs, NUM_GS_GPRS_shift, NUM_GS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_2, num_es_gprs, NUM_ES_GPRS_shift, NUM_ES_GPRS_mask);

    sq_thread_resource_mgmt = 0;
    SETfield(sq_thread_resource_mgmt, num_ps_threads,
	     NUM_PS_THREADS_shift, NUM_PS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt, num_vs_threads,
	     NUM_VS_THREADS_shift, NUM_VS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt, num_gs_threads,
	     NUM_GS_THREADS_shift, NUM_GS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt, num_es_threads,
	     NUM_ES_THREADS_shift, NUM_ES_THREADS_mask);

    sq_stack_resource_mgmt_1 = 0;
    SETfield(sq_stack_resource_mgmt_1, num_ps_stack_entries,
	     NUM_PS_STACK_ENTRIES_shift, NUM_PS_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_1, num_vs_stack_entries,
	     NUM_VS_STACK_ENTRIES_shift, NUM_VS_STACK_ENTRIES_mask);

    sq_stack_resource_mgmt_2 = 0;
    SETfield(sq_stack_resource_mgmt_2, num_gs_stack_entries,
	     NUM_GS_STACK_ENTRIES_shift, NUM_GS_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_2, num_es_stack_entries,
	     NUM_ES_STACK_ENTRIES_shift, NUM_ES_STACK_ENTRIES_mask);

    ta_cntl_aux = 0;
    SETfield(ta_cntl_aux, 28, TD_FIFO_CREDIT_shift, TD_FIFO_CREDIT_mask);
    db_watermarks = 0;
    SETfield(db_watermarks, 4, DEPTH_FREE_shift, DEPTH_FREE_mask);
    SETfield(db_watermarks, 16, DEPTH_FLUSH_shift, DEPTH_FLUSH_mask);
    SETfield(db_watermarks, 0, FORCE_SUMMARIZE_shift, FORCE_SUMMARIZE_mask);
    SETfield(db_watermarks, 4, DEPTH_PENDING_FREE_shift, DEPTH_PENDING_FREE_mask);
    sq_dyn_gpr_cntl_ps_flush_req = 0;
    db_debug = 0;
    if (context->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV770) {
	    SETfield(ta_cntl_aux, 3, GRADIENT_CREDIT_shift, GRADIENT_CREDIT_mask);
	    db_debug = 0x82000000;
	    SETfield(db_watermarks, 16, DEPTH_CACHELINE_FREE_shift, DEPTH_CACHELINE_FREE_mask);
    } else {
	    SETfield(ta_cntl_aux, 2, GRADIENT_CREDIT_shift, GRADIENT_CREDIT_mask);
	    SETfield(db_watermarks, 4, DEPTH_CACHELINE_FREE_shift, DEPTH_CACHELINE_FREE_mask);
	    SETbit(sq_dyn_gpr_cntl_ps_flush_req, VS_PC_LIMIT_ENABLE_bit);
    }

    BEGIN_BATCH_NO_AUTOSTATE(117);
    R600_OUT_BATCH_REGSEQ(SQ_CONFIG, 6);
    R600_OUT_BATCH(sq_config);
    R600_OUT_BATCH(sq_gpr_resource_mgmt_1);
    R600_OUT_BATCH(sq_gpr_resource_mgmt_2);
    R600_OUT_BATCH(sq_thread_resource_mgmt);
    R600_OUT_BATCH(sq_stack_resource_mgmt_1);
    R600_OUT_BATCH(sq_stack_resource_mgmt_2);

    R600_OUT_BATCH_REGVAL(TA_CNTL_AUX, ta_cntl_aux);
    R600_OUT_BATCH_REGVAL(VC_ENHANCE, 0);
    R600_OUT_BATCH_REGVAL(R7xx_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, sq_dyn_gpr_cntl_ps_flush_req);
    R600_OUT_BATCH_REGVAL(DB_DEBUG, db_debug);
    R600_OUT_BATCH_REGVAL(DB_WATERMARKS, db_watermarks);

    R600_OUT_BATCH_REGSEQ(SQ_ESGS_RING_ITEMSIZE, 9);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH_REGVAL(CB_CLRCMP_CONTROL,
                         (CLRCMP_SEL_SRC << CLRCMP_FCN_SEL_shift));
    R600_OUT_BATCH_REGVAL(SQ_VTX_BASE_VTX_LOC, 0);
    R600_OUT_BATCH_REGVAL(SQ_VTX_START_INST_LOC, 0);
    R600_OUT_BATCH_REGVAL(DB_DEPTH_INFO, 0);
    R600_OUT_BATCH_REGVAL(DB_DEPTH_CONTROL, 0);
    R600_OUT_BATCH_REGVAL(CB_SHADER_MASK, (OUTPUT0_ENABLE_mask));
    R600_OUT_BATCH_REGVAL(CB_TARGET_MASK, (TARGET0_ENABLE_mask));
    R600_OUT_BATCH_REGVAL(R7xx_CB_SHADER_CONTROL, (RT0_ENABLE_bit));
    R600_OUT_BATCH_REGVAL(CB_COLOR_CONTROL, (0xcc << ROP3_shift));

    R600_OUT_BATCH_REGVAL(PA_CL_VTE_CNTL, VTX_XY_FMT_bit);
    R600_OUT_BATCH_REGVAL(PA_CL_VS_OUT_CNTL, 0);
    R600_OUT_BATCH_REGVAL(PA_CL_CLIP_CNTL, CLIP_DISABLE_bit);
    R600_OUT_BATCH_REGVAL(PA_SU_SC_MODE_CNTL, (FACE_bit) |
        (POLYMODE_PTYPE__TRIANGLES << POLYMODE_FRONT_PTYPE_shift) |
        (POLYMODE_PTYPE__TRIANGLES << POLYMODE_BACK_PTYPE_shift));
    R600_OUT_BATCH_REGVAL(PA_SU_VTX_CNTL, (PIX_CENTER_bit) |
        (X_ROUND_TO_EVEN << PA_SU_VTX_CNTL__ROUND_MODE_shift) |
        (X_1_256TH << QUANT_MODE_shift));

    R600_OUT_BATCH_REGSEQ(VGT_MAX_VTX_INDX, 4);
    R600_OUT_BATCH(2048);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH_REGSEQ(VGT_OUTPUT_PATH_CNTL, 13);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH_REGVAL(VGT_PRIMITIVEID_EN, 0);
    R600_OUT_BATCH_REGVAL(VGT_MULTI_PRIM_IB_RESET_EN, 0);
    R600_OUT_BATCH_REGVAL(VGT_INSTANCE_STEP_RATE_0, 0);
    R600_OUT_BATCH_REGVAL(VGT_INSTANCE_STEP_RATE_1, 0);

    R600_OUT_BATCH_REGSEQ(VGT_STRMOUT_EN, 3);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH_REGVAL(VGT_STRMOUT_BUFFER_EN, 0);

    END_BATCH();
    COMMIT_BATCH();
}

static GLboolean validate_buffers(context_t *rmesa,
                                  struct radeon_bo *src_bo,
                                  struct radeon_bo *dst_bo)
{
    int ret;

    radeon_cs_space_reset_bos(rmesa->radeon.cmdbuf.cs);

    ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs,
					src_bo, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    if (ret)
        return GL_FALSE;

    ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs,
                                        dst_bo, 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT);
    if (ret)
        return GL_FALSE;

    ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs,
					rmesa->blit_bo,
					RADEON_GEM_DOMAIN_GTT, 0);
    if (ret)
        return GL_FALSE;

    return GL_TRUE;
}

unsigned r600_blit(GLcontext *ctx,
                   struct radeon_bo *src_bo,
                   intptr_t src_offset,
                   gl_format src_mesaformat,
                   unsigned src_pitch,
                   unsigned src_width,
                   unsigned src_height,
                   unsigned src_x,
                   unsigned src_y,
                   struct radeon_bo *dst_bo,
                   intptr_t dst_offset,
                   gl_format dst_mesaformat,
                   unsigned dst_pitch,
                   unsigned dst_width,
                   unsigned dst_height,
                   unsigned dst_x,
                   unsigned dst_y,
                   unsigned w,
                   unsigned h,
                   unsigned flip_y)
{
    context_t *context = R700_CONTEXT(ctx);
    int id = 0;

    if (!r600_check_blit(dst_mesaformat))
        return GL_FALSE;

    if (src_bo == dst_bo) {
        return GL_FALSE;
    }

    if (src_offset % 256 || dst_offset % 256) {
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

    /* Flush is needed to make sure that source buffer has correct data */
    radeonFlush(ctx);

    rcommonEnsureCmdBufSpace(&context->radeon, 304, __FUNCTION__);

    /* load shaders */
    load_shaders(context->radeon.glCtx);

    if (!validate_buffers(context, src_bo, dst_bo))
        return GL_FALSE;

    /* set clear state */
    /* 117 */
    set_default_state(context);

    /* shaders */
    /* 72 */
    set_shaders(context);

    /* src */
    /* 20 */
    set_tex_resource(context, src_mesaformat, src_bo,
		     src_width, src_height, src_pitch, src_offset);

    /* 5 */
    set_tex_sampler(context);

    /* dst */
    /* 27 */
    set_render_target(context, dst_bo, dst_mesaformat,
		      dst_pitch, dst_width, dst_height, dst_offset);
    /* scissors */
    /* 17 */
    set_scissors(context, dst_x, dst_y, dst_x + dst_width, dst_y + dst_height);

    set_vb_data(context, src_x, src_y, dst_x, dst_y, w, h, src_height, flip_y);
    /* Vertex buffer setup */
    /* 24 */
    set_vtx_resource(context);

    /* draw */
    /* 10 */
    draw_auto(context);

    /* 7 */
    r700SyncSurf(context, dst_bo, 0,
                 RADEON_GEM_DOMAIN_VRAM|RADEON_GEM_DOMAIN_GTT,
		 CB_ACTION_ENA_bit | (1 << (id + 6)));

    /* 5 */
    /* XXX drm should handle this in fence submit */
    r700WaitForIdleClean(context);

    radeonFlush(ctx);

    return GL_TRUE;
}
