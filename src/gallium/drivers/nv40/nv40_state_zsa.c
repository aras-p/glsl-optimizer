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
