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

#include "pipe/p_screen.h"

#include "nv50_context.h"
#include "nv50_screen.h"

#include "nouveau/nouveau_stateobj.h"

static boolean
nv50_screen_is_format_supported(struct pipe_screen *pscreen,
				enum pipe_format format,
				enum pipe_texture_target target,
				unsigned tex_usage, unsigned geom_flags)
{
	if (tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET) {
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM:
		case PIPE_FORMAT_Z24S8_UNORM:
		case PIPE_FORMAT_Z16_UNORM:
			return TRUE;
		default:
			break;
		}
	} else {
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_A1R5G5B5_UNORM:
		case PIPE_FORMAT_A4R4G4B4_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM:
		case PIPE_FORMAT_L8_UNORM:
		case PIPE_FORMAT_A8_UNORM:
		case PIPE_FORMAT_I8_UNORM:
		case PIPE_FORMAT_A8L8_UNORM:
		case PIPE_FORMAT_DXT1_RGB:
		case PIPE_FORMAT_DXT1_RGBA:
		case PIPE_FORMAT_DXT3_RGBA:
		case PIPE_FORMAT_DXT5_RGBA:
			return TRUE;
		default:
			break;
		}
	}

	return FALSE;
}

static int
nv50_screen_get_param(struct pipe_screen *pscreen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 32;
	case PIPE_CAP_NPOT_TEXTURES:
		return 1;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_S3TC:
		return 1;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 0;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 8;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 1;
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 1;
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		return 13;
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		return 10;
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 13;
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
	case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
		return 1;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 0;
	case NOUVEAU_CAP_HW_VTXBUF:
		return 1;
	case NOUVEAU_CAP_HW_IDXBUF:
		return 0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static float
nv50_screen_get_paramf(struct pipe_screen *pscreen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
		return 10.0;
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
		return 64.0;
	case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
		return 16.0;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return 4.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static void
nv50_screen_destroy(struct pipe_screen *pscreen)
{
	struct nv50_screen *screen = nv50_screen(pscreen);

	nouveau_screen_fini(&screen->base);
	FREE(screen);
}

struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *nvws)
{
	struct nv50_screen *screen = CALLOC_STRUCT(nv50_screen);
	struct nouveau_device *dev = nvws->channel->device;
	struct pipe_screen *pscreen;
	struct nouveau_stateobj *so;
	unsigned chipset = dev->chipset;
	unsigned tesla_class = 0;
	int ret, i;

	if (!screen)
		return NULL;
	pscreen = &screen->base.base;

	ret = nouveau_screen_init(&screen->base, nvws->channel->device);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

        /* Setup the pipe */
	screen->nvws = nvws;

	pscreen->winsys = ws;
	pscreen->destroy = nv50_screen_destroy;
	pscreen->get_param = nv50_screen_get_param;
	pscreen->get_paramf = nv50_screen_get_paramf;
	pscreen->is_format_supported = nv50_screen_is_format_supported;

	nv50_screen_init_miptree_functions(pscreen);
	nv50_transfer_init_screen_functions(pscreen);

	/* DMA engine object */
	ret = nvws->grobj_alloc(nvws, 0x5039, &screen->m2mf);
	if (ret) {
		NOUVEAU_ERR("Error creating M2MF object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* 2D object */
	ret = nvws->grobj_alloc(nvws, NV50_2D, &screen->eng2d);
	if (ret) {
		NOUVEAU_ERR("Error creating 2D object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* 3D object */
	switch (chipset & 0xf0) {
	case 0x50:
		tesla_class = 0x5097;
		break;
	case 0x80:
	case 0x90:
		tesla_class = 0x8297;
		break;
	case 0xa0:
		tesla_class = 0x8397;
		break;
	default:
		NOUVEAU_ERR("Not a known NV50 chipset: NV%02x\n", chipset);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	if (tesla_class == 0) {
		NOUVEAU_ERR("Unknown G8x chipset: NV%02x\n", chipset);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	ret = nvws->grobj_alloc(nvws, tesla_class, &screen->tesla);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* Sync notifier */
	ret = nvws->notifier_alloc(nvws, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* Static M2MF init */
	so = so_new(32, 0);
	so_method(so, screen->m2mf, 0x0180, 3);
	so_data  (so, screen->sync->handle);
	so_data  (so, screen->nvws->channel->vram->handle);
	so_data  (so, screen->nvws->channel->vram->handle);
	so_emit(nvws, so);
	so_ref (NULL, &so);

	/* Static 2D init */
	so = so_new(64, 0);
	so_method(so, screen->eng2d, NV50_2D_DMA_NOTIFY, 4);
	so_data  (so, screen->sync->handle);
	so_data  (so, screen->nvws->channel->vram->handle);
	so_data  (so, screen->nvws->channel->vram->handle);
	so_data  (so, screen->nvws->channel->vram->handle);
	so_method(so, screen->eng2d, NV50_2D_OPERATION, 1);
	so_data  (so, NV50_2D_OPERATION_SRCCOPY);
	so_method(so, screen->eng2d, 0x0290, 1);
	so_data  (so, 0);
	so_method(so, screen->eng2d, 0x0888, 1);
	so_data  (so, 1);
	so_emit(nvws, so);
	so_ref(NULL, &so);

	/* Static tesla init */
	so = so_new(256, 20);

	so_method(so, screen->tesla, 0x1558, 1);
	so_data  (so, 1);
	so_method(so, screen->tesla, NV50TCL_DMA_NOTIFY, 1);
	so_data  (so, screen->sync->handle);
	so_method(so, screen->tesla, NV50TCL_DMA_UNK0(0),
				     NV50TCL_DMA_UNK0__SIZE);
	for (i = 0; i < NV50TCL_DMA_UNK0__SIZE; i++)
		so_data(so, nvws->channel->vram->handle);
	so_method(so, screen->tesla, NV50TCL_DMA_UNK1(0),
				     NV50TCL_DMA_UNK1__SIZE);
	for (i = 0; i < NV50TCL_DMA_UNK1__SIZE; i++)
		so_data(so, nvws->channel->vram->handle);
	so_method(so, screen->tesla, 0x121c, 1);
	so_data  (so, 1);

	so_method(so, screen->tesla, 0x13bc, 1);
	so_data  (so, 0x54);
	so_method(so, screen->tesla, 0x13ac, 1);
	so_data  (so, 1);
	so_method(so, screen->tesla, 0x16b8, 1);
	so_data  (so, 8);

	/* constant buffers for immediates and VP/FP parameters */
	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 128*4*4,
			     &screen->constbuf_misc[0]);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	for (i = 0; i < 2; i++) {
		ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 128*4*4,
				     &screen->constbuf_parm[i]);
		if (ret) {
			nv50_screen_destroy(pscreen);
			return NULL;
		}
	}

	if (nvws->res_init(&screen->immd_heap[0], 0, 128) ||
		nvws->res_init(&screen->parm_heap[0], 0, 128) ||
		nvws->res_init(&screen->parm_heap[1], 0, 128))
	{
		NOUVEAU_ERR("Error initialising constant buffers.\n");
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/*
	// map constant buffers:
	//  B = buffer ID (maybe more than 1 byte)
	//  N = CB index used in shader instruction
	//  P = program type (0 = VP, 2 = GP, 3 = FP)
	so_method(so, screen->tesla, 0x1694, 1);
	so_data  (so, 0x000BBNP1);
	*/

	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf_misc[0], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_misc[0], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PMISC << 16) | 0x00000800);
	so_method(so, screen->tesla, 0x1694, 1);
	so_data  (so, 0x00000001 | (NV50_CB_PMISC << 12));
	so_method(so, screen->tesla, 0x1694, 1);
	so_data  (so, 0x00000031 | (NV50_CB_PMISC << 12));

	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf_parm[0], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_parm[0], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PVP << 16) | 0x00000800);
	so_method(so, screen->tesla, 0x1694, 1);
	so_data  (so, 0x00000101 | (NV50_CB_PVP << 12));

	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf_parm[1], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_parm[1], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PFP << 16) | 0x00000800);
	so_method(so, screen->tesla, 0x1694, 1);
	so_data  (so, 0x00000131 | (NV50_CB_PFP << 12));

	/* Texture sampler/image unit setup - we abuse the constant buffer
	 * upload mechanism for the moment to upload data to the tex config
	 * blocks.  At some point we *may* want to go the NVIDIA way of doing
	 * things?
	 */
	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 32*8*4, &screen->tic);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->tic, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->tic, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_TIC << 16) | 0x0800);
	so_method(so, screen->tesla, 0x1574, 3);
	so_reloc (so, screen->tic, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->tic, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00000800);

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 32*8*4, &screen->tsc);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->tsc, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->tsc, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_TSC << 16) | 0x0800);
	so_method(so, screen->tesla, 0x155c, 3);
	so_reloc (so, screen->tsc, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->tsc, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00000800);


	/* Vertex array limits - max them out */
	for (i = 0; i < 16; i++) {
		so_method(so, screen->tesla, 0x1080 + (i * 8), 2);
		so_data  (so, 0x000000ff);
		so_data  (so, 0xffffffff);
	}

	so_method(so, screen->tesla, NV50TCL_DEPTH_RANGE_NEAR, 2);
	so_data  (so, fui(0.0));
	so_data  (so, fui(1.0));

	so_method(so, screen->tesla, 0x1234, 1);
	so_data  (so, 1);
	so_method(so, screen->tesla, 0x1458, 1);
	so_data  (so, 1);

	so_emit(nvws, so);
	so_ref (so, &screen->static_init);
	so_ref (NULL, &so);
	nouveau_pushbuf_flush(nvws->channel, 0);

	return pscreen;
}

