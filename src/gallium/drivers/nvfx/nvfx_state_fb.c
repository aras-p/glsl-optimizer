#include "nvfx_context.h"
#include "nvfx_resource.h"
#include "util/u_format.h"

static inline boolean
nvfx_surface_linear_target(struct pipe_surface* surf)
{
	return !!((struct nvfx_miptree*)surf->texture)->linear_pitch;
}

static void
nvfx_surface_get_render_target(struct pipe_surface* surf,
                               struct nvfx_render_target* target)
{
	struct nvfx_surface* ns = (struct nvfx_surface*)surf;

	target->bo = ((struct nvfx_miptree*)surf->texture)->base.bo;
	target->offset = ns->offset;
	target->pitch = align(ns->pitch, 64);
	assert(target->pitch);
}

void
nvfx_framebuffer_validate(struct nvfx_context *nvfx)
{
	struct pipe_framebuffer_state *fb = &nvfx->framebuffer;
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	uint32_t rt_enable, rt_format;
	int i;
	unsigned rt_flags = NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM;
	unsigned w = fb->width;
	unsigned h = fb->height;
	int all_swizzled =1 , cb_format = 0;

	/* do some sanity checks on the render target state and check if the targets
	 * are swizzled
	 */
	nvfx->is_nv4x ? assert(fb->nr_cbufs <= 4) : assert(fb->nr_cbufs <= 1);
	if(fb->nr_cbufs && fb->zsbuf)
		assert(util_format_get_blocksize(fb->cbufs[0]->format) ==
			   util_format_get_blocksize(fb->zsbuf->format));

	for(i = 0; i < fb->nr_cbufs; i++) {
		if(cb_format)
			assert(cb_format == fb->cbufs[i]->format);
		else
			cb_format = fb->cbufs[i]->format;

		if(nvfx_surface_linear_target(fb->cbufs[i]))
			all_swizzled = 0;
	}

	if(fb->zsbuf && nvfx_surface_linear_target(fb->zsbuf))
		all_swizzled = 0;

	rt_enable = (NV30_3D_RT_ENABLE_COLOR0 << fb->nr_cbufs) - 1;
	if(rt_enable & (NV30_3D_RT_ENABLE_COLOR1 |
                    NV40_3D_RT_ENABLE_COLOR2 | NV40_3D_RT_ENABLE_COLOR3))
		rt_enable |= NV30_3D_RT_ENABLE_MRT;

	for(i = 0; i < fb->nr_cbufs; i++)
		nvfx_surface_get_render_target(fb->cbufs[i], &nvfx->hw_rt[i]);

	for(; i < 4; ++i)
		nvfx->hw_rt[i].bo = NULL;

	nvfx->hw_zeta.bo = NULL;

	if(fb->zsbuf) {
		nvfx_surface_get_render_target(fb->zsbuf, &nvfx->hw_zeta);
		assert(util_format_get_stride(fb->zsbuf->format, fb->width) <=
			   nvfx->hw_zeta.pitch);
	}

	if(all_swizzled) {
		/* hardware rounds down render target offset to 64 bytes,
		 * but surfaces with a size of 2x2 pixel (16bpp) or 1x1 pixel (32bpp)
		 * have an unaligned start address, for those two important square
		 * formats we can hack around this limitation by adjusting the viewport
		 */
		if(nvfx->hw_rt[0].offset & 63) {
			int delta = nvfx->hw_rt[0].offset & 63;
			h = 2;
			w = 16;
			nvfx->viewport.translate[0] += delta /
						(util_format_get_blocksize(fb->cbufs[0]->format) * 2);
			nvfx->dirty |= NVFX_NEW_VIEWPORT;
		}

		rt_format = NV30_3D_RT_FORMAT_TYPE_SWIZZLED |
			(util_logbase2(w) << NV30_3D_RT_FORMAT_LOG2_WIDTH__SHIFT) |
			(util_logbase2(h) << NV30_3D_RT_FORMAT_LOG2_HEIGHT__SHIFT);
	} else {
		rt_format = NV30_3D_RT_FORMAT_TYPE_LINEAR;
	}

	if(fb->nr_cbufs > 0) {
		switch (fb->cbufs[0]->format) {
		case PIPE_FORMAT_B8G8R8X8_UNORM:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_X8R8G8B8;
			break;
		case PIPE_FORMAT_B8G8R8A8_UNORM:
		case 0:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_A8R8G8B8;
			break;
		case PIPE_FORMAT_R8G8B8X8_UNORM:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_X8B8G8R8;
			break;
		case PIPE_FORMAT_R8G8B8A8_UNORM:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_A8B8G8R8;
			break;
		case PIPE_FORMAT_B5G6R5_UNORM:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_R5G6B5;
			break;
		case PIPE_FORMAT_R32G32B32A32_FLOAT:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_A32B32G32R32_FLOAT;
			break;
		case PIPE_FORMAT_R16G16B16A16_FLOAT:
			rt_format |= NV30_3D_RT_FORMAT_COLOR_A16B16G16R16_FLOAT;
			break;
		default:
			assert(0);
		}
	} else if(fb->zsbuf && util_format_get_blocksize(fb->zsbuf->format) == 2)
		rt_format |= NV30_3D_RT_FORMAT_COLOR_R5G6B5;
	else
		rt_format |= NV30_3D_RT_FORMAT_COLOR_A8R8G8B8;

	if(fb->zsbuf) {
		switch (fb->zsbuf->format) {
		case PIPE_FORMAT_Z16_UNORM:
			rt_format |= NV30_3D_RT_FORMAT_ZETA_Z16;
			break;
		case PIPE_FORMAT_S8_UINT_Z24_UNORM:
		case PIPE_FORMAT_X8Z24_UNORM:
		case 0:
			rt_format |= NV30_3D_RT_FORMAT_ZETA_Z24S8;
			break;
		default:
			assert(0);
		}
	} else if(fb->nr_cbufs && util_format_get_blocksize(fb->cbufs[0]->format) == 2)
		rt_format |= NV30_3D_RT_FORMAT_ZETA_Z16;
	else
		rt_format |= NV30_3D_RT_FORMAT_ZETA_Z24S8;

	MARK_RING(chan, 42, 10);

	if ((rt_enable & NV30_3D_RT_ENABLE_COLOR0) || fb->zsbuf) {
		struct nvfx_render_target *rt0 = &nvfx->hw_rt[0];
		uint32_t pitch;

		if(!(rt_enable & NV30_3D_RT_ENABLE_COLOR0))
			rt0 = &nvfx->hw_zeta;

		pitch = rt0->pitch;

		if(!nvfx->is_nv4x)
		{
			if (nvfx->hw_zeta.bo)
				pitch |= (nvfx->hw_zeta.pitch << 16);
			else
				pitch |= (pitch << 16);
		}

		//printf("rendering to bo %p [%i] at offset %i with pitch %i\n", rt0->bo, rt0->bo->handle, rt0->offset, pitch);

		BEGIN_RING(chan, eng3d, NV30_3D_DMA_COLOR0, 1);
		OUT_RELOC(chan, rt0->bo, 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		BEGIN_RING(chan, eng3d, NV30_3D_COLOR0_PITCH, 2);
		OUT_RING(chan, pitch);
		OUT_RELOC(chan, rt0->bo,
			      rt0->offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
	}

	if (rt_enable & NV30_3D_RT_ENABLE_COLOR1) {
		BEGIN_RING(chan, eng3d, NV30_3D_DMA_COLOR1, 1);
		OUT_RELOC(chan, nvfx->hw_rt[1].bo, 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		BEGIN_RING(chan, eng3d, NV30_3D_COLOR1_OFFSET, 2);
		OUT_RELOC(chan, nvfx->hw_rt[1].bo,
				nvfx->hw_rt[1].offset, rt_flags | NOUVEAU_BO_LOW,
			      0, 0);
		OUT_RING(chan, nvfx->hw_rt[1].pitch);
	}

	if(nvfx->is_nv4x)
	{
		if (rt_enable & NV40_3D_RT_ENABLE_COLOR2) {
			BEGIN_RING(chan, eng3d, NV40_3D_DMA_COLOR2, 1);
			OUT_RELOC(chan, nvfx->hw_rt[2].bo, 0,
				      rt_flags | NOUVEAU_BO_OR,
				      chan->vram->handle, chan->gart->handle);
			BEGIN_RING(chan, eng3d, NV40_3D_COLOR2_OFFSET, 1);
			OUT_RELOC(chan, nvfx->hw_rt[2].bo,
				      nvfx->hw_rt[2].offset, rt_flags | NOUVEAU_BO_LOW,
				      0, 0);
			BEGIN_RING(chan, eng3d, NV40_3D_COLOR2_PITCH, 1);
			OUT_RING(chan, nvfx->hw_rt[2].pitch);
		}

		if (rt_enable & NV40_3D_RT_ENABLE_COLOR3) {
			BEGIN_RING(chan, eng3d, NV40_3D_DMA_COLOR3, 1);
			OUT_RELOC(chan, nvfx->hw_rt[3].bo, 0,
				      rt_flags | NOUVEAU_BO_OR,
				      chan->vram->handle, chan->gart->handle);
			BEGIN_RING(chan, eng3d, NV40_3D_COLOR3_OFFSET, 1);
			OUT_RELOC(chan, nvfx->hw_rt[3].bo,
					nvfx->hw_rt[3].offset, rt_flags | NOUVEAU_BO_LOW,
				      0, 0);
			BEGIN_RING(chan, eng3d, NV40_3D_COLOR3_PITCH, 1);
			OUT_RING(chan, nvfx->hw_rt[3].pitch);
		}
	}

	if (fb->zsbuf) {
		BEGIN_RING(chan, eng3d, NV30_3D_DMA_ZETA, 1);
		OUT_RELOC(chan, nvfx->hw_zeta.bo, 0,
			      rt_flags | NOUVEAU_BO_OR,
			      chan->vram->handle, chan->gart->handle);
		BEGIN_RING(chan, eng3d, NV30_3D_ZETA_OFFSET, 1);
		/* TODO: reverse engineer LMA */
		OUT_RELOC(chan, nvfx->hw_zeta.bo,
			     nvfx->hw_zeta.offset, rt_flags | NOUVEAU_BO_LOW, 0, 0);
	        if(nvfx->is_nv4x) {
			BEGIN_RING(chan, eng3d, NV40_3D_ZETA_PITCH, 1);
			OUT_RING(chan, nvfx->hw_zeta.pitch);
		}
	}
	else if(nvfx->is_nv4x) {
		BEGIN_RING(chan, eng3d, NV40_3D_ZETA_PITCH, 1);
		OUT_RING(chan, 64);
	}

	BEGIN_RING(chan, eng3d, NV30_3D_RT_ENABLE, 1);
	OUT_RING(chan, rt_enable);
	BEGIN_RING(chan, eng3d, NV30_3D_RT_HORIZ, 3);
	OUT_RING(chan, (w << 16) | 0);
	OUT_RING(chan, (h << 16) | 0);
	OUT_RING(chan, rt_format);
	BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_HORIZ, 2);
	OUT_RING(chan, (w << 16) | 0);
	OUT_RING(chan, (h << 16) | 0);
	BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_CLIP_HORIZ(0), 2);
	OUT_RING(chan, ((w - 1) << 16) | 0);
	OUT_RING(chan, ((h - 1) << 16) | 0);

	if(!nvfx->is_nv4x) {
		/* Wonder why this is needed, context should all be set to zero on init */
		/* TODO: we can most likely remove this, after putting it in context init */
		BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_TX_ORIGIN, 1);
		OUT_RING(chan, 0);
	}
	nvfx->relocs_needed &=~ NVFX_RELOCATE_FRAMEBUFFER;
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
		OUT_RELOC(chan, var.bo, RING_3D(pfx##_3D_DMA_##name, 1), rt_flags, 0, 0); \
		OUT_RELOC(chan, var.bo, 0, \
			rt_flags | NOUVEAU_BO_OR, \
			chan->vram->handle, chan->gart->handle); \
		OUT_RELOC(chan, var.bo, RING_3D(pfx##_3D_##name##_OFFSET, 1), rt_flags, 0, 0); \
		OUT_RELOC(chan, var.bo, \
			var.offset, rt_flags | NOUVEAU_BO_LOW, \
			0, 0); \
	}

#define DO(pfx, num) DO_(nvfx->hw_rt[num], pfx, COLOR##num)
	DO(NV30, 0);
	DO(NV30, 1);
	DO(NV40, 2);
	DO(NV40, 3);

	DO_(nvfx->hw_zeta, NV30, ZETA);
	nvfx->relocs_needed &=~ NVFX_RELOCATE_FRAMEBUFFER;
}
