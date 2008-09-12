#ifndef vl_surface_h
#define vl_surface_h

#include "vl_types.h"

#ifdef VL_INTERNAL
struct pipe_texture;

struct vlSurface
{
	struct vlScreen		*screen;
	struct vlContext	*context;
	unsigned int		width;
	unsigned int		height;
	enum vlFormat		format;
	struct pipe_texture	*texture;
};
#endif

int vlCreateSurface
(
	struct vlScreen *screen,
	unsigned int width,
	unsigned int height,
	enum vlFormat format,
	struct vlSurface **surface
);

int vlDestroySurface
(
	struct vlSurface *surface
);

int vlRenderMacroBlocksMpeg2
(
	struct vlMpeg2MacroBlockBatch *batch,
	struct vlSurface *surface
);

int vlPutPicture
(
	struct vlSurface *surface,
	vlNativeDrawable drawable,
	int srcx,
	int srcy,
	int srcw,
	int srch,
	int destx,
	int desty,
	int destw,
	int desth,
	enum vlPictureType picture_type
);

struct vlScreen* vlSurfaceGetScreen
(
	struct vlSurface *surface
);

struct vlContext* vlBindToContext
(
	struct vlSurface *surface,
	struct vlContext *context
);

#endif
