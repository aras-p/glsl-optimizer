#include "pipe/p_screen.h"
#include "util/u_simple_screen.h"

#include "nv40_context.h"
#include "nv40_screen.h"

#define NV4X_GRCLASS4097_CHIPSETS 0x00000baf
#define NV4X_GRCLASS4497_CHIPSETS 0x00005450
#define NV6X_GRCLASS4497_CHIPSETS 0x00000088

static const char *
nv40_screen_get_name(struct pipe_screen *pscreen)
{
	struct nv40_screen *screen = nv40_screen(pscreen);
	struct nouveau_device *dev = screen->nvws->channel->device;
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", dev->chipset);
	return buffer;
}

static const char *
nv40_screen_get_vendor(struct pipe_screen *pscreen)
{
	return "nouveau";
}

static int
nv40_screen_get_param(struct pipe_screen *pscreen, int param)
{
	struct nv40_screen *screen = nv40_screen(pscreen);

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
		return 1;
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
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
	case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
		return 1;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 0; /* We have 4 - but unsupported currently */
	case NOUVEAU_CAP_HW_VTXBUF:
		return 1;
	case NOUVEAU_CAP_HW_IDXBUF:
		if (screen->curie->grclass == NV40TCL)
			return 1;
		return 0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static float
nv40_screen_get_paramf(struct pipe_screen *pscreen, int param)
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
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static boolean
nv40_screen_surface_format_supported(struct pipe_screen *pscreen,
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
		case PIPE_FORMAT_R16_SNORM:
		case PIPE_FORMAT_L8_UNORM:
		case PIPE_FORMAT_A8_UNORM:
		case PIPE_FORMAT_I8_UNORM:
		case PIPE_FORMAT_A8L8_UNORM:
		case PIPE_FORMAT_Z16_UNORM:
		case PIPE_FORMAT_Z24S8_UNORM:
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

static struct pipe_buffer *
nv40_surface_buffer(struct pipe_surface *surf)
{
	struct nv40_miptree *mt = (struct nv40_miptree *)surf->texture;

	return mt->buffer;
}

static void
nv40_screen_destroy(struct pipe_screen *pscreen)
{
	struct nv40_screen *screen = nv40_screen(pscreen);
	struct nouveau_winsys *nvws = screen->nvws;

	nvws->res_free(&screen->vp_exec_heap);
	nvws->res_free(&screen->vp_data_heap);
	nvws->res_free(&screen->query_heap);
	nvws->notifier_free(&screen->query);
	nvws->notifier_free(&screen->sync);
	nvws->grobj_free(&screen->curie);

	FREE(pscreen);
}

struct pipe_screen *
nv40_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *nvws)
{
	struct nv40_screen *screen = CALLOC_STRUCT(nv40_screen);
	struct nouveau_stateobj *so;
	unsigned curie_class = 0;
	unsigned chipset = nvws->channel->device->chipset;
	int ret;

	if (!screen)
		return NULL;
	screen->nvws = nvws;

	/* 2D engine setup */
	screen->eng2d = nv04_surface_2d_init(nvws);
	screen->eng2d->buf = nv40_surface_buffer;

	/* 3D object */
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
	}

	if (!curie_class) {
		NOUVEAU_ERR("Unknown nv4x chipset: nv%02x\n", chipset);
		return NULL;
	}

	ret = nvws->grobj_alloc(nvws, curie_class, &screen->curie);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	/* Notifier for sync purposes */
	ret = nvws->notifier_alloc(nvws, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv40_screen_destroy(&screen->pipe);
		return NULL;
	}

	/* Query objects */
	ret = nvws->notifier_alloc(nvws, 32, &screen->query);
	if (ret) {
		NOUVEAU_ERR("Error initialising query objects: %d\n", ret);
		nv40_screen_destroy(&screen->pipe);
		return NULL;
	}

	ret = nvws->res_init(&screen->query_heap, 0, 32);
	if (ret) {
		NOUVEAU_ERR("Error initialising query object heap: %d\n", ret);
		nv40_screen_destroy(&screen->pipe);
		return NULL;
	}

	/* Vtxprog resources */
	if (nvws->res_init(&screen->vp_exec_heap, 0, 512) ||
	    nvws->res_init(&screen->vp_data_heap, 0, 256)) {
		nv40_screen_destroy(&screen->pipe);
		return NULL;
	}

	/* Static curie initialisation */
	so = so_new(128, 0);
	so_method(so, screen->curie, NV40TCL_DMA_NOTIFY, 1);
	so_data  (so, screen->sync->handle);
	so_method(so, screen->curie, NV40TCL_DMA_TEXTURE0, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->gart->handle);
	so_method(so, screen->curie, NV40TCL_DMA_COLOR1, 1);
	so_data  (so, nvws->channel->vram->handle);
	so_method(so, screen->curie, NV40TCL_DMA_COLOR0, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->vram->handle);
	so_method(so, screen->curie, NV40TCL_DMA_VTXBUF0, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->gart->handle);
	so_method(so, screen->curie, NV40TCL_DMA_FENCE, 2);
	so_data  (so, 0);
	so_data  (so, screen->query->handle);
	so_method(so, screen->curie, NV40TCL_DMA_UNK01AC, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->vram->handle);
	so_method(so, screen->curie, NV40TCL_DMA_COLOR2, 2);
	so_data  (so, nvws->channel->vram->handle);
	so_data  (so, nvws->channel->vram->handle);

	so_method(so, screen->curie, 0x1ea4, 3);
	so_data  (so, 0x00000010);
	so_data  (so, 0x01000100);
	so_data  (so, 0xff800006);

	/* vtxprog output routing */
	so_method(so, screen->curie, 0x1fc4, 1);
	so_data  (so, 0x06144321);
	so_method(so, screen->curie, 0x1fc8, 2);
	so_data  (so, 0xedcba987);
	so_data  (so, 0x00000021);
	so_method(so, screen->curie, 0x1fd0, 1);
	so_data  (so, 0x00171615);
	so_method(so, screen->curie, 0x1fd4, 1);
	so_data  (so, 0x001b1a19);

	so_method(so, screen->curie, 0x1ef8, 1);
	so_data  (so, 0x0020ffff);
	so_method(so, screen->curie, 0x1d64, 1);
	so_data  (so, 0x00d30000);
	so_method(so, screen->curie, 0x1e94, 1);
	so_data  (so, 0x00000001);

	so_emit(nvws, so);
	so_ref(NULL, &so);
	nvws->push_flush(nvws, 0, NULL);

	screen->pipe.winsys = ws;
	screen->pipe.destroy = nv40_screen_destroy;

	screen->pipe.get_name = nv40_screen_get_name;
	screen->pipe.get_vendor = nv40_screen_get_vendor;
	screen->pipe.get_param = nv40_screen_get_param;
	screen->pipe.get_paramf = nv40_screen_get_paramf;

	screen->pipe.is_format_supported = nv40_screen_surface_format_supported;

	nv40_screen_init_miptree_functions(&screen->pipe);
	nv40_screen_init_transfer_functions(&screen->pipe);
	u_simple_screen_init(&screen->pipe);

	return &screen->pipe;
}

