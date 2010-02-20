#include "nv30_context.h"

static boolean
nv30_state_zsa_validate(struct nvfx_context *nvfx)
{
	so_ref(nvfx->zsa->so,
	       &nvfx->state.hw[NVFX_STATE_ZSA]);
	return TRUE;
}

struct nvfx_state_entry nv30_state_zsa = {
	.validate = nv30_state_zsa_validate,
	.dirty = {
		.pipe = NVFX_NEW_ZSA,
		.hw = NVFX_STATE_ZSA
	}
};

static boolean
nv30_state_sr_validate(struct nvfx_context *nvfx)
{
	struct nouveau_stateobj *so = so_new(2, 2, 0);
	struct pipe_stencil_ref *sr = &nvfx->stencil_ref;

	so_method(so, nvfx->screen->eng3d, NV34TCL_STENCIL_FRONT_FUNC_REF, 1);
	so_data  (so, sr->ref_value[0]);
	so_method(so, nvfx->screen->eng3d, NV34TCL_STENCIL_BACK_FUNC_REF, 1);
	so_data  (so, sr->ref_value[1]);

	so_ref(so, &nvfx->state.hw[NVFX_STATE_SR]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nvfx_state_entry nv30_state_sr = {
	.validate = nv30_state_sr_validate,
	.dirty = {
		.pipe = NVFX_NEW_SR,
		.hw = NVFX_STATE_SR
	}
};
