
/*
 * Authors: Jakob Bornecrantz <jakob-at-tungstengraphics.com>
 */

#include "ws_dri_fencemgr.h"
#include "intel_be_device.h"
#include "intel_be_context.h"
#include "intel_be_batchbuffer.h"

static INLINE struct intel_be_context *
intel_be_context(struct i915_winsys *sws)
{
	return (struct intel_be_context *)sws;
}

/* Simple batchbuffer interface:
 */

static struct i915_batchbuffer*
intel_i915_batch_get(struct i915_winsys *sws)
{
	struct intel_be_context *intel = intel_be_context(sws);
	return &intel->batch->base;
}

static void intel_i915_batch_reloc(struct i915_winsys *sws,
				   struct pipe_buffer *buf,
				   unsigned access_flags,
				   unsigned delta)
{
	struct intel_be_context *intel = intel_be_context(sws);

	unsigned flags = DRM_BO_FLAG_MEM_TT;
	unsigned mask = DRM_BO_MASK_MEM;

	if (access_flags & I915_BUFFER_ACCESS_WRITE) {
		flags |= DRM_BO_FLAG_WRITE;
		mask |= DRM_BO_FLAG_WRITE;
	}

	if (access_flags & I915_BUFFER_ACCESS_READ) {
		flags |= DRM_BO_FLAG_READ;
		mask |= DRM_BO_FLAG_READ;
	}

	intel_be_offset_relocation(intel->batch,
				delta,
				dri_bo(buf),
				flags,
				mask);
}

static void intel_i915_batch_flush(struct i915_winsys *sws,
				   struct pipe_fence_handle **fence)
{
	struct intel_be_context *intel = intel_be_context(sws);

	union {
		struct _DriFenceObject *dri;
		struct pipe_fence_handle *pipe;
	} fu;

	if (fence)
		assert(!*fence);

	fu.dri = intel_be_batchbuffer_flush(intel->batch);

	if (!fu.dri) {
		assert(0);
		*fence = NULL;
		return;
	}

	if (fu.dri) {
		if (fence)
			*fence = fu.pipe;
		else
			driFenceUnReference(&fu.dri);
	}

}

boolean
intel_be_init_context(struct intel_be_context *intel, struct intel_be_device *device)
{
	assert(intel);
	assert(device);

	intel->device = device;

	/* TODO move framebuffer createion to the driver */

	intel->base.batch_get = intel_i915_batch_get;
	intel->base.batch_reloc = intel_i915_batch_reloc;
	intel->base.batch_flush = intel_i915_batch_flush;

	intel->batch = intel_be_batchbuffer_alloc(intel);

	return true;
}

void
intel_be_destroy_context(struct intel_be_context *intel)
{
	intel_be_batchbuffer_free(intel->batch);
}
