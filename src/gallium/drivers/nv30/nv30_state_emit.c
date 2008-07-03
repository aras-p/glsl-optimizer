#include "nv30_context.h"
#include "nv30_state.h"

static struct nv30_state_entry *render_states[] = {
	&nv30_state_framebuffer,
	&nv30_state_rasterizer,
	&nv30_state_scissor,
	&nv30_state_blend,
	&nv30_state_blend_colour,
	&nv30_state_zsa,
	NULL
};

static void
nv30_state_do_validate(struct nv30_context *nv30,
		       struct nv30_state_entry **states)
{
	const struct pipe_framebuffer_state *fb = &nv30->framebuffer;
	unsigned i;

	for (i = 0; i < fb->num_cbufs; i++)
		fb->cbufs[i]->status = PIPE_SURFACE_STATUS_DEFINED;
	if (fb->zsbuf)
		fb->zsbuf->status = PIPE_SURFACE_STATUS_DEFINED;

	while (*states) {
		struct nv30_state_entry *e = *states;

		if (nv30->dirty & e->dirty.pipe) {
			if (e->validate(nv30)) {
				nv30->state.dirty |= (1ULL << e->dirty.hw);
			}
		}

		states++;
	}

/*	TODO: uncomment when finished converting
	nv30->dirty = 0;
*/
}

void
nv30_emit_hw_state(struct nv30_context *nv30)
{
	struct nv30_state *state = &nv30->state;
	unsigned i;
	uint64 states;

	for (i = 0, states = state->dirty; states; i++) {
		if (!(states & (1ULL << i)))
			continue;
		so_ref (state->hw[i], &nv30->screen->state[i]);
		if (state->hw[i])
			so_emit(nv30->nvws, nv30->screen->state[i]);
		states &= ~(1ULL << i);
	}

	if (nv30->dirty & NV30_NEW_FRAGPROG) {
		nv30_fragprog_bind(nv30, nv30->fragprog.current);
		/*XXX: clear NV30_NEW_FRAGPROG if no new program uploaded */
	}

	if (nv30->dirty_samplers || (nv30->dirty & NV30_NEW_FRAGPROG)) {
		nv30_fragtex_bind(nv30);
/*
		BEGIN_RING(rankine, NV34TCL_TX_CACHE_CTL, 1);
		OUT_RING  (2);
		BEGIN_RING(rankine, NV34TCL_TX_CACHE_CTL, 1);
		OUT_RING  (1);*/
		nv30->dirty &= ~NV30_NEW_FRAGPROG;
	}

	if (nv30->dirty & NV30_NEW_VERTPROG) {
		nv30_vertprog_bind(nv30, nv30->vertprog.current);
		nv30->dirty &= ~NV30_NEW_VERTPROG;
	}

	nv30->dirty_samplers = 0;

	so_emit_reloc_markers(nv30->nvws, state->hw[NV30_STATE_FB]);

	/* Texture images, emitted in nv30_fragtex_build */
#if 0
	for (i = 0; i < 16; i++) {
		if (!(nv30->fp_samplers & (1 << i)))
			continue;
		BEGIN_RING(rankine, NV34TCL_TX_OFFSET(i), 2);
		OUT_RELOCl(nv30->tex[i].buffer, 0, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCd(nv30->tex[i].buffer, nv30->tex[i].format,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			   NOUVEAU_BO_OR, NV34TCL_TX_FORMAT_DMA0,
			   NV34TCL_TX_FORMAT_DMA1);
	}
#endif

	/* Fragment program */
	BEGIN_RING(rankine, NV34TCL_FP_ACTIVE_PROGRAM, 1);
	OUT_RELOC (nv30->fragprog.active->buffer, 0, NOUVEAU_BO_VRAM |
	           NOUVEAU_BO_GART | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
		   NOUVEAU_BO_OR, NV34TCL_FP_ACTIVE_PROGRAM_DMA0,
		   NV34TCL_FP_ACTIVE_PROGRAM_DMA1);
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
