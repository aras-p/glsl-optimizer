/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2014 LunarG, Inc.
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
#define ETIME ETIMEDOUT
#endif

#include <xf86drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

#include "os/os_thread.h"
#include "state_tracker/drm_driver.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_debug.h"
#include "../intel_winsys.h"

#define BATCH_SZ (8192 * sizeof(uint32_t))

struct intel_winsys {
   int fd;
   drm_intel_bufmgr *bufmgr;
   struct intel_winsys_info info;

   /* these are protected by the mutex */
   pipe_mutex mutex;
   drm_intel_context *first_gem_ctx;
   struct drm_intel_decode *decode;
};

static drm_intel_bo *
gem_bo(const struct intel_bo *bo)
{
   return (drm_intel_bo *) bo;
}

static bool
get_param(struct intel_winsys *winsys, int param, int *value)
{
   struct drm_i915_getparam gp;
   int err;

   *value = 0;

   memset(&gp, 0, sizeof(gp));
   gp.param = param;
   gp.value = value;

   err = drmCommandWriteRead(winsys->fd, DRM_I915_GETPARAM, &gp, sizeof(gp));
   if (err) {
      *value = 0;
      return false;
   }

   return true;
}

static bool
test_address_swizzling(struct intel_winsys *winsys)
{
   drm_intel_bo *bo;
   uint32_t tiling = I915_TILING_X, swizzle;
   unsigned long pitch;

   bo = drm_intel_bo_alloc_tiled(winsys->bufmgr,
         "address swizzling test", 64, 64, 4, &tiling, &pitch, 0);
   if (bo) {
      drm_intel_bo_get_tiling(bo, &tiling, &swizzle);
      drm_intel_bo_unreference(bo);
   }
   else {
      swizzle = I915_BIT_6_SWIZZLE_NONE;
   }

   return (swizzle != I915_BIT_6_SWIZZLE_NONE);
}

static bool
test_reg_read(struct intel_winsys *winsys, uint32_t reg)
{
   uint64_t dummy;

   return !drm_intel_reg_read(winsys->bufmgr, reg, &dummy);
}

static bool
probe_winsys(struct intel_winsys *winsys)
{
   struct intel_winsys_info *info = &winsys->info;
   int val;

   /*
    * When we need the Nth vertex from a user vertex buffer, and the vertex is
    * uploaded to, say, the beginning of a bo, we want the first vertex in the
    * bo to be fetched.  One way to do this is to set the base address of the
    * vertex buffer to
    *
    *   bo->offset64 + (vb->buffer_offset - vb->stride * N).
    *
    * The second term may be negative, and we need kernel support to do that.
    *
    * This check is taken from the classic driver.  u_vbuf_upload_buffers()
    * guarantees the term is never negative, but it is good to require a
    * recent kernel.
    */
   get_param(winsys, I915_PARAM_HAS_RELAXED_DELTA, &val);
   if (!val) {
      debug_error("kernel 2.6.39 required");
      return false;
   }

   info->devid = drm_intel_bufmgr_gem_get_devid(winsys->bufmgr);

   info->max_batch_size = BATCH_SZ;

   get_param(winsys, I915_PARAM_HAS_LLC, &val);
   info->has_llc = val;
   info->has_address_swizzling = test_address_swizzling(winsys);

   winsys->first_gem_ctx = drm_intel_gem_context_create(winsys->bufmgr);
   info->has_logical_context = (winsys->first_gem_ctx != NULL);

   get_param(winsys, I915_PARAM_HAS_ALIASING_PPGTT, &val);
   info->has_ppgtt = val;

   /* test TIMESTAMP read */
   info->has_timestamp = test_reg_read(winsys, 0x2358);

   get_param(winsys, I915_PARAM_HAS_GEN7_SOL_RESET, &val);
   info->has_gen7_sol_reset = val;

   return true;
}

struct intel_winsys *
intel_winsys_create_for_fd(int fd)
{
   struct intel_winsys *winsys;

   winsys = CALLOC_STRUCT(intel_winsys);
   if (!winsys)
      return NULL;

   winsys->fd = fd;

   winsys->bufmgr = drm_intel_bufmgr_gem_init(winsys->fd, BATCH_SZ);
   if (!winsys->bufmgr) {
      debug_error("failed to create GEM buffer manager");
      FREE(winsys);
      return NULL;
   }

   pipe_mutex_init(winsys->mutex);

   if (!probe_winsys(winsys)) {
      drm_intel_bufmgr_destroy(winsys->bufmgr);
      FREE(winsys);
      return NULL;
   }

   /*
    * No need to implicitly set up a fence register for each non-linear reloc
    * entry.  When a fence register is needed for a reloc entry,
    * drm_intel_bo_emit_reloc_fence() will be called explicitly.
    *
    * intel_bo_add_reloc() currently lacks "bool fenced" for this to work.
    * But we never need a fence register on GEN4+ so we do not need to worry
    * about it yet.
    */
   drm_intel_bufmgr_gem_enable_fenced_relocs(winsys->bufmgr);

   drm_intel_bufmgr_gem_enable_reuse(winsys->bufmgr);

   return winsys;
}

void
intel_winsys_destroy(struct intel_winsys *winsys)
{
   if (winsys->decode)
      drm_intel_decode_context_free(winsys->decode);

   if (winsys->first_gem_ctx)
      drm_intel_gem_context_destroy(winsys->first_gem_ctx);

   pipe_mutex_destroy(winsys->mutex);
   drm_intel_bufmgr_destroy(winsys->bufmgr);
   FREE(winsys);
}

const struct intel_winsys_info *
intel_winsys_get_info(const struct intel_winsys *winsys)
{
   return &winsys->info;
}

struct intel_context *
intel_winsys_create_context(struct intel_winsys *winsys)
{
   drm_intel_context *gem_ctx;

   /* try the preallocated context first */
   pipe_mutex_lock(winsys->mutex);
   gem_ctx = winsys->first_gem_ctx;
   winsys->first_gem_ctx = NULL;
   pipe_mutex_unlock(winsys->mutex);

   if (!gem_ctx)
      gem_ctx = drm_intel_gem_context_create(winsys->bufmgr);

   return (struct intel_context *) gem_ctx;
}

void
intel_winsys_destroy_context(struct intel_winsys *winsys,
                             struct intel_context *ctx)
{
   drm_intel_gem_context_destroy((drm_intel_context *) ctx);
}

int
intel_winsys_read_reg(struct intel_winsys *winsys,
                      uint32_t reg, uint64_t *val)
{
   return drm_intel_reg_read(winsys->bufmgr, reg, val);
}

struct intel_bo *
intel_winsys_alloc_buffer(struct intel_winsys *winsys,
                          const char *name,
                          unsigned long size,
                          uint32_t initial_domain)
{
   const bool for_render =
      (initial_domain & (INTEL_DOMAIN_RENDER | INTEL_DOMAIN_INSTRUCTION));
   const int alignment = 4096; /* always page-aligned */
   drm_intel_bo *bo;

   if (for_render) {
      bo = drm_intel_bo_alloc_for_render(winsys->bufmgr,
            name, size, alignment);
   }
   else {
      bo = drm_intel_bo_alloc(winsys->bufmgr, name, size, alignment);
   }

   return (struct intel_bo *) bo;
}

struct intel_bo *
intel_winsys_alloc_texture(struct intel_winsys *winsys,
                           const char *name,
                           int width, int height, int cpp,
                           enum intel_tiling_mode tiling,
                           uint32_t initial_domain,
                           unsigned long *pitch)
{
   const unsigned long flags =
      (initial_domain & (INTEL_DOMAIN_RENDER | INTEL_DOMAIN_INSTRUCTION)) ?
      BO_ALLOC_FOR_RENDER : 0;
   uint32_t real_tiling = tiling;
   drm_intel_bo *bo;

   bo = drm_intel_bo_alloc_tiled(winsys->bufmgr, name,
         width, height, cpp, &real_tiling, pitch, flags);
   if (!bo)
      return NULL;

   if (real_tiling != tiling) {
      assert(!"tiling mismatch");
      drm_intel_bo_unreference(bo);
      return NULL;
   }

   return (struct intel_bo *) bo;
}

struct intel_bo *
intel_winsys_import_handle(struct intel_winsys *winsys,
                           const char *name,
                           const struct winsys_handle *handle,
                           int width, int height, int cpp,
                           enum intel_tiling_mode *tiling,
                           unsigned long *pitch)
{
   uint32_t real_tiling, swizzle;
   drm_intel_bo *bo;
   int err;

   switch (handle->type) {
   case DRM_API_HANDLE_TYPE_SHARED:
      {
         const uint32_t gem_name = handle->handle;
         bo = drm_intel_bo_gem_create_from_name(winsys->bufmgr,
               name, gem_name);
      }
      break;
   case DRM_API_HANDLE_TYPE_FD:
      {
         const int fd = (int) handle->handle;
         bo = drm_intel_bo_gem_create_from_prime(winsys->bufmgr,
               fd, height * handle->stride);
      }
      break;
   default:
      bo = NULL;
      break;
   }

   if (!bo)
      return NULL;

   err = drm_intel_bo_get_tiling(bo, &real_tiling, &swizzle);
   if (err) {
      drm_intel_bo_unreference(bo);
      return NULL;
   }

   *tiling = real_tiling;
   *pitch = handle->stride;

   return (struct intel_bo *) bo;
}

int
intel_winsys_export_handle(struct intel_winsys *winsys,
                           struct intel_bo *bo,
                           enum intel_tiling_mode tiling,
                           unsigned long pitch,
                           struct winsys_handle *handle)
{
   int err = 0;

   switch (handle->type) {
   case DRM_API_HANDLE_TYPE_SHARED:
      {
         uint32_t name;

         err = drm_intel_bo_flink(gem_bo(bo), &name);
         if (!err)
            handle->handle = name;
      }
      break;
   case DRM_API_HANDLE_TYPE_KMS:
      handle->handle = gem_bo(bo)->handle;
      break;
   case DRM_API_HANDLE_TYPE_FD:
      {
         int fd;

         err = drm_intel_bo_gem_export_to_prime(gem_bo(bo), &fd);
         if (!err)
            handle->handle = fd;
      }
      break;
   default:
      err = -EINVAL;
      break;
   }

   if (err)
      return err;

   handle->stride = pitch;

   return 0;
}

bool
intel_winsys_can_submit_bo(struct intel_winsys *winsys,
                           struct intel_bo **bo_array,
                           int count)
{
   return !drm_intel_bufmgr_check_aperture_space((drm_intel_bo **) bo_array,
                                                 count);
}

int
intel_winsys_submit_bo(struct intel_winsys *winsys,
                       enum intel_ring_type ring,
                       struct intel_bo *bo, int used,
                       struct intel_context *ctx,
                       unsigned long flags)
{
   const unsigned long exec_flags = (unsigned long) ring | flags;

   /* logical contexts are only available for the render ring */
   if (ring != INTEL_RING_RENDER)
      ctx = NULL;

   if (ctx) {
      return drm_intel_gem_bo_context_exec(gem_bo(bo),
            (drm_intel_context *) ctx, used, exec_flags);
   }
   else {
      return drm_intel_bo_mrb_exec(gem_bo(bo),
            used, NULL, 0, 0, exec_flags);
   }
}

void
intel_winsys_decode_bo(struct intel_winsys *winsys,
                       struct intel_bo *bo, int used)
{
   void *ptr;

   ptr = intel_bo_map(bo, false);
   if (!ptr) {
      debug_printf("failed to map buffer for decoding\n");
      return;
   }

   pipe_mutex_lock(winsys->mutex);

   if (!winsys->decode) {
      winsys->decode = drm_intel_decode_context_alloc(winsys->info.devid);
      if (!winsys->decode) {
         pipe_mutex_unlock(winsys->mutex);
         intel_bo_unmap(bo);
         return;
      }

      /* debug_printf()/debug_error() uses stderr by default */
      drm_intel_decode_set_output_file(winsys->decode, stderr);
   }

   /* in dwords */
   used /= 4;

   drm_intel_decode_set_batch_pointer(winsys->decode,
         ptr, gem_bo(bo)->offset64, used);

   drm_intel_decode(winsys->decode);

   pipe_mutex_unlock(winsys->mutex);

   intel_bo_unmap(bo);
}

void
intel_bo_reference(struct intel_bo *bo)
{
   drm_intel_bo_reference(gem_bo(bo));
}

void
intel_bo_unreference(struct intel_bo *bo)
{
   drm_intel_bo_unreference(gem_bo(bo));
}

void *
intel_bo_map(struct intel_bo *bo, bool write_enable)
{
   int err;

   err = drm_intel_bo_map(gem_bo(bo), write_enable);
   if (err) {
      debug_error("failed to map bo");
      return NULL;
   }

   return gem_bo(bo)->virtual;
}

void *
intel_bo_map_gtt(struct intel_bo *bo)
{
   int err;

   err = drm_intel_gem_bo_map_gtt(gem_bo(bo));
   if (err) {
      debug_error("failed to map bo");
      return NULL;
   }

   return gem_bo(bo)->virtual;
}

void *
intel_bo_map_unsynchronized(struct intel_bo *bo)
{
   int err;

   err = drm_intel_gem_bo_map_unsynchronized(gem_bo(bo));
   if (err) {
      debug_error("failed to map bo");
      return NULL;
   }

   return gem_bo(bo)->virtual;
}

void
intel_bo_unmap(struct intel_bo *bo)
{
   int err;

   err = drm_intel_bo_unmap(gem_bo(bo));
   assert(!err);
}

int
intel_bo_pwrite(struct intel_bo *bo, unsigned long offset,
                unsigned long size, const void *data)
{
   return drm_intel_bo_subdata(gem_bo(bo), offset, size, data);
}

int
intel_bo_pread(struct intel_bo *bo, unsigned long offset,
               unsigned long size, void *data)
{
   return drm_intel_bo_get_subdata(gem_bo(bo), offset, size, data);
}

int
intel_bo_add_reloc(struct intel_bo *bo, uint32_t offset,
                   struct intel_bo *target_bo, uint32_t target_offset,
                   uint32_t read_domains, uint32_t write_domain,
                   uint64_t *presumed_offset)
{
   int err;

   err = drm_intel_bo_emit_reloc(gem_bo(bo), offset,
         gem_bo(target_bo), target_offset,
         read_domains, write_domain);

   *presumed_offset = gem_bo(target_bo)->offset64 + target_offset;

   return err;
}

int
intel_bo_get_reloc_count(struct intel_bo *bo)
{
   return drm_intel_gem_bo_get_reloc_count(gem_bo(bo));
}

void
intel_bo_truncate_relocs(struct intel_bo *bo, int start)
{
   drm_intel_gem_bo_clear_relocs(gem_bo(bo), start);
}

bool
intel_bo_has_reloc(struct intel_bo *bo, struct intel_bo *target_bo)
{
   return drm_intel_bo_references(gem_bo(bo), gem_bo(target_bo));
}

int
intel_bo_wait(struct intel_bo *bo, int64_t timeout)
{
   int err;

   err = drm_intel_gem_bo_wait(gem_bo(bo), timeout);
   /* consider the bo idle on errors */
   if (err && err != -ETIME)
      err = 0;

   return err;
}
