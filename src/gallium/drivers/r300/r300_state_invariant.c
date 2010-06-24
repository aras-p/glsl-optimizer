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

/* Calculate and emit invariant state. This is data that the 3D engine
 * will probably want at the beginning of every CS, but it's not currently
 * handled by any CSO setup, and in addition it doesn't really change much.
 *
 * Note that eventually this should be empty, but it's useful for development
 * and general unduplication of code. */
void r300_emit_invariant_state(struct r300_context* r300,
                               unsigned size, void* state)
{
    CS_LOCALS(r300);

    BEGIN_CS(18 + (r300->screen->caps.is_rv350 ? 4 : 0));

    OUT_CS_REG(R300_GB_SELECT, 0);
    OUT_CS_REG(R300_FG_FOG_BLEND, 0);
    OUT_CS_REG(R300_GA_ROUND_MODE, 1);
    OUT_CS_REG(R300_GA_OFFSET, 0);
    OUT_CS_REG(R300_SU_TEX_WRAP, 0);
    OUT_CS_REG(R300_SU_DEPTH_SCALE, 0x4B7FFFFF);
    OUT_CS_REG(R300_SU_DEPTH_OFFSET, 0);
    OUT_CS_REG(R300_SC_HYPERZ, 0x1C);
    OUT_CS_REG(R300_SC_EDGERULE, 0x2DA49525);

    if (r300->screen->caps.is_rv350) {
        OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_LTE_THRESHOLD, 0x01010101);
        OUT_CS_REG(R500_RB3D_DISCARD_SRC_PIXEL_GTE_THRESHOLD, 0xFEFEFEFE);
    }

    END_CS;
}
