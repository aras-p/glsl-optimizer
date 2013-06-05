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
#define ETIME ETIMEDOUT
#endif

#include <xf86drm.h>
#include <i915_drm.h>
#include <intel_bufmgr.h>

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

   struct drm_intel_decode *decode;
};

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
init_info(struct intel_winsys *winsys)
{
   struct intel_winsys_info *info = &winsys->info;
   int val;

   /* follow the classic driver here */
   get_param(winsys, I915_PARAM_HAS_RELAXED_DELTA, &val);
   if (!val) {
      debug_error("kernel 2.6.39 required");
      return false;
   }

   info->devid = drm_intel_bufmgr_gem_get_devid(winsys->bufmgr);

   get_param(winsys, I915_PARAM_HAS_LLC, &val);
   info->has_llc = val;

   get_param(winsys, I915_PARAM_HAS_GEN7_SOL_RESET, &val);
   info->has_gen7_sol_reset = val;

   info->has_address_swizzling = test_address_swizzling(winsys);

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

   if (!init_info(winsys)) {
      drm_intel_bufmgr_destroy(winsys->bufmgr);
      FREE(winsys);
      return NULL;
   }

   drm_intel_bufmgr_gem_enable_fenced_relocs(winsys->bufmgr);

   return winsys;
}

void
intel_winsys_destroy(struct intel_winsys *winsys)
{
   if (winsys->decode)
      drm_intel_decode_context_free(winsys->decode);

   drm_intel_bufmgr_destroy(winsys->bufmgr);
   FREE(winsys);
}

const struct intel_winsys_info *
intel_winsys_get_info(const struct intel_winsys *winsys)
{
   return &winsys->info;
}

void
intel_winsys_enable_reuse(struct intel_winsys *winsys)
{
   drm_intel_bufmgr_gem_enable_reuse(winsys->bufmgr);
}

struct intel_context *
intel_winsys_create_context(struct intel_winsys *winsys)
{
   return (struct intel_context *)
      drm_intel_gem_context_create(winsys->bufmgr);
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
                          unsigned long flags)
{
   const int alignment = 4096; /* always page-aligned */
   drm_intel_bo *bo;

   if (flags == INTEL_ALLOC_FOR_RENDER) {
      bo = drm_intel_bo_alloc_for_render(winsys->bufmgr,
            name, size, alignment);
   }
   else {
      assert(!flags);
      bo = drm_intel_bo_alloc(winsys->bufmgr, name, size, alignment);
   }

   return (struct intel_bo *) bo;
}

struct intel_bo *
intel_winsys_alloc_texture(struct intel_winsys *winsys,
                           const char *name,
                           int width, int height, int cpp,
                           enum intel_tiling_mode tiling,
                           unsigned long flags,
                           unsigned long *pitch)
{
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
#if 0
   case DRM_API_HANDLE_TYPE_FD:
      {
         const int fd = (int) handle->handle;
         bo = drm_intel_bo_gem_create_from_prime(winsys->bufmgr,
               fd, height * handle->stride);
      }
      break;
#endif
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

         err = drm_intel_bo_flink((drm_intel_bo *) bo, &name);
         if (!err)
            handle->handle = name;
      }
      break;
   case DRM_API_HANDLE_TYPE_KMS:
      handle->handle = ((drm_intel_bo *) bo)->handle;
      break;
#if 0
   case DRM_API_HANDLE_TYPE_FD:
      {
         int fd;

         err = drm_intel_bo_gem_export_to_prime((drm_intel_bo *) bo, &fd);
         if (!err)
            handle->handle = fd;
      }
      break;
#endif
   default:
      err = -EINVAL;
      break;
   }

   if (err)
      return err;

   handle->stride = pitch;

   return 0;
}

int
intel_winsys_check_aperture_space(struct intel_winsys *winsys,
                                  struct intel_bo **bo_array,
                                  int count)
{
   return drm_intel_bufmgr_check_aperture_space((drm_intel_bo **) bo_array,
                                                count);
}

void
intel_winsys_decode_commands(struct intel_winsys *winsys,
                             struct intel_bo *bo, int used)
{
   int err;

   if (!winsys->decode) {
      winsys->decode = drm_intel_decode_context_alloc(winsys->info.devid);
      if (!winsys->decode)
         return;

      /* debug_printf()/debug_error() uses stderr by default */
      drm_intel_decode_set_output_file(winsys->decode, stderr);
   }

   err = intel_bo_map(bo, false);
   if (err) {
      debug_printf("failed to map buffer for decoding\n");
      return;
   }

   /* in dwords */
   used /= 4;

   drm_intel_decode_set_batch_pointer(winsys->decode,
         intel_bo_get_virtual(bo), intel_bo_get_offset(bo), used);

   drm_intel_decode(winsys->decode);

   intel_bo_unmap(bo);
}

void
intel_bo_reference(struct intel_bo *bo)
{
   drm_intel_bo_reference((drm_intel_bo *) bo);
}

void
intel_bo_unreference(struct intel_bo *bo)
{
   drm_intel_bo_unreference((drm_intel_bo *) bo);
}

unsigned long
intel_bo_get_size(const struct intel_bo *bo)
{
   return ((drm_intel_bo *) bo)->size;
}

unsigned long
intel_bo_get_offset(const struct intel_bo *bo)
{
   return ((drm_intel_bo *) bo)->offset;
}

void *
intel_bo_get_virtual(const struct intel_bo *bo)
{
   return ((drm_intel_bo *) bo)->virtual;
}

int
intel_bo_map(struct intel_bo *bo, bool write_enable)
{
   return drm_intel_bo_map((drm_intel_bo *) bo, write_enable);
}

int
intel_bo_map_gtt(struct intel_bo *bo)
{
   return drm_intel_gem_bo_map_gtt((drm_intel_bo *) bo);
}

int
intel_bo_map_unsynchronized(struct intel_bo *bo)
{
   return drm_intel_gem_bo_map_unsynchronized((drm_intel_bo *) bo);
}

void
intel_bo_unmap(struct intel_bo *bo)
{
   int err;

   err = drm_intel_bo_unmap((drm_intel_bo *) bo);
   assert(!err);
}

int
intel_bo_pwrite(struct intel_bo *bo, unsigned long offset,
                unsigned long size, const void *data)
{
   return drm_intel_bo_subdata((drm_intel_bo *) bo, offset, size, data);
}

int
intel_bo_pread(struct intel_bo *bo, unsigned long offset,
               unsigned long size, void *data)
{
   return drm_intel_bo_get_subdata((drm_intel_bo *) bo, offset, size, data);
}

int
intel_bo_emit_reloc(struct intel_bo *bo, uint32_t offset,
                    struct intel_bo *target_bo, uint32_t target_offset,
                    uint32_t read_domains, uint32_t write_domain)
{
   return drm_intel_bo_emit_reloc((drm_intel_bo *) bo, offset,
         (drm_intel_bo *) target_bo, target_offset,
         read_domains, write_domain);
}

int
intel_bo_get_reloc_count(struct intel_bo *bo)
{
   return drm_intel_gem_bo_get_reloc_count((drm_intel_bo *) bo);
}

void
intel_bo_clear_relocs(struct intel_bo *bo, int start)
{
   return drm_intel_gem_bo_clear_relocs((drm_intel_bo *) bo, start);
}

bool
intel_bo_references(struct intel_bo *bo, struct intel_bo *target_bo)
{
   return drm_intel_bo_references((drm_intel_bo *) bo,
         (drm_intel_bo *) target_bo);
}

int
intel_bo_exec(struct intel_bo *bo, int used,
              struct intel_context *ctx, unsigned long flags)
{
   if (ctx) {
      return drm_intel_gem_bo_context_exec((drm_intel_bo *) bo,
            (drm_intel_context *) ctx, used, flags);
   }
   else {
      return drm_intel_bo_mrb_exec((drm_intel_bo *) bo,
            used, NULL, 0, 0, flags);
   }
}

int
intel_bo_wait(struct intel_bo *bo, int64_t timeout)
{
   int err;

   err = drm_intel_gem_bo_wait((drm_intel_bo *) bo, timeout);
   /* consider the bo idle on errors */
   if (err && err != -ETIME)
      err = 0;

   return err;
}
