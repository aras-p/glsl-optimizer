#include "pipe/p_screen.h"
#include "pipe/p_util.h"

#include "nv50_context.h"
#include "nv50_screen.h"

static boolean
nv50_screen_is_format_supported(struct pipe_screen *pscreen,
				enum pipe_format format, uint type)
{
	return FALSE;
}

static const char *
nv50_screen_get_name(struct pipe_screen *pscreen)
{
	struct nv50_screen *screen = nv50_screen(pscreen);
	static char buffer[128];

	snprintf(buffer, sizeof(buffer), "NV%02X", screen->chipset);
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
	FREE(pscreen);
}

struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, unsigned chipset)
{
	struct nv50_screen *screen = CALLOC_STRUCT(nv50_screen);

	if (!screen)
		return NULL;

	screen->chipset = chipset;

	screen->pipe.winsys = ws;

	screen->pipe.destroy = nv50_screen_destroy;

	screen->pipe.get_name = nv50_screen_get_name;
	screen->pipe.get_vendor = nv50_screen_get_vendor;
	screen->pipe.get_param = nv50_screen_get_param;
	screen->pipe.get_paramf = nv50_screen_get_paramf;

	screen->pipe.is_format_supported = nv50_screen_is_format_supported;

	nv50_screen_init_miptree_functions(&screen->pipe);

	return &screen->pipe;
}

