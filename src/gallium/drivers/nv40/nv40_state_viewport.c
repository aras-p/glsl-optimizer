#include "nv40_context.h"

static boolean
nv40_state_viewport_validate(struct nv40_context *nv40)
{
	struct pipe_viewport_state *vpt = &nv40->viewport;
	struct nouveau_stateobj *so;

	if (nv40->state.hw[NV40_STATE_VIEWPORT] &&
	    !(nv40->dirty & NV40_NEW_VIEWPORT))
		return FALSE;

	so = so_new(2, 9, 0);
	so_method(so, nv40->screen->curie,
		  NV40TCL_VIEWPORT_TRANSLATE_X, 8);
	so_data  (so, fui(vpt->translate[0]));
	so_data  (so, fui(vpt->translate[1]));
	so_data  (so, fui(vpt->translate[2]));
	so_data  (so, fui(vpt->translate[3]));
	so_data  (so, fui(vpt->scale[0]));
	so_data  (so, fui(vpt->scale[1]));
	so_data  (so, fui(vpt->scale[2]));
	so_data  (so, fui(vpt->scale[3]));
	so_method(so, nv40->screen->curie, 0x1d78, 1);
	so_data  (so, 1);

	so_ref(so, &nv40->state.hw[NV40_STATE_VIEWPORT]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv40_state_entry nv40_state_viewport = {
	.validate = nv40_state_viewport_validate,
	.dirty = {
		.pipe = NV40_NEW_VIEWPORT | NV40_NEW_RAST,
		.hw = NV40_STATE_VIEWPORT
	}
};
