#include "nv30_context.h"
#include "nv30_state.h"

static struct nv30_state_entry *render_states[] = {
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
nv30_state_do_validate(struct nv30_context *nv30,
		       struct nv30_state_entry **states)
{
	while (*states) {
		struct nv30_state_entry *e = *states;

		if (nv30->dirty & e->dirty.pipe) {
			if (e->validate(nv30)) {
				nv30->state.dirty |= (1ULL << e->dirty.hw);
			}
		}

		states++;
	}
	nv30->dirty = 0;
}

void
nv30_state_emit(struct nv30_context *nv30)
{
	struct nouveau_channel *chan = nv30->screen->base.channel;
	struct nv30_state *state = &nv30->state;
	struct nv30_screen *screen = nv30->screen;
	unsigned i;
	uint64_t states;

	/* XXX: racy!
	 */
	if (nv30 != screen->cur_ctx) {
		for (i = 0; i < NV30_STATE_MAX; i++) {
			if (state->hw[i] && screen->state[i] != state->hw[i])
				state->dirty |= (1ULL << i);
		}

		screen->cur_ctx = nv30;
	}

	for (i = 0, states = state->dirty; states; i++) {
		if (!(states & (1ULL << i)))
			continue;
		so_ref (state->hw[i], &nv30->screen->state[i]);
		if (state->hw[i])
			so_emit(chan, nv30->screen->state[i]);
		states &= ~(1ULL << i);
	}

	state->dirty = 0;
}

void
nv30_state_flush_notify(struct nouveau_channel *chan)
{
	struct nv30_context *nv30 = chan->user_private;
	struct nv30_state *state = &nv30->state;
	unsigned i, samplers;

	so_emit_reloc_markers(chan, state->hw[NV30_STATE_FB]);
	for (i = 0, samplers = state->fp_samplers; i < 16 && samplers; i++) {
		if (!(samplers & (1 << i)))
			continue;
		so_emit_reloc_markers(chan,
				      state->hw[NV30_STATE_FRAGTEX0+i]);
		samplers &= ~(1ULL << i);
	}
	so_emit_reloc_markers(chan, state->hw[NV30_STATE_FRAGPROG]);
	if (state->hw[NV30_STATE_VTXBUF] /*&& nv30->render_mode == HW*/)
		so_emit_reloc_markers(chan, state->hw[NV30_STATE_VTXBUF]);
}

boolean
nv30_state_validate(struct nv30_context *nv30)
{
#if 0
	boolean was_sw = nv30->fallback_swtnl ? TRUE : FALSE;

	if (nv30->render_mode != HW) {
		/* Don't even bother trying to go back to hw if none
		 * of the states that caused swtnl previously have changed.
		 */
		if ((nv30->fallback_swtnl & nv30->dirty)
				!= nv30->fallback_swtnl)
			return FALSE;

		/* Attempt to go to hwtnl again */
		nv30->pipe.flush(&nv30->pipe, 0, NULL);
		nv30->dirty |= (NV30_NEW_VIEWPORT |
				NV30_NEW_VERTPROG |
				NV30_NEW_ARRAYS);
		nv30->render_mode = HW;
	}
#endif
	nv30_state_do_validate(nv30, render_states);
#if 0
	if (nv30->fallback_swtnl || nv30->fallback_swrast)
		return FALSE;
	
	if (was_sw)
		NOUVEAU_ERR("swtnl->hw\n");
#endif
	return TRUE;
}
