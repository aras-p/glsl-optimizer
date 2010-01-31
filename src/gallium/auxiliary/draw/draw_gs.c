/**************************************************************************
 *
 * Copyright 2009 VMWare Inc.
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

#include "draw_gs.h"

#include "draw_private.h"
#include "draw_context.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"

#include "pipe/p_shader_tokens.h"

#include "util/u_math.h"
#include "util/u_memory.h"

#define MAX_PRIM_VERTICES 6
/* fixme: move it from here */
#define MAX_PRIMITIVES 64

boolean
draw_gs_init( struct draw_context *draw )
{
   draw->gs.machine = tgsi_exec_machine_create();
   if (!draw->gs.machine)
      return FALSE;

   draw->gs.machine->Primitives = align_malloc(
      MAX_PRIMITIVES * sizeof(struct tgsi_exec_vector), 16);
   if (!draw->gs.machine->Primitives)
      return FALSE;
   memset(draw->gs.machine->Primitives, 0,
          MAX_PRIMITIVES * sizeof(struct tgsi_exec_vector));

   return TRUE;
}

void draw_gs_destroy( struct draw_context *draw )
{
   if (!draw->gs.machine)
      return;

   align_free(draw->gs.machine->Primitives);

   tgsi_exec_machine_destroy(draw->gs.machine);
}

void
draw_gs_set_constants(struct draw_context *draw,
                      unsigned slot,
                      const void *constants,
                      unsigned size)
{
}


struct draw_geometry_shader *
draw_create_geometry_shader(struct draw_context *draw,
                            const struct pipe_shader_state *state)
{
   struct draw_geometry_shader *gs;
   int i;

   gs = CALLOC_STRUCT(draw_geometry_shader);

   if (!gs)
      return NULL;

   gs->state = *state;
   gs->state.tokens = tgsi_dup_tokens(state->tokens);
   if (!gs->state.tokens) {
      FREE(gs);
      return NULL;
   }

   tgsi_scan_shader(state->tokens, &gs->info);

   /* setup the defaults */
   gs->input_primitive = PIPE_PRIM_TRIANGLES;
   gs->output_primitive = PIPE_PRIM_TRIANGLE_STRIP;
   gs->max_output_vertices = 32;

   for (i = 0; i < gs->info.num_properties; ++i) {
      if (gs->info.properties[i].name ==
          TGSI_PROPERTY_GS_INPUT_PRIM)
         gs->input_primitive = gs->info.properties[i].data[0];
      else if (gs->info.properties[i].name ==
               TGSI_PROPERTY_GS_OUTPUT_PRIM)
         gs->output_primitive = gs->info.properties[i].data[0];
      else if (gs->info.properties[i].name ==
               TGSI_PROPERTY_GS_MAX_VERTICES)
         gs->max_output_vertices = gs->info.properties[i].data[0];
   }

   gs->machine = draw->gs.machine;

   if (gs)
   {
      uint i;
      for (i = 0; i < gs->info.num_outputs; i++) {
         if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_POSITION &&
             gs->info.output_semantic_index[i] == 0)
            gs->position_output = i;
      }
   }

   return gs;
}

void draw_bind_geometry_shader(struct draw_context *draw,
                               struct draw_geometry_shader *dgs)
{
   draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);

   if (dgs) {
      draw->gs.geometry_shader = dgs;
      draw->gs.num_gs_outputs = dgs->info.num_outputs;
      draw->gs.position_output = dgs->position_output;
      draw_geometry_shader_prepare(dgs, draw);
   }
   else {
      draw->gs.geometry_shader = NULL;
      draw->gs.num_gs_outputs = 0;
   }
}

void draw_delete_geometry_shader(struct draw_context *draw,
                                 struct draw_geometry_shader *dgs)
{
   FREE(dgs);
}

static INLINE int num_vertices_for_prim(int prim)
{
   switch(prim) {
   case PIPE_PRIM_POINTS:
      return 1;
   case PIPE_PRIM_LINES:
      return 2;
   case PIPE_PRIM_LINE_LOOP:
      return 2;
   case PIPE_PRIM_LINE_STRIP:
      return 2;
   case PIPE_PRIM_TRIANGLES:
      return 3;
   case PIPE_PRIM_TRIANGLE_STRIP:
      return 3;
   case PIPE_PRIM_TRIANGLE_FAN:
      return 3;
   case PIPE_PRIM_LINES_ADJACENCY:
   case PIPE_PRIM_LINE_STRIP_ADJACENCY:
      return 4;
   case PIPE_PRIM_TRIANGLES_ADJACENCY:
   case PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY:
      return 6;
   default:
      assert(!"Bad geometry shader input");
      return 0;
   }
}

static void draw_fetch_geometry_input(struct draw_geometry_shader *shader,
                                      int start_primitive,
                                      int num_primitives,
                                      const float (*input_ptr)[4],
                                      unsigned input_vertex_stride,
                                      unsigned inputs_from_vs)
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned slot, vs_slot, k, j;
   unsigned num_vertices = num_vertices_for_prim(shader->input_primitive);
   int idx = 0;

   for (slot = 0, vs_slot = 0; slot < shader->info.num_inputs; slot++) {
      /*debug_printf("Slot = %d (semantic = %d)\n", slot,
        shader->info.input_semantic_name[slot]);*/
      if (shader->info.input_semantic_name[slot] ==
          TGSI_SEMANTIC_PRIMID) {
         for (j = 0; j < num_primitives; ++j) {
            machine->Inputs[idx].xyzw[0].f[j] = (float)start_primitive + j;
            machine->Inputs[idx].xyzw[1].f[j] = (float)start_primitive + j;
            machine->Inputs[idx].xyzw[2].f[j] = (float)start_primitive + j;
            machine->Inputs[idx].xyzw[3].f[j] = (float)start_primitive + j;
         }
         ++idx;
      } else {
         for (j = 0; j < num_primitives; ++j) {
            int vidx = idx;
            const float (*prim_ptr)[4];
            /*debug_printf("    %d) Prim (num_verts = %d)\n", start_primitive + j,
              num_vertices);*/
            prim_ptr = (const float (*)[4])(
               (const char *)input_ptr +
               (j * num_vertices * input_vertex_stride));

            for (k = 0; k < num_vertices; ++k, ++vidx) {
               const float (*input)[4];
               input = (const float (*)[4])(
                  (const char *)prim_ptr + (k * input_vertex_stride));
               vidx = k * TGSI_EXEC_MAX_INPUT_ATTRIBS + slot;
               /*debug_printf("\t%d)(%d) Input vert:\n", vidx, k);*/
#if 1
               assert(!util_is_inf_or_nan(input[vs_slot][0]));
               assert(!util_is_inf_or_nan(input[vs_slot][1]));
               assert(!util_is_inf_or_nan(input[vs_slot][2]));
               assert(!util_is_inf_or_nan(input[vs_slot][3]));
#endif
               machine->Inputs[vidx].xyzw[0].f[j] = input[vs_slot][0];
               machine->Inputs[vidx].xyzw[1].f[j] = input[vs_slot][1];
               machine->Inputs[vidx].xyzw[2].f[j] = input[vs_slot][2];
               machine->Inputs[vidx].xyzw[3].f[j] = input[vs_slot][3];
#if 0
               debug_printf("\t\t%d %f %f %f %f\n", slot,
                            machine->Inputs[vidx].xyzw[0].f[j],
                            machine->Inputs[vidx].xyzw[1].f[j],
                            machine->Inputs[vidx].xyzw[2].f[j],
                            machine->Inputs[vidx].xyzw[3].f[j]);
#endif
            }
         }
         ++vs_slot;
         idx += num_vertices;
      }
   }
}

static INLINE void
draw_geometry_fetch_outputs(struct draw_geometry_shader *shader,
                            int num_primitives,
                            float (*output)[4],
                            unsigned vertex_size)
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned prim_idx, j, slot;

   /* Unswizzle all output results.
    */
   /* FIXME: handle all the primitives produced by the gs, not just
    * the first one
    unsigned prim_count =
    mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0];*/
   for (prim_idx = 0; prim_idx < num_primitives; ++prim_idx) {
      unsigned num_verts_per_prim = machine->Primitives[0];
      for (j = 0; j < num_verts_per_prim; j++) {
         int idx = (prim_idx * num_verts_per_prim + j) *
                   shader->info.num_outputs;
#ifdef DEBUG_OUTPUTS
         debug_printf("%d) Output vert:\n", idx);
#endif
         for (slot = 0; slot < shader->info.num_outputs; slot++) {
            output[slot][0] = machine->Outputs[idx + slot].xyzw[0].f[prim_idx];
            output[slot][1] = machine->Outputs[idx + slot].xyzw[1].f[prim_idx];
            output[slot][2] = machine->Outputs[idx + slot].xyzw[2].f[prim_idx];
            output[slot][3] = machine->Outputs[idx + slot].xyzw[3].f[prim_idx];
#ifdef DEBUG_OUTPUTS
            debug_printf("\t%d: %f %f %f %f\n", slot,
                         output[slot][0],
                         output[slot][1],
                         output[slot][2],
                         output[slot][3]);
#endif
            debug_assert(!util_is_inf_or_nan(output[slot][0]));
         }
         output = (float (*)[4])((char *)output + vertex_size);
      }
   }
}

void draw_geometry_shader_run(struct draw_geometry_shader *shader,
                              const float (*input)[4],
                              float (*output)[4],
                              const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
                              unsigned count,
                              unsigned input_stride,
                              unsigned vertex_size)
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned int i;
   unsigned num_vertices = num_vertices_for_prim(shader->input_primitive);
   unsigned num_primitives = count/num_vertices;
   unsigned inputs_from_vs = 0;

   for (i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      machine->Consts[i] = constants[i];
   }

   for (i = 0; i < shader->info.num_inputs; ++i) {
      if (shader->info.input_semantic_name[i] != TGSI_SEMANTIC_PRIMID)
         ++inputs_from_vs;
   }

   for (i = 0; i < num_primitives; ++i) {
      unsigned int max_primitives = 1;

      draw_fetch_geometry_input(shader, i, max_primitives, input,
                                input_stride, inputs_from_vs);

      tgsi_set_exec_mask(machine,
                         1,
                         max_primitives > 1,
                         max_primitives > 2,
                         max_primitives > 3);

      /* run interpreter */
      tgsi_exec_machine_run(machine);

      draw_geometry_fetch_outputs(shader, max_primitives,
                                  output, vertex_size);
   }
}

void draw_geometry_shader_delete(struct draw_geometry_shader *shader)
{
   FREE((void*) shader->state.tokens);
   FREE(shader);
}

void draw_geometry_shader_prepare(struct draw_geometry_shader *shader,
                                  struct draw_context *draw)
{
    if (shader->machine->Tokens != shader->state.tokens) {
       tgsi_exec_machine_bind_shader(shader->machine,
                                     shader->state.tokens,
                                     draw->gs.num_samplers,
                                     draw->gs.samplers);
    }
}
