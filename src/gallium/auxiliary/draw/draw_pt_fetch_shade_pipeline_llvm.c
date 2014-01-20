/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_prim.h"
#include "draw/draw_context.h"
#include "draw/draw_gs.h"
#include "draw/draw_vbuf.h"
#include "draw/draw_vertex.h"
#include "draw/draw_pt.h"
#include "draw/draw_prim_assembler.h"
#include "draw/draw_vs.h"
#include "draw/draw_llvm.h"
#include "gallivm/lp_bld_init.h"


struct llvm_middle_end {
   struct draw_pt_middle_end base;
   struct draw_context *draw;

   struct pt_emit *emit;
   struct pt_so_emit *so_emit;
   struct pt_fetch *fetch;
   struct pt_post_vs *post_vs;


   unsigned vertex_data_offset;
   unsigned vertex_size;
   unsigned input_prim;
   unsigned opt;

   struct draw_llvm *llvm;
   struct draw_llvm_variant *current_variant;
};


/** cast wrapper */
static INLINE struct llvm_middle_end *
llvm_middle_end(struct draw_pt_middle_end *middle)
{
   return (struct llvm_middle_end *) middle;
}


static void
llvm_middle_end_prepare_gs(struct llvm_middle_end *fpme)
{
   struct draw_context *draw = fpme->draw;
   struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   struct draw_gs_llvm_variant_key *key;
   struct draw_gs_llvm_variant *variant = NULL;
   struct draw_gs_llvm_variant_list_item *li;
   struct llvm_geometry_shader *shader = llvm_geometry_shader(gs);
   char store[DRAW_GS_LLVM_MAX_VARIANT_KEY_SIZE];
   unsigned i;

   key = draw_gs_llvm_make_variant_key(fpme->llvm, store);

   /* Search shader's list of variants for the key */
   li = first_elem(&shader->variants);
   while (!at_end(&shader->variants, li)) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
         variant = li->base;
         break;
      }
      li = next_elem(li);
   }

   if (variant) {
      /* found the variant, move to head of global list (for LRU) */
      move_to_head(&fpme->llvm->gs_variants_list,
                   &variant->list_item_global);
   }
   else {
      /* Need to create new variant */

      /* First check if we've created too many variants.  If so, free
       * 25% of the LRU to avoid using too much memory.
       */
      if (fpme->llvm->nr_gs_variants >= DRAW_MAX_SHADER_VARIANTS) {
         /*
          * XXX: should we flush here ?
          */
         for (i = 0; i < DRAW_MAX_SHADER_VARIANTS / 4; i++) {
            struct draw_gs_llvm_variant_list_item *item;
            if (is_empty_list(&fpme->llvm->gs_variants_list)) {
               break;
            }
            item = last_elem(&fpme->llvm->gs_variants_list);
            assert(item);
            assert(item->base);
            draw_gs_llvm_destroy_variant(item->base);
         }
      }

      variant = draw_gs_llvm_create_variant(fpme->llvm, gs->info.num_outputs, key);

      if (variant) {
         insert_at_head(&shader->variants, &variant->list_item_local);
         insert_at_head(&fpme->llvm->gs_variants_list,
                        &variant->list_item_global);
         fpme->llvm->nr_gs_variants++;
         shader->variants_cached++;
      }
   }

   gs->current_variant = variant;
}

/**
 * Prepare/validate middle part of the vertex pipeline.
 * NOTE: if you change this function, also look at the non-LLVM
 * function fetch_pipeline_prepare() for similar changes.
 */
static void
llvm_middle_end_prepare( struct draw_pt_middle_end *middle,
                         unsigned in_prim,
                         unsigned opt,
                         unsigned *max_vertices )
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *vs = draw->vs.vertex_shader;
   struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   const unsigned out_prim = gs ? gs->output_primitive :
      u_assembled_prim(in_prim);
   unsigned point_clip = draw->rasterizer->fill_front == PIPE_POLYGON_MODE_POINT ||
                         out_prim == PIPE_PRIM_POINTS;
   unsigned nr;

   fpme->input_prim = in_prim;
   fpme->opt = opt;

   draw_pt_post_vs_prepare( fpme->post_vs,
                            draw->clip_xy,
                            draw->clip_z,
                            draw->clip_user,
                            point_clip ? draw->guard_band_points_xy :
                                         draw->guard_band_xy,
                            draw->identity_viewport,
                            draw->rasterizer->clip_halfz,
                            (draw->vs.edgeflag_output ? TRUE : FALSE) );

   draw_pt_so_emit_prepare( fpme->so_emit, gs == NULL );

   if (!(opt & PT_PIPELINE)) {
      draw_pt_emit_prepare( fpme->emit,
			    out_prim,
                            max_vertices );

      *max_vertices = MAX2( *max_vertices, 4096 );
   }
   else {
      /* limit max fetches by limiting max_vertices */
      *max_vertices = 4096;
   }

   /* Get the number of float[4] attributes per vertex.
    * Note: this must be done after draw_pt_emit_prepare() since that
    * can effect the vertex size.
    */
   nr = MAX2(vs->info.num_inputs, draw_total_vs_outputs(draw));

   /* Always leave room for the vertex header whether we need it or
    * not.  It's hard to get rid of it in particular because of the
    * viewport code in draw_pt_post_vs.c.
    */
   fpme->vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);

   /* Get the number of float[4] attributes per vertex.
    * Note: this must be done after draw_pt_emit_prepare() since that
    * can effect the vertex size.
    */
   nr = MAX2(vs->info.num_inputs, draw_total_vs_outputs(draw));

   /* Always leave room for the vertex header whether we need it or
    * not.  It's hard to get rid of it in particular because of the
    * viewport code in draw_pt_post_vs.c.
    */
   fpme->vertex_size = sizeof(struct vertex_header) + nr * 4 * sizeof(float);

   /* return even number */
   *max_vertices = *max_vertices & ~1;

   /* Find/create the vertex shader variant */
   {
      struct draw_llvm_variant_key *key;
      struct draw_llvm_variant *variant = NULL;
      struct draw_llvm_variant_list_item *li;
      struct llvm_vertex_shader *shader = llvm_vertex_shader(vs);
      char store[DRAW_LLVM_MAX_VARIANT_KEY_SIZE];
      unsigned i;

      key = draw_llvm_make_variant_key(fpme->llvm, store);

      /* Search shader's list of variants for the key */
      li = first_elem(&shader->variants);
      while (!at_end(&shader->variants, li)) {
         if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
            variant = li->base;
            break;
         }
         li = next_elem(li);
      }

      if (variant) {
         /* found the variant, move to head of global list (for LRU) */
         move_to_head(&fpme->llvm->vs_variants_list,
                      &variant->list_item_global);
      }
      else {
         /* Need to create new variant */

         /* First check if we've created too many variants.  If so, free
          * 25% of the LRU to avoid using too much memory.
          */
         if (fpme->llvm->nr_variants >= DRAW_MAX_SHADER_VARIANTS) {
            /*
             * XXX: should we flush here ?
             */
            for (i = 0; i < DRAW_MAX_SHADER_VARIANTS / 4; i++) {
               struct draw_llvm_variant_list_item *item;
               if (is_empty_list(&fpme->llvm->vs_variants_list)) {
                  break;
               }
               item = last_elem(&fpme->llvm->vs_variants_list);
               assert(item);
               assert(item->base);
               draw_llvm_destroy_variant(item->base);
            }
         }

         variant = draw_llvm_create_variant(fpme->llvm, nr, key);

         if (variant) {
            insert_at_head(&shader->variants, &variant->list_item_local);
            insert_at_head(&fpme->llvm->vs_variants_list,
                           &variant->list_item_global);
            fpme->llvm->nr_variants++;
            shader->variants_cached++;
         }
      }

      fpme->current_variant = variant;
   }

   if (gs) {
      llvm_middle_end_prepare_gs(fpme);
   }
}


/**
 * Bind/update constant buffer pointers, clip planes and viewport dims.
 * These are "light weight" parameters which aren't baked into the
 * generated code.  Updating these items is much cheaper than revalidating
 * and rebuilding the generated pipeline code.
 */
static void
llvm_middle_end_bind_parameters(struct draw_pt_middle_end *middle)
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   unsigned i;

   for (i = 0; i < Elements(fpme->llvm->jit_context.vs_constants); ++i) {
      int num_consts =
         draw->pt.user.vs_constants_size[i] / (sizeof(float) * 4);
      fpme->llvm->jit_context.vs_constants[i] = draw->pt.user.vs_constants[i];
      fpme->llvm->jit_context.num_vs_constants[i] = num_consts;
   }
   for (i = 0; i < Elements(fpme->llvm->gs_jit_context.constants); ++i) {
      int num_consts =
         draw->pt.user.gs_constants_size[i] / (sizeof(float) * 4);
      fpme->llvm->gs_jit_context.constants[i] = draw->pt.user.gs_constants[i];
      fpme->llvm->gs_jit_context.num_constants[i] = num_consts;
   }

   fpme->llvm->jit_context.planes =
      (float (*)[DRAW_TOTAL_CLIP_PLANES][4]) draw->pt.user.planes[0];
   fpme->llvm->gs_jit_context.planes =
      (float (*)[DRAW_TOTAL_CLIP_PLANES][4]) draw->pt.user.planes[0];

   fpme->llvm->jit_context.viewport = (float *) draw->viewports[0].scale;
   fpme->llvm->gs_jit_context.viewport = (float *) draw->viewports[0].scale;
}


static void
pipeline(struct llvm_middle_end *llvm,
         const struct draw_vertex_info *vert_info,
         const struct draw_prim_info *prim_info)
{
   if (prim_info->linear)
      draw_pipeline_run_linear( llvm->draw,
                                vert_info,
                                prim_info);
   else
      draw_pipeline_run( llvm->draw,
                         vert_info,
                         prim_info );
}


static void
emit(struct pt_emit *emit,
     const struct draw_vertex_info *vert_info,
     const struct draw_prim_info *prim_info)
{
   if (prim_info->linear) {
      draw_pt_emit_linear(emit, vert_info, prim_info);
   }
   else {
      draw_pt_emit(emit, vert_info, prim_info);
   }
}


static void
llvm_pipeline_generic(struct draw_pt_middle_end *middle,
                      const struct draw_fetch_info *fetch_info,
                      const struct draw_prim_info *in_prim_info)
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_geometry_shader *gshader = draw->gs.geometry_shader;
   struct draw_prim_info gs_prim_info;
   struct draw_vertex_info llvm_vert_info;
   struct draw_vertex_info gs_vert_info;
   struct draw_vertex_info *vert_info;
   struct draw_prim_info ia_prim_info;
   struct draw_vertex_info ia_vert_info;
   const struct draw_prim_info *prim_info = in_prim_info;
   boolean free_prim_info = FALSE;
   unsigned opt = fpme->opt;
   unsigned clipped = 0;

   llvm_vert_info.count = fetch_info->count;
   llvm_vert_info.vertex_size = fpme->vertex_size;
   llvm_vert_info.stride = fpme->vertex_size;
   llvm_vert_info.verts = (struct vertex_header *)
      MALLOC(fpme->vertex_size *
             align(fetch_info->count, lp_native_vector_width / 32));
   if (!llvm_vert_info.verts) {
      assert(0);
      return;
   }

   if (draw->collect_statistics) {
      draw->statistics.ia_vertices += prim_info->count;
      draw->statistics.ia_primitives +=
         u_decomposed_prims_for_vertices(prim_info->prim, prim_info->count);
      draw->statistics.vs_invocations += fetch_info->count;
   }

   if (fetch_info->linear)
      clipped = fpme->current_variant->jit_func( &fpme->llvm->jit_context,
                                       llvm_vert_info.verts,
                                       draw->pt.user.vbuffer,
                                       fetch_info->start,
                                       fetch_info->count,
                                       fpme->vertex_size,
                                       draw->pt.vertex_buffer,
                                       draw->instance_id,
                                       draw->start_index);
   else
      clipped = fpme->current_variant->jit_func_elts( &fpme->llvm->jit_context,
                                            llvm_vert_info.verts,
                                            draw->pt.user.vbuffer,
                                            fetch_info->elts,
                                            draw->pt.user.eltMax,
                                            fetch_info->count,
                                            fpme->vertex_size,
                                            draw->pt.vertex_buffer,
                                            draw->instance_id,
                                            draw->pt.user.eltBias);

   /* Finished with fetch and vs:
    */
   fetch_info = NULL;
   vert_info = &llvm_vert_info;

   if ((opt & PT_SHADE) && gshader) {
      struct draw_vertex_shader *vshader = draw->vs.vertex_shader;
      draw_geometry_shader_run(gshader,
                               draw->pt.user.gs_constants,
                               draw->pt.user.gs_constants_size,
                               vert_info,
                               prim_info,
                               &vshader->info,
                               &gs_vert_info,
                               &gs_prim_info);

      FREE(vert_info->verts);
      vert_info = &gs_vert_info;
      prim_info = &gs_prim_info;
   } else {
      if (draw_prim_assembler_is_required(draw, prim_info, vert_info)) {
         draw_prim_assembler_run(draw, prim_info, vert_info,
                                 &ia_prim_info, &ia_vert_info);

         if (ia_vert_info.count) {
            FREE(vert_info->verts);
            vert_info = &ia_vert_info;
            prim_info = &ia_prim_info;
            free_prim_info = TRUE;
         }
      }
   }
   if (prim_info->count == 0) {
      debug_printf("GS/IA didn't emit any vertices!\n");

      FREE(vert_info->verts);
      if (free_prim_info) {
         FREE(prim_info->primitive_lengths);
      }
      return;
   }

   /* stream output needs to be done before clipping */
   draw_pt_so_emit( fpme->so_emit, vert_info, prim_info );

   draw_stats_clipper_primitives(draw, prim_info);

   /*
    * if there's no position, need to stop now, or the latter stages
    * will try to access non-existent position output.
    */
   if (draw_current_shader_position_output(draw) != -1) {
      if ((opt & PT_SHADE) && gshader) {
         clipped = draw_pt_post_vs_run( fpme->post_vs, vert_info, prim_info );
      }
      if (clipped) {
         opt |= PT_PIPELINE;
      }

      /* Do we need to run the pipeline? Now will come here if clipped
       */
      if (opt & PT_PIPELINE) {
         pipeline( fpme, vert_info, prim_info );
      }
      else {
         emit( fpme->emit, vert_info, prim_info );
      }
   }
   FREE(vert_info->verts);
   if (free_prim_info) {
      FREE(prim_info->primitive_lengths);
   }
}


static void
llvm_middle_end_run(struct draw_pt_middle_end *middle,
                    const unsigned *fetch_elts,
                    unsigned fetch_count,
                    const ushort *draw_elts,
                    unsigned draw_count,
                    unsigned prim_flags)
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_fetch_info fetch_info;
   struct draw_prim_info prim_info;

   fetch_info.linear = FALSE;
   fetch_info.start = 0;
   fetch_info.elts = fetch_elts;
   fetch_info.count = fetch_count;

   prim_info.linear = FALSE;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = fpme->input_prim;
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
   prim_info.primitive_lengths = &draw_count;

   llvm_pipeline_generic( middle, &fetch_info, &prim_info );
}


static void
llvm_middle_end_linear_run(struct draw_pt_middle_end *middle,
                           unsigned start,
                           unsigned count,
                           unsigned prim_flags)
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_fetch_info fetch_info;
   struct draw_prim_info prim_info;

   fetch_info.linear = TRUE;
   fetch_info.start = start;
   fetch_info.count = count;
   fetch_info.elts = NULL;

   prim_info.linear = TRUE;
   prim_info.start = 0;
   prim_info.count = count;
   prim_info.elts = NULL;
   prim_info.prim = fpme->input_prim;
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
   prim_info.primitive_lengths = &count;

   llvm_pipeline_generic( middle, &fetch_info, &prim_info );
}


static boolean
llvm_middle_end_linear_run_elts(struct draw_pt_middle_end *middle,
                                unsigned start,
                                unsigned count,
                                const ushort *draw_elts,
                                unsigned draw_count,
                                unsigned prim_flags)
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_fetch_info fetch_info;
   struct draw_prim_info prim_info;

   fetch_info.linear = TRUE;
   fetch_info.start = start;
   fetch_info.count = count;
   fetch_info.elts = NULL;

   prim_info.linear = FALSE;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = fpme->input_prim;
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
   prim_info.primitive_lengths = &draw_count;

   llvm_pipeline_generic( middle, &fetch_info, &prim_info );

   return TRUE;
}


static void
llvm_middle_end_finish(struct draw_pt_middle_end *middle)
{
   /* nothing to do */
}


static void
llvm_middle_end_destroy(struct draw_pt_middle_end *middle)
{
   struct llvm_middle_end *fpme = llvm_middle_end(middle);

   if (fpme->fetch)
      draw_pt_fetch_destroy( fpme->fetch );

   if (fpme->emit)
      draw_pt_emit_destroy( fpme->emit );

   if (fpme->so_emit)
      draw_pt_so_emit_destroy( fpme->so_emit );

   if (fpme->post_vs)
      draw_pt_post_vs_destroy( fpme->post_vs );

   FREE(middle);
}


struct draw_pt_middle_end *
draw_pt_fetch_pipeline_or_emit_llvm(struct draw_context *draw)
{
   struct llvm_middle_end *fpme = 0;

   if (!draw->llvm)
      return NULL;

   fpme = CALLOC_STRUCT( llvm_middle_end );
   if (!fpme)
      goto fail;

   fpme->base.prepare         = llvm_middle_end_prepare;
   fpme->base.bind_parameters = llvm_middle_end_bind_parameters;
   fpme->base.run             = llvm_middle_end_run;
   fpme->base.run_linear      = llvm_middle_end_linear_run;
   fpme->base.run_linear_elts = llvm_middle_end_linear_run_elts;
   fpme->base.finish          = llvm_middle_end_finish;
   fpme->base.destroy         = llvm_middle_end_destroy;

   fpme->draw = draw;

   fpme->fetch = draw_pt_fetch_create( draw );
   if (!fpme->fetch)
      goto fail;

   fpme->post_vs = draw_pt_post_vs_create( draw );
   if (!fpme->post_vs)
      goto fail;

   fpme->emit = draw_pt_emit_create( draw );
   if (!fpme->emit)
      goto fail;

   fpme->so_emit = draw_pt_so_emit_create( draw );
   if (!fpme->so_emit)
      goto fail;

   fpme->llvm = draw->llvm;
   if (!fpme->llvm)
      goto fail;

   fpme->current_variant = NULL;

   return &fpme->base;

 fail:
   if (fpme)
      llvm_middle_end_destroy( &fpme->base );

   return NULL;
}
