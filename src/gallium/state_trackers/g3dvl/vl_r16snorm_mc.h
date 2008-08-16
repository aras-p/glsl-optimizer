#ifndef vl_mc_h
#define vl_mc_h

#include "vl_types.h"

struct pipe_context;
struct vlRender;

int vlCreateR16SNormMC
(
	struct pipe_context *pipe,
	unsigned int video_width,
	unsigned int video_height,
	enum vlFormat video_format,
	struct vlRender **render
);

#endif
