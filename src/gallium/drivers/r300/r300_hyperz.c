/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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


#include "r300_hyperz.h"
#include "r300_context.h"
#include "r300_reg.h"
#include "r300_fs.h"

/*****************************************************************************/
/* The ZTOP state                                                            */
/*****************************************************************************/

static boolean r300_dsa_writes_depth_stencil(struct r300_dsa_state* dsa)
{
    /* We are interested only in the cases when a new depth or stencil value
     * can be written and changed. */

    /* We might optionally check for [Z func: never] and inspect the stencil
     * state in a similar fashion, but it's not terribly important. */
    return (dsa->z_buffer_control & R300_Z_WRITE_ENABLE) ||
           (dsa->stencil_ref_mask & R300_STENCILWRITEMASK_MASK) ||
           ((dsa->z_buffer_control & R500_STENCIL_REFMASK_FRONT_BACK) &&
            (dsa->stencil_ref_bf & R300_STENCILWRITEMASK_MASK));
}

static boolean r300_dsa_alpha_test_enabled(struct r300_dsa_state* dsa)
{
    /* We are interested only in the cases when alpha testing can kill
     * a fragment. */
    uint32_t af = dsa->alpha_function;

    return (af & R300_FG_ALPHA_FUNC_ENABLE) &&
           (af & R300_FG_ALPHA_FUNC_ALWAYS) != R300_FG_ALPHA_FUNC_ALWAYS;
}

static void r300_update_ztop(struct r300_context* r300)
{
    struct r300_ztop_state* ztop_state =
        (struct r300_ztop_state*)r300->ztop_state.state;

    /* This is important enough that I felt it warranted a comment.
     *
     * According to the docs, these are the conditions where ZTOP must be
     * disabled:
     * 1) Alpha testing enabled
     * 2) Texture kill instructions in fragment shader
     * 3) Chroma key culling enabled
     * 4) W-buffering enabled
     *
     * The docs claim that for the first three cases, if no ZS writes happen,
     * then ZTOP can be used.
     *
     * (3) will never apply since we do not support chroma-keyed operations.
     * (4) will need to be re-examined (and this comment updated) if/when
     * Hyper-Z becomes supported.
     *
     * Additionally, the following conditions require disabled ZTOP:
     * 5) Depth writes in fragment shader
     * 6) Outstanding occlusion queries
     *
     * This register causes stalls all the way from SC to CB when changed,
     * but it is buffered on-chip so it does not hurt to write it if it has
     * not changed.
     *
     * ~C.
     */

    /* ZS writes */
    if (r300_dsa_writes_depth_stencil(r300->dsa_state.state) &&
           (r300_dsa_alpha_test_enabled(r300->dsa_state.state) ||  /* (1) */
            r300_fs(r300)->shader->info.uses_kill)) {              /* (2) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300_fragment_shader_writes_depth(r300_fs(r300))) { /* (5) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300->query_current) {                              /* (6) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else {
        ztop_state->z_buffer_top = R300_ZTOP_ENABLE;
    }

    r300->ztop_state.dirty = TRUE;
}

void r300_update_hyperz_state(struct r300_context* r300)
{
    r300_update_ztop(r300);
}
