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

#include "g_context.h"
#include "g_prim.h"
#include "g_state.h"
#include "g_draw.h"

static void generic_destroy( struct softpipe_context *softpipe )
{
   struct generic_context *generic = generic_context( softpipe );

   draw_destroy( generic->draw );

   FREE( generic );
}


static void generic_draw_vb( struct softpipe_context *softpipe,
			     struct vertex_buffer *VB )
{
   struct generic_context *generic = generic_context( softpipe );

   if (generic->dirty)
      generic_update_derived( generic );

   draw_vb( generic->draw, VB );
}

struct softpipe_context *generic_create( void )
{
   struct generic_context *generic = CALLOC_STRUCT(generic_context);

   generic->softpipe.destroy = generic_destroy;
   generic->softpipe.set_clip_state = generic_set_clip_state;
   generic->softpipe.set_viewport = generic_set_viewport;
   generic->softpipe.set_setup_state = generic_set_setup_state;
   generic->softpipe.set_scissor_rect = generic_set_scissor_rect;
   generic->softpipe.set_fs_state = generic_set_fs_state;
   generic->softpipe.set_polygon_stipple = generic_set_polygon_stipple;
   generic->softpipe.set_cbuf_state = generic_set_cbuf_state;
   generic->softpipe.draw_vb = generic_draw_vb;



   generic->prim.setup     = prim_setup( generic );
   generic->prim.unfilled  = prim_unfilled( generic );
   generic->prim.twoside   = prim_twoside( generic );
   generic->prim.offset    = prim_offset( generic );
   generic->prim.clip      = prim_clip( generic );
   generic->prim.flatshade = prim_flatshade( generic );
   generic->prim.cull      = prim_cull( generic );


   generic->draw = draw_create( generic );

   ASSIGN_4V( generic->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( generic->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( generic->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( generic->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( generic->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( generic->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   generic->nr_planes = 6;

   return &generic->softpipe;
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





