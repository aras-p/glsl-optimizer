#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_clear.h"

#include "nvfx_context.h"

void
nvfx_clear(struct pipe_context *pipe, unsigned buffers,
           const union pipe_color_union *color, double depth, unsigned stencil)
{
	util_clear(pipe, &nvfx_context(pipe)->framebuffer, buffers, color, depth,
		   stencil);
}
