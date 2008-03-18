#include "nv30_context.h"
#include "nv30_state.h"

void
nv30_emit_hw_state(struct nv30_context *nv30)
{
	int i;

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

	/* Emit relocs for every referenced buffer.
	 * This is to ensure the bufmgr has an accurate idea of how
	 * the buffer is used.  This isn't very efficient, but we don't
	 * seem to take a significant performance hit.  Will be improved
	 * at some point.  Vertex arrays are emitted by nv30_vbo.c
	 */

	/* Render targets */
	if (nv30->rt_enable & NV34TCL_RT_ENABLE_COLOR0) {
		BEGIN_RING(rankine, NV34TCL_DMA_COLOR0, 1);
		OUT_RELOCo(nv30->rt[0], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(rankine, NV34TCL_COLOR0_OFFSET, 1);
		OUT_RELOCl(nv30->rt[0], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (nv30->rt_enable & NV34TCL_RT_ENABLE_COLOR1) {
		BEGIN_RING(rankine, NV34TCL_DMA_COLOR1, 1);
		OUT_RELOCo(nv30->rt[1], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(rankine, NV34TCL_COLOR1_OFFSET, 1);
		OUT_RELOCl(nv30->rt[1], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	}

	if (nv30->zeta) {
		BEGIN_RING(rankine, NV34TCL_DMA_ZETA, 1);
		OUT_RELOCo(nv30->zeta, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(rankine, NV34TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(nv30->zeta, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		/* XXX allocate LMA */
/*		BEGIN_RING(rankine, NV34TCL_LMA_DEPTH_OFFSET, 1);
		OUT_RING(0);*/
	}

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

