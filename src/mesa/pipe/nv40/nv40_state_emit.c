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
	unsigned rt_flags, tx_flags, fp_flags;
	int i;	
	
	rt_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR | NOUVEAU_BO_DUMMY;
	tx_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
		   NOUVEAU_BO_DUMMY;
	fp_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
		   NOUVEAU_BO_DUMMY;

	/* Render targets */
	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR0) {
		OUT_RELOCm(nv40->rt[0], rt_flags,
			   curie, NV40TCL_DMA_COLOR0, 1);
		OUT_RELOCo(nv40->rt[0], rt_flags);
		OUT_RELOCm(nv40->rt[0], rt_flags,
			   curie, NV40TCL_COLOR0_OFFSET, 1);
		OUT_RELOCl(nv40->rt[0], 0, rt_flags);
	}

	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR1) {
		OUT_RELOCm(nv40->rt[1], rt_flags,
			   curie, NV40TCL_DMA_COLOR1, 1);
		OUT_RELOCo(nv40->rt[1], rt_flags);
		OUT_RELOCm(nv40->rt[1], rt_flags,
			   curie, NV40TCL_COLOR1_OFFSET, 1);
		OUT_RELOCl(nv40->rt[1], 0, rt_flags);
	}

	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR2) {
		OUT_RELOCm(nv40->rt[2], rt_flags,
			   curie, NV40TCL_DMA_COLOR2, 1);
		OUT_RELOCo(nv40->rt[2], rt_flags);
		OUT_RELOCm(nv40->rt[2], rt_flags,
			   curie, NV40TCL_COLOR2_OFFSET, 1);
		OUT_RELOCl(nv40->rt[2], 0, rt_flags);
	}

	if (nv40->rt_enable & NV40TCL_RT_ENABLE_COLOR3) {
		OUT_RELOCm(nv40->rt[3], rt_flags,
			   curie, NV40TCL_DMA_COLOR3, 1);
		OUT_RELOCo(nv40->rt[3], rt_flags);
		OUT_RELOCm(nv40->rt[3], rt_flags,
			   curie, NV40TCL_COLOR3_OFFSET, 1);
		OUT_RELOCl(nv40->rt[3], 0, rt_flags);
	}

	if (nv40->zeta) {
		OUT_RELOCm(nv40->zeta, rt_flags, curie, NV40TCL_DMA_ZETA, 1);
		OUT_RELOCo(nv40->zeta, rt_flags);
		OUT_RELOCm(nv40->zeta, rt_flags, curie, NV40TCL_ZETA_OFFSET, 1);
		OUT_RELOCl(nv40->zeta, 0, rt_flags);
	}

	/* Texture images */
	for (i = 0; i < 16; i++) {
		if (!(nv40->fp_samplers & (1 << i)))
			continue;
		OUT_RELOCm(nv40->tex[i].buffer, tx_flags,
			   curie, NV40TCL_TEX_OFFSET(i), 2);
		OUT_RELOCl(nv40->tex[i].buffer, 0, tx_flags);
		OUT_RELOCd(nv40->tex[i].buffer, nv40->tex[i].format,
			   tx_flags | NOUVEAU_BO_OR, NV40TCL_TEX_FORMAT_DMA0,
			   NV40TCL_TEX_FORMAT_DMA1);
	}

	/* Fragment program */
	OUT_RELOCm(nv40->fragprog.active->buffer, fp_flags,
		   curie, NV40TCL_FP_ADDRESS, 1);
	OUT_RELOC (nv40->fragprog.active->buffer, 0,
		   fp_flags | NOUVEAU_BO_OR | NOUVEAU_BO_LOW,
		   NV40TCL_FP_ADDRESS_DMA0, NV40TCL_FP_ADDRESS_DMA1);
}

void
nv40_emit_hw_state(struct nv40_context *nv40)
{
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

	nv40_state_emit_dummy_relocs(nv40);
}

