#define VL_INTERNAL
#include "vl_display.h"
#include <assert.h>
#include <util/u_memory.h>

int vlCreateDisplay
(
	vlNativeDisplay native_display,
	struct vlDisplay **display
)
{
	struct vlDisplay *dpy;

	assert(native_display);
	assert(display);

	dpy = CALLOC_STRUCT(vlDisplay);

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

	FREE(display);

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
