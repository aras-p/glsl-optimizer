/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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

#include "draw/draw_context.h"
#include "draw/draw_private.h"

#include "util/u_simple_list.h"
#include "util/u_upload_mgr.h"

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_emit.h"

static void r300_flush(struct pipe_context* pipe,
                       unsigned flags,
                       struct pipe_fence_handle** fence)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_query *query;
    struct r300_atom *atom;
    struct r300_fence **rfence = (struct r300_fence**)fence;

    u_upload_flush(r300->upload_vb);
    u_upload_flush(r300->upload_ib);

    if (r300->draw && !r300->draw_vbo_locked)
	r300_draw_flush_vbuf(r300);

    if (r300->dirty_hw) {
        r300_emit_hyperz_end(r300);
        r300_emit_query_end(r300);
        if (r300->screen->caps.index_bias_supported)
            r500_emit_index_bias(r300, 0);

        r300->flush_counter++;
        r300->rws->cs_flush(r300->cs);
        r300->dirty_hw = 0;

        /* New kitchen sink, baby. */
        foreach_atom(r300, atom) {
            if (atom->state || atom->allow_null_state) {
                r300_mark_atom_dirty(r300, atom);
            }
        }

        /* Unmark HWTCL state for SWTCL. */
        if (!r300->screen->caps.has_tcl) {
            r300->vs_state.dirty = FALSE;
            r300->vs_constants.dirty = FALSE;
        }
    }

    /* reset flushed query */
    foreach(query, &r300->query_list) {
        query->flushed = TRUE;
    }

    /* Create a new fence. */
    if (rfence) {
        *rfence = CALLOC_STRUCT(r300_fence);
        pipe_reference_init(&(*rfence)->reference, 1);
        (*rfence)->ctx = r300;
    }
}

void r300_init_flush_functions(struct r300_context* r300)
{
    r300->context.flush = r300_flush;
}
