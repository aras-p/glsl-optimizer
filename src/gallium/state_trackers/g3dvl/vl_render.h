#ifndef vl_render_h
#define vl_render_h

#include "vl_types.h"

struct pipe_surface;

struct vlRender
{
	int (*vlBegin)
	(
		struct vlRender *render
	);

	int (*vlRenderMacroBlocksMpeg2)
	(
		struct vlRender *render,
		struct vlMpeg2MacroBlockBatch *batch,
		struct vlSurface *surface
	);

	int (*vlEnd)
	(
		struct vlRender *render
	);

	int (*vlFlush)
	(
		struct vlRender *render
	);

	int (*vlDestroy)
	(
		struct vlRender *render
	);
};

#endif
