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

void
nv50_tex_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	int i;

	so = so_new(nv50->miptree_nr * 8 + 3, nv50->miptree_nr * 2);
	so_method(so, tesla, 0x0f00, 1);
	so_data  (so, NV50_CB_TIC);
	so_method(so, tesla, 0x40000f04, nv50->miptree_nr * 8);
	for (i = 0; i < nv50->miptree_nr; i++) {
		struct nv50_miptree *mt = nv50->miptree[i];

		so_data (so, 0x2a712488);
		so_reloc(so, mt->buffer, 0, NOUVEAU_BO_VRAM |
			     NOUVEAU_BO_LOW, 0, 0);
		so_data (so, 0xd0005000);
		so_data (so, 0x00300000);
		so_data (so, mt->base.width[0]);
		so_data (so, (mt->base.depth[0] << 16) |
			      mt->base.height[0]);
		so_data (so, 0x03000000);
		so_reloc(so, mt->buffer, 0, NOUVEAU_BO_VRAM |
			     NOUVEAU_BO_HIGH, 0, 0);
	}

	so_ref(so, &nv50->state.tic_upload);
}

