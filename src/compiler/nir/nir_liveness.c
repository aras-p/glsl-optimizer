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
#include "nir_vla.h"
#include "nir_worklist.h"

/*
 * Basic liveness analysis.  This works only in SSA form.
 *
 * This liveness pass treats phi nodes as being melded to the space between
 * blocks so that the destinations of a phi are in the livein of the block
 * in which it resides and the sources are in the liveout of the
 * corresponding block.  By formulating the liveness information in this
 * way, we ensure that the definition of any variable dominates its entire
 * live range.  This is true because the only way that the definition of an
 * SSA value may not dominate a use is if the use is in a phi node and the
 * uses in phi no are in the live-out of the corresponding predecessor
 * block but not in the live-in of the block containing the phi node.
 */

struct live_defs_state {
   unsigned num_bits;
   void *mem_ctx;

   /* Used in propagate_across_edge() */
   struct u_sparse_bitset tmp_live;

   nir_block_worklist worklist;
};

/* Initialize the liveness data to zero and add the given block to the
 * worklist.
 */
static void
init_liveness_block(nir_block *block,
                    struct live_defs_state *state)
{
   u_sparse_bitset_init(&block->live_in, state->num_bits, state->mem_ctx);
   u_sparse_bitset_init(&block->live_out, state->num_bits, state->mem_ctx);
   nir_block_worklist_push_head(&state->worklist, block);
}

static bool
set_src_live(nir_src *src, void *void_live)
{
   struct u_sparse_bitset *live = void_live;

   if (nir_src_is_undef(*src))
      return true; /* undefined variables are never live */

   u_sparse_bitset_set(live, src->ssa->index);

   return true;
}

static bool
set_ssa_def_dead(nir_def *def, void *void_live)
{
   struct u_sparse_bitset *live = void_live;

   u_sparse_bitset_clear(live, def->index);

   return true;
}

/** Propagates the live in of succ across the edge to the live out of pred
 *
 * Phi nodes exist "between" blocks and all the phi nodes at the start of a
 * block act "in parallel".  When we propagate from the live_in of one
 * block to the live out of the other, we have to kill any writes from phis
 * and make live any sources.
 *
 * Returns true if updating live out of pred added anything
 */
static bool
propagate_across_edge(nir_block *pred, nir_block *succ,
                      struct live_defs_state *state)
{
   struct u_sparse_bitset *live = &state->tmp_live;
   u_sparse_bitset_dup(live, &succ->live_in);

   nir_foreach_phi(phi, succ) {
      set_ssa_def_dead(&phi->def, live);
   }

   nir_foreach_phi(phi, succ) {
      nir_foreach_phi_src(src, phi) {
         if (src->pred == pred) {
            set_src_live(&src->src, live);
            break;
         }
      }
   }

   bool progress = u_sparse_bitset_merge(&pred->live_out, live);
   u_sparse_bitset_free(live);
   return progress;
}

void
nir_live_defs_impl(nir_function_impl *impl)
{
   struct live_defs_state state = {
      .num_bits = impl->ssa_alloc,
      .mem_ctx = impl,
   };

   nir_block_worklist_init(&state.worklist, impl->num_blocks, NULL);

   /* Allocate live_in and live_out sets and add all of the blocks to the
    * worklist.
    */
   nir_foreach_block(block, impl) {
      init_liveness_block(block, &state);
   }

   /* We're now ready to work through the worklist and update the liveness
    * sets of each of the blocks.  By the time we get to this point, every
    * block in the function implementation has been pushed onto the
    * worklist in reverse order.  As long as we keep the worklist
    * up-to-date as we go, everything will get covered.
    */
   while (!nir_block_worklist_is_empty(&state.worklist)) {
      /* We pop them off in the reverse order we pushed them on.  This way
       * the first walk of the instructions is backwards so we only walk
       * once in the case of no control flow.
       */
      nir_block *block = nir_block_worklist_pop_head(&state.worklist);

      u_sparse_bitset_dup(&block->live_in, &block->live_out);

      nir_if *following_if = nir_block_get_following_if(block);
      if (following_if)
         set_src_live(&following_if->condition, &block->live_in);

      nir_foreach_instr_reverse(instr, block) {
         /* Phi nodes are handled seperately so we want to skip them.  Since
          * we are going backwards and they are at the beginning, we can just
          * break as soon as we see one.
          */
         if (instr->type == nir_instr_type_phi)
            break;

         nir_foreach_def(instr, set_ssa_def_dead, &block->live_in);
         nir_foreach_src(instr, set_src_live, &block->live_in);
      }

      /* Walk over all of the predecessors of the current block updating
       * their live in with the live out of this one.  If anything has
       * changed, add the predecessor to the work list so that we ensure
       * that the new information is used.
       */
      nir_foreach_pred(pred, block) {
         if (propagate_across_edge(pred, block, &state))
            nir_block_worklist_push_tail(&state.worklist, pred);
      }
   }

   nir_block_worklist_fini(&state.worklist);
}

/** Return the live set at a cursor
 *
 * Note: The bitset returned may be the live_in or live_out from the block in
 *       which the instruction lives.  Do not ralloc_free() it directly;
 *       instead, provide a mem_ctx and free that.
 */
struct u_sparse_bitset *
nir_get_live_defs(nir_cursor cursor, void *mem_ctx)
{
   nir_block *block = nir_cursor_current_block(cursor);
   ASSERTED nir_function_impl *impl = block->impl;
   assert(impl->valid_metadata & nir_metadata_live_defs);

   switch (cursor.option) {
   case nir_cursor_before_block:
      return &cursor.block->live_in;

   case nir_cursor_after_block:
      return &cursor.block->live_out;

   case nir_cursor_before_instr:
      if (cursor.instr == nir_block_first_instr(cursor.instr->block))
         return &cursor.instr->block->live_in;
      break;

   case nir_cursor_after_instr:
      if (cursor.instr == nir_block_last_instr(cursor.instr->block))
         return &cursor.instr->block->live_out;
      break;
   }

   /* If we got here, we're an instruction cursor mid-block */
   struct u_sparse_bitset *live = rzalloc_size(mem_ctx, sizeof(struct u_sparse_bitset));
   u_sparse_bitset_dup_with_ctx(live, &block->live_out, mem_ctx);

   nir_foreach_instr_reverse(instr, block) {
      if (cursor.option == nir_cursor_after_instr && instr == cursor.instr)
         break;

      /* If someone asked for liveness in the middle of a bunch of phis,
       * that's an error.  Since we are going backwards and they are at the
       * beginning, we can just blow up as soon as we see one.
       */
      assert(instr->type != nir_instr_type_phi);
      if (instr->type == nir_instr_type_phi)
         break;

      nir_foreach_def(instr, set_ssa_def_dead, live);
      nir_foreach_src(instr, set_src_live, live);

      if (cursor.option == nir_cursor_before_instr && instr == cursor.instr)
         break;
   }

   return live;
}

static bool
src_does_not_use_def(nir_src *src, void *def)
{
   return src->ssa != (nir_def *)def;
}

static bool
search_for_use_after_instr(nir_instr *start, nir_def *def)
{
   /* Only look for a use strictly after the given instruction */
   struct exec_node *node = start->node.next;
   while (!exec_node_is_tail_sentinel(node)) {
      nir_instr *instr = exec_node_data(nir_instr, node, node);
      if (!nir_foreach_src(instr, src_does_not_use_def, def))
         return true;
      node = node->next;
   }

   /* If uses are considered to be in the block immediately preceding the if
    * so we need to also check the following if condition, if any.
    */
   nir_if *following_if = nir_block_get_following_if(start->block);
   if (following_if && following_if->condition.ssa == def)
      return true;

   return false;
}

/* Returns true if def is live at instr assuming that def comes before
 * instr in a pre DFS search of the dominance tree.
 */
static bool
nir_def_is_live_at(nir_def *def, nir_instr *instr)
{
   if (u_sparse_bitset_test(&instr->block->live_out, def->index)) {
      /* Since def dominates instr, if def is in the liveout of the block,
       * it's live at instr
       */
      return true;
   } else {
      if (u_sparse_bitset_test(&instr->block->live_in, def->index) ||
          nir_def_block(def) == instr->block) {
         /* In this case it is either live coming into instr's block or it
          * is defined in the same block.  In this case, we simply need to
          * see if it is used after instr.
          */
         return search_for_use_after_instr(instr, def);
      } else {
         return false;
      }
   }
}

bool
nir_defs_interfere(nir_def *a, nir_def *b)
{
   nir_instr *a_instr = nir_def_instr(a);
   nir_instr *b_instr = nir_def_instr(b);
   if (a_instr == b_instr) {
      /* Two variables defined at the same time interfere assuming at
       * least one isn't dead.
       */
      return true;
   } else if (a_instr->type == nir_instr_type_undef ||
              b_instr->type == nir_instr_type_undef) {
      /* If either variable is an ssa_undef, then there's no interference */
      return false;
   } else if (a_instr->index < b_instr->index) {
      return nir_def_is_live_at(a, b_instr);
   } else {
      return nir_def_is_live_at(b, a_instr);
   }
}
