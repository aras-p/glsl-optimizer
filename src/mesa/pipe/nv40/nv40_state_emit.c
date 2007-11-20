#include "nv40_context.h"
#include "nv40_dma.h"
#include "nv40_state.h"

static INLINE void
nv40_state_update_fragprog(struct nv40_context *nv40)
{
	struct pipe_context *pipe = (struct pipe_context *)nv40;
	struct nv40_fragment_program *fp = nv40->fragprog.fp;
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

static INLINE void
nv40_state_update_vertprog(struct nv40_context *nv40)
{
	struct pipe_context *pipe = (struct pipe_context *)nv40;
	struct nv40_vertex_program *vp = nv40->vertprog.vp;
	float *map;
	int i, force_consts = 0;

	if (!nv40->vertprog.vp->translated)
		nv40_vertprog_translate(nv40, nv40->vertprog.vp);

	if (nv40->vertprog.vp != nv40->vertprog.active_vp)
		force_consts = 1;

	if (vp->num_consts) {
		map = pipe->winsys->buffer_map(pipe->winsys,
					       nv40->vertprog.constant_buf,
					       PIPE_BUFFER_FLAG_READ);
		for (i = 0; i < vp->num_consts; i++) {
			uint pid = vp->consts[i].pipe_id;

			if (pid >= 0) {
				if (!force_consts &&
				    !memcmp(vp->consts[i].value, &map[pid*4],
					    4 * sizeof(float)))
					continue;
				memcpy(vp->consts[i].value, &map[pid*4],
				       4 * sizeof(float));
			}

			BEGIN_RING(curie, NV40TCL_VP_UPLOAD_CONST_ID, 5);
			OUT_RING  (vp->consts[i].hw_id);
			OUT_RINGp ((uint32_t *)vp->consts[i].value, 4);
		}
		pipe->winsys->buffer_unmap(pipe->winsys,
					   nv40->vertprog.constant_buf);
	}
}

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
	if (nv40->dirty & NV40_NEW_FRAGPROG) {
		struct nv40_fragment_program *cur = nv40->fragprog.fp;

		nv40_state_update_fragprog(nv40);
	
		if (cur->on_hw)
			nv40->dirty &= ~NV40_NEW_FRAGPROG;

		if (!cur->on_hw || cur != nv40->fragprog.active_fp)
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

	if (nv40->dirty & NV40_NEW_VERTPROG) {
		nv40_state_update_vertprog(nv40);
		if (nv40->vertprog.vp != nv40->vertprog.active_vp)
			nv40_vertprog_bind(nv40, nv40->vertprog.vp);
		nv40->dirty &= ~NV40_NEW_VERTPROG;
	}

	if (nv40->dirty & NV40_NEW_ARRAYS) {
		nv40_vbo_arrays_update(nv40);
		nv40->dirty &= ~NV40_NEW_ARRAYS;
	}
}

