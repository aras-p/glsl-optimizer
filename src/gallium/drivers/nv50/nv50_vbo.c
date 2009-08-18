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

static INLINE unsigned
nv50_vtxeltfmt(unsigned pf)
{
	static const uint8_t vtxelt_32[4] = { 0x90, 0x20, 0x10, 0x08 };
	static const uint8_t vtxelt_16[4] = { 0xd8, 0x78, 0x28, 0x18 };
	static const uint8_t vtxelt_08[4] = { 0xe8, 0xc0, 0x98, 0x50 };

	unsigned nf, c = 0;

	switch (pf_type(pf)) {
	case PIPE_FORMAT_TYPE_FLOAT:
		nf = NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_FLOAT; break;
	case PIPE_FORMAT_TYPE_UNORM:
		nf = NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_UNORM; break;
	case PIPE_FORMAT_TYPE_SNORM:
		nf = NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_SNORM; break;
	case PIPE_FORMAT_TYPE_USCALED:
		nf = NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_USCALED; break;
	case PIPE_FORMAT_TYPE_SSCALED:
		nf = NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_SSCALED; break;
	default:
		NOUVEAU_ERR("invalid vbo type %d\n",pf_type(pf));
		assert(0);
		nf = NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_FLOAT;
		break;
	}

	if (pf_size_y(pf)) c++;
	if (pf_size_z(pf)) c++;
	if (pf_size_w(pf)) c++;

	if (pf_exp2(pf) == 3) {
		switch (pf_size_x(pf)) {
		case 1: return (nf | (vtxelt_08[c] << 16));
		case 2: return (nf | (vtxelt_16[c] << 16));
		case 4: return (nf | (vtxelt_32[c] << 16));
		default:
			break;
		}
	} else
	if (pf_exp2(pf) == 6 && pf_size_x(pf) == 1) {
		NOUVEAU_ERR("unsupported vbo component size 64\n");
		assert(0);
		return (nf | 0x08000000);
	}

	NOUVEAU_ERR("invalid vbo format %s\n",pf_name(pf));
	assert(0);
	return (nf | 0x08000000);
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
			OUT_RING  (chan, (map[1] << 16) | map[0]);

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
			OUT_RING  (chan, (map[1] << 16) | map[0]);

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

void
nv50_vbo_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *vtxbuf, *vtxfmt;
	int i;

	/* don't validate if Gallium took away our buffers */
	if (nv50->vtxbuf_nr == 0)
		return;

	vtxbuf = so_new(nv50->vtxelt_nr * 4, nv50->vtxelt_nr * 2);
	vtxfmt = so_new(nv50->vtxelt_nr + 1, 0);
	so_method(vtxfmt, tesla, NV50TCL_VERTEX_ARRAY_ATTRIB(0),
		nv50->vtxelt_nr);

	for (i = 0; i < nv50->vtxelt_nr; i++) {
		struct pipe_vertex_element *ve = &nv50->vtxelt[i];
		struct pipe_vertex_buffer *vb =
			&nv50->vtxbuf[ve->vertex_buffer_index];
		struct nouveau_bo *bo = nouveau_bo(vb->buffer);

		so_data(vtxfmt, nv50_vtxeltfmt(ve->src_format) | i);

		so_method(vtxbuf, tesla, NV50TCL_VERTEX_ARRAY_FORMAT(i), 3);
		so_data  (vtxbuf, 0x20000000 | vb->stride);
		so_reloc (vtxbuf, bo, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, bo, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	}

	so_ref (vtxfmt, &nv50->state.vtxfmt);
	so_ref (vtxbuf, &nv50->state.vtxbuf);
	so_ref (NULL, &vtxbuf);
	so_ref (NULL, &vtxfmt);
}

