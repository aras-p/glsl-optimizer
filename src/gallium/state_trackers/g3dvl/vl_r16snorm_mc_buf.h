#ifndef vl_r16snorm_mc_buf_h
#define vl_r16snorm_mc_buf_h

#include "vl_types.h"

struct pipe_context;
struct vlRender;

int vlCreateR16SNormBufferedMC
(
	struct pipe_context *pipe,
	unsigned int picture_width,
	unsigned int picture_height,
	enum vlFormat picture_format,
	struct vlRender **render
);

#endif
