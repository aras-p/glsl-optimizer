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

#include "util/u_surface.h"
#include "util/u_transfer.h"
#include "util/u_format_etc.h"

#include "ilo_blit.h"
#include "ilo_cp.h"
#include "ilo_context.h"
#include "ilo_resource.h"
#include "ilo_state.h"
#include "ilo_transfer.h"

static bool
is_bo_busy(struct ilo_context *ilo, struct intel_bo *bo, bool *need_flush)
{
   const bool referenced = intel_bo_has_reloc(ilo->cp->bo, bo);

   if (need_flush)
      *need_flush = referenced;

   if (referenced)
      return true;

   return intel_bo_is_busy(bo);
}

static void *
map_bo_for_transfer(struct ilo_context *ilo, struct intel_bo *bo,
                    const struct ilo_transfer *xfer)
{
   void *ptr;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
      ptr = intel_bo_map(bo, (xfer->base.usage & PIPE_TRANSFER_WRITE));
      break;
   case ILO_TRANSFER_MAP_GTT:
      ptr = intel_bo_map_gtt(bo);
      break;
   case ILO_TRANSFER_MAP_UNSYNC:
      ptr = intel_bo_map_unsynchronized(bo);
      break;
   default:
      assert(!"unknown mapping method");
      ptr = NULL;
      break;
   }

   return ptr;
}

/**
 * Choose the best mapping method, depending on the transfer usage and whether
 * the bo is busy.
 */
static bool
choose_transfer_method(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct pipe_resource *res = xfer->base.resource;
   const unsigned usage = xfer->base.usage;
   /* prefer map() when there is the last-level cache */
   const bool prefer_cpu =
      (ilo->dev->has_llc || (usage & PIPE_TRANSFER_READ));
   struct ilo_texture *tex;
   struct ilo_buffer *buf;
   struct intel_bo *bo;
   bool tiled, need_flush;

   if (res->target == PIPE_BUFFER) {
      tex = NULL;

      buf = ilo_buffer(res);
      bo = buf->bo;
      tiled = false;
   }
   else {
      buf = NULL;

      tex = ilo_texture(res);
      bo = tex->bo;
      tiled = (tex->tiling != INTEL_TILING_NONE);
   }

   /* choose between mapping through CPU or GTT */
   if (usage & PIPE_TRANSFER_MAP_DIRECTLY) {
      /* we do not want fencing */
      if (tiled || prefer_cpu)
         xfer->method = ILO_TRANSFER_MAP_CPU;
      else
         xfer->method = ILO_TRANSFER_MAP_GTT;
   }
   else {
      if (!tiled && prefer_cpu)
         xfer->method = ILO_TRANSFER_MAP_CPU;
      else
         xfer->method = ILO_TRANSFER_MAP_GTT;
   }

   /* see if we can avoid stalling */
   if (is_bo_busy(ilo, bo, &need_flush)) {
      bool will_stall = true;

      if (usage & PIPE_TRANSFER_MAP_DIRECTLY) {
         /* nothing we can do */
      }
      else if (usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
         /* unsynchronized gtt mapping does not stall */
         xfer->method = ILO_TRANSFER_MAP_UNSYNC;
         will_stall = false;
      }
      else if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) {
         /* discard old bo and allocate a new one for mapping */
         if ((tex && ilo_texture_alloc_bo(tex)) ||
             (buf && ilo_buffer_alloc_bo(buf))) {
            ilo_mark_states_with_resource_dirty(ilo, res);
            will_stall = false;
         }
      }
      else if (usage & PIPE_TRANSFER_FLUSH_EXPLICIT) {
         /*
          * We could allocate and return a system buffer here.  When a region of
          * the buffer is explicitly flushed, we pwrite() the region to a
          * temporary bo and emit pipelined copy blit.
          *
          * For now, do nothing.
          */
      }
      else if (usage & PIPE_TRANSFER_DISCARD_RANGE) {
         /*
          * We could allocate a temporary bo for mapping, and emit pipelined copy
          * blit upon unmapping.
          *
          * For now, do nothing.
          */
      }

      if (will_stall) {
         if (usage & PIPE_TRANSFER_DONTBLOCK)
            return false;

         /* flush to make bo busy (so that map() stalls as it should be) */
         if (need_flush)
            ilo_cp_flush(ilo->cp, "syncing for transfers");
      }
   }

   if (tex && !(usage & PIPE_TRANSFER_MAP_DIRECTLY)) {
      if (tex->separate_s8 || tex->bo_format == PIPE_FORMAT_S8_UINT)
         xfer->method = ILO_TRANSFER_MAP_SW_ZS;
      /* need to convert on-the-fly */
      else if (tex->bo_format != tex->base.format)
         xfer->method = ILO_TRANSFER_MAP_SW_CONVERT;
   }

   return true;
}

static void
tex_get_box_origin(const struct ilo_texture *tex,
                   unsigned level, unsigned slice,
                   const struct pipe_box *box,
                   unsigned *mem_x, unsigned *mem_y)
{
   const struct ilo_texture_slice *s =
      ilo_texture_get_slice(tex, level, slice + box->z);
   unsigned x, y;

   x = s->x + box->x;
   y = s->y + box->y;

   assert(x % tex->block_width == 0 && y % tex->block_height == 0);

   *mem_x = x / tex->block_width * tex->bo_cpp;
   *mem_y = y / tex->block_height;
}

static unsigned
tex_get_box_offset(const struct ilo_texture *tex, unsigned level,
                   const struct pipe_box *box)
{
   unsigned mem_x, mem_y;

   tex_get_box_origin(tex, level, 0, box, &mem_x, &mem_y);

   return mem_y * tex->bo_stride + mem_x;
}

static unsigned
tex_get_slice_stride(const struct ilo_texture *tex, unsigned level)
{
   const struct ilo_texture_slice *s0, *s1;
   unsigned qpitch;

   /* there is no 3D array texture */
   assert(tex->base.array_size == 1 || tex->base.depth0 == 1);

   if (tex->base.array_size == 1) {
      /* non-array, non-3D */
      if (tex->base.depth0 == 1)
         return 0;

      /* only the first level has a fixed slice stride */
      if (level > 0) {
         assert(!"no slice stride for 3D texture with level > 0");
         return 0;
      }
   }

   s0 = ilo_texture_get_slice(tex, level, 0);
   s1 = ilo_texture_get_slice(tex, level, 1);
   qpitch = s1->y - s0->y;
   assert(qpitch % tex->block_height == 0);

   return (qpitch / tex->block_height) * tex->bo_stride;
}

static unsigned
tex_tile_x_swizzle(unsigned addr)
{
   /*
    * From the Ivy Bridge PRM, volume 1 part 2, page 24:
    *
    *     "As shown in the tiling algorithm, the new address bit[6] should be:
    *
    *        Address bit[6] <= TiledAddr bit[6] XOR
    *                          TiledAddr bit[9] XOR
    *                          TiledAddr bit[10]"
    */
   return addr ^ (((addr >> 3) ^ (addr >> 4)) & 0x40);
}

static unsigned
tex_tile_y_swizzle(unsigned addr)
{
   /*
    * From the Ivy Bridge PRM, volume 1 part 2, page 24:
    *
    *     "As shown in the tiling algorithm, The new address bit[6] becomes:
    *
    *        Address bit[6] <= TiledAddr bit[6] XOR
    *                          TiledAddr bit[9]"
    */
   return addr ^ ((addr >> 3) & 0x40);
}

static unsigned
tex_tile_x_offset(unsigned mem_x, unsigned mem_y,
                  unsigned tiles_per_row, bool swizzle)
{
   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 21, we know that a
    * X-major tile has 8 rows and 32 OWord columns (512 bytes).  Tiles in the
    * tiled region are numbered in row-major order, starting from zero.  The
    * tile number can thus be calculated as follows:
    *
    *    tile = (mem_y / 8) * tiles_per_row + (mem_x / 512)
    *
    * OWords in that tile are also numbered in row-major order, starting from
    * zero.  The OWord number can thus be calculated as follows:
    *
    *    oword = (mem_y % 8) * 32 + ((mem_x % 512) / 16)
    *
    * and the tiled offset is
    *
    *    offset = tile * 4096 + oword * 16 + (mem_x % 16)
    *           = tile * 4096 + (mem_y % 8) * 512 + (mem_x % 512)
    */
   unsigned tile, offset;

   tile = (mem_y >> 3) * tiles_per_row + (mem_x >> 9);
   offset = tile << 12 | (mem_y & 0x7) << 9 | (mem_x & 0x1ff);

   return (swizzle) ? tex_tile_x_swizzle(offset) : offset;
}

static unsigned
tex_tile_y_offset(unsigned mem_x, unsigned mem_y,
                  unsigned tiles_per_row, bool swizzle)
{
   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 22, we know that a
    * Y-major tile has 32 rows and 8 OWord columns (128 bytes).  Tiles in the
    * tiled region are numbered in row-major order, starting from zero.  The
    * tile number can thus be calculated as follows:
    *
    *    tile = (mem_y / 32) * tiles_per_row + (mem_x / 128)
    *
    * OWords in that tile are numbered in column-major order, starting from
    * zero.  The OWord number can thus be calculated as follows:
    *
    *    oword = ((mem_x % 128) / 16) * 32 + (mem_y % 32)
    *
    * and the tiled offset is
    *
    *    offset = tile * 4096 + oword * 16 + (mem_x % 16)
    */
   unsigned tile, oword, offset;

   tile = (mem_y >> 5) * tiles_per_row + (mem_x >> 7);
   oword = (mem_x & 0x70) << 1 | (mem_y & 0x1f);
   offset = tile << 12 | oword << 4 | (mem_x & 0xf);

   return (swizzle) ? tex_tile_y_swizzle(offset) : offset;
}

static unsigned
tex_tile_w_offset(unsigned mem_x, unsigned mem_y,
                  unsigned tiles_per_row, bool swizzle)
{
   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 23, we know that a
    * W-major tile has 8 8x8-block rows and 8 8x8-block columns.  Tiles in the
    * tiled region are numbered in row-major order, starting from zero.  The
    * tile number can thus be calculated as follows:
    *
    *    tile = (mem_y / 64) * tiles_per_row + (mem_x / 64)
    *
    * 8x8-blocks in that tile are numbered in column-major order, starting
    * from zero.  The 8x8-block number can thus be calculated as follows:
    *
    *    blk8 = ((mem_x % 64) / 8) * 8 + ((mem_y % 64) / 8)
    *
    * Each 8x8-block is divided into 4 4x4-blocks, in row-major order.  Each
    * 4x4-block is further divided into 4 2x2-blocks, also in row-major order.
    * We have
    *
    *    blk4 = (((mem_y % 64) / 4) & 1) * 2 + (((mem_x % 64) / 4) & 1)
    *    blk2 = (((mem_y % 64) / 2) & 1) * 2 + (((mem_x % 64) / 2) & 1)
    *    blk1 = (((mem_y % 64)    ) & 1) * 2 + (((mem_x % 64)    ) & 1)
    *
    * and the tiled offset is
    *
    *    offset = tile * 4096 + blk8 * 64 + blk4 * 16 + blk2 * 4 + blk1
    */
   unsigned tile, blk8, blk4, blk2, blk1, offset;

   tile = (mem_y >> 6) * tiles_per_row + (mem_x >> 6);
   blk8 = ((mem_x >> 3) & 0x7) << 3 | ((mem_y >> 3) & 0x7);
   blk4 = ((mem_y >> 2) & 0x1) << 1 | ((mem_x >> 2) & 0x1);
   blk2 = ((mem_y >> 1) & 0x1) << 1 | ((mem_x >> 1) & 0x1);
   blk1 = ((mem_y     ) & 0x1) << 1 | ((mem_x     ) & 0x1);
   offset = tile << 12 | blk8 << 6 | blk4 << 4 | blk2 << 2 | blk1;

   return (swizzle) ? tex_tile_y_swizzle(offset) : offset;
}

static unsigned
tex_tile_none_offset(unsigned mem_x, unsigned mem_y,
                     unsigned tiles_per_row, bool swizzle)
{
   return mem_y * tiles_per_row + mem_x;
}

typedef unsigned (*tex_tile_offset_func)(unsigned mem_x, unsigned mem_y,
                                         unsigned tiles_per_row,
                                         bool swizzle);

static tex_tile_offset_func
tex_tile_choose_offset_func(const struct ilo_texture *tex,
                            unsigned *tiles_per_row)
{
   switch (tex->tiling) {
   case INTEL_TILING_X:
      *tiles_per_row = tex->bo_stride / 512;
      return tex_tile_x_offset;
   case INTEL_TILING_Y:
      *tiles_per_row = tex->bo_stride / 128;
      return tex_tile_y_offset;
   case INTEL_TILING_NONE:
   default:
      /* W-tiling */
      if (tex->bo_format == PIPE_FORMAT_S8_UINT) {
         *tiles_per_row = tex->bo_stride / 64;
         return tex_tile_w_offset;
      }
      else {
         *tiles_per_row = tex->bo_stride;
         return tex_tile_none_offset;
      }
   }
}

static void *
tex_staging_sys_map_bo(const struct ilo_context *ilo,
                       struct ilo_texture *tex,
                       bool for_read_back, bool linear_view)
{
   const bool prefer_cpu = (ilo->dev->has_llc || for_read_back);
   void *ptr;

   if (prefer_cpu && (tex->tiling == INTEL_TILING_NONE || !linear_view))
      ptr = intel_bo_map(tex->bo, !for_read_back);
   else
      ptr = intel_bo_map_gtt(tex->bo);

   return ptr;
}

static void
tex_staging_sys_unmap_bo(const struct ilo_context *ilo,
                         const struct ilo_texture *tex)
{
   intel_bo_unmap(tex->bo);
}

static bool
tex_staging_sys_zs_read(struct ilo_context *ilo,
                        struct ilo_texture *tex,
                        const struct ilo_transfer *xfer)
{
   const bool swizzle = ilo->dev->has_address_swizzling;
   const struct pipe_box *box = &xfer->base.box;
   const uint8_t *src;
   tex_tile_offset_func tile_offset;
   unsigned tiles_per_row;
   int slice;

   src = tex_staging_sys_map_bo(ilo, tex, true, false);
   if (!src)
      return false;

   tile_offset = tex_tile_choose_offset_func(tex, &tiles_per_row);

   assert(tex->block_width == 1 && tex->block_height == 1);

   if (tex->separate_s8) {
      struct ilo_texture *s8_tex = tex->separate_s8;
      const uint8_t *s8_src;
      tex_tile_offset_func s8_tile_offset;
      unsigned s8_tiles_per_row;
      int dst_cpp, dst_s8_pos, src_cpp_used;

      s8_src = tex_staging_sys_map_bo(ilo, s8_tex, true, false);
      if (!s8_src) {
         tex_staging_sys_unmap_bo(ilo, tex);
         return false;
      }

      s8_tile_offset = tex_tile_choose_offset_func(s8_tex, &s8_tiles_per_row);

      if (tex->base.format == PIPE_FORMAT_Z24_UNORM_S8_UINT) {
         assert(tex->bo_format == PIPE_FORMAT_Z24X8_UNORM);

         dst_cpp = 4;
         dst_s8_pos = 3;
         src_cpp_used = 3;
      }
      else {
         assert(tex->base.format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);
         assert(tex->bo_format == PIPE_FORMAT_Z32_FLOAT);

         dst_cpp = 8;
         dst_s8_pos = 4;
         src_cpp_used = 4;
      }

      for (slice = 0; slice < box->depth; slice++) {
         unsigned mem_x, mem_y, s8_mem_x, s8_mem_y;
         uint8_t *dst;
         int i, j;

         tex_get_box_origin(tex, xfer->base.level, slice,
                            box, &mem_x, &mem_y);
         tex_get_box_origin(s8_tex, xfer->base.level, slice,
                            box, &s8_mem_x, &s8_mem_y);

         dst = xfer->staging_sys + xfer->base.layer_stride * slice;

         for (i = 0; i < box->height; i++) {
            unsigned x = mem_x, s8_x = s8_mem_x;
            uint8_t *d = dst;

            for (j = 0; j < box->width; j++) {
               const unsigned offset =
                  tile_offset(x, mem_y, tiles_per_row, swizzle);
               const unsigned s8_offset =
                  s8_tile_offset(s8_x, s8_mem_y, s8_tiles_per_row, swizzle);

               memcpy(d, src + offset, src_cpp_used);
               d[dst_s8_pos] = s8_src[s8_offset];

               d += dst_cpp;
               x += tex->bo_cpp;
               s8_x++;
            }

            dst += xfer->base.stride;
            mem_y++;
            s8_mem_y++;
         }
      }

      tex_staging_sys_unmap_bo(ilo, s8_tex);
   }
   else {
      assert(tex->bo_format == PIPE_FORMAT_S8_UINT);

      for (slice = 0; slice < box->depth; slice++) {
         unsigned mem_x, mem_y;
         uint8_t *dst;
         int i, j;

         tex_get_box_origin(tex, xfer->base.level, slice,
                            box, &mem_x, &mem_y);

         dst = xfer->staging_sys + xfer->base.layer_stride * slice;

         for (i = 0; i < box->height; i++) {
            unsigned x = mem_x;
            uint8_t *d = dst;

            for (j = 0; j < box->width; j++) {
               const unsigned offset =
                  tile_offset(x, mem_y, tiles_per_row, swizzle);

               *d = src[offset];

               d++;
               x++;
            }

            dst += xfer->base.stride;
            mem_y++;
         }
      }
   }

   tex_staging_sys_unmap_bo(ilo, tex);

   return true;
}

static bool
tex_staging_sys_zs_write(struct ilo_context *ilo,
                         struct ilo_texture *tex,
                         const struct ilo_transfer *xfer)
{
   const bool swizzle = ilo->dev->has_address_swizzling;
   const struct pipe_box *box = &xfer->base.box;
   uint8_t *dst;
   tex_tile_offset_func tile_offset;
   unsigned tiles_per_row;
   int slice;

   dst = tex_staging_sys_map_bo(ilo, tex, false, false);
   if (!dst)
      return false;

   tile_offset = tex_tile_choose_offset_func(tex, &tiles_per_row);

   assert(tex->block_width == 1 && tex->block_height == 1);

   if (tex->separate_s8) {
      struct ilo_texture *s8_tex = tex->separate_s8;
      uint8_t *s8_dst;
      tex_tile_offset_func s8_tile_offset;
      unsigned s8_tiles_per_row;
      int src_cpp, src_s8_pos, dst_cpp_used;

      s8_dst = tex_staging_sys_map_bo(ilo, s8_tex, false, false);
      if (!s8_dst) {
         tex_staging_sys_unmap_bo(ilo, s8_tex);
         return false;
      }

      s8_tile_offset = tex_tile_choose_offset_func(s8_tex, &s8_tiles_per_row);

      if (tex->base.format == PIPE_FORMAT_Z24_UNORM_S8_UINT) {
         assert(tex->bo_format == PIPE_FORMAT_Z24X8_UNORM);

         src_cpp = 4;
         src_s8_pos = 3;
         dst_cpp_used = 3;
      }
      else {
         assert(tex->base.format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);
         assert(tex->bo_format == PIPE_FORMAT_Z32_FLOAT);

         src_cpp = 8;
         src_s8_pos = 4;
         dst_cpp_used = 4;
      }

      for (slice = 0; slice < box->depth; slice++) {
         unsigned mem_x, mem_y, s8_mem_x, s8_mem_y;
         const uint8_t *src;
         int i, j;

         tex_get_box_origin(tex, xfer->base.level, slice,
                            box, &mem_x, &mem_y);
         tex_get_box_origin(s8_tex, xfer->base.level, slice,
                            box, &s8_mem_x, &s8_mem_y);

         src = xfer->staging_sys + xfer->base.layer_stride * slice;

         for (i = 0; i < box->height; i++) {
            unsigned x = mem_x, s8_x = s8_mem_x;
            const uint8_t *s = src;

            for (j = 0; j < box->width; j++) {
               const unsigned offset =
                  tile_offset(x, mem_y, tiles_per_row, swizzle);
               const unsigned s8_offset =
                  s8_tile_offset(s8_x, s8_mem_y, s8_tiles_per_row, swizzle);

               memcpy(dst + offset, s, dst_cpp_used);
               s8_dst[s8_offset] = s[src_s8_pos];

               s += src_cpp;
               x += tex->bo_cpp;
               s8_x++;
            }

            src += xfer->base.stride;
            mem_y++;
            s8_mem_y++;
         }
      }

      tex_staging_sys_unmap_bo(ilo, s8_tex);
   }
   else {
      assert(tex->bo_format == PIPE_FORMAT_S8_UINT);

      for (slice = 0; slice < box->depth; slice++) {
         unsigned mem_x, mem_y;
         const uint8_t *src;
         int i, j;

         tex_get_box_origin(tex, xfer->base.level, slice,
                            box, &mem_x, &mem_y);

         src = xfer->staging_sys + xfer->base.layer_stride * slice;

         for (i = 0; i < box->height; i++) {
            unsigned x = mem_x;
            const uint8_t *s = src;

            for (j = 0; j < box->width; j++) {
               const unsigned offset =
                  tile_offset(x, mem_y, tiles_per_row, swizzle);

               dst[offset] = *s;

               s++;
               x++;
            }

            src += xfer->base.stride;
            mem_y++;
         }
      }
   }

   tex_staging_sys_unmap_bo(ilo, tex);

   return true;
}

static bool
tex_staging_sys_convert_write(struct ilo_context *ilo,
                              struct ilo_texture *tex,
                              const struct ilo_transfer *xfer)
{
   const struct pipe_box *box = &xfer->base.box;
   unsigned dst_slice_stride;
   void *dst;
   int slice;

   dst = tex_staging_sys_map_bo(ilo, tex, false, true);
   if (!dst)
      return false;

   dst += tex_get_box_offset(tex, xfer->base.level, box);

   /* slice stride is not always available */
   if (box->depth > 1)
      dst_slice_stride = tex_get_slice_stride(tex, xfer->base.level);
   else
      dst_slice_stride = 0;

   if (unlikely(tex->bo_format == tex->base.format)) {
      util_copy_box(dst, tex->bo_format, tex->bo_stride, dst_slice_stride,
            0, 0, 0, box->width, box->height, box->depth,
            xfer->staging_sys, xfer->base.stride, xfer->base.layer_stride,
            0, 0, 0);

      tex_staging_sys_unmap_bo(ilo, tex);

      return true;
   }

   switch (tex->base.format) {
   case PIPE_FORMAT_ETC1_RGB8:
      assert(tex->bo_format == PIPE_FORMAT_R8G8B8X8_UNORM);

      for (slice = 0; slice < box->depth; slice++) {
         const void *src =
            xfer->staging_sys + xfer->base.layer_stride * slice;

         util_format_etc1_rgb8_unpack_rgba_8unorm(dst,
               tex->bo_stride, src, xfer->base.stride,
               box->width, box->height);

         dst += dst_slice_stride;
      }
      break;
   default:
      assert(!"unable to convert the staging data");
      break;
   }

   tex_staging_sys_unmap_bo(ilo, tex);

   return true;
}

static void
tex_staging_sys_unmap(struct ilo_context *ilo,
                      struct ilo_texture *tex,
                      struct ilo_transfer *xfer)
{
   bool success;

   if (!(xfer->base.usage & PIPE_TRANSFER_WRITE)) {
      FREE(xfer->staging_sys);
      return;
   }

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_SW_CONVERT:
      success = tex_staging_sys_convert_write(ilo, tex, xfer);
      break;
   case ILO_TRANSFER_MAP_SW_ZS:
      success = tex_staging_sys_zs_write(ilo, tex, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      success = false;
      break;
   }

   if (!success)
      ilo_err("failed to map resource for moving staging data\n");

   FREE(xfer->staging_sys);
}

static bool
tex_staging_sys_map(struct ilo_context *ilo,
                    struct ilo_texture *tex,
                    struct ilo_transfer *xfer)
{
   const struct pipe_box *box = &xfer->base.box;
   const size_t stride = util_format_get_stride(tex->base.format, box->width);
   const size_t size =
      util_format_get_2d_size(tex->base.format, stride, box->height);
   bool read_back = false, success;

   xfer->staging_sys = MALLOC(size * box->depth);
   if (!xfer->staging_sys)
      return false;

   xfer->base.stride = stride;
   xfer->base.layer_stride = size;
   xfer->ptr = xfer->staging_sys;

   /* see if we need to read the resource back */
   if (xfer->base.usage & PIPE_TRANSFER_READ) {
      read_back = true;
   }
   else if (xfer->base.usage & PIPE_TRANSFER_WRITE) {
      const unsigned discard_flags =
         (PIPE_TRANSFER_DISCARD_RANGE | PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE);

      if (!(xfer->base.usage & discard_flags))
         read_back = true;
   }

   if (!read_back)
      return true;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_SW_CONVERT:
      assert(!"no on-the-fly format conversion for mapping");
      success = false;
      break;
   case ILO_TRANSFER_MAP_SW_ZS:
      success = tex_staging_sys_zs_read(ilo, tex, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      success = false;
      break;
   }

   return success;
}

static void
tex_direct_unmap(struct ilo_context *ilo,
                 struct ilo_texture *tex,
                 struct ilo_transfer *xfer)
{
   intel_bo_unmap(tex->bo);
}

static bool
tex_direct_map(struct ilo_context *ilo,
               struct ilo_texture *tex,
               struct ilo_transfer *xfer)
{
   xfer->ptr = map_bo_for_transfer(ilo, tex->bo, xfer);
   if (!xfer->ptr)
      return false;

   xfer->ptr += tex_get_box_offset(tex, xfer->base.level, &xfer->base.box);

   /* note that stride is for a block row, not a texel row */
   xfer->base.stride = tex->bo_stride;

   /* slice stride is not always available */
   if (xfer->base.box.depth > 1)
      xfer->base.layer_stride = tex_get_slice_stride(tex, xfer->base.level);
   else
      xfer->base.layer_stride = 0;

   return true;
}

static bool
tex_map(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct ilo_texture *tex = ilo_texture(xfer->base.resource);
   bool success;

   if (!choose_transfer_method(ilo, xfer))
      return false;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
   case ILO_TRANSFER_MAP_GTT:
   case ILO_TRANSFER_MAP_UNSYNC:
      success = tex_direct_map(ilo, tex, xfer);
      break;
   case ILO_TRANSFER_MAP_SW_CONVERT:
   case ILO_TRANSFER_MAP_SW_ZS:
      success = tex_staging_sys_map(ilo, tex, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      success = false;
      break;
   }

   return success;
}

static void
tex_unmap(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct ilo_texture *tex = ilo_texture(xfer->base.resource);

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
   case ILO_TRANSFER_MAP_GTT:
   case ILO_TRANSFER_MAP_UNSYNC:
      tex_direct_unmap(ilo, tex, xfer);
      break;
   case ILO_TRANSFER_MAP_SW_CONVERT:
   case ILO_TRANSFER_MAP_SW_ZS:
      tex_staging_sys_unmap(ilo, tex, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      break;
   }
}

static bool
buf_map(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct ilo_buffer *buf = ilo_buffer(xfer->base.resource);

   if (!choose_transfer_method(ilo, xfer))
      return false;

   xfer->ptr = map_bo_for_transfer(ilo, buf->bo, xfer);
   if (!xfer->ptr)
      return false;

   assert(xfer->base.level == 0);
   assert(xfer->base.box.y == 0);
   assert(xfer->base.box.z == 0);
   assert(xfer->base.box.height == 1);
   assert(xfer->base.box.depth == 1);

   xfer->ptr += xfer->base.box.x;
   xfer->base.stride = 0;
   xfer->base.layer_stride = 0;

   return true;
}

static void
buf_unmap(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct ilo_buffer *buf = ilo_buffer(xfer->base.resource);

   intel_bo_unmap(buf->bo);
}

static void
buf_pwrite(struct ilo_context *ilo, struct ilo_buffer *buf,
           unsigned usage, int offset, int size, const void *data)
{
   bool need_flush;

   /* see if we can avoid stalling */
   if (is_bo_busy(ilo, buf->bo, &need_flush)) {
      bool will_stall = true;

      if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) {
         /* old data not needed so discard the old bo to avoid stalling */
         if (ilo_buffer_alloc_bo(buf)) {
            ilo_mark_states_with_resource_dirty(ilo, &buf->base);
            will_stall = false;
         }
      }
      else {
         /*
          * We could allocate a temporary bo to hold the data and emit
          * pipelined copy blit to move them to buf->bo.  But for now, do
          * nothing.
          */
      }

      /* flush to make bo busy (so that pwrite() stalls as it should be) */
      if (will_stall && need_flush)
         ilo_cp_flush(ilo->cp, "syncing for pwrites");
   }

   intel_bo_pwrite(buf->bo, offset, size, data);
}

static void
ilo_transfer_flush_region(struct pipe_context *pipe,
                          struct pipe_transfer *transfer,
                          const struct pipe_box *box)
{
}

static void
ilo_transfer_unmap(struct pipe_context *pipe,
                   struct pipe_transfer *transfer)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_transfer *xfer = ilo_transfer(transfer);

   if (xfer->base.resource->target == PIPE_BUFFER)
      buf_unmap(ilo, xfer);
   else
      tex_unmap(ilo, xfer);

   pipe_resource_reference(&xfer->base.resource, NULL);

   util_slab_free(&ilo->transfer_mempool, xfer);
}

static void *
ilo_transfer_map(struct pipe_context *pipe,
                 struct pipe_resource *res,
                 unsigned level,
                 unsigned usage,
                 const struct pipe_box *box,
                 struct pipe_transfer **transfer)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_transfer *xfer;
   bool success;

   xfer = util_slab_alloc(&ilo->transfer_mempool);
   if (!xfer) {
      *transfer = NULL;
      return NULL;
   }

   xfer->base.resource = NULL;
   pipe_resource_reference(&xfer->base.resource, res);
   xfer->base.level = level;
   xfer->base.usage = usage;
   xfer->base.box = *box;

   ilo_blit_resolve_transfer(ilo, &xfer->base);

   if (res->target == PIPE_BUFFER)
      success = buf_map(ilo, xfer);
   else
      success = tex_map(ilo, xfer);

   if (!success) {
      pipe_resource_reference(&xfer->base.resource, NULL);
      FREE(xfer);
      *transfer = NULL;
      return NULL;
   }

   *transfer = &xfer->base;

   return xfer->ptr;
}

static void
ilo_transfer_inline_write(struct pipe_context *pipe,
                          struct pipe_resource *res,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box,
                          const void *data,
                          unsigned stride,
                          unsigned layer_stride)
{
   if (likely(res->target == PIPE_BUFFER) &&
       !(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
      /* they should specify just an offset and a size */
      assert(level == 0);
      assert(box->y == 0);
      assert(box->z == 0);
      assert(box->height == 1);
      assert(box->depth == 1);

      buf_pwrite(ilo_context(pipe), ilo_buffer(res),
            usage, box->x, box->width, data);
   }
   else {
      u_default_transfer_inline_write(pipe, res,
            level, usage, box, data, stride, layer_stride);
   }
}

/**
 * Initialize transfer-related functions.
 */
void
ilo_init_transfer_functions(struct ilo_context *ilo)
{
   ilo->base.transfer_map = ilo_transfer_map;
   ilo->base.transfer_flush_region = ilo_transfer_flush_region;
   ilo->base.transfer_unmap = ilo_transfer_unmap;
   ilo->base.transfer_inline_write = ilo_transfer_inline_write;
}
