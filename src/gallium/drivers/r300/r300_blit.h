/*
 * Copyright 2008 Marek Olšák <maraeo@gmail.com>
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

#ifndef R300_BLIT_H
#define R300_BLIT_H

struct pipe_context;
struct pipe_resource;
struct pipe_subresource;

void r300_clear(struct pipe_context* pipe,
                unsigned buffers,
                const float* rgba,
                double depth,
                unsigned stencil);

void r300_resource_copy_region(struct pipe_context *pipe,
                               struct pipe_resource *dst,
                               struct pipe_subresource subdst,
                               unsigned dstx, unsigned dsty, unsigned dstz,
                               struct pipe_resource *src,
                               struct pipe_subresource subsrc,
                               unsigned srcx, unsigned srcy, unsigned srcz,
                               unsigned width, unsigned height);

void r300_resource_fill_region(struct pipe_context* pipe,
                               struct pipe_resource* dst,
                               struct pipe_subresource subdst,
                               unsigned dstx, unsigned dsty, unsigned dstz,
                               unsigned width, unsigned height,
                               unsigned value);

#endif /* R300_BLIT_H */
