/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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

#ifndef ILO_DRAW_H
#define ILO_DRAW_H

#include "ilo_common.h"
#include "ilo_cp.h"

struct ilo_3d_pipeline;
struct ilo_blitter;
struct ilo_context;
struct ilo_query;

/**
 * 3D context.
 */
struct ilo_3d {
   struct ilo_cp *cp;
   struct ilo_cp_owner owner;

   bool new_batch;

   struct list_head queries;

   struct ilo_3d_pipeline *pipeline;
};

struct ilo_3d *
ilo_3d_create(struct ilo_cp *cp, const struct ilo_dev_info *dev);

void
ilo_3d_destroy(struct ilo_3d *hw3d);

void
ilo_3d_cp_submitted(struct ilo_3d *hw3d);

void
ilo_3d_draw_rectlist(struct ilo_3d *hw3d, const struct ilo_blitter *blitter);

bool
ilo_3d_init_query(struct pipe_context *pipe, struct ilo_query *q);

void
ilo_3d_begin_query(struct pipe_context *pipe, struct ilo_query *q);

void
ilo_3d_end_query(struct pipe_context *pipe, struct ilo_query *q);

void
ilo_3d_process_query(struct pipe_context *pipe, struct ilo_query *q);

void
ilo_init_3d_functions(struct ilo_context *ilo);

#endif /* ILO_DRAW_H */
