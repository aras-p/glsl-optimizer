/**************************************************************************
 *
 * Copyright 2013 VMware
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
 * \brief  Used to decompose adjacency primitives and inject the prim id
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_defines.h"
#include "draw_pipe.h"
#include "draw_fs.h"
#include "draw_gs.h"


struct ia_stage {
   struct draw_stage stage;
   int primid_slot;
   unsigned primid;
};


static INLINE struct ia_stage *
ia_stage(struct draw_stage *stage)
{
   return (struct ia_stage *)stage;
}


static void
inject_primid(struct draw_stage *stage,
              struct prim_header *header,
              unsigned num_verts)
{
   struct ia_stage *ia = ia_stage(stage);
   unsigned slot = ia->primid_slot;
   unsigned i;
   unsigned primid = ia->primid;

   /* In case the backend doesn't care about it */
   if (slot < 0) {
      return;
   }

   for (i = 0; i < num_verts; ++i) {
      struct vertex_header *v = header->v[i];
      /* We have to reset the vertex_id because it's used by
       * vbuf to figure out if the vertex had already been
       * emitted. For line/tri strips the first vertex of
       * subsequent primitives would already be emitted,
       * but since we're changing the primitive id on the vertex
       * we want to make sure it's reemitted with the correct
       * data.
       */
      v->vertex_id = UNDEFINED_VERTEX_ID;
      memcpy(&v->data[slot][0], &primid, sizeof(primid));
      memcpy(&v->data[slot][1], &primid, sizeof(primid));
      memcpy(&v->data[slot][2], &primid, sizeof(primid));
      memcpy(&v->data[slot][3], &primid, sizeof(primid));
   }
   ++ia->primid;
}


static void
ia_point(struct draw_stage *stage,
         struct prim_header *header)
{
   inject_primid(stage, header, 1);
   stage->next->point(stage->next, header);
}

static void
ia_line(struct draw_stage *stage,
        struct prim_header *header)
{
   inject_primid(stage, header, 2);
   stage->next->line(stage->next, header);
}

static void
ia_tri(struct draw_stage *stage,
       struct prim_header *header)
{
   inject_primid(stage, header, 3);
   stage->next->tri(stage->next, header);
}

static void
ia_first_point(struct draw_stage *stage,
               struct prim_header *header)
{
   struct ia_stage *ia = ia_stage(stage);

   if (ia->primid_slot >= 0) {
      stage->point = ia_point;
   } else {
      stage->point = draw_pipe_passthrough_point;
   }

   stage->point(stage, header);
}

static void
ia_first_line(struct draw_stage *stage,
              struct prim_header *header)
{
   struct ia_stage *ia = ia_stage(stage);

   if (ia->primid_slot >= 0) {
      stage->line = ia_line;
   } else {
      stage->line = draw_pipe_passthrough_line;
   }

   stage->line(stage, header);
}

static void
ia_first_tri(struct draw_stage *stage,
             struct prim_header *header)
{
   struct ia_stage *ia = ia_stage(stage);

   if (ia->primid_slot >= 0) {
      stage->tri = ia_tri;
   } else {
      stage->tri = draw_pipe_passthrough_tri;
   }

   stage->tri(stage, header);
}


static void
ia_flush(struct draw_stage *stage, unsigned flags)
{
   stage->point = ia_first_point;
   stage->line = ia_first_line;
   stage->tri = ia_first_tri;
   stage->next->flush(stage->next, flags);
}


static void
ia_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter(stage->next);
}


static void
ia_destroy(struct draw_stage *stage)
{
   draw_free_temp_verts(stage);
   FREE(stage);
}


static boolean
needs_primid(const struct draw_context *draw)
{
   const struct draw_fragment_shader *fs = draw->fs.fragment_shader;
   const struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   if (fs && fs->info.uses_primid) {
      return !gs || !gs->info.uses_primid;
   }
   return FALSE;
}

boolean
draw_ia_stage_required(const struct draw_context *draw, unsigned prim)
{
   const struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   if (needs_primid(draw)) {
      return TRUE;
   }

   if (gs) {
      return FALSE;
   }

   switch (prim) {
   case PIPE_PRIM_LINES_ADJACENCY:
   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
   case PIPE_PRIM_TRIANGLES_ADJACENCY:
   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      return TRUE;
   default:
      return FALSE;
   }
}

void
draw_ia_prepare_outputs(struct draw_context *draw,
                        struct draw_stage *stage)
{
   struct ia_stage *ia = ia_stage(stage);
   if (needs_primid(draw)) {
      ia->primid_slot = draw_alloc_extra_vertex_attrib(
         stage->draw, TGSI_SEMANTIC_PRIMID, 0);
   } else {
      ia->primid_slot = -1;
   }
   ia->primid = 0;
}

struct draw_stage *
draw_ia_stage(struct draw_context *draw)
{
   struct ia_stage *ia = CALLOC_STRUCT(ia_stage);
   if (ia == NULL)
      goto fail;

   ia->stage.draw = draw;
   ia->stage.name = "ia";
   ia->stage.next = NULL;
   ia->stage.point = ia_first_point;
   ia->stage.line = ia_first_line;
   ia->stage.tri = ia_first_tri;
   ia->stage.flush = ia_flush;
   ia->stage.reset_stipple_counter = ia_reset_stipple_counter;
   ia->stage.destroy = ia_destroy;

   if (!draw_alloc_temp_verts(&ia->stage, 0))
      goto fail;

   return &ia->stage;

fail:
   if (ia)
      ia->stage.destroy(&ia->stage);

   return NULL;
}
