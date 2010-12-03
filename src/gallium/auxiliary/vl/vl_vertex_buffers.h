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
   unsigned num_blocks;
   struct quadf *blocks;
};

struct pipe_vertex_buffer vl_vb_upload_quads(struct pipe_context *pipe, unsigned max_blocks);

bool vl_vb_init(struct vl_vertex_buffer *buffer, unsigned max_blocks);

static inline void
vl_vb_add_block(struct vl_vertex_buffer *buffer, signed x, signed y)
{
   struct quadf *quad;

   assert(buffer);

   quad = buffer->blocks + buffer->num_blocks;
   quad->bl.x = quad->tl.x = quad->tr.x = quad->br.x = x;
   quad->bl.y = quad->tl.y = quad->tr.y = quad->br.y = y;
   buffer->num_blocks++;
}

unsigned vl_vb_upload(struct vl_vertex_buffer *buffer, struct quadf *dst);

void vl_vb_cleanup(struct vl_vertex_buffer *buffer);

#endif
