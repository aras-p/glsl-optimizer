#include "nv30_context.h"

static boolean
nv30_state_viewport_validate(struct nv30_context *nv30)
{
	struct pipe_viewport_state *vpt = &nv30->viewport;
	struct nouveau_stateobj *so;

	if (nv30->state.hw[NV30_STATE_VIEWPORT] &&
	    !(nv30->dirty & NV30_NEW_VIEWPORT))
		return FALSE;

	so = so_new(3, 10, 0);
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
/*	so_method(so, nv30->screen->rankine, 0x1d78, 1);
	so_data  (so, 1);
*/
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
