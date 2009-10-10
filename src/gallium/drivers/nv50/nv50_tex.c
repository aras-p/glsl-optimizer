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

#define _(pf, tt, r, g, b, a, tf)                       	\
{                                                       	\
	PIPE_FORMAT_##pf,					\
	NV50TIC_0_0_MAPR_##r | NV50TIC_0_0_TYPER_##tt |		\
	NV50TIC_0_0_MAPG_##g | NV50TIC_0_0_TYPEG_##tt |		\
	NV50TIC_0_0_MAPB_##b | NV50TIC_0_0_TYPEB_##tt |		\
	NV50TIC_0_0_MAPA_##a | NV50TIC_0_0_TYPEA_##tt |		\
	NV50TIC_0_0_FMT_##tf					\
}

struct nv50_texture_format {
	enum pipe_format pf;
	uint32_t hw;
};

#define NV50_TEX_FORMAT_LIST_SIZE \
	(sizeof(nv50_tex_format_list) / sizeof(struct nv50_texture_format))

static const struct nv50_texture_format nv50_tex_format_list[] =
{
	_(A8R8G8B8_UNORM, UNORM, C2, C1, C0, C3,  8_8_8_8),
	_(X8R8G8B8_UNORM, UNORM, C2, C1, C0, ONE, 8_8_8_8),
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
	_(DXT5_RGBA, UNORM, C0, C1, C2, C3, DXT5)
};

#undef _

static int
nv50_tex_construct(struct nv50_context *nv50, struct nouveau_stateobj *so,
		   struct nv50_miptree *mt, int unit)
{
	unsigned i;

	for (i = 0; i < NV50_TEX_FORMAT_LIST_SIZE; i++)
		if (nv50_tex_format_list[i].pf == mt->base.base.format)
			break;
	if (i == NV50_TEX_FORMAT_LIST_SIZE)
                return 1;

	so_data (so, nv50_tex_format_list[i].hw);
	so_reloc(so, mt->base.bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
		     NOUVEAU_BO_RD, 0, 0);
	if (nv50->sampler[unit]->normalized)
		so_data (so, 0xd0005000 | mt->base.bo->tile_mode << 22);
	else
		so_data (so, 0x5001d000 | mt->base.bo->tile_mode << 22);
	so_data (so, 0x00300000);
	so_data (so, mt->base.base.width[0]);
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
	so = so_new(nv50->miptree_nr * 9 + push, nv50->miptree_nr + 2);

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

