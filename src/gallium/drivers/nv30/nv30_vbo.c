#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_util.h"

#include "nv30_context.h"
#include "nv30_state.h"

#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_pushbuf.h"

static INLINE int
nv30_vbo_ncomp(uint format)
{
	int ncomp = 0;

	if (pf_size_x(format)) ncomp++;
	if (pf_size_y(format)) ncomp++;
	if (pf_size_z(format)) ncomp++;
	if (pf_size_w(format)) ncomp++;

	return ncomp;
}

static INLINE int
nv30_vbo_type(uint format)
{
	switch (pf_type(format)) {
	case PIPE_FORMAT_TYPE_FLOAT:
		return NV34TCL_VERTEX_ARRAY_FORMAT_TYPE_FLOAT;
	case PIPE_FORMAT_TYPE_UNORM:
		return NV34TCL_VERTEX_ARRAY_FORMAT_TYPE_UBYTE;
	default:
		NOUVEAU_ERR("Unknown format 0x%08x\n", format);
		return NV40TCL_VTXFMT_TYPE_FLOAT;
	}
}

static boolean
nv30_vbo_static_attrib(struct nv30_context *nv30, int attrib,
		       struct pipe_vertex_element *ve,
		       struct pipe_vertex_buffer *vb)
{
	struct pipe_winsys *ws = nv30->pipe.winsys;
	int type, ncomp;
	void *map;

	type = nv30_vbo_type(ve->src_format);
	ncomp = nv30_vbo_ncomp(ve->src_format);

	map  = ws->buffer_map(ws, vb->buffer, PIPE_BUFFER_USAGE_CPU_READ);
	map += vb->buffer_offset + ve->src_offset;

	switch (type) {
	case NV34TCL_VERTEX_ARRAY_FORMAT_TYPE_FLOAT:
	{
		float *v = map;

		BEGIN_RING(rankine, NV34TCL_VERTEX_ATTR_4F_X(attrib), 4);
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
nv30_vbo_arrays_update(struct nv30_context *nv30)
{
	struct nv30_vertex_program *vp = nv30->vertprog.active;
	uint32_t inputs, vtxfmt[16];
	int hw, num_hw = 0;

	nv30->vb_enable = 0;

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
			vtxfmt[hw] = NV34TCL_VERTEX_ARRAY_FORMAT_TYPE_FLOAT;
			continue;
		}

		ve = &nv30->vtxelt[hw];
		vb = &nv30->vtxbuf[ve->vertex_buffer_index];

		if (vb->pitch == 0) {
			vtxfmt[hw] = NV34TCL_VERTEX_ARRAY_FORMAT_TYPE_FLOAT;
			if (nv30_vbo_static_attrib(nv30, hw, ve, vb) == TRUE)
				continue;
		}

		nv30->vb_enable |= (1 << hw);
		nv30->vb[hw].delta = vb->buffer_offset + ve->src_offset;
		nv30->vb[hw].buffer = vb->buffer;

		vtxfmt[hw] = ((vb->pitch << NV34TCL_VERTEX_ARRAY_FORMAT_STRIDE_SHIFT) |
			      (nv30_vbo_ncomp(ve->src_format) <<
			       NV34TCL_VERTEX_ARRAY_FORMAT_SIZE_SHIFT) |
			      nv30_vbo_type(ve->src_format));
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_ARRAY_FORMAT(0), num_hw);
	OUT_RINGp (vtxfmt, num_hw);
}

static boolean
nv30_vbo_validate_state(struct nv30_context *nv30,
			struct pipe_buffer *ib, unsigned ib_format)
{
	unsigned inputs;

	nv30_emit_hw_state(nv30);

	if (nv30->dirty & NV30_NEW_ARRAYS) {
		nv30_vbo_arrays_update(nv30);
		nv30->dirty &= ~NV30_NEW_ARRAYS;
	}

	inputs = nv30->vb_enable;
	while (inputs) {
		unsigned a = ffs(inputs) - 1;

		inputs &= ~(1 << a);

		BEGIN_RING(rankine, NV34TCL_VERTEX_BUFFER_ADDRESS(a), 1);
		OUT_RELOC (nv30->vb[a].buffer, nv30->vb[a].delta,
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_LOW |
			   NOUVEAU_BO_OR | NOUVEAU_BO_RD, 0,
			   NV34TCL_VERTEX_BUFFER_ADDRESS_DMA1);
	}

	if (ib) {
		BEGIN_RING(rankine, NV40TCL_IDXBUF_ADDRESS, 2);
		OUT_RELOCl(ib, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			   NOUVEAU_BO_RD);
		OUT_RELOCd(ib, ib_format, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			   NOUVEAU_BO_RD | NOUVEAU_BO_OR,
			   0, NV40TCL_IDXBUF_FORMAT_DMA1);
	}

	BEGIN_RING(rankine, 0x1710, 1);
	OUT_RING  (0); /* vtx cache flush */

	return TRUE;
}

boolean
nv30_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	unsigned nr;
	boolean ret;

	ret = nv30_vbo_validate_state(nv30, NULL, 0);
	if (!ret) {
		NOUVEAU_ERR("state validate failed\n");
		return FALSE;
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	nr = (count & 0xff);
	if (nr) {
		BEGIN_RING(rankine, NV34TCL_VB_VERTEX_BATCH, 1);
		OUT_RING  (((nr - 1) << 24) | start);
		start += nr;
	}

	nr = count >> 8;
	while (nr) {
		unsigned push = nr > 2047 ? 2047 : nr;

		nr -= push;

		BEGIN_RING_NI(rankine, NV34TCL_VB_VERTEX_BATCH, push);
		while (push--) {
			OUT_RING(((0x100 - 1) << 24) | start);
			start += 0x100;
		}
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (0);

	pipe->flush(pipe, 0, NULL);
	return TRUE;
}

static INLINE void
nv30_draw_elements_u08(struct nv30_context *nv30, void *ib,
		       unsigned start, unsigned count)
{
	uint8_t *elts = (uint8_t *)ib + start;
	int push, i;

	if (count & 1) {
		BEGIN_RING(rankine, NV34TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (elts[0]);
		elts++; count--;
	}

	while (count) {
		push = MIN2(count, 2047 * 2);

		BEGIN_RING_NI(rankine, NV34TCL_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING((elts[i+1] << 16) | elts[i]);

		count -= push;
		elts  += push;
	}
}

static INLINE void
nv30_draw_elements_u16(struct nv30_context *nv30, void *ib,
		       unsigned start, unsigned count)
{
	uint16_t *elts = (uint16_t *)ib + start;
	int push, i;

	if (count & 1) {
		BEGIN_RING(rankine, NV34TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (elts[0]);
		elts++; count--;
	}

	while (count) {
		push = MIN2(count, 2047 * 2);

		BEGIN_RING_NI(rankine, NV34TCL_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING((elts[i+1] << 16) | elts[i]);

		count -= push;
		elts  += push;
	}
}

static INLINE void
nv30_draw_elements_u32(struct nv30_context *nv30, void *ib,
		       unsigned start, unsigned count)
{
	uint32_t *elts = (uint32_t *)ib + start;
	int push;

	while (count) {
		push = MIN2(count, 2047);

		BEGIN_RING_NI(rankine, NV34TCL_VB_ELEMENT_U32, push);
		OUT_RINGp    (elts, push);

		count -= push;
		elts  += push;
	}
}

static boolean
nv30_draw_elements_inline(struct pipe_context *pipe,
			  struct pipe_buffer *ib, unsigned ib_size,
			  unsigned mode, unsigned start, unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct pipe_winsys *ws = pipe->winsys;
	boolean ret;
	void *map;

	ret =  nv30_vbo_validate_state(nv30, NULL, 0);
	if (!ret) {
		NOUVEAU_ERR("state validate failed\n");
		return FALSE;
	}

	map = ws->buffer_map(ws, ib, PIPE_BUFFER_USAGE_CPU_READ);
	if (!ib) {
		NOUVEAU_ERR("failed mapping ib\n");
		return FALSE;
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	switch (ib_size) {
	case 1:
		nv30_draw_elements_u08(nv30, map, start, count);
		break;
	case 2:
		nv30_draw_elements_u16(nv30, map, start, count);
		break;
	case 4:
		nv30_draw_elements_u32(nv30, map, start, count);
		break;
	default:
		NOUVEAU_ERR("invalid idxbuf fmt %d\n", ib_size);
		break;
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (0);

	ws->buffer_unmap(ws, ib);

	return TRUE;
}

static boolean
nv30_draw_elements_vbo(struct pipe_context *pipe,
		       struct pipe_buffer *ib, unsigned ib_size,
		       unsigned mode, unsigned start, unsigned count)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	unsigned nr, type;
	boolean ret;

	switch (ib_size) {
	case 2:
		type = NV40TCL_IDXBUF_FORMAT_TYPE_U16;
		break;
	case 4:
		type = NV40TCL_IDXBUF_FORMAT_TYPE_U32;
		break;
	default:
		NOUVEAU_ERR("invalid idxbuf fmt %d\n", ib_size);
		return FALSE;
	}

	ret = nv30_vbo_validate_state(nv30, ib, type);
	if (!ret) {
		NOUVEAU_ERR("failed state validation\n");
		return FALSE;
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (nvgl_primitive(mode));

	nr = (count & 0xff);
	if (nr) {
		BEGIN_RING(rankine, NV40TCL_VB_INDEX_BATCH, 1);
		OUT_RING  (((nr - 1) << 24) | start);
		start += nr;
	}

	nr = count >> 8;
	while (nr) {
		unsigned push = nr > 2047 ? 2047 : nr;

		nr -= push;

		BEGIN_RING_NI(rankine, NV40TCL_VB_INDEX_BATCH, push);
		while (push--) {
			OUT_RING(((0x100 - 1) << 24) | start);
			start += 0x100;
		}
	}

	BEGIN_RING(rankine, NV34TCL_VERTEX_BEGIN_END, 1);
	OUT_RING  (0);

	return TRUE;
}

boolean
nv30_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
/*	if (indexSize != 1) {
		nv30_draw_elements_vbo(pipe, indexBuffer, indexSize,
				       mode, start, count);
	} else */{
		nv30_draw_elements_inline(pipe, indexBuffer, indexSize,
					  mode, start, count);
	}

	pipe->flush(pipe, 0, NULL);
	return TRUE;
}


