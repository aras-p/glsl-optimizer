#include "nv40_context.h"

static boolean
nv40_state_scissor_validate(struct nv40_context *nv40)
{
	struct pipe_rasterizer_state *rast = &nv40->rasterizer->pipe;
	struct pipe_scissor_state *s = &nv40->scissor;
	struct nouveau_stateobj *so;

	if (nv40->state.hw[NV40_STATE_SCISSOR] &&
	    (rast->scissor == 0 && nv40->state.scissor_enabled == 0))
		return FALSE;
	nv40->state.scissor_enabled = rast->scissor;

	so = so_new(1, 2, 0);
	so_method(so, nv40->screen->curie, NV40TCL_SCISSOR_HORIZ, 2);
	if (nv40->state.scissor_enabled) {
		so_data  (so, ((s->maxx - s->minx) << 16) | s->minx);
		so_data  (so, ((s->maxy - s->miny) << 16) | s->miny);
	} else {
		so_data  (so, 4096 << 16);
		so_data  (so, 4096 << 16);
	}

	so_ref(so, &nv40->state.hw[NV40_STATE_SCISSOR]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv40_state_entry nv40_state_scissor = {
	.validate = nv40_state_scissor_validate,
	.dirty = {
		.pipe = NV40_NEW_SCISSOR | NV40_NEW_RAST,
		.hw = NV40_STATE_SCISSOR
	}
};
