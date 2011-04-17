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
#include <util/u_format.h>
#include "vl_vertex_buffers.h"
#include "vl_types.h"

struct vl_vertex_stream
{
   struct vertex2s pos;
   uint8_t mb_type_intra;
   uint8_t dct_type_field;
   uint8_t dummy[2];
   uint8_t eb[3][2][2];
   struct vertex4s mv[4];
};

/* vertices for a quad covering a block */
static const struct vertex2f block_quad[4] = {
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

struct pipe_vertex_buffer
vl_vb_upload_quads(struct pipe_context *pipe, unsigned blocks_x, unsigned blocks_y)
{
   struct pipe_vertex_buffer quad;
   struct pipe_transfer *buf_transfer;
   struct vertex4f *v;

   unsigned x, y, i;

   assert(pipe);

   /* create buffer */
   quad.stride = sizeof(struct vertex4f);
   quad.buffer_offset = 0;
   quad.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(struct vertex4f) * 4 * blocks_x * blocks_y
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

   for ( y = 0; y < blocks_y; ++y) {
      for ( x = 0; x < blocks_x; ++x) {
         for (i = 0; i < 4; ++i, ++v) {
            v->x = block_quad[i].x;
            v->y = block_quad[i].y;

            v->z = x;
            v->w = y;
         }
      }
   }

   pipe_buffer_unmap(pipe, buf_transfer);

   return quad;
}

static struct pipe_vertex_element
vl_vb_get_quad_vertex_element(void)
{
   struct pipe_vertex_element element;

   /* setup rectangle element */
   element.src_offset = 0;
   element.instance_divisor = 0;
   element.vertex_buffer_index = 0;
   element.src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

   return element;
}

static void
vl_vb_element_helper(struct pipe_vertex_element* elements, unsigned num_elements,
                              unsigned vertex_buffer_index)
{
   unsigned i, offset = 0;

   assert(elements && num_elements);

   for ( i = 0; i < num_elements; ++i ) {
      if (elements[i].src_offset)
         offset = elements[i].src_offset;
      else
         elements[i].src_offset = offset;
      elements[i].instance_divisor = 1;
      elements[i].vertex_buffer_index = vertex_buffer_index;
      offset += util_format_get_blocksize(elements[i].src_format);
   }
}

void *
vl_vb_get_ves_eb(struct pipe_context *pipe, int component)
{
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];

   assert(pipe);

   memset(&vertex_elems, 0, sizeof(vertex_elems));
   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R16G16_SSCALED;

   /* flags */
   vertex_elems[VS_I_FLAGS].src_format = PIPE_FORMAT_R8G8B8A8_USCALED;

   /* empty block element of selected component */
   vertex_elems[VS_I_EB].src_offset = offsetof(struct vl_vertex_stream, eb[component]);
   vertex_elems[VS_I_EB].src_format = PIPE_FORMAT_R8G8B8A8_USCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_VPOS], NUM_VS_INPUTS - 1, 1);

   return pipe->create_vertex_elements_state(pipe, NUM_VS_INPUTS, vertex_elems);
}

void *
vl_vb_get_ves_mv(struct pipe_context *pipe, int motionvector)
{
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];

   memset(&vertex_elems, 0, sizeof(vertex_elems));
   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();

   assert(pipe);

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R16G16_SSCALED;

   /* motion vector TOP element */
   vertex_elems[VS_I_MV_TOP].src_offset = offsetof(struct vl_vertex_stream, mv[motionvector * 2]);
   vertex_elems[VS_I_MV_TOP].src_format = PIPE_FORMAT_R16G16B16A16_SSCALED;

   /* motion vector BOTTOM element */
   vertex_elems[VS_I_MV_BOTTOM].src_format = PIPE_FORMAT_R16G16B16A16_SSCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_VPOS], NUM_VS_INPUTS - 1, 1);

   return pipe->create_vertex_elements_state(pipe, NUM_VS_INPUTS, vertex_elems);
}

struct pipe_vertex_buffer
vl_vb_init(struct vl_vertex_buffer *buffer, struct pipe_context *pipe, unsigned size)
{
   struct pipe_vertex_buffer buf;

   assert(buffer);

   buffer->size = size;
   buffer->num_not_empty = 0;
   buffer->num_empty = 0;

   buf.stride = sizeof(struct vl_vertex_stream);
   buf.buffer_offset = 0;
   buf.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STREAM,
      sizeof(struct vl_vertex_stream) * size
   );

   pipe_resource_reference(&buffer->resource, buf.buffer);

   vl_vb_map(buffer, pipe);

   return buf;
}

void
vl_vb_map(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   assert(buffer && pipe);

   buffer->start = pipe_buffer_map
   (
      pipe,
      buffer->resource,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buffer->transfer
   );
   buffer->end = buffer->start + buffer->resource->width0 / sizeof(struct vl_vertex_stream);
}

static void
get_motion_vectors(struct pipe_mpeg12_macroblock *mb, struct vertex4s mv[4])
{
   if (mb->mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
      mv[0].x = mv[1].x = mb->mv[0].top.x;
      mv[0].y = mv[1].y = mb->mv[0].top.y;
      mv[0].z = mv[1].z = 0;

      mv[2].x = mv[3].x = mb->mv[1].top.x;
      mv[2].y = mv[3].y = mb->mv[1].top.y;
      mv[2].z = mv[3].z = 0;

   } else {
      mv[0].x = mb->mv[0].top.x;
      mv[0].y = mb->mv[0].top.y;
      mv[0].z = mb->mv[0].top.field_select ? 3 : 1;

      mv[1].x = mb->mv[0].bottom.x;
      mv[1].y = mb->mv[0].bottom.y;
      mv[1].z = mb->mv[0].bottom.field_select ? 3 : 1;

      mv[2].x = mb->mv[1].top.x;
      mv[2].y = mb->mv[1].top.y;
      mv[2].z = mb->mv[1].top.field_select ? 3 : 1;

      mv[3].x = mb->mv[1].bottom.x;
      mv[3].y = mb->mv[1].bottom.y;
      mv[3].z = mb->mv[1].bottom.field_select ? 3 : 1;
   }

   mv[0].w = mv[1].w = mb->mv[0].wheight;
   mv[2].w = mv[3].w = mb->mv[1].wheight;
}

void
vl_vb_add_block(struct vl_vertex_buffer *buffer, struct pipe_mpeg12_macroblock *mb,
                const unsigned (*empty_block_mask)[3][2][2])
{
   struct vl_vertex_stream *stream;
   unsigned i, j, k;

   assert(buffer);
   assert(mb);
   assert(buffer->num_not_empty + buffer->num_empty < buffer->size);

   if(mb->cbp)
      stream = buffer->start + buffer->num_not_empty++;
   else
      stream = buffer->end - ++buffer->num_empty;

   stream->pos.x = mb->mbx;
   stream->pos.y = mb->mby;

   for ( i = 0; i < 3; ++i)
      for ( j = 0; j < 2; ++j)
         for ( k = 0; k < 2; ++k)
            stream->eb[i][j][k] = !(mb->cbp & (*empty_block_mask)[i][j][k]);

   stream->dct_type_field = mb->dct_type == PIPE_MPEG12_DCT_TYPE_FIELD;
   stream->mb_type_intra = mb->dct_intra;

   get_motion_vectors(mb, stream->mv);
}

void
vl_vb_unmap(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   assert(buffer && pipe);

   pipe_buffer_unmap(pipe, buffer->transfer);
}

void
vl_vb_restart(struct vl_vertex_buffer *buffer,
              unsigned *not_empty_start_instance, unsigned *not_empty_num_instances,
              unsigned *empty_start_instance, unsigned *empty_num_instances)
{
   assert(buffer);

   *not_empty_start_instance = 0;
   *not_empty_num_instances = buffer->num_not_empty;
   *empty_start_instance = buffer->size - buffer->num_empty;
   *empty_num_instances = buffer->num_empty;

   buffer->num_not_empty = 0;
   buffer->num_empty = 0;
}

void
vl_vb_cleanup(struct vl_vertex_buffer *buffer)
{
   assert(buffer);

   pipe_resource_reference(&buffer->resource, NULL);
}
