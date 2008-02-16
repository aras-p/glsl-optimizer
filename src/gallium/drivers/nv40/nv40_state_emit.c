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
nv40_state_scissor_validate(struct nv40_context *nv40)
{
	struct pipe_rasterizer_state *rast = &nv40->rasterizer->pipe;
	struct pipe_scissor_state *s = &nv40->pipe_state.scissor;
	struct nouveau_stateobj *so;

	if (nv40->state.scissor &&
	    (rast->scissor == 0 && nv40->state.scissor_enabled == 0))
		return FALSE;

	so = so_new(3, 0);
	so_method(so, nv40->hw->curie, NV40TCL_SCISSOR_HORIZ, 2);
	if (rast->scissor) {
		so_data  (so, ((s->maxx - s->minx) << 16) | s->minx);
		so_data  (so, ((s->maxy - s->miny) << 16) | s->miny);
	} else {
		so_data  (so, 4096 << 16);
		so_data  (so, 4096 << 16);
	}

	so_ref(so, &nv40->state.scissor);
	so_ref(NULL, &so);
	return TRUE;
}

struct nv40_state_atom {
	boolean (*validate)(struct nv40_context *nv40);
	struct {
		unsigned pipe;
		unsigned hw;
	} dirty;
};

static struct nv40_state_atom states[] = {
	{
		.validate = nv40_state_scissor_validate,
		.dirty = {
			.pipe = NV40_NEW_SCISSOR | NV40_NEW_RAST,
			.hw = NV40_NEW_SCISSOR,
		}
	}
};

static void
nv40_state_validate(struct nv40_context *nv40)
{
	unsigned i;

	for (i = 0; i < sizeof(states) / sizeof(states[0]); i++) {
		if (nv40->dirty & states[i].dirty.pipe) {
			if (states[i].validate(nv40))
				nv40->hw_dirty |= states[i].dirty.hw;
		}
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
		so_emit(nv40->nvws, nv40->state.scissor);
		nv40->hw_dirty &= ~NV40_NEW_SCISSOR;
	}

	if (nv40->dirty & NV40_NEW_VIEWPORT)
		so_emit(nv40->nvws, nv40->so_viewport);

	if (nv40->dirty & NV40_NEW_STIPPLE)
		so_emit(nv40->nvws, nv40->so_stipple);

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

