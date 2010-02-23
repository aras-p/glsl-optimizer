#include "nvfx_context.h"

/* Having this depend on FB and RAST looks wrong, but it seems
   necessary to make this work on nv3x
   TODO: find the right fix
*/

static boolean
nvfx_state_viewport_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct pipe_viewport_state *vpt = &nvfx->viewport;

	WAIT_RING(chan, 11);
	if(nvfx->render_mode == HW) {
		OUT_RING(chan, RING_3D(NV34TCL_VIEWPORT_TRANSLATE_X, 8));
		OUT_RINGf(chan, vpt->translate[0]);
		OUT_RINGf(chan, vpt->translate[1]);
		OUT_RINGf(chan, vpt->translate[2]);
		OUT_RINGf(chan, vpt->translate[3]);
		OUT_RINGf(chan, vpt->scale[0]);
		OUT_RINGf(chan, vpt->scale[1]);
		OUT_RINGf(chan, vpt->scale[2]);
		OUT_RINGf(chan, vpt->scale[3]);
		OUT_RING(chan, RING_3D(0x1d78, 1));
		OUT_RING(chan, 1);
	} else {
		OUT_RING(chan, RING_3D(NV34TCL_VIEWPORT_TRANSLATE_X, 8));
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RING(chan, RING_3D(0x1d78, 1));
		OUT_RING(chan, nvfx->is_nv4x ? 0x110 : 1);
	}
	return FALSE;
}

struct nvfx_state_entry nvfx_state_viewport = {
	.validate = nvfx_state_viewport_validate,
	.dirty = {
		.pipe = NVFX_NEW_VIEWPORT | NVFX_NEW_FB | NVFX_NEW_RAST,
	}
};
