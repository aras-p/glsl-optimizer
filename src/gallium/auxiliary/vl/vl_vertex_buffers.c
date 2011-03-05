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

#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_screen.h>
#include <util/u_memory.h>
#include <util/u_inlines.h>
#include <util/u_format.h>
#include "vl_vertex_buffers.h"
#include "vl_types.h"

/* vertices for a quad covering a block */
static const struct quadf const_quad = {
   {0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}
};

struct pipe_vertex_buffer
vl_vb_upload_quads(struct pipe_context *pipe, unsigned max_blocks)
{
   struct pipe_vertex_buffer quad;
   struct pipe_transfer *buf_transfer;
   struct quadf *v;

   unsigned i;

   assert(pipe);
   assert(max_blocks);

   /* create buffer */
   quad.stride = sizeof(struct vertex2f);
   quad.buffer_offset = 0;
   quad.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(struct vertex2f) * 4 * max_blocks
   );

   if(!quad.buffer)
      return quad;

   /* and fill it */
   v = pipe_buffer_map
   (
      pipe,
      quad.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer
   );

   for ( i = 0; i < max_blocks; ++i)
     memcpy(v + i, &const_quad, sizeof(const_quad));

   pipe_buffer_unmap(pipe, buf_transfer);

   return quad;
}

struct pipe_vertex_element
vl_vb_get_quad_vertex_element(void)
{
   struct pipe_vertex_element element;

   /* setup rectangle element */
   element.src_offset = 0;
   element.instance_divisor = 0;
   element.vertex_buffer_index = 0;
   element.src_format = PIPE_FORMAT_R32G32_FLOAT;

   return element;
}

unsigned
vl_vb_element_helper(struct pipe_vertex_element* elements, unsigned num_elements,
                              unsigned vertex_buffer_index)
{
   unsigned i, offset = 0;

   assert(elements && num_elements);

   for ( i = 0; i < num_elements; ++i ) {
      elements[i].src_offset = offset;
      elements[i].instance_divisor = 1;
      elements[i].vertex_buffer_index = vertex_buffer_index;
      offset += util_format_get_blocksize(elements[i].src_format);
   }

   return offset;
}

struct pipe_vertex_buffer
vl_vb_init(struct vl_vertex_buffer *buffer, struct pipe_context *pipe,
           unsigned max_blocks, unsigned stride)
{
   struct pipe_vertex_buffer buf;

   assert(buffer);

   buffer->num_verts = 0;
   buffer->stride = stride;

   buf.stride = stride;
   buf.buffer_offset = 0;
   buf.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STREAM,
      stride * 4 * max_blocks
   );

   pipe_resource_reference(&buffer->resource, buf.buffer);

   vl_vb_map(buffer, pipe);

   return buf;
}

void
vl_vb_map(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   assert(buffer && pipe);

   buffer->vectors = pipe_buffer_map
   (
      pipe,
      buffer->resource,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buffer->transfer
   );
}

void
vl_vb_unmap(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   assert(buffer && pipe);

   pipe_buffer_unmap(pipe, buffer->transfer);
}

unsigned
vl_vb_restart(struct vl_vertex_buffer *buffer)
{
   assert(buffer);

   unsigned todo = buffer->num_verts;
   buffer->num_verts = 0;
   return todo;
}

void
vl_vb_cleanup(struct vl_vertex_buffer *buffer)
{
   assert(buffer);

   pipe_resource_reference(&buffer->resource, NULL);
}
