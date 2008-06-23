#include "nv30_context.h"

static boolean
nv30_state_rasterizer_validate(struct nv30_context *nv30)
{
	so_ref(nv30->rasterizer->so,
	       &nv30->state.hw[NV30_STATE_RAST]);
	return TRUE;
}

struct nv30_state_entry nv30_state_rasterizer = {
	.validate = nv30_state_rasterizer_validate,
	.dirty = {
		.pipe = NV30_NEW_RAST,
		.hw = NV30_STATE_RAST
	}
};
