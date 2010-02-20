#include "nv40_context.h"

static boolean
nv40_state_blend_validate(struct nvfx_context *nvfx)
{
	so_ref(nvfx->blend->so, &nvfx->state.hw[NVFX_STATE_BLEND]);
	return TRUE;
}

struct nvfx_state_entry nv40_state_blend = {
	.validate = nv40_state_blend_validate,
	.dirty = {
		.pipe = NVFX_NEW_BLEND,
		.hw = NVFX_STATE_BLEND
	}
};

static boolean
nv40_state_blend_colour_validate(struct nvfx_context *nvfx)
{
	struct nouveau_stateobj *so = so_new(1, 1, 0);
	struct pipe_blend_color *bcol = &nvfx->blend_colour;

	so_method(so, nvfx->screen->eng3d, NV34TCL_BLEND_COLOR, 1);
	so_data  (so, ((float_to_ubyte(bcol->color[3]) << 24) |
		       (float_to_ubyte(bcol->color[0]) << 16) |
		       (float_to_ubyte(bcol->color[1]) <<  8) |
		       (float_to_ubyte(bcol->color[2]) <<  0)));

	so_ref(so, &nvfx->state.hw[NVFX_STATE_BCOL]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nvfx_state_entry nv40_state_blend_colour = {
	.validate = nv40_state_blend_colour_validate,
	.dirty = {
		.pipe = NVFX_NEW_BCOL,
		.hw = NVFX_STATE_BCOL
	}
};
