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
		case PIPE_FORMAT_B8G8R8X8_UNORM:
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B5G6R5_UNORM:
		case PIPE_FORMAT_R16G16B16A16_SNORM:
		case PIPE_FORMAT_R16G16B16A16_UNORM:
		case PIPE_FORMAT_R32G32B32A32_FLOAT:
		case PIPE_FORMAT_R16G16_SNORM:
		case PIPE_FORMAT_R16G16_UNORM:
			return TRUE;
		default:
			break;
		}
	} else
	if (tex_usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL) {
		switch (format) {
		case PIPE_FORMAT_Z32_FLOAT:
		case PIPE_FORMAT_S8Z24_UNORM:
		case PIPE_FORMAT_Z24X8_UNORM:
		case PIPE_FORMAT_Z24S8_UNORM:
			return TRUE;
		default:
			break;
		}
	} else {
		switch (format) {
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B8G8R8X8_UNORM:
		case PIPE_FORMAT_B8G8R8A8_SRGB:
		case PIPE_FORMAT_B8G8R8X8_SRGB:
		case PIPE_FORMAT_B5G5R5A1_UNORM:
		case PIPE_FORMAT_B4G4R4A4_UNORM:
		case PIPE_FORMAT_B5G6R5_UNORM:
		case PIPE_FORMAT_L8_UNORM:
		case PIPE_FORMAT_A8_UNORM:
		case PIPE_FORMAT_I8_UNORM:
		case PIPE_FORMAT_L8A8_UNORM:
		case PIPE_FORMAT_DXT1_RGB:
		case PIPE_FORMAT_DXT1_RGBA:
		case PIPE_FORMAT_DXT3_RGBA:
		case PIPE_FORMAT_DXT5_RGBA:
		case PIPE_FORMAT_S8Z24_UNORM:
		case PIPE_FORMAT_Z24S8_UNORM:
		case PIPE_FORMAT_Z32_FLOAT:
		case PIPE_FORMAT_R16G16B16A16_SNORM:
		case PIPE_FORMAT_R16G16B16A16_UNORM:
		case PIPE_FORMAT_R32G32B32A32_FLOAT:
		case PIPE_FORMAT_R16G16_SNORM:
		case PIPE_FORMAT_R16G16_UNORM:
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
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 32;
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 64;
	case PIPE_CAP_NPOT_TEXTURES:
		return 1;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 1;
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
	case PIPE_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		return 1;
	case NOUVEAU_CAP_HW_VTXBUF:
		return 1;
	case NOUVEAU_CAP_HW_IDXBUF:
		return 1;
	case PIPE_CAP_INDEP_BLEND_ENABLE:
		return 1;
	case PIPE_CAP_INDEP_BLEND_FUNC:
		return 0;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
		return 1;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
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
	unsigned i;

	for (i = 0; i < 3; i++) {
		if (screen->constbuf_parm[i])
			nouveau_bo_ref(NULL, &screen->constbuf_parm[i]);
	}

	if (screen->constbuf_misc[0])
		nouveau_bo_ref(NULL, &screen->constbuf_misc[0]);
	if (screen->tic)
		nouveau_bo_ref(NULL, &screen->tic);
	if (screen->tsc)
		nouveau_bo_ref(NULL, &screen->tsc);
	if (screen->static_init)
		so_ref(NULL, &screen->static_init);

	nouveau_notifier_free(&screen->sync);
	nouveau_grobj_free(&screen->tesla);
	nouveau_grobj_free(&screen->eng2d);
	nouveau_grobj_free(&screen->m2mf);
	nouveau_resource_destroy(&screen->immd_heap[0]);
	nouveau_resource_destroy(&screen->parm_heap[0]);
	nouveau_resource_destroy(&screen->parm_heap[1]);
	nouveau_screen_fini(&screen->base);
	FREE(screen);
}

static int
nv50_pre_pipebuffer_map(struct pipe_screen *pscreen, struct pipe_buffer *pb,
	unsigned usage)
{
	struct nv50_screen *screen = nv50_screen(pscreen);
	struct nv50_context *ctx = screen->cur_ctx;

	if (!(pb->usage & PIPE_BUFFER_USAGE_VERTEX))
		return 0;

	/* Our vtxbuf got mapped, it can no longer be considered part of current
	 * state, remove it to avoid emitting reloc markers.
	 */
	if (ctx && ctx->state.vtxbuf && so_bo_is_reloc(ctx->state.vtxbuf,
			nouveau_bo(pb))) {
		so_ref(NULL, &ctx->state.vtxbuf);
		ctx->dirty |= NV50_NEW_ARRAYS;
	}

	return 0;
}

struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
	struct nv50_screen *screen = CALLOC_STRUCT(nv50_screen);
	struct nouveau_channel *chan;
	struct pipe_screen *pscreen;
	struct nouveau_stateobj *so;
	unsigned chipset = dev->chipset;
	unsigned tesla_class = 0;
	int ret, i;

	if (!screen)
		return NULL;
	pscreen = &screen->base.base;

	ret = nouveau_screen_init(&screen->base, dev);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}
	chan = screen->base.channel;

	pscreen->winsys = ws;
	pscreen->destroy = nv50_screen_destroy;
	pscreen->get_param = nv50_screen_get_param;
	pscreen->get_paramf = nv50_screen_get_paramf;
	pscreen->is_format_supported = nv50_screen_is_format_supported;
	pscreen->context_create = nv50_create;
	screen->base.pre_pipebuffer_map_callback = nv50_pre_pipebuffer_map;

	nv50_screen_init_miptree_functions(pscreen);
	nv50_transfer_init_screen_functions(pscreen);

	/* DMA engine object */
	ret = nouveau_grobj_alloc(chan, 0xbeef5039,
		NV50_MEMORY_TO_MEMORY_FORMAT, &screen->m2mf);
	if (ret) {
		NOUVEAU_ERR("Error creating M2MF object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* 2D object */
	ret = nouveau_grobj_alloc(chan, 0xbeef502d, NV50_2D, &screen->eng2d);
	if (ret) {
		NOUVEAU_ERR("Error creating 2D object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* 3D object */
	switch (chipset & 0xf0) {
	case 0x50:
		tesla_class = NV50TCL;
		break;
	case 0x80:
	case 0x90:
		tesla_class = NV84TCL;
		break;
	case 0xa0:
		switch (chipset) {
		case 0xa0:
		case 0xaa:
		case 0xac:
			tesla_class = NVA0TCL;
			break;
		default:
			tesla_class = NVA8TCL;
			break;
		}
		break;
	default:
		NOUVEAU_ERR("Not a known NV50 chipset: NV%02x\n", chipset);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	ret = nouveau_grobj_alloc(chan, 0xbeef5097, tesla_class,
		&screen->tesla);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* Sync notifier */
	ret = nouveau_notifier_alloc(chan, 0xbeef0301, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* Static M2MF init */
	so = so_new(1, 3, 0);
	so_method(so, screen->m2mf, NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 3);
	so_data  (so, screen->sync->handle);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->vram->handle);
	so_emit(chan, so);
	so_ref (NULL, &so);

	/* Static 2D init */
	so = so_new(4, 7, 0);
	so_method(so, screen->eng2d, NV50_2D_DMA_NOTIFY, 4);
	so_data  (so, screen->sync->handle);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->vram->handle);
	so_method(so, screen->eng2d, NV50_2D_OPERATION, 1);
	so_data  (so, NV50_2D_OPERATION_SRCCOPY);
	so_method(so, screen->eng2d, NV50_2D_CLIP_ENABLE, 1);
	so_data  (so, 0);
	so_method(so, screen->eng2d, 0x0888, 1);
	so_data  (so, 1);
	so_emit(chan, so);
	so_ref(NULL, &so);

	/* Static tesla init */
	so = so_new(47, 95, 24);

	so_method(so, screen->tesla, NV50TCL_COND_MODE, 1);
	so_data  (so, NV50TCL_COND_MODE_ALWAYS);
	so_method(so, screen->tesla, NV50TCL_DMA_NOTIFY, 1);
	so_data  (so, screen->sync->handle);
	so_method(so, screen->tesla, NV50TCL_DMA_ZETA, 11);
	for (i = 0; i < 11; i++)
		so_data(so, chan->vram->handle);
	so_method(so, screen->tesla, NV50TCL_DMA_COLOR(0),
				     NV50TCL_DMA_COLOR__SIZE);
	for (i = 0; i < NV50TCL_DMA_COLOR__SIZE; i++)
		so_data(so, chan->vram->handle);
	so_method(so, screen->tesla, NV50TCL_RT_CONTROL, 1);
	so_data  (so, 1);

	/* activate all 32 lanes (threads) in a warp */
	so_method(so, screen->tesla, NV50TCL_WARP_HALVES, 1);
	so_data  (so, 0x2);
	so_method(so, screen->tesla, 0x1400, 1);
	so_data  (so, 0xf);

	/* max TIC (bits 4:8) & TSC (ignored) bindings, per program type */
	for (i = 0; i < 3; ++i) {
		so_method(so, screen->tesla, NV50TCL_TEX_LIMITS(i), 1);
		so_data  (so, 0x54);
	}

	/* origin is top left (set to 1 for bottom left) */
	so_method(so, screen->tesla, NV50TCL_Y_ORIGIN_BOTTOM, 1);
	so_data  (so, 0);
	so_method(so, screen->tesla, NV50TCL_VP_REG_ALLOC_RESULT, 1);
	so_data  (so, 8);

	/* constant buffers for immediates and VP/FP parameters */
	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, (32 * 4) * 4,
			     &screen->constbuf_misc[0]);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	for (i = 0; i < 3; i++) {
		ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, (256 * 4) * 4,
				     &screen->constbuf_parm[i]);
		if (ret) {
			nv50_screen_destroy(pscreen);
			return NULL;
		}
	}

	if (nouveau_resource_init(&screen->immd_heap[0], 0, 128) ||
	    nouveau_resource_init(&screen->parm_heap[0], 0, 512) ||
	    nouveau_resource_init(&screen->parm_heap[1], 0, 512))
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
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x000BBNP1);
	*/

	so_method(so, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	so_reloc (so, screen->constbuf_misc[0], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_misc[0], 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PMISC << 16) | 0x00000200);
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000001 | (NV50_CB_PMISC << 12));
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000021 | (NV50_CB_PMISC << 12));
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000031 | (NV50_CB_PMISC << 12));

	/* bind auxiliary constbuf to immediate data bo */
	so_method(so, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	so_reloc (so, screen->constbuf_misc[0], (128 * 4) * 4,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_misc[0], (128 * 4) * 4,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_AUX << 16) | 0x00000200);
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000201 | (NV50_CB_AUX << 12));
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000221 | (NV50_CB_AUX << 12));

	so_method(so, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	so_reloc (so, screen->constbuf_parm[PIPE_SHADER_VERTEX], 0,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_parm[PIPE_SHADER_VERTEX], 0,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PVP << 16) | 0x00000800);
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000101 | (NV50_CB_PVP << 12));

	so_method(so, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	so_reloc (so, screen->constbuf_parm[PIPE_SHADER_GEOMETRY], 0,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_parm[PIPE_SHADER_GEOMETRY], 0,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PGP << 16) | 0x00000800);
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000121 | (NV50_CB_PGP << 12));

	so_method(so, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	so_reloc (so, screen->constbuf_parm[PIPE_SHADER_FRAGMENT], 0,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf_parm[PIPE_SHADER_FRAGMENT], 0,
		  NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PFP << 16) | 0x00000800);
	so_method(so, screen->tesla, NV50TCL_SET_PROGRAM_CB, 1);
	so_data  (so, 0x00000131 | (NV50_CB_PFP << 12));

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, PIPE_SHADER_TYPES*32*32,
			     &screen->tic);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	so_method(so, screen->tesla, NV50TCL_TIC_ADDRESS_HIGH, 3);
	so_reloc (so, screen->tic, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->tic, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, PIPE_SHADER_TYPES * 32 - 1);

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, PIPE_SHADER_TYPES*32*32,
			     &screen->tsc);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	so_method(so, screen->tesla, NV50TCL_TSC_ADDRESS_HIGH, 3);
	so_reloc (so, screen->tsc, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->tsc, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00000000); /* ignored if TSC_LINKED (0x1234) = 1 */


	/* Vertex array limits - max them out */
	for (i = 0; i < 16; i++) {
		so_method(so, screen->tesla, NV50TCL_VERTEX_ARRAY_LIMIT_HIGH(i), 2);
		so_data  (so, 0x000000ff);
		so_data  (so, 0xffffffff);
	}

	so_method(so, screen->tesla, NV50TCL_DEPTH_RANGE_NEAR(0), 2);
	so_data  (so, fui(0.0));
	so_data  (so, fui(1.0));

	/* no dynamic combination of TIC & TSC entries => only BIND_TIC used */
	so_method(so, screen->tesla, NV50TCL_LINKED_TSC, 1);
	so_data  (so, 1);

	/* activate first scissor rectangle */
	so_method(so, screen->tesla, NV50TCL_SCISSOR_ENABLE(0), 1);
	so_data  (so, 1);

	so_method(so, screen->tesla, NV50TCL_EDGEFLAG_ENABLE, 1);
	so_data  (so, 1); /* default edgeflag to TRUE */

	so_emit(chan, so);
	so_ref (so, &screen->static_init);
	so_ref (NULL, &so);
	nouveau_pushbuf_flush(chan, 0);

	return pscreen;
}

