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
#include "pipe/p_shader_tokens.h"
#include "draw_private.h"


struct wide_stage {
   struct draw_stage stage;

   float half_line_width;
   float half_point_size;

   uint texcoord_slot[PIPE_MAX_SHADER_OUTPUTS];
   uint texcoord_mode[PIPE_MAX_SHADER_OUTPUTS];
   uint num_texcoords;

   int psize_slot;
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
   
   /*
    * Draw wide line as a quad (two tris) by "stretching" the line along
    * X or Y.
    * We need to tweak coords in several ways to be conformant here.
    */

   if (dx > dy) {
      /* x-major line */
      pos0[1] = pos0[1] - half_width - 0.25f;
      pos1[1] = pos1[1] + half_width - 0.25f;
      pos2[1] = pos2[1] - half_width - 0.25f;
      pos3[1] = pos3[1] + half_width - 0.25f;
      if (pos0[0] < pos2[0]) {
         /* left to right line */
         pos0[0] -= 0.5f;
         pos1[0] -= 0.5f;
         pos2[0] -= 0.5f;
         pos3[0] -= 0.5f;
      }
      else {
         /* right to left line */
         pos0[0] += 0.5f;
         pos1[0] += 0.5f;
         pos2[0] += 0.5f;
         pos3[0] += 0.5f;
      }
   }
   else {
      /* y-major line */
      pos0[0] = pos0[0] - half_width + 0.25f;
      pos1[0] = pos1[0] + half_width + 0.25f;
      pos2[0] = pos2[0] - half_width + 0.25f;
      pos3[0] = pos3[0] + half_width + 0.25f;
      if (pos0[1] < pos2[1]) {
         /* top to bottom line */
         pos0[1] -= 0.5f;
         pos1[1] -= 0.5f;
         pos2[1] -= 0.5f;
         pos3[1] -= 0.5f;
      }
      else {
         /* bottom to top line */
         pos0[1] += 0.5f;
         pos1[1] += 0.5f;
         pos2[1] += 0.5f;
         pos3[1] += 0.5f;
      }
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


/**
 * Set the vertex texcoords for sprite mode.
 * Coords may be left untouched or set to a right-side-up or upside-down
 * orientation.
 */
static void set_texcoords(const struct wide_stage *wide,
                          struct vertex_header *v, const float tc[4])
{
   uint i;
   for (i = 0; i < wide->num_texcoords; i++) {
      if (wide->texcoord_mode[i] != PIPE_SPRITE_COORD_NONE) {
         uint j = wide->texcoord_slot[i];
         v->data[j][0] = tc[0];
         if (wide->texcoord_mode[i] == PIPE_SPRITE_COORD_LOWER_LEFT)
            v->data[j][1] = 1.0f - tc[1];
         else
            v->data[j][1] = tc[1];
         v->data[j][2] = tc[2];
         v->data[j][3] = tc[3];
      }
   }
}


/* If there are lots of sprite points (and why wouldn't there be?) it
 * would probably be more sensible to change hardware setup to
 * optimize this rather than doing the whole thing in software like
 * this.
 */
static void wide_point( struct draw_stage *stage,
			struct prim_header *header )
{
   const struct wide_stage *wide = wide_stage(stage);
   const boolean sprite = (boolean) stage->draw->rasterizer->point_sprite;
   float half_size;
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

   /* point size is either per-vertex or fixed size */
   if (wide->psize_slot >= 0) {
      half_size = 0.5f * header->v[0]->data[wide->psize_slot][0];
   }
   else {
      half_size = wide->half_point_size;
   }

   left_adj = -half_size; /* + 0.25f;*/
   right_adj = half_size; /* + 0.25f;*/

   pos0[0] += left_adj;
   pos0[1] -= half_size;

   pos1[0] += left_adj;
   pos1[1] += half_size;

   pos2[0] += right_adj;
   pos2[1] -= half_size;

   pos3[0] += right_adj;
   pos3[1] += half_size;

   if (sprite) {
      static const float tex00[4] = { 0, 0, 0, 1 };
      static const float tex01[4] = { 0, 1, 0, 1 };
      static const float tex11[4] = { 1, 1, 0, 1 };
      static const float tex10[4] = { 1, 0, 0, 1 };
      set_texcoords( wide, v0, tex00 );
      set_texcoords( wide, v1, tex01 );
      set_texcoords( wide, v2, tex10 );
      set_texcoords( wide, v3, tex11 );
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


static void wide_first_point( struct draw_stage *stage, 
			      struct prim_header *header )
{
   struct wide_stage *wide = wide_stage(stage);
   struct draw_context *draw = stage->draw;

   wide->half_point_size = 0.5f * draw->rasterizer->point_size;

   /* XXX we won't know the real size if it's computed by the vertex shader! */
   if (draw->rasterizer->point_size > draw->wide_point_threshold) {
      stage->point = wide_point;
   }
   else {
      stage->point = passthrough_point;
   }

   if (draw->rasterizer->point_sprite) {
      /* find vertex shader texcoord outputs */
      const struct draw_vertex_shader *vs = draw->vertex_shader;
      uint i, j = 0;
      for (i = 0; i < vs->info.num_outputs; i++) {
         if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_GENERIC) {
            wide->texcoord_slot[j] = i;
            wide->texcoord_mode[j] = draw->rasterizer->sprite_coord_mode[j];
            j++;
         }
      }
      wide->num_texcoords = j;
   }

   wide->psize_slot = -1;

   if (draw->rasterizer->point_size_per_vertex) {
      /* find PSIZ vertex output */
      const struct draw_vertex_shader *vs = draw->vertex_shader;
      uint i;
      for (i = 0; i < vs->info.num_outputs; i++) {
         if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_PSIZE) {
            wide->psize_slot = i;
            break;
         }
      }
   }
   
   stage->point( stage, header );
}



static void wide_first_line( struct draw_stage *stage,
			     struct prim_header *header )
{
   struct wide_stage *wide = wide_stage(stage);
   struct draw_context *draw = stage->draw;

   wide->half_line_width = 0.5f * draw->rasterizer->line_width;

   if (draw->rasterizer->line_width != 1.0) {
      wide->stage.line = wide_line;
   }
   else {
      wide->stage.line = passthrough_line;
   }
   
   stage->line( stage, header );
}


static void wide_flush( struct draw_stage *stage, unsigned flags )
{
   stage->line = wide_first_line;
   stage->point = wide_first_point;
   stage->next->flush( stage->next, flags );
}


static void wide_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void wide_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


struct draw_stage *draw_wide_stage( struct draw_context *draw )
{
   struct wide_stage *wide = CALLOC_STRUCT(wide_stage);

   draw_alloc_temp_verts( &wide->stage, 4 );

   wide->stage.draw = draw;
   wide->stage.next = NULL;
   wide->stage.point = wide_first_point;
   wide->stage.line = wide_first_line;
   wide->stage.tri = passthrough_tri;
   wide->stage.flush = wide_flush;
   wide->stage.reset_stipple_counter = wide_reset_stipple_counter;
   wide->stage.destroy = wide_destroy;

   return &wide->stage;
}
