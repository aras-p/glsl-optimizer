#include "nv50_context.h"
#include "nouveau/nouveau_stateobj.h"

static void
nv50_state_validate_fb(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(128, 18);
	struct pipe_framebuffer_state *fb = &nv50->framebuffer;
	unsigned i, w, h, gw = 0;

	for (i = 0; i < fb->num_cbufs; i++) {
		if (!gw) {
			w = fb->cbufs[i]->width;
			h = fb->cbufs[i]->height;
			gw = 1;
		} else {
			assert(w == fb->cbufs[i]->width);
			assert(h == fb->cbufs[i]->height);
		}

		so_method(so, tesla, NV50TCL_RT_HORIZ(i), 2);
		so_data  (so, fb->cbufs[i]->width);
		so_data  (so, fb->cbufs[i]->height);

		so_method(so, tesla, NV50TCL_RT_ADDRESS_HIGH(i), 5);
		so_reloc (so, fb->cbufs[i]->buffer, fb->cbufs[i]->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (so, fb->cbufs[i]->buffer, fb->cbufs[i]->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW, 0, 0);
		switch (fb->cbufs[i]->format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
			so_data(so, 0xcf);
			break;
		case PIPE_FORMAT_R5G6B5_UNORM:
			so_data(so, 0xe8);
			break;
		default:
			{
				char fmt[128];
				pf_sprint_name(fmt, fb->cbufs[i]->format);
				NOUVEAU_ERR("AIIII unknown format %s\n", fmt);
			}		    
			so_data(so, 0xe6);
			break;
		}
		so_data(so, 0x00000040);
		so_data(so, 0x00000000);

		so_method(so, tesla, 0x1224, 1);
		so_data  (so, 1);
	}

	if (fb->zsbuf) {
		if (!gw) {
			w = fb->zsbuf->width;
			h = fb->zsbuf->height;
			gw = 1;
		} else {
			assert(w == fb->zsbuf->width);
			assert(h == fb->zsbuf->height);
		}

		so_method(so, tesla, NV50TCL_ZETA_ADDRESS_HIGH, 5);
		so_reloc (so, fb->zsbuf->buffer, fb->zsbuf->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (so, fb->zsbuf->buffer, fb->zsbuf->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW, 0, 0);
		switch (fb->zsbuf->format) {
		case PIPE_FORMAT_Z24S8_UNORM:
			so_data(so, 0x16);
			break;
		default:
			{
				char fmt[128];
				pf_sprint_name(fmt, fb->zsbuf->format);
				NOUVEAU_ERR("AIIII unknown format %s\n", fmt);
			}		    
			so_data(so, 0x16);
			break;
		}
		so_data(so, 0x00000040);
		so_data(so, 0x00000000);
	}

	so_method(so, tesla, NV50TCL_VIEWPORT_HORIZ, 2);
	so_data  (so, w << 16);
	so_data  (so, h << 16);
	so_method(so, tesla, 0xff4, 2);
	so_data  (so, w << 16);
	so_data  (so, h << 16);
	so_method(so, tesla, 0xdf8, 2);
	so_data  (so, 0);
	so_data  (so, h);

	so_emit(nv50->screen->nvws, so);
	so_ref(NULL, &so);
}

boolean
nv50_state_validate(struct nv50_context *nv50)
{
	struct nouveau_winsys *nvws = nv50->screen->nvws;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	unsigned i;

	if (nv50->dirty & NV50_NEW_FRAMEBUFFER)
		nv50_state_validate_fb(nv50);

	if (nv50->dirty & NV50_NEW_BLEND)
		so_emit(nvws, nv50->blend->so);

	if (nv50->dirty & NV50_NEW_ZSA)
		so_emit(nvws, nv50->zsa->so);

	if (nv50->dirty & NV50_NEW_VERTPROG)
		nv50_vertprog_validate(nv50);

	if (nv50->dirty & NV50_NEW_FRAGPROG)
		nv50_fragprog_validate(nv50);

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
		so_data  (so, fui(-nv50->viewport.scale[1]));
		so_data  (so, fui(nv50->viewport.scale[2]));
		so_emit(nvws, so);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & NV50_NEW_ARRAYS)
		nv50_vbo_validate(nv50);

	nv50->dirty = 0;
	return TRUE;
}

