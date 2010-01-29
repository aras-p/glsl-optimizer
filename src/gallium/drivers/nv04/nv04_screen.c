#include "pipe/p_screen.h"
#include "pipe/p_inlines.h"

#include "nv04_context.h"
#include "nv04_screen.h"

static int
nv04_screen_get_param(struct pipe_screen *screen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		return 1;
	case PIPE_CAP_NPOT_TEXTURES:
		return 0;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 0;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 0;
	case PIPE_CAP_POINT_SPRITE:
		return 0;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return 1;
	case PIPE_CAP_OCCLUSION_QUERY:
		return 0;
	case PIPE_CAP_TEXTURE_SHADOW_MAP:
		return 0;
	case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
		return 10;
	case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
		return 0;
	case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
		return 0;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 0;
	case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
		return 0;
	case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
		return 1;
	case PIPE_CAP_TGSI_CONT_SUPPORTED:
		return 0;
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		return 0;
	case NOUVEAU_CAP_HW_VTXBUF:
	case NOUVEAU_CAP_HW_IDXBUF:
		return 0;
	case PIPE_CAP_INDEP_BLEND_ENABLE:
		return 0;
	case PIPE_CAP_INDEP_BLEND_FUNC:
		return 0;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
		return 1;
	case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
	case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
		return 0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0;
	}
}

static float
nv04_screen_get_paramf(struct pipe_screen *screen, int param)
{
	switch (param) {
	case PIPE_CAP_MAX_LINE_WIDTH:
	case PIPE_CAP_MAX_LINE_WIDTH_AA:
		return 0.0;
	case PIPE_CAP_MAX_POINT_WIDTH:
	case PIPE_CAP_MAX_POINT_WIDTH_AA:
		return 0.0;
	case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
		return 0.0;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return 0.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static boolean
nv04_screen_is_format_supported(struct pipe_screen *screen,
				enum pipe_format format,
				enum pipe_texture_target target,
				unsigned tex_usage, unsigned geom_flags)
{
	if (tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET) {
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM: 
			return TRUE;
		default:
			break;
		}
	} else
	if (tex_usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL) {
		switch (format) {
		case PIPE_FORMAT_Z16_UNORM:
			return TRUE;
		default:
			break;
		}
	} else {
		switch (format) {
		case PIPE_FORMAT_A8R8G8B8_UNORM:
		case PIPE_FORMAT_X8R8G8B8_UNORM:
		case PIPE_FORMAT_A1R5G5B5_UNORM:
		case PIPE_FORMAT_R5G6B5_UNORM: 
		case PIPE_FORMAT_L8_UNORM:
		case PIPE_FORMAT_A8_UNORM:
			return TRUE;
		default:
			break;
		}
	}

	return FALSE;
}

static void
nv04_screen_destroy(struct pipe_screen *pscreen)
{
	struct nv04_screen *screen = nv04_screen(pscreen);

	nouveau_notifier_free(&screen->sync);
	nouveau_grobj_free(&screen->fahrenheit);
	nv04_surface_2d_takedown(&screen->eng2d);

	nouveau_screen_fini(&screen->base);

	FREE(pscreen);
}

static struct pipe_buffer *
nv04_surface_buffer(struct pipe_surface *surf)
{
	struct nv04_miptree *mt = (struct nv04_miptree *)surf->texture;

	return mt->buffer;
}

struct pipe_screen *
nv04_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
	struct nv04_screen *screen = CALLOC_STRUCT(nv04_screen);
	struct nouveau_channel *chan;
	struct pipe_screen *pscreen;
	unsigned fahrenheit_class = 0, sub3d_class = 0;
	int ret;

	if (!screen)
		return NULL;
	pscreen = &screen->base.base;

	ret = nouveau_screen_init(&screen->base, dev);
	if (ret) {
		nv04_screen_destroy(pscreen);
		return NULL;
	}
	chan = screen->base.channel;

	pscreen->winsys = ws;
	pscreen->destroy = nv04_screen_destroy;
	pscreen->get_param = nv04_screen_get_param;
	pscreen->get_paramf = nv04_screen_get_paramf;
	pscreen->is_format_supported = nv04_screen_is_format_supported;

	nv04_screen_init_miptree_functions(pscreen);
	nv04_screen_init_transfer_functions(pscreen);

	if (dev->chipset >= 0x20) {
		fahrenheit_class = 0;
		sub3d_class = 0;
	} else if (dev->chipset >= 0x10) {
		fahrenheit_class = NV10_TEXTURED_TRIANGLE;
		sub3d_class = NV10_CONTEXT_SURFACES_3D;
	} else {
		fahrenheit_class=NV04_TEXTURED_TRIANGLE;
		sub3d_class = NV04_CONTEXT_SURFACES_3D;
	}

	if (!fahrenheit_class) {
		NOUVEAU_ERR("Unknown nv04 chipset: nv%02x\n", dev->chipset);
		return NULL;
	}

	/* 3D object */
	ret = nouveau_grobj_alloc(chan, 0xbeef0001, fahrenheit_class,
				  &screen->fahrenheit);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return NULL;
	}
	BIND_RING(chan, screen->fahrenheit, 7);

	/* 3D surface object */
	ret = nouveau_grobj_alloc(chan, 0xbeef0002, sub3d_class,
				  &screen->context_surfaces_3d);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D surface object: %d\n", ret);
		return NULL;
	}
	BIND_RING(chan, screen->context_surfaces_3d, 6);

	/* 2D engine setup */
	screen->eng2d = nv04_surface_2d_init(&screen->base);
	screen->eng2d->buf = nv04_surface_buffer;

	/* Notifier for sync purposes */
	ret = nouveau_notifier_alloc(chan, 0xbeef0301, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nv04_screen_destroy(pscreen);
		return NULL;
	}

	return pscreen;
}

