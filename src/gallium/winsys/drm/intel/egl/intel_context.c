
#include "i915simple/i915_screen.h"

#include "intel_be_device.h"
#include "intel_be_context.h"

#include "pipe/p_defines.h"
#include "pipe/p_context.h"

#include "intel_api.h"

struct intel_context
{
	struct intel_be_context base;

	/* stuff */
};

/*
 * Hardware lock functions.
 * Doesn't do anything in EGL
 */

static void
intel_lock_hardware(struct intel_be_context *context)
{
	(void)context;
}

static void
intel_unlock_hardware(struct intel_be_context *context)
{
	(void)context;
}

static boolean
intel_locked_hardware(struct intel_be_context *context)
{
	(void)context;
	return FALSE;
}


/*
 * Misc functions.
 */
static void
intel_destroy_be_context(struct i915_winsys *winsys)
{
	struct intel_context *intel = (struct intel_context *)winsys;

	intel_be_destroy_context(&intel->base);
	free(intel);
}

struct pipe_context *
intel_create_context(struct pipe_screen *screen)
{
	struct intel_context *intel;
	struct pipe_context *pipe;
	struct intel_be_device *device = (struct intel_be_device *)screen->winsys;

	intel = (struct intel_context *)malloc(sizeof(*intel));
	memset(intel, 0, sizeof(*intel));

	intel->base.hardware_lock = intel_lock_hardware;
	intel->base.hardware_unlock = intel_unlock_hardware;
	intel->base.hardware_locked = intel_locked_hardware;

	intel_be_init_context(&intel->base, device);

	intel->base.base.destroy = intel_destroy_be_context;

#if 0
	pipe = intel_create_softpipe(intel, screen->winsys);
#else
	pipe = i915_create_context(screen, &device->base, &intel->base.base);
#endif

	pipe->priv = intel;

	return pipe;
}
