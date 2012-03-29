/**************************************************************************
 *
 * Copyright 2012 Marek Olšák <maraeo@gmail.com>
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR THEIR SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_helpers.h"
#include "util/u_inlines.h"

/**
 * This function is used to copy an array of pipe_vertex_buffer structures,
 * while properly referencing the pipe_vertex_buffer::buffer member.
 *
 * enabled_buffers is updated such that the bits corresponding to the indices
 * of disabled buffers are set to 0 and the enabled ones are set to 1.
 *
 * \sa util_copy_framebuffer_state
 */
void util_set_vertex_buffers_mask(struct pipe_vertex_buffer *dst,
                                  uint32_t *enabled_buffers,
                                  const struct pipe_vertex_buffer *src,
                                  unsigned start_slot, unsigned count)
{
   unsigned i;
   uint32_t bitmask = 0;

   dst += start_slot;

   if (src) {
      for (i = 0; i < count; i++) {
         if (src[i].buffer || src[i].user_buffer) {
            bitmask |= 1 << i;
         }
         pipe_resource_reference(&dst[i].buffer, src[i].buffer);
      }

      /* Copy over the other members of pipe_vertex_buffer. */
      memcpy(dst, src, count * sizeof(struct pipe_vertex_buffer));

      *enabled_buffers &= ~(((1ull << count) - 1) << start_slot);
      *enabled_buffers |= bitmask << start_slot;
   }
   else {
      /* Unreference the buffers. */
      for (i = 0; i < count; i++) {
         pipe_resource_reference(&dst[i].buffer, NULL);
         dst[i].user_buffer = NULL;
      }

      *enabled_buffers &= ~(((1ull << count) - 1) << start_slot);
   }
}

/**
 * Same as util_set_vertex_buffers_mask, but it only returns the number
 * of bound buffers.
 */
void util_set_vertex_buffers_count(struct pipe_vertex_buffer *dst,
                                   unsigned *dst_count,
                                   const struct pipe_vertex_buffer *src,
                                   unsigned start_slot, unsigned count)
{
   uint32_t enabled_buffers = (1ull << *dst_count) - 1;

   util_set_vertex_buffers_mask(dst, &enabled_buffers, src, start_slot,
                                count);

   *dst_count = util_last_bit(enabled_buffers);
}
