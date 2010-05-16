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
    struct pipe_framebuffer_state *fb;
    unsigned i;

    CS_LOCALS(r300);
    (void) cs_count;
    /* We probably need to flush Draw, but we may have been called from
     * within Draw. This feels kludgy, but it might be the best thing.
     *
     * Of course, the best thing is to kill Draw with fire. :3 */
    if (r300->draw && !r300->draw->flushing) {
        draw_flush(r300->draw);
    }

    if (r300->dirty_hw) {
        r300_emit_query_end(r300);

        FLUSH_CS;
        r300->dirty_hw = 0;

        /* New kitchen sink, baby. */
        foreach(atom, &r300->atom_list) {
            if (atom->state || atom->allow_null_state) {
                atom->dirty = TRUE;
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

    /* XXX
     *
     * This is a preliminary implementation of glFinish. Note that st/mesa
     * uses a non-null fence when glFinish is called and then waits for
     * the fence. Instead of returning the actual fence, we do the sync
     * directly.
     *
     * The ideal implementation should use something like EmitIrqLocked and
     * WaitIrq, or better, real fences.
     *
     * This feature degrades performance to the level of r300c for games that
     * use glFinish a lot, even openarena does. Ideally we wouldn't need
     * glFinish at all if we had proper throttling in swapbuffers so that
     * the CPU wouldn't outrun the GPU by several frames, so this is basically
     * a temporary fix for the input lag. Once swap&sync works with DRI2,
     * I'll be happy to remove this code.
     *
     * - M. */
    if (fence && r300->fb_state.state) {
        fb = r300->fb_state.state;

        for (i = 0; i < fb->nr_cbufs; i++) {
            if (fb->cbufs[i]->texture) {
                r300->rws->buffer_wait(r300->rws,
                    r300_texture(fb->cbufs[i]->texture)->buffer);
            }
            if (fb->zsbuf) {
                r300->rws->buffer_wait(r300->rws,
                    r300_texture(fb->zsbuf->texture)->buffer);
            }
        }
    }
}

void r300_init_flush_functions(struct r300_context* r300)
{
    r300->context.flush = r300_flush;
}
