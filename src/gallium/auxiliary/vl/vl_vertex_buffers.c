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
   quad.max_index = 4 * max_blocks - 1;
   quad.buffer_offset = 0;
   quad.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
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

   pipe_buffer_unmap(pipe, quad.buffer, buf_transfer);

   return quad;
}

bool
vl_vb_init(struct vl_vertex_buffer *buffer, unsigned max_blocks)
{
   assert(buffer);

   buffer->num_blocks = 0;
   buffer->blocks = MALLOC(max_blocks * sizeof(struct quadf));
   return buffer->blocks != NULL;
}

unsigned
vl_vb_upload(struct vl_vertex_buffer *buffer, struct quadf *dst)
{
   unsigned todo;

   assert(buffer);

   todo = buffer->num_blocks;
   buffer->num_blocks = 0;

   if(todo)
      memcpy(dst, buffer->blocks, sizeof(struct quadf) * todo);

   return todo;
}

void
vl_vb_cleanup(struct vl_vertex_buffer *buffer)
{
   assert(buffer);

   FREE(buffer->blocks);
}
