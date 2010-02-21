#include "pipe/p_screen.h"
#include "pipe/p_state.h"

#include "nouveau/nouveau_screen.h"

#include "nvfx_context.h"
#include "nvfx_screen.h"

#define NV30TCL_CHIPSET_3X_MASK 0x00000003
#define NV34TCL_CHIPSET_3X_MASK 0x00000010
#define NV35TCL_CHIPSET_3X_MASK 0x000001e0

/* FIXME: It seems I should not include directly ../../winsys/drm/nouveau/drm/nouveau_drm_api.h
* to get the pointer to the context front buffer, so I copied nouveau_winsys here.
* nv30_screen_surface_format_supported() can then use it to enforce creating fbo
* with same number of bits everywhere.
*/
struct nouveau_winsys {
	struct pipe_winsys base;

	struct pipe_screen *pscreen;

	struct pipe_surface *front;
};
#define NV4X_GRCLASS4097_CHIPSETS 0x00000baf
#define NV4X_GRCLASS4497_CHIPSETS 0x00005450
#define NV6X_GRCLASS4497_CHIPSETS 0x00000088

static int
nvfx_screen_get_param(struct pipe_screen *pscreen, int param)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);

	switch (param) {
	case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
		/* TODO: check this */
		return screen->is_nv4x ? 16 : 8;
	case PIPE_CAP_NPOT_TEXTURES:
		return !!screen->is_nv4x;
	case PIPE_CAP_TWO_SIDED_STENCIL:
		return 1;
	case PIPE_CAP_GLSL:
		return 0;
	case PIPE_CAP_ANISOTROPIC_FILTER:
		return 1;
	case PIPE_CAP_POINT_SPRITE:
		return 1;
	case PIPE_CAP_MAX_RENDER_TARGETS:
		return screen->is_nv4x ? 4 : 2;
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
		return !!screen->is_nv4x;
	case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
		return 1;
	case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
		return 0; /* We have 4 on nv40 - but unsupported currently */
	case PIPE_CAP_TGSI_CONT_SUPPORTED:
		return 0;
	case PIPE_CAP_BLEND_EQUATION_SEPARATE:
		return !!screen->is_nv4x;
	case NOUVEAU_CAP_HW_VTXBUF:
		/* TODO: this is almost surely wrong */
		return !!screen->is_nv4x;
	case NOUVEAU_CAP_HW_IDXBUF:
		/* TODO: this is also almost surely wrong */
		return screen->is_nv4x && screen->eng3d->grclass == NV40TCL;
	case PIPE_CAP_MAX_COMBINED_SAMPLERS:
		return 16;
	case PIPE_CAP_INDEP_BLEND_ENABLE:
		/* TODO: on nv40 we have separate color masks */
		/* TODO: nv40 mrt blending is probably broken */
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
nvfx_screen_get_paramf(struct pipe_screen *pscreen, int param)
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
		return screen->is_nv4x ? 16.0 : 8.0;
	case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
		return screen->is_nv4x ? 16.0 : 4.0;
	default:
		NOUVEAU_ERR("Unknown PIPE_CAP %d\n", param);
		return 0.0;
	}
}

static boolean
nvfx_screen_surface_format_supported(struct pipe_screen *pscreen,
				     enum pipe_format format,
				     enum pipe_texture_target target,
				     unsigned tex_usage, unsigned geom_flags)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);
	struct pipe_surface *front = ((struct nouveau_winsys *) pscreen->winsys)->front;

	if (tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET) {
		switch (format) {
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B5G6R5_UNORM:
			return TRUE;
		default:
			break;
		}
	} else
	if (tex_usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL) {
		switch (format) {
		case PIPE_FORMAT_S8Z24_UNORM:
		case PIPE_FORMAT_X8Z24_UNORM:
			return TRUE;
		case PIPE_FORMAT_Z16_UNORM:
			/* TODO: this nv30 limitation probably does not exist */
			if (!screen->is_nv4x && front)
				return (front->format == PIPE_FORMAT_B5G6R5_UNORM);
			return TRUE;
		default:
			break;
		}
	} else {
		switch (format) {
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case PIPE_FORMAT_B5G5R5A1_UNORM:
		case PIPE_FORMAT_B4G4R4A4_UNORM:
		case PIPE_FORMAT_B5G6R5_UNORM:
		case PIPE_FORMAT_L8_UNORM:
		case PIPE_FORMAT_A8_UNORM:
		case PIPE_FORMAT_I8_UNORM:
		case PIPE_FORMAT_L8A8_UNORM:
		case PIPE_FORMAT_Z16_UNORM:
		case PIPE_FORMAT_S8Z24_UNORM:
		case PIPE_FORMAT_DXT1_RGB:
		case PIPE_FORMAT_DXT1_RGBA:
		case PIPE_FORMAT_DXT3_RGBA:
		case PIPE_FORMAT_DXT5_RGBA:
			return TRUE;
		/* TODO: does nv30 support this? */
		case PIPE_FORMAT_R16_SNORM:
			return !!screen->is_nv4x;
		default:
			break;
		}
	}

	return FALSE;
}

static struct pipe_buffer *
nvfx_surface_buffer(struct pipe_surface *surf)
{
	struct nvfx_miptree *mt = (struct nvfx_miptree *)surf->texture;

	return mt->buffer;
}

static void
nvfx_screen_destroy(struct pipe_screen *pscreen)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);
	unsigned i;

	for (i = 0; i < NVFX_STATE_MAX; i++) {
		if (screen->state[i])
			so_ref(NULL, &screen->state[i]);
	}

	nouveau_resource_destroy(&screen->vp_exec_heap);
	nouveau_resource_destroy(&screen->vp_data_heap);
	nouveau_resource_destroy(&screen->query_heap);
	nouveau_notifier_free(&screen->query);
	nouveau_notifier_free(&screen->sync);
	nouveau_grobj_free(&screen->eng3d);
	nv04_surface_2d_takedown(&screen->eng2d);

	nouveau_screen_fini(&screen->base);

	FREE(pscreen);
}

static void nv30_screen_init(struct nvfx_screen *screen, struct nouveau_stateobj* so)
{
	int i;

	/* TODO: perhaps we should do some of this on nv40 too? */
	for (i=1; i<8; i++) {
		so_method(so, screen->eng3d, NV34TCL_VIEWPORT_CLIP_HORIZ(i), 1);
		so_data  (so, 0);
		so_method(so, screen->eng3d, NV34TCL_VIEWPORT_CLIP_VERT(i), 1);
		so_data  (so, 0);
	}

	so_method(so, screen->eng3d, 0x220, 1);
	so_data  (so, 1);

	so_method(so, screen->eng3d, 0x03b0, 1);
	so_data  (so, 0x00100000);
	so_method(so, screen->eng3d, 0x1454, 1);
	so_data  (so, 0);
	so_method(so, screen->eng3d, 0x1d80, 1);
	so_data  (so, 3);
	so_method(so, screen->eng3d, 0x1450, 1);
	so_data  (so, 0x00030004);

	/* NEW */
	so_method(so, screen->eng3d, 0x1e98, 1);
	so_data  (so, 0);
	so_method(so, screen->eng3d, 0x17e0, 3);
	so_data  (so, fui(0.0));
	so_data  (so, fui(0.0));
	so_data  (so, fui(1.0));
	so_method(so, screen->eng3d, 0x1f80, 16);
	for (i=0; i<16; i++) {
		so_data  (so, (i==8) ? 0x0000ffff : 0);
	}

	so_method(so, screen->eng3d, 0x120, 3);
	so_data  (so, 0);
	so_data  (so, 1);
	so_data  (so, 2);

	so_method(so, screen->eng3d, 0x1d88, 1);
	so_data  (so, 0x00001200);

	so_method(so, screen->eng3d, NV34TCL_RC_ENABLE, 1);
	so_data  (so, 0);

	so_method(so, screen->eng3d, NV34TCL_DEPTH_RANGE_NEAR, 2);
	so_data  (so, fui(0.0));
	so_data  (so, fui(1.0));

	so_method(so, screen->eng3d, NV34TCL_MULTISAMPLE_CONTROL, 1);
	so_data  (so, 0xffff0000);

	/* enables use of vp rather than fixed-function somehow */
	so_method(so, screen->eng3d, 0x1e94, 1);
	so_data  (so, 0x13);
}

static void nv40_screen_init(struct nvfx_screen *screen, struct nouveau_stateobj* so)
{
	so_method(so, screen->eng3d, NV40TCL_DMA_COLOR2, 2);
	so_data  (so, screen->base.channel->vram->handle);
	so_data  (so, screen->base.channel->vram->handle);

	so_method(so, screen->eng3d, 0x1ea4, 3);
	so_data  (so, 0x00000010);
	so_data  (so, 0x01000100);
	so_data  (so, 0xff800006);

	/* vtxprog output routing */
	so_method(so, screen->eng3d, 0x1fc4, 1);
	so_data  (so, 0x06144321);
	so_method(so, screen->eng3d, 0x1fc8, 2);
	so_data  (so, 0xedcba987);
	so_data  (so, 0x00000021);
	so_method(so, screen->eng3d, 0x1fd0, 1);
	so_data  (so, 0x00171615);
	so_method(so, screen->eng3d, 0x1fd4, 1);
	so_data  (so, 0x001b1a19);

	so_method(so, screen->eng3d, 0x1ef8, 1);
	so_data  (so, 0x0020ffff);
	so_method(so, screen->eng3d, 0x1d64, 1);
	so_data  (so, 0x00d30000);
	so_method(so, screen->eng3d, 0x1e94, 1);
	so_data  (so, 0x00000001);
}

struct pipe_screen *
nvfx_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
	struct nvfx_screen *screen = CALLOC_STRUCT(nvfx_screen);
	struct nouveau_channel *chan;
	struct pipe_screen *pscreen;
	struct nouveau_stateobj *so;
	unsigned eng3d_class = 0;
	int ret;

	if (!screen)
		return NULL;

	pscreen = &screen->base.base;

	ret = nouveau_screen_init(&screen->base, dev);
	if (ret) {
		nvfx_screen_destroy(pscreen);
		return NULL;
	}
	chan = screen->base.channel;

	pscreen->winsys = ws;
	pscreen->destroy = nvfx_screen_destroy;
	pscreen->get_param = nvfx_screen_get_param;
	pscreen->get_paramf = nvfx_screen_get_paramf;
	pscreen->is_format_supported = nvfx_screen_surface_format_supported;
	pscreen->context_create = nvfx_create;

	switch (dev->chipset & 0xf0) {
	case 0x30:
		if (NV30TCL_CHIPSET_3X_MASK & (1 << (dev->chipset & 0x0f)))
			eng3d_class = 0x0397;
		else if (NV34TCL_CHIPSET_3X_MASK & (1 << (dev->chipset & 0x0f)))
			eng3d_class = 0x0697;
		else if (NV35TCL_CHIPSET_3X_MASK & (1 << (dev->chipset & 0x0f)))
			eng3d_class = 0x0497;
		break;
	case 0x40:
		if (NV4X_GRCLASS4097_CHIPSETS & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV40TCL;
		else if (NV4X_GRCLASS4497_CHIPSETS & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV44TCL;
		screen->is_nv4x = ~0;
		break;
	case 0x60:
		if (NV6X_GRCLASS4497_CHIPSETS & (1 << (dev->chipset & 0x0f)))
			eng3d_class = NV44TCL;
		screen->is_nv4x = ~0;
		break;
	}

	if (!eng3d_class) {
		NOUVEAU_ERR("Unknown nv3x/nv4x chipset: nv%02x\n", dev->chipset);
		return NULL;
	}

	nvfx_screen_init_miptree_functions(pscreen);

	ret = nouveau_grobj_alloc(chan, 0xbeef3097, eng3d_class, &screen->eng3d);
	if (ret) {
		NOUVEAU_ERR("Error creating 3D object: %d\n", ret);
		return FALSE;
	}

	/* 2D engine setup */
	screen->eng2d = nv04_surface_2d_init(&screen->base);
	screen->eng2d->buf = nvfx_surface_buffer;

	/* Notifier for sync purposes */
	ret = nouveau_notifier_alloc(chan, 0xbeef0301, 1, &screen->sync);
	if (ret) {
		NOUVEAU_ERR("Error creating notifier object: %d\n", ret);
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	/* Query objects */
	ret = nouveau_notifier_alloc(chan, 0xbeef0302, 32, &screen->query);
	if (ret) {
		NOUVEAU_ERR("Error initialising query objects: %d\n", ret);
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	ret = nouveau_resource_init(&screen->query_heap, 0, 32);
	if (ret) {
		NOUVEAU_ERR("Error initialising query object heap: %d\n", ret);
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	/* Vtxprog resources */
	if (nouveau_resource_init(&screen->vp_exec_heap, 0, screen->is_nv4x ? 512 : 256) ||
	    nouveau_resource_init(&screen->vp_data_heap, 0, 256)) {
		nvfx_screen_destroy(pscreen);
		return NULL;
	}

	/* Static eng3d initialisation */
	/* make the so big and don't worry about exact values
	   since we it will be thrown away immediately after use */
	so = so_new(256, 256, 0);
	so_method(so, screen->eng3d, NV34TCL_DMA_NOTIFY, 1);
	so_data  (so, screen->sync->handle);
	so_method(so, screen->eng3d, NV34TCL_DMA_TEXTURE0, 2);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->gart->handle);
	so_method(so, screen->eng3d, NV34TCL_DMA_COLOR1, 1);
	so_data  (so, chan->vram->handle);
	so_method(so, screen->eng3d, NV34TCL_DMA_COLOR0, 2);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->vram->handle);
	so_method(so, screen->eng3d, NV34TCL_DMA_VTXBUF0, 2);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->gart->handle);

	so_method(so, screen->eng3d, NV34TCL_DMA_FENCE, 2);
	so_data  (so, 0);
	so_data  (so, screen->query->handle);

	so_method(so, screen->eng3d, NV34TCL_DMA_IN_MEMORY7, 2);
	so_data  (so, chan->vram->handle);
	so_data  (so, chan->vram->handle);

	if(!screen->is_nv4x)
		nv30_screen_init(screen, so);
	else
		nv40_screen_init(screen, so);

	so_emit(chan, so);
	so_ref(NULL, &so);
	nouveau_pushbuf_flush(chan, 0);

	return pscreen;
}
