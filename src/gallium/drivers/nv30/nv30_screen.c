#include "pipe/p_screen.h"
#include "pipe/p_util.h"

#include "nv30_context.h"
#include "nv30_screen.h"

static const char *
nv30_screen_get_name(struct pipe_screen *screen)
{
	struct nv30_screen *nv30screen = nv30_screen(screen);
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", nv30screen->chipset);
	return buffer;
}

static const char *
nv30_screen_get_vendor(struct pipe_screen *screen)
{
	return "nouveau";
}

static int
nv30_screen_get_param(struct pipe_screen *screen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 16;
	case PIPE_CAP_NPOT_TEXTURES:
		return 0;
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
		return 2;
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
nv30_screen_get_paramf(struct pipe_screen *screen, int param)
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
	case PIPE_CAP_BITMAP_TEXCOORD_BIAS:
		return 0.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static boolean
nv30_screen_is_format_supported(struct pipe_screen *screen,
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

static void
nv30_screen_destroy(struct pipe_screen *screen)
{
	FREE(screen);
}

struct pipe_screen *
nv30_screen_create(struct pipe_winsys *winsys, struct nouveau_winsys *nvws,
		   unsigned chipset)
{
	struct nv30_screen *nv30screen = CALLOC_STRUCT(nv30_screen);

	if (!nv30screen)
		return NULL;

	nv30screen->chipset = chipset;
	nv30screen->nvws = nvws;

	nv30screen->screen.winsys = winsys;

	nv30screen->screen.destroy = nv30_screen_destroy;

	nv30screen->screen.get_name = nv30_screen_get_name;
	nv30screen->screen.get_vendor = nv30_screen_get_vendor;
	nv30screen->screen.get_param = nv30_screen_get_param;
	nv30screen->screen.get_paramf = nv30_screen_get_paramf;
	nv30screen->screen.is_format_supported = 
		nv30_screen_is_format_supported;

	nv30_screen_init_miptree_functions(&nv30screen->screen);

	return &nv30screen->screen;
}

