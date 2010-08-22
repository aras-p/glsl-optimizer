#include "nvfx_context.h"

void
nvfx_state_stipple_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;

	WAIT_RING(chan, 33);
	OUT_RING(chan, RING_3D(NV34TCL_POLYGON_STIPPLE_PATTERN(0), 32));
	OUT_RINGp(chan, nvfx->stipple, 32);
}
