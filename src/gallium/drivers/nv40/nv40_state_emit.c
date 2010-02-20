#include "nv40_context.h"
#include "nvfx_state.h"
#include "draw/draw_context.h"

static struct nvfx_state_entry *render_states[] = {
	&nv40_state_framebuffer,
	&nv40_state_rasterizer,
	&nv40_state_scissor,
	&nv40_state_stipple,
	&nv40_state_fragprog,
	&nv40_state_fragtex,
	&nv40_state_vertprog,
	&nv40_state_blend,
	&nv40_state_blend_colour,
	&nv40_state_zsa,
	&nv40_state_sr,
	&nv40_state_viewport,
	&nv40_state_vbo,
	NULL
};

static struct nvfx_state_entry *swtnl_states[] = {
	&nv40_state_framebuffer,
	&nv40_state_rasterizer,
	&nv40_state_scissor,
	&nv40_state_stipple,
	&nv40_state_fragprog,
	&nv40_state_fragtex,
	&nv40_state_vertprog,
	&nv40_state_blend,
	&nv40_state_blend_colour,
	&nv40_state_zsa,
	&nv40_state_sr,
	&nv40_state_viewport,
	&nv40_state_vtxfmt,
	NULL
};

static void
nv40_state_do_validate(struct nvfx_context *nvfx,
		       struct nvfx_state_entry **states)
{
	while (*states) {
		struct nvfx_state_entry *e = *states;

		if (nvfx->dirty & e->dirty.pipe) {
			if (e->validate(nvfx))
				nvfx->state.dirty |= (1ULL << e->dirty.hw);
		}

		states++;
	}
	nvfx->dirty = 0;
}

void
nv40_state_emit(struct nvfx_context *nvfx)
{
	struct nvfx_state *state = &nvfx->state;
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	unsigned i;
	uint64_t states;

	/* XXX: race conditions
	 */
	if (nvfx != screen->cur_ctx) {
		for (i = 0; i < NVFX_STATE_MAX; i++) {
			if (state->hw[i] && screen->state[i] != state->hw[i])
				state->dirty |= (1ULL << i);
		}

		screen->cur_ctx = nvfx;
	}

	for (i = 0, states = state->dirty; states; i++) {
		if (!(states & (1ULL << i)))
			continue;
		so_ref (state->hw[i], &nvfx->screen->state[i]);
		if (state->hw[i])
			so_emit(chan, nvfx->screen->state[i]);
		states &= ~(1ULL << i);
	}

	if (state->dirty & ((1ULL << NVFX_STATE_FRAGPROG) |
			    (1ULL << NVFX_STATE_FRAGTEX0))) {
		BEGIN_RING(chan, eng3d, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (chan, 2);
		BEGIN_RING(chan, eng3d, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (chan, 1);
	}

	state->dirty = 0;
}

void
nv40_state_flush_notify(struct nouveau_channel *chan)
{
	struct nvfx_context *nvfx = chan->user_private;
	struct nvfx_state *state = &nvfx->state;
	unsigned i, samplers;

	so_emit_reloc_markers(chan, state->hw[NVFX_STATE_FB]);
	for (i = 0, samplers = state->fp_samplers; i < 16 && samplers; i++) {
		if (!(samplers & (1 << i)))
			continue;
		so_emit_reloc_markers(chan,
				      state->hw[NVFX_STATE_FRAGTEX0+i]);
		samplers &= ~(1ULL << i);
	}
	so_emit_reloc_markers(chan, state->hw[NVFX_STATE_FRAGPROG]);
	if (state->hw[NVFX_STATE_VTXBUF] && nvfx->render_mode == HW)
		so_emit_reloc_markers(chan, state->hw[NVFX_STATE_VTXBUF]);
}

boolean
nv40_state_validate(struct nvfx_context *nvfx)
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
		nvfx->pipe.flush(&nvfx->pipe, 0, NULL);
		nvfx->dirty |= (NVFX_NEW_VIEWPORT |
				NVFX_NEW_VERTPROG |
				NVFX_NEW_ARRAYS);
		nvfx->render_mode = HW;
	}

	nv40_state_do_validate(nvfx, render_states);
	if (nvfx->fallback_swtnl || nvfx->fallback_swrast)
		return FALSE;
	
	if (was_sw)
		NOUVEAU_ERR("swtnl->hw\n");

	return TRUE;
}

boolean
nv40_state_validate_swtnl(struct nvfx_context *nvfx)
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

	nv40_state_do_validate(nvfx, swtnl_states);
	if (nvfx->fallback_swrast) {
		NOUVEAU_ERR("swtnl->swrast 0x%08x\n", nvfx->fallback_swrast);
		return FALSE;
	}

	nvfx->draw_dirty = 0;
	return TRUE;
}

