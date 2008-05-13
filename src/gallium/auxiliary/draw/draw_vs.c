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
   return vs;
}


void
draw_bind_vertex_shader(struct draw_context *draw,
                        struct draw_vertex_shader *dvs)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   
   if (dvs) 
   {
      draw->vs.vertex_shader = dvs;
      draw->vs.num_vs_outputs = dvs->info.num_outputs;
      dvs->prepare( dvs, draw );
   }
   else {
      draw->vs.vertex_shader = NULL;
      draw->vs.num_vs_outputs = 0;
   }
}


void
draw_delete_vertex_shader(struct draw_context *draw,
                          struct draw_vertex_shader *dvs)
{
   dvs->delete( dvs );
}



boolean 
draw_vs_init( struct draw_context *draw )
{
   tgsi_exec_machine_init(&draw->vs.machine);
   /* FIXME: give this machine thing a proper constructor:
    */
   draw->vs.machine.Inputs = align_malloc(PIPE_MAX_ATTRIBS * sizeof(struct tgsi_exec_vector), 16);
   draw->vs.machine.Outputs = align_malloc(PIPE_MAX_ATTRIBS * sizeof(struct tgsi_exec_vector), 16);

   return TRUE;
}

void
draw_vs_destroy( struct draw_context *draw )
{
   if (draw->vs.machine.Inputs)
      align_free(draw->vs.machine.Inputs);

   if (draw->vs.machine.Outputs)
      align_free(draw->vs.machine.Outputs);

   tgsi_exec_machine_free_data(&draw->vs.machine);

}
