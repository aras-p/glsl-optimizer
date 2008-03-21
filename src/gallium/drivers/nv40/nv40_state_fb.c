#include "nv40_context.h"

static boolean
nv40_state_framebuffer_validate(struct nv40_context *nv40)
{
	struct pipe_framebuffer_state *fb = &nv40->framebuffer;
	struct pipe_surface *rt[4], *zeta;
	uint32_t rt_enable, rt_format;
	int i, colour_format = 0, zeta_format = 0;
	struct nouveau_stateobj *so = so_new(64, 10);
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	unsigned w = fb->width;
	unsigned h = fb->height;

	rt_enable = 0;
	for (i = 0; i < fb->num_cbufs; i++) {
		if (colour_format) {
			assert(colour_format == fb->cbufs[i]->format);
		} else {
			colour_format = fb->cbufs[i]->format;
			rt_enable |= (NV40TCL_RT_ENABLE_COLOR0 << i);
			rt[i] = fb->cbufs[i];
		}
	}

	if (rt_enable & (NV40TCL_RT_ENABLE_COLOR1 | NV40TCL_RT_ENABLE_COLOR2 |
			 NV40TCL_RT_ENABLE_COLOR3))
		rt_enable |= NV40TCL_RT_ENABLE_MRT;

	if (fb->zsbuf) {
		zeta_format = fb->zsbuf->format;
		zeta = fb->zsbuf;
	}

	rt_format = NV40TCL_RT_FORMAT_TYPE_LINEAR;

	switch (colour_format) {
	case PIPE_FORMAT_A8R8G8B8_UNORM:
	case 0:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_A8R8G8B8;
		break;
	case PIPE_FORMAT_R5G6B5_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_COLOR_R5G6B5;
		break;
	default:
		assert(0);
	}

	switch (zeta_format) {
	case PIPE_FORMAT_Z16_UNORM:
		rt_format |= NV40TCL_RT_FORMAT_ZETA_Z16;
		break;
	case PIPE_FORMAT_Z24S8_UNORM:
	case 0:
		rt_format |= NV40TCL_RT_FORMAT_ZETA_Z24S8;
		break;
	default:
		assert(0);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR0) {
		so_method(so, nv40->screen->curie, NV40TCL_DMA_COLOR0, 1);
		so_reloc (so, rt[0]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->screen->curie, NV40TCL_COLOR0_PITCH, 2);
		so_data  (so, rt[0]->pitch * rt[0]->cpp);
		so_reloc (so, rt[0]->buffer, rt[0]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR1) {
		so_method(so, nv40->screen->curie, NV40TCL_DMA_COLOR1, 1);
		so_reloc (so, rt[1]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->screen->curie, NV40TCL_COLOR1_OFFSET, 2);
		so_reloc (so, rt[1]->buffer, rt[1]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_data  (so, rt[1]->pitch * rt[1]->cpp);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
		so_method(so, nv40->screen->curie, NV40TCL_DMA_COLOR2, 1);
		so_reloc (so, rt[2]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->screen->curie, NV40TCL_COLOR2_OFFSET, 1);
		so_reloc (so, rt[2]->buffer, rt[2]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_method(so, nv40->screen->curie, NV40TCL_COLOR2_PITCH, 1);
		so_data  (so, rt[2]->pitch * rt[2]->cpp);
	}

	if (rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
		so_method(so, nv40->screen->curie, NV40TCL_DMA_COLOR3, 1);
		so_reloc (so, rt[3]->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->screen->curie, NV40TCL_COLOR3_OFFSET, 1);
		so_reloc (so, rt[3]->buffer, rt[3]->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_method(so, nv40->screen->curie, NV40TCL_COLOR3_PITCH, 1);
		so_data  (so, rt[3]->pitch * rt[3]->cpp);
	}

	if (zeta_format) {
		so_method(so, nv40->screen->curie, NV40TCL_DMA_ZETA, 1);
		so_reloc (so, zeta->buffer, 0, rt_flags | NOUVEAU_BO_OR,
			  nv40->nvws->channel->vram->handle,
			  nv40->nvws->channel->gart->handle);
		so_method(so, nv40->screen->curie, NV40TCL_ZETA_OFFSET, 1);
		so_reloc (so, zeta->buffer, zeta->offset, rt_flags |
			  NOUVEAU_BO_LOW, 0, 0);
		so_method(so, nv40->screen->curie, NV40TCL_ZETA_PITCH, 1);
		so_data  (so, zeta->pitch * zeta->cpp);
	}

	so_method(so, nv40->screen->curie, NV40TCL_RT_ENABLE, 1);
	so_data  (so, rt_enable);
	so_method(so, nv40->screen->curie, NV40TCL_RT_HORIZ, 3);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_data  (so, rt_format);
	so_method(so, nv40->screen->curie, NV40TCL_VIEWPORT_HORIZ, 2);
	so_data  (so, (w << 16) | 0);
	so_data  (so, (h << 16) | 0);
	so_method(so, nv40->screen->curie, NV40TCL_VIEWPORT_CLIP_HORIZ(0), 2);
	so_data  (so, ((w - 1) << 16) | 0);
	so_data  (so, ((h - 1) << 16) | 0);

	so_ref(so, &nv40->state.hw[NV40_STATE_FB]);
	return TRUE;
}

struct nv40_state_entry nv40_state_framebuffer = {
	.validate = nv40_state_framebuffer_validate,
	.dirty = {
		.pipe = NV40_NEW_FB,
		.hw = NV40_STATE_FB
	}
};
