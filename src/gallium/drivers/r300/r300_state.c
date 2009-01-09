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

#include "r300_context.h"

static void* r300_create_vs_state(struct pipe_context* pipe,
                                  struct pipe_shader_state* state)
{
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    return draw_create_vertex_shader(context->draw, state);
}

static void r300_bind_vs_state(struct pipe_context* pipe, void* state) {
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    draw_bind_vertex_shader(context->draw, (struct draw_vertex_shader*)state);
}

static void r300_delete_vs_state(struct pipe_context* pipe, void* state)
{
    struct r300_context* context = r300_context(pipe);
    /* XXX handing this off to Draw for now */
    draw_delete_vertex_shader(context->draw, (struct draw_vertex_shader*)state);
}