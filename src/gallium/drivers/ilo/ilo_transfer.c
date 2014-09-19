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
#include "ilo_blitter.h"
#include "ilo_cp.h"
#include "ilo_context.h"
#include "ilo_resource.h"
#include "ilo_state.h"
#include "ilo_transfer.h"

/*
 * For buffers that are not busy, we want to map/unmap them directly.  For
 * those that are busy, we have to worry about synchronization.  We could wait
 * for GPU to finish, but there are cases where we could avoid waiting.
 *
 *  - When PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE is set, the contents of the
 *    buffer can be discarded.  We can replace the backing bo by a new one of
 *    the same size (renaming).
 *  - When PIPE_TRANSFER_DISCARD_RANGE is set, the contents of the mapped
 *    range can be discarded.  We can allocate and map a staging bo on
 *    mapping, and (pipelined-)copy it over to the real bo on unmapping.
 *  - When PIPE_TRANSFER_FLUSH_EXPLICIT is set, there is no reading and only
 *    flushed regions need to be written.  We can still allocate and map a
 *    staging bo, but should copy only the flushed regions over.
 *
 * However, there are other flags to consider.
 *
 *  - When PIPE_TRANSFER_UNSYNCHRONIZED is set, we do not need to worry about
 *    synchronization at all on mapping.
 *  - When PIPE_TRANSFER_MAP_DIRECTLY is set, no staging area is allowed.
 *  - When PIPE_TRANSFER_DONTBLOCK is set, we should fail if we have to block.
 *  - When PIPE_TRANSFER_PERSISTENT is set, GPU may access the buffer while it
 *    is mapped.  Synchronization is done by defining memory barriers,
 *    explicitly via memory_barrier() or implicitly via
 *    transfer_flush_region(), as well as GPU fences.
 *  - When PIPE_TRANSFER_COHERENT is set, updates by either CPU or GPU should
 *    be made visible to the other side immediately.  Since the kernel flushes
 *    GPU caches at the end of each batch buffer, CPU always sees GPU updates.
 *    We could use a coherent mapping to make all persistent mappings
 *    coherent.
 *
 * These also apply to textures, except that we may additionally need to do
 * format conversion or tiling/untiling.
 */

/**
 * Return a transfer method suitable for the usage.  The returned method will
 * correctly block when the resource is busy.
 */
static bool
resource_get_transfer_method(struct pipe_resource *res,
                             const struct pipe_transfer *transfer,
                             enum ilo_transfer_map_method *method)
{
   const struct ilo_screen *is = ilo_screen(res->screen);
   const unsigned usage = transfer->usage;
   enum ilo_transfer_map_method m;
   bool tiled;

   if (res->target == PIPE_BUFFER) {
      tiled = false;
   }
   else {
      struct ilo_texture *tex = ilo_texture(res);
      bool need_convert = false;

      /* we may need to convert on the fly */
      if (tex->separate_s8 || tex->layout.format == PIPE_FORMAT_S8_UINT) {
         /* on GEN6, separate stencil is enabled only when HiZ is */
         if (ilo_dev_gen(&is->dev) >= ILO_GEN(7) ||
             ilo_texture_can_enable_hiz(tex, transfer->level,
                transfer->box.z, transfer->box.depth)) {
            m = ILO_TRANSFER_MAP_SW_ZS;
            need_convert = true;
         }
      } else if (tex->layout.format != tex->base.format) {
         m = ILO_TRANSFER_MAP_SW_CONVERT;
         need_convert = true;
      }

      if (need_convert) {
         if (usage & (PIPE_TRANSFER_MAP_DIRECTLY | PIPE_TRANSFER_PERSISTENT))
            return false;

         *method = m;
         return true;
      }

      tiled = (tex->layout.tiling != INTEL_TILING_NONE);
   }

   if (tiled)
      m = ILO_TRANSFER_MAP_GTT; /* to have a linear view */
   else if (is->dev.has_llc)
      m = ILO_TRANSFER_MAP_CPU; /* fast and mostly coherent */
   else if (usage & PIPE_TRANSFER_PERSISTENT)
      m = ILO_TRANSFER_MAP_GTT; /* for coherency */
   else if (usage & PIPE_TRANSFER_READ)
      m = ILO_TRANSFER_MAP_CPU; /* gtt read is too slow */
   else
      m = ILO_TRANSFER_MAP_GTT;

   *method = m;

   return true;
}

/**
 * Rename the bo of the resource.
 */
static bool
resource_rename_bo(struct pipe_resource *res)
{
   return (res->target == PIPE_BUFFER) ?
      ilo_buffer_rename_bo(ilo_buffer(res)) :
      ilo_texture_rename_bo(ilo_texture(res));
}

/**
 * Return true if usage allows the use of staging bo to avoid blocking.
 */
static bool
usage_allows_staging_bo(unsigned usage)
{
   /* do we know how to write the data back to the resource? */
   const unsigned can_writeback = (PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE |
                                   PIPE_TRANSFER_DISCARD_RANGE |
                                   PIPE_TRANSFER_FLUSH_EXPLICIT);
   const unsigned reasons_against = (PIPE_TRANSFER_READ |
                                     PIPE_TRANSFER_MAP_DIRECTLY |
                                     PIPE_TRANSFER_PERSISTENT);

   return (usage & can_writeback) && !(usage & reasons_against);
}

/**
 * Allocate the staging resource.  It is always linear and its size matches
 * the transfer box, with proper paddings.
 */
static bool
xfer_alloc_staging_res(struct ilo_transfer *xfer)
{
   const struct pipe_resource *res = xfer->base.resource;
   const struct pipe_box *box = &xfer->base.box;
   struct pipe_resource templ;

   memset(&templ, 0, sizeof(templ));

   templ.format = res->format;

   if (res->target == PIPE_BUFFER) {
      templ.target = PIPE_BUFFER;
      templ.width0 =
         (box->x % ILO_TRANSFER_MAP_BUFFER_ALIGNMENT) + box->width;
   }
   else {
      /* use 2D array for any texture target */
      templ.target = PIPE_TEXTURE_2D_ARRAY;
      templ.width0 = box->width;
   }

   templ.height0 = box->height;
   templ.depth0 = 1;
   templ.array_size = box->depth;
   templ.nr_samples = 1;
   templ.usage = PIPE_USAGE_STAGING;
   templ.bind = PIPE_BIND_TRANSFER_WRITE;

   if (xfer->base.usage & PIPE_TRANSFER_FLUSH_EXPLICIT) {
      templ.flags = PIPE_RESOURCE_FLAG_MAP_PERSISTENT |
                    PIPE_RESOURCE_FLAG_MAP_COHERENT;
   }

   xfer->staging.res = res->screen->resource_create(res->screen, &templ);

   if (xfer->staging.res && xfer->staging.res->target != PIPE_BUFFER) {
      assert(ilo_texture(xfer->staging.res)->layout.tiling ==
            INTEL_TILING_NONE);
   }

   return (xfer->staging.res != NULL);
}

/**
 * Use an alternative transfer method or rename the resource to unblock an
 * otherwise blocking transfer.
 */
static bool
xfer_unblock(struct ilo_transfer *xfer, bool *resource_renamed)
{
   struct pipe_resource *res = xfer->base.resource;
   bool unblocked = false, renamed = false;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
   case ILO_TRANSFER_MAP_GTT:
      if (xfer->base.usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
         xfer->method = ILO_TRANSFER_MAP_GTT_ASYNC;
         unblocked = true;
      }
      else if ((xfer->base.usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) &&
               resource_rename_bo(res)) {
         renamed = true;
         unblocked = true;
      }
      else if (usage_allows_staging_bo(xfer->base.usage) &&
               xfer_alloc_staging_res(xfer)) {
         xfer->method = ILO_TRANSFER_MAP_STAGING;
         unblocked = true;
      }
      break;
   case ILO_TRANSFER_MAP_GTT_ASYNC:
   case ILO_TRANSFER_MAP_STAGING:
      unblocked = true;
      break;
   default:
      break;
   }

   *resource_renamed = renamed;

   return unblocked;
}

/**
 * Allocate the staging system buffer based on the resource format and the
 * transfer box.
 */
static bool
xfer_alloc_staging_sys(struct ilo_transfer *xfer)
{
   const enum pipe_format format = xfer->base.resource->format;
   const struct pipe_box *box = &xfer->base.box;
   const unsigned alignment = 64;

   /* need to tell the world the layout */
   xfer->base.stride =
      align(util_format_get_stride(format, box->width), alignment);
   xfer->base.layer_stride =
      util_format_get_2d_size(format, xfer->base.stride, box->height);

   xfer->staging.sys =
      align_malloc(xfer->base.layer_stride * box->depth, alignment);

   return (xfer->staging.sys != NULL);
}

/**
 * Map according to the method.  The staging system buffer should have been
 * allocated if the method requires it.
 */
static void *
xfer_map(struct ilo_transfer *xfer)
{
   void *ptr;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
      ptr = intel_bo_map(ilo_resource_get_bo(xfer->base.resource),
            xfer->base.usage & PIPE_TRANSFER_WRITE);
      break;
   case ILO_TRANSFER_MAP_GTT:
      ptr = intel_bo_map_gtt(ilo_resource_get_bo(xfer->base.resource));
      break;
   case ILO_TRANSFER_MAP_GTT_ASYNC:
      ptr = intel_bo_map_gtt_async(ilo_resource_get_bo(xfer->base.resource));
      break;
   case ILO_TRANSFER_MAP_STAGING:
      {
         const struct ilo_screen *is = ilo_screen(xfer->staging.res->screen);
         struct intel_bo *bo = ilo_resource_get_bo(xfer->staging.res);

         /*
          * We want a writable, optionally persistent and coherent, mapping
          * for a linear bo.  We can call resource_get_transfer_method(), but
          * this turns out to be fairly simple.
          */
         if (is->dev.has_llc)
            ptr = intel_bo_map(bo, true);
         else
            ptr = intel_bo_map_gtt(bo);

         if (ptr && xfer->staging.res->target == PIPE_BUFFER)
            ptr += (xfer->base.box.x % ILO_TRANSFER_MAP_BUFFER_ALIGNMENT);

      }
      break;
   case ILO_TRANSFER_MAP_SW_CONVERT:
   case ILO_TRANSFER_MAP_SW_ZS:
      ptr = xfer->staging.sys;
      break;
   default:
      assert(!"unknown mapping method");
      ptr = NULL;
      break;
   }

   return ptr;
}

/**
 * Unmap a transfer.
 */
static void
xfer_unmap(struct ilo_transfer *xfer)
{
   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
   case ILO_TRANSFER_MAP_GTT:
   case ILO_TRANSFER_MAP_GTT_ASYNC:
      intel_bo_unmap(ilo_resource_get_bo(xfer->base.resource));
      break;
   case ILO_TRANSFER_MAP_STAGING:
      intel_bo_unmap(ilo_resource_get_bo(xfer->staging.res));
      break;
   default:
      break;
   }
}

static void
tex_get_box_origin(const struct ilo_texture *tex,
                   unsigned level, unsigned slice,
                   const struct pipe_box *box,
                   unsigned *mem_x, unsigned *mem_y)
{
   unsigned x, y;

   ilo_layout_get_slice_pos(&tex->layout, level, box->z + slice, &x, &y);
   x += box->x;
   y += box->y;

   ilo_layout_pos_to_mem(&tex->layout, x, y, mem_x, mem_y);
}

static unsigned
tex_get_box_offset(const struct ilo_texture *tex, unsigned level,
                   const struct pipe_box *box)
{
   unsigned mem_x, mem_y;

   tex_get_box_origin(tex, level, 0, box, &mem_x, &mem_y);

   return ilo_layout_mem_to_linear(&tex->layout, mem_x, mem_y);
}

static unsigned
tex_get_slice_stride(const struct ilo_texture *tex, unsigned level)
{
   return ilo_layout_get_slice_stride(&tex->layout, level);
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
   switch (tex->layout.tiling) {
   case INTEL_TILING_X:
      *tiles_per_row = tex->layout.bo_stride / 512;
      return tex_tile_x_offset;
   case INTEL_TILING_Y:
      *tiles_per_row = tex->layout.bo_stride / 128;
      return tex_tile_y_offset;
   case INTEL_TILING_NONE:
   default:
      /* W-tiling */
      if (tex->layout.format == PIPE_FORMAT_S8_UINT) {
         *tiles_per_row = tex->layout.bo_stride / 64;
         return tex_tile_w_offset;
      }
      else {
         *tiles_per_row = tex->layout.bo_stride;
         return tex_tile_none_offset;
      }
   }
}

static void *
tex_staging_sys_map_bo(struct ilo_texture *tex,
                       bool for_read_back,
                       bool linear_view)
{
   const struct ilo_screen *is = ilo_screen(tex->base.screen);
   const bool prefer_cpu = (is->dev.has_llc || for_read_back);
   void *ptr;

   if (prefer_cpu && (tex->layout.tiling == INTEL_TILING_NONE ||
                      !linear_view))
      ptr = intel_bo_map(tex->bo, !for_read_back);
   else
      ptr = intel_bo_map_gtt(tex->bo);

   return ptr;
}

static void
tex_staging_sys_unmap_bo(struct ilo_texture *tex)
{
   intel_bo_unmap(tex->bo);
}

static bool
tex_staging_sys_zs_read(struct ilo_texture *tex,
                        const struct ilo_transfer *xfer)
{
   const struct ilo_screen *is = ilo_screen(tex->base.screen);
   const bool swizzle = is->dev.has_address_swizzling;
   const struct pipe_box *box = &xfer->base.box;
   const uint8_t *src;
   tex_tile_offset_func tile_offset;
   unsigned tiles_per_row;
   int slice;

   src = tex_staging_sys_map_bo(tex, true, false);
   if (!src)
      return false;

   tile_offset = tex_tile_choose_offset_func(tex, &tiles_per_row);

   assert(tex->layout.block_width == 1 && tex->layout.block_height == 1);

   if (tex->separate_s8) {
      struct ilo_texture *s8_tex = tex->separate_s8;
      const uint8_t *s8_src;
      tex_tile_offset_func s8_tile_offset;
      unsigned s8_tiles_per_row;
      int dst_cpp, dst_s8_pos, src_cpp_used;

      s8_src = tex_staging_sys_map_bo(s8_tex, true, false);
      if (!s8_src) {
         tex_staging_sys_unmap_bo(tex);
         return false;
      }

      s8_tile_offset = tex_tile_choose_offset_func(s8_tex, &s8_tiles_per_row);

      if (tex->base.format == PIPE_FORMAT_Z24_UNORM_S8_UINT) {
         assert(tex->layout.format == PIPE_FORMAT_Z24X8_UNORM);

         dst_cpp = 4;
         dst_s8_pos = 3;
         src_cpp_used = 3;
      }
      else {
         assert(tex->base.format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);
         assert(tex->layout.format == PIPE_FORMAT_Z32_FLOAT);

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

         dst = xfer->staging.sys + xfer->base.layer_stride * slice;

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
               x += tex->layout.block_size;
               s8_x++;
            }

            dst += xfer->base.stride;
            mem_y++;
            s8_mem_y++;
         }
      }

      tex_staging_sys_unmap_bo(s8_tex);
   }
   else {
      assert(tex->layout.format == PIPE_FORMAT_S8_UINT);

      for (slice = 0; slice < box->depth; slice++) {
         unsigned mem_x, mem_y;
         uint8_t *dst;
         int i, j;

         tex_get_box_origin(tex, xfer->base.level, slice,
                            box, &mem_x, &mem_y);

         dst = xfer->staging.sys + xfer->base.layer_stride * slice;

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

   tex_staging_sys_unmap_bo(tex);

   return true;
}

static bool
tex_staging_sys_zs_write(struct ilo_texture *tex,
                         const struct ilo_transfer *xfer)
{
   const struct ilo_screen *is = ilo_screen(tex->base.screen);
   const bool swizzle = is->dev.has_address_swizzling;
   const struct pipe_box *box = &xfer->base.box;
   uint8_t *dst;
   tex_tile_offset_func tile_offset;
   unsigned tiles_per_row;
   int slice;

   dst = tex_staging_sys_map_bo(tex, false, false);
   if (!dst)
      return false;

   tile_offset = tex_tile_choose_offset_func(tex, &tiles_per_row);

   assert(tex->layout.block_width == 1 && tex->layout.block_height == 1);

   if (tex->separate_s8) {
      struct ilo_texture *s8_tex = tex->separate_s8;
      uint8_t *s8_dst;
      tex_tile_offset_func s8_tile_offset;
      unsigned s8_tiles_per_row;
      int src_cpp, src_s8_pos, dst_cpp_used;

      s8_dst = tex_staging_sys_map_bo(s8_tex, false, false);
      if (!s8_dst) {
         tex_staging_sys_unmap_bo(s8_tex);
         return false;
      }

      s8_tile_offset = tex_tile_choose_offset_func(s8_tex, &s8_tiles_per_row);

      if (tex->base.format == PIPE_FORMAT_Z24_UNORM_S8_UINT) {
         assert(tex->layout.format == PIPE_FORMAT_Z24X8_UNORM);

         src_cpp = 4;
         src_s8_pos = 3;
         dst_cpp_used = 3;
      }
      else {
         assert(tex->base.format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT);
         assert(tex->layout.format == PIPE_FORMAT_Z32_FLOAT);

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

         src = xfer->staging.sys + xfer->base.layer_stride * slice;

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
               x += tex->layout.block_size;
               s8_x++;
            }

            src += xfer->base.stride;
            mem_y++;
            s8_mem_y++;
         }
      }

      tex_staging_sys_unmap_bo(s8_tex);
   }
   else {
      assert(tex->layout.format == PIPE_FORMAT_S8_UINT);

      for (slice = 0; slice < box->depth; slice++) {
         unsigned mem_x, mem_y;
         const uint8_t *src;
         int i, j;

         tex_get_box_origin(tex, xfer->base.level, slice,
                            box, &mem_x, &mem_y);

         src = xfer->staging.sys + xfer->base.layer_stride * slice;

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

   tex_staging_sys_unmap_bo(tex);

   return true;
}

static bool
tex_staging_sys_convert_write(struct ilo_texture *tex,
                              const struct ilo_transfer *xfer)
{
   const struct pipe_box *box = &xfer->base.box;
   unsigned dst_slice_stride;
   void *dst;
   int slice;

   dst = tex_staging_sys_map_bo(tex, false, true);
   if (!dst)
      return false;

   dst += tex_get_box_offset(tex, xfer->base.level, box);

   /* slice stride is not always available */
   if (box->depth > 1)
      dst_slice_stride = tex_get_slice_stride(tex, xfer->base.level);
   else
      dst_slice_stride = 0;

   if (unlikely(tex->layout.format == tex->base.format)) {
      util_copy_box(dst, tex->layout.format, tex->layout.bo_stride,
            dst_slice_stride, 0, 0, 0, box->width, box->height, box->depth,
            xfer->staging.sys, xfer->base.stride, xfer->base.layer_stride,
            0, 0, 0);

      tex_staging_sys_unmap_bo(tex);

      return true;
   }

   switch (tex->base.format) {
   case PIPE_FORMAT_ETC1_RGB8:
      assert(tex->layout.format == PIPE_FORMAT_R8G8B8X8_UNORM);

      for (slice = 0; slice < box->depth; slice++) {
         const void *src =
            xfer->staging.sys + xfer->base.layer_stride * slice;

         util_format_etc1_rgb8_unpack_rgba_8unorm(dst,
               tex->layout.bo_stride, src, xfer->base.stride,
               box->width, box->height);

         dst += dst_slice_stride;
      }
      break;
   default:
      assert(!"unable to convert the staging data");
      break;
   }

   tex_staging_sys_unmap_bo(tex);

   return true;
}

static void
tex_staging_sys_writeback(struct ilo_transfer *xfer)
{
   struct ilo_texture *tex = ilo_texture(xfer->base.resource);
   bool success;

   if (!(xfer->base.usage & PIPE_TRANSFER_WRITE))
      return;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_SW_CONVERT:
      success = tex_staging_sys_convert_write(tex, xfer);
      break;
   case ILO_TRANSFER_MAP_SW_ZS:
      success = tex_staging_sys_zs_write(tex, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      success = false;
      break;
   }

   if (!success)
      ilo_err("failed to map resource for moving staging data\n");
}

static bool
tex_staging_sys_readback(struct ilo_transfer *xfer)
{
   struct ilo_texture *tex = ilo_texture(xfer->base.resource);
   bool read_back = false, success;

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
      success = tex_staging_sys_zs_read(tex, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      success = false;
      break;
   }

   return success;
}

static void *
tex_map(struct ilo_transfer *xfer)
{
   void *ptr;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
   case ILO_TRANSFER_MAP_GTT:
   case ILO_TRANSFER_MAP_GTT_ASYNC:
      ptr = xfer_map(xfer);
      if (ptr) {
         const struct ilo_texture *tex = ilo_texture(xfer->base.resource);

         ptr += tex_get_box_offset(tex, xfer->base.level, &xfer->base.box);

         /* stride is for a block row, not a texel row */
         xfer->base.stride = tex->layout.bo_stride;
         /* note that slice stride is not always available */
         xfer->base.layer_stride = (xfer->base.box.depth > 1) ?
            tex_get_slice_stride(tex, xfer->base.level) : 0;
      }
      break;
   case ILO_TRANSFER_MAP_STAGING:
      ptr = xfer_map(xfer);
      if (ptr) {
         const struct ilo_texture *staging = ilo_texture(xfer->staging.res);
         xfer->base.stride = staging->layout.bo_stride;
         xfer->base.layer_stride = tex_get_slice_stride(staging, 0);
      }
      break;
   case ILO_TRANSFER_MAP_SW_CONVERT:
   case ILO_TRANSFER_MAP_SW_ZS:
      if (xfer_alloc_staging_sys(xfer) && tex_staging_sys_readback(xfer))
         ptr = xfer_map(xfer);
      else
         ptr = NULL;
      break;
   default:
      assert(!"unknown mapping method");
      ptr = NULL;
      break;
   }

   return ptr;
}

static void *
buf_map(struct ilo_transfer *xfer)
{
   void *ptr;

   ptr = xfer_map(xfer);
   if (!ptr)
      return NULL;

   if (xfer->method != ILO_TRANSFER_MAP_STAGING)
      ptr += xfer->base.box.x;

   xfer->base.stride = 0;
   xfer->base.layer_stride = 0;

   assert(xfer->base.level == 0);
   assert(xfer->base.box.y == 0);
   assert(xfer->base.box.z == 0);
   assert(xfer->base.box.height == 1);
   assert(xfer->base.box.depth == 1);

   return ptr;
}

static void
copy_staging_resource(struct ilo_context *ilo,
                      struct ilo_transfer *xfer,
                      const struct pipe_box *box)
{
   const unsigned pad_x = (xfer->staging.res->target == PIPE_BUFFER) ?
      xfer->base.box.x % ILO_TRANSFER_MAP_BUFFER_ALIGNMENT : 0;
   struct pipe_box modified_box;

   assert(xfer->method == ILO_TRANSFER_MAP_STAGING && xfer->staging.res);

   if (!box) {
      u_box_3d(pad_x, 0, 0, xfer->base.box.width, xfer->base.box.height,
            xfer->base.box.depth, &modified_box);
      box = &modified_box;
   }
   else if (pad_x) {
      modified_box = *box;
      modified_box.x += pad_x;
      box = &modified_box;
   }

   ilo_blitter_blt_copy_resource(ilo->blitter,
         xfer->base.resource, xfer->base.level,
         xfer->base.box.x, xfer->base.box.y, xfer->base.box.z,
         xfer->staging.res, 0, box);
}

static bool
is_bo_busy(struct ilo_context *ilo, struct intel_bo *bo, bool *need_submit)
{
   const bool referenced = ilo_builder_has_reloc(&ilo->cp->builder, bo);

   if (need_submit)
      *need_submit = referenced;

   if (referenced)
      return true;

   return intel_bo_is_busy(bo);
}

/**
 * Choose the best mapping method, depending on the transfer usage and whether
 * the bo is busy.
 */
static bool
choose_transfer_method(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct pipe_resource *res = xfer->base.resource;
   bool need_submit;

   if (!resource_get_transfer_method(res, &xfer->base, &xfer->method))
      return false;

   /* see if we can avoid blocking */
   if (is_bo_busy(ilo, ilo_resource_get_bo(res), &need_submit)) {
      bool resource_renamed;

      if (!xfer_unblock(xfer, &resource_renamed)) {
         if (xfer->base.usage & PIPE_TRANSFER_DONTBLOCK)
            return false;

         /* submit to make bo really busy and map() correctly blocks */
         if (need_submit)
            ilo_cp_submit(ilo->cp, "syncing for transfers");
      }

      if (resource_renamed)
         ilo_state_vector_resource_renamed(&ilo->state_vector, res);
   }

   return true;
}

static void
buf_pwrite(struct ilo_context *ilo, struct ilo_buffer *buf,
           unsigned usage, int offset, int size, const void *data)
{
   bool need_submit;

   /* see if we can avoid blocking */
   if (is_bo_busy(ilo, buf->bo, &need_submit)) {
      bool unblocked = false;

      if ((usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) &&
          ilo_buffer_rename_bo(buf)) {
         ilo_state_vector_resource_renamed(&ilo->state_vector, &buf->base);
         unblocked = true;
      }
      else {
         struct pipe_resource templ, *staging;

         /*
          * allocate a staging buffer to hold the data and pipelined copy it
          * over
          */
         templ = buf->base;
         templ.width0 = size;
         templ.usage = PIPE_USAGE_STAGING;
         templ.bind = PIPE_BIND_TRANSFER_WRITE;
         staging = ilo->base.screen->resource_create(ilo->base.screen, &templ);
         if (staging) {
            struct pipe_box staging_box;

            intel_bo_pwrite(ilo_buffer(staging)->bo, 0, size, data);

            u_box_1d(0, size, &staging_box);
            ilo_blitter_blt_copy_resource(ilo->blitter,
                  &buf->base, 0, offset, 0, 0,
                  staging, 0, &staging_box);

            pipe_resource_reference(&staging, NULL);

            return;
         }
      }

      /* submit to make bo really busy and pwrite() correctly blocks */
      if (!unblocked && need_submit)
         ilo_cp_submit(ilo->cp, "syncing for pwrites");
   }

   intel_bo_pwrite(buf->bo, offset, size, data);
}

static void
ilo_transfer_flush_region(struct pipe_context *pipe,
                          struct pipe_transfer *transfer,
                          const struct pipe_box *box)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_transfer *xfer = ilo_transfer(transfer);

   /*
    * The staging resource is mapped persistently and coherently.  We can copy
    * without unmapping.
    */
   if (xfer->method == ILO_TRANSFER_MAP_STAGING &&
       (xfer->base.usage & PIPE_TRANSFER_FLUSH_EXPLICIT))
      copy_staging_resource(ilo, xfer, box);
}

static void
ilo_transfer_unmap(struct pipe_context *pipe,
                   struct pipe_transfer *transfer)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_transfer *xfer = ilo_transfer(transfer);

   xfer_unmap(xfer);

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_STAGING:
      if (!(xfer->base.usage & PIPE_TRANSFER_FLUSH_EXPLICIT))
         copy_staging_resource(ilo, xfer, NULL);
      pipe_resource_reference(&xfer->staging.res, NULL);
      break;
   case ILO_TRANSFER_MAP_SW_CONVERT:
   case ILO_TRANSFER_MAP_SW_ZS:
      tex_staging_sys_writeback(xfer);
      align_free(xfer->staging.sys);
      break;
   default:
      break;
   }

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
   void *ptr;

   /* note that xfer is not zero'd */
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

   if (choose_transfer_method(ilo, xfer)) {
      if (res->target == PIPE_BUFFER)
         ptr = buf_map(xfer);
      else
         ptr = tex_map(xfer);
   }
   else {
      ptr = NULL;
   }

   if (!ptr) {
      pipe_resource_reference(&xfer->base.resource, NULL);
      util_slab_free(&ilo->transfer_mempool, xfer);
      *transfer = NULL;
      return NULL;
   }

   *transfer = &xfer->base;

   return ptr;
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
