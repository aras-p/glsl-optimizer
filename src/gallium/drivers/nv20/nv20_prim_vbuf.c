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


#include "pipe/p_debug.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"

#include "nv20_context.h"
#include "nv20_state.h"

#include "draw/draw_vbuf.h"

/**
 * Primitive renderer for nv20.
 */
struct nv20_vbuf_render {
	struct vbuf_render base;

	struct nv20_context *nv20;   

	/** Vertex buffer */
	struct pipe_buffer* buffer;

	/** Vertex size in bytes */
	unsigned vertex_size;

	/** Hardware primitive */
	unsigned hwprim;
};


void nv20_vtxbuf_bind( struct nv20_context* nv20 )
{
	int i;
	for(i = 0; i < 8; i++) {
		BEGIN_RING(kelvin, NV10TCL_VERTEX_ARRAY_ATTRIB_OFFSET(i), 1);
		OUT_RING(0/*nv20->vtxbuf*/);
		BEGIN_RING(kelvin, NV10TCL_VERTEX_ARRAY_ATTRIB_FORMAT(i) ,1);
		OUT_RING(0/*XXX*/);
	}
}

/**
 * Basically a cast wrapper.
 */
static INLINE struct nv20_vbuf_render *
nv20_vbuf_render( struct vbuf_render *render )
{
	assert(render);
	return (struct nv20_vbuf_render *)render;
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
nv20_vbuf_render_allocate_vertices( struct vbuf_render *render,
		ushort vertex_size,
		ushort nr_vertices )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct nv20_context *nv20 = nv20_render->nv20;
	struct pipe_winsys *winsys = nv20->pipe.winsys;
	size_t size = (size_t)vertex_size * (size_t)nr_vertices;

	assert(!nv20_render->buffer);
	nv20_render->buffer = winsys->buffer_create(winsys, 64, PIPE_BUFFER_USAGE_VERTEX, size);

	nv20->dirty |= NV20_NEW_VTXARRAYS;

	return winsys->buffer_map(winsys, 
			nv20_render->buffer, 
			PIPE_BUFFER_USAGE_CPU_WRITE);
}


static void 
nv20_vbuf_render_set_primitive( struct vbuf_render *render, 
		unsigned prim )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	nv20_render->hwprim = prim + 1;
}


static void 
nv20_vbuf_render_draw( struct vbuf_render *render,
		const ushort *indices,
		uint nr_indices)
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct nv20_context *nv20 = nv20_render->nv20;
	int push, i;

	nv20_emit_hw_state(nv20);

	BEGIN_RING(kelvin, NV10TCL_VERTEX_ARRAY_OFFSET_POS, 1);
	OUT_RELOCl(nv20_render->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);

	BEGIN_RING(kelvin, NV10TCL_VERTEX_BUFFER_BEGIN_END, 1);
	OUT_RING(nv20_render->hwprim);

	if (nr_indices & 1) {
		BEGIN_RING(kelvin, NV10TCL_VB_ELEMENT_U32, 1);
		OUT_RING  (indices[0]);
		indices++; nr_indices--;
	}

	while (nr_indices) {
		// XXX too big/small ? check the size
		push = MIN2(nr_indices, 1200 * 2);

		BEGIN_RING_NI(kelvin, NV10TCL_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING((indices[i+1] << 16) | indices[i]);

		nr_indices -= push;
		indices  += push;
	}

	BEGIN_RING(kelvin, NV10TCL_VERTEX_BUFFER_BEGIN_END, 1);
	OUT_RING  (0);
}


static void
nv20_vbuf_render_release_vertices( struct vbuf_render *render,
		void *vertices, 
		unsigned vertex_size,
		unsigned vertices_used )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
	struct nv20_context *nv20 = nv20_render->nv20;
	struct pipe_winsys *winsys = nv20->pipe.winsys;
	struct pipe_screen *pscreen = &nv20->screen->pipe;

	assert(nv20_render->buffer);
	winsys->buffer_unmap(winsys, nv20_render->buffer);
	pipe_buffer_reference(pscreen, &nv20_render->buffer, NULL);
}


static void
nv20_vbuf_render_destroy( struct vbuf_render *render )
{
	struct nv20_vbuf_render *nv20_render = nv20_vbuf_render(render);
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
	nv20_render->base.allocate_vertices = nv20_vbuf_render_allocate_vertices;
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
