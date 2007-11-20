#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "nv40_context.h"
#include "nv40_dma.h"


void
nv40_clear(struct pipe_context *pipe, struct pipe_surface *ps,
	   unsigned clearValue)
{
	pipe->region_fill(pipe, ps->region, 0, 0, 0, ps->width, ps->height,
			  clearValue);
}
