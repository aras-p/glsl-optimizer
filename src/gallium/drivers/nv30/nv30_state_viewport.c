#include "nv30_context.h"

static boolean
nv30_state_viewport_validate(struct nv30_context *nv30)
{
	struct pipe_viewport_state *vpt = &nv30->viewport;
	struct nouveau_stateobj *so;
	unsigned bypass;

	if (/*nv30->render_mode == HW &&*/
	    !nv30->rasterizer->pipe.bypass_vs_clip_and_viewport)
		bypass = 0;
	else
		bypass = 1;

	if (nv30->state.hw[NV30_STATE_VIEWPORT] &&
	    (bypass || !(nv30->dirty & NV30_NEW_VIEWPORT)) &&
	    nv30->state.viewport_bypass == bypass)
		return FALSE;
	nv30->state.viewport_bypass = bypass;

	so = so_new(11, 0);
	if (!bypass) {
		so_method(so, nv30->screen->rankine,
			  NV34TCL_VIEWPORT_TRANSLATE_X, 8);
		so_data  (so, fui(vpt->translate[0]));
		so_data  (so, fui(vpt->translate[1]));
		so_data  (so, fui(vpt->translate[2]));
		so_data  (so, fui(vpt->translate[3]));
		so_data  (so, fui(vpt->scale[0]));
		so_data  (so, fui(vpt->scale[1]));
		so_data  (so, fui(vpt->scale[2]));
		so_data  (so, fui(vpt->scale[3]));
/*		so_method(so, nv30->screen->rankine, 0x1d78, 1);
		so_data  (so, 1);
*/	} else {
		so_method(so, nv30->screen->rankine,
			  NV34TCL_VIEWPORT_TRANSLATE_X, 8);
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
/*		so_method(so, nv30->screen->rankine, 0x1d78, 1);
		so_data  (so, 0x110);
*/	}
	/* TODO/FIXME: never saw value 0x0110 in renouveau dumps, only 0x0001 */
	so_method(so, nv30->screen->rankine, 0x1d78, 1);
	so_data  (so, 1);

	so_ref(so, &nv30->state.hw[NV30_STATE_VIEWPORT]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv30_state_entry nv30_state_viewport = {
	.validate = nv30_state_viewport_validate,
	.dirty = {
		.pipe = NV30_NEW_VIEWPORT | NV30_NEW_RAST,
		.hw = NV30_STATE_VIEWPORT
	}
};
