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

static uint32_t mesa_format_to_cb_format(gl_format mesa_format)
{
    uint32_t comp_swap, format, color_info = 0;
    //fprintf(stderr,"format for copy %s\n",_mesa_get_format_name(mesa_format));
    switch(mesa_format) {
        /* XXX check and add these */
        case MESA_FORMAT_RGBA8888:
            comp_swap = SWAP_STD_REV;
            format = FMT_8_8_8_8;
            break;
        case MESA_FORMAT_RGBA8888_REV:
            comp_swap = SWAP_STD;
            format = FMT_8_8_8_8;
            break;
        case MESA_FORMAT_ARGB8888:
        case MESA_FORMAT_XRGB8888:
            comp_swap = SWAP_ALT;
            format = FMT_8_8_8_8;
            break;
        case MESA_FORMAT_ARGB8888_REV:
            comp_swap = SWAP_ALT_REV;
            format = FMT_8_8_8_8;
            break;
        case MESA_FORMAT_RGB565:
            comp_swap = SWAP_ALT_REV;
            format = FMT_5_6_5;
            break;
        case MESA_FORMAT_ARGB1555:
            comp_swap = SWAP_ALT_REV;
            format = FMT_1_5_5_5;
            break;   
        case MESA_FORMAT_ARGB4444:
            comp_swap = SWAP_ALT;
            format = FMT_4_4_4_4;
            break;
        case MESA_FORMAT_S8_Z24:
            format = FMT_8_24;
            comp_swap = SWAP_STD;
            break;
        default:
            fprintf(stderr,"Invalid format for copy %s\n",_mesa_get_format_name(mesa_format));
            assert("Invalid format for US output\n");
            return 0;
    }

    SETfield(color_info, format, CB_COLOR0_INFO__FORMAT_shift,
             CB_COLOR0_INFO__FORMAT_mask);
    SETfield(color_info, comp_swap, COMP_SWAP_shift, COMP_SWAP_mask);

    return color_info;

}

static inline void
set_render_target(context_t *context, struct radeon_bo *bo, gl_format format,
                  int pitch, int bpp, int w, int h, intptr_t dst_offset)
{
    uint32_t cb_color0_base, cb_color0_size = 0, cb_color0_info = 0, cb_color0_view;
    int nPitchInPixel, id = 0;
    BATCH_LOCALS(&context->radeon);

    cb_color0_base = dst_offset / 256;
    cb_color0_view = 0;

    cb_color0_info = mesa_format_to_cb_format(format);

    nPitchInPixel = pitch/bpp;
    SETfield(cb_color0_size, (nPitchInPixel / 8) - 1,
             PITCH_TILE_MAX_shift, PITCH_TILE_MAX_mask);
    SETfield(cb_color0_size, ((nPitchInPixel * h) / 64) - 1,
             SLICE_TILE_MAX_shift, SLICE_TILE_MAX_mask);
    SETfield(cb_color0_info, ENDIAN_NONE, ENDIAN_shift, ENDIAN_mask);
    SETfield(cb_color0_info, ARRAY_LINEAR_GENERAL,
             CB_COLOR0_INFO__ARRAY_MODE_shift, CB_COLOR0_INFO__ARRAY_MODE_mask);
/*  
    if(4 == bpp)
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
*/
    SETbit(cb_color0_info, SOURCE_FORMAT_bit);
    //SETbit(cb_color0_info, BLEND_CLAMP_bit);
    SETbit(cb_color0_info, BLEND_BYPASS_bit);
    SETfield(cb_color0_info, NUMBER_UNORM, NUMBER_TYPE_shift, NUMBER_TYPE_mask);

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

    radeon_cs_space_check_with_bo(context->radeon.cmdbuf.cs,
                                  pbo,
                                  RADEON_GEM_DOMAIN_GTT, 0);

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

    radeon_cs_space_check_with_bo(context->radeon.cmdbuf.cs,
                                  bo,
                                  RADEON_GEM_DOMAIN_GTT, 0);

}

static void
set_tex_resource(context_t * context,
		 gl_format format, struct radeon_bo *bo, int w, int h,
		 int pitch, intptr_t src_offset)
{
    uint32_t sq_tex_resource0, sq_tex_resource1, sq_tex_resource2, sq_tex_resource4, sq_tex_resource6;
    int bpp = _mesa_get_format_bytes(format);
    int TexelPitch = pitch/bpp;
    // int tex_format = mesa_format_to_us_format(format);

    sq_tex_resource0 = sq_tex_resource1 = sq_tex_resource2 = sq_tex_resource4 = sq_tex_resource6 = 0;
    BATCH_LOCALS(&context->radeon);
        
    SETfield(sq_tex_resource0, SQ_TEX_DIM_2D, DIM_shift, DIM_mask);
    SETfield(sq_tex_resource0, ARRAY_LINEAR_GENERAL,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_shift,
                 SQ_TEX_RESOURCE_WORD0_0__TILE_MODE_mask);

    if (bpp == 4) {
        SETfield(sq_tex_resource1, FMT_8_8_8_8,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);

        SETfield(sq_tex_resource4, SQ_SEL_Z,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
        SETfield(sq_tex_resource4, SQ_SEL_Y,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
        SETfield(sq_tex_resource4, SQ_SEL_X,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
        SETfield(sq_tex_resource4, SQ_SEL_W,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);
    } else {
        SETfield(sq_tex_resource1, FMT_5_6_5,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_shift,
                 SQ_TEX_RESOURCE_WORD1_0__DATA_FORMAT_mask);
        SETfield(sq_tex_resource4, SQ_SEL_Z,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_X_mask);
        SETfield(sq_tex_resource4, SQ_SEL_Y,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Y_mask);
        SETfield(sq_tex_resource4, SQ_SEL_X,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_Z_mask);
        SETfield(sq_tex_resource4, SQ_SEL_1,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_shift,
                 SQ_TEX_RESOURCE_WORD4_0__DST_SEL_W_mask);

    }

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
/*
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

*/
}

static void
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
	
    int i;
    uint32_t clip_tl = 0, clip_br = 0;

    SETfield(clip_tl, 0, PA_SC_CLIPRECT_0_TL__TL_X_shift,
                         PA_SC_CLIPRECT_0_TL__TL_X_mask);
    SETfield(clip_tl, 0, PA_SC_CLIPRECT_0_TL__TL_Y_shift,
                         PA_SC_CLIPRECT_0_TL__TL_Y_mask);
    SETfield(clip_br, 8191, PA_SC_CLIPRECT_0_BR__BR_X_shift,
                            PA_SC_CLIPRECT_0_BR__BR_X_mask);
    SETfield(clip_br, 8191, PA_SC_CLIPRECT_0_BR__BR_Y_shift,
                            PA_SC_CLIPRECT_0_BR__BR_Y_mask);

    BATCH_LOCALS(&context->radeon);

    BEGIN_BATCH_NO_AUTOSTATE(13);
    R600_OUT_BATCH_REGSEQ(PA_SC_SCREEN_SCISSOR_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

    R600_OUT_BATCH_REGSEQ(PA_SC_WINDOW_OFFSET, 3);
    R600_OUT_BATCH(0); //PA_SC_WINDOW_OFFSET
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit)); //PA_SC_WINDOW_SCISSOR_TL
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));

/* clip disabled ?
    R600_OUT_BATCH(CLIP_RULE_mask); //PA_SC_CLIPRECT_RULE);
    R600_OUT_BATCH(clip_tl); //PA_SC_CLIPRECT_0_TL
    R600_OUT_BATCH(clip_br); //PA_SC_CLIPRECT_0_BR
    R600_OUT_BATCH(clip_tl); // 1
    R600_OUT_BATCH(clip_br);
    R600_OUT_BATCH(clip_tl); // 2
    R600_OUT_BATCH(clip_br);
    R600_OUT_BATCH(clip_tl); // 3
    R600_OUT_BATCH(clip_br);
*/

    R600_OUT_BATCH_REGSEQ(PA_SC_GENERIC_SCISSOR_TL, 2);
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));
    END_BATCH();

    /* XXX 16 of these PA_SC_VPORT_SCISSOR_0_TL_num ... */
    BEGIN_BATCH_NO_AUTOSTATE(4);
    R600_OUT_BATCH_REGSEQ(PA_SC_VPORT_SCISSOR_0_TL, 2 );
    R600_OUT_BATCH((x1 << 0) | (y1 << 16) | (WINDOW_OFFSET_DISABLE_bit));
    R600_OUT_BATCH((x2 << 0) | (y2 << 16));
    END_BATCH();

/*
    BEGIN_BATCH_NO_AUTOSTATE(2 + 2 * PA_SC_VPORT_ZMIN_0_num);
    R600_OUT_BATCH_REGSEQ(PA_SC_VPORT_ZMIN_0, 2 * PA_SC_VPORT_ZMIN_0_num);
    for(i = 0 i < PA_SC_VPORT_ZMIN_0_num; i++)
        R600_OUT_BATCH(radeonPackFloat32(0.0F));
        R600_OUT_BATCH(radeonPackFloat32(1.0F));
    } 
    END_BATCH();
*/
    COMMIT_BATCH();

}

static void 
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
    int i;
    BATCH_LOCALS(&context->radeon);
/*    
    BEGIN_BATCH_NO_AUTOSTATE(sizeof(r7xx_default_state)/4);
    for(i=0; i< sizeof(r7xx_default_state)/4; i++)
        R600_OUT_BATCH(r7xx_default_state[i]);
    END_BATCH();
*/
    BEGIN_BATCH_NO_AUTOSTATE(45);
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

/*
    R600_OUT_BATCH_REGSEQ(PA_CL_VPORT_XSCALE_0, 6);
    R600_OUT_BATCH(0.0f); // PA_CL_VPORT_XSCALE
    R600_OUT_BATCH(0.0f);      // PA_CL_VPORT_XOFFSET
    R600_OUT_BATCH(0.0f);      // PA_CL_VPORT_YSCALE
    R600_OUT_BATCH(0.0f);      // PA_CL_VPORT_YOFFSET
    R600_OUT_BATCH(0.0f);      // PA_CL_VPORT_ZSCALE
    R600_OUT_BATCH(0.0f);      // PA_CL_VPORT_ZOFFSET
*/
    R600_OUT_BATCH_REGVAL(PA_CL_VTE_CNTL, VTX_XY_FMT_bit);
    R600_OUT_BATCH_REGVAL(PA_CL_VS_OUT_CNTL, 0 );
    R600_OUT_BATCH_REGVAL(PA_CL_CLIP_CNTL, CLIP_DISABLE_bit);
    R600_OUT_BATCH_REGVAL(PA_SU_SC_MODE_CNTL, (FACE_bit) |
        (POLYMODE_PTYPE__TRIANGLES << POLYMODE_FRONT_PTYPE_shift) |
        (POLYMODE_PTYPE__TRIANGLES << POLYMODE_BACK_PTYPE_shift)); 
    R600_OUT_BATCH_REGVAL(PA_SU_VTX_CNTL, (PIX_CENTER_bit) |
        (X_ROUND_TO_EVEN << PA_SU_VTX_CNTL__ROUND_MODE_shift) |
        (X_1_256TH << QUANT_MODE_shift));
    R600_OUT_BATCH_REGVAL(VGT_MAX_VTX_INDX, 2048);
    END_BATCH();
/*
       if (dev_priv->flags & RADEON_FAMILY_MASK) >= CHIP_RV770) {
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

	//OR :
	r700WaitForIdleClean(context_t *context);
*/
	/* SQ config */
	/*XXX*/
/*
     r700SendSQConfig(GLcontext *ctx, struct radeon_state_atom *atom);
	// OR:
	OUT_RING(CP_PACKET3(R600_IT_SET_CONFIG_REG, 6));
	OUT_RING((R600_SQ_CONFIG - R600_SET_CONFIG_REG_OFFSET) >> 2);
	OUT_RING(sq_config);
	OUT_RING(sq_gpr_resource_mgmt_1);
	OUT_RING(sq_gpr_resource_mgmt_2);
	OUT_RING(sq_thread_resource_mgmt);
	OUT_RING(sq_stack_resource_mgmt_1);
	OUT_RING(sq_stack_resource_mgmt_2);
	ADVANCE_RING();
*/
}

static GLboolean validate_buffers(context_t *rmesa,
                                  struct radeon_bo *src_bo,
                                  struct radeon_bo *dst_bo)
{
    int ret;
    radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs,
                                      src_bo, RADEON_GEM_DOMAIN_VRAM, 0);

    radeon_cs_space_add_persistent_bo(rmesa->radeon.cmdbuf.cs,
                                      dst_bo, 0, RADEON_GEM_DOMAIN_VRAM);

    ret = radeon_cs_space_check_with_bo(rmesa->radeon.cmdbuf.cs,
                                        first_elem(&rmesa->radeon.dma.reserved)->bo,
                                        RADEON_GEM_DOMAIN_GTT, 0);
    if (ret)
        return GL_FALSE;

    return GL_TRUE;
}

GLboolean r600_blit(context_t *context,
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
    int id = 0;
    uint32_t cb_bpp;

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

    /* Flush is needed to make sure that source buffer has correct data */
    radeonFlush(context->radeon.glCtx);

    if (!validate_buffers(context, src_bo, dst_bo))
        return GL_FALSE;

    rcommonEnsureCmdBufSpace(&context->radeon, 200, __FUNCTION__);

    /* set clear state */
    set_default_state(context);
    /* src */
    set_tex_resource(context, src_mesaformat, src_bo,
		     src_width, src_height, src_pitch, src_offset);

    set_tex_sampler(context);

    /* dst */
    cb_bpp = _mesa_get_format_bytes(dst_mesaformat);
    set_render_target(context, dst_bo, dst_mesaformat,
		      dst_pitch, cb_bpp, dst_width, dst_height, dst_offset);
    /* shaders */ 
    load_shaders(context->radeon.glCtx);

    set_shaders(context);

    /* scissors */
    set_scissors(context, 0, 0, 8191, 8191);

    set_vb_data(context, src_x, src_y, dst_x, dst_y, w, h, src_height, flip_y);
    /* Vertex buffer setup */
    set_vtx_resource(context);

    /* draw */
    draw_auto(context);

    r700SyncSurf(context, dst_bo, 0,
                 RADEON_GEM_DOMAIN_VRAM|RADEON_GEM_DOMAIN_GTT,
		 CB_ACTION_ENA_bit | (1 << (id + 6)));

    radeonFlush(context->radeon.glCtx);

    return GL_TRUE;
}
