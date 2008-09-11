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
 * Vertex buffer drawing stage.
 * 
 * \author Keith Whitwell <keith@tungstengraphics.com>
 * \author José Fonseca <jrfonsec@tungstengraphics.com>
 */

#ifndef DRAW_VBUF_H_
#define DRAW_VBUF_H_



struct draw_context;
struct vertex_info;


/**
 * Interface for hardware vertex buffer rendering.
 */
struct vbuf_render {

   /**
    * Driver limits.  May be tuned lower to improve cache hits on
    * index list.
    */
   unsigned max_indices;
   unsigned max_vertex_buffer_bytes;

   /**
    * Get the hardware vertex format.
    * 
    * XXX: have this in draw_context instead?
    */ 
   const struct vertex_info *(*get_vertex_info)( struct vbuf_render * );
	 
   /**
    * Request a destination for vertices.
    * Hardware renderers will use ttm memory, others will just malloc
    * something.
    */
   void *(*allocate_vertices)( struct vbuf_render *,
			       ushort vertex_size,
			       ushort nr_vertices );

   /**
    * Notify the renderer of the current primitive when it changes.
    * Must succeed for TRIANGLES, LINES and POINTS.  Other prims at
    * the discretion of the driver, for the benefit of the passthrough
    * path.
    */
   boolean (*set_primitive)( struct vbuf_render *, unsigned prim );

   /**
    * DrawElements, note indices are ushort.  The driver must complete
    * this call, if necessary splitting the index list itself.
    */
   void (*draw)( struct vbuf_render *,
		 const ushort *indices,
		 uint nr_indices );

   /* Draw Arrays path too.
    */
   void (*draw_arrays)( struct vbuf_render *,
			unsigned start,
			uint nr );

   /**
    * Called when vbuf is done with this set of vertices:
    */
   void (*release_vertices)( struct vbuf_render *,
			     void *vertices, 
			     unsigned vertex_size,
			     unsigned vertices_used );

   void (*destroy)( struct vbuf_render * );
};



struct draw_stage *
draw_vbuf_stage( struct draw_context *draw,
                 struct vbuf_render *render );


#endif /*DRAW_VBUF_H_*/
