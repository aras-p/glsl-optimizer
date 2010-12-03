/*
 * Copyright 2010 Red Hat Inc.
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Dave Airlie
 */

#ifndef R300_SCREEN_BUFFER_H
#define R300_SCREEN_BUFFER_H

#include <stdio.h>
#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_transfer.h"

#include "r300_screen.h"
#include "r300_winsys.h"
#include "r300_context.h"

#define R300_BUFFER_MAGIC 0xabcd1234
#define R300_BUFFER_MAX_RANGES 32

struct r300_buffer_range {
    uint32_t start;
    uint32_t end;
};

/* Vertex buffer. */
struct r300_buffer
{
    struct u_resource b;

    uint32_t magic;

    struct r300_winsys_buffer *buf;
    struct r300_winsys_cs_buffer *cs_buf;

    enum r300_buffer_domain domain;

    uint8_t *user_buffer;
    uint8_t *constant_buffer;
    struct r300_buffer_range ranges[R300_BUFFER_MAX_RANGES];
    unsigned num_ranges;
};

/* Functions. */

int r300_upload_user_buffers(struct r300_context *r300);

int r300_upload_index_buffer(struct r300_context *r300,
			     struct pipe_resource **index_buffer,
			     unsigned index_size,
			     unsigned start,
			     unsigned count, unsigned *out_offset);

struct pipe_resource *r300_buffer_create(struct pipe_screen *screen,
					 const struct pipe_resource *templ);

struct pipe_resource *r300_user_buffer_create(struct pipe_screen *screen,
					      void *ptr,
					      unsigned bytes,
					      unsigned usage);

unsigned r300_buffer_is_referenced(struct pipe_context *context,
				   struct pipe_resource *buf,
                                   enum r300_reference_domain domain);

/* Inline functions. */

static INLINE struct r300_buffer *r300_buffer(struct pipe_resource *buffer)
{
    return (struct r300_buffer *)buffer;
}

static INLINE boolean r300_buffer_is_user_buffer(struct pipe_resource *buffer)
{
    return r300_buffer(buffer)->user_buffer ? true : false;
}

#endif
