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

#ifndef ILO_BLITTER_H
#define ILO_BLITTER_H

#include "ilo_common.h"

struct ilo_context;
struct blitter_context;

struct ilo_blitter {
   struct ilo_context *ilo;

   struct blitter_context *pipe_blitter;
};

struct ilo_blitter *
ilo_blitter_create(struct ilo_context *ilo);

void
ilo_blitter_destroy(struct ilo_blitter *blitter);

bool
ilo_blitter_pipe_blit(struct ilo_blitter *blitter,
                      const struct pipe_blit_info *info);

bool
ilo_blitter_pipe_copy_resource(struct ilo_blitter *blitter,
                               struct pipe_resource *dst, unsigned dst_level,
                               unsigned dst_x, unsigned dst_y, unsigned dst_z,
                               struct pipe_resource *src, unsigned src_level,
                               const struct pipe_box *src_box);

bool
ilo_blitter_pipe_clear_rt(struct ilo_blitter *blitter,
                          struct pipe_surface *rt,
                          const union pipe_color_union *color,
                          unsigned x, unsigned y,
                          unsigned width, unsigned height);

bool
ilo_blitter_pipe_clear_zs(struct ilo_blitter *blitter,
                          struct pipe_surface *zs,
                          unsigned clear_flags,
                          double depth, unsigned stencil,
                          unsigned x, unsigned y,
                          unsigned width, unsigned height);

bool
ilo_blitter_pipe_clear_fb(struct ilo_blitter *blitter,
                          unsigned buffers,
                          const union pipe_color_union *color,
                          double depth, unsigned stencil);

bool
ilo_blitter_blt_copy_resource(struct ilo_blitter *blitter,
                              struct pipe_resource *dst, unsigned dst_level,
                              unsigned dst_x, unsigned dst_y, unsigned dst_z,
                              struct pipe_resource *src, unsigned src_level,
                              const struct pipe_box *src_box);

bool
ilo_blitter_blt_clear_rt(struct ilo_blitter *blitter,
                         struct pipe_surface *rt,
                         const union pipe_color_union *color,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height);

bool
ilo_blitter_blt_clear_zs(struct ilo_blitter *blitter,
                         struct pipe_surface *zs,
                         unsigned clear_flags,
                         double depth, unsigned stencil,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height);

#endif /* ILO_BLITTER_H */
