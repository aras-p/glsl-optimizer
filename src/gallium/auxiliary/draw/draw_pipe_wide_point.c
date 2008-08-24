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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "draw_vs.h"
#include "draw_pipe.h"


struct widepoint_stage {
   struct draw_stage stage;

   float half_point_size;
   float point_size_min;
   float point_size_max;

   float xbias;
   float ybias;

   uint texcoord_slot[PIPE_MAX_SHADER_OUTPUTS];
   uint texcoord_mode[PIPE_MAX_SHADER_OUTPUTS];
   uint num_texcoords;

   int psize_slot;

   int point_coord_fs_input;  /**< input for pointcoord (and fog) */
};



static INLINE struct widepoint_stage *
widepoint_stage( struct draw_stage *stage )
{
   return (struct widepoint_stage *)stage;
}




/**
 * Set the vertex texcoords for sprite mode.
 * Coords may be left untouched or set to a right-side-up or upside-down
 * orientation.
 */
static void set_texcoords(const struct widepoint_stage *wide,
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

   if (wide->point_coord_fs_input >= 0) {
      /* put gl_PointCoord into extra vertex output's zw components */
      uint k = wide->stage.draw->extra_vp_outputs.slot;
      v->data[k][2] = tc[0];
      v->data[k][3] = tc[1];
   }
}


/* If there are lots of sprite points (and why wouldn't there be?) it
 * would probably be more sensible to change hardware setup to
 * optimize this rather than doing the whole thing in software like
 * this.
 */
static void widepoint_point( struct draw_stage *stage,
                             struct prim_header *header )
{
   const struct widepoint_stage *wide = widepoint_stage(stage);
   const unsigned pos = stage->draw->vs.position_output;
   const boolean sprite = (boolean) stage->draw->rasterizer->point_sprite;
   float half_size;
   float left_adj, right_adj, bot_adj, top_adj;

   struct prim_header tri;

   /* four dups of original vertex */
   struct vertex_header *v0 = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1 = dup_vert(stage, header->v[0], 1);
   struct vertex_header *v2 = dup_vert(stage, header->v[0], 2);
   struct vertex_header *v3 = dup_vert(stage, header->v[0], 3);

   float *pos0 = v0->data[pos];
   float *pos1 = v1->data[pos];
   float *pos2 = v2->data[pos];
   float *pos3 = v3->data[pos];

   /* point size is either per-vertex or fixed size */
   if (wide->psize_slot >= 0) {
      half_size = header->v[0]->data[wide->psize_slot][0];

      /* XXX: temporary -- do this in the vertex shader??
       */
      half_size = CLAMP(half_size,
                        wide->point_size_min,
                        wide->point_size_max);
      
      half_size *= 0.5f; 
   }
   else {
      half_size = wide->half_point_size;
   }

   left_adj = -half_size + wide->xbias;
   right_adj = half_size + wide->xbias;
   bot_adj = half_size + wide->ybias;
   top_adj = -half_size + wide->ybias;

   pos0[0] += left_adj;
   pos0[1] += top_adj;

   pos1[0] += left_adj;
   pos1[1] += bot_adj;

   pos2[0] += right_adj;
   pos2[1] += top_adj;

   pos3[0] += right_adj;
   pos3[1] += bot_adj;

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


static void widepoint_first_point( struct draw_stage *stage, 
			      struct prim_header *header )
{
   struct widepoint_stage *wide = widepoint_stage(stage);
   struct draw_context *draw = stage->draw;

   wide->half_point_size = 0.5f * draw->rasterizer->point_size;
   wide->point_size_min = draw->rasterizer->point_size_min;
   wide->point_size_max = draw->rasterizer->point_size_max;
   wide->xbias = 0.0;
   wide->ybias = 0.0;

   if (draw->rasterizer->gl_rasterization_rules) {
      wide->xbias = 0.125;
   }

   /* XXX we won't know the real size if it's computed by the vertex shader! */
   if ((draw->rasterizer->point_size > draw->pipeline.wide_point_threshold) ||
       (draw->rasterizer->point_sprite && draw->pipeline.point_sprite)) {
      stage->point = widepoint_point;
   }
   else {
      stage->point = draw_pipe_passthrough_point;
   }

   if (draw->rasterizer->point_sprite) {
      /* find vertex shader texcoord outputs */
      const struct draw_vertex_shader *vs = draw->vs.vertex_shader;
      uint i, j = 0;
      for (i = 0; i < vs->info.num_outputs; i++) {
         if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_GENERIC) {
            wide->texcoord_slot[j] = i;
            wide->texcoord_mode[j] = draw->rasterizer->sprite_coord_mode[j];
            j++;
         }
      }
      wide->num_texcoords = j;

      /* find fragment shader PointCoord/Fog input */
      wide->point_coord_fs_input = 0; /* XXX fix this! */

      /* setup extra vp output */
      draw->extra_vp_outputs.semantic_name = TGSI_SEMANTIC_FOG;
      draw->extra_vp_outputs.semantic_index = 0;
      draw->extra_vp_outputs.slot = draw->vs.num_vs_outputs;
   }
   else {
      wide->point_coord_fs_input = -1;
      draw->extra_vp_outputs.slot = 0;
   }

   wide->psize_slot = -1;
   if (draw->rasterizer->point_size_per_vertex) {
      /* find PSIZ vertex output */
      const struct draw_vertex_shader *vs = draw->vs.vertex_shader;
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


static void widepoint_flush( struct draw_stage *stage, unsigned flags )
{
   stage->point = widepoint_first_point;
   stage->next->flush( stage->next, flags );
}


static void widepoint_reset_stipple_counter( struct draw_stage *stage )
{
   stage->next->reset_stipple_counter( stage->next );
}


static void widepoint_destroy( struct draw_stage *stage )
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


struct draw_stage *draw_wide_point_stage( struct draw_context *draw )
{
   struct widepoint_stage *wide = CALLOC_STRUCT(widepoint_stage);
   if (wide == NULL)
      goto fail;

   if (!draw_alloc_temp_verts( &wide->stage, 4 ))
      goto fail;

   wide->stage.draw = draw;
   wide->stage.next = NULL;
   wide->stage.point = widepoint_first_point;
   wide->stage.line = draw_pipe_passthrough_line;
   wide->stage.tri = draw_pipe_passthrough_tri;
   wide->stage.flush = widepoint_flush;
   wide->stage.reset_stipple_counter = widepoint_reset_stipple_counter;
   wide->stage.destroy = widepoint_destroy;

   return &wide->stage;

 fail:
   if (wide)
      wide->stage.destroy( &wide->stage );
   
   return NULL;
}
