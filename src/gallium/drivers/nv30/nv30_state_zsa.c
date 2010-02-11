#include "nv30_context.h"

static boolean
nv30_state_zsa_validate(struct nv30_context *nv30)
{
	so_ref(nv30->zsa->so,
	       &nv30->state.hw[NV30_STATE_ZSA]);
	return TRUE;
}

struct nv30_state_entry nv30_state_zsa = {
	.validate = nv30_state_zsa_validate,
	.dirty = {
		.pipe = NV30_NEW_ZSA,
		.hw = NV30_STATE_ZSA
	}
};

static boolean
nv30_state_sr_validate(struct nv30_context *nv30)
{
	struct nouveau_stateobj *so = so_new(2, 2, 0);
	struct pipe_stencil_ref *sr = &nv30->stencil_ref;

	so_method(so, nv30->screen->rankine, NV34TCL_STENCIL_FRONT_FUNC_REF, 1);
	so_data  (so, sr->ref_value[0]);
	so_method(so, nv30->screen->rankine, NV34TCL_STENCIL_BACK_FUNC_REF, 1);
	so_data  (so, sr->ref_value[1]);

	so_ref(so, &nv30->state.hw[NV30_STATE_SR]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv30_state_entry nv30_state_sr = {
	.validate = nv30_state_sr_validate,
	.dirty = {
		.pipe = NV30_NEW_SR,
		.hw = NV30_STATE_SR
	}
};
