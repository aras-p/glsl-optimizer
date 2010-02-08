
#ifndef INTEL_DRM_WINSYS_H
#define INTEL_DRM_WINSYS_H

#include "i915/intel_batchbuffer.h"

#include "drm.h"
#include "intel_bufmgr.h"


/*
 * Winsys
 */


struct intel_drm_winsys
{
   struct intel_winsys base;

   boolean dump_cmd;

   int fd; /**< Drm file discriptor */

   unsigned id;

   size_t max_batch_size;

   struct {
      drm_intel_bufmgr *gem;
   } pools;
};

static INLINE struct intel_drm_winsys *
intel_drm_winsys(struct intel_winsys *iws)
{
   return (struct intel_drm_winsys *)iws;
}

struct intel_drm_winsys * intel_drm_winsys_create(int fd, unsigned pci_id);
struct pipe_fence_handle * intel_drm_fence_create(drm_intel_bo *bo);

void intel_drm_winsys_init_batchbuffer_functions(struct intel_drm_winsys *idws);
void intel_drm_winsys_init_buffer_functions(struct intel_drm_winsys *idws);
void intel_drm_winsys_init_fence_functions(struct intel_drm_winsys *idws);


/*
 * Buffer
 */


struct intel_drm_buffer {
   unsigned magic;

   drm_intel_bo *bo;

   void *ptr;
   unsigned map_count;
   boolean map_gtt;

   boolean flinked;
   unsigned flink;
};

static INLINE struct intel_drm_buffer *
intel_drm_buffer(struct intel_buffer *buffer)
{
   return (struct intel_drm_buffer *)buffer;
}

static INLINE drm_intel_bo *
intel_bo(struct intel_buffer *buffer)
{
   return intel_drm_buffer(buffer)->bo;
}

#endif
