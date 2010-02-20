#include "nv40_context.h"

static boolean
nv40_state_rasterizer_validate(struct nvfx_context *nvfx)
{
	so_ref(nvfx->rasterizer->so,
	       &nvfx->state.hw[NVFX_STATE_RAST]);
	return TRUE;
}

struct nvfx_state_entry nv40_state_rasterizer = {
	.validate = nv40_state_rasterizer_validate,
	.dirty = {
		.pipe = NVFX_NEW_RAST,
		.hw = NVFX_STATE_RAST
	}
};
