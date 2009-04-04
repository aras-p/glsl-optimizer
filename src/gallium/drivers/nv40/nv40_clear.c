#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_clear.h"

#include "nv40_context.h"

void
nv40_clear(struct pipe_context *pipe, unsigned buffers,
           const float *rgba, double depth, unsigned stencil)
{
	util_clear(pipe, &nv40_context(pipe)->framebuffer, buffers, rgba, depth,
		   stencil);
}
