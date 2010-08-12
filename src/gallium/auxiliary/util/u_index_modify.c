/*
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

#include "pipe/p_context.h"
#include "util/u_index_modify.h"
#include "util/u_inlines.h"

void util_shorten_ubyte_elts(struct pipe_context *context,
			     struct pipe_resource **elts,
			     int index_bias,
			     unsigned start,
			     unsigned count)
{
    struct pipe_screen* screen = context->screen;
    struct pipe_resource* new_elts;
    unsigned char *in_map;
    unsigned short *out_map;
    struct pipe_transfer *src_transfer, *dst_transfer;
    unsigned i;

    new_elts = pipe_buffer_create(screen,
                                  PIPE_BIND_INDEX_BUFFER,
                                  2 * count);

    in_map = pipe_buffer_map(context, *elts, PIPE_TRANSFER_READ, &src_transfer);
    out_map = pipe_buffer_map(context, new_elts, PIPE_TRANSFER_WRITE, &dst_transfer);

    in_map += start;

    for (i = 0; i < count; i++) {
        *out_map = (unsigned short)(*in_map + index_bias);
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, *elts, src_transfer);
    pipe_buffer_unmap(context, new_elts, dst_transfer);

    *elts = new_elts;
}

void util_rebuild_ushort_elts(struct pipe_context *context,
			      struct pipe_resource **elts,
			      int index_bias,
			      unsigned start, unsigned count)
{
    struct pipe_transfer *in_transfer = NULL;
    struct pipe_transfer *out_transfer = NULL;
    struct pipe_resource *new_elts;
    unsigned short *in_map;
    unsigned short *out_map;
    unsigned i;

    new_elts = pipe_buffer_create(context->screen,
                                  PIPE_BIND_INDEX_BUFFER,
                                  2 * count);

    in_map = pipe_buffer_map(context, *elts,
                             PIPE_TRANSFER_READ, &in_transfer);
    out_map = pipe_buffer_map(context, new_elts,
                              PIPE_TRANSFER_WRITE, &out_transfer);

    in_map += start;
    for (i = 0; i < count; i++) {
        *out_map = (unsigned short)(*in_map + index_bias);
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, *elts, in_transfer);
    pipe_buffer_unmap(context, new_elts, out_transfer);

    *elts = new_elts;
}

void util_rebuild_uint_elts(struct pipe_context *context,
			    struct pipe_resource **elts,
			    int index_bias,
			    unsigned start, unsigned count)
{
    struct pipe_transfer *in_transfer = NULL;
    struct pipe_transfer *out_transfer = NULL;
    struct pipe_resource *new_elts;
    unsigned int *in_map;
    unsigned int *out_map;
    unsigned i;

    new_elts = pipe_buffer_create(context->screen,
                                  PIPE_BIND_INDEX_BUFFER,
                                  2 * count);

    in_map = pipe_buffer_map(context, *elts,
                             PIPE_TRANSFER_READ, &in_transfer);
    out_map = pipe_buffer_map(context, new_elts,
                              PIPE_TRANSFER_WRITE, &out_transfer);

    in_map += start;
    for (i = 0; i < count; i++) {
        *out_map = (unsigned int)(*in_map + index_bias);
        in_map++;
        out_map++;
    }

    pipe_buffer_unmap(context, *elts, in_transfer);
    pipe_buffer_unmap(context, new_elts, out_transfer);

    *elts = new_elts;
}
