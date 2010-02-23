#include "nvfx_context.h"

static boolean
nvfx_state_zsa_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	sb_emit(chan, nvfx->zsa->sb, nvfx->zsa->sb_len);
	return TRUE;
}

struct nvfx_state_entry nvfx_state_zsa = {
	.validate = nvfx_state_zsa_validate,
	.dirty = {
		.pipe = NVFX_NEW_ZSA,
	}
};

static boolean
nvfx_state_sr_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct pipe_stencil_ref *sr = &nvfx->stencil_ref;

	WAIT_RING(chan, 4);
	OUT_RING(chan, RING_3D(NV34TCL_STENCIL_FRONT_FUNC_REF, 1));
	OUT_RING(chan, sr->ref_value[0]);
	OUT_RING(chan, RING_3D(NV34TCL_STENCIL_BACK_FUNC_REF, 1));
	OUT_RING(chan, sr->ref_value[1]);
	return TRUE;
}

struct nvfx_state_entry nvfx_state_sr = {
	.validate = nvfx_state_sr_validate,
	.dirty = {
		.pipe = NVFX_NEW_SR,
	}
};
