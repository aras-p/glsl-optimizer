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

#include "ilo_cp.h"
#include "ilo_context.h"
#include "ilo_resource.h"
#include "ilo_transfer.h"

enum ilo_transfer_map_method {
   ILO_TRANSFER_MAP_DIRECT,
   ILO_TRANSFER_MAP_STAGING_SYS,
};

struct ilo_transfer {
   struct pipe_transfer base;

   enum ilo_transfer_map_method method;
   void *ptr;

   void *staging_sys;
};

static inline struct ilo_transfer *
ilo_transfer(struct pipe_transfer *transfer)
{
   return (struct ilo_transfer *) transfer;
}

static void
ilo_transfer_inline_write(struct pipe_context *pipe,
                          struct pipe_resource *r,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box,
                          const void *data,
                          unsigned stride,
                          unsigned layer_stride)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_resource *res = ilo_resource(r);
   int offset, size;
   bool will_be_busy;

   /*
    * Fall back to map(), memcpy(), and unmap().  We use this path for
    * unsynchronized write, as the buffer is likely to be busy and pwrite()
    * will stall.
    */
   if (unlikely(res->base.target != PIPE_BUFFER) ||
       (usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
      u_default_transfer_inline_write(pipe, r,
            level, usage, box, data, stride, layer_stride);

      return;
   }

   /*
    * XXX With hardware context support, the bo may be needed by GPU without
    * being referenced by ilo->cp->bo.  We have to flush unconditionally, and
    * that is bad.
    */
   if (ilo->cp->hw_ctx)
      ilo_cp_flush(ilo->cp);

   will_be_busy = ilo->cp->bo->references(ilo->cp->bo, res->bo);

   /* see if we can avoid stalling */
   if (will_be_busy || intel_bo_is_busy(res->bo)) {
      bool will_stall = true;

      if (usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) {
         /* old data not needed so discard the old bo to avoid stalling */
         if (ilo_resource_alloc_bo(res))
            will_stall = false;
      }
      else {
         /*
          * We could allocate a temporary bo to hold the data and emit
          * pipelined copy blit to move them to res->bo.  But for now, do
          * nothing.
          */
      }

      /* flush to make bo busy (so that pwrite() stalls as it should be) */
      if (will_stall && will_be_busy)
         ilo_cp_flush(ilo->cp);
   }

   /* for PIPE_BUFFERs, conversion should not be needed */
   assert(res->bo_format == res->base.format);

   /* they should specify just an offset and a size */
   assert(level == 0);
   assert(box->y == 0);
   assert(box->z == 0);
   assert(box->height == 1);
   assert(box->depth == 1);
   offset = box->x;
   size = box->width;

   res->bo->pwrite(res->bo, offset, size, data);
}

static void
transfer_unmap_sys_convert(enum pipe_format dst_fmt,
                           const struct pipe_transfer *dst_xfer,
                           void *dst,
                           enum pipe_format src_fmt,
                           const struct pipe_transfer *src_xfer,
                           const void *src)
{
   int i;

   switch (src_fmt) {
   case PIPE_FORMAT_ETC1_RGB8:
      assert(dst_fmt == PIPE_FORMAT_R8G8B8X8_UNORM);

      for (i = 0; i < dst_xfer->box.depth; i++) {
         util_format_etc1_rgb8_unpack_rgba_8unorm(dst,
               dst_xfer->stride, src, src_xfer->stride,
               dst_xfer->box.width, dst_xfer->box.height);

         dst += dst_xfer->layer_stride;
         src += src_xfer->layer_stride;
      }
      break;
   default:
      assert(!"unable to convert the staging data");
      break;
   }
}

static void
transfer_unmap_sys(struct ilo_context *ilo,
                   struct ilo_resource *res,
                   struct ilo_transfer *xfer)
{
   const void *src = xfer->ptr;
   struct pipe_transfer *dst_xfer;
   void *dst;

   dst = ilo->base.transfer_map(&ilo->base,
         xfer->base.resource, xfer->base.level,
         PIPE_TRANSFER_WRITE |
         PIPE_TRANSFER_MAP_DIRECTLY |
         PIPE_TRANSFER_DISCARD_RANGE,
         &xfer->base.box, &dst_xfer);
   if (!dst_xfer) {
      ilo_err("failed to map resource for moving staging data\n");
      FREE(xfer->staging_sys);
      return;
   }

   if (likely(res->bo_format != res->base.format)) {
      transfer_unmap_sys_convert(res->bo_format, dst_xfer, dst,
            res->base.format, &xfer->base, src);
   }
   else {
      util_copy_box(dst, res->bo_format,
            dst_xfer->stride, dst_xfer->layer_stride, 0, 0, 0,
            dst_xfer->box.width, dst_xfer->box.height, dst_xfer->box.depth,
            src, xfer->base.stride, xfer->base.layer_stride, 0, 0, 0);
   }

   ilo->base.transfer_unmap(&ilo->base, dst_xfer);
   FREE(xfer->staging_sys);
}

static bool
transfer_map_sys(struct ilo_context *ilo,
                 struct ilo_resource *res,
                 struct ilo_transfer *xfer)
{
   const struct pipe_box *box = &xfer->base.box;
   const size_t stride = util_format_get_stride(res->base.format, box->width);
   const size_t size =
      util_format_get_2d_size(res->base.format, stride, box->height);
   bool read_back = false;

   if (xfer->base.usage & PIPE_TRANSFER_READ) {
      read_back = true;
   }
   else if (xfer->base.usage & PIPE_TRANSFER_WRITE) {
      const unsigned discard_flags =
         (PIPE_TRANSFER_DISCARD_RANGE | PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE);

      if (!(xfer->base.usage & discard_flags))
         read_back = true;
   }

   /* TODO */
   if (read_back)
      return false;

   xfer->staging_sys = MALLOC(size * box->depth);
   if (!xfer->staging_sys)
      return false;

   xfer->base.stride = stride;
   xfer->base.layer_stride = size;
   xfer->ptr = xfer->staging_sys;

   return true;
}

static void
transfer_unmap_direct(struct ilo_context *ilo,
                      struct ilo_resource *res,
                      struct ilo_transfer *xfer)
{
   res->bo->unmap(res->bo);
}

static bool
transfer_map_direct(struct ilo_context *ilo,
                    struct ilo_resource *res,
                    struct ilo_transfer *xfer)
{
   int x, y, err;

   if (xfer->base.usage & PIPE_TRANSFER_UNSYNCHRONIZED)
      err = res->bo->map_unsynchronized(res->bo);
   /* prefer map() when there is the last-level cache */
   else if (res->tiling == INTEL_TILING_NONE &&
            (ilo->dev->has_llc || (xfer->base.usage & PIPE_TRANSFER_READ)))
      err = res->bo->map(res->bo, (xfer->base.usage & PIPE_TRANSFER_WRITE));
   else
      err = res->bo->map_gtt(res->bo);

   if (err)
      return false;

   /* note that stride is for a block row, not a texel row */
   xfer->base.stride = res->bo_stride;

   /*
    * we can walk through layers when the resource is a texture array or
    * when this is the first level of a 3D texture being mapped
    */
   if (res->base.array_size > 1 ||
       (res->base.target == PIPE_TEXTURE_3D && xfer->base.level == 0)) {
      const unsigned qpitch = res->slice_offsets[xfer->base.level][1].y -
         res->slice_offsets[xfer->base.level][0].y;

      assert(qpitch % res->block_height == 0);
      xfer->base.layer_stride = (qpitch / res->block_height) * xfer->base.stride;
   }
   else {
      xfer->base.layer_stride = 0;
   }

   x = res->slice_offsets[xfer->base.level][xfer->base.box.z].x;
   y = res->slice_offsets[xfer->base.level][xfer->base.box.z].y;

   x += xfer->base.box.x;
   y += xfer->base.box.y;

   /* in blocks */
   assert(x % res->block_width == 0 && y % res->block_height == 0);
   x /= res->block_width;
   y /= res->block_height;

   xfer->ptr = res->bo->get_virtual(res->bo);
   xfer->ptr += y * res->bo_stride + x * res->bo_cpp;

   return true;
}

/**
 * Choose the best mapping method, depending on the transfer usage and whether
 * the bo is busy.
 */
static bool
transfer_map_choose_method(struct ilo_context *ilo,
                           struct ilo_resource *res,
                           struct ilo_transfer *xfer)
{
   bool will_be_busy, will_stall;

   /* need to convert on-the-fly */
   if (res->bo_format != res->base.format &&
       !(xfer->base.usage & PIPE_TRANSFER_MAP_DIRECTLY)) {
      xfer->method = ILO_TRANSFER_MAP_STAGING_SYS;

      return true;
   }

   xfer->method = ILO_TRANSFER_MAP_DIRECT;

   /* unsynchronized map does not stall */
   if (xfer->base.usage & PIPE_TRANSFER_UNSYNCHRONIZED)
      return true;

   will_be_busy = ilo->cp->bo->references(ilo->cp->bo, res->bo);
   if (!will_be_busy) {
      /*
       * XXX With hardware context support, the bo may be needed by GPU
       * without being referenced by ilo->cp->bo.  We have to flush
       * unconditionally, and that is bad.
       */
      if (ilo->cp->hw_ctx)
         ilo_cp_flush(ilo->cp);

      if (!intel_bo_is_busy(res->bo))
         return true;
   }

   /* bo is busy and mapping it will stall */
   will_stall = true;

   if (xfer->base.usage & PIPE_TRANSFER_MAP_DIRECTLY) {
      /* nothing we can do */
   }
   else if (xfer->base.usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) {
      /* discard old bo and allocate a new one for mapping */
      if (ilo_resource_alloc_bo(res))
         will_stall = false;
   }
   else if (xfer->base.usage & PIPE_TRANSFER_FLUSH_EXPLICIT) {
      /*
       * We could allocate and return a system buffer here.  When a region of
       * the buffer is explicitly flushed, we pwrite() the region to a
       * temporary bo and emit pipelined copy blit.
       *
       * For now, do nothing.
       */
   }
   else if (xfer->base.usage & PIPE_TRANSFER_DISCARD_RANGE) {
      /*
       * We could allocate a temporary bo for mapping, and emit pipelined copy
       * blit upon unmapping.
       *
       * For now, do nothing.
       */
   }

   if (will_stall) {
      if (xfer->base.usage & PIPE_TRANSFER_DONTBLOCK)
         return false;

      /* flush to make bo busy (so that map() stalls as it should be) */
      if (will_be_busy)
         ilo_cp_flush(ilo->cp);
   }

   return true;
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
   struct ilo_resource *res = ilo_resource(transfer->resource);
   struct ilo_transfer *xfer = ilo_transfer(transfer);

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_DIRECT:
      transfer_unmap_direct(ilo, res, xfer);
      break;
   case ILO_TRANSFER_MAP_STAGING_SYS:
      transfer_unmap_sys(ilo, res, xfer);
      break;
   default:
      assert(!"unknown mapping method");
      break;
   }

   pipe_resource_reference(&xfer->base.resource, NULL);
   FREE(xfer);
}

static void *
ilo_transfer_map(struct pipe_context *pipe,
                 struct pipe_resource *r,
                 unsigned level,
                 unsigned usage,
                 const struct pipe_box *box,
                 struct pipe_transfer **transfer)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_resource *res = ilo_resource(r);
   struct ilo_transfer *xfer;
   int ok;

   xfer = MALLOC_STRUCT(ilo_transfer);
   if (!xfer) {
      *transfer = NULL;
      return NULL;
   }

   xfer->base.resource = NULL;
   pipe_resource_reference(&xfer->base.resource, &res->base);
   xfer->base.level = level;
   xfer->base.usage = usage;
   xfer->base.box = *box;

   ok = transfer_map_choose_method(ilo, res, xfer);
   if (ok) {
      switch (xfer->method) {
      case ILO_TRANSFER_MAP_DIRECT:
         ok = transfer_map_direct(ilo, res, xfer);
         break;
      case ILO_TRANSFER_MAP_STAGING_SYS:
         ok = transfer_map_sys(ilo, res, xfer);
         break;
      default:
         assert(!"unknown mapping method");
         ok = false;
         break;
      }
   }

   if (!ok) {
      pipe_resource_reference(&xfer->base.resource, NULL);
      FREE(xfer);

      *transfer = NULL;

      return NULL;
   }

   *transfer = &xfer->base;

   return xfer->ptr;
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
