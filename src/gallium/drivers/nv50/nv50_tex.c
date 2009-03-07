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

static int
nv50_tex_construct(struct nouveau_stateobj *so, struct nv50_miptree *mt)
{
	switch (mt->base.format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_C3 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_8_8_8_8);
		break;
	case PIPE_FORMAT_A1R5G5B5_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_C3 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_1_5_5_5);
		break;
	case PIPE_FORMAT_A4R4G4B4_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_C3 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_4_4_4_4);
		break;
	case PIPE_FORMAT_R5G6B5_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_ONE | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_5_6_5);
		break;
	case PIPE_FORMAT_L8_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_ONE | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C0 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C0 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_8);
		break;
	case PIPE_FORMAT_A8_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_C0 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_ZERO | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_ZERO | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_ZERO | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_8);
		break;
	case PIPE_FORMAT_I8_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_C0 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C0 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C0 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_8);
		break;
	case PIPE_FORMAT_A8L8_UNORM:
		so_data(so, NV50TIC_0_0_MAPA_C1 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C0 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C0 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_8_8);
		break;
	case PIPE_FORMAT_DXT1_RGB:
		so_data(so, NV50TIC_0_0_MAPA_ONE | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_DXT1);
		break;
	case PIPE_FORMAT_DXT1_RGBA:
		so_data(so, NV50TIC_0_0_MAPA_C3 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_DXT1);
		break;
	case PIPE_FORMAT_DXT3_RGBA:
		so_data(so, NV50TIC_0_0_MAPA_C3 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_DXT3);
		break;
	case PIPE_FORMAT_DXT5_RGBA:
		so_data(so, NV50TIC_0_0_MAPA_C3 | NV50TIC_0_0_TYPEA_UNORM |
			    NV50TIC_0_0_MAPR_C0 | NV50TIC_0_0_TYPER_UNORM |
			    NV50TIC_0_0_MAPG_C1 | NV50TIC_0_0_TYPEG_UNORM |
			    NV50TIC_0_0_MAPB_C2 | NV50TIC_0_0_TYPEB_UNORM |
			    NV50TIC_0_0_FMT_DXT5);
		break;
	default:
		return 1;
	}

	so_reloc(so, mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
		     NOUVEAU_BO_RD, 0, 0);
	so_data (so, 0xd0005000);
	so_data (so, 0x00300000);
	so_data (so, mt->base.width[0]);
	so_data (so, (mt->base.depth[0] << 16) | mt->base.height[0]);
	so_data (so, 0x03000000);
	so_reloc(so, mt->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_HIGH |
		     NOUVEAU_BO_RD, 0, 0);

	return 0;
}

void
nv50_tex_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	int unit;

	so = so_new(nv50->miptree_nr * 8 + 3, nv50->miptree_nr * 2);
	so_method(so, tesla, 0x0f00, 1);
	so_data  (so, NV50_CB_TIC);
	so_method(so, tesla, 0x40000f04, nv50->miptree_nr * 8);
	for (unit = 0; unit < nv50->miptree_nr; unit++) {
		struct nv50_miptree *mt = nv50->miptree[unit];

		if (nv50_tex_construct(so, mt)) {
			NOUVEAU_ERR("failed tex validate\n");
			so_ref(NULL, &so);
			return;
		}
	}

	so_ref(so, &nv50->state.tic_upload);
	so_ref(NULL, &so);
}

