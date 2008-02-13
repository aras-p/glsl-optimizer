#include "pipe/draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv40_context.h"

#define NV4X_GRCLASS4097_CHIPSETS 0x00000baf
#define NV4X_GRCLASS4497_CHIPSETS 0x00005450
#define NV6X_GRCLASS4497_CHIPSETS 0x00000088

static const char *
nv40_get_name(struct pipe_context *pipe)
{
	struct nv40_context *nv40 = nv40_context(pipe);
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
		return 16.0;
	case PIPE_CAP_BITMAP_TEXCOORD_BIAS:
		return 0.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static void
nv40_flush(struct pipe_context *pipe, unsigned flags)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nouveau_winsys *nvws = nv40->nvws;
	
	if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(curie, 0x1fd8, 1);
		OUT_RING  (2);
		BEGIN_RING(curie, 0x1fd8, 1);
		OUT_RING  (1);
	}

	if (flags & PIPE_FLUSH_WAIT) {
		nvws->notifier_reset(nv40->hw->sync, 0);
		BEGIN_RING(curie, 0x104, 1);
		OUT_RING  (0);
		BEGIN_RING(curie, 0x100, 1);
		OUT_RING  (0);
	}

	FIRE_RING();

	if (flags & PIPE_FLUSH_WAIT)
		nvws->notifier_wait(nv40->hw->sync, 0, 0, 2000);
}

static void
nv40_channel_takedown(struct nv40_channel_context *cnv40)
{
	struct nouveau_winsys *nvws = cnv40->nvws;

	nvws->res_free(&cnv40->vp_exec_heap);
	nvws->res_free(&cnv40->vp_data_heap);
	nvws->res_free(&cnv40->query_heap);
	nvws->notifier_free(&cnv40->query);
	nvws->notifier_free(&cnv40->sync);
	nvws->grobj_free(&cnv40->curie);
	free(cnv40);
}

static struct nv40_channel_context *
nv40_channel_init(struct pipe_winsys *ws, struct nouveau_winsys *nvws,
		  unsigned chipset)
{
	struct nv40_channel_context *cnv40 = NULL;
	struct nouveau_stateobj *so;
	unsigned curie_class = 0;
	int ret;

	switch (chipset & 0xf0) {
	case 0x40:
		if (NV4X_GRCLASS4097_CHIPSETS & (1 << (chipset & 0x0f)))
			curie_class = NV40TCL;
		else
		if (NV4X_GRCLASS4497_CHIPSETS & (1 << (chipset & 0x0f)))
			curie_class = NV44TCL;
		break;
	case 0x60:
		if (NV6X_GRCLASS4497_CHIPSETS & (1 << (chipset & 0x0f)))
			curie_class = NV44TCL;
		break;
	default:
		break;
	}

	if (!curie_class) {
		NOUVEAU_ERR("Unknown nv4x chipset: nv%02x\n", chipset);
		return NULL;
	}

	cnv40 = calloc(1, sizeof(struct nv40_channel_context));
	if (!cnv40)
		return NULL;
	cnv40->chipset = chipset;
	cnv40->nvws = nvws;

	/* Notifier for sync purposes */
	ret = nvws->notifier_alloc(nvws, 1, &cnv40->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv40_channel_takedown(cnv40);
		return NULL;
	}

	/* Query objects */
	ret = nvws->notifier_alloc(nvws, 32, &cnv40->query);
	if (ret) {
		NOUVEAU_ERR("Error initialising query objects: %d\n", ret);
		nv40_channel_takedown(cnv40);
		return NULL;
	}

	ret = nvws->res_init(&cnv40->query_heap, 0, 32);
	if (ret) {
		NOUVEAU_ERR("Error initialising query object heap: %d\n", ret);
		nv40_channel_takedown(cnv40);
		return NULL;
	}

	/* Vtxprog resources */
	if (nvws->res_init(&cnv40->vp_exec_heap, 0, 512) ||
	    nvws->res_init(&cnv40->vp_data_heap, 0, 256)) {
		nv40_channel_takedown(cnv40);
		return NULL;
	}

	/* 3D object */
	ret = nvws->grobj_alloc(nvws, curie_class, &cnv40->curie);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	/* Static curie initialisation */
	so = so_new(128, 0);
	so_method(so, cnv40->curie, NV40TCL_DMA_NOTIFY, 1);
	so_data  (so, cnv40->sync->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_TEXTURE0, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->gart->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_COLOR1, 1);
	so_data  (so, nvws->channel->vram->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_COLOR0, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->vram->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_VTXBUF0, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->gart->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_FENCE, 2);
	so_data  (so, 0);
	so_data  (so, cnv40->query->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_UNK01AC, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->vram->handle);
	so_method(so, cnv40->curie, NV40TCL_DMA_COLOR2, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->vram->handle);

	so_method(so, cnv40->curie, 0x1ea4, 3);
	so_data  (so, 0x00000010);
	so_data  (so, 0x01000100);
	so_data  (so, 0xff800006);

	/* vtxprog output routing */
	so_method(so, cnv40->curie, 0x1fc4, 1);
	so_data  (so, 0x06144321);
	so_method(so, cnv40->curie, 0x1fc8, 2);
	so_data  (so, 0xedcba987);
	so_data  (so, 0x00000021);
	so_method(so, cnv40->curie, 0x1fd0, 1);
	so_data  (so, 0x00171615);
	so_method(so, cnv40->curie, 0x1fd4, 1);
	so_data  (so, 0x001b1a19);

	so_method(so, cnv40->curie, 0x1ef8, 1);
	so_data  (so, 0x0020ffff);
	so_method(so, cnv40->curie, 0x1d64, 1);
	so_data  (so, 0x00d30000);
	so_method(so, cnv40->curie, 0x1e94, 1);
	so_data  (so, 0x00000001);

	so_emit(nvws, so);
	so_ref(NULL, &so);
	nvws->push_flush(nvws->channel, 0);

	return cnv40;
}

static void
nv40_destroy(struct pipe_context *pipe)
{
	struct nv40_context *nv40 = nv40_context(pipe);

	if (nv40->draw)
		draw_destroy(nv40->draw);

	if (nv40->hw) {
		if (--nv40->hw->refcount == 0)
			nv40_channel_takedown(nv40->hw);
	}

	free(nv40);
}

struct pipe_context *
nv40_create(struct pipe_winsys *ws, struct nouveau_winsys *nvws,
	    unsigned chipset)
{
	struct nv40_context *nv40;

	nv40 = calloc(1, sizeof(struct nv40_context));
	if (!nv40)
		return NULL;

	nv40->hw = nv40_channel_init(ws, nvws, chipset);
	if (!nv40->hw) {
		nv40_destroy(&nv40->pipe);
		return NULL;
	}

	nv40->chipset = chipset;
	nv40->nvws = nvws;

	nv40->pipe.winsys = ws;
	nv40->pipe.destroy = nv40_destroy;
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
	
