#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "nv40_context.h"
#include "nv40_dma.h"


void
nv40_clear(struct pipe_context *pipe, struct pipe_surface *ps,
	   unsigned clearValue)
{
	/*XXX: We're actually Z24_S8... */
	if (ps->format == PIPE_FORMAT_S8_Z24) {
		clearValue = (((clearValue & 0xff000000) >> 24) |
			      ((clearValue & 0x00ffffff) <<  8));
	}

	pipe->region_fill(pipe, ps->region, 0, 0, 0, ps->width, ps->height,
			  clearValue);
}
