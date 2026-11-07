/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "nir.h"

/*
 * Handles management of the metadata.
 */

void
nir_metadata_require(nir_function_impl *impl, nir_metadata required, ...)
{
#define NEEDS_UPDATE(X) ((required & ~impl->valid_metadata) & (X))

   if (NEEDS_UPDATE(nir_metadata_block_index))
      nir_index_blocks(impl);
   if (NEEDS_UPDATE(nir_metadata_instr_index))
      nir_index_instrs(impl);
   if (NEEDS_UPDATE(nir_metadata_dominance))
      nir_calc_dominance_impl(impl);
   if (NEEDS_UPDATE(nir_metadata_dominance_lca))
      nir_calc_dominance_lca_impl(impl);
   if (NEEDS_UPDATE(nir_metadata_live_defs))
      nir_live_defs_impl(impl);
   if (NEEDS_UPDATE(nir_metadata_divergence)) /* glsl-optimizer change: no divergence analysis */
      UNREACHABLE("divergence analysis not available in glsl-optimizer");
   if (required & nir_metadata_loop_analysis) {
      va_list ap;
      va_start(ap, required);
      /* !! Warning !! Do not move these va_arg() call directly to
       * nir_loop_analyze_impl() as parameters because the execution order will
       * become undefined.
       */
      nir_variable_mode indirect_mask = va_arg(ap, nir_variable_mode);
      int force_unroll_sampler_indirect = va_arg(ap, int);
      va_end(ap);

      if (NEEDS_UPDATE(nir_metadata_loop_analysis) ||
          indirect_mask != impl->loop_analysis_indirect_mask ||
          force_unroll_sampler_indirect != impl->loop_analysis_force_unroll_sampler_indirect) {
         nir_loop_analyze_impl(impl, indirect_mask, force_unroll_sampler_indirect);
      }
   }

#undef NEEDS_UPDATE

   impl->valid_metadata |= required;
}

bool
nir_progress(bool progress, nir_function_impl *impl, nir_metadata preserved)
{
   /* If we do not make progress, we preserve all metadata. */
   if (!progress)
      preserved = nir_metadata_all;

   if (!(preserved & nir_metadata_block_index))
      assert(!(preserved & nir_metadata_dominance));
   if (!(preserved & nir_metadata_dominance))
      assert(!(preserved & nir_metadata_dominance_lca));

   /* If we discard valid liveness information, immediately free the
    * liveness information for each block. For large shaders, it can
    * consume a huge amount of memory, and it's usually not immediately
    * needed after dirtying.
    */
   if ((impl->valid_metadata & ~preserved) & nir_metadata_live_defs) {
      nir_foreach_block(block, impl) {
         u_sparse_bitset_free(&block->live_in);
         u_sparse_bitset_free(&block->live_out);
      }
   }

   impl->valid_metadata &= preserved;
   return progress;
}

void
nir_shader_preserve_all_metadata(nir_shader *shader)
{
   nir_foreach_function_impl(impl, shader) {
      nir_no_progress(impl);
   }
}

void
nir_metadata_invalidate(nir_shader *shader)
{
   nir_foreach_function_impl(impl, shader) {
      unsigned instr_idx = UINT32_MAX;
      unsigned block_idx = UINT32_MAX;

      nir_foreach_block_unstructured(block, impl) {
         /* This creates an index that is non-unique, backwards and very large. */
         block->index = (block_idx-- & 0xf) + 0xfffffff0;

         if (impl->valid_metadata & nir_metadata_live_defs) {
            u_sparse_bitset_free(&block->live_in);
            u_sparse_bitset_free(&block->live_out);
         }
         u_sparse_bitset_init(&block->live_in, impl->ssa_alloc, NULL);
         u_sparse_bitset_init(&block->live_out, impl->ssa_alloc, NULL);

         if (impl->valid_metadata & nir_metadata_dominance &&
             block->dom_children != block->_dom_children_storage)
            ralloc_free(block->dom_children);
         block->dom_children = NULL;
         block->num_dom_children = 1;
         block->dom_pre_index = block->dom_post_index = 0;
         _mesa_set_clear(&block->dom_frontier, NULL);

         if (block->cf_node.parent->type == nir_cf_node_loop &&
             nir_cf_node_is_first(&block->cf_node)) {
            nir_loop *loop = nir_cf_node_as_loop(block->cf_node.parent);
            if (impl->valid_metadata & nir_metadata_loop_analysis)
               ralloc_free(loop->info);
            loop->info = NULL;
         }

         block->start_ip = (instr_idx-- & 0xf) + 0xfffffff0;
         nir_foreach_instr(instr, block)
            instr->index = (instr_idx-- & 0xf) + 0xfffffff0;
         block->end_ip = (instr_idx-- & 0xf) + 0xfffffff0;
      }

      impl->num_blocks = 0;
      impl->end_block->index = 0xf;

      impl->valid_metadata = 0;
   }
}

#ifndef NDEBUG
/**
 * Make sure passes properly invalidate metadata (part 1).
 *
 * Call this before running a pass to set a bogus metadata flag, which will
 * only be preserved if the pass forgets to call nir_progress().
 */
void
nir_metadata_set_validation_flag(nir_shader *shader)
{
   nir_foreach_function_impl(impl, shader) {
      impl->valid_metadata |= nir_metadata_not_properly_reset;
   }
}

/**
 * Make sure passes properly invalidate metadata (part 2).
 *
 * Call this after a pass makes progress to verify that the bogus metadata set by
 * the earlier function was properly thrown away.  Note that passes may not call
 * nir_progress() if they don't actually make any changes at all.
 */
void
nir_metadata_check_validation_flag(nir_shader *shader)
{
   nir_foreach_function_impl(impl, shader) {
      assert(!(impl->valid_metadata & nir_metadata_not_properly_reset));
   }
}

void
nir_metadata_require_most(nir_shader *shader)
{
   bool force_unroll_sampler_indirect = shader->options->force_indirect_unrolling_sampler;
   nir_variable_mode indirect_mask = shader->options->force_indirect_unrolling;
   nir_foreach_function_impl(impl, shader) {
      nir_metadata md = nir_metadata_all;

      /* We don't know if divergence analysis supports this shader. */
      md &= ~nir_metadata_divergence;

      if (!impl->structured) {
         /* These don't support unstructured control flow. */
         md &= ~nir_metadata_instr_index;
         md &= ~nir_metadata_loop_analysis;
         md &= ~nir_metadata_live_defs;
      }

      nir_metadata_require(impl, md, indirect_mask,
                           (int)force_unroll_sampler_indirect);
   }
}
#endif
