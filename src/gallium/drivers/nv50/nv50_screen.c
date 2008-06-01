#include "pipe/p_screen.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_screen.h"

#include "nouveau/nouveau_stateobj.h"

#define NV5X_GRCLASS5097_CHIPSETS 0x00000001
#define NV8X_GRCLASS8297_CHIPSETS 0x00000010
#define NV9X_GRCLASS8297_CHIPSETS 0x00000004

static boolean
nv50_screen_is_format_supported(struct pipe_screen *pscreen,
				enum pipe_format format, uint type)
{
	switch (type) {
	case PIPE_SURFACE:
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
		case PIPE_FORMAT_I8_UNORM:
			return TRUE;
		default:
			break;
		}
		break;
	default:
		assert(0);
	}

	return FALSE;
}

static const char *
nv50_screen_get_name(struct pipe_screen *pscreen)
{
	struct nv50_screen *screen = nv50_screen(pscreen);
	struct nouveau_device *dev = screen->nvws->channel->device;
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", dev->chipset);
	return buffer;
}

static const char *
nv50_screen_get_vendor(struct pipe_screen *pscreen)
{
	return "nouveau";
}

static int
nv50_screen_get_param(struct pipe_screen *pscreen, int param)
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
	case NOUVEAU_CAP_HW_VTXBUF:	
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

static void *
nv50_surface_map(struct pipe_screen *screen, struct pipe_surface *surface,
		 unsigned flags )
{
	struct pipe_winsys *ws = screen->winsys;
	void *map;

	map = ws->buffer_map(ws, surface->buffer, flags);
	if (!map)
		return NULL;

	return map + surface->offset;
}

static void
nv50_surface_unmap(struct pipe_screen *screen, struct pipe_surface *surface)
{
	struct pipe_winsys *ws = screen->winsys;

	ws->buffer_unmap(ws, surface->buffer);
}

static void
nv50_screen_destroy(struct pipe_screen *pscreen)
{
	FREE(pscreen);
}

struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *nvws)
{
	struct nv50_screen *screen = CALLOC_STRUCT(nv50_screen);
	struct nouveau_stateobj *so;
	unsigned tesla_class = 0, ret;
	unsigned chipset = nvws->channel->device->chipset;
	int i;

	if (!screen)
		return NULL;
	screen->nvws = nvws;

	/* 3D object */
	if ((chipset & 0xf0) != 0x50 && (chipset & 0xf0) != 0x80) {
		NOUVEAU_ERR("Not a G8x chipset\n");
		nv50_screen_destroy(&screen->pipe);
		return NULL;
	}

	switch (chipset & 0xf0) {
	case 0x50:
		if (NV5X_GRCLASS5097_CHIPSETS & (1 << (chipset & 0x0f)))
			tesla_class = 0x5097;
		break;
	case 0x80:
		if (NV8X_GRCLASS8297_CHIPSETS & (1 << (chipset & 0x0f)))
			tesla_class = 0x8297;
		break;
	case 0x90:
		if (NV9X_GRCLASS8297_CHIPSETS & (1 << (chipset & 0x0f)))
			tesla_class = 0x8297;
		break;
	default:
		break;
	}

	if (tesla_class == 0) {
		NOUVEAU_ERR("Unknown G8x chipset: NV%02x\n", chipset);
		nv50_screen_destroy(&screen->pipe);
		return NULL;
	}

	ret = nvws->grobj_alloc(nvws, tesla_class, &screen->tesla);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		nv50_screen_destroy(&screen->pipe);
		return NULL;
	}

	/* Sync notifier */
	ret = nvws->notifier_alloc(nvws, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv50_screen_destroy(&screen->pipe);
		return NULL;
	}

	/* Static constant buffer */
	screen->constbuf = ws->buffer_create(ws, 0, 0, 256 * 4 * 4);
	if (nvws->res_init(&screen->vp_data_heap, 0, 256)) {
		NOUVEAU_ERR("Error initialising constant buffer\n");
		nv50_screen_destroy(&screen->pipe);
		return NULL;
	}

	/* Static tesla init */
	so = so_new(256, 20);

	so_method(so, screen->tesla, 0x1558, 1);
	so_data  (so, 1);
	so_method(so, screen->tesla, NV50TCL_DMA_NOTIFY, 1);
	so_data  (so, screen->sync->handle);
	so_method(so, screen->tesla, NV50TCL_DMA_IN_MEMORY0(0),
		  NV50TCL_DMA_IN_MEMORY0__SIZE);
	for (i = 0; i < NV50TCL_DMA_IN_MEMORY0__SIZE; i++)
		so_data(so, nvws->channel->vram->handle);
	so_method(so, screen->tesla, NV50TCL_DMA_IN_MEMORY1(0),
		  NV50TCL_DMA_IN_MEMORY1__SIZE);
	for (i = 0; i < NV50TCL_DMA_IN_MEMORY1__SIZE; i++)
		so_data(so, nvws->channel->vram->handle);
	so_method(so, screen->tesla, 0x121c, 1);
	so_data  (so, 1);

	so_method(so, screen->tesla, 0x13bc, 1);
	so_data  (so, 0x54);
	so_method(so, screen->tesla, 0x13ac, 1);
	so_data  (so, 1);
	so_method(so, screen->tesla, 0x16bc, 2);
	so_data  (so, 0x03020100);
	so_data  (so, 0x07060504);
	so_method(so, screen->tesla, 0x1988, 2);
	so_data  (so, 0x08040404);
	so_data  (so, 0x00000004);
	so_method(so, screen->tesla, 0x16ac, 2);
	so_data  (so, 8);
	so_data  (so, 4);
	so_method(so, screen->tesla, 0x16b8, 1);
	so_data  (so, 8);

	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00001000);
	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00014000);
	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00024000);
	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00034000);
	so_method(so, screen->tesla, 0x1280, 3);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, screen->constbuf, 0, NOUVEAU_BO_VRAM |
		  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, 0x00040100);

	for (i = 0; i < 16; i++) {
		so_method(so, screen->tesla, 0x1080 + (i * 8), 2);
		so_data  (so, 0x000000ff);
		so_data  (so, 0xffffffff);
	}

	so_emit(nvws, so);
	so_ref(NULL, &so);
	nvws->push_flush(nvws, 0, NULL);

	screen->pipe.winsys = ws;

	screen->pipe.destroy = nv50_screen_destroy;

	screen->pipe.get_name = nv50_screen_get_name;
	screen->pipe.get_vendor = nv50_screen_get_vendor;
	screen->pipe.get_param = nv50_screen_get_param;
	screen->pipe.get_paramf = nv50_screen_get_paramf;

	screen->pipe.is_format_supported = nv50_screen_is_format_supported;

	screen->pipe.surface_map = nv50_surface_map;
	screen->pipe.surface_unmap = nv50_surface_unmap;

	nv50_screen_init_miptree_functions(&screen->pipe);

	return &screen->pipe;
}

