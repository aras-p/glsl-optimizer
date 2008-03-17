#include "nv10_context.h"
#include "nv10_state.h"

void
nv10_emit_hw_state(struct nv10_context *nv10)
{
	int i;

	if (nv10->dirty & NV10_NEW_FRAGPROG) {
		nv10_fragprog_bind(nv10, nv10->fragprog.current);
		/*XXX: clear NV10_NEW_FRAGPROG if no new program uploaded */
	}

	if (nv10->dirty_samplers || (nv10->dirty & NV10_NEW_FRAGPROG)) {
		nv10_fragtex_bind(nv10);
		nv10->dirty &= ~NV10_NEW_FRAGPROG;
	}

	if (nv10->dirty & NV10_NEW_VERTPROG) {
		//nv10_vertprog_bind(nv10, nv10->vertprog.current);
		nv10->dirty &= ~NV10_NEW_VERTPROG;
	}

	if (nv10->dirty & NV10_NEW_VBO) {
		
	}

	nv10->dirty_samplers = 0;

	/* Emit relocs for every referenced buffer.
	 * This is to ensure the bufmgr has an accurate idea of how
	 * the buffer is used.  This isn't very efficient, but we don't
	 * seem to take a significant performance hit.  Will be improved
	 * at some point.  Vertex arrays are emitted by nv10_vbo.c
	 */

	/* Render target */
// XXX figre out who's who for NV10TCL_DMA_* and fill accordingly
//	BEGIN_RING(celsius, NV10TCL_DMA_COLOR0, 1);
//	OUT_RELOCo(nv10->rt[0], NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(celsius, NV10TCL_COLOR_OFFSET, 1);
	OUT_RELOCl(nv10->rt[0], 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	if (nv10->zeta) {
// XXX
//		BEGIN_RING(celsius, NV10TCL_DMA_ZETA, 1);
//		OUT_RELOCo(nv10->zeta, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		BEGIN_RING(celsius, NV10TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(nv10->zeta, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
		/* XXX for when we allocate LMA on nv17 */
/*		BEGIN_RING(celsius, NV10TCL_LMA_DEPTH_OFFSET, 1);
		OUT_RELOCl(nv10->zeta+lma_offset);*/
	}

	/* Texture images */
	for (i = 0; i < 2; i++) {
		if (!(nv10->fp_samplers & (1 << i)))
			continue;
		BEGIN_RING(celsius, NV10TCL_TX_OFFSET(i), 1);
		OUT_RELOCl(nv10->tex[i].buffer, 0, NOUVEAU_BO_VRAM |
			   NOUVEAU_BO_GART | NOUVEAU_BO_RD);
		BEGIN_RING(celsius, NV10TCL_TX_FORMAT(i), 1);
		OUT_RELOCd(nv10->tex[i].buffer, nv10->tex[i].format,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			   NOUVEAU_BO_OR, NV10TCL_TX_FORMAT_DMA0,
			   NV10TCL_TX_FORMAT_DMA1);
	}
}

