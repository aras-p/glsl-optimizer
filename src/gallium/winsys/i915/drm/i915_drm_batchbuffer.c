
#include "i915_drm_winsys.h"
#include "util/u_memory.h"

#include "i915_drm.h"
#include "i915/i915_debug.h"

#define BATCH_RESERVED 16

#define INTEL_DEFAULT_RELOCS 100
#define INTEL_MAX_RELOCS 400

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

#undef INTEL_RUN_SYNC

struct i915_drm_batchbuffer
{
   struct i915_winsys_batchbuffer base;

   size_t actual_size;

   drm_intel_bo *bo;
};

static INLINE struct i915_drm_batchbuffer *
i915_drm_batchbuffer(struct i915_winsys_batchbuffer *batch)
{
   return (struct i915_drm_batchbuffer *)batch;
}

static void
i915_drm_batchbuffer_reset(struct i915_drm_batchbuffer *batch)
{
   struct i915_drm_winsys *idws = i915_drm_winsys(batch->base.iws);
   int ret;

   if (batch->bo)
      drm_intel_bo_unreference(batch->bo);
   batch->bo = drm_intel_bo_alloc(idws->gem_manager,
                                  "gallium3d_batchbuffer",
                                  batch->actual_size,
                                  4096);

#ifdef INTEL_MAP_BATCHBUFFER
#ifdef INTEL_MAP_GTT
   ret = drm_intel_gem_bo_map_gtt(batch->bo);
#else
   ret = drm_intel_bo_map(batch->bo, TRUE);
#endif
   assert(ret == 0);
   batch->base.map = batch->bo->virtual;
#else
   (void)ret;
#endif

   memset(batch->base.map, 0, batch->actual_size);
   batch->base.ptr = batch->base.map;
   batch->base.size = batch->actual_size - BATCH_RESERVED;
   batch->base.relocs = 0;
}

static struct i915_winsys_batchbuffer *
i915_drm_batchbuffer_create(struct i915_winsys *iws)
{
   struct i915_drm_winsys *idws = i915_drm_winsys(iws);
   struct i915_drm_batchbuffer *batch = CALLOC_STRUCT(i915_drm_batchbuffer);

   batch->actual_size = idws->max_batch_size;

   batch->base.map = MALLOC(batch->actual_size);
   batch->base.ptr = NULL;
   batch->base.size = 0;

   batch->base.relocs = 0;
   batch->base.max_relocs = 300;/*INTEL_DEFAULT_RELOCS;*/

   batch->base.iws = iws;

   i915_drm_batchbuffer_reset(batch);

   return &batch->base;
}

static int
i915_drm_batchbuffer_reloc(struct i915_winsys_batchbuffer *ibatch,
                            struct i915_winsys_buffer *buffer,
                            enum i915_winsys_buffer_usage usage,
                            unsigned pre_add, bool fenced)
{
   struct i915_drm_batchbuffer *batch = i915_drm_batchbuffer(ibatch);
   unsigned write_domain = 0;
   unsigned read_domain = 0;
   unsigned offset;
   int ret = 0;

   assert(batch->base.relocs < batch->base.max_relocs);

   switch (usage) {
   case I915_USAGE_SAMPLER:
      write_domain = 0;
      read_domain = I915_GEM_DOMAIN_SAMPLER;
      break;
   case I915_USAGE_RENDER:
      write_domain = I915_GEM_DOMAIN_RENDER;
      read_domain = I915_GEM_DOMAIN_RENDER;
      break;
   case I915_USAGE_2D_TARGET:
      write_domain = I915_GEM_DOMAIN_RENDER;
      read_domain = I915_GEM_DOMAIN_RENDER;
      break;
   case I915_USAGE_2D_SOURCE:
      write_domain = 0;
      read_domain = I915_GEM_DOMAIN_RENDER;
      break;
   case I915_USAGE_VERTEX:
      write_domain = 0;
      read_domain = I915_GEM_DOMAIN_VERTEX;
      break;
   default:
      assert(0);
      return -1;
   }

   offset = (unsigned)(batch->base.ptr - batch->base.map);

   if (fenced)
      ret = drm_intel_bo_emit_reloc_fence(batch->bo, offset,
				    intel_bo(buffer), pre_add,
				    read_domain,
				    write_domain);
   else
      ret = drm_intel_bo_emit_reloc(batch->bo, offset,
				    intel_bo(buffer), pre_add,
				    read_domain,
				    write_domain);

   ((uint32_t*)batch->base.ptr)[0] = intel_bo(buffer)->offset + pre_add;
   batch->base.ptr += 4;

   if (!ret)
      batch->base.relocs++;

   return ret;
}

static void
i915_drm_batchbuffer_flush(struct i915_winsys_batchbuffer *ibatch,
                            struct pipe_fence_handle **fence)
{
   struct i915_drm_batchbuffer *batch = i915_drm_batchbuffer(ibatch);
   unsigned used;
   int ret;

   /* MI_BATCH_BUFFER_END */
   i915_winsys_batchbuffer_dword_unchecked(ibatch, (0xA<<23));

   used = batch->base.ptr - batch->base.map;
   if (used & 4) {
      /* MI_NOOP */
      i915_winsys_batchbuffer_dword_unchecked(ibatch, 0);
      used += 4;
   }

   /* Do the sending to HW */
   ret = drm_intel_bo_subdata(batch->bo, 0, used, batch->base.map);
   if (ret == 0 && i915_drm_winsys(ibatch->iws)->send_cmd)
      ret = drm_intel_bo_exec(batch->bo, used, NULL, 0, 0);

   if (ret != 0 || i915_drm_winsys(ibatch->iws)->dump_cmd) {
      i915_dump_batchbuffer(ibatch);
      assert(ret == 0);
   }

#ifdef INTEL_RUN_SYNC
   drm_intel_bo_wait_rendering(batch->bo);
#endif

   if (fence) {
      ibatch->iws->fence_reference(ibatch->iws, fence, NULL);

#ifdef INTEL_RUN_SYNC
      /* we run synced to GPU so just pass null */
      (*fence) = i915_drm_fence_create(NULL);
#else
      (*fence) = i915_drm_fence_create(batch->bo);
#endif
   }

   i915_drm_batchbuffer_reset(batch);
}

static void
i915_drm_batchbuffer_destroy(struct i915_winsys_batchbuffer *ibatch)
{
   struct i915_drm_batchbuffer *batch = i915_drm_batchbuffer(ibatch);

   if (batch->bo)
      drm_intel_bo_unreference(batch->bo);

   FREE(batch->base.map);
   FREE(batch);
}

void i915_drm_winsys_init_batchbuffer_functions(struct i915_drm_winsys *idws)
{
   idws->base.batchbuffer_create = i915_drm_batchbuffer_create;
   idws->base.batchbuffer_reloc = i915_drm_batchbuffer_reloc;
   idws->base.batchbuffer_flush = i915_drm_batchbuffer_flush;
   idws->base.batchbuffer_destroy = i915_drm_batchbuffer_destroy;
}
