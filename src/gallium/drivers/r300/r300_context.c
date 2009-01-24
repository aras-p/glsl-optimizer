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

static void r300_destroy_context(struct pipe_context* context) {
    struct r300_context* r300 = r300_context(context);

    draw_destroy(r300->draw);

    FREE(r300->blend_color_state);
    FREE(r300->scissor_state);
    FREE(r300);
}

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct pipe_winsys* winsys,
                                         struct r300_winsys* r300_winsys)
{
    struct r300_context* r300 = CALLOC_STRUCT(r300_context);

    if (!r300)
        return NULL;

    r300->winsys = r300_winsys;
    r300->context.winsys = winsys;
    r300->context.screen = r300_create_screen(winsys, r300_winsys);

    r300->context.destroy = r300_destroy_context;

    r300->draw = draw_create();

    r300->blend_color_state = CALLOC_STRUCT(r300_blend_color_state);
    r300->scissor_state = CALLOC_STRUCT(r300_scissor_state);

    r300_init_flush_functions(r300);

    r300_init_surface_functions(r300);

    r300_init_state_functions(r300);

    r300->dirty_state = R300_NEW_KITCHEN_SINK;
    r300->dirty_hw++;

    return &r300->context;
}
