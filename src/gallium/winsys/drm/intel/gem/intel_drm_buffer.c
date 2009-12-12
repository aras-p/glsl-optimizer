
#include "intel_drm_winsys.h"
#include "util/u_memory.h"

#include "i915_drm.h"

static struct intel_buffer *
intel_drm_buffer_create(struct intel_winsys *iws,
                        unsigned size, unsigned alignment,
                        enum intel_buffer_type type)
{
   struct intel_drm_buffer *buf = CALLOC_STRUCT(intel_drm_buffer);
   struct intel_drm_winsys *idws = intel_drm_winsys(iws);
   drm_intel_bufmgr *pool;
   char *name;

   if (!buf)
      return NULL;

   buf->magic = 0xDEAD1337;
   buf->flinked = FALSE;
   buf->flink = 0;
   buf->map_gtt = FALSE;

   if (type == INTEL_NEW_TEXTURE) {
      name = "gallium3d_texture";
      pool = idws->pools.gem;
   } else if (type == INTEL_NEW_VERTEX) {
      name = "gallium3d_vertex";
      pool = idws->pools.gem;
      buf->map_gtt = TRUE;
   } else if (type == INTEL_NEW_SCANOUT) {
      name = "gallium3d_scanout";
      pool = idws->pools.gem;
      buf->map_gtt = TRUE;
   } else {
      assert(0);
      name = "gallium3d_unknown";
      pool = idws->pools.gem;
   }

   buf->bo = drm_intel_bo_alloc(pool, name, size, alignment);

   if (!buf->bo)
      goto err;

   return (struct intel_buffer *)buf;

err:
   assert(0);
   FREE(buf);
   return NULL;
}

static int
intel_drm_buffer_set_fence_reg(struct intel_winsys *iws,
                               struct intel_buffer *buffer,
                               unsigned stride,
                               enum intel_buffer_tile tile)
{
   struct intel_drm_buffer *buf = intel_drm_buffer(buffer);
   assert(I915_TILING_NONE == INTEL_TILE_NONE);
   assert(I915_TILING_X == INTEL_TILE_X);
   assert(I915_TILING_Y == INTEL_TILE_Y);

   if (tile != INTEL_TILE_NONE) {
      assert(buf->map_count == 0);
      buf->map_gtt = TRUE;
   }

   return drm_intel_bo_set_tiling(buf->bo, &tile, stride);
}

static void *
intel_drm_buffer_map(struct intel_winsys *iws,
                     struct intel_buffer *buffer,
                     boolean write)
{
   struct intel_drm_buffer *buf = intel_drm_buffer(buffer);
   drm_intel_bo *bo = intel_bo(buffer);
   int ret = 0;

   assert(bo);

   if (buf->map_count)
      goto out;

   if (buf->map_gtt)
      ret = drm_intel_gem_bo_map_gtt(bo);
   else
      ret = drm_intel_bo_map(bo, write);

   buf->ptr = bo->virtual;

   assert(ret == 0);
out:
   if (ret)
      return NULL;

   buf->map_count++;
   return buf->ptr;
}

static void
intel_drm_buffer_unmap(struct intel_winsys *iws,
                       struct intel_buffer *buffer)
{
   struct intel_drm_buffer *buf = intel_drm_buffer(buffer);

   if (--buf->map_count)
      return;

   if (buf->map_gtt)
      drm_intel_gem_bo_unmap_gtt(intel_bo(buffer));
   else
      drm_intel_bo_unmap(intel_bo(buffer));
}

static int
intel_drm_buffer_write(struct intel_winsys *iws,
                       struct intel_buffer *buffer,
                       size_t offset,
                       size_t size,
                       const void *data)
{
   struct intel_drm_buffer *buf = intel_drm_buffer(buffer);

   return drm_intel_bo_subdata(buf->bo, offset, size, (void*)data);
}

static void
intel_drm_buffer_destroy(struct intel_winsys *iws,
                         struct intel_buffer *buffer)
{
   drm_intel_bo_unreference(intel_bo(buffer));

#ifdef DEBUG
   intel_drm_buffer(buffer)->magic = 0;
   intel_drm_buffer(buffer)->bo = NULL;
#endif

   FREE(buffer);
}

void
intel_drm_winsys_init_buffer_functions(struct intel_drm_winsys *idws)
{
   idws->base.buffer_create = intel_drm_buffer_create;
   idws->base.buffer_set_fence_reg = intel_drm_buffer_set_fence_reg;
   idws->base.buffer_map = intel_drm_buffer_map;
   idws->base.buffer_unmap = intel_drm_buffer_unmap;
   idws->base.buffer_write = intel_drm_buffer_write;
   idws->base.buffer_destroy = intel_drm_buffer_destroy;
}
