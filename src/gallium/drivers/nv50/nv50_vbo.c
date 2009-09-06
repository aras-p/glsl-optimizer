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
#include "pipe/p_inlines.h"

#include "nv50_context.h"

static INLINE unsigned
nv50_prim(unsigned mode)
{
	switch (mode) {
	case PIPE_PRIM_POINTS: return NV50TCL_VERTEX_BEGIN_POINTS;
	case PIPE_PRIM_LINES: return NV50TCL_VERTEX_BEGIN_LINES;
	case PIPE_PRIM_LINE_LOOP: return NV50TCL_VERTEX_BEGIN_LINE_LOOP;
	case PIPE_PRIM_LINE_STRIP: return NV50TCL_VERTEX_BEGIN_LINE_STRIP;
	case PIPE_PRIM_TRIANGLES: return NV50TCL_VERTEX_BEGIN_TRIANGLES;
	case PIPE_PRIM_TRIANGLE_STRIP:
		return NV50TCL_VERTEX_BEGIN_TRIANGLE_STRIP;
	case PIPE_PRIM_TRIANGLE_FAN: return NV50TCL_VERTEX_BEGIN_TRIANGLE_FAN;
	case PIPE_PRIM_QUADS: return NV50TCL_VERTEX_BEGIN_QUADS;
	case PIPE_PRIM_QUAD_STRIP: return NV50TCL_VERTEX_BEGIN_QUAD_STRIP;
	case PIPE_PRIM_POLYGON: return NV50TCL_VERTEX_BEGIN_POLYGON;
	default:
		break;
	}

	NOUVEAU_ERR("invalid primitive type %d\n", mode);
	return NV50TCL_VERTEX_BEGIN_POINTS;
}

static INLINE uint32_t
nv50_vbo_type_to_hw(unsigned type)
{
	switch (type) {
	case PIPE_FORMAT_TYPE_FLOAT:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_FLOAT;
	case PIPE_FORMAT_TYPE_UNORM:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_UNORM;
	case PIPE_FORMAT_TYPE_SNORM:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_SNORM;
	case PIPE_FORMAT_TYPE_USCALED:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_USCALED;
	case PIPE_FORMAT_TYPE_SSCALED:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_SSCALED;
	/*
	case PIPE_FORMAT_TYPE_UINT:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_UINT;
	case PIPE_FORMAT_TYPE_SINT:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_SINT; */
	default:
		return 0;
	}
}

static INLINE uint32_t
nv50_vbo_size_to_hw(unsigned size, unsigned nr_c)
{
	static const uint32_t hw_values[] = {
		0, 0, 0, 0,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_8_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_8_8_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_8_8_8_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_16,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_16_16,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_16_16_16,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_16_16_16_16,
		0, 0, 0, 0,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_32,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_32_32,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_32_32_32,
		NV50TCL_VERTEX_ARRAY_ATTRIB_SIZE_32_32_32_32 };

	/* we'd also have R11G11B10 and R10G10B10A2 */

	assert(nr_c > 0 && nr_c <= 4);

	if (size > 32)
		return 0;
	size >>= (3 - 2);

	return hw_values[size + (nr_c - 1)];
}

static INLINE uint32_t
nv50_vbo_vtxelt_to_hw(struct pipe_vertex_element *ve)
{
	uint32_t hw_type, hw_size;
	enum pipe_format pf = ve->src_format;
	unsigned size = pf_size_x(pf) << pf_exp2(pf);

	hw_type = nv50_vbo_type_to_hw(pf_type(pf));
	hw_size = nv50_vbo_size_to_hw(size, ve->nr_components);

	if (!hw_type || !hw_size) {
		NOUVEAU_ERR("unsupported vbo format: %s\n", pf_name(pf));
		abort();
		return 0x24e80000;
	}

	if (pf_swizzle_x(pf) == 2) /* BGRA */
		hw_size |= (1 << 31); /* no real swizzle bits :-( */

	return (hw_type | hw_size);
}

boolean
nv50_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	nv50_state_validate(nv50);

	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x1440, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x1334, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (chan, nv50_prim(mode));
	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BUFFER_FIRST, 2);
	OUT_RING  (chan, start);
	OUT_RING  (chan, count);
	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (chan, 0);

	pipe->flush(pipe, 0, NULL);
	return TRUE;
}

static INLINE void
nv50_draw_elements_inline_u08(struct nv50_context *nv50, uint8_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	map += start;

	if (count & 1) {
		BEGIN_RING(chan, tesla, 0x15e8, 1);
		OUT_RING  (chan, map[0]);
		map++;
		count--;
	}

	while (count) {
		unsigned nr = count > 2046 ? 2046 : count;
		int i;

		BEGIN_RING(chan, tesla, 0x400015f0, nr >> 1);
		for (i = 0; i < nr; i += 2)
			OUT_RING  (chan, (map[i + 1] << 16) | map[i]);

		count -= nr;
		map += nr;
	}
}

static INLINE void
nv50_draw_elements_inline_u16(struct nv50_context *nv50, uint16_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	map += start;

	if (count & 1) {
		BEGIN_RING(chan, tesla, 0x15e8, 1);
		OUT_RING  (chan, map[0]);
		map++;
		count--;
	}

	while (count) {
		unsigned nr = count > 2046 ? 2046 : count;
		int i;

		BEGIN_RING(chan, tesla, 0x400015f0, nr >> 1);
		for (i = 0; i < nr; i += 2)
			OUT_RING  (chan, (map[i + 1] << 16) | map[i]);

		count -= nr;
		map += nr;
	}
}

static INLINE void
nv50_draw_elements_inline_u32(struct nv50_context *nv50, uint32_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	map += start;

	while (count) {
		unsigned nr = count > 2047 ? 2047 : count;

		BEGIN_RING(chan, tesla, 0x400015e8, nr);
		OUT_RINGp (chan, map, nr);

		count -= nr;
		map += nr;
	}
}

boolean
nv50_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_screen *pscreen = pipe->screen;
	void *map;
	
	map = pipe_buffer_map(pscreen, indexBuffer, PIPE_BUFFER_USAGE_CPU_READ);

	nv50_state_validate(nv50);

	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (chan, nv50_prim(mode));
	switch (indexSize) {
	case 1:
		nv50_draw_elements_inline_u08(nv50, map, start, count);
		break;
	case 2:
		nv50_draw_elements_inline_u16(nv50, map, start, count);
		break;
	case 4:
		nv50_draw_elements_inline_u32(nv50, map, start, count);
		break;
	default:
		assert(0);
	}
	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (chan, 0);

	pipe_buffer_unmap(pscreen, indexBuffer);
	pipe->flush(pipe, 0, NULL);
	return TRUE;
}

static INLINE boolean
nv50_vbo_static_attrib(struct nv50_context *nv50, unsigned attrib,
		       struct nouveau_stateobj **pso,
		       struct pipe_vertex_element *ve,
		       struct pipe_vertex_buffer *vb)

{
	struct nouveau_stateobj *so;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_bo *bo = nouveau_bo(vb->buffer);
	float *v;
	int ret;
	enum pipe_format pf = ve->src_format;

	if ((pf_type(pf) != PIPE_FORMAT_TYPE_FLOAT) ||
	    (pf_size_x(pf) << pf_exp2(pf)) != 32)
		return FALSE;

	ret = nouveau_bo_map(bo, NOUVEAU_BO_RD);
	if (ret)
		return FALSE;
	v = (float *)(bo->map + (vb->buffer_offset + ve->src_offset));

	so = *pso;
	if (!so)
		*pso = so = so_new(nv50->vtxelt_nr * 5, 0);

	switch (ve->nr_components) {
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
nv50_vbo_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *vtxbuf, *vtxfmt, *vtxattr;
	unsigned i;

	/* don't validate if Gallium took away our buffers */
	if (nv50->vtxbuf_nr == 0)
		return;

	vtxattr = NULL;
	vtxbuf = so_new(nv50->vtxelt_nr * 7, nv50->vtxelt_nr * 4);
	vtxfmt = so_new(nv50->vtxelt_nr + 1, 0);
	so_method(vtxfmt, tesla, NV50TCL_VERTEX_ARRAY_ATTRIB(0),
		nv50->vtxelt_nr);

	for (i = 0; i < nv50->vtxelt_nr; i++) {
		struct pipe_vertex_element *ve = &nv50->vtxelt[i];
		struct pipe_vertex_buffer *vb =
			&nv50->vtxbuf[ve->vertex_buffer_index];
		struct nouveau_bo *bo = nouveau_bo(vb->buffer);
		uint32_t hw = nv50_vbo_vtxelt_to_hw(ve);

		if (!vb->stride &&
		    nv50_vbo_static_attrib(nv50, i, &vtxattr, ve, vb)) {
			so_data(vtxfmt, hw | (1 << 4));

			so_method(vtxbuf, tesla,
				  NV50TCL_VERTEX_ARRAY_FORMAT(i), 1);
			so_data  (vtxbuf, 0);
			continue;
		}
		so_data(vtxfmt, hw | i);

		so_method(vtxbuf, tesla, NV50TCL_VERTEX_ARRAY_FORMAT(i), 3);
		so_data  (vtxbuf, 0x20000000 | vb->stride);
		so_reloc (vtxbuf, bo, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, bo, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);

		/* vertex array limits */
		so_method(vtxbuf, tesla, 0x1080 + (i * 8), 2);
		so_reloc (vtxbuf, bo, vb->buffer->size - 1,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			  NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, bo, vb->buffer->size - 1,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			  NOUVEAU_BO_LOW, 0, 0);
	}

	so_ref (vtxfmt, &nv50->state.vtxfmt);
	so_ref (vtxbuf, &nv50->state.vtxbuf);
	so_ref (vtxattr, &nv50->state.vtxattr);
	so_ref (NULL, &vtxbuf);
	so_ref (NULL, &vtxfmt);
	so_ref (NULL, &vtxattr);
}

