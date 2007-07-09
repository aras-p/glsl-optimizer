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
#include "imports.h"

#include "sp_context.h"
#include "sp_prim.h"
#include "pipe/p_defines.h"


struct unfilled_stage {
   struct prim_stage stage;

   GLuint mode[2];
};


static INLINE struct unfilled_stage *unfilled_stage( struct prim_stage *stage )
{
   return (struct unfilled_stage *)stage;
}


static void unfilled_begin( struct prim_stage *stage )
{
   struct unfilled_stage *unfilled = unfilled_stage(stage);

   unfilled->mode[0] = stage->softpipe->setup.fill_ccw;
   unfilled->mode[1] = stage->softpipe->setup.fill_cw;

   stage->next->begin( stage->next );
}

static void point( struct prim_stage *stage,
		   struct vertex_header *v0 )
{
   struct prim_header tmp;
   tmp.v[0] = v0;
   stage->next->point( stage->next, &tmp );
}

static void line( struct prim_stage *stage,
		  struct vertex_header *v0,
		  struct vertex_header *v1 )
{
   struct prim_header tmp;
   tmp.v[0] = v0;
   tmp.v[1] = v1;
   stage->next->line( stage->next, &tmp );
}


static void points( struct prim_stage *stage,
		    struct prim_header *header )
{
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];
   struct vertex_header *v2 = header->v[2];

   if (v0->edgeflag) point( stage, v0 );
   if (v1->edgeflag) point( stage, v1 );
   if (v2->edgeflag) point( stage, v2 );
}

static void lines( struct prim_stage *stage,
		   struct prim_header *header )
{
   struct vertex_header *v0 = header->v[0];
   struct vertex_header *v1 = header->v[1];
   struct vertex_header *v2 = header->v[2];

   if (v0->edgeflag) line( stage, v0, v1 );
   if (v1->edgeflag) line( stage, v1, v2 );
   if (v2->edgeflag) line( stage, v2, v0 );
}


/* Unfilled tri:  
 *
 * Note edgeflags in the vertex struct is not sufficient as we will
 * need to manipulate them when decomposing primitives???
 */
static void unfilled_tri( struct prim_stage *stage,
			  struct prim_header *header )
{
   struct unfilled_stage *unfilled = unfilled_stage(stage);
   GLuint mode = unfilled->mode[header->det < 0];
  
   switch (mode) {
   case PIPE_POLYGON_MODE_FILL:
      stage->next->tri( stage->next, header );
      break;
   case PIPE_POLYGON_MODE_LINE:
      lines( stage, header );
      break;
   case PIPE_POLYGON_MODE_POINT:
      points( stage, header );
      break;
   default:
      abort();
   }   
}

static void unfilled_line( struct prim_stage *stage,
		       struct prim_header *header )
{
   stage->next->line( stage->next, header );
}


static void unfilled_point( struct prim_stage *stage,
			struct prim_header *header )
{
   stage->next->point( stage->next, header );
}


static void unfilled_end( struct prim_stage *stage )
{
   stage->next->end( stage->next );
}

struct prim_stage *prim_unfilled( struct softpipe_context *softpipe )
{
   struct unfilled_stage *unfilled = CALLOC_STRUCT(unfilled_stage);

   prim_alloc_tmps( &unfilled->stage, 0 );

   unfilled->stage.softpipe = softpipe;
   unfilled->stage.next = NULL;
   unfilled->stage.tmp = NULL;
   unfilled->stage.begin = unfilled_begin;
   unfilled->stage.point = unfilled_point;
   unfilled->stage.line = unfilled_line;
   unfilled->stage.tri = unfilled_tri;
   unfilled->stage.end = unfilled_end;

   return &unfilled->stage;
}
