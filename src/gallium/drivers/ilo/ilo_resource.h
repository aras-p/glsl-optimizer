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

#ifndef ILO_RESOURCE_H
#define ILO_RESOURCE_H

#include "intel_winsys.h"

#include "ilo_common.h"

struct ilo_screen;

struct ilo_buffer {
   struct pipe_resource base;

   struct intel_bo *bo;
   unsigned bo_size;
   unsigned bo_flags;
};

struct ilo_texture {
   struct pipe_resource base;

   bool imported;
   unsigned bo_flags;

   enum pipe_format bo_format;
   struct intel_bo *bo;

   /*
    * These are the values passed to or returned from winsys for bo
    * allocation.  As such,
    *
    *  - width and height are in blocks,
    *  - cpp is the block size in bytes, and
    *  - stride is the distance in bytes between two block rows.
    */
   int bo_width, bo_height, bo_cpp, bo_stride;
   enum intel_tiling_mode tiling;

   bool compressed;
   unsigned block_width;
   unsigned block_height;

   /* true if the mip level alignments are stricter */
   bool halign_8, valign_4;
   /* true if space is reserved between layers */
   bool array_spacing_full;
   /* true if samples are interleaved */
   bool interleaved;

   /* 2D offsets into a layer/slice/face */
   struct ilo_texture_slice {
      unsigned x;
      unsigned y;
   } *slice_offsets[PIPE_MAX_TEXTURE_LEVELS];

   struct ilo_texture *separate_s8;
};

static inline struct ilo_buffer *
ilo_buffer(struct pipe_resource *res)
{
   return (struct ilo_buffer *)
      ((res && res->target == PIPE_BUFFER) ? res : NULL);
}

static inline struct ilo_texture *
ilo_texture(struct pipe_resource *res)
{
   return (struct ilo_texture *)
      ((res && res->target != PIPE_BUFFER) ? res : NULL);
}

void
ilo_init_resource_functions(struct ilo_screen *is);

bool
ilo_buffer_alloc_bo(struct ilo_buffer *buf);

bool
ilo_texture_alloc_bo(struct ilo_texture *tex);

unsigned
ilo_texture_get_slice_offset(const struct ilo_texture *tex,
                             int level, int slice,
                             unsigned *x_offset, unsigned *y_offset);

#endif /* ILO_RESOURCE_H */
