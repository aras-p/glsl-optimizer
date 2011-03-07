#include "nvfx_context.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"
#include "draw/draw_context.h"

void
nvfx_state_viewport_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct pipe_viewport_state *vpt = &nvfx->viewport;

	if(nvfx->render_mode == HW) {
		BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_TRANSLATE_X, 8);
		OUT_RINGf(chan, vpt->translate[0]);
		OUT_RINGf(chan, vpt->translate[1]);
		OUT_RINGf(chan, vpt->translate[2]);
		OUT_RINGf(chan, vpt->translate[3]);
		OUT_RINGf(chan, vpt->scale[0]);
		OUT_RINGf(chan, vpt->scale[1]);
		OUT_RINGf(chan, vpt->scale[2]);
		OUT_RINGf(chan, vpt->scale[3]);
		BEGIN_RING(chan, eng3d, 0x1d78, 1);
		OUT_RING(chan, 1);
	} else {
		BEGIN_RING(chan, eng3d, NV30_3D_VIEWPORT_TRANSLATE_X, 8);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 0.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RINGf(chan, 1.0f);
		OUT_RINGf(chan, 1.0f);
		BEGIN_RING(chan, eng3d, 0x1d78, 1);
		OUT_RING(chan, nvfx->is_nv4x ? 0x110 : 1);
	}
}

void
nvfx_state_scissor_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct pipe_rasterizer_state *rast = &nvfx->rasterizer->pipe;
	struct pipe_scissor_state *s = &nvfx->scissor;

	if ((rast->scissor == 0 && nvfx->state.scissor_enabled == 0))
		return;
	nvfx->state.scissor_enabled = rast->scissor;

	BEGIN_RING(chan, eng3d, NV30_3D_SCISSOR_HORIZ, 2);
	if (nvfx->state.scissor_enabled) {
		OUT_RING(chan, ((s->maxx - s->minx) << 16) | s->minx);
		OUT_RING(chan, ((s->maxy - s->miny) << 16) | s->miny);
	} else {
		OUT_RING(chan, 4096 << 16);
		OUT_RING(chan, 4096 << 16);
	}
}

void
nvfx_state_sr_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct pipe_stencil_ref *sr = &nvfx->stencil_ref;

	BEGIN_RING(chan, eng3d, NV30_3D_STENCIL_FUNC_REF(0), 1);
	OUT_RING(chan, sr->ref_value[0]);
	BEGIN_RING(chan, eng3d, NV30_3D_STENCIL_FUNC_REF(1), 1);
	OUT_RING(chan, sr->ref_value[1]);
}

void
nvfx_state_blend_colour_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct pipe_blend_color *bcol = &nvfx->blend_colour;

	BEGIN_RING(chan, eng3d, NV30_3D_BLEND_COLOR, 1);
	OUT_RING(chan, ((float_to_ubyte(bcol->color[3]) << 24) |
		       (float_to_ubyte(bcol->color[0]) << 16) |
		       (float_to_ubyte(bcol->color[1]) <<  8) |
		       (float_to_ubyte(bcol->color[2]) <<  0)));
}

void
nvfx_state_stipple_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;

	BEGIN_RING(chan, eng3d, NV30_3D_POLYGON_STIPPLE_PATTERN(0), 32);
	OUT_RINGp(chan, nvfx->stipple, 32);
}

static void
nvfx_coord_conventions_validate(struct nvfx_context* nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned value = nvfx->hw_fragprog->coord_conventions;
	if(value & NV30_3D_COORD_CONVENTIONS_ORIGIN_INVERTED)
		value |= nvfx->framebuffer.height << NV30_3D_COORD_CONVENTIONS_HEIGHT__SHIFT;

	BEGIN_RING(chan, eng3d, NV30_3D_COORD_CONVENTIONS, 1);
	OUT_RING(chan, value);
}

static void
nvfx_ucp_validate(struct nvfx_context* nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned enables[7] =
	{
			0,
			NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE0,
			NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE0 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE1,
			NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE0 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE1 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE2,
			NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE0 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE1 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE2 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE3,
			NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE0 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE1 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE2 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE3 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE4,
			NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE0 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE1 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE2 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE3 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE4 | NV30_3D_VP_CLIP_PLANES_ENABLE_PLANE5,
	};

	if(!nvfx->use_vp_clipping)
	{
		BEGIN_RING(chan, eng3d, NV30_3D_VP_CLIP_PLANES_ENABLE, 1);
		OUT_RING(chan, 0);

		BEGIN_RING(chan, eng3d, NV30_3D_VP_CLIP_PLANE(0, 0),
			   nvfx->clip.nr * 4);
		OUT_RINGp(chan, &nvfx->clip.ucp[0][0], nvfx->clip.nr * 4);
	}

	BEGIN_RING(chan, eng3d, NV30_3D_VP_CLIP_PLANES_ENABLE, 1);
	OUT_RING(chan, enables[nvfx->clip.nr]);
}

static void
nvfx_vertprog_ucp_validate(struct nvfx_context* nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned i;
	struct nvfx_vertex_program* vp = nvfx->hw_vertprog;
	if(nvfx->clip.nr != vp->clip_nr)
	{
		unsigned idx;

		/* remove last instruction bit */
		if(vp->clip_nr >= 0)
		{
			idx = vp->nr_insns - 7 + vp->clip_nr;
			BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_FROM_ID, 1);
			OUT_RING(chan,  vp->exec->start + idx);
			BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_INST(0), 4);
			OUT_RINGp (chan, vp->insns[idx].data, 4);
		}

		 /* set last instruction bit */
		idx = vp->nr_insns - 7 + nvfx->clip.nr;
		BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_FROM_ID, 1);
		OUT_RING(chan,  vp->exec->start + idx);
		BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_INST(0), 4);
		OUT_RINGp(chan, vp->insns[idx].data, 3);
		OUT_RING(chan, vp->insns[idx].data[3] | 1);
		vp->clip_nr = nvfx->clip.nr;
	}

	// TODO: only do this for the ones changed
	for(i = 0; i < nvfx->clip.nr; ++i)
	{
		BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_CONST_ID, 5);
		OUT_RING(chan, vp->data->start + i);
		OUT_RINGp (chan, nvfx->clip.ucp[i], 4);
	}
}

static boolean
nvfx_state_validate_common(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned dirty;
	unsigned still_dirty = 0;
	int new_fb_mode = -1; /* 1 = all swizzled, 0 = make all linear */
	boolean flush_tex_cache = FALSE;
	unsigned render_temps;

	if(nvfx != nvfx->screen->cur_ctx)
	{
		nvfx->dirty = ~0;
		nvfx->hw_vtxelt_nr = 16;
		nvfx->hw_pointsprite_control = -1;
		nvfx->hw_vp_output = -1;
		nvfx->screen->cur_ctx = nvfx;
		nvfx->relocs_needed = NVFX_RELOCATE_ALL;
	}

	/* These can trigger use the of 3D engine to copy temporaries.
	 * That will recurse here and thus dirty all 3D state, so we need to this before anything else, and in a loop..
	 * This converges to having clean temps, then binding both fragtexes and framebuffers.
	 */
	while(nvfx->dirty & (NVFX_NEW_FB | NVFX_NEW_SAMPLER))
	{
		if(nvfx->dirty & NVFX_NEW_SAMPLER)
		{
			nvfx->dirty &=~ NVFX_NEW_SAMPLER;
			nvfx_fragtex_validate(nvfx);

			// TODO: only set this if really necessary
			flush_tex_cache = TRUE;
		}

		if(nvfx->dirty & NVFX_NEW_FB)
		{
			nvfx->dirty &=~ NVFX_NEW_FB;
			new_fb_mode = nvfx_framebuffer_prepare(nvfx);

			// TODO: make sure this doesn't happen, i.e. fbs have matching formats
			assert(new_fb_mode >= 0);
		}
	}

	dirty = nvfx->dirty;

	if(nvfx->render_mode == HW)
	{
		if(dirty & (NVFX_NEW_VERTPROG | NVFX_NEW_VERTCONST))
		{
			if(!nvfx_vertprog_validate(nvfx))
				return FALSE;
		}

		if(dirty & NVFX_NEW_ARRAYS)
		{
			if(!nvfx_vbo_validate(nvfx))
				return FALSE;
		}

		if(dirty & NVFX_NEW_INDEX)
		{
			if(nvfx->use_index_buffer)
				nvfx_idxbuf_validate(nvfx);
			else
				still_dirty = NVFX_NEW_INDEX;
		}
	}
	else
	{
		if(dirty & NVFX_NEW_VERTPROG) {
			assert(nvfx_vertprog_validate(nvfx));
			nvfx_vbo_swtnl_validate(nvfx);
		}
	}

	if(dirty & NVFX_NEW_RAST)
		sb_emit(chan, nvfx->rasterizer->sb, nvfx->rasterizer->sb_len);

	if(dirty & NVFX_NEW_SCISSOR)
		nvfx_state_scissor_validate(nvfx);

	if(dirty & NVFX_NEW_STIPPLE)
		nvfx_state_stipple_validate(nvfx);

       if(nvfx->dirty & NVFX_NEW_UCP)
	       nvfx_ucp_validate(nvfx);

	if(nvfx->use_vp_clipping && (nvfx->dirty & (NVFX_NEW_UCP | NVFX_NEW_VERTPROG)))
		nvfx_vertprog_ucp_validate(nvfx);

	if(dirty & (NVFX_NEW_FRAGPROG | NVFX_NEW_FRAGCONST | NVFX_NEW_VERTPROG | NVFX_NEW_SPRITE))
	{
		nvfx_fragprog_validate(nvfx);
		if(dirty & NVFX_NEW_FRAGPROG)
			flush_tex_cache = TRUE; // TODO: do we need this?
	}

	if(nvfx->is_nv4x)
	{
		unsigned vp_output = nvfx->hw_vertprog->or | nvfx->hw_fragprog->or;
		vp_output |= (1 << (nvfx->clip.nr + 6)) - (1 << 6);

		if(vp_output != nvfx->hw_vp_output)
		{
			BEGIN_RING(chan, eng3d, NV40_3D_VP_RESULT_EN, 1);
			OUT_RING(chan, vp_output);
			nvfx->hw_vp_output = vp_output;
		}
	}

	if(new_fb_mode >= 0)
		nvfx_framebuffer_validate(nvfx, new_fb_mode);

	if(dirty & NVFX_NEW_BLEND)
		sb_emit(chan, nvfx->blend->sb, nvfx->blend->sb_len);

	if(dirty & NVFX_NEW_BCOL)
		nvfx_state_blend_colour_validate(nvfx);

	if(dirty & NVFX_NEW_ZSA)
		sb_emit(chan, nvfx->zsa->sb, nvfx->zsa->sb_len);

	if(dirty & NVFX_NEW_SR)
		nvfx_state_sr_validate(nvfx);

/* All these dependencies are wrong, but otherwise
   etracer, neverball, foobillard, glest totally misrender
   TODO: find the right fix
*/
	if(dirty & (NVFX_NEW_VIEWPORT | NVFX_NEW_RAST | NVFX_NEW_ZSA) || (new_fb_mode >= 0))
	{
		nvfx_state_viewport_validate(nvfx);
	}

	if(dirty & NVFX_NEW_ZSA || (new_fb_mode >= 0))
	{
		BEGIN_RING(chan, eng3d, NV30_3D_DEPTH_WRITE_ENABLE, 2);
		OUT_RING(chan, nvfx->framebuffer.zsbuf && nvfx->zsa->pipe.depth.writemask);
	        OUT_RING(chan, nvfx->framebuffer.zsbuf && nvfx->zsa->pipe.depth.enabled);
	}

	if((new_fb_mode >= 0) || (dirty & NVFX_NEW_FRAGPROG))
		nvfx_coord_conventions_validate(nvfx);

	if(flush_tex_cache)
	{
		// TODO: what about nv30?
		if(nvfx->is_nv4x)
		{
			BEGIN_RING(chan, eng3d, NV40_3D_TEX_CACHE_CTL, 1);
			OUT_RING(chan, 2);
			BEGIN_RING(chan, eng3d, NV40_3D_TEX_CACHE_CTL, 1);
			OUT_RING(chan, 1);
		}
	}

	nvfx->dirty = dirty & still_dirty;

	render_temps = nvfx->state.render_temps;
	if(render_temps)
	{
		for(int i = 0; i < nvfx->framebuffer.nr_cbufs; ++i)
		{
			if(render_temps & (1 << i)) {
				assert(((struct nvfx_surface*)nvfx->framebuffer.cbufs[i])->temp);
				util_dirty_surface_set_dirty(nvfx_surface_get_dirty_surfaces(nvfx->framebuffer.cbufs[i]),
						(struct util_dirty_surface*)nvfx->framebuffer.cbufs[i]);
			}
		}

		if(render_temps & 0x80) {
			assert(((struct nvfx_surface*)nvfx->framebuffer.zsbuf)->temp);
			util_dirty_surface_set_dirty(nvfx_surface_get_dirty_surfaces(nvfx->framebuffer.zsbuf),
					(struct util_dirty_surface*)nvfx->framebuffer.zsbuf);
		}
	}

	return TRUE;
}

inline void
nvfx_state_relocate(struct nvfx_context *nvfx, unsigned relocs)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	/* we need to ensure there is enough space to output relocations in one go */
	const unsigned max_relocs = 0
	      + 16 /* vertex buffers, incl. dma flag */
	      + 2 /* index buffer plus format+dma flag */
	      + 2 * 5 /* 4 cbufs + zsbuf, plus dma objects */
	      + 2 * 16 /* fragment textures plus format+dma flag */
	      + 2 * 4 /* vertex textures plus format+dma flag */
	      + 1 /* fragprog incl dma flag */
	      ;

	MARK_RING(chan, max_relocs * 2, max_relocs * 2);

	if(relocs & NVFX_RELOCATE_FRAMEBUFFER)
		nvfx_framebuffer_relocate(nvfx);
	if(relocs & NVFX_RELOCATE_FRAGTEX)
		nvfx_fragtex_relocate(nvfx);
	if(relocs & NVFX_RELOCATE_FRAGPROG)
		nvfx_fragprog_relocate(nvfx);
	if(relocs & NVFX_RELOCATE_VTXBUF)
		nvfx_vbo_relocate(nvfx);
	if(relocs & NVFX_RELOCATE_IDXBUF)
		nvfx_idxbuf_relocate(nvfx);
}

boolean
nvfx_state_validate(struct nvfx_context *nvfx)
{
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

	return TRUE;
}

boolean
nvfx_state_validate_swtnl(struct nvfx_context *nvfx)
{
	struct draw_context *draw = nvfx->draw;

	/* Setup for swtnl */
	if (nvfx->render_mode == HW) {
		static boolean warned = FALSE;
		if(!warned) {
			NOUVEAU_ERR("hw->swtnl 0x%08x\n", nvfx->fallback_swtnl);
			warned = TRUE;
		}
                nvfx->pipe.flush(&nvfx->pipe, NULL);
		nvfx->dirty |= (NVFX_NEW_VIEWPORT |
				NVFX_NEW_VERTPROG |
				NVFX_NEW_ARRAYS);
		nvfx->render_mode = SWTNL;
	}

	if (nvfx->draw_dirty & NVFX_NEW_VERTPROG) {
		if(!nvfx->vertprog->draw_vs)
			nvfx->vertprog->draw_vs = draw_create_vertex_shader(draw, &nvfx->vertprog->pipe);
		draw_bind_vertex_shader(draw, nvfx->vertprog->draw_vs);
	}

	if (nvfx->draw_dirty & NVFX_NEW_RAST)
           draw_set_rasterizer_state(draw, &nvfx->rasterizer->pipe,
                                     nvfx->rasterizer);

	if (nvfx->draw_dirty & NVFX_NEW_UCP)
		draw_set_clip_state(draw, &nvfx->clip);

	if (nvfx->draw_dirty & NVFX_NEW_VIEWPORT)
		draw_set_viewport_state(draw, &nvfx->viewport);

	if (nvfx->draw_dirty & NVFX_NEW_ARRAYS) {
		draw_set_vertex_buffers(draw, nvfx->vtxbuf_nr, nvfx->vtxbuf);
		draw_set_vertex_elements(draw, nvfx->vtxelt->num_elements, nvfx->vtxelt->pipe);
	}

	if (nvfx->draw_dirty & NVFX_NEW_INDEX)
		draw_set_index_buffer(draw, &nvfx->idxbuf);

	nvfx_state_validate_common(nvfx);

	nvfx->draw_dirty = 0;
	return TRUE;
}
