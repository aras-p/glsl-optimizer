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

#include "main/imports.h"
#include "draw_private.h"


struct flatshade_stage {
   struct prim_stage stage;

   const GLuint *lookup;
};



static INLINE struct flatshade_stage *flatshade_stage( struct prim_stage *stage )
{
   return (struct flatshade_stage *)stage;
}


static void flatshade_begin( struct prim_stage *stage )
{
   stage->next->begin( stage->next );
}



static INLINE void copy_attr( GLuint attr,
			      struct vertex_header *dst, 
			      const struct vertex_header *src )
{
   if (attr) {
      memcpy( dst->data[attr],
	      src->data[attr],
	      sizeof(src->data[0]) );
   }
}


static INLINE void copy_colors( struct prim_stage *stage, 
                                struct vertex_header *dst, 
                                const struct vertex_header *src )
{
   const struct flatshade_stage *flatshade = flatshade_stage(stage);
   const GLuint *lookup = flatshade->lookup;

   copy_attr( lookup[VF_ATTRIB_COLOR0], dst, src );
   copy_attr( lookup[VF_ATTRIB_COLOR1], dst, src );
   copy_attr( lookup[VF_ATTRIB_BFC0], dst, src );
   copy_attr( lookup[VF_ATTRIB_BFC1], dst, src );
}


/**
 * Flatshade tri.  Required for clipping and when unfilled tris are
 * active, otherwise handled by hardware.
 */
static void flatshade_tri( struct prim_stage *stage,
			   struct prim_header *header )
{
   struct prim_header tmp;

   tmp.det = header->det;
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
static void flatshade_line( struct prim_stage *stage,
			    struct prim_header *header )
{
   struct prim_header tmp;

   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = header->v[1];

   copy_colors(stage, tmp.v[0], tmp.v[1]);
   
   stage->next->line( stage->next, &tmp );
}


static void flatshade_point( struct prim_stage *stage,
			struct prim_header *header )
{
   stage->next->point( stage->next, header );
}


static void flatshade_end( struct prim_stage *stage )
{
   stage->next->end( stage->next );
}


struct prim_stage *prim_flatshade( struct draw_context *draw )
{
   struct flatshade_stage *flatshade = CALLOC_STRUCT(flatshade_stage);

   prim_alloc_tmps( &flatshade->stage, 2 );

   flatshade->stage.draw = draw;
   flatshade->stage.next = NULL;
   flatshade->stage.begin = flatshade_begin;
   flatshade->stage.point = flatshade_point;
   flatshade->stage.line = flatshade_line;
   flatshade->stage.tri = flatshade_tri;
   flatshade->stage.end = flatshade_end;

   flatshade->lookup = draw->vf_attr_to_slot;

   return &flatshade->stage;
}


