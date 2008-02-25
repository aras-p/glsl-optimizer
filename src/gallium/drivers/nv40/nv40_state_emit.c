#include "nv40_context.h"
#include "nv40_state.h"

static struct nv40_state_entry *render_states[] = {
	&nv40_state_framebuffer,
	&nv40_state_rasterizer,
	&nv40_state_clip,
	&nv40_state_scissor,
	&nv40_state_stipple,
	&nv40_state_fragprog,
	&nv40_state_fragtex,
	&nv40_state_vertprog,
	&nv40_state_blend,
	&nv40_state_blend_colour,
	&nv40_state_zsa,
	&nv40_state_viewport,
	&nv40_state_vbo,
	NULL
};

static void
nv40_state_validate(struct nv40_context *nv40)
{
	struct nv40_state_entry **states = render_states;
	unsigned last_fallback;

	last_fallback = nv40->fallback;
	nv40->fallback = 0;

	while (*states) {
		struct nv40_state_entry *e = *states;

		if (nv40->dirty & e->dirty.pipe) {
			if (e->validate(nv40))
				nv40->state.dirty |= (1ULL << e->dirty.hw);
		}

		states++;
	}
	nv40->dirty = 0;

	if (nv40->fallback & NV40_FALLBACK_TNL &&
	    !(last_fallback & NV40_FALLBACK_TNL)) {
		NOUVEAU_ERR("XXX: hwtnl->swtnl\n");
	} else
	if (last_fallback & NV40_FALLBACK_TNL &&
	    !(nv40->fallback & NV40_FALLBACK_TNL)) {
		NOUVEAU_ERR("XXX: swtnl->hwtnl\n");
	}

	if (nv40->fallback & NV40_FALLBACK_RAST &&
	    !(last_fallback & NV40_FALLBACK_RAST)) {
		NOUVEAU_ERR("XXX: hwrast->swrast\n");
	} else
	if (last_fallback & NV40_FALLBACK_RAST &&
	    !(nv40->fallback & NV40_FALLBACK_RAST)) {
		NOUVEAU_ERR("XXX: swrast->hwrast\n");
	}
}

static void
nv40_state_emit(struct nv40_context *nv40)
{
	struct nv40_state *state = &nv40->state;
	unsigned i, samplers;

	while (state->dirty) {
		unsigned idx = ffsll(state->dirty) - 1;

		so_ref (state->hw[idx], &nv40->hw->state[idx]);
		so_emit(nv40->nvws, nv40->hw->state[idx]);
		state->dirty &= ~(1ULL << idx);
	}

	so_emit_reloc_markers(nv40->nvws, state->hw[NV40_STATE_FB]);
	for (i = 0, samplers = state->fp_samplers; i < 16 && samplers; i++) {
		so_emit_reloc_markers(nv40->nvws,
				      state->hw[NV40_STATE_FRAGTEX0+i]);
		samplers &= ~(1ULL << i);
	}
	so_emit_reloc_markers(nv40->nvws, state->hw[NV40_STATE_FRAGPROG]);
	so_emit_reloc_markers(nv40->nvws, state->hw[NV40_STATE_VTXBUF]);
}

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
	nv40_state_validate(nv40);
	nv40_state_emit(nv40);

	BEGIN_RING(curie, NV40TCL_TEX_CACHE_CTL, 1);
	OUT_RING  (2);
	BEGIN_RING(curie, NV40TCL_TEX_CACHE_CTL, 1);
	OUT_RING  (1);
}

