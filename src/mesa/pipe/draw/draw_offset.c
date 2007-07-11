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

/**
 * \brief  polygon offset state
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */

#include "main/imports.h"
#include "main/macros.h"
#include "draw_private.h"



struct offset_stage {
   struct prim_stage stage;

   GLfloat scale;
   GLfloat units;
};



static INLINE struct offset_stage *offset_stage( struct prim_stage *stage )
{
   return (struct offset_stage *) stage;
}


static void offset_begin( struct prim_stage *stage )
{
   struct offset_stage *offset = offset_stage(stage);
   GLfloat mrd = 1.0 / 65535.0; /* XXX this depends on depthbuffer bits! */

   offset->units = stage->draw->setup.offset_units * mrd;
   offset->scale = stage->draw->setup.offset_scale;

   stage->next->begin( stage->next );
}


/**
 * Offset tri Z.  Some hardware can handle this, but not usually when
 * doing unfilled rendering.
 */
static void do_offset_tri( struct prim_stage *stage,
			   struct prim_header *header )
{
   struct offset_stage *offset = offset_stage(stage);   
   GLfloat inv_det = 1.0 / header->det;

   /* Window coords:
    */
   GLfloat *v0 = header->v[0]->data[0];
   GLfloat *v1 = header->v[1]->data[0];
   GLfloat *v2 = header->v[2]->data[0];

   /* edge vectors e = v0 - v2, f = v1 - v2 */
   GLfloat ex = v0[0] - v2[0];
   GLfloat ey = v0[1] - v2[1];
   GLfloat ez = v0[2] - v2[2];
   GLfloat fx = v1[0] - v2[0];
   GLfloat fy = v1[1] - v2[1];
   GLfloat fz = v1[2] - v2[2];

   /* (a,b) = cross(e,f).xy */
   GLfloat a = ey*fz - ez*fy;
   GLfloat b = ez*fx - ex*fz;

   GLfloat dzdx = FABSF(a * inv_det);
   GLfloat dzdy = FABSF(b * inv_det);

   GLfloat zoffset = offset->units + MAX2(dzdx, dzdy) * offset->scale;

   /*
    * Note: we're applying the offset and clamping per-vertex.
    * Ideally, the offset is applied per-fragment prior to fragment shading.
    */
   v0[2] = CLAMP(v0[2] + zoffset, 0.0, 1.0);
   v1[2] = CLAMP(v1[2] + zoffset, 0.0, 1.0);
   v2[2] = CLAMP(v2[2] + zoffset, 0.0, 1.0);

   stage->next->tri( stage->next, header );
}


static void offset_tri( struct prim_stage *stage,
			struct prim_header *header )
{
   struct prim_header tmp;

   tmp.det = header->det;
   tmp.v[0] = dup_vert(stage, header->v[0], 0);
   tmp.v[1] = dup_vert(stage, header->v[1], 1);
   tmp.v[2] = dup_vert(stage, header->v[2], 2);

   do_offset_tri( stage, &tmp );
}



static void offset_line( struct prim_stage *stage,
		       struct prim_header *header )
{
   stage->next->line( stage->next, header );
}


static void offset_point( struct prim_stage *stage,
			struct prim_header *header )
{
   stage->next->point( stage->next, header );
}


static void offset_end( struct prim_stage *stage )
{
   stage->next->end( stage->next );
}

struct prim_stage *prim_offset( struct draw_context *draw )
{
   struct offset_stage *offset = CALLOC_STRUCT(offset_stage);

   prim_alloc_tmps( &offset->stage, 3 );

   offset->stage.draw = draw;
   offset->stage.next = NULL;
   offset->stage.begin = offset_begin;
   offset->stage.point = offset_point;
   offset->stage.line = offset_line;
   offset->stage.tri = offset_tri;
   offset->stage.end = offset_end;

   return &offset->stage;
}
