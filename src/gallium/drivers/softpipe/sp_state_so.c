/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"

#include "util/u_format.h"
#include "util/u_memory.h"
#include "draw/draw_context.h"


static void *
softpipe_create_stream_output_state(struct pipe_context *pipe,
                                    const struct pipe_stream_output_state *templ)
{
   struct sp_so_state *so;
   so = (struct sp_so_state *) CALLOC_STRUCT(sp_so_state);

   if (so) {
      so->base.num_outputs = templ->num_outputs;
      so->base.stride = templ->stride;
      memcpy(so->base.output_buffer,
             templ->output_buffer,
             sizeof(int) * templ->num_outputs);
      memcpy(so->base.register_index,
             templ->register_index,
             sizeof(int) * templ->num_outputs);
      memcpy(so->base.register_mask,
             templ->register_mask,
             sizeof(ubyte) * templ->num_outputs);
   }
   return so;
}


static void
softpipe_bind_stream_output_state(struct pipe_context *pipe,
                                  void *so)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   struct sp_so_state *sp_so = (struct sp_so_state *) so;

   softpipe->so = sp_so;

   softpipe->dirty |= SP_NEW_SO;

   if (sp_so)
      draw_set_so_state(softpipe->draw, &sp_so->base);
}


static void
softpipe_delete_stream_output_state(struct pipe_context *pipe, void *so)
{
   FREE( so );
}


static void
softpipe_set_stream_output_buffers(struct pipe_context *pipe,
                                   struct pipe_resource **buffers,
                                   int *offsets,
                                   int num_buffers)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   int i;
   void *map_buffers[PIPE_MAX_SO_BUFFERS];

   assert(num_buffers <= PIPE_MAX_SO_BUFFERS);
   if (num_buffers > PIPE_MAX_SO_BUFFERS)
      num_buffers = PIPE_MAX_SO_BUFFERS;

   softpipe->dirty |= SP_NEW_SO_BUFFERS;

   for (i = 0; i < num_buffers; ++i) {
      void *mapped;
      struct softpipe_resource *res = softpipe_resource(buffers[i]);

      if (!res) {
         /* the whole call is invalid, bail out */
         softpipe->so_target.num_buffers = 0;
         draw_set_mapped_so_buffers(softpipe->draw, 0, 0);
         return;
      }

      softpipe->so_target.buffer[i] = res;
      softpipe->so_target.offset[i] = offsets[i];
      softpipe->so_target.so_count[i] = 0;

      mapped = res->data;
      if (offsets[i] >= 0)
         map_buffers[i] = ((char*)mapped) + offsets[i];
      else {
         /* this is a buffer append */
         assert(!"appending not implemented");
         map_buffers[i] = mapped;
      }
   }
   softpipe->so_target.num_buffers = num_buffers;

   draw_set_mapped_so_buffers(softpipe->draw, map_buffers, num_buffers);
}



void
softpipe_init_streamout_funcs(struct pipe_context *pipe)
{
   pipe->create_stream_output_state = softpipe_create_stream_output_state;
   pipe->bind_stream_output_state = softpipe_bind_stream_output_state;
   pipe->delete_stream_output_state = softpipe_delete_stream_output_state;

   pipe->set_stream_output_buffers = softpipe_set_stream_output_buffers;
}

