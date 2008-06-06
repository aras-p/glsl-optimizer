
#ifndef INTEL_BE_BATCHBUFFER_H
#define INTEL_BE_BATCHBUFFER_H

#include "i915simple/i915_batch.h"

#include "ws_dri_bufmgr.h"

#define BATCH_RESERVED 16

#define INTEL_DEFAULT_RELOCS 100
#define INTEL_MAX_RELOCS 400

#define INTEL_BATCH_NO_CLIPRECTS 0x1
#define INTEL_BATCH_CLIPRECTS    0x2

struct intel_be_context;
struct intel_be_device;

struct intel_be_batchbuffer
{
	struct i915_batchbuffer base;

	struct intel_be_context *intel;
	struct intel_be_device *device;

	struct _DriBufferObject *buffer;
	struct _DriFenceObject *last_fence;
	uint32_t flags;

	struct _DriBufferList *list;
	size_t list_count;

	uint32_t *reloc;
	size_t reloc_size;
	size_t nr_relocs;

	uint32_t dirty_state;
	uint32_t id;

	uint32_t poolOffset;
	uint8_t *drmBOVirtual;
	struct _drmBONode *node; /* Validation list node for this buffer */
	int dest_location;     /* Validation list sequence for this buffer */
};

struct intel_be_batchbuffer *
intel_be_batchbuffer_alloc(struct intel_be_context *intel);

void
intel_be_batchbuffer_free(struct intel_be_batchbuffer *batch);

void
intel_be_batchbuffer_finish(struct intel_be_batchbuffer *batch);

struct _DriFenceObject *
intel_be_batchbuffer_flush(struct intel_be_batchbuffer *batch);

void
intel_be_batchbuffer_reset(struct intel_be_batchbuffer *batch);

void
intel_be_offset_relocation(struct intel_be_batchbuffer *batch,
			unsigned pre_add,
			struct _DriBufferObject *driBO,
			uint64_t val_flags,
			uint64_t val_mask);

#endif
