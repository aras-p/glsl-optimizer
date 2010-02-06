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

#ifndef R300_TRANSFER
#define R300_TRANSFER

#include "pipe/p_screen.h"
#include "pipe/p_state.h"

struct r300_texture;
struct r300_screen;

struct r300_transfer {
    /* Parent class */
    struct pipe_transfer transfer;

    /* Parameters of get_tex_transfer. */
    unsigned x, y, level, zslice, face;

    /* Offset from start of buffer. */
    unsigned offset;

    /* Untiled texture. */
    struct r300_texture *untiled_texture;

    /* Transfer and format flags. */
    unsigned buffer_usage, render_target_usage;
};

/* Convenience cast wrapper. */
static INLINE struct r300_transfer*
r300_transfer(struct pipe_transfer* transfer)
{
    return (struct r300_transfer*)transfer;
}

void r300_init_screen_transfer_functions(struct pipe_screen *screen);

#endif

