#include "nvfx_context.h"

static boolean
nvfx_state_rasterizer_validate(struct nvfx_context *nvfx)
{
	so_ref(nvfx->rasterizer->so,
	       &nvfx->state.hw[NVFX_STATE_RAST]);
	return TRUE;
}

struct nvfx_state_entry nvfx_state_rasterizer = {
	.validate = nvfx_state_rasterizer_validate,
	.dirty = {
		.pipe = NVFX_NEW_RAST,
		.hw = NVFX_STATE_RAST
	}
};
