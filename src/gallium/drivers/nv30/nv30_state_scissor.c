#include "nv30_context.h"

static boolean
nv30_state_scissor_validate(struct nvfx_context *nvfx)
{
	struct pipe_rasterizer_state *rast = &nvfx->rasterizer->pipe;
	struct pipe_scissor_state *s = &nvfx->scissor;
	struct nouveau_stateobj *so;

	if (nvfx->state.hw[NVFX_STATE_SCISSOR] &&
	    (rast->scissor == 0 && nvfx->state.scissor_enabled == 0))
		return FALSE;
	nvfx->state.scissor_enabled = rast->scissor;

	so = so_new(1, 2, 0);
	so_method(so, nvfx->screen->eng3d, NV34TCL_SCISSOR_HORIZ, 2);
	if (nvfx->state.scissor_enabled) {
		so_data  (so, ((s->maxx - s->minx) << 16) | s->minx);
		so_data  (so, ((s->maxy - s->miny) << 16) | s->miny);
	} else {
		so_data  (so, 4096 << 16);
		so_data  (so, 4096 << 16);
	}

	so_ref(so, &nvfx->state.hw[NVFX_STATE_SCISSOR]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nvfx_state_entry nv30_state_scissor = {
	.validate = nv30_state_scissor_validate,
	.dirty = {
		.pipe = NVFX_NEW_SCISSOR | NVFX_NEW_RAST,
		.hw = NVFX_STATE_SCISSOR
	}
};
