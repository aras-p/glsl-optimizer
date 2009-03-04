
#ifndef INTEL_BE_FENCE_H
#define INTEL_BE_FENCE_H

#include "pipe/p_defines.h"

#include "drm.h"
#include "intel_bufmgr.h"

/**
 * Because gem does not have fence's we have to create our own fences.
 *
 * They work by keeping the batchbuffer around and checking if that has
 * been idled. If bo is NULL fence has expired.
 */
struct intel_be_fence
{
	struct pipe_reference reference;
	drm_intel_bo *bo;
};

static INLINE void
intel_be_fence_reference(struct intel_be_fence **ptr, struct intel_be_fence *f)
{
	struct intel_be_fence *old_fence = *ptr;

        if (pipe_reference((struct pipe_reference**)ptr, &f->reference)) {
		if (old_fence->bo)
			drm_intel_bo_unreference(old_fence->bo);
		free(old_fence);
	}
}

#endif
