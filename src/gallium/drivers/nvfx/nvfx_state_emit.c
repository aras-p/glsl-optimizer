#include "nvfx_context.h"
#include "nvfx_state.h"
#include "draw/draw_context.h"

static boolean
nvfx_state_validate_common(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	unsigned dirty = nvfx->dirty;

	if(nvfx != nvfx->screen->cur_ctx)
		dirty = ~0;

	if(nvfx->render_mode == HW)
	{
		if(dirty & (NVFX_NEW_VERTPROG | NVFX_NEW_VERTCONST | NVFX_NEW_UCP))
		{
			if(!nvfx_vertprog_validate(nvfx))
				return FALSE;
		}

		if(dirty & (NVFX_NEW_ARRAYS))
		{
			if(!nvfx_vbo_validate(nvfx))
				return FALSE;
		}
	}
	else
	{
		/* TODO: this looks a bit misdesigned */
		if(dirty & (NVFX_NEW_VERTPROG | NVFX_NEW_UCP))
			nvfx_vertprog_validate(nvfx);

		if(dirty & (NVFX_NEW_ARRAYS | NVFX_NEW_FRAGPROG))
			nvfx_vtxfmt_validate(nvfx);
	}

	if(dirty & NVFX_NEW_FB)
		nvfx_state_framebuffer_validate(nvfx);

	if(dirty & NVFX_NEW_RAST)
		sb_emit(chan, nvfx->rasterizer->sb, nvfx->rasterizer->sb_len);

	if(dirty & NVFX_NEW_SCISSOR)
		nvfx_state_scissor_validate(nvfx);

	if(dirty & NVFX_NEW_STIPPLE)
		nvfx_state_stipple_validate(nvfx);

	if(dirty & (NVFX_NEW_FRAGPROG | NVFX_NEW_FRAGCONST))
		nvfx_fragprog_validate(nvfx);

	if(dirty & NVFX_NEW_SAMPLER)
		nvfx_fragtex_validate(nvfx);

	if(dirty & NVFX_NEW_BLEND)
		sb_emit(chan, nvfx->blend->sb, nvfx->blend->sb_len);

	if(dirty & NVFX_NEW_BCOL)
		nvfx_state_blend_colour_validate(nvfx);

	if(dirty & NVFX_NEW_ZSA)
		sb_emit(chan, nvfx->zsa->sb, nvfx->zsa->sb_len);

	if(dirty & NVFX_NEW_SR)
		nvfx_state_sr_validate(nvfx);

/* Having this depend on FB looks wrong, but it seems
   necessary to make this work on nv3x
   TODO: find the right fix
*/
	if(dirty & (NVFX_NEW_VIEWPORT | NVFX_NEW_FB))
		nvfx_state_viewport_validate(nvfx);

	/* TODO: could nv30 need this or something similar too? */
	if((dirty & (NVFX_NEW_FRAGPROG | NVFX_NEW_SAMPLER)) && nvfx->is_nv4x) {
		WAIT_RING(chan, 4);
		OUT_RING(chan, RING_3D(NV40TCL_TEX_CACHE_CTL, 1));
		OUT_RING(chan, 2);
		OUT_RING(chan, RING_3D(NV40TCL_TEX_CACHE_CTL, 1));
		OUT_RING(chan, 1);
	}
	nvfx->dirty = 0;
	return TRUE;
}

void
nvfx_state_emit(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	/* we need to ensure there is enough space to output relocations in one go */
	unsigned max_relocs = 0
	      + 16 /* vertex buffers, incl. dma flag */
	      + 2 /* index buffer plus format+dma flag */
	      + 2 * 5 /* 4 cbufs + zsbuf, plus dma objects */
	      + 2 * 16 /* fragment textures plus format+dma flag */
	      + 2 * 4 /* vertex textures plus format+dma flag */
	      + 1 /* fragprog incl dma flag */
	      ;
	MARK_RING(chan, max_relocs * 2, max_relocs * 2);
	nvfx_state_relocate(nvfx);
}

void
nvfx_state_relocate(struct nvfx_context *nvfx)
{
	nvfx_framebuffer_relocate(nvfx);
	nvfx_fragtex_relocate(nvfx);
	nvfx_fragprog_relocate(nvfx);
	if (nvfx->render_mode == HW)
		nvfx_vbo_relocate(nvfx);
}

boolean
nvfx_state_validate(struct nvfx_context *nvfx)
{
	boolean was_sw = nvfx->fallback_swtnl ? TRUE : FALSE;

	if (nvfx->render_mode != HW) {
		/* Don't even bother trying to go back to hw if none
		 * of the states that caused swtnl previously have changed.
		 */
		if ((nvfx->fallback_swtnl & nvfx->dirty)
				!= nvfx->fallback_swtnl)
			return FALSE;

		/* Attempt to go to hwtnl again */
		nvfx->dirty |= (NVFX_NEW_VIEWPORT |
				NVFX_NEW_VERTPROG |
				NVFX_NEW_ARRAYS);
		nvfx->render_mode = HW;
	}

	if(!nvfx_state_validate_common(nvfx))
		return FALSE;

	if (was_sw)
		NOUVEAU_ERR("swtnl->hw\n");

	return TRUE;
}

boolean
nvfx_state_validate_swtnl(struct nvfx_context *nvfx)
{
	struct draw_context *draw = nvfx->draw;

	/* Setup for swtnl */
	if (nvfx->render_mode == HW) {
		NOUVEAU_ERR("hw->swtnl 0x%08x\n", nvfx->fallback_swtnl);
		nvfx->pipe.flush(&nvfx->pipe, 0, NULL);
		nvfx->dirty |= (NVFX_NEW_VIEWPORT |
				NVFX_NEW_VERTPROG |
				NVFX_NEW_ARRAYS);
		nvfx->render_mode = SWTNL;
	}

	if (nvfx->draw_dirty & NVFX_NEW_VERTPROG)
		draw_bind_vertex_shader(draw, nvfx->vertprog->draw);

	if (nvfx->draw_dirty & NVFX_NEW_RAST)
		draw_set_rasterizer_state(draw, &nvfx->rasterizer->pipe);

	if (nvfx->draw_dirty & NVFX_NEW_UCP)
		draw_set_clip_state(draw, &nvfx->clip);

	if (nvfx->draw_dirty & NVFX_NEW_VIEWPORT)
		draw_set_viewport_state(draw, &nvfx->viewport);

	if (nvfx->draw_dirty & NVFX_NEW_ARRAYS) {
		draw_set_vertex_buffers(draw, nvfx->vtxbuf_nr, nvfx->vtxbuf);
		draw_set_vertex_elements(draw, nvfx->vtxelt->num_elements, nvfx->vtxelt->pipe);
	}

	nvfx_state_validate_common(nvfx);

	nvfx->draw_dirty = 0;
	return TRUE;
}
