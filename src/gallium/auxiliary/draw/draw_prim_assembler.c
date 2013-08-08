/**************************************************************************
 *
 * Copyright 2013 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "draw_prim_assembler.h"

#include "draw_fs.h"
#include "draw_gs.h"

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_prim.h"

#include "pipe/p_defines.h"

struct draw_assembler
{
   struct draw_context *draw;

   struct draw_prim_info *output_prims;
   struct draw_vertex_info *output_verts;

   const struct draw_prim_info *input_prims;
   const struct draw_vertex_info *input_verts;

   boolean needs_primid;
   int primid_slot;
   unsigned primid;

   boolean is_strip;
   boolean is_first_prim;
   unsigned num_prims;
};


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
draw_prim_assembler_is_required(const struct draw_context *draw,
                                const struct draw_prim_info *prim_info,
                                const struct draw_vertex_info *vert_info)
{
   switch (prim_info->prim) {
   case PIPE_PRIM_LINES_ADJACENCY:
   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
   case PIPE_PRIM_TRIANGLES_ADJACENCY:
   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      return TRUE;
   default:
      return needs_primid(draw);
   }
}

/*
 * Copy the vertex header along with its data from the current
 * vertex buffer into a buffer holding vertices arranged
 * into decomposed primitives (i.e. buffer without the
 * adjacency vertices)
 */
static void
copy_verts(struct draw_assembler *asmblr,
           unsigned *indices, unsigned num_indices)
{
   unsigned i;

   char *output = (char*)asmblr->output_verts->verts;
   const char *input = (const char*)asmblr->input_verts->verts;

   for (i = 0; i < num_indices; ++i) {
      unsigned idx = indices[i];
      unsigned output_offset =
         asmblr->output_verts->count * asmblr->output_verts->stride;
      unsigned input_offset = asmblr->input_verts->stride * idx;
      memcpy(output + output_offset, input + input_offset,
             asmblr->input_verts->vertex_size);
      asmblr->output_verts->count += 1;
   }
   ++asmblr->num_prims;
}


static void
inject_primid(struct draw_assembler *asmblr,
                      unsigned idx,
                      unsigned primid)
{
   int slot = asmblr->primid_slot;
   char *input = (char*)asmblr->input_verts->verts;
   unsigned input_offset = asmblr->input_verts->stride * idx;
   struct vertex_header *v = (struct vertex_header*)(input + input_offset);

   /* In case the backend doesn't care about it */
   if (slot < 0) {
      return;
   }

   memcpy(&v->data[slot][0], &primid, sizeof(primid));
   memcpy(&v->data[slot][1], &primid, sizeof(primid));
   memcpy(&v->data[slot][2], &primid, sizeof(primid));
   memcpy(&v->data[slot][3], &primid, sizeof(primid));
}


static void
prim_point(struct draw_assembler *asmblr,
           unsigned idx)
{
   unsigned indices[1];

   if (asmblr->needs_primid) {
      inject_primid(asmblr, idx, asmblr->primid++);
   }
   indices[0] = idx;
   
   copy_verts(asmblr, indices, 1);
}

static void
prim_line(struct draw_assembler *asmblr,
          unsigned i0, unsigned i1)
{
   unsigned indices[2];

   if (asmblr->needs_primid) {
      if (asmblr->is_strip && asmblr->is_first_prim) {
         inject_primid(asmblr, i0, asmblr->primid++);
         inject_primid(asmblr, i1, asmblr->primid++);
         asmblr->is_first_prim = FALSE;
      } else if (asmblr->is_strip) {
            inject_primid(asmblr, i1, asmblr->primid++);
      } else {
         inject_primid(asmblr, i0, asmblr->primid);
         inject_primid(asmblr, i1, asmblr->primid++);
      }
   }
   indices[0] = i0;
   indices[1] = i1;

   copy_verts(asmblr, indices, 2);
}

static void
prim_line_adj(struct draw_assembler *asmblr,
              unsigned i0, unsigned i1, unsigned i2, unsigned i3)
{
   unsigned indices[2];

   if (asmblr->needs_primid) {
      if (asmblr->is_strip && asmblr->is_first_prim) {
         inject_primid(asmblr, i1, asmblr->primid++);
         inject_primid(asmblr, i2, asmblr->primid++);
         asmblr->is_first_prim = FALSE;
      } else if (asmblr->is_strip) {
         inject_primid(asmblr, i2, asmblr->primid++);
      } else {
         inject_primid(asmblr, i1, asmblr->primid);
         inject_primid(asmblr, i2, asmblr->primid++);
      }
   }

   indices[0] = i1;
   indices[1] = i2;

   copy_verts(asmblr, indices, 2);
}

static void
prim_tri(struct draw_assembler *asmblr,
         unsigned i0, unsigned i1, unsigned i2)
{
   unsigned indices[3];

   if (asmblr->needs_primid) {
      if (asmblr->is_strip && asmblr->is_first_prim) {
         inject_primid(asmblr, i0, asmblr->primid++);
         inject_primid(asmblr, i1, asmblr->primid++);
         inject_primid(asmblr, i2, asmblr->primid++);
         asmblr->is_first_prim = FALSE;
      } else if (asmblr->is_strip) {
         if (asmblr->num_prims & 1) {
            inject_primid(asmblr, i1, asmblr->primid++);
         } else {
            inject_primid(asmblr, i2, asmblr->primid++);
         }
      } else {
         inject_primid(asmblr, i0, asmblr->primid);
         inject_primid(asmblr, i1, asmblr->primid);
         inject_primid(asmblr, i2, asmblr->primid++);
      }
   }
   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;

   copy_verts(asmblr, indices, 3);
}

static void
prim_tri_adj(struct draw_assembler *asmblr,
             unsigned i0, unsigned i1, unsigned i2,
             unsigned i3, unsigned i4, unsigned i5)
{
   unsigned indices[3];

   if (asmblr->needs_primid) {
      if (asmblr->is_strip && asmblr->is_first_prim) {
         inject_primid(asmblr, i0, asmblr->primid++);
         inject_primid(asmblr, i2, asmblr->primid++);
         inject_primid(asmblr, i4, asmblr->primid++);
         asmblr->is_first_prim = FALSE;
      } else if (asmblr->is_strip) {
         if (asmblr->num_prims & 1) {
            inject_primid(asmblr, i2, asmblr->primid++);
         } else {
            inject_primid(asmblr, i4, asmblr->primid++);
         }
      } else {
         inject_primid(asmblr, i0, asmblr->primid);
         inject_primid(asmblr, i2, asmblr->primid);
         inject_primid(asmblr, i4, asmblr->primid);
         asmblr->primid++;
      }
   }
   indices[0] = i0;
   indices[1] = i2;
   indices[2] = i4;

   copy_verts(asmblr, indices, 3);
}

void
draw_prim_assembler_prepare_outputs(struct draw_assembler *ia)
{
   struct draw_context *draw = ia->draw;
   if (needs_primid(draw)) {
      ia->primid_slot = draw_alloc_extra_vertex_attrib(
         ia->draw, TGSI_SEMANTIC_PRIMID, 0);
   } else {
      ia->primid_slot = -1;
   }
   ia->primid = 0;
}


#define FUNC assembler_run_linear
#define GET_ELT(idx) (start + (idx))
#include "draw_prim_assembler_tmp.h"

#define FUNC assembler_run_elts
#define LOCAL_VARS   const ushort *elts = input_prims->elts;
#define GET_ELT(idx) (elts[start + (idx)])
#include "draw_prim_assembler_tmp.h"



/*
 * Primitive assembler breaks up adjacency primitives and assembles
 * the base primitives they represent, e.g. vertices forming
 * PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY
 * become vertices forming PIPE_PRIM_TRIANGLES 
 * This is needed because specification says that the adjacency
 * primitives are only visible in the geometry shader so we need
 * to get rid of them so that the rest of the pipeline can
 * process the inputs.
 */
void
draw_prim_assembler_run(struct draw_context *draw,
                        const struct draw_prim_info *input_prims,
                        const struct draw_vertex_info *input_verts,
                        struct draw_prim_info *output_prims,
                        struct draw_vertex_info *output_verts)
{
   struct draw_assembler *asmblr = draw->ia;
   unsigned start, i;
   unsigned assembled_prim = u_assembled_prim(input_prims->prim);
   unsigned max_primitives = u_decomposed_prims_for_vertices(
      input_prims->prim, input_prims->count);
   unsigned max_verts = u_vertices_per_prim(assembled_prim) * max_primitives;

   asmblr->output_prims = output_prims;
   asmblr->output_verts = output_verts;
   asmblr->input_prims = input_prims;
   asmblr->input_verts = input_verts;
   asmblr->is_strip =
      (input_prims->prim == PIPE_PRIM_TRIANGLE_STRIP ||
       input_prims->prim == PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY) ||
      (input_prims->prim == PIPE_PRIM_LINE_STRIP ||
       input_prims->prim == PIPE_PRIM_LINE_STRIP_ADJACENCY);
   asmblr->needs_primid = needs_primid(asmblr->draw);
   asmblr->is_first_prim = asmblr->is_strip;
   asmblr->primid = 0;
   asmblr->num_prims = 0;

   output_prims->linear = TRUE;
   output_prims->elts = NULL;
   output_prims->start = 0;
   output_prims->prim = u_assembled_prim(input_prims->prim);
   output_prims->flags = 0x0;
   output_prims->primitive_lengths = MALLOC(sizeof(unsigned));
   output_prims->primitive_lengths[0] = 0;
   output_prims->primitive_count = 1;

   output_verts->vertex_size = input_verts->vertex_size;
   output_verts->stride = input_verts->stride;
   output_verts->verts = (struct vertex_header*)MALLOC(
      input_verts->vertex_size * max_verts);
   output_verts->count = 0;


   for (start = i = 0; i < input_prims->primitive_count;
        start += input_prims->primitive_lengths[i], i++)
   {
      unsigned count = input_prims->primitive_lengths[i];
      if (input_prims->linear) {
         assembler_run_linear(asmblr, input_prims, input_verts,
                              start, count);
      } else {
         assembler_run_elts(asmblr, input_prims, input_verts,
                            start, count);
      }
   }

   output_prims->primitive_lengths[0] = output_verts->count;
   output_prims->count = output_verts->count;
}

struct draw_assembler *
draw_prim_assembler_create(struct draw_context *draw)
{
   struct draw_assembler *ia = CALLOC_STRUCT( draw_assembler );

   ia->draw = draw;

   return ia;
}

void
draw_prim_assembler_destroy(struct draw_assembler *ia)
{
   FREE(ia);
}
