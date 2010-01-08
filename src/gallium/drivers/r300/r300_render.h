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

#ifndef R300_RENDER_H
#define R300_RENDER_H

uint32_t r300_translate_primitive(unsigned prim);

void r300_draw_range_elements(struct pipe_context* pipe,
                              struct pipe_buffer* indexBuffer,
                              unsigned indexSize,
                              unsigned minIndex,
                              unsigned maxIndex,
                              unsigned mode,
                              unsigned start,
                              unsigned count);

void r300_draw_elements(struct pipe_context* pipe,
                        struct pipe_buffer* indexBuffer,
                        unsigned indexSize, unsigned mode,
                        unsigned start, unsigned count);

void r300_draw_arrays(struct pipe_context* pipe, unsigned mode,
                      unsigned start, unsigned count);

void r300_swtcl_draw_arrays(struct pipe_context* pipe,
                            unsigned mode,
                            unsigned start,
                            unsigned count);

void r300_swtcl_draw_range_elements(struct pipe_context* pipe,
                                    struct pipe_buffer* indexBuffer,
                                    unsigned indexSize,
                                    unsigned minIndex,
                                    unsigned maxIndex,
                                    unsigned mode,
                                    unsigned start,
                                    unsigned count);

#endif /* R300_RENDER_H */
