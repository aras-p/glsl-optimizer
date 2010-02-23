#include "nvfx_context.h"
#include "nvfx_resource.h"
#include "nouveau/nouveau_util.h"



void
nvfx_state_framebuffer_validate(struct nvfx_context *nvfx)
{
	struct pipe_framebuffer_state *fb = &nvfx->framebuffer;
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	uint32_t rt_enable = 0, rt_format = 0;
	int i, colour_format = 0, zeta_format = 0;
	int depth_only = 0;
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	unsigned w = fb->width;
	unsigned h = fb->height;
	int colour_bits = 32, zeta_bits = 32;

	if(!nvfx->is_nv4x)
		assert(fb->nr_cbufs <= 2);
	else
		assert(fb->nr_cbufs <= 4);

	for (i = 0; i < fb->nr_cbufs; i++) {
		if (colour_format)
			assert(colour_format == fb->cbufs[i]->format);
		else
			colour_format = fb->cbufs[i]->format;

		rt_enable |= (NV34TCL_RT_ENABLE_COLOR0 << i);
		nvfx->hw_rt[i].bo = nvfx_surface_buffer(fb->cbufs[i]);
		nvfx->hw_rt[i].offset = fb->cbufs[i]->offset;
		nvfx->hw_rt[i].pitch = ((struct nv04_surface *)fb->cbufs[i])->pitch;
	}
	for(; i < 4; ++i)
		nvfx->hw_rt[i].bo = 0;

	if (rt_enable & (NV34TCL_RT_ENABLE_COLOR1 |
			 NV40TCL_RT_ENABLE_COLOR2 | NV40TCL_RT_ENABLE_COLOR3))
		rt_enable |= NV34TCL_RT_ENABLE_MRT;

	if (fb->zsbuf) {
		zeta_format = fb->zsbuf->format;
		nvfx->hw_zeta.bo = nvfx_surface_buffer(fb->zsbuf);
		nvfx->hw_zeta.offset = fb->zsbuf->offset;
		nvfx->hw_zeta.pitch = ((struct nv04_surface *)fb->zsbuf)->pitch;
	}
	else
		nvfx->hw_zeta.bo = 0;

	if (rt_enable & (NV34TCL_RT_ENABLE_COLOR0 | NV34TCL_RT_ENABLE_COLOR1 |
		NV40TCL_RT_ENABLE_COLOR2 | NV40TCL_RT_ENABLE_COLOR3)) {
		/* Render to at least a colour buffer */
		if (!(fb->cbufs[0]->texture->flags & NVFX_RESOURCE_FLAG_LINEAR)) {
			assert(!(fb->width & (fb->width - 1)) && !(fb->height & (fb->height - 1)));
			for (i = 1; i < fb->nr_cbufs; i++)
				assert(!(fb->cbufs[i]->texture->flags & NVFX_RESOURCE_FLAG_LINEAR));

			rt_format = NV34TCL_RT_FORMAT_TYPE_SWIZZLED |
				(log2i(fb->cbufs[0]->width) << NV34TCL_RT_FORMAT_LOG2_WIDTH_SHIFT) |
				(log2i(fb->cbufs[0]->height) << NV34TCL_RT_FORMAT_LOG2_HEIGHT_SHIFT);
		}
		else
			rt_format = NV34TCL_RT_FORMAT_TYPE_LINEAR;
	} else if (fb->zsbuf) {
		depth_only = 1;

		/* Render to depth buffer only */
		if (!(fb->zsbuf->texture->_usage & NVFX_RESOURCE_FLAG_LINEAR)) {
			assert(!(fb->width & (fb->width - 1)) && !(fb->height & (fb->height - 1)));

			rt_format = NV34TCL_RT_FORMAT_TYPE_SWIZZLED |
				(log2i(fb->zsbuf->width) << NV34TCL_RT_FORMAT_LOG2_WIDTH_SHIFT) |
				(log2i(fb->zsbuf->height) << NV34TCL_RT_FORMAT_LOG2_HEIGHT_SHIFT);
		}
		else
			rt_format = NV34TCL_RT_FORMAT_TYPE_LINEAR;
	} else {
		return;
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
	case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
	case PIPE_FORMAT_X8Z24_UNORM:
	case 0:
		rt_format |= NV34TCL_RT_FORMAT_ZETA_Z24S8;
		break;
	default:
		assert(0);
	}

	if ((!nvfx->is_nv4x) && colour_bits > zeta_bits) {
		/* TODO: does this limitation really exist?
		   TODO: can it be worked around somehow? */
		assert(0);
	}

	if ((rt_enable & NV34TCL_RT_ENABLE_COLOR0)
		|| ((!nvfx->is_nv4x) && depth_only)) {
		struct nvfx_render_target *rt0 = (depth_only ? &nvfx->hw_zeta : &nvfx->hw_rt[0]);
		uint32_t pitch = rt0->pitch;

		if(!nvfx->is_nv4x)
		{
			if (nvfx->hw_zeta.bo) {
				pitch |= (nvfx->hw_zeta.pitch << 16);
			} else {
				pitch |= (pitch << 16);
			}
		}

		OUT_RING(chan, RING_3D(NV34TCL_DMA_COLOR0, 1));
		OUT_RELOC(chan, rt0->bo, 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		OUT_RING(chan, RING_3D(NV34TCL_COLOR0_PITCH, 2));
		OUT_RING(chan, pitch);
		OUT_RELOC(chan, rt0->bo,
			      rt0->offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
	}

	if (rt_enable & NV34TCL_RT_ENABLE_COLOR1) {
		OUT_RING(chan, RING_3D(NV34TCL_DMA_COLOR1, 1));
		OUT_RELOC(chan, nvfx->hw_rt[1].bo, 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		OUT_RING(chan, RING_3D(NV34TCL_COLOR1_OFFSET, 2));
		OUT_RELOC(chan, nvfx->hw_rt[1].bo,
				nvfx->hw_rt[1].offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
		OUT_RING(chan, nvfx->hw_rt[1].pitch);
	}

	if(nvfx->is_nv4x)
	{
		if (rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
			OUT_RING(chan, RING_3D(NV40TCL_DMA_COLOR2, 1));
			OUT_RELOC(chan, nvfx->hw_rt[2].bo, 0,
				      rt_flags | NOUVEAU_BO_OR,
				      chan->vram->handle, chan->gart->handle);
			OUT_RING(chan, RING_3D(NV40TCL_COLOR2_OFFSET, 1));
			OUT_RELOC(chan, nvfx->hw_rt[2].bo,
				      nvfx->hw_rt[2].offset, rt_flags | NOUVEAU_BO_LOW,
				      0, 0);
			OUT_RING(chan, RING_3D(NV40TCL_COLOR2_PITCH, 1));
			OUT_RING(chan, nvfx->hw_rt[2].pitch);
		}

		if (rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
			OUT_RING(chan, RING_3D(NV40TCL_DMA_COLOR3, 1));
			OUT_RELOC(chan, nvfx->hw_rt[3].bo, 0,
				      rt_flags | NOUVEAU_BO_OR,
				      chan->vram->handle, chan->gart->handle);
			OUT_RING(chan, RING_3D(NV40TCL_COLOR3_OFFSET, 1));
			OUT_RELOC(chan, nvfx->hw_rt[3].bo,
					nvfx->hw_rt[3].offset, rt_flags | NOUVEAU_BO_LOW,
				      0, 0);
			OUT_RING(chan, RING_3D(NV40TCL_COLOR3_PITCH, 1));
			OUT_RING(chan, nvfx->hw_rt[3].pitch);
		}
	}

	if (zeta_format) {
		OUT_RING(chan, RING_3D(NV34TCL_DMA_ZETA, 1));
		OUT_RELOC(chan, nvfx->hw_zeta.bo, 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		OUT_RING(chan, RING_3D(NV34TCL_ZETA_OFFSET, 1));
		/* TODO: reverse engineer LMA */
		OUT_RELOC(chan, nvfx->hw_zeta.bo,
			     nvfx->hw_zeta.offset, rt_flags | NOUVEAU_BO_LOW, 0, 0);
	        if(nvfx->is_nv4x) {
			OUT_RING(chan, RING_3D(NV40TCL_ZETA_PITCH, 1));
			OUT_RING(chan, nvfx->hw_zeta.pitch);
		}
	}

	OUT_RING(chan, RING_3D(NV34TCL_RT_ENABLE, 1));
	OUT_RING(chan, rt_enable);
	OUT_RING(chan, RING_3D(NV34TCL_RT_HORIZ, 3));
	OUT_RING(chan, (w << 16) | 0);
	OUT_RING(chan, (h << 16) | 0);
	OUT_RING(chan, rt_format);
	OUT_RING(chan, RING_3D(NV34TCL_VIEWPORT_HORIZ, 2));
	OUT_RING(chan, (w << 16) | 0);
	OUT_RING(chan, (h << 16) | 0);
	OUT_RING(chan, RING_3D(NV34TCL_VIEWPORT_CLIP_HORIZ(0), 2));
	OUT_RING(chan, ((w - 1) << 16) | 0);
	OUT_RING(chan, ((h - 1) << 16) | 0);
	OUT_RING(chan, RING_3D(0x1d88, 1));
	OUT_RING(chan, (1 << 12) | h);

	if(!nvfx->is_nv4x) {
		/* Wonder why this is needed, context should all be set to zero on init */
		/* TODO: we can most likely remove this, after putting it in context init */
		OUT_RING(chan, RING_3D(NV34TCL_VIEWPORT_TX_ORIGIN, 1));
		OUT_RING(chan, 0);
	}
}

void
nvfx_framebuffer_relocate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	rt_flags |= NOUVEAU_BO_DUMMY;
	MARK_RING(chan, 20, 20);

#define DO_(var, pfx, name) \
	if(var.bo) { \
		OUT_RELOC(chan, var.bo, RING_3D(pfx##TCL_DMA_##name, 1), rt_flags, 0, 0); \
		OUT_RELOC(chan, var.bo, 0, \
			rt_flags | NOUVEAU_BO_OR, \
			chan->vram->handle, chan->gart->handle); \
		OUT_RELOC(chan, var.bo, RING_3D(pfx##TCL_##name##_OFFSET, 1), rt_flags, 0, 0); \
		OUT_RELOC(chan, var.bo, \
			var.offset, rt_flags | NOUVEAU_BO_LOW, \
			0, 0); \
	}

#define DO(pfx, num) DO_(nvfx->hw_rt[num], pfx, COLOR##num)
	DO(NV34, 0);
	DO(NV34, 1);
	DO(NV40, 2);
	DO(NV40, 3);

	DO_(nvfx->hw_zeta, NV34, ZETA);
}
