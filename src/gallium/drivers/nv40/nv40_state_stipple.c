#include "nv40_context.h"

static boolean
nv40_state_stipple_validate(struct nvfx_context *nvfx)
{
	struct pipe_rasterizer_state *rast = &nvfx->rasterizer->pipe;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct nouveau_stateobj *so;

	if (nvfx->state.hw[NVFX_STATE_STIPPLE] &&
	   (rast->poly_stipple_enable == 0 && nvfx->state.stipple_enabled == 0))
		return FALSE;

	if (rast->poly_stipple_enable) {
		unsigned i;

		so = so_new(2, 33, 0);
		so_method(so, eng3d, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, eng3d, NV34TCL_POLYGON_STIPPLE_PATTERN(0), 32);
		for (i = 0; i < 32; i++)
			so_data(so, nvfx->stipple[i]);
	} else {
		so = so_new(1, 1, 0);
		so_method(so, eng3d, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
		so_data  (so, 0);
	}

	so_ref(so, &nvfx->state.hw[NVFX_STATE_STIPPLE]);
	return TRUE;
}

struct nvfx_state_entry nv40_state_stipple = {
	.validate = nv40_state_stipple_validate,
	.dirty = {
		.pipe = NVFX_NEW_STIPPLE | NVFX_NEW_RAST,
		.hw = NVFX_STATE_STIPPLE,
	}
};
