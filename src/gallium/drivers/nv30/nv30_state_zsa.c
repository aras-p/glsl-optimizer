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
