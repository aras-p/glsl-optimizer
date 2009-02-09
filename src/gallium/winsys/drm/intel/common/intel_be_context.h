/* These need to be diffrent from the intel winsys */
#ifndef INTEL_BE_CONTEXT_H
#define INTEL_BE_CONTEXT_H

#include "i915simple/i915_winsys.h"

struct intel_be_context
{
	/** Interface to i915simple driver */
	struct i915_winsys base;

	struct intel_be_device *device;
	struct intel_be_batchbuffer *batch;

	/*
     * Hardware lock functions.
     *
     * Needs to be filled in by the winsys.
     */
	void (*hardware_lock)(struct intel_be_context *context);
	void (*hardware_unlock)(struct intel_be_context *context);
	boolean (*hardware_locked)(struct intel_be_context *context);
};

/**
 * Intialize a allocated intel_be_context struct.
 *
 * Remember to set the hardware_* functions.
 */
boolean
intel_be_init_context(struct intel_be_context *intel,
		      struct intel_be_device *device);

/**
 * Destroy a intel_be_context.
 * Does not free the struct that is up to the winsys.
 */
void
intel_be_destroy_context(struct intel_be_context *intel);
#endif
