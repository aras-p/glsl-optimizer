#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_state.h"

boolean
nv50_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	NOUVEAU_ERR("unimplemented\n");
	return TRUE;
}

boolean
nv50_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	NOUVEAU_ERR("unimplemented\n");
	return TRUE;
}

