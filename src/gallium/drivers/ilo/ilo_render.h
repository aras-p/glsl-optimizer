/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_RENDER_H
#define ILO_RENDER_H

#include "ilo_common.h"

struct ilo_blitter;
struct ilo_builder;
struct ilo_query;
struct ilo_render;
struct ilo_state_vector;

struct ilo_render *
ilo_render_create(struct ilo_builder *builder);

void
ilo_render_destroy(struct ilo_render *render);

/**
 * Estimate the size of an action.
 */
void
ilo_render_get_sample_position(const struct ilo_render *render,
                               unsigned sample_count,
                               unsigned sample_index,
                               float *x, float *y);

void
ilo_render_invalidate_hw(struct ilo_render *render);

void
ilo_render_invalidate_builder(struct ilo_render *render);

int
ilo_render_get_flush_len(const struct ilo_render *render);

void
ilo_render_emit_flush(struct ilo_render *render);

int
ilo_render_get_query_len(const struct ilo_render *render,
                         unsigned query_type);

void
ilo_render_emit_query(struct ilo_render *render,
                      struct ilo_query *q, uint32_t offset);

int
ilo_render_get_rectlist_len(const struct ilo_render *render,
                            const struct ilo_blitter *blitter);

void
ilo_render_emit_rectlist(struct ilo_render *render,
                         const struct ilo_blitter *blitter);

int
ilo_render_get_draw_len(const struct ilo_render *render,
                        const struct ilo_state_vector *vec);

void
ilo_render_emit_draw(struct ilo_render *render,
                     const struct ilo_state_vector *vec);

#endif /* ILO_RENDER_H */
