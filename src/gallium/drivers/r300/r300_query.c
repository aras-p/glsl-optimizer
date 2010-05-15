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

#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "r300_context.h"
#include "r300_screen.h"
#include "r300_emit.h"
#include "r300_query.h"

#include <stdio.h>

static struct pipe_query *r300_create_query(struct pipe_context *pipe,
                                            unsigned query_type)
{
    struct r300_context *r300 = r300_context(pipe);
    struct r300_screen *r300screen = r300->screen;
    unsigned query_size;
    struct r300_query *q, *qptr;

    q = CALLOC_STRUCT(r300_query);

    q->type = query_type;
    assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

    q->active = FALSE;

    if (r300screen->caps.family == CHIP_FAMILY_RV530)
        query_size = r300screen->caps.num_z_pipes * sizeof(uint32_t);
    else
        query_size = r300screen->caps.num_frag_pipes * sizeof(uint32_t);

    if (!is_empty_list(&r300->query_list)) {
        qptr = last_elem(&r300->query_list);
        q->offset = qptr->offset + query_size;
    }
    insert_at_tail(&r300->query_list, q);

    /* XXX */
    if (q->offset >= 4096) {
        q->offset = 0;
        fprintf(stderr, "r300: Rewinding OQBO...\n");
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
    uint32_t value = ~0U;
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query* q = (struct r300_query*)query;

    if (r300->query_current != NULL) {
        fprintf(stderr, "r300: begin_query: "
                "Some other query has already been started.\n");
        assert(0);
        return;
    }

    pipe_buffer_write(pipe,
		      r300->oqbo,
		      q->offset,
		      sizeof value,
		      &value);

    q->flushed = FALSE;
    r300->query_current = q;
    r300->query_start.dirty = TRUE;
}

static void r300_end_query(struct pipe_context* pipe,
	                   struct pipe_query* query)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_query* q = (struct r300_query*)query;

    if ((struct r300_query*)query != r300->query_current) {
        fprintf(stderr, "r300: end_query: Got invalid query.\n");
        assert(0);
        return;
    }

    r300_emit_query_end(r300);
    q->begin_emitted = false;
    r300->query_current = NULL;
}

static boolean r300_get_query_result(struct pipe_context* pipe,
                                     struct pipe_query* query,
                                     boolean wait,
                                     uint64_t* result)
{
    struct r300_context* r300 = r300_context(pipe);
    struct r300_screen* r300screen = r300->screen;
    struct r300_query *q = (struct r300_query*)query;
    struct pipe_transfer *transfer;
    unsigned flags = PIPE_TRANSFER_READ;
    uint32_t* map;
    uint32_t temp = 0;
    unsigned i, num_results;

    if (q->flushed == FALSE)
        pipe->flush(pipe, 0, NULL);
    if (!wait) {
        flags |= PIPE_TRANSFER_DONTBLOCK;
    }

    map = pipe_buffer_map(pipe, r300->oqbo, flags, &transfer);
    if (!map)
        return FALSE;
    map += q->offset / 4;

    if (r300screen->caps.family == CHIP_FAMILY_RV530)
        num_results = r300screen->caps.num_z_pipes;
    else
        num_results = r300screen->caps.num_frag_pipes;

    for (i = 0; i < num_results; i++) {
        if (*map == ~0U) {
            /* Looks like our results aren't ready yet. */
            if (wait) {
                fprintf(stderr, "r300: Despite waiting, OQ results haven't "
                                "come in yet.\n");
            }
            temp = ~0U;
            break;
        }
        temp += *map;
        map++;
    }
    pipe_buffer_unmap(pipe, r300->oqbo, transfer);

    if (temp == ~0U) {
        /* Our results haven't been written yet... */
        return FALSE;
    }

    *result = temp;
    return TRUE;
}

static void r300_render_condition(struct pipe_context *pipe,
                                  struct pipe_query *query,
                                  uint mode)
{
    struct r300_context *r300 = r300_context(pipe);
    uint64_t result;
    boolean wait;

    if (query) {
        wait = mode == PIPE_RENDER_COND_WAIT ||
               mode == PIPE_RENDER_COND_BY_REGION_WAIT;

        if (!r300_get_query_result(pipe, query, wait, &result)) {
            r300->skip_rendering = FALSE;
        }

        r300->skip_rendering = result == 0;
    } else {
        r300->skip_rendering = FALSE;
    }
}

void r300_init_query_functions(struct r300_context* r300) {
    r300->context.create_query = r300_create_query;
    r300->context.destroy_query = r300_destroy_query;
    r300->context.begin_query = r300_begin_query;
    r300->context.end_query = r300_end_query;
    r300->context.get_query_result = r300_get_query_result;
    r300->context.render_condition = r300_render_condition;
}
