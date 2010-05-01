#include "nvfx_context.h"

void
nvfx_state_blend_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	sb_emit(chan, nvfx->blend->sb, nvfx->blend->sb_len);
}

void
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
}
