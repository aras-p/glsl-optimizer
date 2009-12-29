#include "nv40_context.h"

static boolean
nv40_state_stipple_validate(struct nv40_context *nv40)
{
	struct pipe_rasterizer_state *rast = &nv40->rasterizer->pipe;
	struct nouveau_grobj *curie = nv40->screen->curie;
	struct nouveau_stateobj *so;

	if (nv40->state.hw[NV40_STATE_STIPPLE] &&
	   (rast->poly_stipple_enable == 0 && nv40->state.stipple_enabled == 0))
		return FALSE;

	if (rast->poly_stipple_enable) {
		unsigned i;

		so = so_new(2, 33, 0);
		so_method(so, curie, NV40TCL_POLYGON_STIPPLE_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, curie, NV40TCL_POLYGON_STIPPLE_PATTERN(0), 32);
		for (i = 0; i < 32; i++)
			so_data(so, nv40->stipple[i]);
	} else {
		so = so_new(1, 1, 0);
		so_method(so, curie, NV40TCL_POLYGON_STIPPLE_ENABLE, 1);
		so_data  (so, 0);
	}

	so_ref(so, &nv40->state.hw[NV40_STATE_STIPPLE]);
	return TRUE;
}

struct nv40_state_entry nv40_state_stipple = {
	.validate = nv40_state_stipple_validate,
	.dirty = {
		.pipe = NV40_NEW_STIPPLE | NV40_NEW_RAST,
		.hw = NV40_STATE_STIPPLE,
	}
};
