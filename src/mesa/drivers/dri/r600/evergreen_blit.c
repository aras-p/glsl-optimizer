/*
 * Copyright (C) 2010 Advanced Micro Devices, Inc.
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

#include "evergreen_off.h"
#include "evergreen_diff.h"

#include "evergreen_blit.h"
#include "evergreen_blit_shaders.h"
#include "r600_cmdbuf.h"

/* common formats supported as both textures and render targets */
unsigned evergreen_check_blit(gl_format mesa_format)
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
    case MESA_FORMAT_SARGB8:
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
eg_set_render_target(context_t *context, struct radeon_bo *bo, gl_format mesa_format,
		     int nPitchInPixel, int w, int h, intptr_t dst_offset)
{
    uint32_t cb_color0_base, cb_color0_info = 0;
    uint32_t cb_color0_pitch = 0, cb_color0_slice = 0, cb_color0_attrib = 0;
    int id = 0;
    uint32_t endian, comp_swap, format, source_format, number_type;
    BATCH_LOCALS(&context->radeon);

    cb_color0_base = dst_offset / 256;
    endian = ENDIAN_NONE;

    /* pitch */
    SETfield(cb_color0_pitch, (nPitchInPixel / 8) - 1,
             EG_CB_COLOR0_PITCH__TILE_MAX_shift,
             EG_CB_COLOR0_PITCH__TILE_MAX_mask);

    /* slice */
    SETfield(cb_color0_slice,
	     ((nPitchInPixel * h) / 64) - 1,
             EG_CB_COLOR0_SLICE__TILE_MAX_shift,
             EG_CB_COLOR0_SLICE__TILE_MAX_mask);

    /* CB_COLOR0_ATTRIB */ /* TODO : for z clear, this should be set to 0 */
    SETbit(cb_color0_attrib,
           EG_CB_COLOR0_ATTRIB__NON_DISP_TILING_ORDER_bit);

    SETfield(cb_color0_info,
             ARRAY_LINEAR_GENERAL,
             EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
             EG_CB_COLOR0_INFO__ARRAY_MODE_mask);

    SETbit(cb_color0_info, EG_CB_COLOR0_INFO__BLEND_BYPASS_bit);

    switch(mesa_format) {
    case MESA_FORMAT_RGBA8888:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_SIGNED_RGBA8888:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_SNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGBA8888_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_SIGNED_RGBA8888_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_SNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB8888:
    case MESA_FORMAT_XRGB8888:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB8888_REV:
    case MESA_FORMAT_XRGB8888_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGB565:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_5_6_5;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGB565_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_5_6_5;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB4444:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_4_4_4_4;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB4444_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_4_4_4_4;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB1555:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_1_5_5_5;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_ARGB1555_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_1_5_5_5;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_AL88:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_8_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_AL88_REV:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_8_8;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGB332:
            format = COLOR_3_3_2;
            comp_swap = SWAP_STD_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_A8:
            format = COLOR_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_I8:
    case MESA_FORMAT_CI8:
            format = COLOR_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_L8:
            format = COLOR_8;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_UNORM;
	    source_format = 1;
            break;
    case MESA_FORMAT_RGBA_FLOAT32:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_32_32_32_32_FLOAT;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_RGBA_FLOAT16:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_16_16_16_16_FLOAT;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_ALPHA_FLOAT32:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_ALPHA_FLOAT16:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_FLOAT32:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_FLOAT16:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_32_32_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_LUMINANCE_ALPHA_FLOAT16:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_16_16_FLOAT;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_INTENSITY_FLOAT32: /* X, X, X, X */
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_32_FLOAT;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_FLOAT;
	    source_format = 0;
            break;
    case MESA_FORMAT_INTENSITY_FLOAT16: /* X, X, X, X */
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_16_FLOAT;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    source_format = 0;
            break;
    case MESA_FORMAT_X8_Z24:
    case MESA_FORMAT_S8_Z24:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_24;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(cb_color0_info,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_Z24_S8:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_24_8;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(cb_color0_info,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_Z16:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_16;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(cb_color0_info,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_Z32:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_32;
            comp_swap = SWAP_STD;
	    number_type = NUMBER_UNORM;
	    SETfield(cb_color0_info,
		     ARRAY_1D_TILED_THIN1,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_shift,
		     EG_CB_COLOR0_INFO__ARRAY_MODE_mask);
	    source_format = 0;
            break;
    case MESA_FORMAT_SARGB8:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN32;
#endif
            format = COLOR_8_8_8_8;
            comp_swap = SWAP_ALT;
	    number_type = NUMBER_SRGB;
	    source_format = 1;
            break;
    case MESA_FORMAT_SLA8:
#ifdef MESA_BIG_ENDIAN
	    endian = ENDIAN_8IN16;
#endif
            format = COLOR_8_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_SRGB;
	    source_format = 1;
            break;
    case MESA_FORMAT_SL8:
            format = COLOR_8;
            comp_swap = SWAP_ALT_REV;
	    number_type = NUMBER_SRGB;
	    source_format = 1;
            break;
    default:
            fprintf(stderr,"Invalid format for copy %s\n",_mesa_get_format_name(mesa_format));
            assert("Invalid format for US output\n");
            return;
    }

    SETfield(cb_color0_info,
	     endian,
	     EG_CB_COLOR0_INFO__ENDIAN_shift,
	     EG_CB_COLOR0_INFO__ENDIAN_mask);
    SETfield(cb_color0_info,
	     format,
	     EG_CB_COLOR0_INFO__FORMAT_shift,
	     EG_CB_COLOR0_INFO__FORMAT_mask);
    SETfield(cb_color0_info,
	     comp_swap,
	     EG_CB_COLOR0_INFO__COMP_SWAP_shift,
	     EG_CB_COLOR0_INFO__COMP_SWAP_mask);
    SETfield(cb_color0_info,
             number_type,
             EG_CB_COLOR0_INFO__NUMBER_TYPE_shift,
             EG_CB_COLOR0_INFO__NUMBER_TYPE_mask);
    SETfield(cb_color0_info,
             source_format,
             EG_CB_COLOR0_INFO__SOURCE_FORMAT_shift,
             EG_CB_COLOR0_INFO__SOURCE_FORMAT_mask);

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_BASE + (4 * id), 1);
    R600_OUT_BATCH(cb_color0_base);
    R600_OUT_BATCH_RELOC(cb_color0_base,
			 bo,
			 cb_color0_base,
			 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGVAL(EG_CB_COLOR0_INFO, cb_color0_info);
    R600_OUT_BATCH_RELOC(cb_color0_info,
			 bo,
			 cb_color0_info,
			 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(5);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_PITCH, 3);
    R600_OUT_BATCH(cb_color0_pitch);
    R600_OUT_BATCH(cb_color0_slice);
    R600_OUT_BATCH(0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR0_ATTRIB, 2);
    R600_OUT_BATCH(cb_color0_attrib);
    R600_OUT_BATCH(0);
    /*
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_CMASK.u32All);
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_CMASK_SLICE.u32All);
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_FMASK.u32All);
    R600_OUT_BATCH(evergreen->render_target[id].CB_COLOR0_FMASK_SLICE.u32All);
    */
    END_BATCH();

    COMMIT_BATCH();

}

static inline void eg_load_shaders(struct gl_context * ctx)
{

    radeonContextPtr radeonctx = RADEON_CONTEXT(ctx);
    context_t *context = EVERGREEN_CONTEXT(ctx);
    int i, size;
    uint32_t *shader;

    if (context->blit_bo_loaded == 1)
        return;

    size = 4096;
    context->blit_bo = radeon_bo_open(radeonctx->radeonScreen->bom, 0,
                                      size, 256, RADEON_GEM_DOMAIN_GTT, 0);
    radeon_bo_map(context->blit_bo, 1);
    shader = context->blit_bo->ptr;

    for(i=0; i<sizeof(evergreen_vs)/4; i++) {
	    shader[128+i] = CPU_TO_LE32(evergreen_vs[i]);
    }
    for(i=0; i<sizeof(evergreen_ps)/4; i++) {
	    shader[256+i] = CPU_TO_LE32(evergreen_ps[i]);
    }

    radeon_bo_unmap(context->blit_bo);
    context->blit_bo_loaded = 1;

}

static inline void
eg_set_shaders(context_t *context)
{
    struct radeon_bo * pbo = context->blit_bo;
    uint32_t sq_pgm_start_fs = (512 >> 8);
    uint32_t sq_pgm_resources_fs = 0;

    uint32_t sq_pgm_start_vs = (512 >> 8);
    uint32_t sq_pgm_resources_vs = (2 << NUM_GPRS_shift);

    uint32_t sq_pgm_start_ps = (1024 >> 8);
    uint32_t sq_pgm_resources_ps = (1 << NUM_GPRS_shift);
    uint32_t sq_pgm_exports_ps = (1 << 1);
    BATCH_LOCALS(&context->radeon);

    r700SyncSurf(context, pbo, RADEON_GEM_DOMAIN_GTT, 0, SH_ACTION_ENA_bit);

    /* FS */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_START_FS, 1);
    R600_OUT_BATCH(sq_pgm_start_fs);
    R600_OUT_BATCH_RELOC(sq_pgm_start_fs,
			 pbo,
			 sq_pgm_start_fs,
			 RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(3);
    EVERGREEN_OUT_BATCH_REGVAL(EG_SQ_PGM_RESOURCES_FS, sq_pgm_resources_fs);
    END_BATCH();

    /* VS */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_START_VS, 1);
    R600_OUT_BATCH(sq_pgm_start_vs);
    R600_OUT_BATCH_RELOC(sq_pgm_start_vs,
		         pbo,
		         sq_pgm_start_vs,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(4);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_VS, 2);
    R600_OUT_BATCH(sq_pgm_resources_vs);
    R600_OUT_BATCH(0);
    END_BATCH();

    /* PS */
    BEGIN_BATCH_NO_AUTOSTATE(3 + 2);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_START_PS, 1);
    R600_OUT_BATCH(sq_pgm_start_ps);
    R600_OUT_BATCH_RELOC(sq_pgm_start_ps,
		         pbo,
		         sq_pgm_start_ps,
		         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();

    BEGIN_BATCH_NO_AUTOSTATE(5);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_PGM_RESOURCES_PS, 3);
    R600_OUT_BATCH(sq_pgm_resources_ps);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(sq_pgm_exports_ps);
    END_BATCH();

    COMMIT_BATCH();

}

static inline void
eg_set_vtx_resource(context_t *context)
{
    struct radeon_bo *bo = context->blit_bo;
    uint32_t sq_vtx_constant_word3 = 0;
    uint32_t sq_vtx_constant_word2 = 0;
    BATCH_LOCALS(&context->radeon);

    BEGIN_BATCH_NO_AUTOSTATE(6);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_BASE_VTX_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_CTL_CONST, 1));
    R600_OUT_BATCH(mmSQ_VTX_START_INST_LOC - ASIC_CTL_CONST_BASE_INDEX);
    R600_OUT_BATCH(0);
    END_BATCH();

    if (context->radeon.radeonScreen->chip_family == CHIP_FAMILY_CEDAR)
	    r700SyncSurf(context, bo, RADEON_GEM_DOMAIN_GTT, 0, TC_ACTION_ENA_bit);
    else
	    r700SyncSurf(context, bo, RADEON_GEM_DOMAIN_GTT, 0, VC_ACTION_ENA_bit);

    SETfield(sq_vtx_constant_word3, SQ_SEL_X,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_shift,
        EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_X_mask);
    SETfield(sq_vtx_constant_word3, SQ_SEL_Y,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_shift,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Y_mask);
    SETfield(sq_vtx_constant_word3, SQ_SEL_Z,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_shift,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_Z_mask);
    SETfield(sq_vtx_constant_word3, SQ_SEL_W,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_shift,
	     EG_SQ_VTX_CONSTANT_WORD3_0__DST_SEL_W_mask);

    sq_vtx_constant_word2 = 0
#ifdef MESA_BIG_ENDIAN
	    | (SQ_ENDIAN_8IN32 << SQ_VTX_CONSTANT_WORD2_0__ENDIAN_SWAP_shift)
#endif
	    | (16 << SQ_VTX_CONSTANT_WORD2_0__STRIDE_shift);

    BEGIN_BATCH_NO_AUTOSTATE(10 + 2);

    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 8));
    R600_OUT_BATCH(EG_SQ_FETCH_RESOURCE_VS_OFFSET * EG_FETCH_RESOURCE_STRIDE);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(48 - 1);
    R600_OUT_BATCH(sq_vtx_constant_word2);
    R600_OUT_BATCH(sq_vtx_constant_word3);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(SQ_TEX_VTX_VALID_BUFFER << SQ_TEX_RESOURCE_WORD6_0__TYPE_shift);
    R600_OUT_BATCH_RELOC(0,
                         bo,
                         0,
                         RADEON_GEM_DOMAIN_GTT, 0, 0);
    END_BATCH();
    COMMIT_BATCH();

}

static inline void
eg_set_tex_resource(context_t * context,
		    gl_format mesa_format, struct radeon_bo *bo, int w, int h,
		    int TexelPitch, intptr_t src_offset)
{
    uint32_t sq_tex_resource0, sq_tex_resource1, sq_tex_resource2, sq_tex_resource4, sq_tex_resource7;

    sq_tex_resource0 = sq_tex_resource1 = sq_tex_resource2 = sq_tex_resource4 = sq_tex_resource7 = 0;
    BATCH_LOCALS(&context->radeon);

    SETfield(sq_tex_resource0, SQ_TEX_DIM_2D, DIM_shift, DIM_mask);
    SETfield(sq_tex_resource0, ARRAY_LINEAR_GENERAL,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);

    switch (mesa_format) {
    case MESA_FORMAT_RGBA8888:
    case MESA_FORMAT_SIGNED_RGBA8888:
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_5_6_5,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_5_6_5,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_4_4_4_4,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_4_4_4_4,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_1_5_5_5,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_1_5_5_5,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_3_3_2,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_32_32_32_32_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_16_16_16_16_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_32_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_16_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_32_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_16_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_32_32_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_16_16_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_32_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_16_FLOAT,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    /* ??? */
	    CLEARbit(sq_tex_resource0, EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit);
	    SETfield(sq_tex_resource1, ARRAY_1D_TILED_THIN1,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask);
	    SETfield(sq_tex_resource7, FMT_16,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    /* ??? */
	    CLEARbit(sq_tex_resource0, EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit);
	    SETfield(sq_tex_resource1, ARRAY_1D_TILED_THIN1,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask);
	    SETfield(sq_tex_resource7, FMT_8_24,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    /* ??? */
	    CLEARbit(sq_tex_resource0, EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit);
	    SETfield(sq_tex_resource1, ARRAY_1D_TILED_THIN1,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask);
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource7, FMT_8_24,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    /* ??? */
	    CLEARbit(sq_tex_resource0, EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit);
	    SETfield(sq_tex_resource1, ARRAY_1D_TILED_THIN1,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask);
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource7, FMT_24_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    /* ??? */
	    CLEARbit(sq_tex_resource0, EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit);
	    SETfield(sq_tex_resource1, ARRAY_1D_TILED_THIN1,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask);
	    SETbit(sq_tex_resource0, TILE_TYPE_bit);
	    SETfield(sq_tex_resource0, ARRAY_1D_TILED_THIN1,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
		     SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);
	    SETfield(sq_tex_resource7, FMT_32,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    /* ??? */
	    CLEARbit(sq_tex_resource0, EG_SQ_TEX_RESOURCE_WORD0_0__NDTO_bit);
	    SETfield(sq_tex_resource1, ARRAY_1D_TILED_THIN1,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_shift,
		     EG_SQ_TEX_RESOURCE_WORD1_0__ARRAY_MODE_mask);
	    SETfield(sq_tex_resource7, FMT_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    break;
    case MESA_FORMAT_SARGB8:
	    SETfield(sq_tex_resource7, FMT_8_8_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Z,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_Y,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_X,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
	    SETfield(sq_tex_resource4, SQ_SEL_W,
		     SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift, SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
	    SETbit(sq_tex_resource4, SQ_TEX_RESOURCE_WORD4_0__FORCE_DEGAMMA_bit);
	    break;
    case MESA_FORMAT_SLA8:
	    SETfield(sq_tex_resource7, FMT_8_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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
	    SETfield(sq_tex_resource7, FMT_8,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_shift,
		     EG_SQ_TEX_RESOURCE_WORD7_0__DATA_FORMAT_mask);
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

    SETfield(sq_tex_resource0, (TexelPitch/8)-1,
             EG_SQ_TEX_RESOURCE_WORD0_0__PITCH_shift,
             EG_SQ_TEX_RESOURCE_WORD0_0__PITCH_mask);
    SETfield(sq_tex_resource0, w - 1,
	     EG_SQ_TEX_RESOURCE_WORD0_0__TEX_WIDTH_shift,
	     EG_SQ_TEX_RESOURCE_WORD0_0__TEX_WIDTH_mask);
    SETfield(sq_tex_resource1, h - 1,
	     EG_SQ_TEX_RESOURCE_WORD1_0__TEX_HEIGHT_shift,
	     EG_SQ_TEX_RESOURCE_WORD1_0__TEX_HEIGHT_mask);

    sq_tex_resource2 = src_offset / 256;

    SETfield(sq_tex_resource7, SQ_TEX_VTX_VALID_TEXTURE,
             SQ_TEX_RESOURCE_WORD6_0__TYPE_shift,
             SQ_TEX_RESOURCE_WORD6_0__TYPE_mask);

    r700SyncSurf(context, bo,
                 RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM,
		 0, TC_ACTION_ENA_bit);

    BEGIN_BATCH_NO_AUTOSTATE(10 + 4);
    R600_OUT_BATCH(CP_PACKET3(R600_IT_SET_RESOURCE, 8));
    R600_OUT_BATCH(0 * 7);
    R600_OUT_BATCH(sq_tex_resource0);
    R600_OUT_BATCH(sq_tex_resource1);
    R600_OUT_BATCH(sq_tex_resource2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(sq_tex_resource4);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(sq_tex_resource7);
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
eg_set_tex_sampler(context_t * context)
{
    uint32_t sq_tex_sampler_word0 = 0, sq_tex_sampler_word1 = 0, sq_tex_sampler_word2 = 0;
    int i = 0;

    SETbit(sq_tex_sampler_word2, EG_SQ_TEX_SAMPLER_WORD2_0__TYPE_bit);

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
eg_set_scissors(context_t *context, int x1, int y1, int x2, int y2)
{
    BATCH_LOCALS(&context->radeon);

    BEGIN_BATCH_NO_AUTOSTATE(17);
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_SCREEN_SCISSOR_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_WINDOW_OFFSET, 3);
    R600_OUT_BATCH(0); //PA_SC_WINDOW_OFFSET
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit)); //PA_SC_WINDOW_SCISSOR_TL
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_GENERIC_SCISSOR_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    /* XXX 16 of these PA_SC_VPORT_SCISSOR_0_TL_num ... */
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_VPORT_SCISSOR_0_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));
    END_BATCH();

    COMMIT_BATCH();

}

static inline void
eg_set_vb_data(context_t * context, int src_x, int src_y, int dst_x, int dst_y,
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
eg_draw_auto(context_t *context)
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
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_PRIMITIVE_TYPE, 1);
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
eg_set_default_state(context_t *context)
{
    int num_ps_gprs;
    int num_vs_gprs;
    int num_gs_gprs;
    int num_es_gprs;
    int num_hs_gprs;
    int num_ls_gprs;
    int num_temp_gprs;
    int num_ps_threads;
    int num_vs_threads;
    int num_gs_threads;
    int num_es_threads;
    int num_hs_threads;
    int num_ls_threads;
    int num_ps_stack_entries;
    int num_vs_stack_entries;
    int num_gs_stack_entries;
    int num_es_stack_entries;
    int num_hs_stack_entries;
    int num_ls_stack_entries;
    uint32_t sq_config = 0, sq_gpr_resource_mgmt_1 = 0, sq_gpr_resource_mgmt_2 = 0;
    uint32_t sq_gpr_resource_mgmt_3 = 0;
    uint32_t sq_thread_resource_mgmt = 0, sq_thread_resource_mgmt_2 = 0;
    uint32_t sq_stack_resource_mgmt_1 = 0, sq_stack_resource_mgmt_2 = 0, sq_stack_resource_mgmt_3 = 0;
    BATCH_LOCALS(&context->radeon);

    switch (context->radeon.radeonScreen->chip_family) {
    case CHIP_FAMILY_CEDAR:
    default:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 96;
	    num_vs_threads = 16;
	    num_gs_threads = 16;
	    num_es_threads = 16;
	    num_hs_threads = 16;
	    num_ls_threads = 16;
	    num_ps_stack_entries = 42;
	    num_vs_stack_entries = 42;
	    num_gs_stack_entries = 42;
	    num_es_stack_entries = 42;
	    num_hs_stack_entries = 42;
	    num_ls_stack_entries = 42;
	    break;
    case CHIP_FAMILY_REDWOOD:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 128;
	    num_vs_threads = 20;
	    num_gs_threads = 20;
	    num_es_threads = 20;
	    num_hs_threads = 20;
	    num_ls_threads = 20;
	    num_ps_stack_entries = 42;
	    num_vs_stack_entries = 42;
	    num_gs_stack_entries = 42;
	    num_es_stack_entries = 42;
	    num_hs_stack_entries = 42;
	    num_ls_stack_entries = 42;
	    break;
    case CHIP_FAMILY_JUNIPER:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 128;
	    num_vs_threads = 20;
	    num_gs_threads = 20;
	    num_es_threads = 20;
	    num_hs_threads = 20;
	    num_ls_threads = 20;
	    num_ps_stack_entries = 85;
	    num_vs_stack_entries = 85;
	    num_gs_stack_entries = 85;
	    num_es_stack_entries = 85;
	    num_hs_stack_entries = 85;
	    num_ls_stack_entries = 85;
	    break;
    case CHIP_FAMILY_CYPRESS:
    case CHIP_FAMILY_HEMLOCK:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 128;
	    num_vs_threads = 20;
	    num_gs_threads = 20;
	    num_es_threads = 20;
	    num_hs_threads = 20;
	    num_ls_threads = 20;
	    num_ps_stack_entries = 85;
	    num_vs_stack_entries = 85;
	    num_gs_stack_entries = 85;
	    num_es_stack_entries = 85;
	    num_hs_stack_entries = 85;
	    num_ls_stack_entries = 85;
	    break;
    case CHIP_FAMILY_PALM:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 96;
	    num_vs_threads = 16;
	    num_gs_threads = 16;
	    num_es_threads = 16;
	    num_hs_threads = 16;
	    num_ls_threads = 16;
	    num_ps_stack_entries = 42;
	    num_vs_stack_entries = 42;
	    num_gs_stack_entries = 42;
	    num_es_stack_entries = 42;
	    num_hs_stack_entries = 42;
	    num_ls_stack_entries = 42;
	    break;
    case CHIP_FAMILY_SUMO:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 96;
	    num_vs_threads = 25;
	    num_gs_threads = 25;
	    num_es_threads = 25;
	    num_hs_threads = 25;
	    num_ls_threads = 25;
	    num_ps_stack_entries = 42;
	    num_vs_stack_entries = 42;
	    num_gs_stack_entries = 42;
	    num_es_stack_entries = 42;
	    num_hs_stack_entries = 42;
	    num_ls_stack_entries = 42;
	    break;
    case CHIP_FAMILY_SUMO2:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 96;
	    num_vs_threads = 25;
	    num_gs_threads = 25;
	    num_es_threads = 25;
	    num_hs_threads = 25;
	    num_ls_threads = 25;
	    num_ps_stack_entries = 85;
	    num_vs_stack_entries = 85;
	    num_gs_stack_entries = 85;
	    num_es_stack_entries = 85;
	    num_hs_stack_entries = 85;
	    num_ls_stack_entries = 85;
	    break;
    case CHIP_FAMILY_BARTS:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 128;
	    num_vs_threads = 20;
	    num_gs_threads = 20;
	    num_es_threads = 20;
	    num_hs_threads = 20;
	    num_ls_threads = 20;
	    num_ps_stack_entries = 85;
	    num_vs_stack_entries = 85;
	    num_gs_stack_entries = 85;
	    num_es_stack_entries = 85;
	    num_hs_stack_entries = 85;
	    num_ls_stack_entries = 85;
	    break;
    case CHIP_FAMILY_TURKS:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 128;
	    num_vs_threads = 20;
	    num_gs_threads = 20;
	    num_es_threads = 20;
	    num_hs_threads = 20;
	    num_ls_threads = 20;
	    num_ps_stack_entries = 42;
	    num_vs_stack_entries = 42;
	    num_gs_stack_entries = 42;
	    num_es_stack_entries = 42;
	    num_hs_stack_entries = 42;
	    num_ls_stack_entries = 42;
	    break;
    case CHIP_FAMILY_CAICOS:
	    num_ps_gprs = 93;
	    num_vs_gprs = 46;
	    num_temp_gprs = 4;
	    num_gs_gprs = 31;
	    num_es_gprs = 31;
	    num_hs_gprs = 23;
	    num_ls_gprs = 23;
	    num_ps_threads = 128;
	    num_vs_threads = 10;
	    num_gs_threads = 10;
	    num_es_threads = 10;
	    num_hs_threads = 10;
	    num_ls_threads = 10;
	    num_ps_stack_entries = 42;
	    num_vs_stack_entries = 42;
	    num_gs_stack_entries = 42;
	    num_es_stack_entries = 42;
	    num_hs_stack_entries = 42;
	    num_ls_stack_entries = 42;
	    break;
    }

    if ((context->radeon.radeonScreen->chip_family == CHIP_FAMILY_CEDAR) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_PALM) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_SUMO) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_SUMO2) ||
	(context->radeon.radeonScreen->chip_family == CHIP_FAMILY_CAICOS))
	    CLEARbit(sq_config, EG_SQ_CONFIG__VC_ENABLE_bit);
    else
	    SETbit(sq_config, EG_SQ_CONFIG__VC_ENABLE_bit);
    SETbit(sq_config, EG_SQ_CONFIG__EXPORT_SRC_C_bit);

    SETfield(sq_config, 0,
             EG_SQ_CONFIG__PS_PRIO_shift,
             EG_SQ_CONFIG__PS_PRIO_mask);
    SETfield(sq_config, 1,
             EG_SQ_CONFIG__VS_PRIO_shift,
             EG_SQ_CONFIG__VS_PRIO_mask);
    SETfield(sq_config, 2,
             EG_SQ_CONFIG__GS_PRIO_shift,
             EG_SQ_CONFIG__GS_PRIO_mask);
    SETfield(sq_config, 3,
             EG_SQ_CONFIG__ES_PRIO_shift,
             EG_SQ_CONFIG__ES_PRIO_mask);


    SETfield(sq_gpr_resource_mgmt_1, num_ps_gprs,
             NUM_PS_GPRS_shift, NUM_PS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_1, num_vs_gprs,
             NUM_VS_GPRS_shift, NUM_VS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_1, num_temp_gprs,
	         NUM_CLAUSE_TEMP_GPRS_shift, NUM_CLAUSE_TEMP_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_2, num_gs_gprs,
             NUM_GS_GPRS_shift, NUM_GS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_2, num_es_gprs,
             NUM_ES_GPRS_shift, NUM_ES_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_3, num_hs_gprs,
             NUM_PS_GPRS_shift, NUM_PS_GPRS_mask);
    SETfield(sq_gpr_resource_mgmt_3, num_ls_gprs,
             NUM_VS_GPRS_shift, NUM_VS_GPRS_mask);

    SETfield(sq_thread_resource_mgmt, num_ps_threads,
	     NUM_PS_THREADS_shift, NUM_PS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt, num_vs_threads,
	     NUM_VS_THREADS_shift, NUM_VS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt, num_gs_threads,
	     NUM_GS_THREADS_shift, NUM_GS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt, num_es_threads,
	     NUM_ES_THREADS_shift, NUM_ES_THREADS_mask);
    SETfield(sq_thread_resource_mgmt_2, num_hs_threads,
	     NUM_PS_THREADS_shift, NUM_PS_THREADS_mask);
    SETfield(sq_thread_resource_mgmt_2, num_ls_threads,
	     NUM_VS_THREADS_shift, NUM_VS_THREADS_mask);

    SETfield(sq_stack_resource_mgmt_1, num_ps_stack_entries,
	     NUM_PS_STACK_ENTRIES_shift, NUM_PS_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_1, num_vs_stack_entries,
	     NUM_VS_STACK_ENTRIES_shift, NUM_VS_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_2, num_gs_stack_entries,
	     NUM_GS_STACK_ENTRIES_shift, NUM_GS_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_2, num_es_stack_entries,
	     NUM_ES_STACK_ENTRIES_shift, NUM_ES_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_3, num_hs_stack_entries,
	     NUM_PS_STACK_ENTRIES_shift, NUM_PS_STACK_ENTRIES_mask);
    SETfield(sq_stack_resource_mgmt_3, num_ls_stack_entries,
	     NUM_VS_STACK_ENTRIES_shift, NUM_VS_STACK_ENTRIES_mask);


    BEGIN_BATCH_NO_AUTOSTATE(196);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, 0);
    //6
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_CONFIG, 4);
    R600_OUT_BATCH(sq_config);
    R600_OUT_BATCH(sq_gpr_resource_mgmt_1);
    R600_OUT_BATCH(sq_gpr_resource_mgmt_2);
    R600_OUT_BATCH(sq_gpr_resource_mgmt_3);
    //7
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_THREAD_RESOURCE_MGMT, 5);
    R600_OUT_BATCH(sq_thread_resource_mgmt);
    R600_OUT_BATCH(sq_thread_resource_mgmt_2);
    R600_OUT_BATCH(sq_stack_resource_mgmt_1);
    R600_OUT_BATCH(sq_stack_resource_mgmt_2);
    R600_OUT_BATCH(sq_stack_resource_mgmt_3);
    //3
    R600_OUT_BATCH(CP_PACKET3(R600_IT_CONTEXT_CONTROL, 1));
    R600_OUT_BATCH(0x80000000);
    R600_OUT_BATCH(0x80000000);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_SQ_LDS_ALLOC_PS, 0);
    //8
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_ESGS_RING_ITEMSIZE, 6);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //6
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SQ_GS_VERT_ITEMSIZE, 4);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_DB_DEPTH_CONTROL, 0);
    //7
    EVERGREEN_OUT_BATCH_REGSEQ(EG_DB_RENDER_CONTROL, 5);
    R600_OUT_BATCH(0x00000060);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0x0000002a);
    R600_OUT_BATCH(0);
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_DB_STENCIL_CLEAR, 2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_DB_ALPHA_TO_MASK, 0x0000aa00);
    //15
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_CLIPRECT_RULE, 13);
    R600_OUT_BATCH(0x0000ffff);
    R600_OUT_BATCH(0x00000000);
    R600_OUT_BATCH(0x20002000);
    R600_OUT_BATCH(0x00000000);
    R600_OUT_BATCH(0x20002000);
    R600_OUT_BATCH(0x00000000);
    R600_OUT_BATCH(0x20002000);
    R600_OUT_BATCH(0x00000000);
    R600_OUT_BATCH(0x20002000);
    R600_OUT_BATCH(0xaaaaaaaa);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0x0000000f);
    R600_OUT_BATCH(0x0000000f);
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_VPORT_ZMIN_0, 2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0x3f800000);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_SX_MISC, 0);
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_MODE_CNTL_0, 2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //18
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SC_LINE_CNTL, 16);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0x00000005);
    R600_OUT_BATCH(0x3f800000);
    R600_OUT_BATCH(0x3f800000);
    R600_OUT_BATCH(0x3f800000);
    R600_OUT_BATCH(0x3f800000);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0xffffffff);
    //15
    EVERGREEN_OUT_BATCH_REGSEQ(EG_CB_COLOR_CONTROL, 13);
    R600_OUT_BATCH(0x00cc0010);
    R600_OUT_BATCH(0x00000210);
    R600_OUT_BATCH(0x00010000);
    R600_OUT_BATCH(0x00000004);
    R600_OUT_BATCH(0x00000100);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //8
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SU_POLY_OFFSET_DB_FMT_CNTL, 6);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //11
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_MAX_VTX_INDX, 9);
    R600_OUT_BATCH(0x00ffffff);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_INSTANCE_STEP_RATE_0, 2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_REUSE_OFF, 2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //19
    EVERGREEN_OUT_BATCH_REGSEQ(EG_PA_SU_POINT_SIZE, 17);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0x00000008);
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
    R600_OUT_BATCH(0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_VGT_PRIMITIVEID_EN, 0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_VGT_MULTI_PRIM_IB_RESET_EN, 0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_VGT_SHADER_STAGES_EN, 0);
    //4
    EVERGREEN_OUT_BATCH_REGSEQ(EG_VGT_STRMOUT_CONFIG, 2);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_CB_BLEND0_CONTROL, 0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_SPI_VS_OUT_CONFIG, 0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_SPI_VS_OUT_ID_0, 0);
    //3
    EVERGREEN_OUT_BATCH_REGVAL(EG_SPI_PS_INPUT_CNTL_0, 0);
    //13
    EVERGREEN_OUT_BATCH_REGSEQ(EG_SPI_PS_IN_CONTROL_0, 11);
    R600_OUT_BATCH(0x20000001);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0x00100000);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);
    R600_OUT_BATCH(0);

    END_BATCH();
    COMMIT_BATCH();
}

static GLboolean eg_validate_buffers(context_t *rmesa,
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

unsigned evergreen_blit(struct gl_context *ctx,
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
    context_t *context = EVERGREEN_CONTEXT(ctx);
    int id = 0;

    if (!evergreen_check_blit(dst_mesaformat))
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

    rcommonEnsureCmdBufSpace(&context->radeon, 327, __FUNCTION__);

    /* load shaders */
    eg_load_shaders(context->radeon.glCtx);

    if (!eg_validate_buffers(context, src_bo, dst_bo))
        return GL_FALSE;

    /* set clear state */
    /* 196 */
    eg_set_default_state(context);

    /* shaders */
    /* 34 */
    eg_set_shaders(context);

    /* src */
    /* 21 */
    eg_set_tex_resource(context, src_mesaformat, src_bo,
			src_width, src_height, src_pitch, src_offset);

    /* 5 */
    eg_set_tex_sampler(context);

    /* dst */
    /* 19 */
    eg_set_render_target(context, dst_bo, dst_mesaformat,
			 dst_pitch, dst_width, dst_height, dst_offset);
    /* scissors */
    /* 17 */
    eg_set_scissors(context, dst_x, dst_y, dst_x + dst_width, dst_y + dst_height);

    eg_set_vb_data(context, src_x, src_y, dst_x, dst_y, w, h, src_height, flip_y);
    /* Vertex buffer setup */
    /* 18 */
    eg_set_vtx_resource(context);

    /* draw */
    /* 10 */
    eg_draw_auto(context);

    /* 7 */
    r700SyncSurf(context, dst_bo, 0,
                 RADEON_GEM_DOMAIN_VRAM|RADEON_GEM_DOMAIN_GTT,
		 CB_ACTION_ENA_bit | (1 << (id + 6)));

    radeonFlush(ctx);

    return GL_TRUE;
}
