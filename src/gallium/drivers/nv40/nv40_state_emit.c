#include "nv40_context.h"
#include "nv40_state.h"
#include "draw/draw_context.h"

static struct nv40_state_entry *render_states[] = {
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

static struct nv40_state_entry *swtnl_states[] = {
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
nv40_state_do_validate(struct nv40_context *nv40,
		       struct nv40_state_entry **states)
{
	while (*states) {
		struct nv40_state_entry *e = *states;

		if (nv40->dirty & e->dirty.pipe) {
			if (e->validate(nv40))
				nv40->state.dirty |= (1ULL << e->dirty.hw);
		}

		states++;
	}
	nv40->dirty = 0;
}

void
nv40_state_emit(struct nv40_context *nv40)
{
	struct nv40_state *state = &nv40->state;
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;
	unsigned i;
	uint64_t states;

	/* XXX: race conditions
	 */
	if (nv40 != screen->cur_ctx) {
		for (i = 0; i < NV40_STATE_MAX; i++) {
			if (state->hw[i] && screen->state[i] != state->hw[i])
				state->dirty |= (1ULL << i);
		}

		screen->cur_ctx = nv40;
	}

	for (i = 0, states = state->dirty; states; i++) {
		if (!(states & (1ULL << i)))
			continue;
		so_ref (state->hw[i], &nv40->screen->state[i]);
		if (state->hw[i])
			so_emit(chan, nv40->screen->state[i]);
		states &= ~(1ULL << i);
	}

	if (state->dirty & ((1ULL << NV40_STATE_FRAGPROG) |
			    (1ULL << NV40_STATE_FRAGTEX0))) {
		BEGIN_RING(chan, curie, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (chan, 2);
		BEGIN_RING(chan, curie, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (chan, 1);
	}

	state->dirty = 0;
}

void
nv40_state_flush_notify(struct nouveau_channel *chan)
{
	struct nv40_context *nv40 = chan->user_private;
	struct nv40_state *state = &nv40->state;
	unsigned i, samplers;

	so_emit_reloc_markers(chan, state->hw[NV40_STATE_FB]);
	for (i = 0, samplers = state->fp_samplers; i < 16 && samplers; i++) {
		if (!(samplers & (1 << i)))
			continue;
		so_emit_reloc_markers(chan,
				      state->hw[NV40_STATE_FRAGTEX0+i]);
		samplers &= ~(1ULL << i);
	}
	so_emit_reloc_markers(chan, state->hw[NV40_STATE_FRAGPROG]);
	if (state->hw[NV40_STATE_VTXBUF] && nv40->render_mode == HW)
		so_emit_reloc_markers(chan, state->hw[NV40_STATE_VTXBUF]);
}

boolean
nv40_state_validate(struct nv40_context *nv40)
{
	boolean was_sw = nv40->fallback_swtnl ? TRUE : FALSE;

	if (nv40->render_mode != HW) {
		/* Don't even bother trying to go back to hw if none
		 * of the states that caused swtnl previously have changed.
		 */
		if ((nv40->fallback_swtnl & nv40->dirty)
				!= nv40->fallback_swtnl)
			return FALSE;

		/* Attempt to go to hwtnl again */
		nv40->pipe.flush(&nv40->pipe, 0, NULL);
		nv40->dirty |= (NV40_NEW_VIEWPORT |
				NV40_NEW_VERTPROG |
				NV40_NEW_ARRAYS);
		nv40->render_mode = HW;
	}

	nv40_state_do_validate(nv40, render_states);
	if (nv40->fallback_swtnl || nv40->fallback_swrast)
		return FALSE;
	
	if (was_sw)
		NOUVEAU_ERR("swtnl->hw\n");

	return TRUE;
}

boolean
nv40_state_validate_swtnl(struct nv40_context *nv40)
{
	struct draw_context *draw = nv40->draw;

	/* Setup for swtnl */
	if (nv40->render_mode == HW) {
		NOUVEAU_ERR("hw->swtnl 0x%08x\n", nv40->fallback_swtnl);
		nv40->pipe.flush(&nv40->pipe, 0, NULL);
		nv40->dirty |= (NV40_NEW_VIEWPORT |
				NV40_NEW_VERTPROG |
				NV40_NEW_ARRAYS);
		nv40->render_mode = SWTNL;
	}

	if (nv40->draw_dirty & NV40_NEW_VERTPROG)
		draw_bind_vertex_shader(draw, nv40->vertprog->draw);

	if (nv40->draw_dirty & NV40_NEW_RAST)
		draw_set_rasterizer_state(draw, &nv40->rasterizer->pipe, nv40->rasterizer);

	if (nv40->draw_dirty & NV40_NEW_UCP)
		draw_set_clip_state(draw, &nv40->clip);

	if (nv40->draw_dirty & NV40_NEW_VIEWPORT)
		draw_set_viewport_state(draw, &nv40->viewport);

	if (nv40->draw_dirty & NV40_NEW_ARRAYS) {
		draw_set_vertex_buffers(draw, nv40->vtxbuf_nr, nv40->vtxbuf);
		draw_set_vertex_elements(draw, nv40->vtxelt_nr, nv40->vtxelt);	
	}

	nv40_state_do_validate(nv40, swtnl_states);
	if (nv40->fallback_swrast) {
		NOUVEAU_ERR("swtnl->swrast 0x%08x\n", nv40->fallback_swrast);
		return FALSE;
	}

	nv40->draw_dirty = 0;
	return TRUE;
}

