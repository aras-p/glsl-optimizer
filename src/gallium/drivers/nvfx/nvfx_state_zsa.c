#include "nvfx_context.h"

void
nvfx_state_zsa_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	sb_emit(chan, nvfx->zsa->sb, nvfx->zsa->sb_len);
}

void
nvfx_state_sr_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct pipe_stencil_ref *sr = &nvfx->stencil_ref;

	WAIT_RING(chan, 4);
	OUT_RING(chan, RING_3D(NV34TCL_STENCIL_FRONT_FUNC_REF, 1));
	OUT_RING(chan, sr->ref_value[0]);
	OUT_RING(chan, RING_3D(NV34TCL_STENCIL_BACK_FUNC_REF, 1));
	OUT_RING(chan, sr->ref_value[1]);
}
