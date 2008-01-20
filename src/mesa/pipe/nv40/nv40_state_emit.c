#include "nv40_context.h"
#include "nv40_state.h"

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
	int i;

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

	/* Emit relocs for every referenced buffer.
	 * This is to ensure the bufmgr has an accurate idea of how
	 * the buffer is used.  This isn't very efficient, but we don't
	 * seem to take a significant performance hit.  Will be improved
	 * at some point.  Vertex arrays are emitted by nv40_vbo.c
	 */

	/* Render targets */
	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR0) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR0, 1);
		OUT_RELOCo(nv40->rt[0], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR0_OFFSET, 1);
		OUT_RELOCl(nv40->rt[0], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR1) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR1, 1);
		OUT_RELOCo(nv40->rt[1], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR1_OFFSET, 1);
		OUT_RELOCl(nv40->rt[1], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR2, 1);
		OUT_RELOCo(nv40->rt[2], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR2_OFFSET, 1);
		OUT_RELOCl(nv40->rt[2], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
		BEGIN_RING(curie, NV40TCL_DMA_COLOR3, 1);
		OUT_RELOCo(nv40->rt[3], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_COLOR3_OFFSET, 1);
		OUT_RELOCl(nv40->rt[3], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (nv40->zeta) {
		BEGIN_RING(curie, NV40TCL_DMA_ZETA, 1);
		OUT_RELOCo(nv40->zeta, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(curie, NV40TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(nv40->zeta, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	/* Texture images */
	for (i = 0; i < 16; i++) {
		if (!(nv40->fp_samplers & (1 << i)))
			continue;
		BEGIN_RING(curie, NV40TCL_TEX_OFFSET(i), 2);
		OUT_RELOCl(nv40->tex[i].buffer, 0, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		OUT_RELOCd(nv40->tex[i].buffer, nv40->tex[i].format,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			   NOUVEAU_BO_OR, NV40TCL_TEX_FORMAT_DMA0,
			   NV40TCL_TEX_FORMAT_DMA1);
	}

	/* Fragment program */
	BEGIN_RING(curie, NV40TCL_FP_ADDRESS, 1);
	OUT_RELOC (nv40->fragprog.active->buffer, 0, NOUVEAU_BO_VRAM |
	           NOUVEAU_BO_GART | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
		   NOUVEAU_BO_OR, NV40TCL_FP_ADDRESS_DMA0,
		   NV40TCL_FP_ADDRESS_DMA1);
}

