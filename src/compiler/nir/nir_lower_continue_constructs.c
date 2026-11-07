/*
 * Copyright © 2021 Valve Corporation
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
 *
 */

#include "nir.h"
#include "nir_builder.h"
#include "nir_control_flow.h"

/* NIR pass to lower loop continue constructs.
 *
 * NIR loops are maintained in canonical form with these properties:
 *  - a pre-header: the only predecessor of the loop header
 *  - a dedicated exit node: dominated by the loop-header
 *  - a single back-edge to the loop header: the trivial continue
 *
 * If the loop has a continue construct, the trivial continue is the
 * back-edge from the last block of the continue construct to the loop
 * header. Otherwise, it is the back-edge from the last block of the
 * loop body to the loop header.
 *
 * In order to lower the continue construct of a loop, all continue
 * statements are being removed by either
 * - moving the following code to the other side of a branch or
 * - guarding following code by inserted IF-statements
 *
 * Afterwards, the continue construct is inlined before the trivial
 * back-edge.
 *
 */

struct loop_simplify_state {
   nir_builder *b;
   nir_def *jump_flag;
   struct exec_list *cf_list;
   nir_jump_type type;
   nir_loop *loop;
};

static bool
block_ends_in_jump_type(nir_block *block, nir_jump_type type)
{
   if (nir_block_ends_in_jump(block)) {
      nir_jump_instr *jump = nir_instr_as_jump(nir_block_last_instr(block));
      return jump->type == type;
   }

   return false;
}

static bool
lower_jumps_in_cf_list(struct exec_list *cf_list,
                           struct loop_simplify_state *state);

static bool
lower_jump(nir_block *block, struct loop_simplify_state *state)
{
   if (!block_ends_in_jump_type(block, state->type))
      return false;

   assert(nir_cf_node_is_last(&block->cf_node));

   /* Remove the jump instruction and set the predicate to 'true'. */
   state->b->cursor = nir_instr_remove(nir_block_last_instr(block));
   nir_store_reg(state->b, nir_imm_true(state->b), state->jump_flag);

   return true;
}

static bool
cf_list_ends_in_jump(struct exec_list *cf_list)
{
   foreach_list_typed(nir_cf_node, node, node, cf_list) {
      if (node->type == nir_cf_node_block) {
         if (nir_block_ends_in_jump(nir_cf_node_as_block(node)))
            return true;
      } else if (node->type == nir_cf_node_if) {
         nir_if *nif = nir_cf_node_as_if(node);
         if (cf_list_ends_in_jump(&nif->then_list) &&
             cf_list_ends_in_jump(&nif->else_list))
            return true;
      }
   }

   return false;
}

static bool
lower_jumps_in_if(nir_if *nif, struct loop_simplify_state *state)
{
   /* Lower phis before jump removal makes more predecessors reachable.
    * Push their writes up the predecessor tree so newly reachable paths
    * cannot bypass the definitions used by the writes.
    */
   nir_block *next_block = nir_cf_node_cf_tree_next(&nif->cf_node);
   nir_lower_phis_to_regs_block(next_block, false);

   nir_block *then_block = nir_if_last_then_block(nif);
   nir_block *else_block = nir_if_last_else_block(nif);
   /* A branch leg can also exit through jumps in both sides of a nested if.
    * Detect this before lowering so following code is moved into the
    * surviving branch leg before any of its jumps are removed.
    */
   bool then_jumps = cf_list_ends_in_jump(&nif->then_list);
   bool else_jumps = cf_list_ends_in_jump(&nif->else_list);

   bool progress = false;
   progress |= lower_jump(then_block, state);
   progress |= lower_jump(else_block, state);

   bool is_empty_block = nir_cf_node_is_last(&next_block->cf_node) &&
                         exec_list_is_empty(&next_block->instr_list);

   /* If a branch leg ends in a jump, we lower already any continue/break statements,
    * so that we know if we have to move the following blocks to the other side.
    */
   if (then_jumps)
      progress |= lower_jumps_in_cf_list(&nif->then_list, state);
   if (else_jumps)
      progress |= lower_jumps_in_cf_list(&nif->else_list, state);

   if (!is_empty_block && progress) {
      /* If one side ends in a jump and has a jump that we need to remove, move the
       * following code to the other side.  This is necessary to maintain SSA
       * dominance, because the jump gets guarded or removed.
       * Doing this based on (then_jumps || else_jumps) would also be a valid
       * transform.  However, it's unnecessary if we don't have any jumps
       * to lower and increases control-flow depth.
       */
      assert(then_jumps || else_jumps);
      nir_cf_list list;
      nir_cf_extract(&list, nir_after_cf_node(&nif->cf_node),
                     nir_after_cf_list(state->cf_list));

      /* If this is top-level within the loop, phi lowering from the caller
       * should handle this.
       */
      if (nif->cf_node.parent != &state->loop->cf_node)
         nir_cf_list_detach_ssa(&list);

      if (then_jumps && else_jumps) {
         /* Both branches jump, just delete instructions following the IF. */
         nir_cf_delete(&list);
      } else if (then_jumps) {
         nir_cf_reinsert(&list, nir_after_cf_list(&nif->else_list));
      } else {
         nir_cf_reinsert(&list, nir_after_cf_list(&nif->then_list));
      }

      /* The successor is now empty. No need to predicate following blocks. */
      is_empty_block = true;
   }

   /* Recursively lower any jump statements for the sides that didn't jump.
    * We do that here so that this also lowers the code which was after the IF
    * which we just moved inside.
    */
   if (!then_jumps)
      progress |= lower_jumps_in_cf_list(&nif->then_list, state);
   if (!else_jumps)
      progress |= lower_jumps_in_cf_list(&nif->else_list, state);

   if (!is_empty_block && progress) {
      /* Predicate following blocks. */
      nir_cf_list list;
      nir_cf_extract(&list, nir_after_cf_node(&nif->cf_node),
                     nir_after_cf_list(state->cf_list));

      if (nif->cf_node.parent != &state->loop->cf_node)
         nir_cf_list_detach_ssa(&list);

      state->b->cursor = nir_after_cf_node(&nif->cf_node);
      nir_if *if_stmt = nir_push_if(state->b, nir_load_reg(state->b, state->jump_flag));

      nir_cf_reinsert(&list, nir_before_cf_list(&if_stmt->else_list));
      nir_pop_if(state->b, NULL);
   }

   return progress;
}

static bool
lower_jumps_in_cf_list(struct exec_list *cf_list,
                       struct loop_simplify_state *state)
{
   bool progress = false;

   struct exec_list *parent_list = state->cf_list;
   state->cf_list = cf_list;

   /* We iterate over the list backwards because any given lower call may
    * take everything following the given CF node and predicate it.  In
    * order to avoid recursion/iteration problems, we want everything after
    * a given node to already be lowered before this happens.
    */
   foreach_list_typed_reverse_safe(nir_cf_node, node, node, cf_list) {
      switch (node->type) {
      case nir_cf_node_if:
         if (lower_jumps_in_if(nir_cf_node_as_if(node), state))
            progress = true;
         break;

      case nir_cf_node_block:
      case nir_cf_node_loop:
         break;

      default:
         UNREACHABLE("Invalid inner CF node type");
      }
   }

   state->cf_list = parent_list;

   return progress;
}

/* Simplify continues/breaks in the loop.
 *
 * For continues, this means removing all explicit jumps,
 * only leaving the trivial back edge.
 *
 * For breaks, insert a single conditional break just before
 * the back edge.
 *
 * It's assumed that all phis for the jumps in question are
 * lowered to registers before the call to this function.
 */
void
nir_simplify_loop(nir_loop *loop, nir_jump_type type)
{
   assert(type == nir_jump_continue || type == nir_jump_break);

   struct loop_simplify_state state;
   state.type = type;
   nir_builder b = nir_builder_at(nir_before_block_after_phis(nir_loop_first_block(loop)));
   state.b = &b;
   state.loop = loop;

   /* Initialize the variable to False. */
   state.jump_flag = nir_decl_reg(&b, 1, 1, 0);
   nir_store_reg(&b, nir_imm_false(&b), state.jump_flag);

   lower_jumps_in_cf_list(&loop->body, &state);

   if (type == nir_jump_break) {
      b.cursor = nir_after_cf_list(&loop->body);
      nir_break_if(&b, nir_load_reg(&b, state.jump_flag));
   }
}

static void
simplify_loop_continues(nir_loop *loop)
{
   nir_block *cont = nir_loop_first_continue_block(loop);
   nir_block *last = nir_loop_last_block(loop);

   /* Remove trivial continue statement. */
   if (block_ends_in_jump_type(last, nir_jump_continue))
      nir_instr_remove(nir_block_last_instr(last));

   /* If the loop has only the trivial continue, there is nothing to do. */
   if (!nir_block_ends_in_jump(last) && nir_block_num_preds(cont) == 1)
      return;

   nir_simplify_loop(loop, nir_jump_continue);
}

static bool
lower_loop_continue_block(nir_builder *b, nir_loop *loop)
{
   if (!nir_loop_has_continue_construct(loop))
      return false;

   /* Lower loop header and continue-phis to regs as we are going to move the predecessors. */
   nir_lower_phis_to_regs_block(nir_loop_first_block(loop), true);
   nir_lower_phis_to_regs_block(nir_loop_first_continue_block(loop), true);

   /* Simplify the loop in order to ensure that it has at most one back-edge. */
   simplify_loop_continues(loop);

   nir_cf_list extracted;
   nir_cf_list_extract(&extracted, &loop->continue_list);

   if (nir_block_num_preds(nir_loop_first_continue_block(loop)) == 0) {
      /* This loop doesn't continue at all. Delete the continue construct. */
      nir_cf_delete(&extracted);
   } else {
      /* Inline the continue construct before the trivial continue. */
      nir_cf_reinsert(&extracted, nir_after_cf_list(&loop->body));
   }

   nir_loop_remove_continue_construct(loop);
   return true;
}

static bool
visit_cf_list(nir_builder *b, struct exec_list *list)
{
   bool progress = false;

   foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
      case nir_cf_node_block:
         continue;
      case nir_cf_node_if: {
         nir_if *nif = nir_cf_node_as_if(node);
         progress |= visit_cf_list(b, &nif->then_list);
         progress |= visit_cf_list(b, &nif->else_list);
         break;
      }
      case nir_cf_node_loop: {
         nir_loop *loop = nir_cf_node_as_loop(node);
         /* By first lowering inner loops, we ensure that we don't encounter
          * any continue statements which don't belong to the current loop.
          */
         progress |= visit_cf_list(b, &loop->body);

         /* If we lower continue constructs after inlining functions, they
          * might contain nested loops.
          */
         progress |= visit_cf_list(b, &loop->continue_list);

         /* Lower continue construct. */
         progress |= lower_loop_continue_block(b, loop);
         break;
      }
      case nir_cf_node_function:
         UNREACHABLE("Unsupported cf_node type.");
      }
   }

   return progress;
}

static bool
lower_continue_constructs_impl(nir_function_impl *impl)
{
   nir_builder b = nir_builder_create(impl);
   bool progress = visit_cf_list(&b, &impl->body);

   if (progress) {
      nir_progress(true, impl, nir_metadata_none);

      /* Merge the Phis from Header and Continue Target */
      nir_lower_reg_intrinsics_to_ssa_impl(impl);
   } else {
      nir_no_progress(impl);
   }

   return progress;
}

bool
nir_lower_continue_constructs(nir_shader *shader)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      if (lower_continue_constructs_impl(impl))
         progress = true;
   }

   return progress;
}
