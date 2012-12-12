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

#include <string.h>
#include <errno.h>
#ifndef ETIME
#define ETIME ETIMEOUT
#endif

#include <xf86drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

#include "state_tracker/drm_driver.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "intel_drm_public.h"
#include "intel_winsys.h"

#define BATCH_SZ (8192 * sizeof(uint32_t))

struct intel_drm_winsys {
   struct intel_winsys base;

   int fd;
   drm_intel_bufmgr *bufmgr;
   struct drm_intel_decode *decode;

   struct intel_winsys_info info;

   drm_intel_bo **drm_bo_array;
   int array_size;
};

struct intel_drm_bo {
   struct intel_bo base;
   struct pipe_reference reference;

   drm_intel_bo *bo;
   enum intel_tiling_mode tiling;
   unsigned long pitch;
};

static inline struct intel_drm_winsys *
intel_drm_winsys(struct intel_winsys *ws)
{
   return (struct intel_drm_winsys *) ws;
}

static inline struct intel_drm_bo *
intel_drm_bo(struct intel_bo *bo)
{
   return (struct intel_drm_bo *) bo;
}

static int
intel_drm_bo_export_handle(struct intel_bo *bo, struct winsys_handle *handle)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   int err = 0;

   switch (handle->type) {
   case DRM_API_HANDLE_TYPE_SHARED:
      {
         uint32_t name;

         err = drm_intel_bo_flink(drm_bo->bo, &name);
         if (!err)
            handle->handle = name;
      }
      break;
   case DRM_API_HANDLE_TYPE_KMS:
      handle->handle = drm_bo->bo->handle;
      break;
#if 0
   case DRM_API_HANDLE_TYPE_PRIME:
      {
         int fd;

         err = drm_intel_bo_gem_export_to_prime(drm_bo->bo, &fd);
         if (!err)
            handle->handle = fd;
      }
#endif
      break;
   default:
      err = -EINVAL;
      break;
   }

   if (err)
      return err;

   handle->stride = drm_bo->pitch;

   return 0;
}

static int
intel_drm_bo_exec(struct intel_bo *bo, int used,
                  struct intel_context *ctx, unsigned long flags)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);

   if (ctx) {
      return drm_intel_gem_bo_context_exec(drm_bo->bo,
            (drm_intel_context *) ctx, used, flags);
   }
   else {
      return drm_intel_bo_mrb_exec(drm_bo->bo, used, NULL, 0, 0, flags);
   }
}

static int
intel_drm_bo_wait(struct intel_bo *bo, int64_t timeout)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   int err;

   err = drm_intel_gem_bo_wait(drm_bo->bo, timeout);
   /* consider the bo idle on errors */
   if (err && err != -ETIME)
      err = 0;

   return err;
}

static int
intel_drm_bo_emit_reloc(struct intel_bo *bo, uint32_t offset,
                        struct intel_bo *target_bo, uint32_t target_offset,
                        uint32_t read_domains, uint32_t write_domain)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   struct intel_drm_bo *target = intel_drm_bo(target_bo);

   return drm_intel_bo_emit_reloc(drm_bo->bo, offset,
         target->bo, target_offset, read_domains, write_domain);
}

static int
intel_drm_bo_get_reloc_count(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_gem_bo_get_reloc_count(drm_bo->bo);
}

static void
intel_drm_bo_clear_relocs(struct intel_bo *bo, int start)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_gem_bo_clear_relocs(drm_bo->bo, start);
}

static bool
intel_drm_bo_references(struct intel_bo *bo, struct intel_bo *target_bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   struct intel_drm_bo *target = intel_drm_bo(target_bo);

   return drm_intel_bo_references(drm_bo->bo, target->bo);
}

static int
intel_drm_bo_map(struct intel_bo *bo, bool write_enable)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_bo_map(drm_bo->bo, write_enable);
}

static int
intel_drm_bo_map_gtt(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_gem_bo_map_gtt(drm_bo->bo);
}

static int
intel_drm_bo_map_unsynchronized(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_gem_bo_map_unsynchronized(drm_bo->bo);
}

static int
intel_drm_bo_unmap(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_bo_unmap(drm_bo->bo);
}

static int
intel_drm_bo_pwrite(struct intel_bo *bo, unsigned long offset,
                    unsigned long size, const void *data)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_bo_subdata(drm_bo->bo, offset, size, data);
}

static int
intel_drm_bo_pread(struct intel_bo *bo, unsigned long offset,
                   unsigned long size, void *data)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_intel_bo_get_subdata(drm_bo->bo, offset, size, data);
}

static unsigned long
intel_drm_bo_get_size(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_bo->bo->size;
}

static unsigned long
intel_drm_bo_get_offset(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_bo->bo->offset;
}

static void *
intel_drm_bo_get_virtual(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_bo->bo->virtual;
}

static enum intel_tiling_mode
intel_drm_bo_get_tiling(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_bo->tiling;
}

static unsigned long
intel_drm_bo_get_pitch(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   return drm_bo->pitch;
}

static void
intel_drm_bo_reference(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);

   pipe_reference(NULL, &drm_bo->reference);
}

static void
intel_drm_bo_unreference(struct intel_bo *bo)
{
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);

   if (pipe_reference(&drm_bo->reference, NULL)) {
      drm_intel_bo_unreference(drm_bo->bo);
      FREE(drm_bo);
   }
}

static struct intel_drm_bo *
create_bo(void)
{
   struct intel_drm_bo *drm_bo;

   drm_bo = CALLOC_STRUCT(intel_drm_bo);
   if (!drm_bo)
      return NULL;

   pipe_reference_init(&drm_bo->reference, 1);
   drm_bo->tiling = INTEL_TILING_NONE;
   drm_bo->pitch = 0;

   drm_bo->base.reference = intel_drm_bo_reference;
   drm_bo->base.unreference = intel_drm_bo_unreference;

   drm_bo->base.get_size = intel_drm_bo_get_size;
   drm_bo->base.get_offset = intel_drm_bo_get_offset;
   drm_bo->base.get_virtual = intel_drm_bo_get_virtual;
   drm_bo->base.get_tiling = intel_drm_bo_get_tiling;
   drm_bo->base.get_pitch = intel_drm_bo_get_pitch;

   drm_bo->base.map = intel_drm_bo_map;
   drm_bo->base.map_gtt = intel_drm_bo_map_gtt;
   drm_bo->base.map_unsynchronized = intel_drm_bo_map_unsynchronized;
   drm_bo->base.unmap = intel_drm_bo_unmap;

   drm_bo->base.pwrite = intel_drm_bo_pwrite;
   drm_bo->base.pread = intel_drm_bo_pread;

   drm_bo->base.emit_reloc = intel_drm_bo_emit_reloc;
   drm_bo->base.get_reloc_count = intel_drm_bo_get_reloc_count;
   drm_bo->base.clear_relocs = intel_drm_bo_clear_relocs;
   drm_bo->base.references = intel_drm_bo_references;

   drm_bo->base.exec = intel_drm_bo_exec;
   drm_bo->base.wait = intel_drm_bo_wait;

   drm_bo->base.export_handle = intel_drm_bo_export_handle;

   return drm_bo;
}

static struct intel_bo *
intel_drm_winsys_alloc(struct intel_winsys *ws,
                       const char *name,
                       int width, int height, int cpp,
                       enum intel_tiling_mode tiling,
                       unsigned long flags)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   struct intel_drm_bo *drm_bo;
   uint32_t real_tiling = tiling;
   unsigned long pitch;

   drm_bo = create_bo();
   if (!drm_bo)
      return NULL;

   drm_bo->bo = drm_intel_bo_alloc_tiled(drm_ws->bufmgr, name,
         width, height, cpp, &real_tiling, &pitch, flags);
   if (!drm_bo->bo) {
      FREE(drm_bo);
      return NULL;
   }

   drm_bo->tiling = real_tiling;
   drm_bo->pitch = pitch;

   return &drm_bo->base;
}

static struct intel_bo *
intel_drm_winsys_alloc_buffer(struct intel_winsys *ws,
                              const char *name,
                              unsigned long size,
                              unsigned long flags)
{
   const int alignment = 4096; /* always page-aligned */
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   struct intel_drm_bo *drm_bo;

   drm_bo = create_bo();
   if (!drm_bo)
      return NULL;

   if (flags == INTEL_ALLOC_FOR_RENDER) {
      drm_bo->bo = drm_intel_bo_alloc_for_render(drm_ws->bufmgr,
            name, size, alignment);
   }
   else {
      assert(!flags);
      drm_bo->bo = drm_intel_bo_alloc(drm_ws->bufmgr, name, size, alignment);
   }

   if (!drm_bo->bo) {
      FREE(drm_bo);
      return NULL;
   }

   return &drm_bo->base;
}

static struct intel_bo *
intel_drm_winsys_import_handle(struct intel_winsys *ws,
                               const char *name,
                               int width, int height, int cpp,
                               const struct winsys_handle *handle)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   struct intel_drm_bo *drm_bo;
   const unsigned long pitch = handle->stride;
   uint32_t tiling, swizzle;
   int err;

   drm_bo = create_bo();
   if (!drm_bo)
      return NULL;

   switch (handle->type) {
   case DRM_API_HANDLE_TYPE_SHARED:
      {
         const uint32_t gem_name = handle->handle;
         drm_bo->bo = drm_intel_bo_gem_create_from_name(drm_ws->bufmgr,
               name, gem_name);
      }
      break;
#if 0
   case DRM_API_HANDLE_TYPE_PRIME:
      {
         const int fd = (int) handle->handle;
         drm_bo->bo = drm_intel_bo_gem_create_from_prime(drm_ws->bufmgr,
               fd, height * pitch);
      }
      break;
#endif
   default:
      break;
   }

   if (!drm_bo->bo) {
      FREE(drm_bo);
      return NULL;
   }

   err = drm_intel_bo_get_tiling(drm_bo->bo, &tiling, &swizzle);
   if (err) {
      drm_intel_bo_unreference(drm_bo->bo);
      FREE(drm_bo);
      return NULL;
   }

   drm_bo->tiling = tiling;
   drm_bo->pitch = pitch;

   return &drm_bo->base;
}

static int
intel_drm_winsys_check_aperture_space(struct intel_winsys *ws,
                                      struct intel_bo **bo_array,
                                      int count)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   drm_intel_bo *drm_bo_array[8];
   int i;

   if (likely(count <= Elements(drm_bo_array))) {
      for (i = 0; i < count; i++)
         drm_bo_array[i] = ((struct intel_drm_bo *) bo_array[i])->bo;

      return drm_intel_bufmgr_check_aperture_space(drm_bo_array, count);
   }

   /* resize bo array if necessary */
   if (drm_ws->array_size < count) {
      void *tmp = MALLOC(count * sizeof(*drm_ws->drm_bo_array));

      if (!tmp)
         return -1;

      FREE(drm_ws->drm_bo_array);
      drm_ws->drm_bo_array = tmp;
      drm_ws->array_size = count;
   }

   for (i = 0; i < count; i++)
      drm_ws->drm_bo_array[i] = ((struct intel_drm_bo *) bo_array[i])->bo;

   return drm_intel_bufmgr_check_aperture_space(drm_ws->drm_bo_array, count);
}

static void
intel_drm_winsys_decode_commands(struct intel_winsys *ws,
                                 struct intel_bo *bo, int used)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   struct intel_drm_bo *drm_bo = intel_drm_bo(bo);
   int err;

   if (!drm_ws->decode) {
      drm_ws->decode = drm_intel_decode_context_alloc(drm_ws->info.devid);
      if (!drm_ws->decode)
         return;

      /* debug_printf()/debug_error() uses stderr by default */
      drm_intel_decode_set_output_file(drm_ws->decode, stderr);
   }

   err = drm_intel_bo_map(drm_bo->bo, false);
   if (err) {
      debug_printf("failed to map buffer for decoding\n");
      return;
   }

   /* in dwords */
   used /= 4;

   drm_intel_decode_set_batch_pointer(drm_ws->decode,
         drm_bo->bo->virtual,
         drm_bo->bo->offset,
         used);

   drm_intel_decode(drm_ws->decode);

   drm_intel_bo_unmap(drm_bo->bo);
}

static int
intel_drm_winsys_read_reg(struct intel_winsys *ws,
                          uint32_t reg, uint64_t *val)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   return drm_intel_reg_read(drm_ws->bufmgr, reg, val);
}

static void
intel_drm_winsys_enable_reuse(struct intel_winsys *ws)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   drm_intel_bufmgr_gem_enable_reuse(drm_ws->bufmgr);
}

static void
intel_drm_winsys_destroy_context(struct intel_winsys *ws,
                                 struct intel_context *ctx)
{
   drm_intel_gem_context_destroy((drm_intel_context *) ctx);
}

static struct intel_context *
intel_drm_winsys_create_context(struct intel_winsys *ws)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);

   return (struct intel_context *)
      drm_intel_gem_context_create(drm_ws->bufmgr);
}

static const struct intel_winsys_info *
intel_drm_winsys_get_info(struct intel_winsys *ws)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);
   return &drm_ws->info;
}

static void
intel_drm_winsys_destroy(struct intel_winsys *ws)
{
   struct intel_drm_winsys *drm_ws = intel_drm_winsys(ws);

   if (drm_ws->decode)
      drm_intel_decode_context_free(drm_ws->decode);

   drm_intel_bufmgr_destroy(drm_ws->bufmgr);
   FREE(drm_ws->drm_bo_array);
   FREE(drm_ws);
}

static bool
get_param(struct intel_drm_winsys *drm_ws, int param, int *value)
{
   struct drm_i915_getparam gp;
   int err;

   *value = 0;

   memset(&gp, 0, sizeof(gp));
   gp.param = param;
   gp.value = value;

   err = drmCommandWriteRead(drm_ws->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
   if (err) {
      *value = 0;
      return false;
   }

   return true;
}

static bool
init_info(struct intel_drm_winsys *drm_ws)
{
   struct intel_winsys_info *info = &drm_ws->info;
   int val;

   /* follow the classic driver here */
   get_param(drm_ws, I915_PARAM_HAS_RELAXED_DELTA, &val);
   if (!val) {
      debug_error("kernel 2.6.39 required");
      return false;
   }

   info->devid = drm_intel_bufmgr_gem_get_devid(drm_ws->bufmgr);

   get_param(drm_ws, I915_PARAM_HAS_LLC, &val);
   info->has_llc = val;

   get_param(drm_ws, I915_PARAM_HAS_GEN7_SOL_RESET, &val);
   info->has_gen7_sol_reset = val;

   return true;
}

struct intel_winsys *
intel_drm_winsys_create(int fd)
{
   struct intel_drm_winsys *drm_ws;

   drm_ws = CALLOC_STRUCT(intel_drm_winsys);
   if (!drm_ws)
      return NULL;

   drm_ws->fd = fd;

   drm_ws->bufmgr = drm_intel_bufmgr_gem_init(drm_ws->fd, BATCH_SZ);
   if (!drm_ws->bufmgr) {
      debug_error("failed to create GEM buffer manager");
      FREE(drm_ws);
      return NULL;
   }

   if (!init_info(drm_ws)) {
      drm_intel_bufmgr_destroy(drm_ws->bufmgr);
      FREE(drm_ws);
      return NULL;
   }

   drm_intel_bufmgr_gem_enable_fenced_relocs(drm_ws->bufmgr);

   drm_ws->base.destroy = intel_drm_winsys_destroy;
   drm_ws->base.get_info = intel_drm_winsys_get_info;
   drm_ws->base.enable_reuse = intel_drm_winsys_enable_reuse;
   drm_ws->base.create_context = intel_drm_winsys_create_context;
   drm_ws->base.destroy_context = intel_drm_winsys_destroy_context;
   drm_ws->base.read_reg = intel_drm_winsys_read_reg;
   drm_ws->base.alloc = intel_drm_winsys_alloc;
   drm_ws->base.alloc_buffer = intel_drm_winsys_alloc_buffer;
   drm_ws->base.import_handle = intel_drm_winsys_import_handle;
   drm_ws->base.check_aperture_space = intel_drm_winsys_check_aperture_space;
   drm_ws->base.decode_commands = intel_drm_winsys_decode_commands;

   return &drm_ws->base;
}
