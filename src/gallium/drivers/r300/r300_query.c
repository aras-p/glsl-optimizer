/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_query.h"

static struct pipe_query* r300_create_query(struct pipe_context* pipe,
                                            unsigned query_type)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    unsigned query_size = r300screen->caps->num_frag_pipes * 4;
    struct r300_query* q, * qptr;

    q = CALLOC_STRUCT(r300_query);

    q->type = query_type;
    assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

    q->active = FALSE;

    if (!r300->query_list) {
        r300->query_list = q;
    } else if (!is_empty_list(r300->query_list)) {
        qptr = last_elem(r300->query_list);
        q->offset = qptr->offset + query_size;
        insert_at_tail(r300->query_list, q);
    }

    /* XXX */
    if (q->offset >= 4096) {
        q->offset = 0;
    }

    return (struct pipe_query*)q;
}

static void r300_destroy_query(struct pipe_context* pipe,
                               struct pipe_query* query)
{
    struct r300_query* q = (struct r300_query*)query;

    remove_from_list(q);
    FREE(query);
}

static void r300_begin_query(struct pipe_context* pipe,
                             struct pipe_query* query)
{
    uint32_t* map;
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query* q = (struct r300_query*)query;

    map = pipe->screen->buffer_map(pipe->screen, r300->oqbo,
            PIPE_BUFFER_USAGE_CPU_WRITE);
    map += q->offset / 4;
    *map = ~0;
    pipe->screen->buffer_unmap(pipe->screen, r300->oqbo);

    r300_emit_dirty_state(r300);
    r300_emit_query_begin(r300, q);
}

static void r300_end_query(struct pipe_context* pipe,
                           struct pipe_query* query)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query* q = (struct r300_query*)query;

    r300_emit_dirty_state(r300);
    r300_emit_query_end(r300, q);
}

static boolean r300_get_query_result(struct pipe_context* pipe,
                                     struct pipe_query* query,
                                     boolean wait,
                                     uint64_t* result)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct r300_query* q = (struct r300_query*)query;
    unsigned flags = PIPE_BUFFER_USAGE_CPU_READ;
    uint32_t* map;
    uint32_t temp;
    unsigned i;

    if (wait) {
        pipe->flush(pipe, 0, NULL);
    } else {
        flags |= PIPE_BUFFER_USAGE_DONTBLOCK;
    }

    map = pipe->screen->buffer_map(pipe->screen, r300->oqbo, flags);
    map += q->offset / 4;
    for (i = 0; i < r300screen->caps->num_frag_pipes; i++) {
        if (*map == ~0) {
            /* Looks like our results aren't ready yet. */
            if (wait) {
                debug_printf("r300: Despite waiting, OQ results haven't"
                        " come in yet.\n");
            }
            temp = ~0;
            break;
        }
        temp += *map;
        map++;
    }
    pipe->screen->buffer_unmap(pipe->screen, r300->oqbo);

    if (temp == ~0) {
        /* Our results haven't been written yet... */
        return FALSE;
    }

    *result = temp;
    return TRUE;
}

void r300_init_query_functions(struct r300_context* r300) {
    r300->context.create_query = r300_create_query;
    r300->context.destroy_query = r300_destroy_query;
    r300->context.begin_query = r300_begin_query;
    r300->context.end_query = r300_end_query;
    r300->context.get_query_result = r300_get_query_result;
}
