#include "nvfx_context.h"

static boolean
nvfx_state_blend_validate(struct nvfx_context *nvfx)
{
	so_ref(nvfx->blend->so, &nvfx->state.hw[NVFX_STATE_BLEND]);
	return TRUE;
}

struct nvfx_state_entry nvfx_state_blend = {
	.validate = nvfx_state_blend_validate,
	.dirty = {
		.pipe = NVFX_NEW_BLEND,
		.hw = NVFX_STATE_BLEND
	}
};

static boolean
nvfx_state_blend_colour_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct pipe_blend_color *bcol = &nvfx->blend_colour;

	WAIT_RING(chan, 2);
	OUT_RING(chan, RING_3D(NV34TCL_BLEND_COLOR, 1));
	OUT_RING(chan, ((float_to_ubyte(bcol->color[3]) << 24) |
		       (float_to_ubyte(bcol->color[0]) << 16) |
		       (float_to_ubyte(bcol->color[1]) <<  8) |
		       (float_to_ubyte(bcol->color[2]) <<  0)));
	return TRUE;
}

struct nvfx_state_entry nvfx_state_blend_colour = {
	.validate = nvfx_state_blend_colour_validate,
	.dirty = {
		.pipe = NVFX_NEW_BCOL,
	}
};
