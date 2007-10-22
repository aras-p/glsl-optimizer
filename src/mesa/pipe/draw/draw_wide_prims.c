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
#include "pipe/p_defines.h"
#include "draw_private.h"



struct wide_stage {
   struct draw_stage stage;

   float half_line_width;
   float half_point_size;
};



static INLINE struct wide_stage *wide_stage( struct draw_stage *stage )
{
   return (struct wide_stage *)stage;
}


static void passthrough_point( struct draw_stage *stage,
                               struct prim_header *header )
{
   stage->next->point( stage->next, header );
}

static void passthrough_line( struct draw_stage *stage,
                              struct prim_header *header )
{
   stage->next->line(stage->next, header);
}

static void passthrough_tri( struct draw_stage *stage,
                             struct prim_header *header )
{
   stage->next->tri(stage->next, header);
}


/**
 * Draw a wide line by drawing a quad (two triangles).
 * XXX still need line stipple.
 * XXX need to disable polygon stipple.
 */
static void wide_line( struct draw_stage *stage,
		       struct prim_header *header )
{
   const struct wide_stage *wide = wide_stage(stage);
   const float half_width = wide->half_line_width;

   struct prim_header tri;

   struct vertex_header *v0 = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1 = dup_vert(stage, header->v[0], 1);
   struct vertex_header *v2 = dup_vert(stage, header->v[1], 2);
   struct vertex_header *v3 = dup_vert(stage, header->v[1], 3);

   float *pos0 = v0->data[0];
   float *pos1 = v1->data[0];
   float *pos2 = v2->data[0];
   float *pos3 = v3->data[0];

   const float dx = FABSF(pos0[0] - pos2[0]);
   const float dy = FABSF(pos0[1] - pos2[1]);
   
   if (dx > dy) {
      pos0[1] -= half_width;
      pos1[1] += half_width;
      pos2[1] -= half_width;
      pos3[1] += half_width;
   }
   else {
      pos0[0] -= half_width;
      pos1[0] += half_width;
      pos2[0] -= half_width;
      pos3[0] += half_width;
   }

   tri.det = header->det;  /* only the sign matters */
   tri.v[0] = v0;
   tri.v[1] = v2;
   tri.v[2] = v3;
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v0;
   tri.v[1] = v3;
   tri.v[2] = v1;
   stage->next->tri( stage->next, &tri );
}


static void wide_point( struct draw_stage *stage,
			struct prim_header *header )
{
   const struct wide_stage *wide = wide_stage(stage);
   float half_size = wide->half_point_size;
   float left_adj, right_adj;

   struct prim_header tri;

   /* four dups of original vertex */
   struct vertex_header *v0 = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1 = dup_vert(stage, header->v[0], 1);
   struct vertex_header *v2 = dup_vert(stage, header->v[0], 2);
   struct vertex_header *v3 = dup_vert(stage, header->v[0], 3);

   float *pos0 = v0->data[0];
   float *pos1 = v1->data[0];
   float *pos2 = v2->data[0];
   float *pos3 = v3->data[0];

   left_adj = -half_size + 0.25;
   right_adj = half_size + 0.25;

   pos0[0] += left_adj;
   pos0[1] -= half_size;

   pos1[0] += left_adj;
   pos1[1] += half_size;

   pos2[0] += right_adj;
   pos2[1] -= half_size;

   pos3[0] += right_adj;
   pos3[1] += half_size;

   tri.det = header->det;  /* only the sign matters */
   tri.v[0] = v0;
   tri.v[1] = v2;
   tri.v[2] = v3;
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v0;
   tri.v[1] = v3;
   tri.v[2] = v1;
   stage->next->tri( stage->next, &tri );
}


/* If there are lots of sprite points (and why wouldn't there be?) it
 * would probably be more sensible to change hardware setup to
 * optimize this rather than doing the whole thing in software like
 * this.
 */
static void sprite_point( struct draw_stage *stage,
		       struct prim_header *header )
{
#if 0
   struct vertex_header *v[4];
   struct vertex_fetch *vf = stage->pipe->draw->vb.vf;

   static const float tex00[4] = { 0, 0, 0, 1 };
   static const float tex01[4] = { 0, 1, 0, 1 };
   static const float tex11[4] = { 1, 1, 0, 1 };
   static const float tex10[4] = { 1, 0, 0, 1 };

   make_wide_point(stage, header->v[0], &v[0] );

   set_texcoord( vf, v[0], tex00 );
   set_texcoord( vf, v[1], tex01 );
   set_texcoord( vf, v[2], tex10 );
   set_texcoord( vf, v[3], tex11 );

   quad( stage->next, v[0], v[1], v[2], v[3] );
#endif
}



static void wide_begin( struct draw_stage *stage )
{
   struct wide_stage *wide = wide_stage(stage);
   struct draw_context *draw = stage->draw;

   wide->half_point_size = 0.5 * draw->rasterizer->point_size;
   wide->half_line_width = 0.5 * draw->rasterizer->line_width;

   if (draw->rasterizer->line_width != 1.0) {
      wide->stage.line = wide_line;
   }
   else {
      wide->stage.line = passthrough_line;
   }

   if (0/*draw->state.point_sprite*/) {
      wide->stage.point = sprite_point;
   }
   else if (draw->rasterizer->point_size != 1.0) {
      wide->stage.point = wide_point;
   }
   else {
      wide->stage.point = passthrough_point;
   }

   
   stage->next->begin( stage->next );
}



static void wide_end( struct draw_stage *stage )
{
   stage->next->end( stage->next );
}


static void draw_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}

struct draw_stage *draw_wide_stage( struct draw_context *draw )
{
   struct wide_stage *wide = CALLOC_STRUCT(wide_stage);

   draw_alloc_tmps( &wide->stage, 4 );

   wide->stage.draw = draw;
   wide->stage.next = NULL;
   wide->stage.begin = wide_begin;
   wide->stage.point = wide_point;
   wide->stage.line = wide_line;
   wide->stage.tri = passthrough_tri;
   wide->stage.end = wide_end;
   wide->stage.reset_stipple_counter = draw_reset_stipple_counter;

   return &wide->stage;
}
