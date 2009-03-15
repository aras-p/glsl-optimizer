#include "nv40_context.h"

static boolean
nv40_state_viewport_validate(struct nv40_context *nv40)
{
	struct pipe_viewport_state *vpt = &nv40->viewport;
	struct nouveau_stateobj *so;
	unsigned bypass;

	if (nv40->render_mode == HW &&
	    !nv40->rasterizer->pipe.bypass_vs_clip_and_viewport)
		bypass = 0;
	else
		bypass = 1;

	if (nv40->state.hw[NV40_STATE_VIEWPORT] &&
	    (bypass || !(nv40->dirty & NV40_NEW_VIEWPORT)) &&
	    nv40->state.viewport_bypass == bypass)
		return FALSE;
	nv40->state.viewport_bypass = bypass;

	so = so_new(11, 0);
	if (!bypass) {
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
	} else {
		so_method(so, nv40->screen->curie,
			  NV40TCL_VIEWPORT_TRANSLATE_X, 8);
		so_data  (so, fui(0.0));
		so_data  (so, fui(0.0));
		so_data  (so, fui(0.0));
		so_data  (so, fui(0.0));
		so_data  (so, fui(1.0));
		so_data  (so, fui(1.0));
		so_data  (so, fui(1.0));
		so_data  (so, fui(0.0));
		/* Not entirely certain what this is yet.  The DDX uses this
		 * value also as it fixes rendering when you pass
		 * pre-transformed vertices to the GPU.  My best gusss is that
		 * this bypasses some culling/clipping stage.  Might be worth
		 * noting that points/lines are uneffected by whatever this
		 * value fixes, only filled polygons are effected.
		 */
		so_method(so, nv40->screen->curie, 0x1d78, 1);
		so_data  (so, 0x110);
	}

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
