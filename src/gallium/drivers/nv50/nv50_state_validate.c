#include "nv50_context.h"
#include "nouveau/nouveau_stateobj.h"

boolean
nv50_state_validate(struct nv50_context *nv50)
{
	struct nouveau_winsys *nvws = nv50->screen->nvws;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	unsigned i;

	if (nv50->dirty & NV50_NEW_BLEND)
		so_emit(nvws, nv50->blend->so);

	if (nv50->dirty & NV50_NEW_ZSA)
		so_emit(nvws, nv50->zsa->so);

	if (nv50->dirty & NV50_NEW_RASTERIZER)
		so_emit(nvws, nv50->rasterizer->so);

	if (nv50->dirty & NV50_NEW_BLEND_COLOUR) {
		so = so_new(5, 0);
		so_method(so, tesla, NV50TCL_BLEND_COLOR(0), 8);
		so_data  (so, fui(nv50->blend_colour.color[3]));
		so_data  (so, fui(nv50->blend_colour.color[0]));
		so_data  (so, fui(nv50->blend_colour.color[1]));
		so_data  (so, fui(nv50->blend_colour.color[2]));
		so_emit(nvws, so);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & NV50_NEW_STIPPLE) {
		so = so_new(33, 0);
		so_method(so, tesla, NV50TCL_POLYGON_STIPPLE_PATTERN(0), 32);
		for (i = 0; i < 32; i++)
			so_data(so, nv50->stipple.stipple[i]);
		so_emit(nvws, so);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & NV50_NEW_SCISSOR) {
		so = so_new(3, 0);
		so_method(so, tesla, NV50TCL_SCISSOR_HORIZ, 2);
		so_data  (so, (nv50->scissor.maxx << 16) |
			       nv50->scissor.minx);
		so_data  (so, (nv50->scissor.maxy << 16) |
			       nv50->scissor.miny);
		so_emit(nvws, so);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & NV50_NEW_VIEWPORT) {
		so = so_new(8, 0);
		so_method(so, tesla, NV50TCL_VIEWPORT_UNK0(0), 3);
		so_data  (so, fui(nv50->viewport.translate[0]));
		so_data  (so, fui(nv50->viewport.translate[1]));
		so_data  (so, fui(nv50->viewport.translate[2]));
		so_method(so, tesla, NV50TCL_VIEWPORT_UNK1(0), 3);
		so_data  (so, fui(nv50->viewport.scale[0]));
		so_data  (so, fui(nv50->viewport.scale[1]));
		so_data  (so, fui(nv50->viewport.scale[2]));
		so_emit(nvws, so);
		so_ref(NULL, &so);
	}

	nv50->dirty = 0;
	return TRUE;
}

