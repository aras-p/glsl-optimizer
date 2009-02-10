
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
	uint32_t refcount;
	drm_intel_bo *bo;
};

static INLINE void
intel_be_fence_reference(struct intel_be_fence *f)
{
	f->refcount++;
}

static INLINE void
intel_be_fence_unreference(struct intel_be_fence *f)
{
	if (!--f->refcount) {
		if (f->bo)
			drm_intel_bo_unreference(f->bo);
		free(f);
	}
}

#endif
