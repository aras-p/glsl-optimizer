#include "nv40_context.h"

static boolean
nv40_state_rasterizer_validate(struct nv40_context *nv40)
{
	so_ref(nv40->rasterizer->so,
	       &nv40->state.hw[NV40_STATE_RAST]);
	return TRUE;
}

struct nv40_state_entry nv40_state_rasterizer = {
	.validate = nv40_state_rasterizer_validate,
	.dirty = {
		.pipe = NV40_NEW_RAST,
		.hw = NV40_STATE_RAST
	}
};
