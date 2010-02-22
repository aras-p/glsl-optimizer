/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "util/u_inlines.h"
#include "pipe/p_defines.h"
#include "util/u_math.h"
#include "tgsi/tgsi_parse.h"

#include "svga_screen.h"
#include "svga_screen_buffer.h"
#include "svga_context.h"


static void svga_set_vertex_buffers(struct pipe_context *pipe,
                                    unsigned count,
                                    const struct pipe_vertex_buffer *buffers)
{
   struct svga_context *svga = svga_context(pipe);
   unsigned i;
   boolean any_user_buffer = FALSE;

   /* Check for no change */
   if (count == svga->curr.num_vertex_buffers &&
       memcmp(svga->curr.vb, buffers, count * sizeof buffers[0]) == 0)
      return;

   /* Adjust refcounts */
   for (i = 0; i < count; i++) {
      pipe_buffer_reference(&svga->curr.vb[i].buffer, buffers[i].buffer);
      if (svga_buffer_is_user_buffer(buffers[i].buffer))
         any_user_buffer = TRUE;
   }

   for ( ; i < svga->curr.num_vertex_buffers; i++)
      pipe_buffer_reference(&svga->curr.vb[i].buffer, NULL);

   /* Copy remaining data */
   memcpy(svga->curr.vb, buffers, count * sizeof buffers[0]);
   svga->curr.num_vertex_buffers = count;
   svga->curr.any_user_vertex_buffers = any_user_buffer;

   svga->dirty |= SVGA_NEW_VBUFFER;
}

static void svga_set_vertex_elements(struct pipe_context *pipe,
                                     unsigned count,
                                     const struct pipe_vertex_element *elements)
{
   struct svga_context *svga = svga_context(pipe);
   unsigned i;

   for (i = 0; i < count; i++)
      svga->curr.ve[i] = elements[i];

   svga->curr.num_vertex_elements = count;
   svga->dirty |= SVGA_NEW_VELEMENT;
}


void svga_cleanup_vertex_state( struct svga_context *svga )
{
   unsigned i;
   
   for (i = 0 ; i < svga->curr.num_vertex_buffers; i++)
      pipe_buffer_reference(&svga->curr.vb[i].buffer, NULL);
}


void svga_init_vertex_functions( struct svga_context *svga )
{
   svga->pipe.set_vertex_buffers = svga_set_vertex_buffers;
   svga->pipe.set_vertex_elements = svga_set_vertex_elements;
}


