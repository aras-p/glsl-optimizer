#include "nv40_context.h"

static boolean
nv40_state_viewport_validate(struct nv40_context *nv40)
{
	struct nouveau_stateobj *so = so_new(9, 0);
	struct pipe_viewport_state *vpt = &nv40->viewport;

	so_method(so, nv40->hw->curie, NV40TCL_VIEWPORT_TRANSLATE_X, 8);
	so_data  (so, fui(vpt->translate[0]));
	so_data  (so, fui(vpt->translate[1]));
	so_data  (so, fui(vpt->translate[2]));
	so_data  (so, fui(vpt->translate[3]));
	so_data  (so, fui(vpt->scale[0]));
	so_data  (so, fui(vpt->scale[1]));
	so_data  (so, fui(vpt->scale[2]));
	so_data  (so, fui(vpt->scale[3]));

	so_ref(so, &nv40->state.hw[NV40_STATE_VIEWPORT]);
	return TRUE;
}

struct nv40_state_entry nv40_state_viewport = {
	.validate = nv40_state_viewport_validate,
	.dirty = {
		.pipe = NV40_NEW_VIEWPORT,
		.hw = NV40_STATE_VIEWPORT
	}
};
