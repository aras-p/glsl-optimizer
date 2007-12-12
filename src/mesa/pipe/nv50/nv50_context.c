#include "pipe/draw/draw_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_dma.h"

static boolean
nv50_is_format_supported(struct pipe_context *pipe, enum pipe_format format,
			 uint type)
{
	return FALSE;
}

static const char *
nv50_get_name(struct pipe_context *pipe)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", nv50->chipset);
	return buffer;
}

static const char *
nv50_get_vendor(struct pipe_context *pipe)
{
	return "nouveau";
}

static int
nv50_get_param(struct pipe_context *pipe, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 32;
	case PIPE_CAP_NPOT_TEXTURES:
		return 0;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_S3TC:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 0;
	case PIPE_CAP_POINT_SPRITE:
		return 0;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 8;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 0;
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 0;
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
nv50_get_paramf(struct pipe_context *pipe, int param)
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
nv50_flush(struct pipe_context *pipe, unsigned flags)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->nvws;
	
	if (flags & PIPE_FLUSH_WAIT) {
		nvws->notifier_reset(nv50->sync, 0);
		BEGIN_RING(tesla, 0x104, 1);
		OUT_RING  (0);
		BEGIN_RING(tesla, 0x100, 1);
		OUT_RING  (0);
	}

	FIRE_RING();

	if (flags & PIPE_FLUSH_WAIT)
		nvws->notifier_wait(nv50->sync, 0, 0, 2000);
}

static void
nv50_destroy(struct pipe_context *pipe)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;

	draw_destroy(nv50->draw);
	free(nv50);
}

static boolean
nv50_init_hwctx(struct nv50_context *nv50, int tesla_class)
{
	struct nouveau_winsys *nvws = nv50->nvws;
	int ret;

	if ((ret = nvws->grobj_alloc(nvws, tesla_class, &nv50->tesla))) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	BEGIN_RING(tesla, NV50TCL_DMA_NOTIFY, 1);
	OUT_RING  (nv50->sync->handle);

	FIRE_RING ();
	return TRUE;
}

#define GRCLASS5097_CHIPSETS 0x00000000
#define GRCLASS8297_CHIPSETS 0x00000010
struct pipe_context *
nv50_create(struct pipe_winsys *pipe_winsys, struct nouveau_winsys *nvws,
	    unsigned chipset)
{
	struct nv50_context *nv50;
	int tesla_class, ret;

	if ((chipset & 0xf0) != 0x50 && (chipset & 0xf0) != 0x80) {
		NOUVEAU_ERR("Not a G8x chipset\n");
		return NULL;
	}

	if (GRCLASS5097_CHIPSETS & (1 << (chipset & 0x0f))) {
		tesla_class = 0x5097;
	} else
	if (GRCLASS8297_CHIPSETS & (1 << (chipset & 0x0f))) {
		tesla_class = 0x8297;
	} else {
		NOUVEAU_ERR("Unknown G8x chipset: NV%02x\n", chipset);
		return NULL;
	}

	nv50 = CALLOC_STRUCT(nv50_context);
	if (!nv50)
		return NULL;
	nv50->chipset = chipset;
	nv50->nvws = nvws;

	if ((ret = nvws->notifier_alloc(nvws, 1, &nv50->sync))) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		free(nv50);
		return NULL;
	}

	if (!nv50_init_hwctx(nv50, tesla_class)) {
		free(nv50);
		return NULL;
	}

	nv50->pipe.winsys = pipe_winsys;

	nv50->pipe.destroy = nv50_destroy;
	nv50->pipe.is_format_supported = nv50_is_format_supported;
	nv50->pipe.get_name = nv50_get_name;
	nv50->pipe.get_vendor = nv50_get_vendor;
	nv50->pipe.get_param = nv50_get_param;
	nv50->pipe.get_paramf = nv50_get_paramf;

	nv50->pipe.draw_arrays = nv50_draw_arrays;
	nv50->pipe.draw_elements = nv50_draw_elements;
	nv50->pipe.clear = nv50_clear;

	nv50->pipe.flush = nv50_flush;

	nv50_init_miptree_functions(nv50);
	nv50_init_surface_functions(nv50);
	nv50_init_state_functions(nv50);
	nv50_init_query_functions(nv50);

	nv50->draw = draw_create();
	assert(nv50->draw);
	draw_set_rasterize_stage(nv50->draw, nv50_draw_render_stage(nv50));

	return &nv50->pipe;
}

		
