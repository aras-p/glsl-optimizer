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

#include "nv10_context.h"
#include "nv10_state.h"

#include "draw/draw_vbuf.h"

/**
 * Primitive renderer for nv10.
 */
struct nv10_vbuf_render {
	struct vbuf_render base;

	struct nv10_context *nv10;   

	/** Vertex buffer */
	struct pipe_buffer* buffer;

	/** Vertex size in bytes */
	unsigned vertex_size;

	/** Hardware primitive */
	unsigned hwprim;
};


void nv10_vtxbuf_bind( struct nv10_context* nv10 )
{
	struct nv10_screen *screen = nv10->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *celsius = screen->celsius;
	int i;
	for(i = 0; i < 8; i++) {
		BEGIN_RING(chan, celsius, NV10TCL_VTXBUF_ADDRESS(i), 1);
		OUT_RING(chan, 0/*nv10->vtxbuf*/);
		BEGIN_RING(chan, celsius, NV10TCL_VTXFMT(i), 1);
		OUT_RING(chan, 0/*XXX*/);
	}
}

/**
 * Basically a cast wrapper.
 */
static INLINE struct nv10_vbuf_render *
nv10_vbuf_render( struct vbuf_render *render )
{
	assert(render);
	return (struct nv10_vbuf_render *)render;
}


static const struct vertex_info *
nv10_vbuf_render_get_vertex_info( struct vbuf_render *render )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	struct nv10_context *nv10 = nv10_render->nv10;

	nv10_emit_hw_state(nv10);

	return &nv10->vertex_info;
}

static boolean
nv10_vbuf_render_allocate_vertices( struct vbuf_render *render,
		ushort vertex_size,
		ushort nr_vertices )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	struct nv10_context *nv10 = nv10_render->nv10;
	struct pipe_screen *screen = nv10->pipe.screen;
	size_t size = (size_t)vertex_size * (size_t)nr_vertices;

	assert(!nv10_render->buffer);
	nv10_render->buffer = screen->buffer_create(screen, 64, PIPE_BUFFER_USAGE_VERTEX, size);

	nv10->dirty |= NV10_NEW_VTXARRAYS;

	if (nv10_render->buffer)
		return FALSE;
	return TRUE;
}

static void *
nv10_vbuf_render_map_vertices( struct vbuf_render *render )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	struct nv10_context *nv10 = nv10_render->nv10;
	struct pipe_screen *pscreen = nv10->pipe.screen;

	return pipe_buffer_map(pscreen, nv10_render->buffer,
			       PIPE_BUFFER_USAGE_CPU_WRITE);
}

static void
nv10_vbuf_render_unmap_vertices( struct vbuf_render *render,
		ushort min_index,
		ushort max_index )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	struct nv10_context *nv10 = nv10_render->nv10;
	struct pipe_screen *pscreen = nv10->pipe.screen;

	assert(!nv10_render->buffer);
	pipe_buffer_unmap(pscreen, nv10_render->buffer);
}

static boolean
nv10_vbuf_render_set_primitive( struct vbuf_render *render, 
		unsigned prim )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	unsigned hwp = nvgl_primitive(prim);
	if (hwp == 0)
		return FALSE;

	nv10_render->hwprim = hwp;
	return TRUE;
}


static void 
nv10_vbuf_render_draw( struct vbuf_render *render,
		const ushort *indices,
		uint nr_indices)
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	struct nv10_context *nv10 = nv10_render->nv10;
	struct nv10_screen *screen = nv10->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *celsius = screen->celsius;
	int push, i;

	nv10_emit_hw_state(nv10);

	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_ARRAY_OFFSET_POS, 1);
	OUT_RELOCl(chan, nouveau_bo(nv10_render->buffer), 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);

	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_BUFFER_BEGIN_END, 1);
	OUT_RING(chan, nv10_render->hwprim);

	if (nr_indices & 1) {
		BEGIN_RING(chan, celsius, NV10TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, indices[0]);
		indices++; nr_indices--;
	}

	while (nr_indices) {
		// XXX too big/small ? check the size
		push = MIN2(nr_indices, 1200 * 2);

		BEGIN_RING_NI(chan, celsius, NV10TCL_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING(chan, (indices[i+1] << 16) | indices[i]);

		nr_indices -= push;
		indices  += push;
	}

	BEGIN_RING(chan, celsius, NV10TCL_VERTEX_BUFFER_BEGIN_END, 1);
	OUT_RING  (chan, 0);
}


static void
nv10_vbuf_render_release_vertices( struct vbuf_render *render )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);

	assert(nv10_render->buffer);
	pipe_buffer_reference(&nv10_render->buffer, NULL);
}


static void
nv10_vbuf_render_destroy( struct vbuf_render *render )
{
	struct nv10_vbuf_render *nv10_render = nv10_vbuf_render(render);
	FREE(nv10_render);
}


/**
 * Create a new primitive render.
 */
static struct vbuf_render *
nv10_vbuf_render_create( struct nv10_context *nv10 )
{
	struct nv10_vbuf_render *nv10_render = CALLOC_STRUCT(nv10_vbuf_render);

	nv10_render->nv10 = nv10;

	nv10_render->base.max_vertex_buffer_bytes = 16*1024;
	nv10_render->base.max_indices = 1024;
	nv10_render->base.get_vertex_info = nv10_vbuf_render_get_vertex_info;
	nv10_render->base.allocate_vertices = nv10_vbuf_render_allocate_vertices;
	nv10_render->base.map_vertices = nv10_vbuf_render_map_vertices;
	nv10_render->base.unmap_vertices = nv10_vbuf_render_unmap_vertices;
	nv10_render->base.set_primitive = nv10_vbuf_render_set_primitive;
	nv10_render->base.draw = nv10_vbuf_render_draw;
	nv10_render->base.release_vertices = nv10_vbuf_render_release_vertices;
	nv10_render->base.destroy = nv10_vbuf_render_destroy;

	return &nv10_render->base;
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *nv10_draw_vbuf_stage( struct nv10_context *nv10 )
{
	struct vbuf_render *render;
	struct draw_stage *stage;

	render = nv10_vbuf_render_create(nv10);
	if(!render)
		return NULL;

	stage = draw_vbuf_stage( nv10->draw, render );
	if(!stage) {
		render->destroy(render);
		return NULL;
	}

	return stage;
}
