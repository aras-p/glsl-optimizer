#include "nv30_context.h"

static boolean
nv30_state_scissor_validate(struct nv30_context *nv30)
{
	struct pipe_rasterizer_state *rast = &nv30->rasterizer->pipe;
	struct pipe_scissor_state *s = &nv30->scissor;
	struct nouveau_stateobj *so;

	if (nv30->state.hw[NV30_STATE_SCISSOR] &&
	    (rast->scissor == 0 && nv30->state.scissor_enabled == 0))
		return FALSE;
	nv30->state.scissor_enabled = rast->scissor;

	so = so_new(1, 2, 0);
	so_method(so, nv30->screen->rankine, NV34TCL_SCISSOR_HORIZ, 2);
	if (nv30->state.scissor_enabled) {
		so_data  (so, ((s->maxx - s->minx) << 16) | s->minx);
		so_data  (so, ((s->maxy - s->miny) << 16) | s->miny);
	} else {
		so_data  (so, 4096 << 16);
		so_data  (so, 4096 << 16);
	}

	so_ref(so, &nv30->state.hw[NV30_STATE_SCISSOR]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv30_state_entry nv30_state_scissor = {
	.validate = nv30_state_scissor_validate,
	.dirty = {
		.pipe = NV30_NEW_SCISSOR | NV30_NEW_RAST,
		.hw = NV30_STATE_SCISSOR
	}
};
