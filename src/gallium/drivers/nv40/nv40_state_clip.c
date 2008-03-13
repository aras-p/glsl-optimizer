#include "nv40_context.h"

static boolean
nv40_state_clip_validate(struct nv40_context *nv40)
{

	if (nv40->render_mode == HW) {
		nv40->fallback_swtnl &= ~NV40_NEW_UCP;
		if (nv40->clip.nr)
			nv40->fallback_swtnl |= NV40_NEW_UCP;
	}

	return FALSE;
}

struct nv40_state_entry nv40_state_clip = {
	.validate = nv40_state_clip_validate,
	.dirty = {
		.pipe = NV40_NEW_UCP,
		.hw = 0
	}
};
