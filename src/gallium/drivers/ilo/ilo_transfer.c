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
   /* map() / map_gtt() / map_unsynchronized() */
   ILO_TRANSFER_MAP_CPU,
   ILO_TRANSFER_MAP_GTT,
   ILO_TRANSFER_MAP_UNSYNC,

   /* use staging system buffer */
   ILO_TRANSFER_MAP_SW_CONVERT,
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

static bool
is_bo_busy(struct ilo_context *ilo, struct intel_bo *bo, bool *need_flush)
{
   const bool referenced = ilo->cp->bo->references(ilo->cp->bo, bo);

   if (need_flush)
      *need_flush = referenced;

   if (referenced)
      return true;

   /*
    * XXX With hardware context support, the bo may be needed by GPU
    * without being referenced by ilo->cp->bo.  We have to flush
    * unconditionally, and that is bad.
    */
   if (ilo->cp->hw_ctx)
      ilo_cp_flush(ilo->cp);

   return intel_bo_is_busy(bo);
}

static bool
map_bo_for_transfer(struct ilo_context *ilo, struct intel_bo *bo,
                    const struct ilo_transfer *xfer)
{
   int err;

   switch (xfer->method) {
   case ILO_TRANSFER_MAP_CPU:
      err = bo->map(bo, (xfer->base.usage & PIPE_TRANSFER_WRITE));
      break;
   case ILO_TRANSFER_MAP_GTT:
      err = bo->map_gtt(bo);
      break;
   case ILO_TRANSFER_MAP_UNSYNC:
      err = bo->map_unsynchronized(bo);
      break;
   default:
      assert(!"unknown mapping method");
      err = -1;
      break;
   }

   return !err;
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
             (buf && ilo_buffer_alloc_bo(buf)))
            will_stall = false;
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
            ilo_cp_flush(ilo->cp);
      }
   }

   if (tex && !(usage & PIPE_TRANSFER_MAP_DIRECTLY)) {
      /* need to convert on-the-fly */
      if (tex->bo_format != tex->base.format)
         xfer->method = ILO_TRANSFER_MAP_SW_CONVERT;
   }

   return true;
}

static unsigned
tex_get_box_offset(const struct ilo_texture *tex, unsigned level,
                   const struct pipe_box *box)
{
   unsigned x, y;

   x = tex->slice_offsets[level][box->z].x;
   y = tex->slice_offsets[level][box->z].y;

   x += box->x;
   y += box->y;

   /* in blocks */
   assert(x % tex->block_width == 0 && y % tex->block_height == 0);
   x /= tex->block_width;
   y /= tex->block_height;

   return y * tex->bo_stride + x * tex->bo_cpp;
}

static unsigned
tex_get_slice_stride(const struct ilo_texture *tex, unsigned level)
{
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

   qpitch = tex->slice_offsets[level][1].y - tex->slice_offsets[level][0].y;
   assert(qpitch % tex->block_height == 0);

   return (qpitch / tex->block_height) * tex->bo_stride;
}

static void
tex_staging_sys_convert_write(enum pipe_format dst_fmt,
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
tex_staging_sys_unmap(struct ilo_context *ilo,
                      struct ilo_texture *tex,
                      struct ilo_transfer *xfer)
{
   const void *src = xfer->ptr;
   struct pipe_transfer *dst_xfer;
   void *dst;

   if (!(xfer->base.usage & PIPE_TRANSFER_WRITE)) {
      FREE(xfer->staging_sys);
      return;
   }

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

   if (likely(tex->bo_format != tex->base.format)) {
      tex_staging_sys_convert_write(tex->bo_format, dst_xfer, dst,
            tex->base.format, &xfer->base, src);
   }
   else {
      util_copy_box(dst, tex->bo_format,
            dst_xfer->stride, dst_xfer->layer_stride, 0, 0, 0,
            dst_xfer->box.width, dst_xfer->box.height, dst_xfer->box.depth,
            src, xfer->base.stride, xfer->base.layer_stride, 0, 0, 0);
   }

   ilo->base.transfer_unmap(&ilo->base, dst_xfer);
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
   bool read_back = false;

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

   /* TODO */
   return false;
}

static void
tex_direct_unmap(struct ilo_context *ilo,
                 struct ilo_texture *tex,
                 struct ilo_transfer *xfer)
{
   tex->bo->unmap(tex->bo);
}

static bool
tex_direct_map(struct ilo_context *ilo,
               struct ilo_texture *tex,
               struct ilo_transfer *xfer)
{
   if (!map_bo_for_transfer(ilo, tex->bo, xfer))
      return false;

   /* note that stride is for a block row, not a texel row */
   xfer->base.stride = tex->bo_stride;

   /* slice stride is not always available */
   if (xfer->base.box.depth > 1)
      xfer->base.layer_stride = tex_get_slice_stride(tex, xfer->base.level);
   else
      xfer->base.layer_stride = 0;

   xfer->ptr = tex->bo->get_virtual(tex->bo);
   xfer->ptr += tex_get_box_offset(tex, xfer->base.level, &xfer->base.box);

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

   if (!map_bo_for_transfer(ilo, buf->bo, xfer))
      return false;

   assert(xfer->base.level == 0);
   assert(xfer->base.box.y == 0);
   assert(xfer->base.box.z == 0);
   assert(xfer->base.box.height == 1);
   assert(xfer->base.box.depth == 1);

   xfer->base.stride = 0;
   xfer->base.layer_stride = 0;

   xfer->ptr = buf->bo->get_virtual(buf->bo);
   xfer->ptr += xfer->base.box.x;

   return true;
}

static void
buf_unmap(struct ilo_context *ilo, struct ilo_transfer *xfer)
{
   struct ilo_buffer *buf = ilo_buffer(xfer->base.resource);

   buf->bo->unmap(buf->bo);
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
         if (ilo_buffer_alloc_bo(buf))
            will_stall = false;
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
         ilo_cp_flush(ilo->cp);
   }

   buf->bo->pwrite(buf->bo, offset, size, data);
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
   FREE(xfer);
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

   xfer = MALLOC_STRUCT(ilo_transfer);
   if (!xfer) {
      *transfer = NULL;
      return NULL;
   }

   xfer->base.resource = NULL;
   pipe_resource_reference(&xfer->base.resource, res);
   xfer->base.level = level;
   xfer->base.usage = usage;
   xfer->base.box = *box;

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
