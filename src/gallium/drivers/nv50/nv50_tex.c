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

#define _MIXED(pf, t0, t1, t2, t3, cr, cg, cb, ca, f)		\
{                                                       	\
	PIPE_FORMAT_##pf,					\
	NV50TIC_0_0_MAPR_##cr | NV50TIC_0_0_TYPER_##t0 |	\
	NV50TIC_0_0_MAPG_##cg | NV50TIC_0_0_TYPEG_##t1 |	\
	NV50TIC_0_0_MAPB_##cb | NV50TIC_0_0_TYPEB_##t2 |	\
	NV50TIC_0_0_MAPA_##ca | NV50TIC_0_0_TYPEA_##t3 |	\
	NV50TIC_0_0_FMT_##f					\
}

#define _(pf, t, cr, cg, cb, ca, f) _MIXED(pf, t, t, t, t, cr, cg, cb, ca, f)

struct nv50_texture_format {
	enum pipe_format pf;
	uint32_t hw;
};

#define NV50_TEX_FORMAT_LIST_SIZE \
	(sizeof(nv50_tex_format_list) / sizeof(struct nv50_texture_format))

static const struct nv50_texture_format nv50_tex_format_list[] =
{
	_(A8R8G8B8_UNORM, UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(A8R8G8B8_SRGB,  UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(X8R8G8B8_UNORM, UNORM, C2, C1, C0, ONE, 8_8_8_8),
	_(X8R8G8B8_SRGB,  UNORM, C2, C1, C0, ONE, 8_8_8_8),
	_(A1R5G5B5_UNORM, UNORM, C2, C1, C0, C3,  1_5_5_5),
	_(A4R4G4B4_UNORM, UNORM, C2, C1, C0, C3,  4_4_4_4),

	_(R5G6B5_UNORM, UNORM, C2, C1, C0, ONE, 5_6_5),

	_(L8_UNORM, UNORM, C0, C0, C0, ONE, 8),
	_(A8_UNORM, UNORM, ZERO, ZERO, ZERO, C0, 8),
	_(I8_UNORM, UNORM, C0, C0, C0, C0, 8),

	_(A8L8_UNORM, UNORM, C0, C0, C0, C1, 8_8),

	_(DXT1_RGB, UNORM, C0, C1, C2, ONE, DXT1),
	_(DXT1_RGBA, UNORM, C0, C1, C2, C3, DXT1),
	_(DXT3_RGBA, UNORM, C0, C1, C2, C3, DXT3),
	_(DXT5_RGBA, UNORM, C0, C1, C2, C3, DXT5),

	_MIXED(Z24S8_UNORM, UINT, UNORM, UINT, UINT, C1, C1, C1, ONE, 24_8),

	_(R16G16B16A16_SNORM, UNORM, C0, C1, C2, C3, 16_16_16_16),
	_(R16G16B16A16_UNORM, SNORM, C0, C1, C2, C3, 16_16_16_16),
	_(R32G32B32A32_FLOAT, FLOAT, C0, C1, C2, C3, 32_32_32_32),

	_(R16G16_SNORM, SNORM, C0, C1, ZERO, ONE, 16_16),
	_(R16G16_UNORM, UNORM, C0, C1, ZERO, ONE, 16_16),

	_MIXED(Z32_FLOAT, FLOAT, UINT, UINT, UINT, C0, C0, C0, ONE, 32_DEPTH)

};

#undef _
#undef _MIXED

static int
nv50_tex_construct(struct nv50_context *nv50, struct nouveau_stateobj *so,
		   struct nv50_miptree *mt, int unit)
{
	unsigned i;
	uint32_t mode;

	for (i = 0; i < NV50_TEX_FORMAT_LIST_SIZE; i++)
		if (nv50_tex_format_list[i].pf == mt->base.base.format)
			break;
	if (i == NV50_TEX_FORMAT_LIST_SIZE)
                return 1;

	if (nv50->sampler[unit]->normalized)
		mode = 0x50001000 | (1 << 31);
	else {
		mode = 0x50001000 | (7 << 14);
		assert(mt->base.base.target == PIPE_TEXTURE_2D);
	}

	mode |= ((mt->base.bo->tile_mode & 0x0f) << 22) |
		((mt->base.bo->tile_mode & 0xf0) << 21);

	if (pf_type(mt->base.base.format) == PIPE_FORMAT_TYPE_SRGB)
		mode |= 0x0400;

	switch (mt->base.base.target) {
	case PIPE_TEXTURE_1D:
		break;
	case PIPE_TEXTURE_2D:
		mode |= (1 << 14);
		break;
	case PIPE_TEXTURE_3D:
		mode |= (2 << 14);
		break;
	case PIPE_TEXTURE_CUBE:
		mode |= (3 << 14);
		break;
	default:
		assert(!"unsupported texture target");
		break;
	}

	so_data (so, nv50_tex_format_list[i].hw);
	so_reloc(so, mt->base.bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
		 NOUVEAU_BO_RD, 0, 0);
	so_data (so, mode);
	so_data (so, 0x00300000);
	so_data (so, mt->base.base.width[0] | (1 << 31));
	so_data (so, (mt->base.base.last_level << 28) |
		 (mt->base.base.depth[0] << 16) | mt->base.base.height[0]);
	so_data (so, 0x03000000);
	so_data (so, mt->base.base.last_level << 4);

	return 0;
}

void
nv50_tex_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *eng2d = nv50->screen->eng2d;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	unsigned i, unit, push;

	push = MAX2(nv50->miptree_nr, nv50->state.miptree_nr) * 2 + 23 + 6;
	so = so_new(nv50->miptree_nr * 9 + push, nv50->miptree_nr * 2 + 2);

	nv50_so_init_sifc(nv50, so, nv50->screen->tic, NOUVEAU_BO_VRAM,
			  nv50->miptree_nr * 8 * 4);

	for (i = 0, unit = 0; unit < nv50->miptree_nr; ++unit) {
		struct nv50_miptree *mt = nv50->miptree[unit];

		if (!mt)
			continue;

		so_method(so, eng2d, NV50_2D_SIFC_DATA | (2 << 29), 8);
		if (nv50_tex_construct(nv50, so, mt, unit)) {
			NOUVEAU_ERR("failed tex validate\n");
			so_ref(NULL, &so);
			return;
		}

		so_method(so, tesla, NV50TCL_SET_SAMPLER_TEX, 1);
		so_data  (so, (i++ << NV50TCL_SET_SAMPLER_TEX_TIC_SHIFT) |
			  (unit << NV50TCL_SET_SAMPLER_TEX_SAMPLER_SHIFT) |
			  NV50TCL_SET_SAMPLER_TEX_VALID);
	}

	for (; unit < nv50->state.miptree_nr; unit++) {
		so_method(so, tesla, NV50TCL_SET_SAMPLER_TEX, 1);
		so_data  (so,
			  (unit << NV50TCL_SET_SAMPLER_TEX_SAMPLER_SHIFT) | 0);
	}

	/* not sure if the following really do what I think: */
	so_method(so, tesla, 0x1440, 1); /* sync SIFC */
	so_data  (so, 0);
	so_method(so, tesla, 0x1330, 1); /* flush TIC */
	so_data  (so, 0);
	so_method(so, tesla, 0x1338, 1); /* flush texture caches */
	so_data  (so, 0x20);

	so_ref(so, &nv50->state.tic_upload);
	so_ref(NULL, &so);
	nv50->state.miptree_nr = nv50->miptree_nr;
}

