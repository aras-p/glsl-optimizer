
#include "state_tracker/drm_driver.h"
#include "i915_drm_winsys.h"
#include "util/u_memory.h"

#include "i915_drm.h"

static struct i915_winsys_buffer *
i915_drm_buffer_create(struct i915_winsys *iws,
                        unsigned size, unsigned alignment,
                        enum i915_winsys_buffer_type type)
{
   struct i915_drm_buffer *buf = CALLOC_STRUCT(i915_drm_buffer);
   struct i915_drm_winsys *idws = i915_drm_winsys(iws);
   char *name;

   if (!buf)
      return NULL;

   buf->magic = 0xDEAD1337;
   buf->flinked = FALSE;
   buf->flink = 0;

   if (type == I915_NEW_TEXTURE) {
      name = "gallium3d_texture";
   } else if (type == I915_NEW_VERTEX) {
      name = "gallium3d_vertex";
   } else if (type == I915_NEW_SCANOUT) {
      name = "gallium3d_scanout";
   } else {
      assert(0);
      name = "gallium3d_unknown";
   }

   buf->bo = drm_intel_bo_alloc(idws->gem_manager, name, size, alignment);

   if (!buf->bo)
      goto err;

   return (struct i915_winsys_buffer *)buf;

err:
   assert(0);
   FREE(buf);
   return NULL;
}

static struct i915_winsys_buffer *
i915_drm_buffer_from_handle(struct i915_winsys *iws,
                             struct winsys_handle *whandle,
                             unsigned *stride)
{
   struct i915_drm_winsys *idws = i915_drm_winsys(iws);
   struct i915_drm_buffer *buf = CALLOC_STRUCT(i915_drm_buffer);
   uint32_t tile = 0, swizzle = 0;

   if (!buf)
      return NULL;

   buf->magic = 0xDEAD1337;
   buf->bo = drm_intel_bo_gem_create_from_name(idws->gem_manager, "gallium3d_from_handle", whandle->handle);
   buf->flinked = TRUE;
   buf->flink = whandle->handle;

   if (!buf->bo)
      goto err;

   drm_intel_bo_get_tiling(buf->bo, &tile, &swizzle);

   *stride = whandle->stride;

   return (struct i915_winsys_buffer *)buf;

err:
   FREE(buf);
   return NULL;
}

static boolean
i915_drm_buffer_get_handle(struct i915_winsys *iws,
                            struct i915_winsys_buffer *buffer,
                            struct winsys_handle *whandle,
                            unsigned stride)
{
   struct i915_drm_buffer *buf = i915_drm_buffer(buffer);

   if (whandle->type == DRM_API_HANDLE_TYPE_SHARED) {
      if (!buf->flinked) {
         if (drm_intel_bo_flink(buf->bo, &buf->flink))
            return FALSE;
         buf->flinked = TRUE;
      }

      whandle->handle = buf->flink;
   } else if (whandle->type == DRM_API_HANDLE_TYPE_KMS) {
      whandle->handle = buf->bo->handle;
   } else {
      assert(!"unknown usage");
      return FALSE;
   }

   whandle->stride = stride;
   return TRUE;
}

static int
i915_drm_buffer_set_fence_reg(struct i915_winsys *iws,
                               struct i915_winsys_buffer *buffer,
                               unsigned stride,
                               enum i915_winsys_buffer_tile tile)
{
   struct i915_drm_buffer *buf = i915_drm_buffer(buffer);
   assert(I915_TILING_NONE == I915_TILE_NONE);
   assert(I915_TILING_X == I915_TILE_X);
   assert(I915_TILING_Y == I915_TILE_Y);

   if (tile != I915_TILE_NONE) {
      assert(buf->map_count == 0);
   }

   return drm_intel_bo_set_tiling(buf->bo, &tile, stride);
}

static void *
i915_drm_buffer_map(struct i915_winsys *iws,
                     struct i915_winsys_buffer *buffer,
                     boolean write)
{
   struct i915_drm_buffer *buf = i915_drm_buffer(buffer);
   drm_intel_bo *bo = intel_bo(buffer);
   int ret = 0;

   assert(bo);

   if (buf->map_count)
      goto out;

   ret = drm_intel_gem_bo_map_gtt(bo);

   buf->ptr = bo->virtual;

   assert(ret == 0);
out:
   if (ret)
      return NULL;

   buf->map_count++;
   return buf->ptr;
}

static void
i915_drm_buffer_unmap(struct i915_winsys *iws,
                       struct i915_winsys_buffer *buffer)
{
   struct i915_drm_buffer *buf = i915_drm_buffer(buffer);

   if (--buf->map_count)
      return;

   drm_intel_gem_bo_unmap_gtt(intel_bo(buffer));
}

static int
i915_drm_buffer_write(struct i915_winsys *iws,
                       struct i915_winsys_buffer *buffer,
                       size_t offset,
                       size_t size,
                       const void *data)
{
   struct i915_drm_buffer *buf = i915_drm_buffer(buffer);

   return drm_intel_bo_subdata(buf->bo, offset, size, (void*)data);
}

static void
i915_drm_buffer_destroy(struct i915_winsys *iws,
                         struct i915_winsys_buffer *buffer)
{
   drm_intel_bo_unreference(intel_bo(buffer));

#ifdef DEBUG
   i915_drm_buffer(buffer)->magic = 0;
   i915_drm_buffer(buffer)->bo = NULL;
#endif

   FREE(buffer);
}

void
i915_drm_winsys_init_buffer_functions(struct i915_drm_winsys *idws)
{
   idws->base.buffer_create = i915_drm_buffer_create;
   idws->base.buffer_from_handle = i915_drm_buffer_from_handle;
   idws->base.buffer_get_handle = i915_drm_buffer_get_handle;
   idws->base.buffer_set_fence_reg = i915_drm_buffer_set_fence_reg;
   idws->base.buffer_map = i915_drm_buffer_map;
   idws->base.buffer_unmap = i915_drm_buffer_unmap;
   idws->base.buffer_write = i915_drm_buffer_write;
   idws->base.buffer_destroy = i915_drm_buffer_destroy;
}
