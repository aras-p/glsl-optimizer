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

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_reg.h"
#include "r300_screen.h"
#include "r300_state_invariant.h"

struct pipe_viewport_state r300_viewport_identity = {
    .scale = {1.0, 1.0, 1.0, 1.0},
    .translate = {0.0, 0.0, 0.0, 0.0},
};

/* Calculate and emit invariant state. This is data that the 3D engine
 * will probably want at the beginning of every CS, but it's not currently
 * handled by any CSO setup, and in addition it doesn't really change much.
 *
 * Note that eventually this should be empty, but it's useful for development
 * and general unduplication of code. */
void r300_emit_invariant_state(struct r300_context* r300,
                               unsigned size, void* state)
{
    struct r300_capabilities* caps = r300_screen(r300->context.screen)->caps;
    CS_LOCALS(r300);

    BEGIN_CS(14 + (caps->has_tcl ? 2: 0));

    /*** Graphics Backend (GB) ***/
    /* Various GB enables */
    OUT_CS_REG(R300_GB_ENABLE, R300_GB_POINT_STUFF_ENABLE |
                               R300_GB_LINE_STUFF_ENABLE  |
                               R300_GB_TRIANGLE_STUFF_ENABLE);
    /* Subpixel multisampling for AA
     * These are commented out because glisse's CS checker doesn't like them.
     * I presume these will be re-enabled later.
     * OUT_CS_REG(R300_GB_MSPOS0, 0x66666666);
     * OUT_CS_REG(R300_GB_MSPOS1, 0x6666666);
     */
    /* Source of fog depth */
    OUT_CS_REG(R300_GB_SELECT, R300_GB_FOG_SELECT_1_1_W);

    /*** Fog (FG) ***/
    OUT_CS_REG(R300_FG_FOG_BLEND, 0x0);
    OUT_CS_REG(R300_FG_FOG_COLOR_R, 0x0);
    OUT_CS_REG(R300_FG_FOG_COLOR_G, 0x0);
    OUT_CS_REG(R300_FG_FOG_COLOR_B, 0x0);

    /*** VAP ***/
    /* Sign/normalize control */
    OUT_CS_REG(R300_VAP_PSC_SGN_NORM_CNTL, R300_SGN_NORM_NO_ZERO);
    /* TCL-only stuff */
    if (caps->has_tcl) {
        /* Amount of time to wait for vertex fetches in PVS */
        OUT_CS_REG(VAP_PVS_VTX_TIMEOUT_REG, 0xffff);
    }

    END_CS;

    /* XXX unsorted stuff from surface_fill */
    BEGIN_CS(44 + (caps->has_tcl ? 7 : 0) +
             (caps->family >= CHIP_FAMILY_RV350 ? 4 : 0));

    if (caps->has_tcl) {
        /*Flushing PVS is required before the VAP_GB registers can be changed*/
        OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0);
        OUT_CS_REG_SEQ(R300_VAP_GB_VERT_CLIP_ADJ, 4);
        OUT_CS_32F(1.0);
        OUT_CS_32F(1.0);
        OUT_CS_32F(1.0);
        OUT_CS_32F(1.0);
    }
    /* XXX point tex stuffing */
    OUT_CS_REG_SEQ(R300_GA_POINT_S0, 1);
    OUT_CS_32F(0.0);
    OUT_CS_REG_SEQ(R300_GA_POINT_S1, 1);
    OUT_CS_32F(1.0);
    /* XXX line tex stuffing */
    OUT_CS_REG_SEQ(R300_GA_LINE_S0, 1);
    OUT_CS_32F(0.0);
    OUT_CS_REG_SEQ(R300_GA_LINE_S1, 1);
    OUT_CS_32F(1.0);
    OUT_CS_REG(R300_GA_TRIANGLE_STIPPLE, 0x5 |
        (0x5 << R300_GA_TRIANGLE_STIPPLE_Y_SHIFT_SHIFT));
    /* XXX this big chunk should be refactored into rs_state */
    OUT_CS_REG(R300_GA_SOLID_RG, 0x00000000);
    OUT_CS_REG(R300_GA_SOLID_BA, 0x00000000);
    OUT_CS_REG(R300_GA_ROUND_MODE, 0x00000001);
    OUT_CS_REG(R300_GA_OFFSET, 0x00000000);
    OUT_CS_REG(R300_GA_FOG_SCALE, 0x3DBF1412);
    OUT_CS_REG(R300_GA_FOG_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SU_TEX_WRAP, 0x00000000);
    OUT_CS_REG(R300_SU_DEPTH_SCALE, 0x4B7FFFFF);
    OUT_CS_REG(R300_SU_DEPTH_OFFSET, 0x00000000);
    OUT_CS_REG(R300_SC_HYPERZ, 0x0000001C);
    OUT_CS_REG(R300_SC_EDGERULE, 0x2DA49525);
    OUT_CS_REG(R300_RB3D_AARESOLVE_CTL, 0x00000000);

    if (caps->family >= CHIP_FAMILY_RV350) {
        OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 0x01010101);
        OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD, 0xFEFEFEFE);
    }

    OUT_CS_REG(R300_ZB_BW_CNTL, 0x00000000);
    OUT_CS_REG(R300_ZB_DEPTHCLEARVALUE, 0x00000000);
    OUT_CS_REG(R300_ZB_HIZ_OFFSET, 0x00000000);
    OUT_CS_REG(R300_ZB_HIZ_PITCH, 0x00000000);

    /* XXX */
    OUT_CS_REG(R300_SC_CLIP_RULE, 0xaaaa);

    END_CS;
}
