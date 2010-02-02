/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * \file
 * Build post-transformation, post-clipping vertex buffers and element
 * lists by hooking into the end of the primitive pipeline and
 * manipulating the vertex_id field in the vertex headers.
 *
 * XXX: work in progress 
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */


#include "util/u_debug.h"
#include "util/u_inlines.h"
#include "pipe/internal/p_winsys_screen.h"

#include "nv20_context.h"
#include "nv20_state.h"

#include "draw/draw_vbuf.h"

/**
 * Primitive renderer for nv20.
 */
struct nv20_vbuf_render {
	struct vbuf_render base;

	struct nv20_context *nv20;   

	/** Vertex buffer in VRAM */
	struct pipe_buffer *pbuffer;

	/** Vertex buffer in normal memory */
	void *mbuffer;

	/** Vertex size in bytes */
	/*unsigned vertex_size;*/

	/** Hardware primitive */
	unsigned hwprim;
};

/**
 * Basically a cast wrapper.
 */
static INLINE struct nv20_vbuf_render *
nv20_vbuf_render(struct vbuf_render *render)
{
	assert(render);
	return (struct nv20_vbuf_render *)render;
}

void nv20_vtxbuf_bind( struct nv20_context* nv20 )
{
#if 0
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;
	int i;
	for(i = 0; i < NV20TCL_VTXBUF_ADDRESS__SIZE; i++) {
		BEGIN_RING(chan, kelvin, NV20TCL_VTXBUF_ADDRESS(i), 1);
		OUT_RING(chan, 0/*nv20->vtxbuf*/);
		BEGIN_RING(chan, kelvin, NV20TCL_VTXFMT(i) ,1);
		OUT_RING(chan, 0/*XXX*/);
	}
#endif
}

static const struct vertex_info *
nv20_vbuf_render_get_vertex_info( struct vbuf_render *render )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct nv20_context *nv20 = nv20_render->nv20;

	nv20_emit_hw_state(nv20);

	return &nv20->vertex_info;
}

static void *
nv20__allocate_mbuffer(struct nv20_vbuf_render *nv20_render, size_t size)
{
	nv20_render->mbuffer = MALLOC(size);
	return nv20_render->mbuffer;
}

static void
nv20__allocate_pbuffer(struct nv20_vbuf_render *nv20_render, size_t size)
{
	struct pipe_screen *screen = nv20_render->nv20->pipe.screen;
	nv20_render->pbuffer = screen->buffer_create(screen, 64,
					PIPE_BUFFER_USAGE_VERTEX, size);
}

static boolean
nv20_vbuf_render_allocate_vertices( struct vbuf_render *render,
		ushort vertex_size,
		ushort nr_vertices )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	size_t size = (size_t)vertex_size * (size_t)nr_vertices;
	void *buf;

	assert(!nv20_render->pbuffer);
	assert(!nv20_render->mbuffer);

	/*
	 * For small amount of vertices, don't bother with pipe vertex
	 * buffer, the data will be passed directly via the fifo.
	 */
	/* XXX: Pipe vertex buffers don't work. */
	if (0 && size > 16 * 1024) {
		nv20__allocate_pbuffer(nv20_render, size);
		/* umm yeah so this is ugly */
		buf = nv20_render->pbuffer;
	} else {
		buf = nv20__allocate_mbuffer(nv20_render, size);
	}

	if (buf)
		nv20_render->nv20->dirty |= NV20_NEW_VTXARRAYS;

	return buf ? TRUE : FALSE;
}

static void *
nv20_vbuf_render_map_vertices( struct vbuf_render *render )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct pipe_screen *pscreen = nv20_render->nv20->pipe.screen;

	if (nv20_render->pbuffer) {
		return pipe_buffer_map(pscreen, nv20_render->pbuffer,
				       PIPE_BUFFER_USAGE_CPU_WRITE);
	} else if (nv20_render->mbuffer) {
		return nv20_render->mbuffer;
	} else
		assert(0);

	/* warnings be gone */
	return NULL;
}

static void
nv20_vbuf_render_unmap_vertices( struct vbuf_render *render,
		ushort min_index,
		ushort max_index )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct pipe_screen *pscreen = nv20_render->nv20->pipe.screen;

	if (nv20_render->pbuffer)
		pipe_buffer_unmap(pscreen, nv20_render->pbuffer);
}

static boolean
nv20_vbuf_render_set_primitive( struct vbuf_render *render, 
		unsigned prim )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	unsigned hwp = nvgl_primitive(prim);
	if (hwp == 0)
		return FALSE;

	nv20_render->hwprim = hwp;
	return TRUE;
}

static uint32_t
nv20__vtxhwformat(unsigned stride, unsigned fields, unsigned type)
{
	return (stride << NV20TCL_VTXFMT_STRIDE_SHIFT) |
		(fields << NV20TCL_VTXFMT_SIZE_SHIFT) |
		(type << NV20TCL_VTXFMT_TYPE_SHIFT);
}

static unsigned
nv20__emit_format(struct nv20_context *nv20, enum attrib_emit type, int hwattr)
{
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;
	uint32_t hwfmt = 0;
	unsigned fields;

	switch (type) {
	case EMIT_OMIT:
		hwfmt = nv20__vtxhwformat(0, 0, 2);
		fields = 0;
		break;
	case EMIT_1F:
		hwfmt = nv20__vtxhwformat(4, 1, 2);
		fields = 1;
		break;
	case EMIT_2F:
		hwfmt = nv20__vtxhwformat(8, 2, 2);
		fields = 2;
		break;
	case EMIT_3F:
		hwfmt = nv20__vtxhwformat(12, 3, 2);
		fields = 3;
		break;
	case EMIT_4F:
		hwfmt = nv20__vtxhwformat(16, 4, 2);
		fields = 4;
		break;
	default:
		NOUVEAU_ERR("unhandled attrib_emit %d\n", type);
		return 0;
	}

	BEGIN_RING(chan, kelvin, NV20TCL_VTXFMT(hwattr), 1);
	OUT_RING(chan, hwfmt);
	return fields;
}

static unsigned
nv20__emit_vertex_array_format(struct nv20_context *nv20)
{
	struct vertex_info *vinfo = &nv20->vertex_info;
	int hwattr = NV20TCL_VTXFMT__SIZE;
	int attr = 0;
	unsigned nr_fields = 0;

	while (hwattr-- > 0) {
		if (vinfo->hwfmt[0] & (1 << hwattr)) {
			nr_fields += nv20__emit_format(nv20,
					vinfo->attrib[attr].emit, hwattr);
			attr++;
		} else
			nv20__emit_format(nv20, EMIT_OMIT, hwattr);
	}

	return nr_fields;
}

static void
nv20__draw_mbuffer(struct nv20_vbuf_render *nv20_render,
		const ushort *indices,
		uint nr_indices)
{
	struct nv20_context *nv20 = nv20_render->nv20;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;
	struct vertex_info *vinfo = &nv20->vertex_info;
	unsigned nr_fields;
	int max_push;
	ubyte *data = nv20_render->mbuffer;
	int vsz = 4 * vinfo->size;

	nr_fields = nv20__emit_vertex_array_format(nv20);

	BEGIN_RING(chan, kelvin, NV20TCL_VERTEX_BEGIN_END, 1);
	OUT_RING(chan, nv20_render->hwprim);

	max_push = 1200 / nr_fields;
	while (nr_indices) {
		int i;
		int push = MIN2(nr_indices, max_push);

		BEGIN_RING_NI(chan, kelvin, NV20TCL_VERTEX_DATA, push * nr_fields);
		for (i = 0; i < push; i++) {
			/* XXX: fixme to handle other than floats? */
			int f = nr_fields;
			float *attrv = (float*)&data[indices[i] * vsz];
			while (f-- > 0)
				OUT_RINGf(chan, *attrv++);
		}

		nr_indices -= push;
		indices += push;
	}

	BEGIN_RING(chan, kelvin, NV20TCL_VERTEX_BEGIN_END, 1);
	OUT_RING(chan, NV20TCL_VERTEX_BEGIN_END_STOP);
}

static void
nv20__draw_pbuffer(struct nv20_vbuf_render *nv20_render,
		const ushort *indices,
		uint nr_indices)
{
	struct nv20_context *nv20 = nv20_render->nv20;
	struct nv20_screen *screen = nv20->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *kelvin = screen->kelvin;
	int push, i;

	NOUVEAU_ERR("nv20__draw_pbuffer: this path is broken.\n");

	BEGIN_RING(chan, kelvin, NV10TCL_VERTEX_ARRAY_OFFSET_POS, 1);
	OUT_RELOCl(chan, nouveau_bo(nv20_render->pbuffer), 0,
			NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);

	BEGIN_RING(chan, kelvin, NV10TCL_VERTEX_BUFFER_BEGIN_END, 1);
	OUT_RING(chan, nv20_render->hwprim);

	if (nr_indices & 1) {
		BEGIN_RING(chan, kelvin, NV10TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, indices[0]);
		indices++; nr_indices--;
	}

	while (nr_indices) {
		// XXX too big/small ? check the size
		push = MIN2(nr_indices, 1200 * 2);

		BEGIN_RING_NI(chan, kelvin, NV10TCL_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING(chan, (indices[i+1] << 16) | indices[i]);

		nr_indices -= push;
		indices  += push;
	}

	BEGIN_RING(chan, kelvin, NV10TCL_VERTEX_BUFFER_BEGIN_END, 1);
	OUT_RING  (chan, 0);
}

static void
nv20_vbuf_render_draw( struct vbuf_render *render,
		const ushort *indices,
		uint nr_indices)
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);

	nv20_emit_hw_state(nv20_render->nv20);

	if (nv20_render->pbuffer)
		nv20__draw_pbuffer(nv20_render, indices, nr_indices);
	else if (nv20_render->mbuffer)
		nv20__draw_mbuffer(nv20_render, indices, nr_indices);
	else
		assert(0);
}


static void
nv20_vbuf_render_release_vertices( struct vbuf_render *render )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct nv20_context *nv20 = nv20_render->nv20;

	if (nv20_render->pbuffer) {
		pipe_buffer_reference(&nv20_render->pbuffer, NULL);
	} else if (nv20_render->mbuffer) {
		FREE(nv20_render->mbuffer);
		nv20_render->mbuffer = NULL;
	} else
		assert(0);
}


static void
nv20_vbuf_render_destroy( struct vbuf_render *render )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);

	assert(!nv20_render->pbuffer);
	assert(!nv20_render->mbuffer);

	FREE(nv20_render);
}


/**
 * Create a new primitive render.
 */
static struct vbuf_render *
nv20_vbuf_render_create( struct nv20_context *nv20 )
{
	struct nv20_vbuf_render *nv20_render = CALLOC_STRUCT(nv20_vbuf_render);

	nv20_render->nv20 = nv20;

	nv20_render->base.max_vertex_buffer_bytes = 16*1024;
	nv20_render->base.max_indices = 1024;
	nv20_render->base.get_vertex_info = nv20_vbuf_render_get_vertex_info;
	nv20_render->base.allocate_vertices =
					nv20_vbuf_render_allocate_vertices;
	nv20_render->base.map_vertices = nv20_vbuf_render_map_vertices;
	nv20_render->base.unmap_vertices = nv20_vbuf_render_unmap_vertices;
	nv20_render->base.set_primitive = nv20_vbuf_render_set_primitive;
	nv20_render->base.draw = nv20_vbuf_render_draw;
	nv20_render->base.release_vertices = nv20_vbuf_render_release_vertices;
	nv20_render->base.destroy = nv20_vbuf_render_destroy;

	return &nv20_render->base;
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *nv20_draw_vbuf_stage( struct nv20_context *nv20 )
{
	struct vbuf_render *render;
	struct draw_stage *stage;

	render = nv20_vbuf_render_create(nv20);
	if(!render)
		return NULL;

	stage = draw_vbuf_stage( nv20->draw, render );
	if(!stage) {
		render->destroy(render);
		return NULL;
	}

	return stage;
}
