/*
 * Copyright 2009 Joakim Sindholt <opensource@zhasha.com>
 *                Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_state_invariant.h"

/* Calculate and emit invariant state. This is data that the 3D engine
 * will probably want at the beginning of every CS, but it's not currently
 * handled by any CSO setup, and in addition it doesn't really change much.
 *
 * Note that eventually this should be empty, but it's useful for development
 * and general unduplication of code. */
void r300_emit_invariant_state(struct r300_context* r300)
{
    struct r300_capabilities* caps = r300_screen(r300->context.screen)->caps;
    CS_LOCALS(r300);

    BEGIN_CS(24 + (caps->has_tcl ? 2: 0));

    /* Various GB enables */
    OUT_CS_REG(R300_GB_ENABLE, R300_GB_POINT_STUFF_ENABLE |
        R300_GB_LINE_STUFF_ENABLE | R300_GB_TRIANGLE_STUFF_ENABLE);
    /* Subpixel multisampling for AA */
    OUT_CS_REG(R300_GB_MSPOS0, 0x66666666);
    OUT_CS_REG(R300_GB_MSPOS1, 0x66666666);
    /* GB tile config and pipe setup */
    OUT_CS_REG(R300_GB_TILE_CONFIG, R300_GB_TILE_DISABLE |
        r300_translate_gb_pipes(caps->num_frag_pipes));
    /* Source of fog depth */
    OUT_CS_REG(R300_GB_SELECT, R300_GB_FOG_SELECT_1_1_W);
    /* AA enable */
    OUT_CS_REG(R300_GB_AA_CONFIG, 0x0);
    /* GA errata fixes. */
    if (caps->is_r500) {
        OUT_CS_REG(R300_GA_ENHANCE,
                R300_GA_ENHANCE_DEADLOCK_CNTL_PREVENT_TCL |
                R300_GA_ENHANCE_FASTSYNC_CNTL_ENABLE |
                R500_GA_ENHANCE_REG_READWRITE_ENABLE |
                R500_GA_ENHANCE_REG_NOSTALL_ENABLE);
    } else {
        OUT_CS_REG(R300_GA_ENHANCE,
                R300_GA_ENHANCE_DEADLOCK_CNTL_PREVENT_TCL |
                R300_GA_ENHANCE_FASTSYNC_CNTL_ENABLE);
    }

    /* Fog block. */
    OUT_CS_REG(R300_FG_FOG_BLEND, 0x00000000);
    OUT_CS_REG(R300_FG_FOG_COLOR_R, 0x00000000);
    OUT_CS_REG(R300_FG_FOG_COLOR_G, 0x00000000);
    OUT_CS_REG(R300_FG_FOG_COLOR_B, 0x00000000);
    OUT_CS_REG(R300_FG_DEPTH_SRC, 0x00000000);

    /* TCL-only stuff */
    if (caps->has_tcl) {
        /* Amount of time to wait for vertex fetches in PVS */
        OUT_CS_REG(VAP_PVS_VTX_TIMEOUT_REG, 0xffff);
    }

    END_CS;

    /* XXX unsorted stuff from surface_fill */
    BEGIN_CS(99 + (caps->has_tcl ? 28 : 0));
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
        OUT_CS_REG(R300_VAP_CLIP_CNTL, R300_CLIP_DISABLE |
            R300_PS_UCP_MODE_CLIP_AS_TRIFAN);
        OUT_CS_REG_SEQ(R300_VAP_GB_VERT_CLIP_ADJ, 4);
        OUT_CS_32F(1.0);
        OUT_CS_32F(1.0);
        OUT_CS_32F(1.0);
        OUT_CS_32F(1.0);
    } else {
        OUT_CS_REG(R300_VAP_CNTL_STATUS, R300_VC_NO_SWAP |
                R300_VAP_TCL_BYPASS);
    }
    /* XXX magic number not in r300_reg */
    OUT_CS_REG(R300_VAP_PSC_SGN_NORM_CNTL, 0xAAAAAAAA);
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

    /* XXX */
    OUT_CS_REG(R300_SC_CLIP_RULE, 0xaaaa);

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
}
