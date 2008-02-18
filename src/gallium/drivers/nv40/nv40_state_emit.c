#include "nv40_context.h"
#include "nv40_state.h"

/* Emit relocs for every referenced buffer.
 *
 * This is to ensure the bufmgr has an accurate idea of how
 * the buffer is used.  These relocs appear in the push buffer as
 * NOPs, and will only be turned into state changes if a buffer
 * actually moves.
 */
static void
nv40_state_emit_dummy_relocs(struct nv40_context *nv40)
{
	unsigned i;	
	
	so_emit_reloc_markers(nv40->nvws, nv40->so_framebuffer);
	for (i = 0; i < 16; i++) {
		if (!(nv40->fp_samplers & (1 << i)))
			continue;
		so_emit_reloc_markers(nv40->nvws, nv40->so_fragtex[i]);
	}
	so_emit_reloc_markers(nv40->nvws, nv40->fragprog.active->so);
}

static boolean
nv40_state_clip_validate(struct nv40_context *nv40)
{
	if (nv40->pipe_state.clip.nr)
		nv40->fallback |= NV40_FALLBACK_TNL;
	return FALSE;
}

static struct nv40_state_entry *render_states[] = {
	&nv40_state_clip,
	&nv40_state_scissor,
	&nv40_state_stipple,
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
				nv40->hw_dirty |= e->dirty.hw;
		}

		states++;
	}

	if (nv40->fallback & NV40_FALLBACK_TNL &&
	    !(last_fallback & NV40_FALLBACK_TNL)) {
		NOUVEAU_ERR("XXX: hwtnl->swtnl\n");
	} else
	if (last_fallback & NV40_FALLBACK_TNL &&
	    !(nv40->fallback & NV40_FALLBACK_TNL)) {
		NOUVEAU_ERR("XXX: swtnl->hwtnl\n");
	}
}

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
	nv40_state_validate(nv40);

	if (nv40->dirty & NV40_NEW_FB)
		so_emit(nv40->nvws, nv40->so_framebuffer);

	if (nv40->dirty & NV40_NEW_BLEND)
		so_emit(nv40->nvws, nv40->so_blend);

	if (nv40->dirty & NV40_NEW_RAST)
		so_emit(nv40->nvws, nv40->so_rast);

	if (nv40->dirty & NV40_NEW_ZSA)
		so_emit(nv40->nvws, nv40->so_zsa);

	if (nv40->dirty & NV40_NEW_BCOL)
		so_emit(nv40->nvws, nv40->so_bcol);

	if (nv40->hw_dirty & NV40_NEW_SCISSOR) {
		so_emit(nv40->nvws, nv40->state.scissor.so);
		nv40->hw_dirty &= ~NV40_NEW_SCISSOR;
	}

	if (nv40->dirty & NV40_NEW_VIEWPORT)
		so_emit(nv40->nvws, nv40->so_viewport);

	if (nv40->hw_dirty & NV40_NEW_STIPPLE) {
		so_emit(nv40->nvws, nv40->state.stipple.so);
		nv40->hw_dirty &= ~NV40_NEW_STIPPLE;
	}

	if (nv40->dirty & NV40_NEW_FRAGPROG) {
		nv40_fragprog_bind(nv40, nv40->fragprog.current);
		/*XXX: clear NV40_NEW_FRAGPROG if no new program uploaded */
	}

	if (nv40->dirty_samplers || (nv40->dirty & NV40_NEW_FRAGPROG)) {
		nv40_fragtex_bind(nv40);

		BEGIN_RING(curie, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (2);
		BEGIN_RING(curie, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (1);
		nv40->dirty &= ~NV40_NEW_FRAGPROG;
	}

	if (nv40->dirty & NV40_NEW_VERTPROG) {
		nv40_vertprog_bind(nv40, nv40->vertprog.current);
		nv40->dirty &= ~NV40_NEW_VERTPROG;
	}

	nv40->dirty_samplers = 0;
	nv40->dirty = 0;

	nv40_state_emit_dummy_relocs(nv40);
}

