#ifndef vl_screen_h
#define vl_screen_h

#include "vl_types.h"

struct pipe_screen;

#ifdef VL_INTERNAL
struct vlScreen
{
	struct vlDisplay	*display;
	unsigned int		ordinal;
	struct pipe_screen	*pscreen;
};
#endif

int vlCreateScreen
(
	struct vlDisplay *display,
	int screen,
	struct pipe_screen *pscreen,
	struct vlScreen **vl_screen
);

int vlDestroyScreen
(
	struct vlScreen *screen
);

struct vlDisplay* vlGetDisplay
(
	struct vlScreen *screen
);

struct pipe_screen* vlGetPipeScreen
(
	struct vlScreen *screen
);

unsigned int vlGetMaxProfiles
(
	struct vlScreen *screen
);

int vlQueryProfiles
(
	struct vlScreen *screen,
	enum vlProfile *profiles
);

unsigned int vlGetMaxEntryPoints
(
	struct vlScreen *screen
);

int vlQueryEntryPoints
(
	struct vlScreen *screen,
	enum vlProfile profile,
	enum vlEntryPoint *entry_points
);

#endif
