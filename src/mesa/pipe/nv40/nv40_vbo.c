#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_util.h"

#include "nv40_context.h"
#include "nv40_dma.h"
#include "nv40_state.h"

boolean
nv40_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	unsigned nr;

	if (nv40->dirty)
		nv40_emit_hw_state(nv40);

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	nr = (count & 0xff);
	if (nr) {
		BEGIN_RING(curie, NV40TCL_VB_VERTEX_BATCH, 1);
		OUT_RING  (((nr - 1) << 24) | start);
		start += nr;
	}

	/*XXX: large arrays (nr>2047) will blow up */
	nr = count >> 8;
	if (nr) {
		assert (nr <= 2047);

		BEGIN_RING_NI(curie, NV40TCL_VB_VERTEX_BATCH, nr);
		while (nr--) {
			OUT_RING(((0x100 - 1) << 24) | start);
			start += 0x100;
		}
	}

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (0);

	pipe->flush(pipe, 0);
	return TRUE;
}

static INLINE void
nv40_draw_elements_u08(struct nv40_context *nv40, void *ib,
		       unsigned start, unsigned count)
{
	uint8_t *elts = (uint8_t *)ib + start;
	int push, i;

	if (count & 1) {
		BEGIN_RING(curie, NV40TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (elts[0]);
		elts++; count--;
	}

	while (count) {
		push = MIN2(count, 2046);

		BEGIN_RING_NI(curie, NV40TCL_VB_ELEMENT_U16, push);
		for (i = 0; i < push; i+=2)
			OUT_RING((elts[i+1] << 16) | elts[i]);

		count -= push;
		elts  += push;
	}
}

static INLINE void
nv40_draw_elements_u16(struct nv40_context *nv40, void *ib,
		       unsigned start, unsigned count)
{
	uint16_t *elts = (uint16_t *)ib + start;
	int push, i;

	if (count & 1) {
		BEGIN_RING(curie, NV40TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (elts[0]);
		elts++; count--;
	}

	while (count) {
		push = MIN2(count, 2046);

		BEGIN_RING_NI(curie, NV40TCL_VB_ELEMENT_U16, push);
		for (i = 0; i < push; i+=2)
			OUT_RING((elts[i+1] << 16) | elts[i]);

		count -= push;
		elts  += push;
	}
}

static INLINE void
nv40_draw_elements_u32(struct nv40_context *nv40, void *ib,
		       unsigned start, unsigned count)
{
	uint32_t *elts = (uint32_t *)ib + start;
	int push;

	while (count) {
		push = MIN2(count, 2047);

		BEGIN_RING_NI(curie, NV40TCL_VB_ELEMENT_U32, push);
		OUT_RINGp    (elts, push);

		count -= push;
		elts  += push;
	}
}

boolean
nv40_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer_handle *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	void *ib;

	if (nv40->dirty)
		nv40_emit_hw_state(nv40);

	ib = pipe->winsys->buffer_map(pipe->winsys, indexBuffer,
				      PIPE_BUFFER_FLAG_READ);
	if (!ib) {
		NOUVEAU_ERR("Couldn't map index buffer!!\n");
		return FALSE;
	}

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	switch (indexSize) {
	case 1:
		nv40_draw_elements_u08(nv40, ib, start, count);
		break;
	case 2:
		nv40_draw_elements_u16(nv40, ib, start, count);
		break;
	case 4:
		nv40_draw_elements_u32(nv40, ib, start, count);
		break;
	default:
		NOUVEAU_ERR("unsupported elt size %d\n", indexSize);
		break;
	}

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (0);

	pipe->winsys->buffer_unmap(pipe->winsys, indexBuffer);
	pipe->flush(pipe, 0);
	return TRUE;
}

static INLINE int
nv40_vbo_ncomp(uint format)
{
	int ncomp = 0;

	if (pf_size_x(format)) ncomp++;
	if (pf_size_y(format)) ncomp++;
	if (pf_size_z(format)) ncomp++;
	if (pf_size_w(format)) ncomp++;

	return ncomp;
}

static INLINE int
nv40_vbo_type(uint format)
{
	switch (pf_type(format)) {
	case PIPE_FORMAT_TYPE_FLOAT:
		return NV40TCL_VTXFMT_TYPE_FLOAT;
	case PIPE_FORMAT_TYPE_UNORM:
		return NV40TCL_VTXFMT_TYPE_UBYTE;
	default:
		assert(0);
	}
}

void
nv40_vbo_arrays_update(struct nv40_context *nv40)
{
	struct nv40_vertex_program *vp = nv40->vertprog.active;
	uint32_t inputs, vtxfmt[16];
	int hw, num_hw;

	inputs = vp->ir;
	for (hw = 0; hw < 16 && inputs; hw++) {
		if (inputs & (1 << hw)) {
			num_hw = hw;
			inputs &= ~(1 << hw);
		}
	}
	num_hw++;

	inputs = vp->ir;
	BEGIN_RING(curie, NV40TCL_VTXBUF_ADDRESS(0), num_hw);
	for (hw = 0; hw < num_hw; hw++) {
		struct pipe_vertex_element *ve;
		struct pipe_vertex_buffer *vb;

		if (!(inputs & (1 << hw))) {
			OUT_RING(0);
			vtxfmt[hw] = NV40TCL_VTXFMT_TYPE_FLOAT;
			continue;
		}

		ve = &nv40->vtxelt[hw];
		vb = &nv40->vtxbuf[ve->vertex_buffer_index];

		OUT_RELOC(vb->buffer, vb->buffer_offset + ve->src_offset,
			  NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_LOW |
			  NOUVEAU_BO_OR | NOUVEAU_BO_RD, 0,
			  NV40TCL_VTXBUF_ADDRESS_DMA1);
		vtxfmt[hw] = ((vb->pitch << NV40TCL_VTXFMT_STRIDE_SHIFT) |
			      (nv40_vbo_ncomp(ve->src_format) <<
			       NV40TCL_VTXFMT_SIZE_SHIFT) |
			      nv40_vbo_type(ve->src_format));
	}

	BEGIN_RING(curie, 0x1710, 1);
	OUT_RING  (0); /* vtx cache flush */
	BEGIN_RING(curie, NV40TCL_VTXFMT(0), num_hw);
	OUT_RINGp (vtxfmt, num_hw);
}

