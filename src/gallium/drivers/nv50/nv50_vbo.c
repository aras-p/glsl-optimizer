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

boolean
nv50_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
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
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
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
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
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
nv50_draw_elements_inline_u32(struct nv50_context *nv50, uint8_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
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
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_winsys *ws = pipe->winsys;
	void *map = ws->buffer_map(ws, indexBuffer, PIPE_BUFFER_USAGE_CPU_READ);

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

	pipe->flush(pipe, 0, NULL);
	return TRUE;
}

void
nv50_vbo_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *vtxbuf, *vtxfmt;
	int i;

	vtxbuf = so_new(nv50->vtxelt_nr * 4, nv50->vtxelt_nr * 2);
	vtxfmt = so_new(nv50->vtxelt_nr + 1, 0);
	so_method(vtxfmt, tesla, 0x1ac0, nv50->vtxelt_nr);

	for (i = 0; i < nv50->vtxelt_nr; i++) {
		struct pipe_vertex_element *ve = &nv50->vtxelt[i];
		struct pipe_vertex_buffer *vb =
			&nv50->vtxbuf[ve->vertex_buffer_index];

		switch (ve->src_format) {
		case PIPE_FORMAT_R32G32B32A32_FLOAT:
			so_data(vtxfmt, 0x7e080000 | i);
			break;
		case PIPE_FORMAT_R32G32B32_FLOAT:
			so_data(vtxfmt, 0x7e100000 | i);
			break;
		case PIPE_FORMAT_R32G32_FLOAT:
			so_data(vtxfmt, 0x7e200000 | i);
			break;
		case PIPE_FORMAT_R32_FLOAT:
			so_data(vtxfmt, 0x7e900000 | i);
			break;
		case PIPE_FORMAT_R8G8B8A8_UNORM:
			so_data(vtxfmt, 0x24500000 | i);
			break;
		default:
		{
			NOUVEAU_ERR("invalid vbo format %s\n",
				    pf_name(ve->src_format));
			assert(0);
			return;
		}
		}

		so_method(vtxbuf, tesla, 0x900 + (i * 16), 3);
		so_data  (vtxbuf, 0x20000000 | vb->stride);
		so_reloc (vtxbuf, vb->buffer, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, vb->buffer, vb->buffer_offset +
			  ve->src_offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
			  NOUVEAU_BO_RD | NOUVEAU_BO_LOW, 0, 0);
	}

	so_ref (vtxfmt, &nv50->state.vtxfmt);
	so_ref (vtxbuf, &nv50->state.vtxbuf);
	so_ref (NULL, &vtxbuf);
	so_ref (NULL, &vtxfmt);
}

