#include "nv30_context.h"

static boolean
nv30_state_blend_validate(struct nv30_context *nv30)
{
	so_ref(nv30->blend->so, &nv30->state.hw[NV30_STATE_BLEND]);
	return TRUE;
}

struct nv30_state_entry nv30_state_blend = {
	.validate = nv30_state_blend_validate,
	.dirty = {
		.pipe = NV30_NEW_BLEND,
		.hw = NV30_STATE_BLEND
	}
};

static boolean
nv30_state_blend_colour_validate(struct nv30_context *nv30)
{
	struct nouveau_stateobj *so = so_new(1, 1, 0);
	struct pipe_blend_color *bcol = &nv30->blend_colour;

	so_method(so, nv30->screen->rankine, NV34TCL_BLEND_COLOR, 1);
	so_data  (so, ((float_to_ubyte(bcol->color[3]) << 24) |
		       (float_to_ubyte(bcol->color[0]) << 16) |
		       (float_to_ubyte(bcol->color[1]) <<  8) |
		       (float_to_ubyte(bcol->color[2]) <<  0)));

	so_ref(so, &nv30->state.hw[NV30_STATE_BCOL]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv30_state_entry nv30_state_blend_colour = {
	.validate = nv30_state_blend_colour_validate,
	.dirty = {
		.pipe = NV30_NEW_BCOL,
		.hw = NV30_STATE_BCOL
	}
};
