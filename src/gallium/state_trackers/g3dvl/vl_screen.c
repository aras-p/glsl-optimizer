#define VL_INTERNAL
#include "vl_screen.h"
#include <assert.h>
#include <util/u_memory.h>

int vlCreateScreen
(
	struct vlDisplay *display,
	int screen,
	struct pipe_screen *pscreen,
	struct vlScreen **vl_screen
)
{
	struct vlScreen *scrn;

	assert(display);
	assert(pscreen);
	assert(vl_screen);

	scrn = CALLOC_STRUCT(vlScreen);

	if (!scrn)
		return 1;

	scrn->display = display;
	scrn->ordinal = screen;
	scrn->pscreen = pscreen;
	*vl_screen = scrn;

	return 0;
}

int vlDestroyScreen
(
	struct vlScreen *screen
)
{
	assert(screen);

	FREE(screen);

	return 0;
}

struct vlDisplay* vlGetDisplay
(
	struct vlScreen *screen
)
{
	assert(screen);

	return screen->display;
}

struct pipe_screen* vlGetPipeScreen
(
	struct vlScreen *screen
)
{
	assert(screen);

	return screen->pscreen;
}

unsigned int vlGetMaxProfiles
(
	struct vlScreen *screen
)
{
	assert(screen);

	return vlProfileCount;
}

int vlQueryProfiles
(
	struct vlScreen *screen,
	enum vlProfile *profiles
)
{
	assert(screen);
	assert(profiles);

	profiles[0] = vlProfileMpeg2Simple;
	profiles[1] = vlProfileMpeg2Main;

	return 0;
}

unsigned int vlGetMaxEntryPoints
(
	struct vlScreen *screen
)
{
	assert(screen);

	return vlEntryPointCount;
}

int vlQueryEntryPoints
(
	struct vlScreen *screen,
	enum vlProfile profile,
	enum vlEntryPoint *entry_points
)
{
	assert(screen);
	assert(entry_points);

	entry_points[0] = vlEntryPointIDCT;
	entry_points[1] = vlEntryPointMC;
	entry_points[2] = vlEntryPointCSC;

	return 0;
}
