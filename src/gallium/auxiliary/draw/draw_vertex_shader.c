/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vs.h"

/**
 * Run the vertex shader on all vertices in the vertex queue.
 * Called by the draw module when the vertx cache needs to be flushed.
 */
void
draw_vertex_shader_queue_flush(struct draw_context *draw)
{
   struct draw_vertex_shader *shader = draw->vertex_shader;
   unsigned i;

   assert(draw->vs.queue_nr != 0);

   /* XXX: do this on statechange: 
    */
   shader->prepare( shader, draw );

//   fprintf(stderr, "%s %d\n", __FUNCTION__, draw->vs.queue_nr );

   /* run vertex shader on vertex cache entries, four per invokation */
   for (i = 0; i < draw->vs.queue_nr; i += MAX_SHADER_VERTICES) {
      struct vertex_header *dests[MAX_SHADER_VERTICES];
      unsigned elts[MAX_SHADER_VERTICES];
      int j, n = MIN2(MAX_SHADER_VERTICES, draw->vs.queue_nr  - i);

      for (j = 0; j < n; j++) {
         elts[j] = draw->vs.queue[i + j].elt;
         dests[j] = draw->vs.queue[i + j].vertex;
      }

      for ( ; j < MAX_SHADER_VERTICES; j++) {
	 elts[j] = elts[0];
         dests[j] = draw->vs.queue[i + j].vertex;
      }

      assert(n > 0);
      assert(n <= MAX_SHADER_VERTICES);

      shader->run(shader, draw, elts, n, dests);
   }

   draw->vs.post_nr = draw->vs.queue_nr;
   draw->vs.queue_nr = 0;
}


struct draw_vertex_shader *
draw_create_vertex_shader(struct draw_context *draw,
                          const struct pipe_shader_state *shader)
{
   struct draw_vertex_shader *vs;

   vs = draw_create_vs_llvm( draw, shader );
   if (!vs) {
      vs = draw_create_vs_sse( draw, shader );
      if (!vs) {
         vs = draw_create_vs_exec( draw, shader );
      }
   }
   assert(vs);

   tgsi_scan_shader(shader->tokens, &vs->info);

   return vs;
}


void
draw_bind_vertex_shader(struct draw_context *draw,
                        struct draw_vertex_shader *dvs)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   
   if (dvs) 
   {
      draw->vertex_shader = dvs;
      draw->num_vs_outputs = dvs->info.num_outputs;

      tgsi_exec_machine_init(&draw->machine);

      dvs->prepare( dvs, draw );
   }
   else {
      draw->vertex_shader = NULL;
      draw->num_vs_outputs = 0;
   }
}


void
draw_delete_vertex_shader(struct draw_context *draw,
                          struct draw_vertex_shader *dvs)
{
   dvs->delete( dvs );
}
