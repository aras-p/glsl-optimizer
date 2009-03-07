/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_surface.h"

/* Provides pipe_context's "surface_fill". Commonly used for clearing
 * buffers. */
static void r300_surface_fill(struct pipe_context* pipe,
                              struct pipe_surface* dest,
                              unsigned x, unsigned y,
                              unsigned w, unsigned h,
                              unsigned color)
{
    struct r300_context* r300 = r300_context(pipe);
    CS_LOCALS(r300);
    struct r300_capabilities* caps = r300_screen(pipe->screen)->caps;
    struct r300_texture* tex = (struct r300_texture*)dest->texture;
    int i;

    float r, g, b, a;
    unsigned pixpitch = tex->stride / tex->tex.block.size;
    r = (float)((color >> 16) & 0xff) / 255.0f;
    g = (float)((color >>  8) & 0xff) / 255.0f;
    b = (float)((color >>  0) & 0xff) / 255.0f;
    debug_printf("r300: Filling surface %p at (%d,%d),"
        " dimensions %dx%d (pixel pitch %d), color 0x%x\n",
        dest, x, y, w, h, pixpitch, color);

    /* Fallback? */
    /*if (0) {
        debug_printf("r300: Falling back on surface clear...");
        void* map = pipe->screen->surface_map(pipe->screen, dest,
            PIPE_BUFFER_USAGE_CPU_WRITE);
        pipe_fill_rect(map, &dest->block, &dest->stride, x, y, w, h, color);
        pipe->screen->surface_unmap(pipe->screen, dest);
        return;
    }*/

    r300_emit_invariant_state(r300);

    r300_emit_blend_state(r300, &blend_clear_state);
    r300_emit_blend_color_state(r300, &blend_color_clear_state);
    r300_emit_dsa_state(r300, &dsa_clear_state);
    r300_emit_rs_state(r300, &rs_clear_state);

    /* Fragment shader setup */
    if (caps->is_r500) {
        r500_emit_fragment_shader(r300, &r500_passthrough_fragment_shader);
        r300_emit_rs_block_state(r300, &r500_rs_block_clear_state);
    } else {
        r300_emit_fragment_shader(r300, &r300_passthrough_fragment_shader);
        r300_emit_rs_block_state(r300, &r300_rs_block_clear_state);
    }

    BEGIN_CS(106 + (caps->has_tcl ? 2 : 0));
    /* Flush PVS. */
    OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x0);

    OUT_CS_REG(R300_SE_VTE_CNTL, R300_VPORT_X_SCALE_ENA |
        R300_VPORT_X_OFFSET_ENA | R300_VPORT_Y_SCALE_ENA |
        R300_VPORT_Y_OFFSET_ENA | R300_VPORT_Z_SCALE_ENA |
        R300_VPORT_Z_OFFSET_ENA | R300_VTX_W0_FMT);
    /* Max and min vertex index clamp. */
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, 0xFFFFFF);
    OUT_CS_REG(R300_VAP_VF_MIN_VTX_INDX, 0x0);
    /* XXX endian */
    if (caps->has_tcl) {
        OUT_CS_REG(R300_VAP_CNTL_STATUS, R300_VC_NO_SWAP);
    } else {
        OUT_CS_REG(R300_VAP_CNTL_STATUS, R300_VC_NO_SWAP |
                R300_VAP_TCL_BYPASS);
    }
    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0, 0x0);
    /* XXX magic number not in r300_reg */
    OUT_CS_REG(R300_VAP_PSC_SGN_NORM_CNTL, 0xAAAAAAAA);
    OUT_CS_REG(R300_VAP_CLIP_CNTL, 0x0);
    OUT_CS_REG_SEQ(R300_VAP_GB_VERT_CLIP_ADJ, 4);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    /* XXX point tex stuffing */
    OUT_CS_REG_SEQ(R300_GA_POINT_S0, 1);
    OUT_CS_32F(0.0);
    OUT_CS_REG_SEQ(R300_GA_POINT_S1, 1);
    OUT_CS_32F(1.0);
    OUT_CS_REG(R300_GA_TRIANGLE_STIPPLE, 0x5 |
        (0x5 << R300_GA_TRIANGLE_STIPPLE_Y_SHIFT_SHIFT));
    /* XXX this big chunk should be refactored into rs_state */
    OUT_CS_REG(R300_GA_LINE_S0, 0x00000000);
    OUT_CS_REG(R300_GA_LINE_S1, 0x3F800000);
    OUT_CS_REG(R300_GA_SOLID_RG, 0x00000000);
    OUT_CS_REG(R300_GA_SOLID_BA, 0x00000000);
    OUT_CS_REG(R300_GA_POLY_MODE, 0x00000000);
    OUT_CS_REG(R300_GA_ROUND_MODE, 0x00000001);
    OUT_CS_REG(R300_GA_OFFSET, 0x00000000);
    OUT_CS_REG(R300_GA_FOG_SCALE, 0x3DBF1412);
    OUT_CS_REG(R300_GA_FOG_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SU_TEX_WRAP, 0x00000000);
    OUT_CS_REG(R300_SU_DEPTH_SCALE, 0x4B7FFFFF);
    OUT_CS_REG(R300_SU_DEPTH_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SC_HYPERZ, 0x0000001C);
    OUT_CS_REG(R300_SC_EDGERULE, 0x2DA49525);
    OUT_CS_REG(R300_RB3D_CCTL, 0x00000000);
    OUT_CS_REG(RB3D_COLOR_CHANNEL_MASK, 0x0000000F);
    OUT_CS_REG(R300_RB3D_AARESOLVE_CTL, 0x00000000);
    OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 0x00000000);
    OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD, 0xFFFFFFFF);
    OUT_CS_REG(R300_ZB_FORMAT, 0x00000002);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT, 0x00000003);
    OUT_CS_REG(R300_ZB_BW_CNTL, 0x00000000);
    OUT_CS_REG(R300_ZB_DEPTHCLEARVALUE, 0x00000000);
    OUT_CS_REG(R300_ZB_HIZ_OFFSET, 0x00000000);
    OUT_CS_REG(R300_ZB_HIZ_PITCH, 0x00000000);
    if (caps->has_tcl) {
        OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0,
            (R300_DATA_TYPE_FLOAT_4 << R300_DATA_TYPE_0_SHIFT) |
            ((R300_LAST_VEC | (1 << R300_DST_VEC_LOC_SHIFT) |
                R300_DATA_TYPE_FLOAT_4) << R300_DATA_TYPE_1_SHIFT));
    } else {
        OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0,
            (R300_DATA_TYPE_FLOAT_4 << R300_DATA_TYPE_0_SHIFT) |
            ((R300_LAST_VEC | (2 << R300_DST_VEC_LOC_SHIFT) |
                R300_DATA_TYPE_FLOAT_4) << R300_DATA_TYPE_1_SHIFT));
    }
    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_EXT_0,
            (R300_VAP_SWIZZLE_XYZW << R300_SWIZZLE0_SHIFT) |
            (R300_VAP_SWIZZLE_XYZW << R300_SWIZZLE1_SHIFT));
    OUT_CS_REG(R300_VAP_VTX_STATE_CNTL, 0x1);
    OUT_CS_REG(R300_VAP_VSM_VTX_ASSM, 0x405);
    OUT_CS_REG(R300_SE_VTE_CNTL, 0x0000043F);
    /* Vertex size. */
    OUT_CS_REG(R300_VAP_VTX_SIZE, 0x8);
    OUT_CS_REG(R300_VAP_PSC_SGN_NORM_CNTL, 0xAAAAAAAA);
    OUT_CS_REG(R300_VAP_OUTPUT_VTX_FMT_0, 0x00000003);
    OUT_CS_REG(R300_VAP_OUTPUT_VTX_FMT_1, 0x00000000);
    OUT_CS_REG(R300_TX_ENABLE, 0x0);
    /* XXX viewport setup */
    OUT_CS_REG_SEQ(R300_SE_VPORT_XSCALE, 6);
    OUT_CS_32F(1.0);
    OUT_CS_32F((float)x);
    OUT_CS_32F(1.0);
    OUT_CS_32F((float)y);
    OUT_CS_32F(1.0);
    OUT_CS_32F(0.0);

    if (caps->has_tcl) {
        OUT_CS_REG(R300_VAP_CLIP_CNTL, R300_CLIP_DISABLE |
            R300_PS_UCP_MODE_CLIP_AS_TRIFAN);
    }

    /* XXX */
    OUT_CS_REG(R300_SC_CLIP_RULE, 0xaaaa);
    END_CS;

    BEGIN_CS(7 + (caps->has_tcl ? 21 : 2));
    OUT_CS_REG_SEQ(R300_US_OUT_FMT_0, 4);
    OUT_CS(R300_C0_SEL_B | R300_C1_SEL_G | R300_C2_SEL_R | R300_C3_SEL_A);
    OUT_CS(R300_US_OUT_FMT_UNUSED);
    OUT_CS(R300_US_OUT_FMT_UNUSED);
    OUT_CS(R300_US_OUT_FMT_UNUSED);
    OUT_CS_REG(R300_US_W_FMT, R300_W_FMT_W0);
    /* XXX these magic numbers should be explained when
     * this becomes a cached state object */
    if (caps->has_tcl) {
        OUT_CS_REG(R300_VAP_CNTL, 0xA |
            (0x5 << R300_PVS_NUM_CNTLRS_SHIFT) |
            (0xB << R300_VF_MAX_VTX_NUM_SHIFT) |
            (caps->num_vert_fpus << R300_PVS_NUM_FPUS_SHIFT));
        OUT_CS_REG(R300_VAP_PVS_CODE_CNTL_0, 0x00100000);
        OUT_CS_REG(R300_VAP_PVS_CONST_CNTL, 0x00000000);
        OUT_CS_REG(R300_VAP_PVS_CODE_CNTL_1, 0x00000001);
        /* XXX translate these back into normal instructions */
        OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x1);
        OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG, 0x0);
        OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, 8);
        OUT_CS(0x00F00203);
        OUT_CS(0x00D10001);
        OUT_CS(0x01248001);
        OUT_CS(0x00000000);
        OUT_CS(0x00F02203);
        OUT_CS(0x00D10021);
        OUT_CS(0x01248021);
        OUT_CS(0x00000000);
    } else {
        OUT_CS_REG(R300_VAP_CNTL, 0xA |
            (0x5 << R300_PVS_NUM_CNTLRS_SHIFT) |
            (0x5 << R300_VF_MAX_VTX_NUM_SHIFT) |
            (caps->num_vert_fpus << R300_PVS_NUM_FPUS_SHIFT));
    }
    END_CS;

    BEGIN_CS(29);

    /* Pixel scissors */
    OUT_CS_REG_SEQ(R300_SC_SCISSORS_TL, 2);
    OUT_CS((x << R300_SCISSORS_X_SHIFT) | (y << R300_SCISSORS_Y_SHIFT));
    OUT_CS((w << R300_SCISSORS_X_SHIFT) | (h << R300_SCISSORS_Y_SHIFT));

    /* The size of the point we're about to draw, in sixths of pixels */
    OUT_CS_REG(R300_GA_POINT_SIZE,
        ((h * 6) & R300_POINTSIZE_Y_MASK) |
        ((w * 6) << R300_POINTSIZE_X_SHIFT));

    /* Flush colorbuffer and blend caches. */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FINISH_SIGNAL);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);

    OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0, 1);
    OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    OUT_CS_REG(R300_RB3D_COLORPITCH0, pixpitch |
        r300_translate_colorformat(tex->tex.format));
    OUT_CS_REG(RB3D_COLOR_CHANNEL_MASK, 0x0000000F);
    /* XXX Packet3 */
    OUT_CS(CP_PACKET3(R200_3D_DRAW_IMMD_2, 8));
    OUT_CS(R300_PRIM_TYPE_POINT | R300_PRIM_WALK_RING |
    (1 << R300_PRIM_NUM_VERTICES_SHIFT));
    OUT_CS_32F(w / 2.0);
    OUT_CS_32F(h / 2.0);
    /* XXX this should be the depth value to clear to */
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(r);
    OUT_CS_32F(g);
    OUT_CS_32F(b);
    OUT_CS_32F(1.0);

    /* XXX figure out why this is 0xA and not 0x2 */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT, 0xA);
    /* XXX OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE); */

    END_CS;

    r300->dirty_hw++;
}

static void r300_surface_copy(struct pipe_context* pipe,
                              boolean do_flip,
                              struct pipe_surface* dest,
                              unsigned destx, unsigned desty,
                              struct pipe_surface* src,
                              unsigned srcx, unsigned srcy,
                              unsigned w, unsigned h)
{
    struct r300_context* r300 = r300_context(pipe);
    CS_LOCALS(r300);
    struct r300_texture* srctex = (struct r300_texture*)src->texture;
    struct r300_texture* desttex = (struct r300_texture*)dest->texture;

    unsigned pixpitch = srctex->stride / srctex->tex.block.size;
    debug_printf("r300: Copying surface %p at (%d,%d) to %p at (%d, %d),"
        " dimensions %dx%d (pixel pitch %d)\n",
        src, srcx, srcy, dest, destx, desty, w, h, pixpitch);

    if (TRUE) {
        debug_printf("r300: Falling back on surface_copy\n");
        return util_surface_copy(pipe, do_flip, dest, destx, desty, src,
                srcx, srcy, w, h);
    }
#if 0
    BEGIN_CS();
    OUT_CS_REG(RADEON_DEFAULT_SC_BOTTOM_RIGHT,(RADEON_DEFAULT_SC_RIGHT_MAX |
                RADEON_DEFAULT_SC_BOTTOM_MAX));
    OUT_ACCEL_REG(RADEON_DP_GUI_MASTER_CNTL, (RADEON_GMC_DST_PITCH_OFFSET_CNTL |
					  RADEON_GMC_SRC_PITCH_OFFSET_CNTL |
					  RADEON_GMC_BRUSH_NONE |
					  (datatype << 8) |
					  RADEON_GMC_SRC_DATATYPE_COLOR |
					  RADEON_ROP[rop].rop |
					  RADEON_DP_SRC_SOURCE_MEMORY |
					  RADEON_GMC_CLR_CMP_CNTL_DIS));
    OUT_CS_REG(RADEON_DP_BRUSH_FRGD_CLR, 0xffffffff);
    OUT_CS_REG(RADEON_DP_BRUSH_BKGD_CLR, 0x0);
    OUT_CS_REG(RADEON_DP_SRC_FRGD_CLR, 0xffffffff);
    OUT_CS_REG(RADEON_DP_SRC_BKGD_CLR, 0x0);
    OUT_ACCEL_REG(RADEON_DP_WRITE_MASK, planemask);
    OUT_ACCEL_REG(RADEON_DP_CNTL, ((info->accel_state->xdir >= 0 ? RADEON_DST_X_LEFT_TO_RIGHT : 0) |
			       (info->accel_state->ydir >= 0 ? RADEON_DST_Y_TOP_TO_BOTTOM : 0));
);

    OUT_CS_REG_SEQ(RADEON_DST_PITCH_OFFSET, 1);
    OUT_CS_RELOC(desttex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);

    OUT_CS_REG_SEQ(RADEON_SRC_PITCH_OFFSET, 1);
    OUT_CS_RELOC(srctex->buffer, 0,
            RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0, 0);

    OUT_CS_REG(RADEON_SRC_Y_X, (srcy << 16) | srcx);
    OUT_CS_REG(RADEON_DST_Y_X, (desty << 16) | destx);
    OUT_CS_REG(RADEON_DST_HEIGHT_WIDTH, (h << 16) | w);
    OUT_CS_REG(RADEON_DSTCACHE_CTLSTAT, RADEON_RB2D_DC_FLUSH_ALL);
    OUT_CS_REG(RADEON_WAIT_UNTIL,
                  RADEON_WAIT_2D_IDLECLEAN | RADEON_WAIT_DMA_GUI_IDLE);
    END_CS;
#endif
}

void r300_init_surface_functions(struct r300_context* r300)
{
    r300->context.surface_fill = r300_surface_fill;
    r300->context.surface_copy = r300_surface_copy;
}
