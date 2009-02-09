
#ifndef INTEL_BE_BATCHBUFFER_H
#define INTEL_BE_BATCHBUFFER_H

#include "i915simple/i915_batch.h"

#include "drm.h"
#include "intel_bufmgr.h"

#define BATCH_RESERVED 16

#define INTEL_DEFAULT_RELOCS 100
#define INTEL_MAX_RELOCS 400

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

struct intel_be_context;
struct intel_be_device;
struct intel_be_fence;

struct intel_be_batchbuffer
{
	struct i915_batchbuffer base;

	struct intel_be_context *intel;
	struct intel_be_device *device;

	drm_intel_bo *bo;
};

struct intel_be_batchbuffer *
intel_be_batchbuffer_alloc(struct intel_be_context *intel);

void
intel_be_batchbuffer_free(struct intel_be_batchbuffer *batch);

void
intel_be_batchbuffer_finish(struct intel_be_batchbuffer *batch);

void
intel_be_batchbuffer_flush(struct intel_be_batchbuffer *batch,
			   struct intel_be_fence **fence);

void
intel_be_batchbuffer_reset(struct intel_be_batchbuffer *batch);

int
intel_be_offset_relocation(struct intel_be_batchbuffer *batch,
			   unsigned pre_add,
			   drm_intel_bo *bo,
			   uint32_t read_domains,
			   uint32_t write_doman);

#endif
