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
#include "nv50_texture.h"
#include "nv50_resource.h"

#include "nouveau/nouveau_stateobj.h"
#include "nouveau/nouveau_reloc.h"

#include "util/u_format.h"

static INLINE uint32_t
nv50_tic_swizzle(uint32_t tc, unsigned swz)
{
	switch (swz) {
	case PIPE_SWIZZLE_RED:
		return (tc & NV50TIC_0_0_MAPR_MASK) >> NV50TIC_0_0_MAPR_SHIFT;
	case PIPE_SWIZZLE_GREEN:
		return (tc & NV50TIC_0_0_MAPG_MASK) >> NV50TIC_0_0_MAPG_SHIFT;
	case PIPE_SWIZZLE_BLUE:
		return (tc & NV50TIC_0_0_MAPB_MASK) >> NV50TIC_0_0_MAPB_SHIFT;
	case PIPE_SWIZZLE_ALPHA:
		return (tc & NV50TIC_0_0_MAPA_MASK) >> NV50TIC_0_0_MAPA_SHIFT;
	case PIPE_SWIZZLE_ONE:
		return 7;
	case PIPE_SWIZZLE_ZERO:
	default:
		return 0;
	}
}

boolean
nv50_tex_construct(struct nv50_sampler_view *view)
{
	const struct util_format_description *desc;
	struct nv50_miptree *mt = nv50_miptree(view->pipe.texture);
	uint32_t swz[4], *tic = view->tic;

	tic[0] = nv50_format_table[view->pipe.format].tic;

	swz[0] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_r);
	swz[1] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_g);
	swz[2] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_b);
	swz[3] = nv50_tic_swizzle(tic[0], view->pipe.swizzle_a);
	view->tic[0] = (tic[0] &  ~NV50TIC_0_0_SWIZZLE_MASK) |
		(swz[0] << NV50TIC_0_0_MAPR_SHIFT) |
		(swz[1] << NV50TIC_0_0_MAPG_SHIFT) |
		(swz[2] << NV50TIC_0_0_MAPB_SHIFT) |
		(swz[3] << NV50TIC_0_0_MAPA_SHIFT);

	tic[2] = 0x50001000;
	tic[2] |= ((mt->base.bo->tile_mode & 0x0f) << 22) |
		  ((mt->base.bo->tile_mode & 0xf0) << 21);

	desc = util_format_description(mt->base.base.format);
	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
		tic[2] |= NV50TIC_0_2_COLORSPACE_SRGB;

	switch (mt->base.base.target) {
	case PIPE_TEXTURE_1D:
		tic[2] |= NV50TIC_0_2_TARGET_1D;
		break;
	case PIPE_TEXTURE_2D:
		tic[2] |= NV50TIC_0_2_TARGET_2D;
		break;
	case PIPE_TEXTURE_RECT:
		tic[2] |= NV50TIC_0_2_TARGET_RECT;
		break;
	case PIPE_TEXTURE_3D:
		tic[2] |= NV50TIC_0_2_TARGET_3D;
		break;
	case PIPE_TEXTURE_CUBE:
		tic[2] |= NV50TIC_0_2_TARGET_CUBE;
		break;
	default:
		NOUVEAU_ERR("invalid texture target: %d\n",
			    mt->base.base.target);
		return FALSE;
	}

	tic[3] = 0x00300000;

	tic[4] = (1 << 31) | mt->base.base.width0;
	tic[5] = (mt->base.base.last_level << 28) |
		(mt->base.base.depth0 << 16) | mt->base.base.height0;

	tic[6] = 0x03000000;

	tic[7] = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;

	return TRUE;
}

static int
nv50_validate_textures(struct nv50_context *nv50, struct nouveau_stateobj *so,
		       unsigned p)
{
	struct nouveau_grobj *eng2d = nv50->screen->eng2d;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned unit, j;

	const unsigned rll = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW;
	const unsigned rlh = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_HIGH
		| NOUVEAU_BO_OR;

	nv50_so_init_sifc(nv50, so, nv50->screen->tic, NOUVEAU_BO_VRAM,
			  p * (32 * 8 * 4), nv50->sampler_view_nr[p] * 8 * 4);

	for (unit = 0; unit < nv50->sampler_view_nr[p]; ++unit) {
		struct nv50_sampler_view *view =
			nv50_sampler_view(nv50->sampler_views[p][unit]);

		so_method(so, eng2d, NV50_2D_SIFC_DATA | (2 << 29), 8);
		if (view) {
			uint32_t tic2 = view->tic[2];
			struct nv50_miptree *mt =
				nv50_miptree(view->pipe.texture);

			tic2 &= ~NV50TIC_0_2_NORMALIZED_COORDS;
			if (nv50->sampler[p][unit]->normalized)
				tic2 |= NV50TIC_0_2_NORMALIZED_COORDS;
			view->tic[2] = tic2;

			so_data  (so, view->tic[0]);
			so_reloc (so, mt->base.bo, 0, rll, 0, 0);
			so_reloc (so, mt->base.bo, 0, rlh, tic2, tic2);
			so_datap (so, &view->tic[3], 5);

			/* Set TEX insn $t src binding $unit in program type p
			 * to TIC, TSC entry (32 * p + unit), mark valid (1).
			 */
			so_method(so, tesla, NV50TCL_BIND_TIC(p), 1);
			so_data  (so, ((32 * p + unit) << 9) | (unit << 1) | 1);
		} else {
			for (j = 0; j < 8; ++j)
				so_data(so, 0);
			so_method(so, tesla, NV50TCL_BIND_TIC(p), 1);
			so_data  (so, (unit << 1) | 0);
		}
	}

	for (; unit < nv50->state.sampler_view_nr[p]; unit++) {
		/* Make other bindings invalid. */
		so_method(so, tesla, NV50TCL_BIND_TIC(p), 1);
		so_data  (so, (unit << 1) | 0);
	}

	nv50->state.sampler_view_nr[p] = nv50->sampler_view_nr[p];
	return TRUE;
}

static void
nv50_emit_texture_relocs(struct nv50_context *nv50, int prog)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_bo *tic = nv50->screen->tic;
	int unit;

	for (unit = 0; unit < nv50->sampler_view_nr[prog]; unit++) {
		struct nv50_sampler_view *view;
		struct nv50_miptree *mt;
		const unsigned base = ((prog * 32) + unit) * 32;

		view = nv50_sampler_view(nv50->sampler_views[prog][unit]);
		if (!view)
			continue;
		mt = nv50_miptree(view->pipe.texture);

		nouveau_reloc_emit(chan, tic, base + 4, NULL, mt->base.bo, 0, 0,
				   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
				   NOUVEAU_BO_LOW, 0, 0);
		nouveau_reloc_emit(chan, tic, base + 8, NULL, mt->base.bo, 0, 0,
				   NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
				   NOUVEAU_BO_HIGH, view->tic[2], view->tic[2]);
	}
}

void
nv50_tex_relocs(struct nv50_context *nv50)
{
	nv50_emit_texture_relocs(nv50, 2); /* FP */
	nv50_emit_texture_relocs(nv50, 0); /* VP */
}

struct nouveau_stateobj *
nv50_tex_validate(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned p, m = 0, d = 0, r = 0;

	for (p = 0; p < 3; ++p) {
		unsigned nr = MAX2(nv50->sampler_view_nr[p],
				   nv50->state.sampler_view_nr[p]);
		m += nr;
		d += nr;
		r += nv50->sampler_view_nr[p];
	}
	m = m * 2 + 3 * 4 + 1;
	d = d * 9 + 3 * 19 + 1;
	r = r * 2 + 3 * 2;

	so = so_new(m, d, r);

	if (nv50_validate_textures(nv50, so, 0) == FALSE ||
	    nv50_validate_textures(nv50, so, 2) == FALSE) {
		so_ref(NULL, &so);

		NOUVEAU_ERR("failed tex validate\n");
		return NULL;
	}

	so_method(so, tesla, 0x1330, 1); /* flush TIC */
	so_data  (so, 0);

	return so;
}
