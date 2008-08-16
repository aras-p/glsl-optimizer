#ifndef vl_context_h
#define vl_context_h

#include "vl_types.h"

struct pipe_context;

#ifdef VL_INTERNAL
struct vlRender;
struct vlCSC;

struct vlContext
{
	struct vlScreen		*screen;
	struct pipe_context	*pipe;
	unsigned int		picture_width;
	unsigned int		picture_height;
	enum vlFormat		picture_format;
	enum vlProfile		profile;
	enum vlEntryPoint	entry_point;

	void			*raster;
	void			*dsa;
	void			*blend;

	struct vlRender		*render;
	struct vlCSC		*csc;
};
#endif

int vlCreateContext
(
	struct vlScreen *screen,
	struct pipe_context *pipe,
	unsigned int picture_width,
	unsigned int picture_height,
	enum vlFormat picture_format,
	enum vlProfile profile,
	enum vlEntryPoint entry_point,
	struct vlContext **context
);

int vlDestroyContext
(
	struct vlContext *context
);

struct vlScreen* vlContextGetScreen
(
	struct vlContext *context
);

struct pipe_context* vlGetPipeContext
(
	struct vlContext *context
);

unsigned int vlGetPictureWidth
(
	struct vlContext *context
);

unsigned int vlGetPictureHeight
(
	struct vlContext *context
);

enum vlFormat vlGetPictureFormat
(
	struct vlContext *context
);

#endif
