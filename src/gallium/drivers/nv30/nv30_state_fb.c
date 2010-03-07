#include "nv30_context.h"
#include "nouveau/nouveau_util.h"

static boolean
nv30_state_framebuffer_validate(struct nv30_context *nv30)
{
	struct pipe_framebuffer_state *fb = &nv30->framebuffer;
	struct nouveau_channel *chan = nv30->screen->base.channel;
	struct nouveau_grobj *rankine = nv30->screen->rankine;
	struct nv04_surface *rt[2], *zeta = NULL;
	uint32_t rt_enable = 0, rt_format = 0;
	int i, colour_format = 0, zeta_format = 0, depth_only = 0;
	struct nouveau_stateobj *so = so_new(12, 18, 10);
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	unsigned w = fb->width;
	unsigned h = fb->height;
	struct nv30_miptree *nv30mt;
	int colour_bits = 32, zeta_bits = 32;

	for (i = 0; i < fb->nr_cbufs; i++) {
		if (colour_format) {
			assert(colour_format == fb->cbufs[i]->format);
		} else {
			colour_format = fb->cbufs[i]->format;
			rt_enable |= (NV34TCL_RT_ENABLE_COLOR0 << i);
			rt[i] = (struct nv04_surface *)fb->cbufs[i];
		}
	}

	if (rt_enable & NV34TCL_RT_ENABLE_COLOR1)
		rt_enable |= NV34TCL_RT_ENABLE_MRT;

	if (fb->zsbuf) {
		zeta_format = fb->zsbuf->format;
		zeta = (struct nv04_surface *)fb->zsbuf;
	}

	if (rt_enable & (NV34TCL_RT_ENABLE_COLOR0|NV34TCL_RT_ENABLE_COLOR1)) {
		/* Render to at least a colour buffer */
		if (!(rt[0]->base.texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR)) {
			assert(!(fb->width & (fb->width - 1)) && !(fb->height & (fb->height - 1)));
			for (i = 1; i < fb->nr_cbufs; i++)
				assert(!(rt[i]->base.texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR));

			rt_format = NV34TCL_RT_FORMAT_TYPE_SWIZZLED |
				(log2i(rt[0]->base.width) << NV34TCL_RT_FORMAT_LOG2_WIDTH_SHIFT) |
				(log2i(rt[0]->base.height) << NV34TCL_RT_FORMAT_LOG2_HEIGHT_SHIFT);
		}
		else
			rt_format = NV34TCL_RT_FORMAT_TYPE_LINEAR;
	} else if (fb->zsbuf) {
		depth_only = 1;

		/* Render to depth buffer only */
		if (!(zeta->base.texture->tex_usage & NOUVEAU_TEXTURE_USAGE_LINEAR)) {
			assert(!(fb->width & (fb->width - 1)) && !(fb->height & (fb->height - 1)));

			rt_format = NV34TCL_RT_FORMAT_TYPE_SWIZZLED |
				(log2i(zeta->base.width) << NV34TCL_RT_FORMAT_LOG2_WIDTH_SHIFT) |
				(log2i(zeta->base.height) << NV34TCL_RT_FORMAT_LOG2_HEIGHT_SHIFT);
		}
		else
			rt_format = NV34TCL_RT_FORMAT_TYPE_LINEAR;
	} else {
		return FALSE;
	}

	switch (colour_format) {
	case PIPE_FORMAT_B8G8R8X8_UNORM:
		rt_format |= NV34TCL_RT_FORMAT_COLOR_X8R8G8B8;
		break;
	case PIPE_FORMAT_B8G8R8A8_UNORM:
	case 0:
		rt_format |= NV34TCL_RT_FORMAT_COLOR_A8R8G8B8;
		break;
	case PIPE_FORMAT_B5G6R5_UNORM:
		rt_format |= NV34TCL_RT_FORMAT_COLOR_R5G6B5;
		colour_bits = 16;
		break;
	default:
		assert(0);
	}

	switch (zeta_format) {
	case PIPE_FORMAT_Z16_UNORM:
		rt_format |= NV34TCL_RT_FORMAT_ZETA_Z16;
		zeta_bits = 16;
		break;
	case PIPE_FORMAT_S8Z24_UNORM:
	case PIPE_FORMAT_X8Z24_UNORM:
	case 0:
		rt_format |= NV34TCL_RT_FORMAT_ZETA_Z24S8;
		break;
	default:
		assert(0);
	}

	if (colour_bits > zeta_bits) {
		return FALSE;
	}

	if (depth_only || (rt_enable & NV34TCL_RT_ENABLE_COLOR0)) {
		struct nv04_surface *rt0 = (depth_only ? zeta : rt[0]);
		uint32_t pitch = rt0->pitch;

		if (zeta) {
			pitch |= (zeta->pitch << 16);
		} else {
			pitch |= (pitch << 16);
		}

		nv30mt = (struct nv30_miptree *) rt0->base.texture;
		so_method(so, rankine, NV34TCL_DMA_COLOR0, 1);
		so_reloc (so, nouveau_bo(nv30mt->buffer), 0, rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, rankine, NV34TCL_COLOR0_PITCH, 2);
		so_data  (so, pitch);
		so_reloc (so, nouveau_bo(nv30mt->buffer), rt0->base.offset,
			      rt_flags | NOUVEAU_BO_LOW, 0, 0);
	}

	if (rt_enable & NV34TCL_RT_ENABLE_COLOR1) {
		nv30mt = (struct nv30_miptree *)rt[1]->base.texture;
		so_method(so, rankine, NV34TCL_DMA_COLOR1, 1);
		so_reloc (so, nouveau_bo(nv30mt->buffer), 0, rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, rankine, NV34TCL_COLOR1_OFFSET, 2);
		so_reloc (so, nouveau_bo(nv30mt->buffer), rt[1]->base.offset,
			      rt_flags | NOUVEAU_BO_LOW, 0, 0);
		so_data  (so, rt[1]->pitch);
	}

	if (zeta_format) {
		nv30mt = (struct nv30_miptree *)zeta->base.texture;
		so_method(so, rankine, NV34TCL_DMA_ZETA, 1);
		so_reloc (so, nouveau_bo(nv30mt->buffer), 0, rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		so_method(so, rankine, NV34TCL_ZETA_OFFSET, 1);
		so_reloc (so, nouveau_bo(nv30mt->buffer), zeta->base.offset,
			      rt_flags | NOUVEAU_BO_LOW, 0, 0);
		/* TODO: allocate LMA depth buffer */
	}

	so_method(so, rankine, NV34TCL_RT_ENABLE, 1);
	so_data  (so, rt_enable);
	so_method(so, rankine, NV34TCL_RT_HORIZ, 3);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_data  (so, rt_format);
	so_method(so, rankine, NV34TCL_VIEWPORT_HORIZ, 2);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_method(so, rankine, NV34TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	so_data  (so, ((w - 1) << 16) | 0);
	so_data  (so, ((h - 1) << 16) | 0);
	so_method(so, rankine, 0x1d88, 1);
	so_data  (so, (1 << 12) | h);
	/* Wonder why this is needed, context should all be set to zero on init */
	so_method(so, rankine, NV34TCL_VIEWPORT_TX_ORIGIN, 1);
	so_data  (so, 0);

	so_ref(so, &nv30->state.hw[NV30_STATE_FB]);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv30_state_entry nv30_state_framebuffer = {
	.validate = nv30_state_framebuffer_validate,
	.dirty = {
		.pipe = NV30_NEW_FB,
		.hw = NV30_STATE_FB
	}
};
