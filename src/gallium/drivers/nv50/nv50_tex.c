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

#include "util/u_format.h"

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
	_(B8G8R8A8_UNORM, UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(B8G8R8A8_SRGB,  UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(B8G8R8X8_UNORM, UNORM, C2, C1, C0, ONE, 8_8_8_8),
	_(B8G8R8X8_SRGB,  UNORM, C2, C1, C0, ONE, 8_8_8_8),
	_(B5G5R5A1_UNORM, UNORM, C2, C1, C0, C3,  1_5_5_5),
	_(B4G4R4A4_UNORM, UNORM, C2, C1, C0, C3,  4_4_4_4),

	_(B5G6R5_UNORM, UNORM, C2, C1, C0, ONE, 5_6_5),

	_(L8_UNORM, UNORM, C0, C0, C0, ONE, 8),
	_(A8_UNORM, UNORM, ZERO, ZERO, ZERO, C0, 8),
	_(I8_UNORM, UNORM, C0, C0, C0, C0, 8),

	_(L8A8_UNORM, UNORM, C0, C0, C0, C1, 8_8),

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

static int
nv50_tex_construct(struct nv50_context *nv50, struct nouveau_stateobj *so,
		   struct nv50_miptree *mt, int unit, unsigned p)
{
	unsigned i;
	uint32_t mode;
	const struct util_format_description *desc;

	for (i = 0; i < NV50_TEX_FORMAT_LIST_SIZE; i++)
		if (nv50_tex_format_list[i].pf == mt->base.base.format)
			break;
	if (i == NV50_TEX_FORMAT_LIST_SIZE)
                return 1;

	if (nv50->sampler[p][unit]->normalized)
		mode = 0x50001000 | (1 << 31);
	else {
		mode = 0x50001000 | (7 << 14);
		assert(mt->base.base.target == PIPE_TEXTURE_2D);
	}

	mode |= ((mt->base.bo->tile_mode & 0x0f) << 22) |
		((mt->base.bo->tile_mode & 0xf0) << 21);

	desc = util_format_description(mt->base.base.format);
	assert(desc);

	if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
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
	so_data (so, mt->base.base.width0 | (1 << 31));
	so_data (so, (mt->base.base.last_level << 28) |
		 (mt->base.base.depth0 << 16) | mt->base.base.height0);
	so_data (so, 0x03000000);
	so_data (so, mt->base.base.last_level << 4);

	return 0;
}

#ifndef NV50TCL_BIND_TIC
#define NV50TCL_BIND_TIC(n) (0x1448 + 8 * n)
#endif

static boolean
nv50_validate_textures(struct nv50_context *nv50, struct nouveau_stateobj *so,
		       unsigned p)
{
	static const unsigned p_remap[PIPE_SHADER_TYPES] = { 0, 2, 1 };

	struct nouveau_grobj *eng2d = nv50->screen->eng2d;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned unit, j, p_hw = p_remap[p];

	nv50_so_init_sifc(nv50, so, nv50->screen->tic, NOUVEAU_BO_VRAM,
			  p * (32 * 8 * 4), nv50->miptree_nr[p] * 8 * 4);

	for (unit = 0; unit < nv50->miptree_nr[p]; ++unit) {
		struct nv50_miptree *mt = nv50->miptree[p][unit];

		so_method(so, eng2d, NV50_2D_SIFC_DATA | (2 << 29), 8);
		if (mt) {
			if (nv50_tex_construct(nv50, so, mt, unit, p))
				return FALSE;
			/* Set TEX insn $t src binding $unit in program type p
			 * to TIC, TSC entry (32 * p + unit), mark valid (1).
			 */
			so_method(so, tesla, NV50TCL_BIND_TIC(p_hw), 1);
			so_data  (so, ((32 * p + unit) << 9) | (unit << 1) | 1);
		} else {
			for (j = 0; j < 8; ++j)
				so_data(so, 0);
			so_method(so, tesla, NV50TCL_BIND_TIC(p_hw), 1);
			so_data  (so, (unit << 1) | 0);
		}
	}

	for (; unit < nv50->state.miptree_nr[p]; unit++) {
		/* Make other bindings invalid. */
		so_method(so, tesla, NV50TCL_BIND_TIC(p_hw), 1);
		so_data  (so, (unit << 1) | 0);
	}

	nv50->state.miptree_nr[p] = nv50->miptree_nr[p];
	return TRUE;
}

void
nv50_tex_validate(struct nv50_context *nv50)
{
	struct nouveau_stateobj *so;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned p, start, push, nrlc;

	for (nrlc = 0, start = 0, push = 0, p = 0; p < PIPE_SHADER_TYPES; ++p) {
		start += MAX2(nv50->miptree_nr[p], nv50->state.miptree_nr[p]);
		push += MAX2(nv50->miptree_nr[p], nv50->state.miptree_nr[p]);
		nrlc += nv50->miptree_nr[p];
	}
	start = start * 2 + 4 * PIPE_SHADER_TYPES + 2;
	push = push * 9 + 19 * PIPE_SHADER_TYPES + 2;
	nrlc = nrlc * 2 + 2 * PIPE_SHADER_TYPES;

	so = so_new(start, push, nrlc);

	if (nv50_validate_textures(nv50, so, PIPE_SHADER_VERTEX) == FALSE ||
	    nv50_validate_textures(nv50, so, PIPE_SHADER_FRAGMENT) == FALSE) {
		so_ref(NULL, &so);

		NOUVEAU_ERR("failed tex validate\n");
		return;
	}

	so_method(so, tesla, 0x1330, 1); /* flush TIC */
	so_data  (so, 0);

	so_ref(so, &nv50->state.tic_upload);
	so_ref(NULL, &so);
}
