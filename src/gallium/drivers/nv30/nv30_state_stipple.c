#include "nv30_context.h"

static boolean
nv30_state_stipple_validate(struct nv30_context *nv30)
{
	struct pipe_rasterizer_state *rast = &nv30->rasterizer->pipe;
	struct nouveau_grobj *rankine = nv30->screen->rankine;
	struct nouveau_stateobj *so;

	if (nv30->state.hw[NV30_STATE_STIPPLE] &&
	   (rast->poly_stipple_enable == 0 && nv30->state.stipple_enabled == 0))
		return FALSE;

	if (rast->poly_stipple_enable) {
		unsigned i;

		so = so_new(2, 33, 0);
		so_method(so, rankine, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
		so_data  (so, 1);
		so_method(so, rankine, NV34TCL_POLYGON_STIPPLE_PATTERN(0), 32);
		for (i = 0; i < 32; i++)
			so_data(so, nv30->stipple[i]);
	} else {
		so = so_new(1, 1, 0);
		so_method(so, rankine, NV34TCL_POLYGON_STIPPLE_ENABLE, 1);
		so_data  (so, 0);
	}

	so_ref(so, &nv30->state.hw[NV30_STATE_STIPPLE]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv30_state_entry nv30_state_stipple = {
	.validate = nv30_state_stipple_validate,
	.dirty = {
		.pipe = NV30_NEW_STIPPLE | NV30_NEW_RAST,
		.hw = NV30_STATE_STIPPLE,
	}
};
