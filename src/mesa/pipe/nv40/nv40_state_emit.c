#include "nv40_context.h"
#include "nv40_dma.h"
#include "nv40_state.h"

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
	if (nv40->dirty & NV40_NEW_FRAGPROG) {
		nv40_fragprog_bind(nv40, nv40->fragprog.current);
		/*XXX: clear NV40_NEW_FRAGPROG if no now program uploaded */
	}

	if (nv40->dirty & NV40_NEW_TEXTURE)
		nv40_state_tex_update(nv40);

	if (nv40->dirty & (NV40_NEW_TEXTURE | NV40_NEW_FRAGPROG)) {
		BEGIN_RING(curie, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (2);
		BEGIN_RING(curie, NV40TCL_TEX_CACHE_CTL, 1);
		OUT_RING  (1);
		nv40->dirty &= ~(NV40_NEW_TEXTURE | NV40_NEW_FRAGPROG);
	}

	if (nv40->dirty & NV40_NEW_VERTPROG) {
		nv40_vertprog_bind(nv40, nv40->vertprog.current);
		nv40->dirty &= ~NV40_NEW_VERTPROG;
	}
}

