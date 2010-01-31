/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_emit.h"
#include "r300_flush.h"

static void r300_flush(struct pipe_context* pipe,
                       unsigned flags,
                       struct pipe_fence_handle** fence)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_query *query;
    struct r300_atom *atom;

    CS_LOCALS(r300);
    (void) cs_count;
    /* We probably need to flush Draw, but we may have been called from
     * within Draw. This feels kludgy, but it might be the best thing.
     *
     * Of course, the best thing is to kill Draw with fire. :3 */
    if (r300->draw && !r300->draw->flushing) {
        draw_flush(r300->draw);
    }

    r300_emit_query_end(r300);

    if (r300->dirty_hw) {
        FLUSH_CS;
        r300->dirty_state = R300_NEW_KITCHEN_SINK;
        r300->dirty_hw = 0;

        /* New kitchen sink, baby. */
        foreach(atom, &r300->atom_list) {
            if (atom->state) {
                atom->dirty = TRUE;
            }
        }
    }

    /* reset flushed query */
    foreach(query, &r300->query_list) {
        query->flushed = TRUE;
    }
}


void r300_init_flush_functions(struct r300_context* r300)
{
    r300->context.flush = r300_flush;
}
