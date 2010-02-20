#include "nv30_context.h"
#include "nvfx_state.h"

static struct nvfx_state_entry *render_states[] = {
	&nv30_state_framebuffer,
	&nv30_state_rasterizer,
	&nv30_state_scissor,
	&nv30_state_stipple,
	&nv30_state_fragprog,
	&nv30_state_fragtex,
	&nv30_state_vertprog,
	&nv30_state_blend,
	&nv30_state_blend_colour,
	&nv30_state_zsa,
	&nv30_state_sr,
	&nv30_state_viewport,
	&nv30_state_vbo,
	NULL
};

static void
nv30_state_do_validate(struct nvfx_context *nvfx,
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
nv30_state_emit(struct nvfx_context *nvfx)
{
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nvfx_state *state = &nvfx->state;
	struct nvfx_screen *screen = nvfx->screen;
	unsigned i;
	uint64_t states;

	/* XXX: racy!
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

	state->dirty = 0;
}

void
nv30_state_flush_notify(struct nouveau_channel *chan)
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
	if (state->hw[NVFX_STATE_VTXBUF] /*&& nvfx->render_mode == HW*/)
		so_emit_reloc_markers(chan, state->hw[NVFX_STATE_VTXBUF]);
}

boolean
nv30_state_validate(struct nvfx_context *nvfx)
{
#if 0
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
#endif
	nv30_state_do_validate(nvfx, render_states);
#if 0
	if (nvfx->fallback_swtnl || nvfx->fallback_swrast)
		return FALSE;
	
	if (was_sw)
		NOUVEAU_ERR("swtnl->hw\n");
#endif
	return TRUE;
}
