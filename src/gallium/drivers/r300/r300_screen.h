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

#ifndef R300_SCREEN_H
#define R300_SCREEN_H

#include "pipe/p_screen.h"

#include "r300_chipset.h"

struct r300_screen {
    /* Parent class */
    struct pipe_screen screen;

    /* Chipset capabilities */
    struct r300_capabilities* caps;
};

struct r300_transfer {
    /* Parent class */
    struct pipe_transfer transfer;

    /* Offset from start of buffer. */
    unsigned offset;
};

/* Convenience cast wrapper. */
static INLINE struct r300_screen* r300_screen(struct pipe_screen* screen) {
    return (struct r300_screen*)screen;
}

/* Convenience cast wrapper. */
static INLINE struct r300_transfer*
r300_transfer(struct pipe_transfer* transfer)
{
    return (struct r300_transfer*)transfer;
}

/* Creates a new r300 screen. */
struct pipe_screen* r300_create_screen(struct r300_winsys* r300_winsys);

#endif /* R300_SCREEN_H */
