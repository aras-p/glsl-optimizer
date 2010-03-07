#include "nv40_context.h"
#include "nouveau/nouveau_util.h"

static struct pipe_buffer *
nv40_do_surface_buffer(struct pipe_surface *surface)
{
	struct nv40_miptree *mt = (struct nv40_miptree *)surface->texture;
	return mt->buffer;
}

#define nv40_surface_buffer(ps) nouveau_bo(nv40_do_surface_buffer(ps))

static boolean
nv40_state_framebuffer_validate(struct nv40_context *nv40)
{
	struct nouveau_channel *chan = nv40->screen->base.channel;
	struct nouveau_grobj *curie = nv40->screen->curie;
	struct pipe_framebuffer_state *fb = &nv40->framebuffer;
	struct nv04_surface *rt[4], *zeta;
	uint32_t rt_enable, rt_format;
	int i, colour_format = 0, zeta_format = 0;
	struct nouveau_stateobj *so = so_new(18, 24, 10);
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	unsigned w = fb->width;
	unsigned h = fb->height;

	rt_enable = 0;
	for (i = 0; i < fb->nr_cbufs; i++) {
		if (colour_format) {
			assert(colour_format == fb->cbufs[i]->format);
		} else {
			colour_format = fb->cbufs[i]->format;
			rt_enable |= (NV40TCL_RT_ENABLE_COLOR0 << i);
			rt[i] = (struct nv04_surface *)fb->cbufs[i];
		}
	}

	if (rt_enable & (NV40TCL_RT_ENABLE_COLOR1 | NV40TCL_RT_ENABLE_COLOR2 |
			 NV40TCL_RT_ENABLE_COLOR3))
		rt_enable |= NV40TCL_RT_ENABLE_MRT;

	if (fb->zsbuf) {
		zeta_format = fb->zsbuf->format;
		zeta = (struct nv04_surface *)fb->zsbuf;
	}

	if (!(rt[0]->base.texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR)) {
		assert(!(fb->width & (fb->width - 1)) && !(fb->height & (fb->height - 1)));
		for (i = 1; i < fb->nr_cbufs; i++)
			assert(!(rt[i]->base.texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR));

		rt_format = NV40TCL_RT_FORMAT_TYPE_SWIZZLED |
		            log2i(fb->width) << NV40TCL_RT_FORMAT_LOG2_WIDTH_SHIFT |
		            log2i(fb->height) << NV40TCL_RT_FORMAT_LOG2_HEIGHT_SHIFT;
	}
	else
		rt_format = NV40TCL_RT_FORMAT_TYPE_LINEAR;

	switch (colour_format) {
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_X8R8G8B8;
		break;
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case 0:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_A8R8G8B8;
		break;
	case PIPE_FORMAT_B5G6R5_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_R5G6B5;
		break;
	default:
		assert(0);
	}

	switch (zeta_format) {
	case PIPE_FORMAT_Z16_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_ZETA_Z16;
		break;
	case PIPE_FORMAT_S8Z24_UNORM:
	case PIPE_FORMAT_X8Z24_UNORM:
	case 0:
		rt_format |= NV40TCL_RT_FORMAT_ZETA_Z24S8;
		break;
	default:
		assert(0);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR0) {
		so_method(so, curie, NV40TCL_DMA_COLOR0, 1);
		so_reloc (so, nv40_surface_buffer(&rt[0]->base), 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, curie, NV40TCL_COLOR0_PITCH, 2);
		so_data  (so, rt[0]->pitch);
		so_reloc (so, nv40_surface_buffer(&rt[0]->base),
			      rt[0]->base.offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR1) {
		so_method(so, curie, NV40TCL_DMA_COLOR1, 1);
		so_reloc (so, nv40_surface_buffer(&rt[1]->base), 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, curie, NV40TCL_COLOR1_OFFSET, 2);
		so_reloc (so, nv40_surface_buffer(&rt[1]->base),
			      rt[1]->base.offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
		so_data  (so, rt[1]->pitch);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
		so_method(so, curie, NV40TCL_DMA_COLOR2, 1);
		so_reloc (so, nv40_surface_buffer(&rt[2]->base), 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, curie, NV40TCL_COLOR2_OFFSET, 1);
		so_reloc (so, nv40_surface_buffer(&rt[2]->base),
			      rt[2]->base.offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
		so_method(so, curie, NV40TCL_COLOR2_PITCH, 1);
		so_data  (so, rt[2]->pitch);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
		so_method(so, curie, NV40TCL_DMA_COLOR3, 1);
		so_reloc (so, nv40_surface_buffer(&rt[3]->base), 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, curie, NV40TCL_COLOR3_OFFSET, 1);
		so_reloc (so, nv40_surface_buffer(&rt[3]->base),
			      rt[3]->base.offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
		so_method(so, curie, NV40TCL_COLOR3_PITCH, 1);
		so_data  (so, rt[3]->pitch);
	}

	if (zeta_format) {
		so_method(so, curie, NV40TCL_DMA_ZETA, 1);
		so_reloc (so, nv40_surface_buffer(&zeta->base), 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, curie, NV40TCL_ZETA_OFFSET, 1);
		so_reloc (so, nv40_surface_buffer(&zeta->base),
			      zeta->base.offset, rt_flags | NOUVEAU_BO_LOW, 0, 0);
		so_method(so, curie, NV40TCL_ZETA_PITCH, 1);
		so_data  (so, zeta->pitch);
	}

	so_method(so, curie, NV40TCL_RT_ENABLE, 1);
	so_data  (so, rt_enable);
	so_method(so, curie, NV40TCL_RT_HORIZ, 3);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_data  (so, rt_format);
	so_method(so, curie, NV40TCL_VIEWPORT_HORIZ, 2);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_method(so, curie, NV40TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	so_data  (so, ((w - 1) << 16) | 0);
	so_data  (so, ((h - 1) << 16) | 0);
	so_method(so, curie, 0x1d88, 1);
	so_data  (so, (1 << 12) | h);

	so_ref(so, &nv40->state.hw[NV40_STATE_FB]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv40_state_entry nv40_state_framebuffer = {
	.validate = nv40_state_framebuffer_validate,
	.dirty = {
		.pipe = NV40_NEW_FB,
		.hw = NV40_STATE_FB
	}
};
