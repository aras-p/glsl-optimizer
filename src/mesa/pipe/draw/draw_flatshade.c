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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "pipe/p_util.h"
#include "draw_private.h"


static void flatshade_begin( struct draw_stage *stage )
{
   stage->next->begin( stage->next );
}



static INLINE void copy_attr( unsigned attr,
			      struct vertex_header *dst, 
			      const struct vertex_header *src )
{
   if (attr) {
      memcpy( dst->data[attr],
	      src->data[attr],
	      sizeof(src->data[0]) );
   }
}


static INLINE void copy_colors( struct draw_stage *stage, 
                                struct vertex_header *dst, 
                                const struct vertex_header *src )
{
   const uint num_attribs = stage->draw->vertex_info.num_attribs;
   const uint *interp_mode = stage->draw->vertex_info.interp_mode;
   uint i;

   /* Look for constant/flat attribs and duplicate from src to dst vertex */
   for (i = 1; i < num_attribs - 2; i++) {
      if (interp_mode[i + 2] == INTERP_CONSTANT) {
         copy_attr( i, dst, src );
      }
   }
}


/**
 * Flatshade tri.  Required for clipping and when unfilled tris are
 * active, otherwise handled by hardware.
 */
static void flatshade_tri( struct draw_stage *stage,
			   struct prim_header *header )
{
   struct prim_header tmp;

   tmp.det = header->det;
   tmp.edgeflags = header->edgeflags;
   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = dup_vert(stage, header->v[1], 1);
   tmp.v[2] = header->v[2];

   copy_colors(stage, tmp.v[0], tmp.v[2]);
   copy_colors(stage, tmp.v[1], tmp.v[2]);
   
   stage->next->tri( stage->next, &tmp );
}


/**
 * Flatshade line.  Required for clipping.
 */
static void flatshade_line( struct draw_stage *stage,
			    struct prim_header *header )
{
   struct prim_header tmp;

   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = header->v[1];

   copy_colors(stage, tmp.v[0], tmp.v[1]);
   
   stage->next->line( stage->next, &tmp );
}


static void flatshade_point( struct draw_stage *stage,
                             struct prim_header *header )
{
   stage->next->point( stage->next, header );
}


static void flatshade_end( struct draw_stage *stage )
{
   stage->next->end( stage->next );
}


static void flatshade_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


/**
 * Create flatshading drawing stage.
 */
struct draw_stage *draw_flatshade_stage( struct draw_context *draw )
{
   struct draw_stage *flatshade = CALLOC_STRUCT(draw_stage);

   draw_alloc_tmps( &flatshade->stage, 2 );

   flatshade->draw = draw;
   flatshade->next = NULL;
   flatshade->begin = flatshade_begin;
   flatshade->point = flatshade_point;
   flatshade->line = flatshade_line;
   flatshade->tri = flatshade_tri;
   flatshade->end = flatshade_end;
   flatshade->reset_stipple_counter = flatshade_reset_stipple_counter;

   return flatshade;
}


