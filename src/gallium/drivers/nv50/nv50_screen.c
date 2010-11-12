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

#include "util/u_format_s3tc.h"
#include "pipe/p_screen.h"

#include "nv50_context.h"
#include "nv50_screen.h"
#include "nv50_resource.h"
#include "nv50_program.h"

#include "nouveau/nouveau_stateobj.h"

static boolean
nv50_screen_is_format_supported(struct pipe_screen *pscreen,
				enum pipe_format format,
				enum pipe_texture_target target,
				unsigned sample_count,
				unsigned usage, unsigned geom_flags)
{
	if (sample_count > 1)
		return FALSE;

	if (!util_format_s3tc_enabled) {
		switch (format) {
		case PIPE_FORMAT_DXT1_RGB:
		case PIPE_FORMAT_DXT1_RGBA:
		case PIPE_FORMAT_DXT3_RGBA:
		case PIPE_FORMAT_DXT5_RGBA:
			return FALSE;
		default:
			break;
		}
	}

	switch (format) {
	case PIPE_FORMAT_Z16_UNORM:
		if ((nouveau_screen(pscreen)->device->chipset & 0xf0) != 0xa0)
			return FALSE;
		break;
	default:
		break;
	}

	/* transfers & shared are always supported */
	usage &= ~(PIPE_BIND_TRANSFER_READ |
		   PIPE_BIND_TRANSFER_WRITE |
		   PIPE_BIND_SHARED);

	return (nv50_format_table[format].usage & usage) == usage;
}

static int
nv50_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
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
	case PIPE_CAP_SM3:
		return 1;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 1;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 8;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 1;
        case PIPE_CAP_TIMER_QUERY:
		return 0;
	case PIPE_CAP_STREAM_OUTPUT:
		return 0;
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
	case PIPE_CAP_TEXTURE_SWIZZLE:
		return 1;
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		return 1;
	case PIPE_CAP_INDEP_BLEND_ENABLE:
		return 1;
	case PIPE_CAP_INDEP_BLEND_FUNC:
		return 0;
	case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
		return 1;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
		return 1;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
		return 0;
	case PIPE_CAP_DEPTH_CLAMP:
		return 1;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static int
nv50_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
			     enum pipe_shader_cap param)
{
	switch(shader) {
	case PIPE_SHADER_FRAGMENT:
	case PIPE_SHADER_VERTEX:
		break;
	case PIPE_SHADER_GEOMETRY:
	default:
		return 0;
	}

	switch(param) {
	case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
	case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS: /* arbitrary limit */
		return 16384;
	case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH: /* need stack bo */
		return 4;
	case PIPE_SHADER_CAP_MAX_INPUTS: /* 128 / 4 with GP */
		if (shader == PIPE_SHADER_GEOMETRY)
			return 128 / 4;
		else
			return 64 / 4;
	case PIPE_SHADER_CAP_MAX_CONSTS:
		return 65536 / 16;
	case PIPE_SHADER_CAP_MAX_CONST_BUFFERS: /* 16 - 1, but not implemented */
		return 1;
	case PIPE_SHADER_CAP_MAX_ADDRS: /* no spilling atm */
		return 1;
	case PIPE_SHADER_CAP_MAX_PREDS: /* not yet handled */
		return 0;
	case PIPE_SHADER_CAP_MAX_TEMPS: /* no spilling atm */
		return NV50_CAP_MAX_PROGRAM_TEMPS;
	case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		return 1;
	case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
	case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
		return 1;
	default:
		return 0;
	}
}

static float
nv50_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_cap param)
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

	nouveau_notifier_free(&screen->sync);
	nouveau_grobj_free(&screen->tesla);
	nouveau_grobj_free(&screen->eng2d);
	nouveau_grobj_free(&screen->m2mf);
	nouveau_resource_destroy(&screen->immd_heap);
	nouveau_screen_fini(&screen->base);
	FREE(screen);
}

#define BGN_RELOC(ch, bo, gr, m, n, fl) \
   OUT_RELOC(ch, bo, (n << 18) | (gr->subc << 13) | m, fl, 0, 0)

void
nv50_screen_reloc_constbuf(struct nv50_screen *screen, unsigned cbi)
{
	struct nouveau_bo *bo;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *tesla = screen->tesla;
	unsigned size;
	const unsigned rl = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_DUMMY;

	switch (cbi) {
	case NV50_CB_PMISC:
		bo = screen->constbuf_misc[0];
		size = 0x200;
		break;
	case NV50_CB_PVP:
	case NV50_CB_PFP:
	case NV50_CB_PGP:
		bo = screen->constbuf_parm[cbi - NV50_CB_PVP];
		size = 0;
		break;
	default:
		return;
	}

	BGN_RELOC (chan, bo, tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3, rl);
	OUT_RELOCh(chan, bo, 0, rl);
	OUT_RELOCl(chan, bo, 0, rl);
	OUT_RELOC (chan, bo, (cbi << 16) | size, rl, 0, 0);
}

void
nv50_screen_relocs(struct nv50_screen *screen)
{
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *tesla = screen->tesla;
	unsigned i;
	const unsigned rl = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD | NOUVEAU_BO_DUMMY;

	MARK_RING (chan, 28, 26);

	/* cause grobj autobind */
	BEGIN_RING(chan, tesla, 0x0100, 1);
	OUT_RING  (chan, 0);

	BGN_RELOC (chan, screen->tic, tesla, NV50TCL_TIC_ADDRESS_HIGH, 2, rl);
	OUT_RELOCh(chan, screen->tic, 0, rl);
	OUT_RELOCl(chan, screen->tic, 0, rl);

	BGN_RELOC (chan, screen->tsc, tesla, NV50TCL_TSC_ADDRESS_HIGH, 2, rl);
	OUT_RELOCh(chan, screen->tsc, 0, rl);
	OUT_RELOCl(chan, screen->tsc, 0, rl);

	nv50_screen_reloc_constbuf(screen, NV50_CB_PMISC);

	BGN_RELOC (chan, screen->constbuf_misc[0],
		   tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3, rl);
	OUT_RELOCh(chan, screen->constbuf_misc[0], 0x200, rl);
	OUT_RELOCl(chan, screen->constbuf_misc[0], 0x200, rl);
	OUT_RELOC (chan, screen->constbuf_misc[0],
		   (NV50_CB_AUX << 16) | 0x0200, rl, 0, 0);

	for (i = 0; i < 3; ++i)
		nv50_screen_reloc_constbuf(screen, NV50_CB_PVP + i);

	BGN_RELOC (chan, screen->stack_bo,
		   tesla, NV50TCL_STACK_ADDRESS_HIGH, 2, rl);
	OUT_RELOCh(chan, screen->stack_bo, 0, rl);
	OUT_RELOCl(chan, screen->stack_bo, 0, rl);

	if (!screen->cur_ctx->req_lmem)
		return;

	BGN_RELOC (chan, screen->local_bo,
		   tesla, NV50TCL_LOCAL_ADDRESS_HIGH, 2, rl);
	OUT_RELOCh(chan, screen->local_bo, 0, rl);
	OUT_RELOCl(chan, screen->local_bo, 0, rl);
}

#ifndef NOUVEAU_GETPARAM_GRAPH_UNITS
# define NOUVEAU_GETPARAM_GRAPH_UNITS 13
#endif

extern int nouveau_device_get_param(struct nouveau_device *dev,
                                    uint64_t param, uint64_t *value);

struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
	struct nv50_screen *screen = CALLOC_STRUCT(nv50_screen);
	struct nouveau_channel *chan;
	struct pipe_screen *pscreen;
	uint64_t value;
	unsigned chipset = dev->chipset;
	unsigned tesla_class = 0;
	unsigned stack_size, local_size, max_warps;
	int ret, i;
	const unsigned rl = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

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
	pscreen->get_shader_param = nv50_screen_get_shader_param;
	pscreen->get_paramf = nv50_screen_get_paramf;
	pscreen->is_format_supported = nv50_screen_is_format_supported;
	pscreen->context_create = nv50_create;

	nv50_screen_init_resource_functions(pscreen);

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

	/* this is necessary for the new RING_3D / statebuffer code */
	BIND_RING(chan, screen->tesla, 7);

	/* Sync notifier */
	ret = nouveau_notifier_alloc(chan, 0xbeef0301, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	/* Static M2MF init */
	BEGIN_RING(chan, screen->m2mf,
		   NV04_MEMORY_TO_MEMORY_FORMAT_DMA_NOTIFY, 3);
	OUT_RING  (chan, screen->sync->handle);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);

	/* Static 2D init */
	BEGIN_RING(chan, screen->eng2d, NV50_2D_DMA_NOTIFY, 4);
	OUT_RING  (chan, screen->sync->handle);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);
	BEGIN_RING(chan, screen->eng2d, NV50_2D_OPERATION, 1);
	OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
	BEGIN_RING(chan, screen->eng2d, NV50_2D_CLIP_ENABLE, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, screen->eng2d, 0x0888, 1);
	OUT_RING  (chan, 1);

	/* Static tesla init */
	BEGIN_RING(chan, screen->tesla, NV50TCL_COND_MODE, 1);
	OUT_RING  (chan, NV50TCL_COND_MODE_ALWAYS);
	BEGIN_RING(chan, screen->tesla, NV50TCL_DMA_NOTIFY, 1);
	OUT_RING  (chan, screen->sync->handle);
	BEGIN_RING(chan, screen->tesla, NV50TCL_DMA_ZETA, 11);
	for (i = 0; i < 11; i++)
		OUT_RING  (chan, chan->vram->handle);
	BEGIN_RING(chan, screen->tesla,
		   NV50TCL_DMA_COLOR(0), NV50TCL_DMA_COLOR__SIZE);
	for (i = 0; i < NV50TCL_DMA_COLOR__SIZE; i++)
		OUT_RING  (chan, chan->vram->handle);

	BEGIN_RING(chan, screen->tesla, NV50TCL_RT_CONTROL, 1);
	OUT_RING  (chan, 1);

	/* activate all 32 lanes (threads) in a warp */
	BEGIN_RING(chan, screen->tesla, NV50TCL_REG_MODE, 1);
	OUT_RING  (chan, NV50TCL_REG_MODE_STRIPED);
	BEGIN_RING(chan, screen->tesla, 0x1400, 1);
	OUT_RING  (chan, 0xf);

	/* max TIC (bits 4:8) & TSC (ignored) bindings, per program type */
	for (i = 0; i < 3; ++i) {
		BEGIN_RING(chan, screen->tesla, NV50TCL_TEX_LIMITS(i), 1);
		OUT_RING  (chan, 0x54);
	}

	/* origin is top left (set to 1 for bottom left) */
	BEGIN_RING(chan, screen->tesla, NV50TCL_Y_ORIGIN_BOTTOM, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, screen->tesla, NV50TCL_VP_REG_ALLOC_RESULT, 1);
	OUT_RING  (chan, 8);

	BEGIN_RING(chan, screen->tesla, NV50TCL_CLEAR_FLAGS, 1);
	OUT_RING  (chan, NV50TCL_CLEAR_FLAGS_D3D);

	/* constant buffers for immediates and VP/FP parameters */
	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, (32 * 4) * 4,
			     &screen->constbuf_misc[0]);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}
	BEGIN_RING(chan, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, screen->constbuf_misc[0], 0, rl);
	OUT_RELOCl(chan, screen->constbuf_misc[0], 0, rl);
	OUT_RING  (chan, (NV50_CB_PMISC << 16) | 0x0200);
	BEGIN_RING(chan, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, screen->constbuf_misc[0], 0x200, rl);
	OUT_RELOCl(chan, screen->constbuf_misc[0], 0x200, rl);
	OUT_RING  (chan, (NV50_CB_AUX << 16) | 0x0200);

	for (i = 0; i < 3; i++) {
		ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, (4096 * 4) * 4,
				     &screen->constbuf_parm[i]);
		if (ret) {
			nv50_screen_destroy(pscreen);
			return NULL;
		}
		BEGIN_RING(chan, screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
		OUT_RELOCh(chan, screen->constbuf_parm[i], 0, rl);
		OUT_RELOCl(chan, screen->constbuf_parm[i], 0, rl);
		/* CB_DEF_SET_SIZE value of 0x0000 means 65536 */
		OUT_RING  (chan, ((NV50_CB_PVP + i) << 16) | 0x0000);
	}

	if (nouveau_resource_init(&screen->immd_heap, 0, 128)) {
		NOUVEAU_ERR("Error initialising shader immediates heap.\n");
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 3 * 32 * (8 * 4),
			     &screen->tic);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}
	BEGIN_RING(chan, screen->tesla, NV50TCL_TIC_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, screen->tic, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, screen->tic, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RING  (chan, 3 * 32 - 1);

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 3 * 32 * (8 * 4),
			     &screen->tsc);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}
	BEGIN_RING(chan, screen->tesla, NV50TCL_TSC_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, screen->tsc, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, screen->tsc, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RING  (chan, 0); /* ignored if TSC_LINKED (0x1234) == 1 */

	/* map constant buffers:
	 *  B = buffer ID (maybe more than 1 byte)
	 *  N = CB index used in shader instruction
	 *  P = program type (0 = VP, 2 = GP, 3 = FP)
	 * SET_PROGRAM_CB = 0x000BBNP1
	 */
	BEGIN_RING_NI(chan, screen->tesla, NV50TCL_SET_PROGRAM_CB, 8);
	/* bind immediate buffer */
	OUT_RING  (chan, 0x001 | (NV50_CB_PMISC << 12));
	OUT_RING  (chan, 0x021 | (NV50_CB_PMISC << 12));
	OUT_RING  (chan, 0x031 | (NV50_CB_PMISC << 12));
	/* bind auxiliary constbuf to immediate data bo */
	OUT_RING  (chan, 0x201 | (NV50_CB_AUX << 12));
	OUT_RING  (chan, 0x221 | (NV50_CB_AUX << 12));
	/* bind parameter buffers */
	OUT_RING  (chan, 0x101 | (NV50_CB_PVP << 12));
	OUT_RING  (chan, 0x121 | (NV50_CB_PGP << 12));
	OUT_RING  (chan, 0x131 | (NV50_CB_PFP << 12));

	/* shader stack */
	nouveau_device_get_param(dev, NOUVEAU_GETPARAM_GRAPH_UNITS, &value);

	max_warps  = util_bitcount(value & 0xffff);
	max_warps *= util_bitcount((value >> 24) & 0xf) * 32;

	stack_size = max_warps * 64 * 8;

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16,
			     stack_size, &screen->stack_bo);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}
	BEGIN_RING(chan, screen->tesla, NV50TCL_STACK_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, screen->stack_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, screen->stack_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RING  (chan, 4);

	local_size = (NV50_CAP_MAX_PROGRAM_TEMPS * 16) * max_warps * 32;

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16,
			     local_size, &screen->local_bo);
	if (ret) {
		nv50_screen_destroy(pscreen);
		return NULL;
	}

	local_size = NV50_CAP_MAX_PROGRAM_TEMPS * 16;

	BEGIN_RING(chan, screen->tesla, NV50TCL_LOCAL_ADDRESS_HIGH, 3);
	OUT_RELOCh(chan, screen->local_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, screen->local_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RING  (chan, util_unsigned_logbase2(local_size / 8));

	/* Vertex array limits - max them out */
	for (i = 0; i < 16; i++) {
		BEGIN_RING(chan, screen->tesla,
			   NV50TCL_VERTEX_ARRAY_LIMIT_HIGH(i), 2);
		OUT_RING  (chan, 0x000000ff);
		OUT_RING  (chan, 0xffffffff);
	}

	BEGIN_RING(chan, screen->tesla, NV50TCL_DEPTH_RANGE_NEAR(0), 2);
	OUT_RINGf (chan, 0.0f);
	OUT_RINGf (chan, 1.0f);

	BEGIN_RING(chan, screen->tesla, NV50TCL_VIEWPORT_TRANSFORM_EN, 1);
	OUT_RING  (chan, 1);

	/* no dynamic combination of TIC & TSC entries => only BIND_TIC used */
	BEGIN_RING(chan, screen->tesla, NV50TCL_LINKED_TSC, 1);
	OUT_RING  (chan, 1);

	BEGIN_RING(chan, screen->tesla, NV50TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING  (chan, 1); /* default edgeflag to TRUE */

	FIRE_RING (chan);

	screen->force_push = debug_get_bool_option("NV50_ALWAYS_PUSH", FALSE);
	if(!screen->force_push)
		screen->base.vertex_buffer_flags = screen->base.index_buffer_flags = NOUVEAU_BO_GART;
	return pscreen;
}

