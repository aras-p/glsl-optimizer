#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_util.h"

#include "nv40_context.h"
#include "nv40_state.h"

#include "pipe/nouveau/nouveau_channel.h"
#include "pipe/nouveau/nouveau_pushbuf.h"

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

static boolean
nv40_vbo_static_attrib(struct nv40_context *nv40, int attrib,
		       struct pipe_vertex_element *ve,
		       struct pipe_vertex_buffer *vb)
{
	struct pipe_winsys *ws = nv40->pipe.winsys;
	int type, ncomp;
	void *map;

	type = nv40_vbo_type(ve->src_format);
	ncomp = nv40_vbo_ncomp(ve->src_format);

	map  = ws->buffer_map(ws, vb->buffer, PIPE_BUFFER_USAGE_CPU_READ);
	map += vb->buffer_offset + ve->src_offset;

	switch (type) {
	case NV40TCL_VTXFMT_TYPE_FLOAT:
	{
		float *v = map;

		BEGIN_RING(curie, NV40TCL_VTX_ATTR_4F_X(attrib), 4);
		switch (ncomp) {
		case 4:
			OUT_RINGf(v[0]);
			OUT_RINGf(v[1]);
			OUT_RINGf(v[2]);
			OUT_RINGf(v[3]);
			break;
		case 3:
			OUT_RINGf(v[0]);
			OUT_RINGf(v[1]);
			OUT_RINGf(v[2]);
			OUT_RINGf(1.0);
			break;
		case 2:
			OUT_RINGf(v[0]);
			OUT_RINGf(v[1]);
			OUT_RINGf(0.0);
			OUT_RINGf(1.0);
			break;
		case 1:
			OUT_RINGf(v[0]);
			OUT_RINGf(0.0);
			OUT_RINGf(0.0);
			OUT_RINGf(1.0);
			break;
		default:
			ws->buffer_unmap(ws, vb->buffer);
			return FALSE;
		}
	}
		break;
	default:
		ws->buffer_unmap(ws, vb->buffer);
		return FALSE;
	}

	ws->buffer_unmap(ws, vb->buffer);

	return TRUE;
}

static void
nv40_vbo_arrays_update(struct nv40_context *nv40)
{
	struct nv40_vertex_program *vp = nv40->vertprog.active;
	uint32_t inputs, vtxfmt[16];
	int hw, num_hw;

	nv40->vb_enable = 0;

	inputs = vp->ir;
	for (hw = 0; hw < 16 && inputs; hw++) {
		if (inputs & (1 << hw)) {
			num_hw = hw;
			inputs &= ~(1 << hw);
		}
	}
	num_hw++;

	inputs = vp->ir;
	for (hw = 0; hw < num_hw; hw++) {
		struct pipe_vertex_element *ve;
		struct pipe_vertex_buffer *vb;

		if (!(inputs & (1 << hw))) {
			vtxfmt[hw] = NV40TCL_VTXFMT_TYPE_FLOAT;
			continue;
		}

		ve = &nv40->vtxelt[hw];
		vb = &nv40->vtxbuf[ve->vertex_buffer_index];

		if (vb->pitch == 0) {
			vtxfmt[hw] = NV40TCL_VTXFMT_TYPE_FLOAT;
			if (nv40_vbo_static_attrib(nv40, hw, ve, vb) == TRUE)
				continue;
		}

		nv40->vb_enable |= (1 << hw);
		nv40->vb[hw].delta = vb->buffer_offset + ve->src_offset;
		nv40->vb[hw].buffer = vb->buffer;

		vtxfmt[hw] = ((vb->pitch << NV40TCL_VTXFMT_STRIDE_SHIFT) |
			      (nv40_vbo_ncomp(ve->src_format) <<
			       NV40TCL_VTXFMT_SIZE_SHIFT) |
			      nv40_vbo_type(ve->src_format));
	}

	BEGIN_RING(curie, NV40TCL_VTXFMT(0), num_hw);
	OUT_RINGp (vtxfmt, num_hw);
}

static boolean
nv40_vbo_validate_state(struct nv40_context *nv40,
			struct pipe_buffer *ib, unsigned ib_format)
{
	unsigned inputs;

	nv40_emit_hw_state(nv40);

	if (nv40->dirty & NV40_NEW_ARRAYS) {
		nv40_vbo_arrays_update(nv40);
		nv40->dirty &= ~NV40_NEW_ARRAYS;
	}

	inputs = nv40->vb_enable;
	while (inputs) {
		unsigned a = ffs(inputs) - 1;

		inputs &= ~(1 << a);

		BEGIN_RING(curie, NV40TCL_VTXBUF_ADDRESS(a), 1);
		OUT_RELOC (nv40->vb[a].buffer, nv40->vb[a].delta,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_LOW |
			   NOUVEAU_BO_OR | NOUVEAU_BO_RD, 0,
			   NV40TCL_VTXBUF_ADDRESS_DMA1);
	}

	if (ib) {
		BEGIN_RING(curie, NV40TCL_IDXBUF_ADDRESS, 2);
		OUT_RELOCl(ib, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			   NOUVEAU_BO_RD);
		OUT_RELOCd(ib, ib_format, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			   NOUVEAU_BO_RD | NOUVEAU_BO_OR,
			   0, NV40TCL_IDXBUF_FORMAT_DMA1);
	}

	BEGIN_RING(curie, 0x1710, 1);
	OUT_RING  (0); /* vtx cache flush */

	return TRUE;
}

boolean
nv40_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	unsigned nr;

	assert(nv40_vbo_validate_state(nv40, NULL, 0));

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	nr = (count & 0xff);
	if (nr) {
		BEGIN_RING(curie, NV40TCL_VB_VERTEX_BATCH, 1);
		OUT_RING  (((nr - 1) << 24) | start);
		start += nr;
	}

	nr = count >> 8;
	while (nr) {
		unsigned push = nr > 2047 ? 2047 : nr;

		nr -= push;

		BEGIN_RING_NI(curie, NV40TCL_VB_VERTEX_BATCH, push);
		while (push--) {
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

static boolean
nv40_draw_elements_inline(struct pipe_context *pipe,
			  struct pipe_buffer *ib, unsigned ib_size,
			  unsigned mode, unsigned start, unsigned count)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct pipe_winsys *ws = pipe->winsys;
	void *map;

	assert(nv40_vbo_validate_state(nv40, NULL, 0));

	map = ws->buffer_map(ws, ib, PIPE_BUFFER_USAGE_CPU_READ);
	if (!ib)
		assert(0);

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	switch (ib_size) {
	case 1:
		nv40_draw_elements_u08(nv40, map, start, count);
		break;
	case 2:
		nv40_draw_elements_u16(nv40, map, start, count);
		break;
	case 4:
		nv40_draw_elements_u32(nv40, map, start, count);
		break;
	default:
		assert(0);
		break;
	}

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (0);

	ws->buffer_unmap(ws, ib);

	return TRUE;
}

static boolean
nv40_draw_elements_vbo(struct pipe_context *pipe,
		       struct pipe_buffer *ib, unsigned ib_size,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	unsigned nr, type;

	switch (ib_size) {
	case 2:
		type = NV40TCL_IDXBUF_FORMAT_TYPE_U16;
		break;
	case 4:
		type = NV40TCL_IDXBUF_FORMAT_TYPE_U32;
		break;
	default:
		assert(0);
	}

	assert(nv40_vbo_validate_state(nv40, ib, type));

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	nr = (count & 0xff);
	if (nr) {
		BEGIN_RING(curie, NV40TCL_VB_INDEX_BATCH, 1);
		OUT_RING  (((nr - 1) << 24) | start);
		start += nr;
	}

	nr = count >> 8;
	while (nr) {
		unsigned push = nr > 2047 ? 2047 : nr;

		nr -= push;

		BEGIN_RING_NI(curie, NV40TCL_VB_INDEX_BATCH, push);
		while (push--) {
			OUT_RING(((0x100 - 1) << 24) | start);
			start += 0x100;
		}
	}

	BEGIN_RING(curie, NV40TCL_BEGIN_END, 1);
	OUT_RING  (0);

	return TRUE;
}

boolean
nv40_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	if (indexSize != 1) {
		nv40_draw_elements_vbo(pipe, indexBuffer, indexSize,
				       mode, start, count);
	} else {
		nv40_draw_elements_inline(pipe, indexBuffer, indexSize,
					  mode, start, count);
	}

	pipe->flush(pipe, 0);
	return TRUE;
}


