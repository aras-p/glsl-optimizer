/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_context.h"
#include "nouveau/nouveau_stateobj.h"

static void
nv50_state_validate_fb(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(128, 18);
	struct pipe_framebuffer_state *fb = &nv50->framebuffer;
	unsigned i, w, h, gw = 0;

	for (i = 0; i < fb->nr_cbufs; i++) {
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
		so_reloc (so, nv50_surface_buffer(fb->cbufs[i]), fb->cbufs[i]->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_HIGH |
			  NOUVEAU_BO_RDWR, 0, 0);
		so_reloc (so, nv50_surface_buffer(fb->cbufs[i]), fb->cbufs[i]->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
			  NOUVEAU_BO_RDWR, 0, 0);
		switch (fb->cbufs[i]->format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
			so_data(so, 0xcf);
			break;
		case PIPE_FORMAT_R5G6B5_UNORM:
			so_data(so, 0xe8);
			break;
		default:
			NOUVEAU_ERR("AIIII unknown format %s\n",
				    pf_name(fb->cbufs[i]->format));
			so_data(so, 0xe6);
			break;
		}
		so_data(so, 0x00000000);
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
		so_reloc (so, nv50_surface_buffer(fb->zsbuf), fb->zsbuf->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_HIGH |
			  NOUVEAU_BO_RDWR, 0, 0);
		so_reloc (so, nv50_surface_buffer(fb->zsbuf), fb->zsbuf->offset,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
			  NOUVEAU_BO_RDWR, 0, 0);
		switch (fb->zsbuf->format) {
		case PIPE_FORMAT_Z24S8_UNORM:
			so_data(so, 0x16);
			break;
		case PIPE_FORMAT_Z16_UNORM:
			so_data(so, 0x15);
			break;
		default:
			NOUVEAU_ERR("AIIII unknown format %s\n",
				    pf_name(fb->zsbuf->format));
			so_data(so, 0x16);
			break;
		}
		so_data(so, 0x00000000);
		so_data(so, 0x00000000);

		so_method(so, tesla, 0x1538, 1);
		so_data  (so, 1);
		so_method(so, tesla, 0x1228, 3);
		so_data  (so, fb->zsbuf->width);
		so_data  (so, fb->zsbuf->height);
		so_data  (so, 0x00010001);
	}

	so_method(so, tesla, NV50TCL_VIEWPORT_HORIZ, 2);
	so_data  (so, w << 16);
	so_data  (so, h << 16);
	so_method(so, tesla, 0x0e04, 2);
	so_data  (so, w << 16);
	so_data  (so, h << 16);
	so_method(so, tesla, 0xdf8, 2);
	so_data  (so, 0);
	so_data  (so, h);

	so_ref(so, &nv50->state.fb);
	so_ref(NULL, &so);
}

static void
nv50_state_emit(struct nv50_context *nv50)
{
	struct nv50_screen *screen = nv50->screen;
	struct nouveau_winsys *nvws = screen->nvws;

	if (nv50->pctx_id != screen->cur_pctx) {
		nv50->state.dirty |= 0xffffffff;
		screen->cur_pctx = nv50->pctx_id;
	}

	if (nv50->state.dirty & NV50_NEW_FRAMEBUFFER)
		so_emit(nvws, nv50->state.fb);
	if (nv50->state.dirty & NV50_NEW_BLEND)
		so_emit(nvws, nv50->state.blend);
	if (nv50->state.dirty & NV50_NEW_ZSA)
		so_emit(nvws, nv50->state.zsa);
	if (nv50->state.dirty & NV50_NEW_VERTPROG)
		so_emit(nvws, nv50->state.vertprog);
	if (nv50->state.dirty & NV50_NEW_FRAGPROG)
		so_emit(nvws, nv50->state.fragprog);
	if (nv50->state.dirty & NV50_NEW_RASTERIZER)
		so_emit(nvws, nv50->state.rast);
	if (nv50->state.dirty & NV50_NEW_BLEND_COLOUR)
		so_emit(nvws, nv50->state.blend_colour);
	if (nv50->state.dirty & NV50_NEW_STIPPLE)
		so_emit(nvws, nv50->state.stipple);
	if (nv50->state.dirty & NV50_NEW_SCISSOR)
		so_emit(nvws, nv50->state.scissor);
	if (nv50->state.dirty & NV50_NEW_VIEWPORT)
		so_emit(nvws, nv50->state.viewport);
	if (nv50->state.dirty & NV50_NEW_SAMPLER)
		so_emit(nvws, nv50->state.tsc_upload);
	if (nv50->state.dirty & NV50_NEW_TEXTURE)
		so_emit(nvws, nv50->state.tic_upload);
	if (nv50->state.dirty & NV50_NEW_ARRAYS) {
		so_emit(nvws, nv50->state.vtxfmt);
		so_emit(nvws, nv50->state.vtxbuf);
	}
	nv50->state.dirty = 0;

	so_emit_reloc_markers(nvws, nv50->state.fb);
	so_emit_reloc_markers(nvws, nv50->state.vertprog);
	so_emit_reloc_markers(nvws, nv50->state.fragprog);
	so_emit_reloc_markers(nvws, nv50->state.vtxbuf);
	so_emit_reloc_markers(nvws, nv50->screen->static_init);
}

boolean
nv50_state_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	unsigned i;

	if (nv50->dirty & NV50_NEW_FRAMEBUFFER)
		nv50_state_validate_fb(nv50);

	if (nv50->dirty & NV50_NEW_BLEND)
		so_ref(nv50->blend->so, &nv50->state.blend);

	if (nv50->dirty & NV50_NEW_ZSA)
		so_ref(nv50->zsa->so, &nv50->state.zsa);

	if (nv50->dirty & (NV50_NEW_VERTPROG | NV50_NEW_VERTPROG_CB))
		nv50_vertprog_validate(nv50);

	if (nv50->dirty & (NV50_NEW_FRAGPROG | NV50_NEW_FRAGPROG_CB))
		nv50_fragprog_validate(nv50);

	if (nv50->dirty & NV50_NEW_RASTERIZER)
		so_ref(nv50->rasterizer->so, &nv50->state.rast);

	if (nv50->dirty & NV50_NEW_BLEND_COLOUR) {
		so = so_new(5, 0);
		so_method(so, tesla, NV50TCL_BLEND_COLOR(0), 4);
		so_data  (so, fui(nv50->blend_colour.color[0]));
		so_data  (so, fui(nv50->blend_colour.color[1]));
		so_data  (so, fui(nv50->blend_colour.color[2]));
		so_data  (so, fui(nv50->blend_colour.color[3]));
		so_ref(so, &nv50->state.blend_colour);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & NV50_NEW_STIPPLE) {
		so = so_new(33, 0);
		so_method(so, tesla, NV50TCL_POLYGON_STIPPLE_PATTERN(0), 32);
		for (i = 0; i < 32; i++)
			so_data(so, nv50->stipple.stipple[i]);
		so_ref(so, &nv50->state.stipple);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & (NV50_NEW_SCISSOR | NV50_NEW_RASTERIZER)) {
		struct pipe_rasterizer_state *rast = &nv50->rasterizer->pipe;
		struct pipe_scissor_state *s = &nv50->scissor;

		if (nv50->state.scissor &&
		    (rast->scissor == 0 && nv50->state.scissor_enabled == 0))
			goto scissor_uptodate;
		nv50->state.scissor_enabled = rast->scissor;

		so = so_new(3, 0);
		so_method(so, tesla, 0x0ff4, 2);
		if (nv50->state.scissor_enabled) {
			so_data(so, ((s->maxx - s->minx) << 16) | s->minx);
			so_data(so, ((s->maxy - s->miny) << 16) | s->miny);
		} else {
			so_data(so, (8192 << 16));
			so_data(so, (8192 << 16));
		}
		so_ref(so, &nv50->state.scissor);
		so_ref(NULL, &so);
		nv50->state.dirty |= NV50_NEW_SCISSOR;
	}
scissor_uptodate:

	if (nv50->dirty & (NV50_NEW_VIEWPORT | NV50_NEW_RASTERIZER)) {
		unsigned bypass;

		if (!nv50->rasterizer->pipe.bypass_vs_clip_and_viewport)
			bypass = 0;
		else
			bypass = 1;

		if (nv50->state.viewport &&
		    (bypass || !(nv50->dirty & NV50_NEW_VIEWPORT)) &&
		    nv50->state.viewport_bypass == bypass)
			goto viewport_uptodate;
		nv50->state.viewport_bypass = bypass;

		so = so_new(12, 0);
		if (!bypass) {
			so_method(so, tesla, NV50TCL_VIEWPORT_UNK1(0), 3);
			so_data  (so, fui(nv50->viewport.translate[0]));
			so_data  (so, fui(nv50->viewport.translate[1]));
			so_data  (so, fui(nv50->viewport.translate[2]));
			so_method(so, tesla, NV50TCL_VIEWPORT_UNK0(0), 3);
			so_data  (so, fui(nv50->viewport.scale[0]));
			so_data  (so, fui(-nv50->viewport.scale[1]));
			so_data  (so, fui(nv50->viewport.scale[2]));
			so_method(so, tesla, 0x192c, 1);
			so_data  (so, 1);
			so_method(so, tesla, 0x0f90, 1);
			so_data  (so, 0);
		} else {
			so_method(so, tesla, 0x192c, 1);
			so_data  (so, 0);
			so_method(so, tesla, 0x0f90, 1);
			so_data  (so, 1);
		}

		so_ref(so, &nv50->state.viewport);
		so_ref(NULL, &so);
		nv50->state.dirty |= NV50_NEW_VIEWPORT;
	}
viewport_uptodate:

	if (nv50->dirty & NV50_NEW_SAMPLER) {
		int i;

		so = so_new(nv50->sampler_nr * 8 + 3, 0);
		so_method(so, tesla, 0x0f00, 1);
		so_data  (so, NV50_CB_TSC);
		so_method(so, tesla, 0x40000f04, nv50->sampler_nr * 8);
		for (i = 0; i < nv50->sampler_nr; i++)
			so_datap (so, nv50->sampler[i], 8);
		so_ref(so, &nv50->state.tsc_upload);
		so_ref(NULL, &so);
	}

	if (nv50->dirty & NV50_NEW_TEXTURE)
		nv50_tex_validate(nv50);

	if (nv50->dirty & NV50_NEW_ARRAYS)
		nv50_vbo_validate(nv50);

	nv50->state.dirty |= nv50->dirty;
	nv50->dirty = 0;
	nv50_state_emit(nv50);

	return TRUE;
}

