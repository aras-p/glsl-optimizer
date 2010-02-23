#include "nvfx_context.h"

static boolean
nvfx_state_rasterizer_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	sb_emit(chan, nvfx->rasterizer->sb, nvfx->rasterizer->sb_len);
	return TRUE;
}

struct nvfx_state_entry nvfx_state_rasterizer = {
	.validate = nvfx_state_rasterizer_validate,
	.dirty = {
		.pipe = NVFX_NEW_RAST,
	}
};
