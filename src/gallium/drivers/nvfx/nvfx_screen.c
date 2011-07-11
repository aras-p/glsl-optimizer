#include "pipe/p_screen.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_simple_screen.h"

#include "nouveau/nouveau_screen.h"
#include "nouveau/nv_object.xml.h"
#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_resource.h"
#include "nvfx_tex.h"

#define NV30_3D_CHIPSET_3X_MASK 0x00000003
#define NV34_3D_CHIPSET_3X_MASK 0x00000010
#define NV35_3D_CHIPSET_3X_MASK 0x000001e0

#define NV4X_GRCLASS4097_CHIPSETS 0x00000baf
#define NV4X_GRCLASS4497_CHIPSETS 0x00005450
#define NV6X_GRCLASS4497_CHIPSETS 0x00000088

static int
nvfx_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);

	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 16;
	case PIPE_CAP_NPOT_TEXTURES:
		return screen->advertise_npot;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 1;
	case PIPE_CAP_SM3:
		/* TODO: >= nv4x support Shader Model 3.0 */
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 1;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return screen->use_nv4x ? 4 : 1;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 1;
        case PIPE_CAP_TIMER_QUERY:
		return 0;
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 1;
	case PIPE_CAP_TEXTURE_SWIZZLE:
		return 1;
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		return 13;
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		return 10;
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 13;
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
		return !!screen->use_nv4x;
	case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
		return 1;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 0; /* We have 4 on nv40 - but unsupported currently */
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		return screen->advertise_blend_equation_separate;
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 16;
	case PIPE_CAP_INDEP_BLEND_ENABLE:
		/* TODO: on nv40 we have separate color masks */
		/* TODO: nv40 mrt blending is probably broken */
		return 0;
	case PIPE_CAP_INDEP_BLEND_FUNC:
		return 0;
	case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
		return 0;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
		return 1;
	case PIPE_CAP_DEPTH_CLAMP:
		return 0; // TODO: implement depth clamp
	case PIPE_CAP_PRIMITIVE_RESTART:
		return 0; // TODO: implement primitive restart
	case PIPE_CAP_ARRAY_TEXTURES:
	case PIPE_CAP_TGSI_INSTANCEID:
	case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
	case PIPE_CAP_FRAGMENT_COLOR_CLAMP_CONTROL:
	case PIPE_CAP_SEAMLESS_CUBE_MAP:
	case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
	case PIPE_CAP_SHADER_STENCIL_EXPORT:
		return 0;
	case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
                return 0;
	default:
		NOUVEAU_ERR("Warning: unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static int
nvfx_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader, enum pipe_shader_cap param)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);

	switch(shader) {
	case PIPE_SHADER_FRAGMENT:
		switch(param) {
		case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
			return 4096;
		case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
			/* FIXME: is it the dynamic (nv30:0/nv40:24) or the static
			 value (nv30:0/nv40:4) ? */
			return screen->use_nv4x ? 4 : 0;
		case PIPE_SHADER_CAP_MAX_INPUTS:
			return screen->use_nv4x ? 12 : 10;
		case PIPE_SHADER_CAP_MAX_CONSTS:
			return screen->use_nv4x ? 224 : 32;
		case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
		    return 1;
		case PIPE_SHADER_CAP_MAX_TEMPS:
			return 32;
		case PIPE_SHADER_CAP_MAX_ADDRS:
			return screen->use_nv4x ? 1 : 0;
		case PIPE_SHADER_CAP_MAX_PREDS:
			return 0; /* we could expose these, but nothing uses them */
		case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
		    return 0;
		case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
			return 0;
		case PIPE_SHADER_CAP_SUBROUTINES:
			return screen->use_nv4x ? 1 : 0;
		default:
			break;
		}
		break;
	case PIPE_SHADER_VERTEX:
		switch(param) {
		case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
			return screen->use_nv4x ? 512 : 256;
		case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
		case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
			return screen->use_nv4x ? 512 : 0;
		case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
			/* FIXME: is it the dynamic (nv30:24/nv40:24) or the static
			 value (nv30:1/nv40:4) ? */
			return screen->use_nv4x ? 4 : 1;
		case PIPE_SHADER_CAP_MAX_INPUTS:
			return 16;
		case PIPE_SHADER_CAP_MAX_CONSTS:
			/* - 6 is for clip planes; Gallium should be fixed to put
			 * them in the vertex shader itself, so we don't need to reserve these */
			return (screen->use_nv4x ? 468 : 256) - 6;
	             case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
	                    return 1;
		case PIPE_SHADER_CAP_MAX_TEMPS:
			return screen->use_nv4x ? 32 : 13;
		case PIPE_SHADER_CAP_MAX_ADDRS:
			return 2;
		case PIPE_SHADER_CAP_MAX_PREDS:
			return 0; /* we could expose these, but nothing uses them */
		case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
                        return 1;
		case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
		case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
			return 0;
		case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
			return 1;
		case PIPE_SHADER_CAP_SUBROUTINES:
			return 1;
		default:
			break;
		}
		break;
	default:
		break;
	}
	return 0;
}

static float
nvfx_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_cap param)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);

	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
		return 10.0;
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
		return 64.0;
	case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
		return screen->use_nv4x ? 16.0 : 8.0;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return 15.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static boolean
nvfx_screen_is_format_supported(struct pipe_screen *pscreen,
				     enum pipe_format format,
				     enum pipe_texture_target target,
				     unsigned sample_count,
                                     unsigned bind)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);

        if (!util_format_is_supported(format, bind))
                return FALSE;

	 if (sample_count > 1)
		return FALSE;

	if (bind & PIPE_BIND_RENDER_TARGET) {
		switch (format) {
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B8G8R8X8_UNORM:
		case PIPE_FORMAT_R8G8B8A8_UNORM:
		case PIPE_FORMAT_R8G8B8X8_UNORM:
		case PIPE_FORMAT_B5G6R5_UNORM:
			break;
		case PIPE_FORMAT_R16G16B16A16_FLOAT:
			if(!screen->advertise_fp16)
				return FALSE;
			break;
		case PIPE_FORMAT_R32G32B32A32_FLOAT:
			if(!screen->advertise_fp32)
				return FALSE;
			break;
		default:
			return FALSE;
		}
	}

	if (bind & PIPE_BIND_DEPTH_STENCIL) {
		switch (format) {
		case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
		case PIPE_FORMAT_X8Z24_UNORM:
		case PIPE_FORMAT_Z16_UNORM:
			break;
		default:
			return FALSE;
		}
	}

	if (bind & PIPE_BIND_SAMPLER_VIEW) {
		struct nvfx_texture_format* tf = &nvfx_texture_formats[format];
		if(util_format_is_s3tc(format) && !util_format_s3tc_enabled)
			return FALSE;
		if(format == PIPE_FORMAT_R16G16B16A16_FLOAT && !screen->advertise_fp16)
			return FALSE;
		if(format == PIPE_FORMAT_R32G32B32A32_FLOAT && !screen->advertise_fp32)
			return FALSE;
		if(screen->use_nv4x)
		{
			if(tf->fmt[4] < 0)
				return FALSE;
		}
		else
		{
			if(tf->fmt[0] < 0)
				return FALSE;
		}
	}

	// note that we do actually support everything through translate
	if (bind & PIPE_BIND_VERTEX_BUFFER) {
		unsigned type = nvfx_vertex_formats[format];
		if(!type)
			return FALSE;
	}

	if (bind & PIPE_BIND_INDEX_BUFFER) {
		// 8-bit indices supported, but not in hardware index buffer
		if(format != PIPE_FORMAT_R16_USCALED && format != PIPE_FORMAT_R32_USCALED)
			return FALSE;
	}

	if(bind & PIPE_BIND_STREAM_OUTPUT)
		return FALSE;

	return TRUE;
}

static void
nvfx_screen_destroy(struct pipe_screen *pscreen)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);

	nouveau_resource_destroy(&screen->vp_exec_heap);
	nouveau_resource_destroy(&screen->vp_data_heap);
	nouveau_resource_destroy(&screen->query_heap);
	nouveau_notifier_free(&screen->query);
	nouveau_notifier_free(&screen->sync);
	nouveau_grobj_free(&screen->eng3d);
	nvfx_screen_surface_takedown(pscreen);
	nouveau_bo_ref(NULL, &screen->fence);

	nouveau_screen_fini(&screen->base);

	FREE(pscreen);
}

static void nv30_screen_init(struct nvfx_screen *screen)
{
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	int i;

	/* TODO: perhaps we should do some of this on nv40 too? */
	for (i=1; i<8; i++) {
		BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_CLIP_HORIZ(i), 1);
		OUT_RING(chan, 0);
		BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_CLIP_VERT(i), 1);
		OUT_RING(chan, 0);
	}

	BEGIN_RING(chan, eng3d, 0x220, 1);
	OUT_RING(chan, 1);

	BEGIN_RING(chan, eng3d, 0x03b0, 1);
	OUT_RING(chan, 0x00100000);
	BEGIN_RING(chan, eng3d, 0x1454, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, eng3d, 0x1d80, 1);
	OUT_RING(chan, 3);
	BEGIN_RING(chan, eng3d, 0x1450, 1);
	OUT_RING(chan, 0x00030004);

	/* NEW */
	BEGIN_RING(chan, eng3d, 0x1e98, 1);
	OUT_RING(chan, 0);
	BEGIN_RING(chan, eng3d, 0x17e0, 3);
	OUT_RING(chan, fui(0.0));
	OUT_RING(chan, fui(0.0));
	OUT_RING(chan, fui(1.0));
	BEGIN_RING(chan, eng3d, 0x1f80, 16);
	for (i=0; i<16; i++) {
		OUT_RING(chan, (i==8) ? 0x0000ffff : 0);
	}

	BEGIN_RING(chan, eng3d, 0x120, 3);
	OUT_RING(chan, 0);
	OUT_RING(chan, 1);
	OUT_RING(chan, 2);

	BEGIN_RING(chan, eng3d, 0x1d88, 1);
	OUT_RING(chan, 0x00001200);

	BEGIN_RING(chan, eng3d, NV30_3D_RC_ENABLE, 1);
	OUT_RING(chan, 0);

	BEGIN_RING(chan, eng3d, NV30_3D_DEPTH_RANGE_NEAR, 2);
	OUT_RING(chan, fui(0.0));
	OUT_RING(chan, fui(1.0));

	BEGIN_RING(chan, eng3d, NV30_3D_MULTISAMPLE_CONTROL, 1);
	OUT_RING(chan, 0xffff0000);

	/* enables use of vp rather than fixed-function somehow */
	BEGIN_RING(chan, eng3d, 0x1e94, 1);
	OUT_RING(chan, 0x13);
}

static void nv40_screen_init(struct nvfx_screen *screen)
{
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;

	BEGIN_RING(chan, eng3d, NV40_3D_DMA_COLOR2, 2);
	OUT_RING(chan, screen->base.channel->vram->handle);
	OUT_RING(chan, screen->base.channel->vram->handle);

	BEGIN_RING(chan, eng3d, 0x1450, 1);
	OUT_RING(chan, 0x00000004);

	BEGIN_RING(chan, eng3d, 0x1ea4, 3);
	OUT_RING(chan, 0x00000010);
	OUT_RING(chan, 0x01000100);
	OUT_RING(chan, 0xff800006);

	/* vtxprog output routing */
	BEGIN_RING(chan, eng3d, 0x1fc4, 1);
	OUT_RING(chan, 0x06144321);
	BEGIN_RING(chan, eng3d, 0x1fc8, 2);
	OUT_RING(chan, 0xedcba987);
	OUT_RING(chan, 0x0000006f);
	BEGIN_RING(chan, eng3d, 0x1fd0, 1);
	OUT_RING(chan, 0x00171615);
	BEGIN_RING(chan, eng3d, 0x1fd4, 1);
	OUT_RING(chan, 0x001b1a19);

	BEGIN_RING(chan, eng3d, 0x1ef8, 1);
	OUT_RING(chan, 0x0020ffff);
	BEGIN_RING(chan, eng3d, 0x1d64, 1);
	OUT_RING(chan, 0x01d300d4);
	BEGIN_RING(chan, eng3d, 0x1e94, 1);
	OUT_RING(chan, 0x00000001);

	BEGIN_RING(chan, eng3d, NV40_3D_MIPMAP_ROUNDING, 1);
	OUT_RING(chan, NV40_3D_MIPMAP_ROUNDING_MODE_DOWN);
}

static unsigned
nvfx_screen_get_vertex_buffer_flags(struct nvfx_screen* screen)
{
	int vram_hack_default = 0;
	int vram_hack;
	// TODO: this is a bit of a guess; also add other cards that may need this hack.
	// It may also depend on the specific card or the AGP/PCIe chipset.
	if(screen->base.device->chipset == 0x47 /* G70 */
		|| screen->base.device->chipset == 0x49 /* G71 */
		|| screen->base.device->chipset == 0x46 /* G72 */
		)
		vram_hack_default = 1;
	vram_hack = debug_get_bool_option("NOUVEAU_VTXIDX_IN_VRAM", vram_hack_default);

	return vram_hack ? NOUVEAU_BO_VRAM : NOUVEAU_BO_GART;
}

static void nvfx_channel_flush_notify(struct nouveau_channel* chan)
{
	struct nvfx_screen* screen = chan->user_private;
	struct nvfx_context* nvfx = screen->cur_ctx;
	if(nvfx)
		nvfx->relocs_needed = NVFX_RELOCATE_ALL;
}

struct pipe_screen *
nvfx_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
	static const unsigned query_sizes[] = {(4096 - 4 * 32) / 32, 3 * 1024 / 32, 2 * 1024 / 32, 1024 / 32};
	struct nvfx_screen *screen = CALLOC_STRUCT(nvfx_screen);
	struct nouveau_channel *chan;
	struct pipe_screen *pscreen;
	unsigned eng3d_class = 0;
	int ret, i;

	if (!screen)
		return NULL;

	pscreen = &screen->base.base;

	ret = nouveau_screen_init(&screen->base, dev);
	if (ret) {
		nvfx_screen_destroy(pscreen);
		return NULL;
	}
	chan = screen->base.channel;
	screen->cur_ctx = NULL;
	chan->user_private = screen;
	chan->flush_notify = nvfx_channel_flush_notify;

	pscreen->winsys = ws;
	pscreen->destroy = nvfx_screen_destroy;
	pscreen->get_param = nvfx_screen_get_param;
	pscreen->get_shader_param = nvfx_screen_get_shader_param;
	pscreen->get_paramf = nvfx_screen_get_paramf;
	pscreen->is_format_supported = nvfx_screen_is_format_supported;
	pscreen->context_create = nvfx_create;

	ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 4096, &screen->fence);
	if (ret) {
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	switch (dev->chipset & 0xf0) {
	case 0x30:
		if (NV30_3D_CHIPSET_3X_MASK & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV30_3D;
		else if (NV34_3D_CHIPSET_3X_MASK & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV34_3D;
		else if (NV35_3D_CHIPSET_3X_MASK & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV35_3D;
		break;
	case 0x40:
		if (NV4X_GRCLASS4097_CHIPSETS & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV40_3D;
		else if (NV4X_GRCLASS4497_CHIPSETS & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV44_3D;
		screen->is_nv4x = ~0;
		break;
	case 0x60:
		if (NV6X_GRCLASS4497_CHIPSETS & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV44_3D;
		screen->is_nv4x = ~0;
		break;
	}

	if (!eng3d_class) {
		NOUVEAU_ERR("Unknown nv3x/nv4x chipset: nv%02x\n", dev->chipset);
		return NULL;
	}

	screen->advertise_npot = !!screen->is_nv4x;
	screen->advertise_blend_equation_separate = !!screen->is_nv4x;
	screen->use_nv4x = screen->is_nv4x;

	if(screen->is_nv4x) {
		if(debug_get_bool_option("NVFX_SIMULATE_NV30", FALSE))
			screen->use_nv4x = 0;
		if(!debug_get_bool_option("NVFX_NPOT", TRUE))
			screen->advertise_npot = 0;
		if(!debug_get_bool_option("NVFX_BLEND_EQ_SEP", TRUE))
			screen->advertise_blend_equation_separate = 0;
	}

	screen->force_swtnl = debug_get_bool_option("NVFX_SWTNL", FALSE);
	screen->trace_draw = debug_get_bool_option("NVFX_TRACE_DRAW", FALSE);

	screen->buffer_allocation_cost = debug_get_num_option("NVFX_BUFFER_ALLOCATION_COST", 16384);
	screen->inline_cost_per_hardware_cost = atof(debug_get_option("NVFX_INLINE_COST_PER_HARDWARE_COST", "1.0"));
	screen->static_reuse_threshold = atof(debug_get_option("NVFX_STATIC_REUSE_THRESHOLD", "2.0"));

	/* We don't advertise these by default because filtering and blending doesn't work as
	 * it should, due to several restrictions.
	 * The only exception is fp16 on nv40.
	 */
	screen->advertise_fp16 = debug_get_bool_option("NVFX_FP16", !!screen->use_nv4x);
	screen->advertise_fp32 = debug_get_bool_option("NVFX_FP32", 0);

	screen->vertex_buffer_reloc_flags = nvfx_screen_get_vertex_buffer_flags(screen);

	/* surely both nv3x and nv44 support index buffers too: find out how and test that */
	if(eng3d_class == NV40_3D)
		screen->index_buffer_reloc_flags = screen->vertex_buffer_reloc_flags;

	if(!screen->force_swtnl && screen->vertex_buffer_reloc_flags == screen->index_buffer_reloc_flags)
		screen->base.vertex_buffer_flags = screen->base.index_buffer_flags = screen->vertex_buffer_reloc_flags;

	nvfx_screen_init_resource_functions(pscreen);

	ret = nouveau_grobj_alloc(chan, 0xbeef3097, eng3d_class, &screen->eng3d);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	/* 2D engine setup */
	nvfx_screen_surface_init(pscreen);

	/* Notifier for sync purposes */
	ret = nouveau_notifier_alloc(chan, 0xbeef0301, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	/* Query objects */
	for(i = 0; i < sizeof(query_sizes) / sizeof(query_sizes[0]); ++i)
	{
		ret = nouveau_notifier_alloc(chan, 0xbeef0302, query_sizes[i], &screen->query);
		if(!ret)
			break;
	}

	if (ret) {
		NOUVEAU_ERR("Error initialising query objects: %d\n", ret);
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	ret = nouveau_resource_init(&screen->query_heap, 0, query_sizes[i]);
	if (ret) {
		NOUVEAU_ERR("Error initialising query object heap: %d\n", ret);
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	LIST_INITHEAD(&screen->query_list);

	/* Vtxprog resources */
	if (nouveau_resource_init(&screen->vp_exec_heap, 0, screen->use_nv4x ? 512 : 256) ||
	    nouveau_resource_init(&screen->vp_data_heap, 0, screen->use_nv4x ? 468 : 256)) {
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	BIND_RING(chan, screen->eng3d, 7);

	/* Static eng3d initialisation */
	/* note that we just started using the channel, so we must have space in the pushbuffer */
	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_NOTIFY, 1);
	OUT_RING(chan, screen->sync->handle);
	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_TEXTURE0, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);
	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_COLOR1, 1);
	OUT_RING(chan, chan->vram->handle);
	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_COLOR0, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->vram->handle);
	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_VTXBUF0, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->gart->handle);

	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_FENCE, 2);
	OUT_RING(chan, 0);
	OUT_RING(chan, screen->query->handle);

	BEGIN_RING(chan, screen->eng3d, NV30_3D_DMA_UNK1AC, 2);
	OUT_RING(chan, chan->vram->handle);
	OUT_RING(chan, chan->vram->handle);

	if(!screen->is_nv4x)
		nv30_screen_init(screen);
	else
		nv40_screen_init(screen);

	return pscreen;
}
