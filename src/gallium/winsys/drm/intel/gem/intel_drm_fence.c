
#include "intel_drm_winsys.h"
#include "util/u_memory.h"
#include "util/u_atomic.h"
#include "util/u_inlines.h"

/**
 * Because gem does not have fence's we have to create our own fences.
 *
 * They work by keeping the batchbuffer around and checking if that has
 * been idled. If bo is NULL fence has expired.
 */
struct intel_drm_fence
{
   struct pipe_reference reference;
   drm_intel_bo *bo;
};


struct pipe_fence_handle *
intel_drm_fence_create(drm_intel_bo *bo)
{
   struct intel_drm_fence *fence = CALLOC_STRUCT(intel_drm_fence);

   pipe_reference_init(&fence->reference, 1);
   /* bo is null if fence already expired */
   if (bo) {
      drm_intel_bo_reference(bo);
      fence->bo = bo;
   }

   return (struct pipe_fence_handle *)fence;
}

static void
intel_drm_fence_reference(struct intel_winsys *iws,
                          struct pipe_fence_handle **ptr,
                          struct pipe_fence_handle *fence)
{
   struct intel_drm_fence *old = (struct intel_drm_fence *)*ptr;
   struct intel_drm_fence *f = (struct intel_drm_fence *)fence;

   if (pipe_reference(&((struct intel_drm_fence *)(*ptr))->reference, &f->reference)) {
      if (old->bo)
         drm_intel_bo_unreference(old->bo);
      FREE(old);
   }
   *ptr = fence;
}

static int
intel_drm_fence_signalled(struct intel_winsys *iws,
                          struct pipe_fence_handle *fence)
{
   assert(0);

   return 0;
}

static int
intel_drm_fence_finish(struct intel_winsys *iws,
                       struct pipe_fence_handle *fence)
{
   struct intel_drm_fence *f = (struct intel_drm_fence *)fence;

   /* fence already expired */
   if (!f->bo)
      return 0;

   drm_intel_bo_wait_rendering(f->bo);
   drm_intel_bo_unreference(f->bo);
   f->bo = NULL;

   return 0;
}

void
intel_drm_winsys_init_fence_functions(struct intel_drm_winsys *idws)
{
   idws->base.fence_reference = intel_drm_fence_reference;
   idws->base.fence_signalled = intel_drm_fence_signalled;
   idws->base.fence_finish = intel_drm_fence_finish;
}
