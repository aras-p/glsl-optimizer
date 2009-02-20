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
    struct r300_capabilities* caps = ((struct r300_screen*)pipe->screen)->caps;
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

    BEGIN_CS(163 + (caps->is_r500 ? 22 : 14) + (caps->has_tcl ? 4 : 2));
    /* Flush PVS. */
    OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x0);

    OUT_CS_REG(R300_SE_VTE_CNTL, R300_VPORT_X_SCALE_ENA |
        R300_VPORT_X_OFFSET_ENA | R300_VPORT_Y_SCALE_ENA |
        R300_VPORT_Y_OFFSET_ENA | R300_VPORT_Z_SCALE_ENA |
        R300_VPORT_Z_OFFSET_ENA | R300_VTX_W0_FMT);
    /* Vertex size. */
    OUT_CS_REG(R300_VAP_VTX_SIZE, 0x8);
    /* Max and min vertex index clamp. */
    OUT_CS_REG(R300_VAP_VF_MAX_VTX_INDX, 0xFFFFFF);
    OUT_CS_REG(R300_VAP_VF_MIN_VTX_INDX, 0x0);
    /* XXX endian */
    OUT_CS_REG(R300_VAP_CNTL_STATUS, R300_VC_NO_SWAP);
    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_0, 0x0);
    /* XXX magic number not in r300_reg */
    OUT_CS_REG(R300_VAP_PSC_SGN_NORM_CNTL, 0xAAAAAAAA);
    OUT_CS_REG(R300_VAP_CLIP_CNTL, 0x0);
    OUT_CS_REG_SEQ(R300_VAP_GB_VERT_CLIP_ADJ, 4);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    /* XXX is this too long? */
    OUT_CS_REG(VAP_PVS_VTX_TIMEOUT_REG, 0xFFFF);
    OUT_CS_REG(R300_GB_ENABLE, R300_GB_POINT_STUFF_ENABLE |
        R300_GB_LINE_STUFF_ENABLE | R300_GB_TRIANGLE_STUFF_ENABLE);
    /* XXX more magic numbers */
    OUT_CS_REG(R300_GB_MSPOS0, 0x66666666);
    OUT_CS_REG(R300_GB_MSPOS1, 0x66666666);
    /* XXX why doesn't classic Mesa write the number of pipes, too? */
    OUT_CS_REG(R300_GB_TILE_CONFIG, R300_GB_TILE_ENABLE |
        R300_GB_TILE_SIZE_16);
    OUT_CS_REG(R300_GB_SELECT, R300_GB_FOG_SELECT_1_1_W);
    OUT_CS_REG(R300_GB_AA_CONFIG, 0x0);
    /* XXX point tex stuffing */
    OUT_CS_REG_SEQ(R300_GA_POINT_S0, 1);
    OUT_CS_32F(0.0);
    OUT_CS_REG_SEQ(R300_GA_POINT_S1, 1);
    OUT_CS_32F(1.0);
    OUT_CS_REG(R300_GA_TRIANGLE_STIPPLE, 0x5 |
        (0x5 << R300_GA_TRIANGLE_STIPPLE_Y_SHIFT_SHIFT));
    /* XXX should this be related to the actual point size? */
    OUT_CS_REG(R300_GA_POINT_MINMAX, 0x6 |
        (0x1800 << R300_GA_POINT_MINMAX_MAX_SHIFT));
    /* XXX this big chunk should be refactored into rs_state */
    OUT_CS_REG(R300_GA_LINE_CNTL, 0x00030006);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_CONFIG, 0x3BAAAAAB);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_VALUE, 0x00000000);
    OUT_CS_REG(R300_GA_LINE_S0, 0x00000000);
    OUT_CS_REG(R300_GA_LINE_S1, 0x3F800000);
    OUT_CS_REG(R300_GA_ENHANCE, 0x00000002);
    OUT_CS_REG(R300_GA_COLOR_CONTROL, 0x0003AAAA);
    OUT_CS_REG(R300_GA_SOLID_RG, 0x00000000);
    OUT_CS_REG(R300_GA_SOLID_BA, 0x00000000);
    OUT_CS_REG(R300_GA_POLY_MODE, 0x00000000);
    OUT_CS_REG(R300_GA_ROUND_MODE, 0x00000001);
    OUT_CS_REG(R300_GA_OFFSET, 0x00000000);
    OUT_CS_REG(R300_GA_FOG_SCALE, 0x3DBF1412);
    OUT_CS_REG(R300_GA_FOG_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SU_TEX_WRAP, 0x00000000);
    OUT_CS_REG(R300_SU_POLY_OFFSET_FRONT_SCALE, 0x00000000);
    OUT_CS_REG(R300_SU_POLY_OFFSET_FRONT_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SU_POLY_OFFSET_BACK_SCALE, 0x00000000);
    OUT_CS_REG(R300_SU_POLY_OFFSET_BACK_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SU_POLY_OFFSET_ENABLE, 0x00000000);
    OUT_CS_REG(R300_SU_CULL_MODE, 0x00000000);
    OUT_CS_REG(R300_SU_DEPTH_SCALE, 0x4B7FFFFF);
    OUT_CS_REG(R300_SU_DEPTH_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SC_HYPERZ, 0x0000001C);
    OUT_CS_REG(R300_SC_EDGERULE, 0x2DA49525);
    OUT_CS_REG(R300_FG_FOG_BLEND, 0x00000002);
    OUT_CS_REG(R300_FG_FOG_COLOR_R, 0x00000000);
    OUT_CS_REG(R300_FG_FOG_COLOR_G, 0x00000000);
    OUT_CS_REG(R300_FG_FOG_COLOR_B, 0x00000000);
    OUT_CS_REG(R300_FG_DEPTH_SRC, 0x00000000);
    OUT_CS_REG(R300_FG_DEPTH_SRC, 0x00000000);
    OUT_CS_REG(R300_RB3D_CCTL, 0x00000000);
    OUT_CS_REG(RB3D_COLOR_CHANNEL_MASK, 0x0000000F);

    /* XXX: Oh the wonderful unknown.
     * Not writing these 8 regs seems to make no difference at all and seeing
     * as how they're not documented, we should leave them out for now.
    OUT_CS_REG_SEQ(0x4E54, 8);
    for (i = 0; i < 8; i++) {
        OUT_CS(0x00000000);
    } */
    OUT_CS_REG(R300_RB3D_AARESOLVE_CTL, 0x00000000);
    OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 0x00000000);
    OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD, 0xFFFFFFFF);
    OUT_CS_REG(R300_ZB_FORMAT, 0x00000002);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT, 0x00000003);
    OUT_CS_REG(R300_ZB_BW_CNTL, 0x00000000);
    OUT_CS_REG(R300_ZB_DEPTHCLEARVALUE, 0x00000000);
    /* XXX Moar unknown that should probably be left out.
    OUT_CS_REG(0x4F30, 0x00000000);
    OUT_CS_REG(0x4F34, 0x00000000); */
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
    OUT_CS_REG(R300_FG_FOG_BLEND, 0x00000000);
    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_EXT_0, 0xF688F688);
    OUT_CS_REG(R300_VAP_VTX_STATE_CNTL, 0x1);
    OUT_CS_REG(R300_VAP_VSM_VTX_ASSM, 0x405);
    OUT_CS_REG(R300_SE_VTE_CNTL, 0x0000043F);
    OUT_CS_REG(R300_VAP_VTX_SIZE, 0x00000008);
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

    /* The size of the point we're about to draw, in sixths of pixels */
    OUT_CS_REG(R300_GA_POINT_SIZE,
        ((h * 6) & R300_POINTSIZE_Y_MASK) |
        ((w * 6) << R300_POINTSIZE_X_SHIFT));

    /* XXX */
    OUT_CS_REG(R300_SC_CLIP_RULE, 0xaaaa);

    /* Pixel scissors */
    OUT_CS_REG_SEQ(R300_SC_SCISSORS_TL, 2);
    OUT_CS((x << R300_SCISSORS_X_SHIFT) | (y << R300_SCISSORS_Y_SHIFT));
    OUT_CS((w << R300_SCISSORS_X_SHIFT) | (h << R300_SCISSORS_Y_SHIFT));

    /* RS block setup */
    if (caps->is_r500) {
        /* XXX We seem to be in disagreement about how many of these we have
         * RS:RS_IP_[0-15] [R/W] 32 bits Access: 8/16/32 MMReg:0x4074-0x40b0
         * Now that's from the docs. I don't care what the mesa driver says */
        OUT_CS_REG_SEQ(R500_RS_IP_0, 16);
        for (i = 0; i < 16; i++) {
            OUT_CS((R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_S_SHIFT) |
                (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_T_SHIFT) |
                (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_R_SHIFT) |
                (R500_RS_IP_PTR_K1 << R500_RS_IP_TEX_PTR_Q_SHIFT));
        }
        OUT_CS_REG_SEQ(R300_RS_COUNT, 2);
        OUT_CS((1 << R300_IC_COUNT_SHIFT) | R300_HIRES_EN);
        OUT_CS(0x00000000);
        OUT_CS_REG(R500_RS_INST_0, R500_RS_INST_COL_CN_WRITE);
    } else {
        OUT_CS_REG_SEQ(R300_RS_IP_0, 8);
        for (i = 0; i < 8; i++) {
            OUT_CS(R300_RS_SEL_T(R300_RS_SEL_K0) |
                R300_RS_SEL_R(R300_RS_SEL_K0) | R300_RS_SEL_Q(R300_RS_SEL_K1));
        }
        OUT_CS_REG_SEQ(R300_RS_COUNT, 2);
        OUT_CS((1 << R300_IC_COUNT_SHIFT) | R300_HIRES_EN);
        /* XXX Shouldn't this be 0? */
        OUT_CS(1);
        OUT_CS_REG(R300_RS_INST_0, R300_RS_INST_COL_CN_WRITE);
    }
    END_CS;

    /* Fragment shader setup */
    if (caps->is_r500) {
        r500_emit_fragment_shader(r300, &r500_passthrough_fragment_shader);
    } else {
        r300_emit_fragment_shader(r300, &r300_passthrough_fragment_shader);
    }

    BEGIN_CS(8 + (caps->has_tcl ? 20 : 2));
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

    r300_emit_blend_state(r300, &blend_clear_state);
    r300_emit_blend_color_state(r300, &blend_color_clear_state);
    r300_emit_dsa_state(r300, &dsa_clear_state);

    BEGIN_CS(24);
    /* Flush colorbuffer and blend caches. */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FINISH_SIGNAL);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);

    OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0, 1);
    OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    /* XXX Fix color format in case it's not ARGB8888 */
    OUT_CS_REG(R300_RB3D_COLORPITCH0, pixpitch | R300_COLOR_FORMAT_ARGB8888);
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

void r300_init_surface_functions(struct r300_context* r300)
{
    r300->context.surface_fill = r300_surface_fill;
}
