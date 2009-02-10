
#include <stdio.h>
#include "pipe/p_defines.h"
#include "intel_be_device.h"
#include "i915simple/i915_screen.h"

#include "intel_api.h"

struct intel_device
{
	struct intel_be_device base;

	int deviceID;
};

static void
intel_destroy_winsys(struct pipe_winsys *winsys)
{
	struct intel_device *dev = (struct intel_device *)winsys;

	intel_be_destroy_device(&dev->base);

	free(dev);
}

struct pipe_screen *
intel_create_screen(int drmFD, int deviceID)
{
	struct intel_device *dev;
	struct pipe_screen *screen;

	/* Allocate the private area */
	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;
	memset(dev, 0, sizeof(*dev));

	dev->deviceID = deviceID;

	intel_be_init_device(&dev->base, drmFD, deviceID);

	/* we need to hock our own destroy function in here */
	dev->base.base.destroy = intel_destroy_winsys;

	screen = i915_create_screen(&dev->base.base, deviceID);

	return screen;
}
