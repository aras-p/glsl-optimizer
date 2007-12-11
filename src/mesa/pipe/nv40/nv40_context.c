#include "pipe/draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv40_context.h"
#include "nv40_dma.h"

static boolean
nv40_is_format_supported(struct pipe_context *pipe, enum pipe_format format,
			 uint type)
{
	switch (type) {
	case PIPE_SURFACE:
	case PIPE_SCREEN_SURFACE:
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM: 
		case PIPE_FORMAT_Z24S8_UNORM:
		case PIPE_FORMAT_Z16_UNORM:
			return TRUE;
		default:
			break;
		}
		break;
	case PIPE_TEXTURE:
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_A1R5G5B5_UNORM:
		case PIPE_FORMAT_A4R4G4B4_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM: 
		case PIPE_FORMAT_U_L8:
		case PIPE_FORMAT_U_A8:
		case PIPE_FORMAT_U_I8:
		case PIPE_FORMAT_U_A8_L8:
		case PIPE_FORMAT_Z16_UNORM:
		case PIPE_FORMAT_Z24S8_UNORM:
			return TRUE;
		default:
			break;
		}
		break;
	default:
		assert(0);
	};

	return FALSE;
}

static const char *
nv40_get_name(struct pipe_context *pipe)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", nv40->chipset);
	return buffer;
}

static const char *
nv40_get_vendor(struct pipe_context *pipe)
{
	return "nouveau";
}

static int
nv40_get_param(struct pipe_context *pipe, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 16;
	case PIPE_CAP_NPOT_TEXTURES:
		return 1;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_S3TC:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 1;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 4;
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
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static float
nv40_get_paramf(struct pipe_context *pipe, int param)
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
nv40_flush(struct pipe_context *pipe, unsigned flags)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;
	
	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(curie, 0x1fd8, 1);
		OUT_RING  (2);
		BEGIN_RING(curie, 0x1fd8, 1);
		OUT_RING  (1);
	}

	if (flags & PIPE_FLUSH_WAIT) {
		nvws->notifier_reset(nv40->sync, 0);
		BEGIN_RING(curie, 0x104, 1);
		OUT_RING  (0);
		BEGIN_RING(curie, 0x100, 1);
		OUT_RING  (0);
	}

	FIRE_RING();

	if (flags & PIPE_FLUSH_WAIT)
		nvws->notifier_wait(nv40->sync, 0, 0, 2000);
}

static void
nv40_destroy(struct pipe_context *pipe)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;

	draw_destroy(nv40->draw);
	free(nv40);
}

static boolean
nv40_init_hwctx(struct nv40_context *nv40, int curie_class)
{
	struct nouveau_winsys *nvws = nv40->nvws;
	int ret;

	if ((ret = nvws->notifier_alloc(nvws, nv40->num_query_objects,
					&nv40->query))) {
		NOUVEAU_ERR("Error creating query notifier objects: %d\n", ret);
		return FALSE;
	}

	if ((ret = nvws->grobj_alloc(nvws, curie_class,
				     &nv40->curie))) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	BEGIN_RING(curie, NV40TCL_DMA_NOTIFY, 1);
	OUT_RING  (nv40->sync->handle);
	BEGIN_RING(curie, NV40TCL_DMA_TEXTURE0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->gart->handle);
	BEGIN_RING(curie, NV40TCL_DMA_COLOR1, 1);
	OUT_RING  (nvws->channel->vram->handle);
	BEGIN_RING(curie, NV40TCL_DMA_COLOR0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->vram->handle);
	BEGIN_RING(curie, NV40TCL_DMA_VTXBUF0, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->gart->handle);
	BEGIN_RING(curie, NV40TCL_DMA_FENCE, 2);
	OUT_RING  (0);
	OUT_RING  (nv40->query->handle);
	BEGIN_RING(curie, NV40TCL_DMA_UNK01AC, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->vram->handle);
	BEGIN_RING(curie, NV40TCL_DMA_COLOR2, 2);
	OUT_RING  (nvws->channel->vram->handle);
	OUT_RING  (nvws->channel->vram->handle);

	BEGIN_RING(curie, 0x1ea4, 3);
	OUT_RING  (0x00000010);
	OUT_RING  (0x01000100);
	OUT_RING  (0xff800006);

	/* vtxprog output routing */
	BEGIN_RING(curie, 0x1fc4, 1);
	OUT_RING  (0x06144321);
	BEGIN_RING(curie, 0x1fc8, 2);
	OUT_RING  (0xedcba987);
	OUT_RING  (0x00000021);
	BEGIN_RING(curie, 0x1fd0, 1);
	OUT_RING  (0x00171615);
	BEGIN_RING(curie, 0x1fd4, 1);
	OUT_RING  (0x001b1a19);

	BEGIN_RING(curie, 0x1ef8, 1);
	OUT_RING  (0x0020ffff);
	BEGIN_RING(curie, 0x1d64, 1);
	OUT_RING  (0x00d30000);
	BEGIN_RING(curie, 0x1e94, 1);
	OUT_RING  (0x00000001);

	FIRE_RING ();
	return TRUE;
}

#define GRCLASS4097_CHIPSETS 0x00000baf
#define GRCLASS4497_CHIPSETS 0x00005450
struct pipe_context *
nv40_create(struct pipe_winsys *pipe_winsys, struct nouveau_winsys *nvws,
	    unsigned chipset)
{
	struct nv40_context *nv40;
	int curie_class, ret;

	if ((chipset & 0xf0) != 0x40) {
		NOUVEAU_ERR("Not a NV4X chipset\n");
		return NULL;
	}

	if (GRCLASS4097_CHIPSETS & (1 << (chipset & 0x0f))) {
		curie_class = 0x4097;
	} else
	if (GRCLASS4497_CHIPSETS & (1 << (chipset & 0x0f))) {
		curie_class = 0x4497;
	} else {
		NOUVEAU_ERR("Unknown NV4X chipset: NV%02x\n", chipset);
		return NULL;
	}

	nv40 = CALLOC_STRUCT(nv40_context);
	if (!nv40)
		return NULL;
	nv40->chipset = chipset;
	nv40->nvws = nvws;

	if ((ret = nvws->notifier_alloc(nvws, 1, &nv40->sync))) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		free(nv40);
		return NULL;
	}

	nv40->num_query_objects = 32;
	nv40->query_objects = calloc(nv40->num_query_objects,
				     sizeof(struct pipe_query_object *));
	if (!nv40->query_objects) {
		free(nv40);
		return NULL;
	}

	if (nvws->res_init(&nv40->vertprog.exec_heap, 0, 512) ||
	    nvws->res_init(&nv40->vertprog.data_heap, 0, 256)) {
		nvws->res_free(&nv40->vertprog.exec_heap);
		nvws->res_free(&nv40->vertprog.data_heap);
		free(nv40);
		return NULL;
	}

	if (!nv40_init_hwctx(nv40, curie_class)) {
		free(nv40);
		return NULL;
	}

	nv40->pipe.winsys = pipe_winsys;

	nv40->pipe.destroy = nv40_destroy;
	nv40->pipe.is_format_supported = nv40_is_format_supported;
	nv40->pipe.get_name = nv40_get_name;
	nv40->pipe.get_vendor = nv40_get_vendor;
	nv40->pipe.get_param = nv40_get_param;
	nv40->pipe.get_paramf = nv40_get_paramf;

	nv40->pipe.draw_arrays = nv40_draw_arrays;
	nv40->pipe.draw_elements = nv40_draw_elements;
	nv40->pipe.clear = nv40_clear;

	nv40->pipe.flush = nv40_flush;

	nv40_init_query_functions(nv40);
	nv40_init_surface_functions(nv40);
	nv40_init_state_functions(nv40);
	nv40_init_miptree_functions(nv40);

	nv40->draw = draw_create();
	assert(nv40->draw);
	draw_set_rasterize_stage(nv40->draw, nv40_draw_render_stage(nv40));

	return &nv40->pipe;
}

		
