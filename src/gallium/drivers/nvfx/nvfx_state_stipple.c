#include "nvfx_context.h"

void
nvfx_state_stipple_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct pipe_rasterizer_state *rast = &nvfx->rasterizer->pipe;

	if ((rast->poly_stipple_enable == 0 && nvfx->state.stipple_enabled == 0))
		return;

	if (rast->poly_stipple_enable) {
		unsigned i;

		WAIT_RING(chan, 35);
		OUT_RING(chan, RING_3D(NV34TCL_POLYGON_STIPPLE_ENABLE, 1));
		OUT_RING(chan, 1);
		OUT_RING(chan, RING_3D(NV34TCL_POLYGON_STIPPLE_PATTERN(0), 32));
		for (i = 0; i < 32; i++)
			OUT_RING(chan, nvfx->stipple[i]);
	} else {
		WAIT_RING(chan, 2);
		OUT_RING(chan, RING_3D(NV34TCL_POLYGON_STIPPLE_ENABLE, 1));
		OUT_RING(chan, 0);
	}
}
