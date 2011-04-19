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

struct vl_ycbcr_vertex_stream
{
   uint8_t x;
   uint8_t y;
   uint8_t intra;
   uint8_t field;
};

struct vl_mv_vertex_stream
{
   struct vertex4s mv[2];
};

/* vertices for a quad covering a block */
static const struct vertex2f block_quad[4] = {
   {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
};

struct pipe_vertex_buffer
vl_vb_upload_quads(struct pipe_context *pipe)
{
   struct pipe_vertex_buffer quad;
   struct pipe_transfer *buf_transfer;
   struct vertex2f *v;

   unsigned i;

   assert(pipe);

   /* create buffer */
   quad.stride = sizeof(struct vertex2f);
   quad.buffer_offset = 0;
   quad.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(struct vertex2f) * 4
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

   for (i = 0; i < 4; ++i, ++v) {
      v->x = block_quad[i].x;
      v->y = block_quad[i].y;
   }

   pipe_buffer_unmap(pipe, buf_transfer);

   return quad;
}

struct pipe_vertex_buffer
vl_vb_upload_pos(struct pipe_context *pipe, unsigned width, unsigned height)
{
   struct pipe_vertex_buffer pos;
   struct pipe_transfer *buf_transfer;
   struct vertex2s *v;

   unsigned x, y;

   assert(pipe);

   /* create buffer */
   pos.stride = sizeof(struct vertex2s);
   pos.buffer_offset = 0;
   pos.buffer = pipe_buffer_create
   (
      pipe->screen,
      PIPE_BIND_VERTEX_BUFFER,
      PIPE_USAGE_STATIC,
      sizeof(struct vertex2s) * width * height
   );

   if(!pos.buffer)
      return pos;

   /* and fill it */
   v = pipe_buffer_map
   (
      pipe,
      pos.buffer,
      PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
      &buf_transfer
   );

   for ( y = 0; y < height; ++y) {
      for ( x = 0; x < width; ++x, ++v) {
         v->x = x;
         v->y = y;
      }
   }

   pipe_buffer_unmap(pipe, buf_transfer);

   return pos;
}

static struct pipe_vertex_element
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

static void
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
}

void *
vl_vb_get_ves_ycbcr(struct pipe_context *pipe)
{
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];

   assert(pipe);

   memset(&vertex_elems, 0, sizeof(vertex_elems));
   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R8G8B8A8_USCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_VPOS], 1, 1);

   return pipe->create_vertex_elements_state(pipe, 2, vertex_elems);
}

void *
vl_vb_get_ves_mv(struct pipe_context *pipe)
{
   struct pipe_vertex_element vertex_elems[NUM_VS_INPUTS];

   assert(pipe);

   memset(&vertex_elems, 0, sizeof(vertex_elems));
   vertex_elems[VS_I_RECT] = vl_vb_get_quad_vertex_element();

   /* Position element */
   vertex_elems[VS_I_VPOS].src_format = PIPE_FORMAT_R16G16_SSCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_VPOS], 1, 1);

   /* motion vector TOP element */
   vertex_elems[VS_I_MV_TOP].src_format = PIPE_FORMAT_R16G16B16A16_SSCALED;

   /* motion vector BOTTOM element */
   vertex_elems[VS_I_MV_BOTTOM].src_format = PIPE_FORMAT_R16G16B16A16_SSCALED;

   vl_vb_element_helper(&vertex_elems[VS_I_MV_TOP], 2, 2);

   return pipe->create_vertex_elements_state(pipe, NUM_VS_INPUTS, vertex_elems);
}

void
vl_vb_init(struct vl_vertex_buffer *buffer, struct pipe_context *pipe,
           unsigned width, unsigned height)
{
   unsigned i, size;

   assert(buffer);

   buffer->width = width;
   buffer->height = height;

   size = width * height;

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      buffer->ycbcr[i].num_instances = 0;
      buffer->ycbcr[i].resource = pipe_buffer_create
      (
         pipe->screen,
         PIPE_BIND_VERTEX_BUFFER,
         PIPE_USAGE_STREAM,
         sizeof(struct vl_ycbcr_vertex_stream) * size * 4
      );
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      buffer->mv[i].resource = pipe_buffer_create
      (
         pipe->screen,
         PIPE_BIND_VERTEX_BUFFER,
         PIPE_USAGE_STREAM,
         sizeof(struct vl_mv_vertex_stream) * size
      );
   }

   vl_vb_map(buffer, pipe);
}

struct pipe_vertex_buffer
vl_vb_get_ycbcr(struct vl_vertex_buffer *buffer, int component)
{
   struct pipe_vertex_buffer buf;

   assert(buffer);

   buf.stride = sizeof(struct vl_ycbcr_vertex_stream);
   buf.buffer_offset = 0;
   buf.buffer = buffer->ycbcr[component].resource;

   return buf;
}

struct pipe_vertex_buffer
vl_vb_get_mv(struct vl_vertex_buffer *buffer, int motionvector)
{
   struct pipe_vertex_buffer buf;

   assert(buffer);

   buf.stride = sizeof(struct vl_mv_vertex_stream);
   buf.buffer_offset = 0;
   buf.buffer = buffer->mv[motionvector].resource;

   return buf;
}

void
vl_vb_map(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   unsigned i;

   assert(buffer && pipe);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      buffer->ycbcr[i].vertex_stream = pipe_buffer_map
      (
         pipe,
         buffer->ycbcr[i].resource,
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &buffer->ycbcr[i].transfer
      );
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      buffer->mv[i].vertex_stream = pipe_buffer_map
      (
         pipe,
         buffer->mv[i].resource,
         PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD,
         &buffer->mv[i].transfer
      );
   }

}

void vl_vb_add_ycbcr(struct vl_vertex_buffer *buffer,
                     unsigned component, unsigned x, unsigned y,
                     bool intra, enum pipe_mpeg12_dct_type type)
{
   struct vl_ycbcr_vertex_stream *stream;

   assert(buffer);
   assert(buffer->ycbcr[component].num_instances < buffer->width * buffer->height * 4);

   stream = buffer->ycbcr[component].vertex_stream++;
   stream->x = x;
   stream->y = y;
   stream->intra = intra;
   stream->field = type == PIPE_MPEG12_DCT_TYPE_FIELD;

   buffer->ycbcr[component].num_instances++;
}

static void
get_motion_vectors(enum pipe_mpeg12_motion_type mo_type, struct pipe_motionvector *src, struct vertex4s dst[2])
{
   if (mo_type == PIPE_MPEG12_MOTION_TYPE_FRAME) {
      dst[0].x = dst[1].x = src->top.x;
      dst[0].y = dst[1].y = src->top.y;
      dst[0].z = dst[1].z = 0;

   } else {
      dst[0].x = src->top.x;
      dst[0].y = src->top.y;
      dst[0].z = src->top.field_select ? 3 : 1;

      dst[1].x = src->bottom.x;
      dst[1].y = src->bottom.y;
      dst[1].z = src->bottom.field_select ? 3 : 1;
   }

   dst[0].w = src->top.wheight;
   dst[1].w = src->bottom.wheight;
}

void
vl_vb_add_block(struct vl_vertex_buffer *buffer, struct pipe_mpeg12_macroblock *mb)
{
   unsigned mv_pos;

   assert(buffer);
   assert(mb);

   mv_pos = mb->mbx + mb->mby * buffer->width;
   get_motion_vectors(mb->mo_type, &mb->mv[0], buffer->mv[0].vertex_stream[mv_pos].mv);
   get_motion_vectors(mb->mo_type, &mb->mv[1], buffer->mv[1].vertex_stream[mv_pos].mv);
}

void
vl_vb_unmap(struct vl_vertex_buffer *buffer, struct pipe_context *pipe)
{
   unsigned i;

   assert(buffer && pipe);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      pipe_buffer_unmap(pipe, buffer->ycbcr[i].transfer);
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      pipe_buffer_unmap(pipe, buffer->mv[i].transfer);
   }
}

unsigned
vl_vb_restart(struct vl_vertex_buffer *buffer, int component)
{
   unsigned num_instances;

   assert(buffer);

   num_instances = buffer->ycbcr[component].num_instances;
   buffer->ycbcr[component].num_instances = 0;
   return num_instances;
}

void
vl_vb_cleanup(struct vl_vertex_buffer *buffer)
{
   unsigned i;

   assert(buffer);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      pipe_resource_reference(&buffer->ycbcr[i].resource, NULL);
   }

   for (i = 0; i < VL_MAX_REF_FRAMES; ++i) {
      pipe_resource_reference(&buffer->mv[i].resource, NULL);
   }
}
