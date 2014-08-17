/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#ifndef ILO_LAYOUT_H
#define ILO_LAYOUT_H

#include "intel_winsys.h"

#include "ilo_common.h"

struct pipe_resource;

enum ilo_layout_walk_type {
   /*
    * Array layers of an LOD are packed together vertically.  This maps to
    * ARYSPC_LOD0 for non-mipmapped 2D textures, and is extended to support
    * mipmapped stencil textures and HiZ on GEN6.
    */
   ILO_LAYOUT_WALK_LOD,

   /*
    * LODs of an array layer are packed together.  This maps to ARYSPC_FULL
    * and is used for mipmapped 2D textures.
    */
   ILO_LAYOUT_WALK_LAYER,

   /*
    * 3D slices of an LOD are packed together, horizontally with wrapping.
    * Used for 3D textures.
    */
   ILO_LAYOUT_WALK_3D,
};

enum ilo_layout_aux_type {
   ILO_LAYOUT_AUX_NONE,
   ILO_LAYOUT_AUX_HIZ,
   ILO_LAYOUT_AUX_MCS,
};

struct ilo_layout_lod {
   /* physical position */
   unsigned x;
   unsigned y;

   /*
    * Physical size of an LOD slice.  There may be multiple slices when the
    * walk type is not ILO_LAYOUT_WALK_LAYER.
    */
   unsigned slice_width;
   unsigned slice_height;
};

/**
 * Texture layout.
 */
struct ilo_layout {
   enum ilo_layout_aux_type aux;

   /* physical width0, height0, and format */
   unsigned width0;
   unsigned height0;
   enum pipe_format format;
   bool separate_stencil;

   /*
    * width, height, and size of pixel blocks, for conversion between 2D
    * coordinates and memory offsets
    */
   unsigned block_width;
   unsigned block_height;
   unsigned block_size;

   enum ilo_layout_walk_type walk;
   bool interleaved_samples;

   /* bitmask of valid tiling modes */
   unsigned valid_tilings;
   enum intel_tiling_mode tiling;

   /* mipmap alignments */
   unsigned align_i;
   unsigned align_j;

   struct ilo_layout_lod lods[PIPE_MAX_TEXTURE_LEVELS];

   /* physical height of layers for ILO_LAYOUT_WALK_LAYER */
   unsigned layer_height;

   /* distance in bytes between two pixel block rows */
   unsigned bo_stride;
   /* number of pixel block rows */
   unsigned bo_height;

   /* bitmask of levels that can use aux */
   unsigned aux_enables;
   unsigned aux_offsets[PIPE_MAX_TEXTURE_LEVELS];
   unsigned aux_stride;
   unsigned aux_height;
};

void ilo_layout_init(struct ilo_layout *layout,
                     const struct ilo_dev_info *dev,
                     const struct pipe_resource *templ);

bool
ilo_layout_update_for_imported_bo(struct ilo_layout *layout,
                                  enum intel_tiling_mode tiling,
                                  unsigned bo_stride);

/**
 * Convert from pixel position to 2D memory offset.
 */
static inline void
ilo_layout_pos_to_mem(const struct ilo_layout *layout,
                      unsigned pos_x, unsigned pos_y,
                      unsigned *mem_x, unsigned *mem_y)
{
   assert(pos_x % layout->block_width == 0);
   assert(pos_y % layout->block_height == 0);

   *mem_x = pos_x / layout->block_width * layout->block_size;
   *mem_y = pos_y / layout->block_height;
}

/**
 * Convert from 2D memory offset to linear offset.
 */
static inline unsigned
ilo_layout_mem_to_linear(const struct ilo_layout *layout,
                         unsigned mem_x, unsigned mem_y)
{
   return mem_y * layout->bo_stride + mem_x;
}

/**
 * Convert from 2D memory offset to raw offset.
 */
static inline unsigned
ilo_layout_mem_to_raw(const struct ilo_layout *layout,
                      unsigned mem_x, unsigned mem_y)
{
   unsigned tile_w, tile_h;

   switch (layout->tiling) {
   case INTEL_TILING_NONE:
      if (layout->format == PIPE_FORMAT_S8_UINT) {
         /* W-tile */
         tile_w = 64;
         tile_h = 64;
      } else {
         tile_w = 1;
         tile_h = 1;
      }
      break;
   case INTEL_TILING_X:
      tile_w = 512;
      tile_h = 8;
      break;
   case INTEL_TILING_Y:
      tile_w = 128;
      tile_h = 32;
      break;
   default:
      assert(!"unknown tiling");
      tile_w = 1;
      tile_h = 1;
      break;
   }

   assert(mem_x % tile_w == 0);
   assert(mem_y % tile_h == 0);

   return mem_y * layout->bo_stride + mem_x * tile_h;
}

/**
 * Return the stride, in bytes, between slices within a level.
 */
static inline unsigned
ilo_layout_get_slice_stride(const struct ilo_layout *layout, unsigned level)
{
   unsigned h;

   switch (layout->walk) {
   case ILO_LAYOUT_WALK_LOD:
      h = layout->lods[level].slice_height;
      break;
   case ILO_LAYOUT_WALK_LAYER:
      h = layout->layer_height;
      break;
   case ILO_LAYOUT_WALK_3D:
      if (level == 0) {
         h = layout->lods[0].slice_height;
         break;
      }
      /* fall through */
   default:
      assert(!"no single stride to walk across slices");
      h = 0;
      break;
   }

   assert(h % layout->block_height == 0);

   return (h / layout->block_height) * layout->bo_stride;
}

/**
 * Return the physical size, in bytes, of a slice in a level.
 */
static inline unsigned
ilo_layout_get_slice_size(const struct ilo_layout *layout, unsigned level)
{
   const unsigned w = layout->lods[level].slice_width;
   const unsigned h = layout->lods[level].slice_height;

   assert(w % layout->block_width == 0);
   assert(h % layout->block_height == 0);

   return (w / layout->block_width * layout->block_size) *
      (h / layout->block_height);
}

/**
 * Return the pixel position of a slice.
 */
static inline void
ilo_layout_get_slice_pos(const struct ilo_layout *layout,
                         unsigned level, unsigned slice,
                         unsigned *x, unsigned *y)
{
   switch (layout->walk) {
   case ILO_LAYOUT_WALK_LOD:
      *x = layout->lods[level].x;
      *y = layout->lods[level].y + layout->lods[level].slice_height * slice;
      break;
   case ILO_LAYOUT_WALK_LAYER:
      *x = layout->lods[level].x;
      *y = layout->lods[level].y + layout->layer_height * slice;
      break;
   case ILO_LAYOUT_WALK_3D:
      {
         /* slices are packed horizontally with wrapping */
         const unsigned sx = slice & ((1 << level) - 1);
         const unsigned sy = slice >> level;

         *x = layout->lods[level].x + layout->lods[level].slice_width * sx;
         *y = layout->lods[level].y + layout->lods[level].slice_height * sy;

         /* should not overlap with the next level */
         if (level + 1 < Elements(layout->lods) &&
             layout->lods[level + 1].y) {
            assert(*y + layout->lods[level].slice_height <=
                  layout->lods[level + 1].y);
         }
         break;
      }
   default:
      assert(!"unknown layout walk type");
      break;
   }

   /* should not exceed the bo size */
   assert(*y + layout->lods[level].slice_height <=
         layout->bo_height * layout->block_height);
}

#endif /* ILO_LAYOUT_H */
