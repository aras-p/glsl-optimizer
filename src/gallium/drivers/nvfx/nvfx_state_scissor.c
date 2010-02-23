#include "nvfx_context.h"

void
nvfx_state_scissor_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct pipe_rasterizer_state *rast = &nvfx->rasterizer->pipe;
	struct pipe_scissor_state *s = &nvfx->scissor;

	if ((rast->scissor == 0 && nvfx->state.scissor_enabled == 0))
		return;
	nvfx->state.scissor_enabled = rast->scissor;

	WAIT_RING(chan, 3);
	OUT_RING(chan, RING_3D(NV34TCL_SCISSOR_HORIZ, 2));
	if (nvfx->state.scissor_enabled) {
		OUT_RING(chan, ((s->maxx - s->minx) << 16) | s->minx);
		OUT_RING(chan, ((s->maxy - s->miny) << 16) | s->miny);
	} else {
		OUT_RING(chan, 4096 << 16);
		OUT_RING(chan, 4096 << 16);
	}
}
