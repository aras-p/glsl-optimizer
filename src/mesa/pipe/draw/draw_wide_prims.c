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

   unsigned hw_data_offset;

   float half_line_width;
   float half_point_size;
};



static INLINE struct wide_stage *wide_stage( struct draw_stage *stage )
{
   return (struct wide_stage *)stage;
}





static void tri( struct draw_stage *next,
		 struct vertex_header *v0,
		 struct vertex_header *v1,
		 struct vertex_header *v2 )
{
   struct prim_header tmp;

   tmp.det = 1.0;
   tmp.v[0] = v0;
   tmp.v[1] = v1;
   tmp.v[2] = v2;
   next->tri( next, &tmp );
}

static void quad( struct draw_stage *next,
		  struct vertex_header *v0,
		  struct vertex_header *v1,
		  struct vertex_header *v2,
		  struct vertex_header *v3 )
{
   /* XXX: Need to disable tri-stipple
    */
   tri( next, v0, v1, v3 );
   tri( next, v2, v0, v3 );
}

static void wide_line( struct draw_stage *stage,
		       struct prim_header *header )
{
   struct wide_stage *wide = wide_stage(stage);
   unsigned hw_data_offset = wide->hw_data_offset;
   float half_width = wide->half_line_width;

   struct vertex_header *v0 = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1 = dup_vert(stage, header->v[0], 1);
   struct vertex_header *v2 = dup_vert(stage, header->v[1], 2);
   struct vertex_header *v3 = dup_vert(stage, header->v[1], 3);

   float *pos0 = (float *)&(v0->data[hw_data_offset]);
   float *pos1 = (float *)&(v1->data[hw_data_offset]);
   float *pos2 = (float *)&(v2->data[hw_data_offset]);
   float *pos3 = (float *)&(v3->data[hw_data_offset]);
   
   float dx = FABSF(pos0[0] - pos2[0]);
   float dy = FABSF(pos0[1] - pos2[1]);
   
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

   quad( stage->next, v0, v1, v2, v3 );
}


static void make_wide_point( struct draw_stage *stage,
			     const struct vertex_header *vin,
			     struct vertex_header *v[] )
{
   struct wide_stage *wide = wide_stage(stage);
   unsigned hw_data_offset = wide->hw_data_offset;
   float half_size = wide->half_point_size;
   float *pos[4];

   v[0] = dup_vert(stage, vin, 0);
   pos[0] = (float *)&(v[0]->data[hw_data_offset]);

   /* Probably not correct:
    */
   pos[0][0] = pos[0][0];
   pos[0][1] = pos[0][1] - .5;

   v[1] = dup_vert(stage, v[0], 1);
   v[2] = dup_vert(stage, v[0], 2);
   v[3] = dup_vert(stage, v[0], 3);

   pos[1] = (float *)&(v[1]->data[hw_data_offset]);
   pos[2] = (float *)&(v[2]->data[hw_data_offset]);
   pos[3] = (float *)&(v[3]->data[hw_data_offset]);
   
   _mesa_printf("point %f %f, %f\n", pos[0][0], pos[0][1], half_size);

   pos[0][0] -= half_size;
   pos[0][1] -= half_size;

   pos[1][0] -= half_size;
   pos[1][1] += half_size;

   pos[2][0] += half_size;
   pos[2][1] -= half_size;

   pos[3][0] += half_size;
   pos[3][1] += half_size;
   
//   quad( stage->next, v[0], v[1], v[2], v[3] );
}

static void wide_point( struct draw_stage *stage,
			struct prim_header *header )
{
   struct vertex_header *v[4];
   make_wide_point(stage, header->v[0], v );
   quad( stage->next, v[0], v[1], v[2], v[3] );
}


static void set_texcoord( struct vertex_fetch *vf,			   
			  struct vertex_header *v,
			  const float *val )
{ 
   GLubyte *dst = (GLubyte *)v;
   const struct vf_attr *a = vf->attr;
   const unsigned attr_count = vf->attr_count;
   unsigned j;

   /* XXX: precompute which attributes need to be set.
    */
   for (j = 1; j < attr_count; j++) {
      if (a[j].attrib >= VF_ATTRIB_TEX0 &&
	  a[j].attrib <= VF_ATTRIB_TEX7)
	 a[j].insert[4-1]( &a[j], dst + a[j].vertoffset, val );
   }
}



/* If there are lots of sprite points (and why wouldn't there be?) it
 * would probably be more sensible to change hardware setup to
 * optimize this rather than doing the whole thing in software like
 * this.
 */
static void sprite_point( struct draw_stage *stage,
		       struct prim_header *header )
{
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
}



static void wide_begin( struct draw_stage *stage )
{
   struct wide_stage *wide = wide_stage(stage);
   struct clip_context *draw = stage->pipe->draw;

   if (draw->vb_state.clipped_prims)
      wide->hw_data_offset = 16;
   else
      wide->hw_data_offset = 0;	

   wide->half_point_size = draw->state.point_size / 2;
   wide->half_line_width = draw->state.line_width / 2;

   if (draw->state.line_width != 1.0) {
      wide->stage.line = wide_line;
   }
   else {
      wide->stage.line = clip_passthrough_line;
   }

   if (draw->state.point_sprite) {
      wide->stage.point = sprite_point;
   }
   else if (draw->state.point_size != 1.0) {
      wide->stage.point = wide_point;
   }
   else {
      wide->stage.point = clip_passthrough_point;
   }

   
   stage->next->begin( stage->next );
}



static void wide_end( struct draw_stage *stage )
{
   stage->next->end( stage->next );
}

struct draw_stage *clip_pipe_wide( struct clip_pipeline *pipe )
{
   struct wide_stage *wide = CALLOC_STRUCT(wide_stage);

   clip_pipe_alloc_tmps( &wide->stage, 4 );

   wide->stage.pipe = pipe;
   wide->stage.next = NULL;
   wide->stage.begin = wide_begin;
   wide->stage.point = wide_point;
   wide->stage.line = wide_line;
   wide->stage.tri = clip_passthrough_tri;
   wide->stage.reset_tmps = clip_pipe_reset_tmps;
   wide->stage.end = wide_end;

   return &wide->stage;
}
