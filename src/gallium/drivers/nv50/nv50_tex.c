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

#include "nouveau/nouveau_stateobj.h"
#include "nouveau/nouveau_reloc.h"

#include "util/u_format.h"

#define _MIXED(pf, t0, t1, t2, t3, cr, cg, cb, ca, f)		\
[PIPE_FORMAT_##pf] = (						\
	NV50TIC_0_0_MAPR_##cr | NV50TIC_0_0_TYPER_##t0 |	\
	NV50TIC_0_0_MAPG_##cg | NV50TIC_0_0_TYPEG_##t1 |	\
	NV50TIC_0_0_MAPB_##cb | NV50TIC_0_0_TYPEB_##t2 |	\
	NV50TIC_0_0_MAPA_##ca | NV50TIC_0_0_TYPEA_##t3 |	\
	NV50TIC_0_0_FMT_##f)

#define _(pf, t, cr, cg, cb, ca, f) _MIXED(pf, t, t, t, t, cr, cg, cb, ca, f)

static const uint32_t nv50_texture_formats[PIPE_FORMAT_COUNT] =
{
	_(B8G8R8A8_UNORM, UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(B8G8R8A8_SRGB,  UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(B8G8R8X8_UNORM, UNORM, C2, C1, C0, ONE, 8_8_8_8),
	_(B8G8R8X8_SRGB,  UNORM, C2, C1, C0, ONE, 8_8_8_8),
	_(B5G5R5A1_UNORM, UNORM, C2, C1, C0, C3,  1_5_5_5),
	_(B4G4R4A4_UNORM, UNORM, C2, C1, C0, C3,  4_4_4_4),

	_(B5G6R5_UNORM, UNORM, C2, C1, C0, ONE, 5_6_5),

	_(L8_UNORM, UNORM, C0, C0, C0, ONE, 8),
	_(L8_SRGB,  UNORM, C0, C0, C0, ONE, 8),
	_(A8_UNORM, UNORM, ZERO, ZERO, ZERO, C0, 8),
	_(I8_UNORM, UNORM, C0, C0, C0, C0, 8),

	_(L8A8_UNORM, UNORM, C0, C0, C0, C1, 8_8),
	_(L8A8_SRGB,  UNORM, C0, C0, C0, C1, 8_8),

	_(DXT1_RGB, UNORM, C0, C1, C2, ONE, DXT1),
	_(DXT1_RGBA, UNORM, C0, C1, C2, C3, DXT1),
	_(DXT3_RGBA, UNORM, C0, C1, C2, C3, DXT3),
	_(DXT5_RGBA, UNORM, C0, C1, C2, C3, DXT5),

	_MIXED(S8Z24_UNORM, UINT, UNORM, UINT, UINT, C1, C1, C1, ONE, 24_8),
	_MIXED(Z24S8_UNORM, UNORM, UINT, UINT, UINT, C0, C0, C0, ONE, 8_24),

	_(R16G16B16A16_SNORM, UNORM, C0, C1, C2, C3, 16_16_16_16),
	_(R16G16B16A16_UNORM, SNORM, C0, C1, C2, C3, 16_16_16_16),
	_(R32G32B32A32_FLOAT, FLOAT, C0, C1, C2, C3, 32_32_32_32),

	_(R16G16_SNORM, SNORM, C0, C1, ZERO, ONE, 16_16),
	_(R16G16_UNORM, UNORM, C0, C1, ZERO, ONE, 16_16),

	_MIXED(Z32_FLOAT, FLOAT, UINT, UINT, UINT, C0, C0, C0, ONE, 32_DEPTH)
};

#undef _
#undef _MIXED

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

	tic[0] = nv50_texture_formats[view->pipe.format];

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

	tic[7] = (view->pipe.last_level << 4) | view->pipe.first_level;

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

			if (nv50->sampler[p][unit]->normalized)
				tic2 |= NV50TIC_0_2_NORMALIZED_COORDS;

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

void
nv50_tex_relocs(struct nv50_context *nv50)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	int p, unit;

	p = PIPE_SHADER_FRAGMENT;
	for (unit = 0; unit < nv50->sampler_view_nr[p]; unit++) {
		struct pipe_sampler_view *view = nv50->sampler_views[p][unit];
		if (!view)
			continue;
		nouveau_reloc_emit(chan, nv50->screen->tic,
				   ((p * 32) + unit) * 32, NULL,
				   nv50_miptree(view->texture)->base.bo, 0, 0,
				   NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
				   NOUVEAU_BO_RD, 0, 0);
	}

	p = PIPE_SHADER_VERTEX;
	for (unit = 0; unit < nv50->sampler_view_nr[p]; unit++) {
		struct pipe_sampler_view *view = nv50->sampler_views[p][unit];
		if (!view)
			continue;
		nouveau_reloc_emit(chan, nv50->screen->tic,
				   ((p * 32) + unit) * 32, NULL,
				   nv50_miptree(view->texture)->base.bo, 0, 0,
				   NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
				   NOUVEAU_BO_RD, 0, 0);
	}
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
