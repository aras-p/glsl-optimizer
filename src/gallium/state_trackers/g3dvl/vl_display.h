#ifndef vl_display_h
#define vl_display_h

#include "vl_types.h"

#ifdef VL_INTERNAL
struct vlDisplay
{
	vlNativeDisplay native;
};
#endif

int vlCreateDisplay
(
	vlNativeDisplay native_display,
	struct vlDisplay **display
);

int vlDestroyDisplay
(
	struct vlDisplay *display
);

vlNativeDisplay vlGetNativeDisplay
(
	struct vlDisplay *display
);

#endif
