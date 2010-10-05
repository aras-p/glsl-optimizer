/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_split_prim.h"

#include "nv50_context.h"
#include "nv50_resource.h"

struct instance {
	struct nouveau_bo *bo;
	unsigned delta;
	unsigned stride;
	unsigned step;
	unsigned divisor;
};

static void
instance_init(struct nv50_context *nv50, struct instance *a, unsigned first)
{
	int i;

	for (i = 0; i < nv50->vtxelt->num_elements; i++) {
		struct pipe_vertex_element *ve = &nv50->vtxelt->pipe[i];
		struct pipe_vertex_buffer *vb;

		a[i].divisor = ve->instance_divisor;
		if (a[i].divisor) {
			vb = &nv50->vtxbuf[ve->vertex_buffer_index];

			a[i].bo = nv50_resource(vb->buffer)->bo;
			a[i].stride = vb->stride;
			a[i].step = first % a[i].divisor;
			a[i].delta = vb->buffer_offset + ve->src_offset +
				     (first * a[i].stride);
		}
	}
}

static void
instance_step(struct nv50_context *nv50, struct instance *a)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	int i;

	for (i = 0; i < nv50->vtxelt->num_elements; i++) {
		if (!a[i].divisor)
			continue;

		BEGIN_RING(chan, tesla,
			   NV50TCL_VERTEX_ARRAY_START_HIGH(i), 2);
		OUT_RELOCh(chan, a[i].bo, a[i].delta, NOUVEAU_BO_RD |
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART);
		OUT_RELOCl(chan, a[i].bo, a[i].delta, NOUVEAU_BO_RD |
			   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART);
		if (++a[i].step == a[i].divisor) {
			a[i].step = 0;
			a[i].delta += a[i].stride;
		}
	}
}

static void
nv50_draw_arrays_instanced(struct pipe_context *pipe,
			   unsigned mode, unsigned start, unsigned count,
			   unsigned startInstance, unsigned instanceCount)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct instance a[16];
	unsigned prim = nv50_prim(mode);

	instance_init(nv50, a, startInstance);
	if (!nv50_state_validate(nv50, 10 + 16*3))
		return;

	if (nv50->vbo_fifo) {
		nv50_push_elements_instanced(pipe, NULL, 0, 0, mode, start,
					     count, startInstance,
					     instanceCount);
		return;
	}

	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 2);
	OUT_RING  (chan, NV50_CB_AUX | (24 << 8));
	OUT_RING  (chan, startInstance);
	while (instanceCount--) {
		if (AVAIL_RING(chan) < (7 + 16*3)) {
			FIRE_RING(chan);
			if (!nv50_state_validate(nv50, 7 + 16*3)) {
				assert(0);
				return;
			}
		}
		instance_step(nv50, a);

		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
		OUT_RING  (chan, prim);
		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BUFFER_FIRST, 2);
		OUT_RING  (chan, start);
		OUT_RING  (chan, count);
		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
		OUT_RING  (chan, 0);

		prim |= (1 << 28);
	}
}

struct inline_ctx {
	struct nv50_context *nv50;
	void *map;
};

static void
inline_elt08(void *priv, unsigned start, unsigned count)
{
	struct inline_ctx *ctx = priv;
	struct nouveau_grobj *tesla = ctx->nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;
	uint8_t *map = (uint8_t *)ctx->map + start;

	if (count & 1) {
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, map[0]);
		map++;
		count &= ~1;
	}

	count >>= 1;
	if (!count)
		return;

	BEGIN_RING_NI(chan, tesla, NV50TCL_VB_ELEMENT_U16, count);
	while (count--) {
		OUT_RING(chan, (map[1] << 16) | map[0]);
		map += 2;
	}
}

static void
inline_elt16(void *priv, unsigned start, unsigned count)
{
	struct inline_ctx *ctx = priv;
	struct nouveau_grobj *tesla = ctx->nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;
	uint16_t *map = (uint16_t *)ctx->map + start;

	if (count & 1) {
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, map[0]);
		count &= ~1;
		map++;
	}

	count >>= 1;
	if (!count)
		return;

	BEGIN_RING_NI(chan, tesla, NV50TCL_VB_ELEMENT_U16, count);
	while (count--) {
		OUT_RING(chan, (map[1] << 16) | map[0]);
		map += 2;
	}
}

static void
inline_elt32(void *priv, unsigned start, unsigned count)
{
	struct inline_ctx *ctx = priv;
	struct nouveau_grobj *tesla = ctx->nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;

	BEGIN_RING_NI(chan, tesla, NV50TCL_VB_ELEMENT_U32, count);
	OUT_RINGp    (chan, (uint32_t *)ctx->map + start, count);
}

static void
inline_edgeflag(void *priv, boolean enabled)
{
	struct inline_ctx *ctx = priv;
	struct nouveau_grobj *tesla = ctx->nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;

	BEGIN_RING(chan, tesla, NV50TCL_EDGEFLAG_ENABLE, 1);
	OUT_RING  (chan, enabled ? 1 : 0);
}

static void
nv50_draw_elements_inline(struct pipe_context *pipe,
			  struct pipe_resource *indexBuffer, unsigned indexSize,
			  unsigned mode, unsigned start, unsigned count,
			  unsigned startInstance, unsigned instanceCount)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_transfer *transfer;
	struct instance a[16];
	struct inline_ctx ctx;
	struct util_split_prim s;
	boolean nzi = FALSE;
	unsigned overhead;

	overhead = 16*3; /* potential instance adjustments */
	overhead += 4; /* Begin()/End() */
	overhead += 4; /* potential edgeflag disable/reenable */
	overhead += 3; /* potentially 3 VTX_ELT_U16/U32 packet headers */

	s.priv = &ctx;
	if (indexSize == 1)
		s.emit = inline_elt08;
	else
	if (indexSize == 2)
		s.emit = inline_elt16;
	else
		s.emit = inline_elt32;
	s.edge = inline_edgeflag;

	ctx.nv50 = nv50;
	ctx.map = pipe_buffer_map(pipe, indexBuffer, PIPE_TRANSFER_READ, &transfer);
	assert(ctx.map);
	if (!ctx.map)
		return;

	instance_init(nv50, a, startInstance);
	if (!nv50_state_validate(nv50, overhead + 6 + 3))
		return;

	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 2);
	OUT_RING  (chan, NV50_CB_AUX | (24 << 8));
	OUT_RING  (chan, startInstance);
	while (instanceCount--) {
		unsigned max_verts;
		boolean done;

		util_split_prim_init(&s, mode, start, count);
		do {
			if (AVAIL_RING(chan) < (overhead + 6)) {
				FIRE_RING(chan);
				if (!nv50_state_validate(nv50, (overhead + 6))) {
					assert(0);
					return;
				}
			}

			max_verts = AVAIL_RING(chan) - overhead;
			if (max_verts > 2047)
				max_verts = 2047;
			if (indexSize != 4)
				max_verts <<= 1;
			instance_step(nv50, a);

			BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
			OUT_RING  (chan, nv50_prim(s.mode) | (nzi ? (1<<28) : 0));
			done = util_split_prim_next(&s, max_verts);
			BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
			OUT_RING  (chan, 0);
		} while (!done);

		nzi = TRUE;
	}

	pipe_buffer_unmap(pipe, indexBuffer, transfer);
}

static void
nv50_draw_elements_instanced(struct pipe_context *pipe,
			     struct pipe_resource *indexBuffer,
			     unsigned indexSize, int indexBias,
			     unsigned mode, unsigned start, unsigned count,
			     unsigned startInstance, unsigned instanceCount)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct instance a[16];
	unsigned prim = nv50_prim(mode);

	instance_init(nv50, a, startInstance);
	if (!nv50_state_validate(nv50, 13 + 16*3))
		return;

	if (nv50->vbo_fifo) {
		nv50_push_elements_instanced(pipe, indexBuffer, indexSize,
					     indexBias, mode, start, count,
					     startInstance, instanceCount);
		return;
	}

	/* indices are uint32 internally, so large indexBias means negative */
	BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_BASE, 1);
	OUT_RING  (chan, indexBias);

	if (!nv50_resource_mapped_by_gpu(indexBuffer) || indexSize == 1) {
		nv50_draw_elements_inline(pipe, indexBuffer, indexSize,
					  mode, start, count, startInstance,
					  instanceCount);
		return;
	}

	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 2);
	OUT_RING  (chan, NV50_CB_AUX | (24 << 8));
	OUT_RING  (chan, startInstance);
	while (instanceCount--) {
		if (AVAIL_RING(chan) < (7 + 16*3)) {
			FIRE_RING(chan);
			if (!nv50_state_validate(nv50, 10 + 16*3)) {
				assert(0);
				return;
			}
		}
		instance_step(nv50, a);

		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
		OUT_RING  (chan, prim);
		if (indexSize == 4) {
			BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U32 | 0x30000, 0);
			OUT_RING  (chan, count);
			nouveau_pushbuf_submit(chan, 
					       nv50_resource(indexBuffer)->bo,
					       start << 2, count << 2);
		} else
		if (indexSize == 2) {
			unsigned vb_start = (start & ~1);
			unsigned vb_end = (start + count + 1) & ~1;
			unsigned dwords = (vb_end - vb_start) >> 1;

			BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U16_SETUP, 1);
			OUT_RING  (chan, ((start & 1) << 31) | count);
			BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U16 | 0x30000, 0);
			OUT_RING  (chan, dwords);
			nouveau_pushbuf_submit(chan,
					       nv50_resource(indexBuffer)->bo,
					       vb_start << 1, dwords << 2);
			BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U16_SETUP, 1);
			OUT_RING  (chan, 0);
		}
		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
		OUT_RING  (chan, 0);

		prim |= (1 << 28);
	}
}

void
nv50_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
	struct nv50_context *nv50 = nv50_context(pipe);

	if (info->indexed && nv50->idxbuf.buffer) {
		unsigned offset;

		assert(nv50->idxbuf.offset % nv50->idxbuf.index_size == 0);
		offset = nv50->idxbuf.offset / nv50->idxbuf.index_size;

		nv50_draw_elements_instanced(pipe,
					     nv50->idxbuf.buffer,
					     nv50->idxbuf.index_size,
					     info->index_bias,
					     info->mode,
					     info->start + offset,
					     info->count,
					     info->start_instance,
					     info->instance_count);
	}
	else {
		nv50_draw_arrays_instanced(pipe,
					   info->mode,
					   info->start,
					   info->count,
					   info->start_instance,
					   info->instance_count);
	}
}

static INLINE boolean
nv50_vbo_static_attrib(struct nv50_context *nv50, unsigned attrib,
		       struct nouveau_stateobj **pso,
		       struct pipe_vertex_element *ve,
		       struct pipe_vertex_buffer *vb)

{
	struct nouveau_stateobj *so;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_bo *bo = nv50_resource(vb->buffer)->bo;
	float v[4];
	int ret;
	unsigned nr_components = util_format_get_nr_components(ve->src_format);

	ret = nouveau_bo_map(bo, NOUVEAU_BO_RD);
	if (ret)
		return FALSE;

	util_format_read_4f(ve->src_format, v, 0, (uint8_t *)bo->map +
			    (vb->buffer_offset + ve->src_offset), 0,
			    0, 0, 1, 1);
	so = *pso;
	if (!so)
		*pso = so = so_new(nv50->vtxelt->num_elements,
				   nv50->vtxelt->num_elements * 4, 0);

	switch (nr_components) {
	case 4:
		so_method(so, tesla, NV50TCL_VTX_ATTR_4F_X(attrib), 4);
		so_data  (so, fui(v[0]));
		so_data  (so, fui(v[1]));
		so_data  (so, fui(v[2]));
		so_data  (so, fui(v[3]));
		break;
	case 3:
		so_method(so, tesla, NV50TCL_VTX_ATTR_3F_X(attrib), 3);
		so_data  (so, fui(v[0]));
		so_data  (so, fui(v[1]));
		so_data  (so, fui(v[2]));
		break;
	case 2:
		so_method(so, tesla, NV50TCL_VTX_ATTR_2F_X(attrib), 2);
		so_data  (so, fui(v[0]));
		so_data  (so, fui(v[1]));
		break;
	case 1:
		if (attrib == nv50->vertprog->vp.edgeflag) {
			so_method(so, tesla, NV50TCL_EDGEFLAG_ENABLE, 1);
			so_data  (so, v[0] ? 1 : 0);
		}
		so_method(so, tesla, NV50TCL_VTX_ATTR_1F(attrib), 1);
		so_data  (so, fui(v[0]));
		break;
	default:
		nouveau_bo_unmap(bo);
		return FALSE;
	}

	nouveau_bo_unmap(bo);
	return TRUE;
}

void
nv50_vtxelt_construct(struct nv50_vtxelt_stateobj *cso)
{
	unsigned i;

	for (i = 0; i < cso->num_elements; ++i)
		cso->hw[i] = nv50_format_table[cso->pipe[i].src_format].vtx;
}

struct nouveau_stateobj *
nv50_vbo_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *vtxbuf, *vtxfmt, *vtxattr;
	unsigned i, n_ve;

	/* don't validate if Gallium took away our buffers */
	if (nv50->vtxbuf_nr == 0)
		return NULL;

	nv50->vbo_fifo = 0;
	if (nv50->screen->force_push ||
	    nv50->vertprog->vp.edgeflag < 16)
		nv50->vbo_fifo = 0xffff;

	for (i = 0; i < nv50->vtxbuf_nr; i++) {
		if (nv50->vtxbuf[i].stride &&
		    !nv50_resource_mapped_by_gpu(nv50->vtxbuf[i].buffer))
			nv50->vbo_fifo = 0xffff;
	}

	n_ve = MAX2(nv50->vtxelt->num_elements, nv50->state.vtxelt_nr);

	vtxattr = NULL;
	vtxbuf = so_new(n_ve * 2, n_ve * 5, nv50->vtxelt->num_elements * 4);
	vtxfmt = so_new(1, n_ve, 0);
	so_method(vtxfmt, tesla, NV50TCL_VERTEX_ARRAY_ATTRIB(0), n_ve);

	for (i = 0; i < nv50->vtxelt->num_elements; i++) {
		struct pipe_vertex_element *ve = &nv50->vtxelt->pipe[i];
		struct pipe_vertex_buffer *vb =
			&nv50->vtxbuf[ve->vertex_buffer_index];
		struct nouveau_bo *bo = nv50_resource(vb->buffer)->bo;
		uint32_t hw = nv50->vtxelt->hw[i];

		if (!vb->stride &&
		    nv50_vbo_static_attrib(nv50, i, &vtxattr, ve, vb)) {
			so_data(vtxfmt, hw | (1 << 4));

			so_method(vtxbuf, tesla,
				  NV50TCL_VERTEX_ARRAY_FORMAT(i), 1);
			so_data  (vtxbuf, 0);

			nv50->vbo_fifo &= ~(1 << i);
			continue;
		}

		if (nv50->vbo_fifo) {
			so_data  (vtxfmt, hw | (ve->instance_divisor ? (1 << 4) : i));
			so_method(vtxbuf, tesla,
				  NV50TCL_VERTEX_ARRAY_FORMAT(i), 1);
			so_data  (vtxbuf, 0);
			continue;
		}

		so_data(vtxfmt, hw | i);

		so_method(vtxbuf, tesla, NV50TCL_VERTEX_ARRAY_FORMAT(i), 3);
		so_data  (vtxbuf, 0x20000000 |
			  (ve->instance_divisor ? 0 : vb->stride));
		so_reloc (vtxbuf, bo, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, bo, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);

		/* vertex array limits */
		so_method(vtxbuf, tesla, NV50TCL_VERTEX_ARRAY_LIMIT_HIGH(i), 2);
		so_reloc (vtxbuf, bo, vb->buffer->width0 - 1,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			  NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, bo, vb->buffer->width0 - 1,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			  NOUVEAU_BO_LOW, 0, 0);
	}
	for (; i < n_ve; ++i) {
		so_data  (vtxfmt, 0x7e080010);

		so_method(vtxbuf, tesla, NV50TCL_VERTEX_ARRAY_FORMAT(i), 1);
		so_data  (vtxbuf, 0);
	}
	nv50->state.vtxelt_nr = nv50->vtxelt->num_elements;

	so_ref (vtxbuf, &nv50->state.vtxbuf);
	so_ref (vtxattr, &nv50->state.vtxattr);
	so_ref (NULL, &vtxbuf);
	so_ref (NULL, &vtxattr);
	return vtxfmt;
}


