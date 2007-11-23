#include "nv40_context.h"
#include "nv40_dma.h"
#include "nv40_state.h"

static INLINE void
nv40_state_update_fragprog(struct nv40_context *nv40)
{
	struct pipe_context *pipe = (struct pipe_context *)nv40;
	struct nv40_fragment_program *fp = nv40->fragprog.current;
	float *map;
	int i;

	if (!fp->translated)
		nv40_fragprog_translate(nv40, fp);

	if (fp->num_consts) {
		map = pipe->winsys->buffer_map(pipe->winsys,
					       nv40->fragprog.constant_buf,
					       PIPE_BUFFER_FLAG_READ);
		for (i = 0; i < fp->num_consts; i++) {
			uint pid = fp->consts[i].pipe_id;

			if (pid == -1)
				continue;

			if (!memcmp(&fp->insn[fp->consts[i].hw_id], &map[pid*4],
				    4 * sizeof(float)))
				continue;

			memcpy(&fp->insn[fp->consts[i].hw_id], &map[pid*4],
			       4 * sizeof(float));
			fp->on_hw = 0;
		}
		pipe->winsys->buffer_unmap(pipe->winsys,
					   nv40->fragprog.constant_buf);
	}
}

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
	if (nv40->dirty & NV40_NEW_FRAGPROG) {
		struct nv40_fragment_program *cur = nv40->fragprog.current;

		nv40_state_update_fragprog(nv40);
	
		if (cur->on_hw)
			nv40->dirty &= ~NV40_NEW_FRAGPROG;

		if (!cur->on_hw || cur != nv40->fragprog.active)
			nv40_fragprog_bind(nv40, cur);
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

	if (nv40->dirty & NV40_NEW_VERTPROG)
		nv40_vertprog_bind(nv40, nv40->vertprog.current);

	if (nv40->dirty & NV40_NEW_ARRAYS) {
		nv40_vbo_arrays_update(nv40);
		nv40->dirty &= ~NV40_NEW_ARRAYS;
	}
}

