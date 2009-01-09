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

#ifndef R300_CONTEXT_H
#define R300_CONTEXT_H

#include "pipe/p_context.h"

struct r300_context {
    /* Parent class */
    struct pipe_context context;

    struct amd_winsys* winsys;
    struct draw_context* draw;

    /* CS object. This is very much like Intel's batchbuffer.
     * Fill it full of dwords and relocs and then submit.
     * Repeat as needed. */
    /* Note: Unlike Mesa's version of this, we don't keep a copy of the CSM
     * that was used to create this CS. Is this a good idea? */
    struct radeon_cs* cs;
};

/* Convenience cast wrapper. */
static struct r300_context* r300_context(struct pipe_context* context) {
    return (struct r300_context*)context;
}

#endif /* R300_CONTEXT_H */