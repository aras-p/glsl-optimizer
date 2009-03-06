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

    BEGIN_CS(16);
    /* Amount of time to wait for vertex fetches in PVS */
    OUT_CS_REG(VAP_PVS_VTX_TIMEOUT_REG, 0xffff);
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
    END_CS;
}
