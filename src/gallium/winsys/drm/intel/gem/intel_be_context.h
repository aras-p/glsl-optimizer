
#ifndef INTEL_BE_CONTEXT_H
#define INTEL_BE_CONTEXT_H

#include "i915simple/i915_winsys.h"

struct intel_be_context
{
	/** Interface to i915simple driver */
	struct i915_winsys base;

	struct intel_be_device *device;
	struct intel_be_batchbuffer *batch;
};

static INLINE struct intel_be_context *
intel_be_context(struct i915_winsys *sws)
{
	return (struct intel_be_context *)sws;
}

/**
 * Intialize a allocated intel_be_context struct.
 *
 * Remember to set the hardware_* functions.
 */
boolean
intel_be_init_context(struct intel_be_context *intel,
		      struct intel_be_device *device);

#endif
