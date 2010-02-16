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

#include "nv50_context.h"

static boolean
nv50_push_elements_u08(struct nv50_context *, uint8_t *, unsigned);

static boolean
nv50_push_elements_u16(struct nv50_context *, uint16_t *, unsigned);

static boolean
nv50_push_elements_u32(struct nv50_context *, uint32_t *, unsigned);

static boolean
nv50_push_arrays(struct nv50_context *, unsigned, unsigned);

#define NV50_USING_LOATHED_EDGEFLAG(ctx) ((ctx)->vertprog->cfg.edgeflag_in < 16)

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
	case PIPE_PRIM_LINES_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_LINES_ADJACENCY;
	case PIPE_PRIM_LINE_STRIP_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_LINE_STRIP_ADJACENCY;
	case PIPE_PRIM_TRIANGLES_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_TRIANGLES_ADJACENCY;
	case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
		return NV50TCL_VERTEX_BEGIN_TRIANGLE_STRIP_ADJACENCY;
	default:
		break;
	}

	NOUVEAU_ERR("invalid primitive type %d\n", mode);
	return NV50TCL_VERTEX_BEGIN_POINTS;
}

static INLINE uint32_t
nv50_vbo_type_to_hw(enum pipe_format format)
{
	const struct util_format_description *desc;

	desc = util_format_description(format);
	assert(desc);

	switch (desc->channel[0].type) {
	case UTIL_FORMAT_TYPE_FLOAT:
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_FLOAT;
	case UTIL_FORMAT_TYPE_UNSIGNED:
		if (desc->channel[0].normalized) {
			return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_UNORM;
		}
		return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_USCALED;
	case UTIL_FORMAT_TYPE_SIGNED:
		if (desc->channel[0].normalized) {
			return NV50TCL_VERTEX_ARRAY_ATTRIB_TYPE_SNORM;
		}
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
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_8_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_8_8_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_8_8_8_8,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_16,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_16_16,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_16_16_16,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_16_16_16_16,
		0, 0, 0, 0,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_32,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_32_32,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_32_32_32,
		NV50TCL_VERTEX_ARRAY_ATTRIB_FORMAT_32_32_32_32 };

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
	const struct util_format_description *desc;
	unsigned size;

	desc = util_format_description(pf);
	assert(desc);

	size = util_format_get_component_bits(pf, UTIL_FORMAT_COLORSPACE_RGB, 0);

	hw_type = nv50_vbo_type_to_hw(pf);
	hw_size = nv50_vbo_size_to_hw(size, ve->nr_components);

	if (!hw_type || !hw_size) {
		NOUVEAU_ERR("unsupported vbo format: %s\n", util_format_name(pf));
		abort();
		return 0x24e80000;
	}

	if (desc->swizzle[0] == UTIL_FORMAT_SWIZZLE_Z) /* BGRA */
		hw_size |= (1 << 31); /* no real swizzle bits :-( */

	return (hw_type | hw_size);
}

/* For instanced drawing from user buffers, hitting the FIFO repeatedly
 * with the same vertex data is probably worse than uploading all data.
 */
static boolean
nv50_upload_vtxbuf(struct nv50_context *nv50, unsigned i)
{
	struct nv50_screen *nscreen = nv50->screen;
	struct pipe_screen *pscreen = &nscreen->base.base;
	struct pipe_buffer *buf = nscreen->strm_vbuf[i];
	struct pipe_vertex_buffer *vb = &nv50->vtxbuf[i];
	uint8_t *src;
	unsigned size = align(vb->buffer->size, 4096);

	if (buf && buf->size < size)
		pipe_buffer_reference(&nscreen->strm_vbuf[i], NULL);

	if (!nscreen->strm_vbuf[i]) {
		nscreen->strm_vbuf[i] = pipe_buffer_create(
			pscreen, 0, PIPE_BUFFER_USAGE_VERTEX, size);
		buf = nscreen->strm_vbuf[i];
	}

	src = pipe_buffer_map(pscreen, vb->buffer, PIPE_BUFFER_USAGE_CPU_READ);
	if (!src)
		return FALSE;
	src += vb->buffer_offset;

	size = (vb->max_index + 1) * vb->stride + 16; /* + 16 is for stride 0 */
	if (vb->buffer_offset + size > vb->buffer->size)
		size = vb->buffer->size - vb->buffer_offset;

	pipe_buffer_write(pscreen, buf, vb->buffer_offset, size, src);
	pipe_buffer_unmap(pscreen, vb->buffer);

	vb->buffer = buf; /* don't pipe_reference, this is a private copy */
	return TRUE;
}

static void
nv50_upload_user_vbufs(struct nv50_context *nv50)
{
	unsigned i;

	if (nv50->vbo_fifo)
		nv50->dirty |= NV50_NEW_ARRAYS;
	if (!(nv50->dirty & NV50_NEW_ARRAYS))
		return;

	for (i = 0; i < nv50->vtxbuf_nr; ++i) {
		if (nv50->vtxbuf[i].buffer->usage & PIPE_BUFFER_USAGE_VERTEX)
			continue;
		nv50_upload_vtxbuf(nv50, i);
	}
}

static void
nv50_set_static_vtxattr(struct nv50_context *nv50, unsigned i, void *data)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;
	float v[4];

	util_format_read_4f(nv50->vtxelt[i].src_format,
			    v, 0, data, 0, 0, 0, 1, 1);

	switch (nv50->vtxelt[i].nr_components) {
	case 4:
		BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_4F_X(i), 4);
		OUT_RINGf (chan, v[0]);
		OUT_RINGf (chan, v[1]);
		OUT_RINGf (chan, v[2]);
		OUT_RINGf (chan, v[3]);
		break;
	case 3:
		BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_3F_X(i), 3);
		OUT_RINGf (chan, v[0]);
		OUT_RINGf (chan, v[1]);
		OUT_RINGf (chan, v[2]);
		break;
	case 2:
		BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_2F_X(i), 2);
		OUT_RINGf (chan, v[0]);
		OUT_RINGf (chan, v[1]);
		break;
	case 1:
		BEGIN_RING(chan, tesla, NV50TCL_VTX_ATTR_1F(i), 1);
		OUT_RINGf (chan, v[0]);
		break;
	default:
		assert(0);
		break;
	}
}

static unsigned
init_per_instance_arrays_immd(struct nv50_context *nv50,
			      unsigned startInstance,
			      unsigned pos[16], unsigned step[16])
{
	struct nouveau_bo *bo;
	unsigned i, b, count = 0;

	for (i = 0; i < nv50->vtxelt_nr; ++i) {
		if (!nv50->vtxelt[i].instance_divisor)
			continue;
		++count;
		b = nv50->vtxelt[i].vertex_buffer_index;

		pos[i] = nv50->vtxelt[i].src_offset +
			nv50->vtxbuf[b].buffer_offset +
			startInstance * nv50->vtxbuf[b].stride;
		step[i] = startInstance % nv50->vtxelt[i].instance_divisor;

		bo = nouveau_bo(nv50->vtxbuf[b].buffer);
		if (!bo->map)
			nouveau_bo_map(bo, NOUVEAU_BO_RD);

		nv50_set_static_vtxattr(nv50, i, (uint8_t *)bo->map + pos[i]);
	}

	return count;
}

static unsigned
init_per_instance_arrays(struct nv50_context *nv50,
			 unsigned startInstance,
			 unsigned pos[16], unsigned step[16])
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;
	struct nouveau_bo *bo;
	struct nouveau_stateobj *so;
	unsigned i, b, count = 0;
	const uint32_t rl = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	if (nv50->vbo_fifo)
		return init_per_instance_arrays_immd(nv50, startInstance,
						     pos, step);

	so = so_new(nv50->vtxelt_nr, nv50->vtxelt_nr * 2, nv50->vtxelt_nr * 2);

	for (i = 0; i < nv50->vtxelt_nr; ++i) {
		if (!nv50->vtxelt[i].instance_divisor)
			continue;
		++count;
		b = nv50->vtxelt[i].vertex_buffer_index;

		pos[i] = nv50->vtxelt[i].src_offset +
			nv50->vtxbuf[b].buffer_offset +
			startInstance * nv50->vtxbuf[b].stride;

		if (!startInstance) {
			step[i] = 0;
			continue;
		}
		step[i] = startInstance % nv50->vtxelt[i].instance_divisor;

		bo = nouveau_bo(nv50->vtxbuf[b].buffer);

		so_method(so, tesla, NV50TCL_VERTEX_ARRAY_START_HIGH(i), 2);
		so_reloc (so, bo, pos[i], rl | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (so, bo, pos[i], rl | NOUVEAU_BO_LOW, 0, 0);
	}

	if (count && startInstance) {
		so_ref (so, &nv50->state.instbuf); /* for flush notify */
		so_emit(chan, nv50->state.instbuf);
	}
	so_ref (NULL, &so);

	return count;
}

static void
step_per_instance_arrays_immd(struct nv50_context *nv50,
			      unsigned pos[16], unsigned step[16])
{
	struct nouveau_bo *bo;
	unsigned i, b;

	for (i = 0; i < nv50->vtxelt_nr; ++i) {
		if (!nv50->vtxelt[i].instance_divisor)
			continue;
		if (++step[i] != nv50->vtxelt[i].instance_divisor)
			continue;
		b = nv50->vtxelt[i].vertex_buffer_index;
		bo = nouveau_bo(nv50->vtxbuf[b].buffer);

		step[i] = 0;
		pos[i] += nv50->vtxbuf[b].stride;

		nv50_set_static_vtxattr(nv50, i, (uint8_t *)bo->map + pos[i]);
	}
}

static void
step_per_instance_arrays(struct nv50_context *nv50,
			 unsigned pos[16], unsigned step[16])
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;
	struct nouveau_bo *bo;
	struct nouveau_stateobj *so;
	unsigned i, b;
	const uint32_t rl = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	if (nv50->vbo_fifo) {
		step_per_instance_arrays_immd(nv50, pos, step);
		return;
	}

	so = so_new(nv50->vtxelt_nr, nv50->vtxelt_nr * 2, nv50->vtxelt_nr * 2);

	for (i = 0; i < nv50->vtxelt_nr; ++i) {
		if (!nv50->vtxelt[i].instance_divisor)
			continue;
		b = nv50->vtxelt[i].vertex_buffer_index;

		if (++step[i] == nv50->vtxelt[i].instance_divisor) {
			step[i] = 0;
			pos[i] += nv50->vtxbuf[b].stride;
		}

		bo = nouveau_bo(nv50->vtxbuf[b].buffer);

		so_method(so, tesla, NV50TCL_VERTEX_ARRAY_START_HIGH(i), 2);
		so_reloc (so, bo, pos[i], rl | NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (so, bo, pos[i], rl | NOUVEAU_BO_LOW, 0, 0);
	}

	so_ref (so, &nv50->state.instbuf); /* for flush notify */
	so_ref (NULL, &so);

	so_emit(chan, nv50->state.instbuf);
}

static INLINE void
nv50_unmap_vbufs(struct nv50_context *nv50)
{
        unsigned i;

        for (i = 0; i < nv50->vtxbuf_nr; ++i)
                if (nouveau_bo(nv50->vtxbuf[i].buffer)->map)
                        nouveau_bo_unmap(nouveau_bo(nv50->vtxbuf[i].buffer));
}

void
nv50_draw_arrays_instanced(struct pipe_context *pipe,
			   unsigned mode, unsigned start, unsigned count,
			   unsigned startInstance, unsigned instanceCount)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	unsigned i, nz_divisors;
	unsigned step[16], pos[16];

	if (!NV50_USING_LOATHED_EDGEFLAG(nv50))
		nv50_upload_user_vbufs(nv50);

	nv50_state_validate(nv50);

	nz_divisors = init_per_instance_arrays(nv50, startInstance, pos, step);

	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 2);
	OUT_RING  (chan, NV50_CB_AUX | (24 << 8));
	OUT_RING  (chan, startInstance);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (chan, nv50_prim(mode));

	if (nv50->vbo_fifo)
		nv50_push_arrays(nv50, start, count);
	else {
		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BUFFER_FIRST, 2);
		OUT_RING  (chan, start);
		OUT_RING  (chan, count);
	}
	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (chan, 0);

	for (i = 1; i < instanceCount; i++) {
		if (nz_divisors) /* any non-zero array divisors ? */
			step_per_instance_arrays(nv50, pos, step);

		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
		OUT_RING  (chan, nv50_prim(mode) | (1 << 28));

		if (nv50->vbo_fifo)
			nv50_push_arrays(nv50, start, count);
		else {
			BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BUFFER_FIRST, 2);
			OUT_RING  (chan, start);
			OUT_RING  (chan, count);
		}
		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
		OUT_RING  (chan, 0);
	}
	nv50_unmap_vbufs(nv50);

	so_ref(NULL, &nv50->state.instbuf);
}

void
nv50_draw_arrays(struct pipe_context *pipe, unsigned mode, unsigned start,
		 unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	boolean ret;

	nv50_state_validate(nv50);

	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (chan, nv50_prim(mode));

	if (nv50->vbo_fifo)
		ret = nv50_push_arrays(nv50, start, count);
	else {
		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BUFFER_FIRST, 2);
		OUT_RING  (chan, start);
		OUT_RING  (chan, count);
		ret = TRUE;
	}
	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (chan, 0);

	nv50_unmap_vbufs(nv50);

        /* XXX: not sure what to do if ret != TRUE: flush and retry?
         */
        assert(ret);
}

static INLINE boolean
nv50_draw_elements_inline_u08(struct nv50_context *nv50, uint8_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	map += start;

	if (nv50->vbo_fifo)
		return nv50_push_elements_u08(nv50, map, count);

	if (count & 1) {
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, map[0]);
		map++;
		count--;
	}

	while (count) {
		unsigned nr = count > 2046 ? 2046 : count;
		int i;

		BEGIN_RING_NI(chan, tesla, NV50TCL_VB_ELEMENT_U16, nr >> 1);
		for (i = 0; i < nr; i += 2)
			OUT_RING  (chan, (map[i + 1] << 16) | map[i]);

		count -= nr;
		map += nr;
	}
	return TRUE;
}

static INLINE boolean
nv50_draw_elements_inline_u16(struct nv50_context *nv50, uint16_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	map += start;

	if (nv50->vbo_fifo)
		return nv50_push_elements_u16(nv50, map, count);

	if (count & 1) {
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, map[0]);
		map++;
		count--;
	}

	while (count) {
		unsigned nr = count > 2046 ? 2046 : count;
		int i;

		BEGIN_RING_NI(chan, tesla, NV50TCL_VB_ELEMENT_U16, nr >> 1);
		for (i = 0; i < nr; i += 2)
			OUT_RING  (chan, (map[i + 1] << 16) | map[i]);

		count -= nr;
		map += nr;
	}
	return TRUE;
}

static INLINE boolean
nv50_draw_elements_inline_u32(struct nv50_context *nv50, uint32_t *map,
			      unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	map += start;

	if (nv50->vbo_fifo)
		return nv50_push_elements_u32(nv50, map, count);

	while (count) {
		unsigned nr = count > 2047 ? 2047 : count;

		BEGIN_RING_NI(chan, tesla, NV50TCL_VB_ELEMENT_U32, nr);
		OUT_RINGp (chan, map, nr);

		count -= nr;
		map += nr;
	}
	return TRUE;
}

static INLINE void
nv50_draw_elements_inline(struct nv50_context *nv50,
			  void *map, unsigned indexSize,
			  unsigned start, unsigned count)
{
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
	}
}

void
nv50_draw_elements_instanced(struct pipe_context *pipe,
			     struct pipe_buffer *indexBuffer,
			     unsigned indexSize,
			     unsigned mode, unsigned start, unsigned count,
			     unsigned startInstance, unsigned instanceCount)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_channel *chan = tesla->channel;
	struct pipe_screen *pscreen = pipe->screen;
	void *map;
	unsigned i, nz_divisors;
	unsigned step[16], pos[16];

	map = pipe_buffer_map(pscreen, indexBuffer, PIPE_BUFFER_USAGE_CPU_READ);

	if (!NV50_USING_LOATHED_EDGEFLAG(nv50))
		nv50_upload_user_vbufs(nv50);

	nv50_state_validate(nv50);

	nz_divisors = init_per_instance_arrays(nv50, startInstance, pos, step);

	BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 2);
	OUT_RING  (chan, NV50_CB_AUX | (24 << 8));
	OUT_RING  (chan, startInstance);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (chan, nv50_prim(mode));

	nv50_draw_elements_inline(nv50, map, indexSize, start, count);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (chan, 0);

	for (i = 1; i < instanceCount; ++i) {
		if (nz_divisors) /* any non-zero array divisors ? */
			step_per_instance_arrays(nv50, pos, step);

		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
		OUT_RING  (chan, nv50_prim(mode) | (1 << 28));

		nv50_draw_elements_inline(nv50, map, indexSize, start, count);

		BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
		OUT_RING  (chan, 0);
	}
	nv50_unmap_vbufs(nv50);

	so_ref(NULL, &nv50->state.instbuf);
}

void
nv50_draw_elements(struct pipe_context *pipe,
		   struct pipe_buffer *indexBuffer, unsigned indexSize,
		   unsigned mode, unsigned start, unsigned count)
{
	struct nv50_context *nv50 = nv50_context(pipe);
	struct nouveau_channel *chan = nv50->screen->tesla->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_screen *pscreen = pipe->screen;
	void *map;
	
	nv50_state_validate(nv50);

	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);
	BEGIN_RING(chan, tesla, 0x142c, 1);
	OUT_RING  (chan, 0);

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_BEGIN, 1);
	OUT_RING  (chan, nv50_prim(mode));

	if (!nv50->vbo_fifo && indexSize == 4) {
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U32 | 0x30000, 0);
		OUT_RING  (chan, count);
		nouveau_pushbuf_submit(chan, nouveau_bo(indexBuffer),
				       start << 2, count << 2);
	} else
	if (!nv50->vbo_fifo && indexSize == 2) {
		unsigned vb_start = (start & ~1);
		unsigned vb_end = (start + count + 1) & ~1;
		unsigned dwords = (vb_end - vb_start) >> 1;

		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U16_SETUP, 1);
		OUT_RING  (chan, ((start & 1) << 31) | count);
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U16 | 0x30000, 0);
		OUT_RING  (chan, dwords);
		nouveau_pushbuf_submit(chan, nouveau_bo(indexBuffer),
				       vb_start << 1, dwords << 2);
		BEGIN_RING(chan, tesla, NV50TCL_VB_ELEMENT_U16_SETUP, 1);
		OUT_RING  (chan, 0);
	} else {
		map = pipe_buffer_map(pscreen, indexBuffer,
				      PIPE_BUFFER_USAGE_CPU_READ);
		nv50_draw_elements_inline(nv50, map, indexSize, start, count);
		nv50_unmap_vbufs(nv50);
		pipe_buffer_unmap(pscreen, indexBuffer);
	}

	BEGIN_RING(chan, tesla, NV50TCL_VERTEX_END, 1);
	OUT_RING  (chan, 0);
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
	float v[4];
	int ret;

	ret = nouveau_bo_map(bo, NOUVEAU_BO_RD);
	if (ret)
		return FALSE;

	util_format_read_4f(ve->src_format, v, 0, (uint8_t *)bo->map +
			    (vb->buffer_offset + ve->src_offset), 0,
			    0, 0, 1, 1);
	so = *pso;
	if (!so)
		*pso = so = so_new(nv50->vtxelt_nr, nv50->vtxelt_nr * 4, 0);

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
		if (attrib == nv50->vertprog->cfg.edgeflag_in) {
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
nv50_vbo_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *vtxbuf, *vtxfmt, *vtxattr;
	unsigned i, n_ve;

	/* don't validate if Gallium took away our buffers */
	if (nv50->vtxbuf_nr == 0)
		return;
	nv50->vbo_fifo = 0;

	for (i = 0; i < nv50->vtxbuf_nr; ++i)
		if (nv50->vtxbuf[i].stride &&
		    !(nv50->vtxbuf[i].buffer->usage & PIPE_BUFFER_USAGE_VERTEX))
			nv50->vbo_fifo = 0xffff;

	if (NV50_USING_LOATHED_EDGEFLAG(nv50))
		nv50->vbo_fifo = 0xffff; /* vertprog can't set edgeflag */

	n_ve = MAX2(nv50->vtxelt_nr, nv50->state.vtxelt_nr);

	vtxattr = NULL;
	vtxbuf = so_new(n_ve * 2, n_ve * 5, nv50->vtxelt_nr * 4);
	vtxfmt = so_new(1, n_ve, 0);
	so_method(vtxfmt, tesla, NV50TCL_VERTEX_ARRAY_ATTRIB(0), n_ve);

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

			nv50->vbo_fifo &= ~(1 << i);
			continue;
		}

		if (nv50->vbo_fifo) {
			so_data  (vtxfmt, hw |
				  (ve->instance_divisor ? (1 << 4) : i));
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
		so_reloc (vtxbuf, bo, vb->buffer->size - 1,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			  NOUVEAU_BO_HIGH, 0, 0);
		so_reloc (vtxbuf, bo, vb->buffer->size - 1,
			  NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD |
			  NOUVEAU_BO_LOW, 0, 0);
	}
	for (; i < n_ve; ++i) {
		so_data  (vtxfmt, 0x7e080010);

		so_method(vtxbuf, tesla, NV50TCL_VERTEX_ARRAY_FORMAT(i), 1);
		so_data  (vtxbuf, 0);
	}
	nv50->state.vtxelt_nr = nv50->vtxelt_nr;

	so_ref (vtxfmt, &nv50->state.vtxfmt);
	so_ref (vtxbuf, &nv50->state.vtxbuf);
	so_ref (vtxattr, &nv50->state.vtxattr);
	so_ref (NULL, &vtxbuf);
	so_ref (NULL, &vtxfmt);
	so_ref (NULL, &vtxattr);
}

typedef void (*pfn_push)(struct nouveau_channel *, void *);

struct nv50_vbo_emitctx
{
	pfn_push push[16];
	uint8_t *map[16];
	unsigned stride[16];
	unsigned nr_ve;
	unsigned vtx_dwords;
	unsigned vtx_max;

	float edgeflag;
	unsigned ve_edgeflag;
};

static INLINE void
emit_vtx_next(struct nouveau_channel *chan, struct nv50_vbo_emitctx *emit)
{
	unsigned i;

	for (i = 0; i < emit->nr_ve; ++i) {
		emit->push[i](chan, emit->map[i]);
		emit->map[i] += emit->stride[i];
	}
}

static INLINE void
emit_vtx(struct nouveau_channel *chan, struct nv50_vbo_emitctx *emit,
	 uint32_t vi)
{
	unsigned i;

	for (i = 0; i < emit->nr_ve; ++i)
		emit->push[i](chan, emit->map[i] + emit->stride[i] * vi);
}

static INLINE boolean
nv50_map_vbufs(struct nv50_context *nv50)
{
	int i;

	for (i = 0; i < nv50->vtxbuf_nr; ++i) {
		struct pipe_vertex_buffer *vb = &nv50->vtxbuf[i];
		unsigned size = vb->stride * (vb->max_index + 1) + 16;

		if (nouveau_bo(vb->buffer)->map)
			continue;

		size = vb->stride * (vb->max_index + 1) + 16;
		size = MIN2(size, vb->buffer->size);
		if (!size)
			size = vb->buffer->size;

		if (nouveau_bo_map_range(nouveau_bo(vb->buffer),
					 0, size, NOUVEAU_BO_RD))
			break;
	}

	if (i == nv50->vtxbuf_nr)
		return TRUE;
	for (; i >= 0; --i)
		nouveau_bo_unmap(nouveau_bo(nv50->vtxbuf[i].buffer));
	return FALSE;
}

static void
emit_b32_1(struct nouveau_channel *chan, void *data)
{
	uint32_t *v = data;

	OUT_RING(chan, v[0]);
}

static void
emit_b32_2(struct nouveau_channel *chan, void *data)
{
	uint32_t *v = data;

	OUT_RING(chan, v[0]);
	OUT_RING(chan, v[1]);
}

static void
emit_b32_3(struct nouveau_channel *chan, void *data)
{
	uint32_t *v = data;

	OUT_RING(chan, v[0]);
	OUT_RING(chan, v[1]);
	OUT_RING(chan, v[2]);
}

static void
emit_b32_4(struct nouveau_channel *chan, void *data)
{
	uint32_t *v = data;

	OUT_RING(chan, v[0]);
	OUT_RING(chan, v[1]);
	OUT_RING(chan, v[2]);
	OUT_RING(chan, v[3]);
}

static void
emit_b16_1(struct nouveau_channel *chan, void *data)
{
	uint16_t *v = data;

	OUT_RING(chan, v[0]);
}

static void
emit_b16_3(struct nouveau_channel *chan, void *data)
{
	uint16_t *v = data;

	OUT_RING(chan, (v[1] << 16) | v[0]);
	OUT_RING(chan, v[2]);
}

static void
emit_b08_1(struct nouveau_channel *chan, void *data)
{
	uint8_t *v = data;

	OUT_RING(chan, v[0]);
}

static void
emit_b08_3(struct nouveau_channel *chan, void *data)
{
	uint8_t *v = data;

	OUT_RING(chan, (v[2] << 16) | (v[1] << 8) | v[0]);
}

static boolean
emit_prepare(struct nv50_context *nv50, struct nv50_vbo_emitctx *emit,
	     unsigned start)
{
	unsigned i;

	if (nv50_map_vbufs(nv50) == FALSE)
		return FALSE;

	emit->ve_edgeflag = nv50->vertprog->cfg.edgeflag_in;

	emit->edgeflag = 0.5f;
	emit->nr_ve = 0;
	emit->vtx_dwords = 0;

	for (i = 0; i < nv50->vtxelt_nr; ++i) {
		struct pipe_vertex_element *ve;
		struct pipe_vertex_buffer *vb;
		unsigned n, size;
		const struct util_format_description *desc;

		ve = &nv50->vtxelt[i];
		vb = &nv50->vtxbuf[ve->vertex_buffer_index];
		if (!(nv50->vbo_fifo & (1 << i)) || ve->instance_divisor)
			continue;
		n = emit->nr_ve++;

		emit->stride[n] = vb->stride;
		emit->map[n] = (uint8_t *)nouveau_bo(vb->buffer)->map +
			vb->buffer_offset +
			(start * vb->stride + ve->src_offset);

		desc = util_format_description(ve->src_format);
		assert(desc);

		size = util_format_get_component_bits(
			ve->src_format, UTIL_FORMAT_COLORSPACE_RGB, 0);

		assert(ve->nr_components > 0 && ve->nr_components <= 4);

		/* It shouldn't be necessary to push the implicit 1s
		 * for case 3 and size 8 cases 1, 2, 3.
		 */
		switch (size) {
		default:
			NOUVEAU_ERR("unsupported vtxelt size: %u\n", size);
			return FALSE;
		case 32:
			switch (ve->nr_components) {
			case 1: emit->push[n] = emit_b32_1; break;
			case 2: emit->push[n] = emit_b32_2; break;
			case 3: emit->push[n] = emit_b32_3; break;
			case 4: emit->push[n] = emit_b32_4; break;
			}
			emit->vtx_dwords += ve->nr_components;
			break;
		case 16:
			switch (ve->nr_components) {
			case 1: emit->push[n] = emit_b16_1; break;
			case 2: emit->push[n] = emit_b32_1; break;
			case 3: emit->push[n] = emit_b16_3; break;
			case 4: emit->push[n] = emit_b32_2; break;
			}
			emit->vtx_dwords += (ve->nr_components + 1) >> 1;
			break;
		case 8:
			switch (ve->nr_components) {
			case 1: emit->push[n] = emit_b08_1; break;
			case 2: emit->push[n] = emit_b16_1; break;
			case 3: emit->push[n] = emit_b08_3; break;
			case 4: emit->push[n] = emit_b32_1; break;
			}
			emit->vtx_dwords += 1;
			break;
		}
	}

	emit->vtx_max = 512 / emit->vtx_dwords;
	if (emit->ve_edgeflag < 16)
		emit->vtx_max = 1;

	return TRUE;
}

static INLINE void
set_edgeflag(struct nouveau_channel *chan,
	     struct nouveau_grobj *tesla,
	     struct nv50_vbo_emitctx *emit, uint32_t index)
{
	unsigned i = emit->ve_edgeflag;

	if (i < 16) {
		float f = *((float *)(emit->map[i] + index * emit->stride[i]));

		if (emit->edgeflag != f) {
			emit->edgeflag = f;

			BEGIN_RING(chan, tesla, 0x15e4, 1);
			OUT_RING  (chan, f ? 1 : 0);
		}
	}
}

static boolean
nv50_push_arrays(struct nv50_context *nv50, unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_vbo_emitctx emit;

	if (emit_prepare(nv50, &emit, start) == FALSE)
		return FALSE;

	while (count) {
		unsigned i, dw, nr = MIN2(count, emit.vtx_max);
	        dw = nr * emit.vtx_dwords;

		set_edgeflag(chan, tesla, &emit, 0); /* nr will be 1 */

		BEGIN_RING_NI(chan, tesla, NV50TCL_VERTEX_DATA, dw);
		for (i = 0; i < nr; ++i)
			emit_vtx_next(chan, &emit);

		count -= nr;
	}

	return TRUE;
}

static boolean
nv50_push_elements_u32(struct nv50_context *nv50, uint32_t *map, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_vbo_emitctx emit;

	if (emit_prepare(nv50, &emit, 0) == FALSE)
		return FALSE;

	while (count) {
		unsigned i, dw, nr = MIN2(count, emit.vtx_max);
	        dw = nr * emit.vtx_dwords;

		set_edgeflag(chan, tesla, &emit, *map);

		BEGIN_RING_NI(chan, tesla, NV50TCL_VERTEX_DATA, dw);
		for (i = 0; i < nr; ++i)
			emit_vtx(chan, &emit, *map++);

		count -= nr;
	}

	return TRUE;
}

static boolean
nv50_push_elements_u16(struct nv50_context *nv50, uint16_t *map, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_vbo_emitctx emit;

	if (emit_prepare(nv50, &emit, 0) == FALSE)
		return FALSE;

	while (count) {
		unsigned i, dw, nr = MIN2(count, emit.vtx_max);
	        dw = nr * emit.vtx_dwords;

		set_edgeflag(chan, tesla, &emit, *map);

		BEGIN_RING_NI(chan, tesla, NV50TCL_VERTEX_DATA, dw);
		for (i = 0; i < nr; ++i)
			emit_vtx(chan, &emit, *map++);

		count -= nr;
	}

	return TRUE;
}

static boolean
nv50_push_elements_u08(struct nv50_context *nv50, uint8_t *map, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_vbo_emitctx emit;

	if (emit_prepare(nv50, &emit, 0) == FALSE)
		return FALSE;

	while (count) {
		unsigned i, dw, nr = MIN2(count, emit.vtx_max);
	        dw = nr * emit.vtx_dwords;

		set_edgeflag(chan, tesla, &emit, *map);

		BEGIN_RING_NI(chan, tesla, NV50TCL_VERTEX_DATA, dw);
		for (i = 0; i < nr; ++i)
			emit_vtx(chan, &emit, *map++);

		count -= nr;
	}

	return TRUE;
}
