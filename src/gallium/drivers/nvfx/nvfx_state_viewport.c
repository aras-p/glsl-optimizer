#include "nvfx_context.h"

/* Having this depend on FB and RAST looks wrong, but it seems
   necessary to make this work on nv3x
   TODO: find the right fix
*/

static boolean
nvfx_state_viewport_validate(struct nvfx_context *nvfx)
{
	struct pipe_viewport_state *vpt = &nvfx->viewport;
	struct nouveau_stateobj *so;

	so = so_new(2, 9, 0);
	so_method(so, nvfx->screen->eng3d,
		  NV34TCL_VIEWPORT_TRANSLATE_X, 8);
	if(nvfx->render_mode == HW) {
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
	} else {
		so_data  (so, fui(0.0f));
		so_data  (so, fui(0.0f));
		so_data  (so, fui(0.0f));
		so_data  (so, fui(0.0f));
		so_data  (so, fui(1.0f));
		so_data  (so, fui(1.0f));
		so_data  (so, fui(1.0f));
		so_data  (so, fui(1.0f));
		so_method(so, nvfx->screen->eng3d, 0x1d78, 1);
		so_data  (so, nvfx->is_nv4x ? 0x110 : 1);
	}

	so_ref(so, &nvfx->state.hw[NVFX_STATE_VIEWPORT]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nvfx_state_entry nvfx_state_viewport = {
	.validate = nvfx_state_viewport_validate,
	.dirty = {
		.pipe = NVFX_NEW_VIEWPORT | NVFX_NEW_FB | NVFX_NEW_RAST,
		.hw = NVFX_STATE_VIEWPORT
	}
};
