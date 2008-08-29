#ifndef vl_r16snorm_mc_buf_h
#define vl_r16snorm_mc_buf_h

#include "vl_types.h"

struct pipe_context;
struct vlRender;

int vlCreateR16SNormBufferedMC
(
	struct pipe_context *pipe,
	unsigned int video_width,
	unsigned int video_height,
	enum vlFormat video_format,
	struct vlRender **render
);

#endif
