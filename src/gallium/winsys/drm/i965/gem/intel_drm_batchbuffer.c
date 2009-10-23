
#include "intel_drm_winsys.h"
#include "util/u_memory.h"

#include "i915_drm.h"

#define BATCH_RESERVED 16

#define INTEL_DEFAULT_RELOCS 100
#define INTEL_MAX_RELOCS 400

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

#undef INTEL_RUN_SYNC
#undef INTEL_MAP_BATCHBUFFER
#undef INTEL_MAP_GTT
#define INTEL_ALWAYS_FLUSH

struct intel_drm_batchbuffer
{
   struct intel_batchbuffer base;

   size_t actual_size;

   drm_intel_bo *bo;
};

static INLINE struct intel_drm_batchbuffer *
intel_drm_batchbuffer(struct intel_batchbuffer *batch)
{
   return (struct intel_drm_batchbuffer *)batch;
}

static void
intel_drm_batchbuffer_reset(struct intel_drm_batchbuffer *batch)
{
   struct intel_drm_winsys *idws = intel_drm_winsys(batch->base.iws);
   int ret;

   if (batch->bo)
      drm_intel_bo_unreference(batch->bo);
   batch->bo = drm_intel_bo_alloc(idws->pools.gem,
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

static struct intel_batchbuffer *
intel_drm_batchbuffer_create(struct intel_winsys *iws)
{
   struct intel_drm_winsys *idws = intel_drm_winsys(iws);
   struct intel_drm_batchbuffer *batch = CALLOC_STRUCT(intel_drm_batchbuffer);

   batch->actual_size = idws->max_batch_size;

#ifdef INTEL_MAP_BATCHBUFFER
   batch->base.map = NULL;
#else
   batch->base.map = MALLOC(batch->actual_size);
#endif
   batch->base.ptr = NULL;
   batch->base.size = 0;

   batch->base.relocs = 0;
   batch->base.max_relocs = 300;/*INTEL_DEFAULT_RELOCS;*/

   batch->base.iws = iws;

   intel_drm_batchbuffer_reset(batch);

   return &batch->base;
}

static int
intel_drm_batchbuffer_reloc(struct intel_batchbuffer *ibatch,
                            struct intel_buffer *buffer,
                            enum intel_buffer_usage usage,
                            unsigned pre_add)
{
   struct intel_drm_batchbuffer *batch = intel_drm_batchbuffer(ibatch);
   unsigned write_domain = 0;
   unsigned read_domain = 0;
   unsigned offset;
   int ret = 0;

   assert(batch->base.relocs < batch->base.max_relocs);

   if (usage == INTEL_USAGE_SAMPLER) {
      write_domain = 0;
      read_domain = I915_GEM_DOMAIN_SAMPLER;

   } else if (usage == INTEL_USAGE_RENDER) {
      write_domain = I915_GEM_DOMAIN_RENDER;
      read_domain = I915_GEM_DOMAIN_RENDER;

   } else if (usage == INTEL_USAGE_2D_TARGET) {
      write_domain = I915_GEM_DOMAIN_RENDER;
      read_domain = I915_GEM_DOMAIN_RENDER;

   } else if (usage == INTEL_USAGE_2D_SOURCE) {
      write_domain = 0;
      read_domain = I915_GEM_DOMAIN_RENDER;

   } else if (usage == INTEL_USAGE_VERTEX) {
      write_domain = 0;
      read_domain = I915_GEM_DOMAIN_VERTEX;

   } else {
      assert(0);
      return -1;
   }

   offset = (unsigned)(batch->base.ptr - batch->base.map);

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
intel_drm_batchbuffer_flush(struct intel_batchbuffer *ibatch,
                            struct pipe_fence_handle **fence)
{
   struct intel_drm_batchbuffer *batch = intel_drm_batchbuffer(ibatch);
   unsigned used = 0;
   int ret = 0;
   int i;

   assert(intel_batchbuffer_space(ibatch) >= 0);

   used = batch->base.ptr - batch->base.map;
   assert((used & 3) == 0);


#ifdef INTEL_ALWAYS_FLUSH
   /* MI_FLUSH | FLUSH_MAP_CACHE */
   intel_batchbuffer_dword(ibatch, (0x4<<23)|(1<<0));
   used += 4;
#endif

   if ((used & 4) == 0) {
      /* MI_NOOP */
      intel_batchbuffer_dword(ibatch, 0);
   }
   /* MI_BATCH_BUFFER_END */
   intel_batchbuffer_dword(ibatch, (0xA<<23));

   used = batch->base.ptr - batch->base.map;
   assert((used & 4) == 0);

#ifdef INTEL_MAP_BATCHBUFFER
#ifdef INTEL_MAP_GTT
   drm_intel_gem_bo_unmap_gtt(batch->bo);
#else
   drm_intel_bo_unmap(batch->bo);
#endif
#else
   drm_intel_bo_subdata(batch->bo, 0, used, batch->base.map);
#endif

   /* Do the sending to HW */
   ret = drm_intel_bo_exec(batch->bo, used, NULL, 0, 0);
   assert(ret == 0);

   if (intel_drm_winsys(ibatch->iws)->dump_cmd) {
      unsigned *ptr;
      drm_intel_bo_map(batch->bo, FALSE);
      ptr = (unsigned*)batch->bo->virtual;

      debug_printf("%s:\n", __func__);
      for (i = 0; i < used / 4; i++, ptr++) {
         debug_printf("\t%08x:    %08x\n", i*4, *ptr);
      }

      drm_intel_bo_unmap(batch->bo);
   } else {
#ifdef INTEL_RUN_SYNC
      drm_intel_bo_map(batch->bo, FALSE);
      drm_intel_bo_unmap(batch->bo);
#endif
   }

   if (fence) {
      ibatch->iws->fence_reference(ibatch->iws, fence, NULL);

#ifdef INTEL_RUN_SYNC
      /* we run synced to GPU so just pass null */
      (*fence) = intel_drm_fence_create(NULL);
#else
      (*fence) = intel_drm_fence_create(batch->bo);
#endif
   }

   intel_drm_batchbuffer_reset(batch);
}

static void
intel_drm_batchbuffer_destroy(struct intel_batchbuffer *ibatch)
{
   struct intel_drm_batchbuffer *batch = intel_drm_batchbuffer(ibatch);

   if (batch->bo)
      drm_intel_bo_unreference(batch->bo);

#ifndef INTEL_MAP_BATCHBUFFER
   FREE(batch->base.map);
#endif
   FREE(batch);
}

void intel_drm_winsys_init_batchbuffer_functions(struct intel_drm_winsys *idws)
{
   idws->base.batchbuffer_create = intel_drm_batchbuffer_create;
   idws->base.batchbuffer_reloc = intel_drm_batchbuffer_reloc;
   idws->base.batchbuffer_flush = intel_drm_batchbuffer_flush;
   idws->base.batchbuffer_destroy = intel_drm_batchbuffer_destroy;
}
