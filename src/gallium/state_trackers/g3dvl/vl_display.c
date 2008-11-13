#define VL_INTERNAL
#include "vl_display.h"
#include <assert.h>
#include <stdlib.h>

int vlCreateDisplay
(
	vlNativeDisplay native_display,
	struct vlDisplay **display
)
{
	struct vlDisplay *dpy;

	assert(native_display);
	assert(display);

	dpy = calloc(1, sizeof(struct vlDisplay));

	if (!dpy)
		return 1;

	dpy->native = native_display;
	*display = dpy;

	return 0;
}

int vlDestroyDisplay
(
	struct vlDisplay *display
)
{
	assert(display);

	free(display);

	return 0;
}

vlNativeDisplay vlGetNativeDisplay
(
	struct vlDisplay *display
)
{
	assert(display);

	return display->native;
}
