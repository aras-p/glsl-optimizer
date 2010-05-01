#include "nvfx_context.h"

void
nvfx_state_rasterizer_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	sb_emit(chan, nvfx->rasterizer->sb, nvfx->rasterizer->sb_len);
}

