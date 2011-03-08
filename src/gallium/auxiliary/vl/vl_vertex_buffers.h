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
#include <pipe/p_video_state.h>
#include "vl_types.h"

enum VS_INPUT
{
   VS_I_RECT,
   VS_I_VPOS,
   VS_I_EB_0_0,
   VS_I_EB_0_1,
   VS_I_EB_1_0,
   VS_I_EB_1_1,
   VS_I_MV0,
   VS_I_MV1,
   VS_I_MV2,
   VS_I_MV3,

   NUM_VS_INPUTS
};

struct vl_vertex_buffer
{
   unsigned size;
   unsigned num_not_empty;
   unsigned num_empty;
   struct pipe_resource *resource;
   struct pipe_transfer *transfer;
   struct vl_vertex_stream *start;
   struct vl_vertex_stream *end;
};

struct pipe_vertex_buffer vl_vb_upload_quads(struct pipe_context *pipe,
                                             unsigned blocks_x, unsigned blocks_y);

void *vl_vb_get_elems_state(struct pipe_context *pipe, bool include_mvs);

struct pipe_vertex_buffer vl_vb_init(struct vl_vertex_buffer *buffer,
                                     struct pipe_context *pipe,
                                     unsigned max_blocks);

void vl_vb_map(struct vl_vertex_buffer *buffer, struct pipe_context *pipe);

void vl_vb_add_block(struct vl_vertex_buffer *buffer, struct pipe_mpeg12_macroblock *mb,
                     const unsigned (*empty_block_mask)[3][2][2]);

void vl_vb_unmap(struct vl_vertex_buffer *buffer, struct pipe_context *pipe);

void vl_vb_restart(struct vl_vertex_buffer *buffer);

void vl_vb_cleanup(struct vl_vertex_buffer *buffer);

#endif
