#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "nv50_context.h"
#include "nv50_dma.h"


void
nv50_clear(struct pipe_context *pipe, struct pipe_surface *ps,
	   unsigned clearValue)
{
	pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height, clearValue);
}
