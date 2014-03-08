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
#include "ilo_screen.h"

enum ilo_texture_flags {
   /*
    * Possible writers of a texture.  There can be at most one writer at any
    * time.
    *
    * Wine set in resolve flags (in ilo_blit_resolve_slices()), they indicate
    * the new writer.  When set in slice flags (ilo_texture_slice::flags),
    * they indicate the writer since last resolve.
    */
   ILO_TEXTURE_RENDER_WRITE   = 1 << 0,
   ILO_TEXTURE_BLT_WRITE      = 1 << 1,
   ILO_TEXTURE_CPU_WRITE      = 1 << 2,

   /*
    * Possible readers of a texture.  There may be multiple readers at any
    * time.
    *
    * When set in resolve flags, they indicate the new readers.  They are
    * never set in slice flags.
    */
   ILO_TEXTURE_RENDER_READ    = 1 << 3,
   ILO_TEXTURE_BLT_READ       = 1 << 4,
   ILO_TEXTURE_CPU_READ       = 1 << 5,

   /*
    * Set when the texture is cleared.
    *
    * When set in resolve flags, the new writer will clear.  When set in slice
    * flags, the slice has been cleared to ilo_texture_slice::clear_value.
    */
   ILO_TEXTURE_CLEAR          = 1 << 6,

   /*
    * Set when HiZ can be enabled.
    *
    * It is never set in resolve flags.  When set in slice flags, the slice
    * can have HiZ enabled.  It is to be noted that this bit is always set for
    * either all or none of the slices in a level, allowing quick check in
    * case of layered rendering.
    */
   ILO_TEXTURE_HIZ            = 1 << 7,
};

struct ilo_buffer {
   struct pipe_resource base;

   struct intel_bo *bo;
   unsigned bo_size;
};

/**
 * A 3D image slice, cube face, or array layer.
 */
struct ilo_texture_slice {
   /* 2D offset to the slice */
   unsigned x, y;
   unsigned flags;

   /*
    * Slice clear value.  It is served for two purposes
    *
    *  - the clear value used in commands such as 3DSTATE_CLEAR_PARAMS
    *  - the clear value when ILO_TEXTURE_CLEAR is set
    *
    * Since commands such as 3DSTATE_CLEAR_PARAMS expect a single clear value
    * for all slices, ilo_blit_resolve_slices() will silently make all slices
    * to have the same clear value.
    */
   uint32_t clear_value;
};

struct ilo_texture {
   struct pipe_resource base;

   bool imported;

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

   struct ilo_texture_slice *slices[PIPE_MAX_TEXTURE_LEVELS];

   struct ilo_texture *separate_s8;

   struct {
      struct intel_bo *bo;
      int bo_stride;
   } hiz;
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

static inline struct ilo_texture_slice *
ilo_texture_get_slice(const struct ilo_texture *tex,
                      unsigned level, unsigned slice)
{
   assert(level <= tex->base.last_level);
   assert(slice < ((tex->base.target == PIPE_TEXTURE_3D) ?
         u_minify(tex->base.depth0, level) : tex->base.array_size));

   return &tex->slices[level][slice];
}

unsigned
ilo_texture_get_slice_offset(const struct ilo_texture *tex,
                             unsigned level, unsigned slice,
                             unsigned *x_offset, unsigned *y_offset);

static inline void
ilo_texture_set_slice_flags(struct ilo_texture *tex, unsigned level,
                            unsigned first_slice, unsigned num_slices,
                            unsigned mask, unsigned value)
{
   const struct ilo_texture_slice *last =
      ilo_texture_get_slice(tex, level, first_slice + num_slices - 1);
   struct ilo_texture_slice *slice =
      ilo_texture_get_slice(tex, level, first_slice);

   while (slice <= last) {
      slice->flags = (slice->flags & ~mask) | (value & mask);
      slice++;
   }
}

static inline void
ilo_texture_set_slice_clear_value(struct ilo_texture *tex, unsigned level,
                                  unsigned first_slice, unsigned num_slices,
                                  uint32_t clear_value)
{
   const struct ilo_texture_slice *last =
      ilo_texture_get_slice(tex, level, first_slice + num_slices - 1);
   struct ilo_texture_slice *slice =
      ilo_texture_get_slice(tex, level, first_slice);

   while (slice <= last) {
      slice->clear_value = clear_value;
      slice++;
   }
}

static inline bool
ilo_texture_can_enable_hiz(const struct ilo_texture *tex, unsigned level,
                           unsigned first_slice, unsigned num_slices)
{
   const struct ilo_screen *is = ilo_screen(tex->base.screen);
   const struct ilo_texture_slice *slice =
      ilo_texture_get_slice(tex, level, first_slice);

   if (!tex->hiz.bo)
      return false;

   /* we can adjust 3DSTATE_DEPTH_BUFFER for the first slice */
   if (level == 0 && first_slice == 0 && num_slices == 1)
      return true;

   /* HiZ is non-mipmapped and non-array on GEN6 */
   assert(is->dev.gen > ILO_GEN(6));

   /*
    * Either all or none of the slices in the same level have ILO_TEXTURE_HIZ
    * set.  It suffices to check only the first slice.
    */
   return (slice->flags & ILO_TEXTURE_HIZ);
}

#endif /* ILO_RESOURCE_H */
