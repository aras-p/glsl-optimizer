#include "nv40_context.h"

static boolean
nv40_state_zsa_validate(struct nv40_context *nv40)
{
	so_ref(nv40->zsa->so,
	       &nv40->state.hw[NV40_STATE_ZSA]);
	return TRUE;
}

struct nv40_state_entry nv40_state_zsa = {
	.validate = nv40_state_zsa_validate,
	.dirty = {
		.pipe = NV40_NEW_ZSA,
		.hw = NV40_STATE_ZSA
	}
};

static boolean
nv40_state_sr_validate(struct nv40_context *nv40)
{
	struct nouveau_stateobj *so = so_new(2, 2, 0);
	struct pipe_stencil_ref *sr = &nv40->stencil_ref;

	so_method(so, nv40->screen->curie, NV40TCL_STENCIL_FRONT_FUNC_REF, 1);
	so_data  (so, sr->ref_value[0]);
	so_method(so, nv40->screen->curie, NV40TCL_STENCIL_BACK_FUNC_REF, 1);
	so_data  (so, sr->ref_value[1]);

	so_ref(so, &nv40->state.hw[NV40_STATE_SR]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv40_state_entry nv40_state_sr = {
	.validate = nv40_state_sr_validate,
	.dirty = {
		.pipe = NV40_NEW_SR,
		.hw = NV40_STATE_SR
	}
};
