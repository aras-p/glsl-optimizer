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
    struct r300_query* q = CALLOC_STRUCT(r300_query);

    q->type = query_type;
    assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

    return (struct pipe_query*)q;
}

static void r300_destroy_query(struct pipe_context* pipe,
                               struct pipe_query* q)
{
    FREE(q);
}

static void r300_begin_query(struct pipe_context* pipe,
                             struct pipe_query* q)
{
    struct r300_context* r300 = r300_context(pipe);
    CS_LOCALS(r300);

    BEGIN_CS(2);
    OUT_CS_REG(R300_ZB_ZPASS_DATA, 0);
    END_CS;
}

static void r300_end_query(struct pipe_context* pipe,
                           struct pipe_query* q)
{
    struct r300_context* r300 = r300_context(pipe);
}

static boolean r300_get_query_result(struct pipe_context* pipe,
                                     struct pipe_query* q,
                                     boolean wait,
                                     uint64_t* result)
{
    *result = 0;
    return TRUE;
}

void r300_init_query_functions(struct r300_context* r300) {
    r300->context.create_query = r300_create_query;
    r300->context.destroy_query = r300_destroy_query;
    r300->context.begin_query = r300_begin_query;
    r300->context.end_query = r300_end_query;
    r300->context.get_query_result = r300_get_query_result;
}
