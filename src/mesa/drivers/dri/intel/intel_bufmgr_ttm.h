
#ifndef INTEL_BUFMGR_TTM_H
#define INTEL_BUFMGR_TTM_H

#include "dri_bufmgr.h"

extern dri_bo *intel_ttm_bo_create_from_handle(dri_bufmgr *bufmgr, const char *name,
					       unsigned int handle);

#ifdef TTM_API
dri_fence *intel_ttm_fence_create_from_arg(dri_bufmgr *bufmgr, const char *name,
					   drm_fence_arg_t *arg);
#endif


dri_bufmgr *intel_bufmgr_ttm_init(int fd, unsigned int fence_type,
				  unsigned int fence_type_flush, int batch_size);

void
intel_ttm_enable_bo_reuse(dri_bufmgr *bufmgr);

#ifndef TTM_API
#define DRM_I915_FENCE_CLASS_ACCEL 0
#define DRM_I915_FENCE_TYPE_RW 2
#define DRM_I915_FENCE_FLAG_FLUSHED 0x01000000
#endif

#endif
