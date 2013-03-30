/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
#ifdef HAVE_LLVM
#include "draw_llvm.h"
#endif

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"

#include "pipe/p_shader_tokens.h"

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_prim.h"

/* fixme: move it from here */
#define MAX_PRIMITIVES 64

static INLINE int
draw_gs_get_input_index(int semantic, int index,
                        const struct tgsi_shader_info *input_info)
{
   int i;
   const ubyte *input_semantic_names = input_info->output_semantic_name;
   const ubyte *input_semantic_indices = input_info->output_semantic_index;
   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; i++) {
      if (input_semantic_names[i] == semantic &&
          input_semantic_indices[i] == index)
         return i;
   }
   debug_assert(0);
   return -1;
}

/**
 * We execute geometry shaders in the SOA mode, so ideally we want to
 * flush when the number of currently fetched primitives is equal to
 * the number of elements in the SOA vector. This ensures that the
 * throughput is optimized for the given vector instrunction set.
 */
static INLINE boolean
draw_gs_should_flush(struct draw_geometry_shader *shader)
{
   return (shader->fetched_prim_count == shader->vector_length);
}

/*#define DEBUG_OUTPUTS 1*/
static void
tgsi_fetch_gs_outputs(struct draw_geometry_shader *shader,
                      unsigned num_primitives,
                      float (**p_output)[4])
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned prim_idx, j, slot;
   unsigned current_idx = 0;
   float (*output)[4];

   output = *p_output;

   /* Unswizzle all output results.
    */

   for (prim_idx = 0; prim_idx < num_primitives; ++prim_idx) {
      unsigned num_verts_per_prim = machine->Primitives[prim_idx];
      shader->primitive_lengths[prim_idx +   shader->emitted_primitives] =
         machine->Primitives[prim_idx];
      shader->emitted_vertices += num_verts_per_prim;
      for (j = 0; j < num_verts_per_prim; j++, current_idx++) {
         int idx = current_idx * shader->info.num_outputs;
#ifdef DEBUG_OUTPUTS
         debug_printf("%d) Output vert:\n", idx / shader->info.num_outputs);
#endif
         for (slot = 0; slot < shader->info.num_outputs; slot++) {
            output[slot][0] = machine->Outputs[idx + slot].xyzw[0].f[0];
            output[slot][1] = machine->Outputs[idx + slot].xyzw[1].f[0];
            output[slot][2] = machine->Outputs[idx + slot].xyzw[2].f[0];
            output[slot][3] = machine->Outputs[idx + slot].xyzw[3].f[0];
#ifdef DEBUG_OUTPUTS
            debug_printf("\t%d: %f %f %f %f\n", slot,
                         output[slot][0],
                         output[slot][1],
                         output[slot][2],
                         output[slot][3]);
#endif
            debug_assert(!util_is_inf_or_nan(output[slot][0]));
         }
         output = (float (*)[4])((char *)output + shader->vertex_size);
      }
   }
   *p_output = output;
   shader->emitted_primitives += num_primitives;
}

/*#define DEBUG_INPUTS 1*/
static void tgsi_fetch_gs_input(struct draw_geometry_shader *shader,
                                unsigned *indices,
                                unsigned num_vertices,
                                unsigned prim_idx)
{
   struct tgsi_exec_machine *machine = shader->machine;
   unsigned slot, vs_slot, i;
   unsigned input_vertex_stride = shader->input_vertex_stride;
   const float (*input_ptr)[4];

   input_ptr = shader->input;

   for (i = 0; i < num_vertices; ++i) {
      const float (*input)[4];
#if DEBUG_INPUTS
      debug_printf("%d) vertex index = %d (prim idx = %d)\n",
                   i, indices[i], prim_idx);
#endif
      input = (const float (*)[4])(
         (const char *)input_ptr + (indices[i] * input_vertex_stride));
      for (slot = 0, vs_slot = 0; slot < shader->info.num_inputs; ++slot) {
         unsigned idx = i * TGSI_EXEC_MAX_INPUT_ATTRIBS + slot;
         if (shader->info.input_semantic_name[slot] == TGSI_SEMANTIC_PRIMID) {
            machine->Inputs[idx].xyzw[0].f[prim_idx] =
               (float)shader->in_prim_idx;
            machine->Inputs[idx].xyzw[1].f[prim_idx] =
               (float)shader->in_prim_idx;
            machine->Inputs[idx].xyzw[2].f[prim_idx] =
               (float)shader->in_prim_idx;
            machine->Inputs[idx].xyzw[3].f[prim_idx] =
               (float)shader->in_prim_idx;
         } else {
            vs_slot = draw_gs_get_input_index(
                        shader->info.input_semantic_name[slot],
                        shader->info.input_semantic_index[slot],
                        shader->input_info);
#if DEBUG_INPUTS
            debug_printf("\tSlot = %d, vs_slot = %d, idx = %d:\n",
                         slot, vs_slot, idx);
            assert(!util_is_inf_or_nan(input[vs_slot][0]));
            assert(!util_is_inf_or_nan(input[vs_slot][1]));
            assert(!util_is_inf_or_nan(input[vs_slot][2]));
            assert(!util_is_inf_or_nan(input[vs_slot][3]));
#endif
            machine->Inputs[idx].xyzw[0].f[prim_idx] = input[vs_slot][0];
            machine->Inputs[idx].xyzw[1].f[prim_idx] = input[vs_slot][1];
            machine->Inputs[idx].xyzw[2].f[prim_idx] = input[vs_slot][2];
            machine->Inputs[idx].xyzw[3].f[prim_idx] = input[vs_slot][3];
#if DEBUG_INPUTS
            debug_printf("\t\t%f %f %f %f\n",
                         machine->Inputs[idx].xyzw[0].f[prim_idx],
                         machine->Inputs[idx].xyzw[1].f[prim_idx],
                         machine->Inputs[idx].xyzw[2].f[prim_idx],
                         machine->Inputs[idx].xyzw[3].f[prim_idx]);
#endif
            ++vs_slot;
         }
      }
   }
}

static void tgsi_gs_prepare(struct draw_geometry_shader *shader,
                            const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
                            const unsigned constants_size[PIPE_MAX_CONSTANT_BUFFERS])
{
   struct tgsi_exec_machine *machine = shader->machine;

   tgsi_exec_set_constant_buffers(machine, PIPE_MAX_CONSTANT_BUFFERS,
                                  constants, constants_size);
}

static unsigned tgsi_gs_run(struct draw_geometry_shader *shader,
                            unsigned input_primitives)
{
   struct tgsi_exec_machine *machine = shader->machine;

   tgsi_set_exec_mask(machine,
                      1,
                      input_primitives > 1,
                      input_primitives > 2,
                      input_primitives > 3);

   /* run interpreter */
   tgsi_exec_machine_run(machine);

   return
      machine->Temps[TGSI_EXEC_TEMP_PRIMITIVE_I].xyzw[TGSI_EXEC_TEMP_PRIMITIVE_C].u[0];
}

#ifdef HAVE_LLVM

static void
llvm_fetch_gs_input(struct draw_geometry_shader *shader,
                    unsigned *indices,
                    unsigned num_vertices,
                    unsigned prim_idx)
{
   unsigned slot, vs_slot, i;
   unsigned input_vertex_stride = shader->input_vertex_stride;
   const float (*input_ptr)[4];
   float (*input_data)[6][PIPE_MAX_SHADER_INPUTS][TGSI_NUM_CHANNELS][TGSI_NUM_CHANNELS] = &shader->gs_input->data;

   shader->llvm_prim_ids[shader->fetched_prim_count] =
      shader->in_prim_idx;

   input_ptr = shader->input;

   for (i = 0; i < num_vertices; ++i) {
      const float (*input)[4];
#if DEBUG_INPUTS
      debug_printf("%d) vertex index = %d (prim idx = %d)\n",
                   i, indices[i], prim_idx);
#endif
      input = (const float (*)[4])(
         (const char *)input_ptr + (indices[i] * input_vertex_stride));
      for (slot = 0, vs_slot = 0; slot < shader->info.num_inputs; ++slot) {
         if (shader->info.input_semantic_name[slot] == TGSI_SEMANTIC_PRIMID) {
            /* skip. we handle system values through gallivm */
         } else {
            vs_slot = draw_gs_get_input_index(
                        shader->info.input_semantic_name[slot],
                        shader->info.input_semantic_index[slot],
                        shader->input_info);
#if DEBUG_INPUTS
            debug_printf("\tSlot = %d, vs_slot = %d, i = %d:\n",
                         slot, vs_slot, i);
            assert(!util_is_inf_or_nan(input[vs_slot][0]));
            assert(!util_is_inf_or_nan(input[vs_slot][1]));
            assert(!util_is_inf_or_nan(input[vs_slot][2]));
            assert(!util_is_inf_or_nan(input[vs_slot][3]));
#endif
            (*input_data)[i][slot][0][prim_idx] = input[vs_slot][0];
            (*input_data)[i][slot][1][prim_idx] = input[vs_slot][1];
            (*input_data)[i][slot][2][prim_idx] = input[vs_slot][2];
            (*input_data)[i][slot][3][prim_idx] = input[vs_slot][3];
#if DEBUG_INPUTS
            debug_printf("\t\t%f %f %f %f\n",
                         (*input_data)[i][slot][0][prim_idx],
                         (*input_data)[i][slot][1][prim_idx],
                         (*input_data)[i][slot][2][prim_idx],
                         (*input_data)[i][slot][3][prim_idx]);
#endif
            ++vs_slot;
         }
      }
   }
}

static void
llvm_fetch_gs_outputs(struct draw_geometry_shader *shader,
                      unsigned num_primitives,
                      float (**p_output)[4])
{
   int total_verts = 0;
   int vertex_count = 0;
   int total_prims = 0;
   int max_prims_per_invocation = 0;
   char *output_ptr = (char*)shader->gs_output;
   int i, j, prim_idx;

   for (i = 0; i < shader->vector_length; ++i) {
      int prims = shader->llvm_emitted_primitives[i];
      total_prims += prims;
      max_prims_per_invocation = MAX2(max_prims_per_invocation, prims);
   }
   for (i = 0; i < shader->vector_length; ++i) {
      total_verts += shader->llvm_emitted_vertices[i];
   }


   output_ptr += shader->emitted_vertices * shader->vertex_size;
   for (i = 0; i < shader->vector_length - 1; ++i) {
      int current_verts = shader->llvm_emitted_vertices[i];

      if (current_verts != shader->max_output_vertices) {
         memcpy(output_ptr + (vertex_count + current_verts) * shader->vertex_size,
                output_ptr + (vertex_count + shader->max_output_vertices) * shader->vertex_size,
                shader->vertex_size * (total_verts - vertex_count));
      }
      vertex_count += current_verts;
   }

   prim_idx = 0;
   for (i = 0; i < shader->vector_length; ++i) {
      int num_prims = shader->llvm_emitted_primitives[i];
      for (j = 0; j < num_prims; ++j) {
         int prim_length =
            shader->llvm_prim_lengths[j][i];
         shader->primitive_lengths[shader->emitted_primitives + prim_idx] =
            prim_length;
         ++prim_idx;
      }
   }

   shader->emitted_primitives += total_prims;
   shader->emitted_vertices += total_verts;
}

static void
llvm_gs_prepare(struct draw_geometry_shader *shader,
                const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
                const unsigned constants_size[PIPE_MAX_CONSTANT_BUFFERS])
{
}

static unsigned
llvm_gs_run(struct draw_geometry_shader *shader,
            unsigned input_primitives)
{
   unsigned ret;
   char *input = (char*)shader->gs_output;

   input += (shader->emitted_vertices * shader->vertex_size);

   ret = shader->current_variant->jit_func(
      shader->jit_context, shader->gs_input->data,
      (struct vertex_header*)input,
      input_primitives,
      shader->draw->instance_id,
      shader->llvm_prim_ids);

   return ret;
}

#endif

static void gs_flush(struct draw_geometry_shader *shader)
{
   unsigned out_prim_count;

   unsigned input_primitives = shader->fetched_prim_count;

   debug_assert(input_primitives > 0 &&
                input_primitives <= 4);

   out_prim_count = shader->run(shader, input_primitives);
   shader->fetch_outputs(shader, out_prim_count,
                         &shader->tmp_output);

#if 0
   debug_printf("PRIM emitted prims = %d (verts=%d), cur prim count = %d\n",
                shader->emitted_primitives, shader->emitted_vertices,
                out_prim_count);
#endif

   shader->fetched_prim_count = 0;
}

static void gs_point(struct draw_geometry_shader *shader,
                     int idx)
{
   unsigned indices[1];

   indices[0] = idx;

   shader->fetch_inputs(shader, indices, 1,
                        shader->fetched_prim_count);
   ++shader->in_prim_idx;
   ++shader->fetched_prim_count;

   if (draw_gs_should_flush(shader))
      gs_flush(shader);
}

static void gs_line(struct draw_geometry_shader *shader,
                    int i0, int i1)
{
   unsigned indices[2];

   indices[0] = i0;
   indices[1] = i1;

   shader->fetch_inputs(shader, indices, 2,
                        shader->fetched_prim_count);
   ++shader->in_prim_idx;
   ++shader->fetched_prim_count;
   
   if (draw_gs_should_flush(shader))   
      gs_flush(shader);
}

static void gs_line_adj(struct draw_geometry_shader *shader,
                        int i0, int i1, int i2, int i3)
{
   unsigned indices[4];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
   indices[3] = i3;

   shader->fetch_inputs(shader, indices, 4,
                        shader->fetched_prim_count);
   ++shader->in_prim_idx;
   ++shader->fetched_prim_count;

   if (draw_gs_should_flush(shader))
      gs_flush(shader);
}

static void gs_tri(struct draw_geometry_shader *shader,
                   int i0, int i1, int i2)
{
   unsigned indices[3];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;

   shader->fetch_inputs(shader, indices, 3,
                        shader->fetched_prim_count);
   ++shader->in_prim_idx;
   ++shader->fetched_prim_count;

   if (draw_gs_should_flush(shader))
      gs_flush(shader);
}

static void gs_tri_adj(struct draw_geometry_shader *shader,
                       int i0, int i1, int i2,
                       int i3, int i4, int i5)
{
   unsigned indices[6];

   indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
   indices[3] = i3;
   indices[4] = i4;
   indices[5] = i5;

   shader->fetch_inputs(shader, indices, 6,
                        shader->fetched_prim_count);
   ++shader->in_prim_idx;
   ++shader->fetched_prim_count;

   if (draw_gs_should_flush(shader))
      gs_flush(shader);
}

#define FUNC         gs_run
#define GET_ELT(idx) (idx)
#include "draw_gs_tmp.h"


#define FUNC         gs_run_elts
#define LOCAL_VARS   const ushort *elts = input_prims->elts;
#define GET_ELT(idx) (elts[idx])
#include "draw_gs_tmp.h"


/**
 * Execute geometry shader.
 */
int draw_geometry_shader_run(struct draw_geometry_shader *shader,
                             const void *constants[PIPE_MAX_CONSTANT_BUFFERS],
                             const unsigned constants_size[PIPE_MAX_CONSTANT_BUFFERS],
                             const struct draw_vertex_info *input_verts,
                             const struct draw_prim_info *input_prim,
                             const struct tgsi_shader_info *input_info,
                             struct draw_vertex_info *output_verts,
                             struct draw_prim_info *output_prims )
{
   const float (*input)[4] = (const float (*)[4])input_verts->verts->data;
   unsigned input_stride = input_verts->vertex_size;
   unsigned num_outputs = shader->info.num_outputs;
   unsigned vertex_size = sizeof(struct vertex_header) + num_outputs * 4 * sizeof(float);
   unsigned num_input_verts = input_prim->linear ?
      input_verts->count :
      input_prim->count;
   unsigned num_in_primitives =
      align(
         MAX2(u_gs_prims_for_vertices(input_prim->prim, num_input_verts),
              u_gs_prims_for_vertices(shader->input_primitive, num_input_verts)),
         shader->vector_length);
   unsigned max_out_prims = u_gs_prims_for_vertices(shader->output_primitive,
                                                    shader->max_output_vertices)
      * num_in_primitives;

   //Assume at least one primitive
   max_out_prims = MAX2(max_out_prims, 1);


   output_verts->vertex_size = vertex_size;
   output_verts->stride = output_verts->vertex_size;
   output_verts->verts =
      (struct vertex_header *)MALLOC(output_verts->vertex_size *
                                     max_out_prims *
                                     shader->max_output_vertices);

#if 0
   debug_printf("%s count = %d (in prims # = %d)\n",
                __FUNCTION__, num_input_verts, num_in_primitives);
   debug_printf("\tlinear = %d, prim_info->count = %d\n",
                input_prim->linear, input_prim->count);
   debug_printf("\tprim pipe = %s, shader in = %s, shader out = %s, max out = %d\n",
                u_prim_name(input_prim->prim),
                u_prim_name(shader->input_primitive),
                u_prim_name(shader->output_primitive),
                shader->max_output_vertices);
#endif

   shader->emitted_vertices = 0;
   shader->emitted_primitives = 0;
   shader->vertex_size = vertex_size;
   shader->tmp_output = (float (*)[4])output_verts->verts->data;
   shader->in_prim_idx = 0;
   shader->fetched_prim_count = 0;
   shader->input_vertex_stride = input_stride;
   shader->input = input;
   shader->input_info = input_info;
   FREE(shader->primitive_lengths);
   shader->primitive_lengths = MALLOC(max_out_prims * sizeof(unsigned));


#ifdef HAVE_LLVM
   if (draw_get_option_use_llvm()) {
      shader->gs_output = output_verts->verts;
      if (max_out_prims > shader->max_out_prims) {
         unsigned i;
         if (shader->llvm_prim_lengths) {
            for (i = 0; i < shader->max_out_prims; ++i) {
               align_free(shader->llvm_prim_lengths[i]);
            }
            FREE(shader->llvm_prim_lengths);
         }

         shader->llvm_prim_lengths = MALLOC(max_out_prims * sizeof(unsigned*));
         for (i = 0; i < max_out_prims; ++i) {
            int vector_size = shader->vector_length * sizeof(unsigned);
            shader->llvm_prim_lengths[i] =
               align_malloc(vector_size, vector_size);
         }

         shader->max_out_prims = max_out_prims;
      }
      shader->jit_context->prim_lengths = shader->llvm_prim_lengths;
      shader->jit_context->emitted_vertices = shader->llvm_emitted_vertices;
      shader->jit_context->emitted_prims = shader->llvm_emitted_primitives;
   }
#endif

   shader->prepare(shader, constants, constants_size);

   if (input_prim->linear)
      gs_run(shader, input_prim, input_verts,
             output_prims, output_verts);
   else
      gs_run_elts(shader, input_prim, input_verts,
                  output_prims, output_verts);

   /* Flush the remaining primitives. Will happen if
    * num_input_primitives % 4 != 0
    */
   if (shader->fetched_prim_count > 0) {
      gs_flush(shader);
   }

   debug_assert(shader->fetched_prim_count == 0);

   /* Update prim_info:
    */
   output_prims->linear = TRUE;
   output_prims->elts = NULL;
   output_prims->start = 0;
   output_prims->count = shader->emitted_vertices;
   output_prims->prim = shader->output_primitive;
   output_prims->flags = 0x0;
   output_prims->primitive_lengths = shader->primitive_lengths;
   output_prims->primitive_count = shader->emitted_primitives;
   output_verts->count = shader->emitted_vertices;

#if 0
   debug_printf("GS finished, prims = %d, verts = %d\n",
                output_prims->primitive_count,
                output_verts->count);
#endif

   return shader->emitted_vertices;
}

void draw_geometry_shader_prepare(struct draw_geometry_shader *shader,
                                  struct draw_context *draw)
{
   if (shader && shader->machine->Tokens != shader->state.tokens) {
      tgsi_exec_machine_bind_shader(shader->machine,
                                    shader->state.tokens,
                                    draw->gs.tgsi.sampler);
   }
}


boolean
draw_gs_init( struct draw_context *draw )
{
   draw->gs.tgsi.machine = tgsi_exec_machine_create();
   if (!draw->gs.tgsi.machine)
      return FALSE;

   draw->gs.tgsi.machine->Primitives = align_malloc(
      MAX_PRIMITIVES * sizeof(struct tgsi_exec_vector), 16);
   if (!draw->gs.tgsi.machine->Primitives)
      return FALSE;
   memset(draw->gs.tgsi.machine->Primitives, 0,
          MAX_PRIMITIVES * sizeof(struct tgsi_exec_vector));

   return TRUE;
}

void draw_gs_destroy( struct draw_context *draw )
{
   if (draw->gs.tgsi.machine) {
      align_free(draw->gs.tgsi.machine->Primitives);
      tgsi_exec_machine_destroy(draw->gs.tgsi.machine);
   }
}

struct draw_geometry_shader *
draw_create_geometry_shader(struct draw_context *draw,
                            const struct pipe_shader_state *state)
{
#ifdef HAVE_LLVM
   struct llvm_geometry_shader *llvm_gs;
#endif
   struct draw_geometry_shader *gs;
   unsigned i;

#ifdef HAVE_LLVM
   if (draw_get_option_use_llvm()) {
      llvm_gs = CALLOC_STRUCT(llvm_geometry_shader);

      if (llvm_gs == NULL)
         return NULL;

      gs = &llvm_gs->base;

      make_empty_list(&llvm_gs->variants);
   } else
#endif
   {
      gs = CALLOC_STRUCT(draw_geometry_shader);
   }

   if (!gs)
      return NULL;

   gs->draw = draw;
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
   gs->max_out_prims = 0;

#ifdef HAVE_LLVM
   if (draw_get_option_use_llvm()) {
      /* TODO: change the input array to handle the following
         vector length, instead of the currently hardcoded
         TGSI_NUM_CHANNELS
      gs->vector_length = lp_native_vector_width / 32;*/
      gs->vector_length = TGSI_NUM_CHANNELS;
   } else
#endif
   {
      gs->vector_length = 1;
   }

   for (i = 0; i < gs->info.num_properties; ++i) {
      if (gs->info.properties[i].name ==
          TGSI_PROPERTY_GS_INPUT_PRIM)
         gs->input_primitive = gs->info.properties[i].data[0];
      else if (gs->info.properties[i].name ==
               TGSI_PROPERTY_GS_OUTPUT_PRIM)
         gs->output_primitive = gs->info.properties[i].data[0];
      else if (gs->info.properties[i].name ==
               TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES)
         gs->max_output_vertices = gs->info.properties[i].data[0];
   }

   for (i = 0; i < gs->info.num_outputs; i++) {
      if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_POSITION &&
          gs->info.output_semantic_index[i] == 0)
         gs->position_output = i;
   }

   gs->machine = draw->gs.tgsi.machine;

#ifdef HAVE_LLVM
   if (draw_get_option_use_llvm()) {
      int vector_size = gs->vector_length * sizeof(float);
      gs->gs_input = align_malloc(sizeof(struct draw_gs_inputs), 16);
      memset(gs->gs_input, 0, sizeof(struct draw_gs_inputs));
      gs->llvm_prim_lengths = 0;

      gs->llvm_emitted_primitives = align_malloc(vector_size, vector_size);
      gs->llvm_emitted_vertices = align_malloc(vector_size, vector_size);
      gs->llvm_prim_ids = align_malloc(vector_size, vector_size);

      gs->fetch_outputs = llvm_fetch_gs_outputs;
      gs->fetch_inputs = llvm_fetch_gs_input;
      gs->prepare = llvm_gs_prepare;
      gs->run = llvm_gs_run;

      gs->jit_context = &draw->llvm->gs_jit_context;


      llvm_gs->variant_key_size =
         draw_gs_llvm_variant_key_size(
            MAX2(gs->info.file_max[TGSI_FILE_SAMPLER]+1,
                 gs->info.file_max[TGSI_FILE_SAMPLER_VIEW]+1));
   } else
#endif
   {
      gs->fetch_outputs = tgsi_fetch_gs_outputs;
      gs->fetch_inputs = tgsi_fetch_gs_input;
      gs->prepare = tgsi_gs_prepare;
      gs->run = tgsi_gs_run;
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
#ifdef HAVE_LLVM
   if (draw_get_option_use_llvm()) {
      struct llvm_geometry_shader *shader = llvm_geometry_shader(dgs);
      struct draw_gs_llvm_variant_list_item *li;

      li = first_elem(&shader->variants);
      while(!at_end(&shader->variants, li)) {
         struct draw_gs_llvm_variant_list_item *next = next_elem(li);
         draw_gs_llvm_destroy_variant(li->base);
         li = next;
      }

      assert(shader->variants_cached == 0);

      if (dgs->llvm_prim_lengths) {
         unsigned i;
         for (i = 0; i < dgs->max_out_prims; ++i) {
            align_free(dgs->llvm_prim_lengths[i]);
         }
         FREE(dgs->llvm_prim_lengths);
      }
      align_free(dgs->llvm_emitted_primitives);
      align_free(dgs->llvm_emitted_vertices);
      align_free(dgs->llvm_prim_ids);

      align_free(dgs->gs_input);
   }
#endif

   FREE(dgs->primitive_lengths);
   FREE((void*) dgs->state.tokens);
   FREE(dgs);
}


#ifdef HAVE_LLVM
void draw_gs_set_current_variant(struct draw_geometry_shader *shader,
                                 struct draw_gs_llvm_variant *variant)
{
   shader->current_variant = variant;
}
#endif
