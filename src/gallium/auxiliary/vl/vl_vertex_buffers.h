/**************************************************************************
 *
 * Copyright 2010 Christian König
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
#ifndef vl_vertex_buffers_h
#define vl_vertex_buffers_h

#include <assert.h>
#include <pipe/p_state.h>
#include "vl_types.h"

struct vl_vertex_buffer
{
   unsigned num_verts;
   unsigned stride;
   struct pipe_resource *resource;
   struct pipe_transfer *transfer;
   void *vectors;
};

struct pipe_vertex_buffer vl_vb_upload_quads(struct pipe_context *pipe, unsigned max_blocks);

struct pipe_vertex_element vl_vb_get_quad_vertex_element(void);

unsigned vl_vb_element_helper(struct pipe_vertex_element* elements, unsigned num_elements,
                              unsigned vertex_buffer_index);

struct pipe_vertex_buffer vl_vb_init(struct vl_vertex_buffer *buffer,
                                     struct pipe_context *pipe,
                                     unsigned max_blocks, unsigned stride);

void vl_vb_map(struct vl_vertex_buffer *buffer, struct pipe_context *pipe);

static inline void
vl_vb_add_block(struct vl_vertex_buffer *buffer, void *elements)
{
   void *pos;

   assert(buffer);

   pos = buffer->vectors + buffer->num_verts * buffer->stride;
   memcpy(pos, elements, buffer->stride);
   buffer->num_verts++;
}

void vl_vb_unmap(struct vl_vertex_buffer *buffer, struct pipe_context *pipe);

unsigned vl_vb_restart(struct vl_vertex_buffer *buffer);

void vl_vb_cleanup(struct vl_vertex_buffer *buffer);

#endif
