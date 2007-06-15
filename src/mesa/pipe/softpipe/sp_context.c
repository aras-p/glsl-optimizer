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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "imports.h"
#include "macros.h"

#include "tnl/t_context.h"
#include "vf/vf.h"

#include "sp_context.h"
#include "sp_prim.h"
#include "sp_state.h"
#include "sp_draw.h"

static void softpipe_destroy( struct pipe_context *pipe )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   draw_destroy( softpipe->draw );

   FREE( softpipe );
}


static void softpipe_draw_vb( struct pipe_context *pipe,
			     struct vertex_buffer *VB )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );

   if (softpipe->dirty)
      softpipe_update_derived( softpipe );

   draw_vb( softpipe->draw, VB );
}

struct pipe_context *softpipe_create( void )
{
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);

   softpipe->pipe.destroy = softpipe_destroy;
   softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;
   softpipe->pipe.set_clip_state = softpipe_set_clip_state;
   softpipe->pipe.set_viewport = softpipe_set_viewport;
   softpipe->pipe.set_setup_state = softpipe_set_setup_state;
   softpipe->pipe.set_scissor_rect = softpipe_set_scissor_rect;
   softpipe->pipe.set_fs_state = softpipe_set_fs_state;
   softpipe->pipe.set_polygon_stipple = softpipe_set_polygon_stipple;
   softpipe->pipe.set_cbuf_state = softpipe_set_cbuf_state;
   softpipe->pipe.draw_vb = softpipe_draw_vb;



   softpipe->prim.setup     = prim_setup( softpipe );
   softpipe->prim.unfilled  = prim_unfilled( softpipe );
   softpipe->prim.twoside   = prim_twoside( softpipe );
   softpipe->prim.offset    = prim_offset( softpipe );
   softpipe->prim.clip      = prim_clip( softpipe );
   softpipe->prim.flatshade = prim_flatshade( softpipe );
   softpipe->prim.cull      = prim_cull( softpipe );


   softpipe->draw = draw_create( softpipe );

   ASSIGN_4V( softpipe->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( softpipe->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( softpipe->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( softpipe->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( softpipe->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( softpipe->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   softpipe->nr_planes = 6;

   return &softpipe->pipe;
}






#define MAX_VERTEX_SIZE ((2 + FRAG_ATTRIB_MAX) * 4 * sizeof(GLfloat))

void prim_alloc_tmps( struct prim_stage *stage, GLuint nr )
{
   stage->nr_tmps = nr;

   if (nr) {
      GLubyte *store = MALLOC(MAX_VERTEX_SIZE * nr);
      GLuint i;

      stage->tmp = MALLOC(sizeof(struct vertex_header *) * nr);
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}

void prim_free_tmps( struct prim_stage *stage )
{
   if (stage->tmp) {
      FREE(stage->tmp[0]);
      FREE(stage->tmp);
   }
}





