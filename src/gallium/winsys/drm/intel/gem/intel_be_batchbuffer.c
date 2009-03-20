
#include "i915simple/i915_debug.h"
#include "intel_be_batchbuffer.h"
#include "intel_be_context.h"
#include "intel_be_device.h"
#include "intel_be_fence.h"
#include <errno.h>

#include "util/u_memory.h"

struct intel_be_batchbuffer *
intel_be_batchbuffer_alloc(struct intel_be_context *intel)
{
	struct intel_be_batchbuffer *batch = CALLOC_STRUCT(intel_be_batchbuffer);


	batch->base.buffer = NULL;
	batch->base.winsys = &intel->base;
	batch->base.map = NULL;
	batch->base.ptr = NULL;
	batch->base.size = 0;
	batch->base.actual_size = intel->device->max_batch_size;
	batch->base.relocs = 0;
	batch->base.max_relocs = 500;/*INTEL_DEFAULT_RELOCS;*/

	batch->base.map = malloc(batch->base.actual_size);
	memset(batch->base.map, 0, batch->base.actual_size);

	batch->base.ptr = batch->base.map;

	intel_be_batchbuffer_reset(batch);

	return batch;
}

void
intel_be_batchbuffer_reset(struct intel_be_batchbuffer *batch)
{
	struct intel_be_context *intel = intel_be_context(batch->base.winsys);
	struct intel_be_device *dev = intel->device;

	if (batch->bo)
		drm_intel_bo_unreference(batch->bo);

	memset(batch->base.map, 0, batch->base.actual_size);
	batch->base.ptr = batch->base.map;
	batch->base.size = batch->base.actual_size - BATCH_RESERVED;

	batch->base.relocs = 0;

	batch->bo = drm_intel_bo_alloc(dev->pools.gem,
	                               "gallium3d_batch_buffer",
	                               batch->base.actual_size, 0);
}

int
intel_be_offset_relocation(struct intel_be_batchbuffer *batch,
			   unsigned pre_add,
			   drm_intel_bo *bo,
			   uint32_t read_domains,
			   uint32_t write_domain)
{
	unsigned offset;
	int ret = 0;

	assert(batch->base.relocs < batch->base.max_relocs);

	offset = (unsigned)(batch->base.ptr - batch->base.map);

	ret = drm_intel_bo_emit_reloc(batch->bo, offset,
	                              bo, pre_add,
	                              read_domains,
	                              write_domain);

	((uint32_t*)batch->base.ptr)[0] = bo->offset + pre_add;
	batch->base.ptr += 4;

	if (!ret)
		batch->base.relocs++;

	return ret;
}

void
intel_be_batchbuffer_flush(struct intel_be_batchbuffer *batch,
			   struct intel_be_fence **fence)
{
	struct i915_batchbuffer *i915 = &batch->base;
	unsigned used = 0;
	int ret = 0;

	assert(i915_batchbuffer_space(i915) >= 0);

	used = batch->base.ptr - batch->base.map;
	assert((used & 3) == 0);

	if (used & 4) {
		i915_batchbuffer_dword(i915, (0x0<<29)|(0x4<<23)|(1<<0)); // MI_FLUSH | FLUSH_MAP_CACHE;
		i915_batchbuffer_dword(i915, (0x0<<29)|(0x0<<23)); // MI_NOOP
		i915_batchbuffer_dword(i915, (0x0<<29)|(0xA<<23)); // MI_BATCH_BUFFER_END;
	} else {
		i915_batchbuffer_dword(i915, (0x0<<29)|(0x4<<23)|(1<<0)); //MI_FLUSH | FLUSH_MAP_CACHE;
		i915_batchbuffer_dword(i915, (0x0<<29)|(0xA<<23)); // MI_BATCH_BUFFER_END;
	}

	used = batch->base.ptr - batch->base.map;

	drm_intel_bo_subdata(batch->bo, 0, used, batch->base.map);
	ret = drm_intel_bo_exec(batch->bo, used, NULL, 0, 0);

	assert(ret == 0);

	intel_be_batchbuffer_reset(batch);

	if (fence) {
		if (*fence)
			intel_be_fence_reference(fence, NULL);

		(*fence) = CALLOC_STRUCT(intel_be_fence);
		pipe_reference_init(&(*fence)->reference, 1);
		(*fence)->bo = NULL;
	}
}

void
intel_be_batchbuffer_finish(struct intel_be_batchbuffer *batch)
{

}

void
intel_be_batchbuffer_free(struct intel_be_batchbuffer *batch)
{
	if (batch->bo)
		drm_intel_bo_unreference(batch->bo);

	free(batch->base.map);
	free(batch);
}
