#include "nv40_context.h"

static boolean
nv40_state_blend_validate(struct nv40_context *nv40)
{
	so_ref(nv40->blend->so, &nv40->state.hw[NV40_STATE_BLEND]);
	return TRUE;
}

struct nv40_state_entry nv40_state_blend = {
	.validate = nv40_state_blend_validate,
	.dirty = {
		.pipe = NV40_NEW_BLEND,
		.hw = NV40_STATE_BLEND
	}
};

static boolean
nv40_state_blend_colour_validate(struct nv40_context *nv40)
{
	struct nouveau_stateobj *so = so_new(1, 1, 0);
	struct pipe_blend_color *bcol = &nv40->blend_colour;

	so_method(so, nv40->screen->curie, NV40TCL_BLEND_COLOR, 1);
	so_data  (so, ((float_to_ubyte(bcol->color[3]) << 24) |
		       (float_to_ubyte(bcol->color[0]) << 16) |
		       (float_to_ubyte(bcol->color[1]) <<  8) |
		       (float_to_ubyte(bcol->color[2]) <<  0)));

	so_ref(so, &nv40->state.hw[NV40_STATE_BCOL]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv40_state_entry nv40_state_blend_colour = {
	.validate = nv40_state_blend_colour_validate,
	.dirty = {
		.pipe = NV40_NEW_BCOL,
		.hw = NV40_STATE_BCOL
	}
};
