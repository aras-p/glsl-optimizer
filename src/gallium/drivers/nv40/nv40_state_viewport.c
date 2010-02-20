#include "nv40_context.h"

static boolean
nv40_state_viewport_validate(struct nvfx_context *nvfx)
{
	struct pipe_viewport_state *vpt = &nvfx->viewport;
	struct nouveau_stateobj *so;

	if (nvfx->state.hw[NVFX_STATE_VIEWPORT] &&
	    !(nvfx->dirty & NVFX_NEW_VIEWPORT))
		return FALSE;

	so = so_new(2, 9, 0);
	so_method(so, nvfx->screen->eng3d,
		  NV34TCL_VIEWPORT_TRANSLATE_X, 8);
	so_data  (so, fui(vpt->translate[0]));
	so_data  (so, fui(vpt->translate[1]));
	so_data  (so, fui(vpt->translate[2]));
	so_data  (so, fui(vpt->translate[3]));
	so_data  (so, fui(vpt->scale[0]));
	so_data  (so, fui(vpt->scale[1]));
	so_data  (so, fui(vpt->scale[2]));
	so_data  (so, fui(vpt->scale[3]));
	so_method(so, nvfx->screen->eng3d, 0x1d78, 1);
	so_data  (so, 1);

	so_ref(so, &nvfx->state.hw[NVFX_STATE_VIEWPORT]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nvfx_state_entry nv40_state_viewport = {
	.validate = nv40_state_viewport_validate,
	.dirty = {
		.pipe = NVFX_NEW_VIEWPORT | NVFX_NEW_RAST,
		.hw = NVFX_STATE_VIEWPORT
	}
};
