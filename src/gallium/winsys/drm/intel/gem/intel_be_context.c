
#include "pipe/p_screen.h"

#include "softpipe/sp_winsys.h"

#include "intel_be_device.h"
#include "intel_be_context.h"
#include "intel_be_batchbuffer.h"

#include "i915_drm.h"

#include "intel_be_api.h"

static struct i915_batchbuffer *
intel_be_batch_get(struct i915_winsys *sws)
{
	struct intel_be_context *intel = intel_be_context(sws);
	return &intel->batch->base;
}

static void
intel_be_batch_reloc(struct i915_winsys *sws,
		     struct pipe_buffer *buf,
		     unsigned access_flags,
		     unsigned delta)
{
	struct intel_be_context *intel = intel_be_context(sws);
	drm_intel_bo *bo = intel_bo(buf);
	int ret;
	uint32_t read = 0;
	uint32_t write = 0;

	if (access_flags & I915_BUFFER_ACCESS_WRITE) {
		write = I915_GEM_DOMAIN_RENDER;
		read = I915_GEM_DOMAIN_RENDER;
	}

	if (access_flags & I915_BUFFER_ACCESS_READ) {
		read |= I915_GEM_DOMAIN_VERTEX;
	}

	ret = intel_be_offset_relocation(intel->batch,
	                                 delta,
	                                 bo,
	                                 read,
	                                 write);
    assert(ret == 0);

	/* TODO change return type */
	/* return ret; */
}

static void
intel_be_batch_flush(struct i915_winsys *sws,
		     struct pipe_fence_handle **fence)
{
	struct intel_be_context *intel = intel_be_context(sws);
	struct intel_be_fence **f = (struct intel_be_fence **)fence;

	if (fence && *fence)
		assert(0);

	intel_be_batchbuffer_flush(intel->batch, f);
}


/*
 * Misc functions.
 */

static void
intel_be_destroy_context(struct i915_winsys *winsys)
{
	struct intel_be_context *intel = intel_be_context(winsys);

	intel_be_batchbuffer_free(intel->batch);

	free(intel);
}

boolean
intel_be_init_context(struct intel_be_context *intel, struct intel_be_device *device)
{
	assert(intel);
	assert(device);
	intel->device = device;

	intel->base.batch_get = intel_be_batch_get;
	intel->base.batch_reloc = intel_be_batch_reloc;
	intel->base.batch_flush = intel_be_batch_flush;

	intel->base.destroy = intel_be_destroy_context;

	intel->batch = intel_be_batchbuffer_alloc(intel);

	return true;
}

struct pipe_context *
intel_be_create_context(struct pipe_screen *screen)
{
	struct intel_be_context *intel;
	struct pipe_context *pipe;
	struct intel_be_device *device = intel_be_device(screen->winsys);

	intel = (struct intel_be_context *)malloc(sizeof(*intel));
	memset(intel, 0, sizeof(*intel));

	intel_be_init_context(intel, device);

	if (device->softpipe)
		pipe = softpipe_create(screen);
	else
		pipe = i915_create_context(screen, &device->base, &intel->base);

	if (pipe)
		pipe->priv = intel;

	return pipe;
}
