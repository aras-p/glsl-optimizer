#include "pipe/p_screen.h"
#include "util/u_simple_screen.h"

#include "nv20_context.h"
#include "nv20_screen.h"

static const char *
nv20_screen_get_name(struct pipe_screen *screen)
{
	struct nv20_screen *nv20screen = nv20_screen(screen);
	struct nouveau_device *dev = nv20screen->nvws->channel->device;
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", dev->chipset);
	return buffer;
}

static const char *
nv20_screen_get_vendor(struct pipe_screen *screen)
{
	return "nouveau";
}

static int
nv20_screen_get_param(struct pipe_screen *screen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 2;
	case PIPE_CAP_NPOT_TEXTURES:
		return 0;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 0;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_S3TC:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 0;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 1;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 0;
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 0;
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		return 12;
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		return 0;
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 12;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 0;
	case NOUVEAU_CAP_HW_VTXBUF:
	case NOUVEAU_CAP_HW_IDXBUF:
		return 0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static float
nv20_screen_get_paramf(struct pipe_screen *screen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
		return 10.0;
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
		return 64.0;
	case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
		return 2.0;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return 4.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static boolean
nv20_screen_is_format_supported(struct pipe_screen *screen,
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
			return TRUE;
		default:
			break;
		}
	}

	return FALSE;
}

static void
nv20_screen_destroy(struct pipe_screen *pscreen)
{
	struct nv20_screen *screen = nv20_screen(pscreen);
	struct nouveau_winsys *nvws = screen->nvws;

	nvws->notifier_free(&screen->sync);
	nvws->grobj_free(&screen->kelvin);

	FREE(pscreen);
}

static struct pipe_buffer *
nv20_surface_buffer(struct pipe_surface *surf)
{
	struct nv20_miptree *mt = (struct nv20_miptree *)surf->texture;

	return mt->buffer;
}

struct pipe_screen *
nv20_screen_create(struct pipe_winsys *ws, struct nouveau_winsys *nvws)
{
	struct nv20_screen *screen = CALLOC_STRUCT(nv20_screen);
	unsigned kelvin_class = 0;
	unsigned chipset = nvws->channel->device->chipset;
	int ret;

	if (!screen)
		return NULL;
	screen->nvws = nvws;

	/* 2D engine setup */
	screen->eng2d = nv04_surface_2d_init(nvws);
	screen->eng2d->buf = nv20_surface_buffer;

	/* 3D object */
	if (chipset >= 0x25)
		kelvin_class = NV25TCL;
	else if (chipset >= 0x20)
		kelvin_class = NV20TCL;

	if (!kelvin_class || chipset >= 0x30) {
		NOUVEAU_ERR("Unknown nv2x chipset: nv%02x\n", chipset);
		return NULL;
	}

	ret = nvws->grobj_alloc(nvws, kelvin_class, &screen->kelvin);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	/* Notifier for sync purposes */
	ret = nvws->notifier_alloc(nvws, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv20_screen_destroy(&screen->pipe);
		return NULL;
	}

	screen->pipe.winsys = ws;
	screen->pipe.destroy = nv20_screen_destroy;

	screen->pipe.get_name = nv20_screen_get_name;
	screen->pipe.get_vendor = nv20_screen_get_vendor;
	screen->pipe.get_param = nv20_screen_get_param;
	screen->pipe.get_paramf = nv20_screen_get_paramf;

	screen->pipe.is_format_supported = nv20_screen_is_format_supported;

	nv20_screen_init_miptree_functions(&screen->pipe);
	nv20_screen_init_transfer_functions(&screen->pipe);
	u_simple_screen_init(&screen->pipe);

	return &screen->pipe;
}

