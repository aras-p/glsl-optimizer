/*
 * Copyright 2023 Valve Corporation
 * SPDX-License-Identifier: MIT
 */

#include "nir/nir_builder.h"
#include "nir.h"
#include "nir_control_flow.h"
#include "nir_loop_analyze.h"

static bool
is_block_empty(nir_block *block)
{
   return nir_cf_node_is_last(&block->cf_node) &&
          exec_list_is_empty(&block->instr_list);
}

static bool
is_block_singular(nir_block *block)
{
   return nir_cf_node_is_last(&block->cf_node) &&
          (exec_list_is_empty(&block->instr_list) ||
           (exec_list_is_singular(&block->instr_list) && nir_block_ends_in_jump(block)));
}

static bool
nir_block_ends_in_continue(nir_block *block)
{
   if (exec_list_is_empty(&block->instr_list))
      return false;

   nir_instr *instr = nir_block_last_instr(block);
   return instr->type == nir_instr_type_jump &&
          nir_instr_as_jump(instr)->type == nir_jump_continue;
}

/**
 * This optimization tries to merge two equal jump instructions (break or
 * continue) into a single one.
 *
 * This optimization turns
 *
 *     loop {
 *        ...
 *        if (cond) {
 *           do_work_1();
 *           break;
 *        } else {
 *           do_work_2();
 *           break;
 *        }
 *     }
 *
 * into:
 *
 *     loop {
 *        ...
 *        if (cond) {
 *           do_work_1();
 *        } else {
 *           do_work_2();
 *        }
 *        break;
 *     }
 *
 * It does the same with continue statements, respectively.
 *
 */
static bool
opt_loop_merge_break_continue(nir_if *nif)
{
   nir_block *after_if = nir_cf_node_cf_tree_next(&nif->cf_node);

   /* The block after the IF must have no predecessors and be empty. */
   if (nir_block_num_preds(after_if) > 0 || !is_block_empty(after_if))
      return false;

   nir_block *last_then = nir_if_last_then_block(nif);
   nir_block *last_else = nir_if_last_else_block(nif);
   const bool then_break = nir_block_ends_in_break(last_then);
   const bool else_break = nir_block_ends_in_break(last_else);
   const bool then_cont = nir_block_ends_in_continue(last_then);
   const bool else_cont = nir_block_ends_in_continue(last_else);

   /* If both branch legs end with the same jump instruction,
    * merge the statement after the branch
    */
   if ((then_break && else_break) || (then_cont && else_cont)) {
      nir_lower_phis_to_regs_block(last_then->successors[0], false);
      nir_instr_remove_v(nir_block_last_instr(last_then));
      nir_instr *jump = nir_block_last_instr(last_else);
      nir_instr_remove_v(jump);
      nir_instr_insert(nir_after_block(after_if), jump);
      return true;
   }

   return false;
}

/**
 * This optimization simplifies potential loop terminators which then allows
 * other passes such as opt_if_simplification() and loop unrolling to progress
 * further:
 *
 *     if (cond) {
 *        ... then block instructions ...
 *     } else {
 *         ...
 *        break;
 *     }
 *
 * into:
 *
 *     if (cond) {
 *     } else {
 *         ...
 *        break;
 *     }
 *     ... then block instructions ...
 */
static bool
opt_loop_terminator(nir_if *nif)
{
   nir_block *break_blk = NULL;
   nir_block *continue_from_blk = NULL;
   nir_block *first_continue_from_blk = NULL;

   nir_block *last_then = nir_if_last_then_block(nif);
   nir_block *last_else = nir_if_last_else_block(nif);

   if (nir_block_ends_in_break(last_then)) {
      break_blk = last_then;
      continue_from_blk = last_else;
      first_continue_from_blk = nir_if_first_else_block(nif);
   } else if (nir_block_ends_in_break(last_else)) {
      break_blk = last_else;
      continue_from_blk = last_then;
      first_continue_from_blk = nir_if_first_then_block(nif);
   }

   /* Continue if the if-statement contained no jumps at all */
   if (!break_blk)
      return false;

   /* If the continue from block is empty then return as there is nothing to
    * move.
    */
   if (is_block_empty(first_continue_from_blk))
      return false;

   if (nir_block_ends_in_jump(continue_from_blk)) {
      /* Let nir_opt_dead_cf() clean up any dead code. */
      if (!is_block_empty(nir_cf_node_cf_tree_next(&nif->cf_node)))
         return false;

      /* We are about to move the predecessor. */
      nir_lower_phis_to_regs_block(continue_from_blk->successors[0], false);
   }

   /* Even though this if statement has a jump on one side, we may still have
    * phis afterwards.  Single-source phis can be produced by loop unrolling
    * or dead control-flow passes and are perfectly legal.  Run a quick phi
    * removal on the block after the if to clean up any such phis.
    */
   nir_remove_single_src_phis_block(nir_cf_node_as_block(nir_cf_node_next(&nif->cf_node)));

   /* Finally, move the continue from branch after the if-statement. */
   nir_cf_list tmp;
   nir_cf_extract(&tmp, nir_before_block(first_continue_from_blk),
                  nir_after_block(continue_from_blk));
   nir_cf_reinsert(&tmp, nir_after_cf_node(&nif->cf_node));

   return true;
}

/**
 * This optimization tries to merge the jump instruction (break or continue)
 * of a block with an equal one from a previous IF.
 *
 * This optimization turns:
 *
 *     loop {
 *        ...
 *        if (cond) {
 *           do_work_1();
 *           break;
 *        } else {
 *        }
 *        do_work_2();
 *        break;
 *     }
 *
 * into:
 *
 *     loop {
 *        ...
 *        if (cond) {
 *           do_work_1();
 *        } else {
 *           do_work_2();
 *        }
 *        break;
 *     }
 *
 * It does the same with continue statements, respectively.
 *
 */
static bool
opt_loop_last_block(nir_block *block, bool is_trivial_continue, bool is_trivial_break)
{
   /* If this block has no predecessors, let nir_opt_dead_cf() do the cleanup */
   if (nir_block_num_preds(block) == 0)
      return false;

   bool progress = false;
   bool has_break = nir_block_ends_in_break(block);
   bool has_continue = nir_block_ends_in_continue(block);

   /* Remove any "trivial" break and continue, i.e. those that are at the tail
    * of a CF-list where we can just delete the instruction and
    * control-flow will naturally take us to the same target block.
    */
   if ((has_break && is_trivial_break) || (has_continue && is_trivial_continue)) {
      nir_lower_phis_to_regs_block(block->successors[0], false);
      nir_instr_remove_v(nir_block_last_instr(block));
      return true;
   }

   if (!nir_block_ends_in_jump(block)) {
      has_break = is_trivial_break;
      has_continue = is_trivial_continue;
   } else if (is_trivial_continue || is_trivial_break) {
      /* This block ends in a jump that cannot be removed because the implicit
       * fallthrough leads to a different target block.
       *
       * We already optimized this block's jump with the predecessors' when visiting
       * this block with opt_loop_last_block(block, is_trivial_* = false, false).
       */
      return false;
   }

   /* Nothing to do. */
   if (!has_continue && !has_break)
      return false;

   /* Walk backwards and check for previous IF statements whether one of the
    * branch legs ends with an equal jump instruction as this block.
    */
   for (nir_cf_node *prev = nir_cf_node_prev(&block->cf_node); prev != NULL; prev = nir_cf_node_prev(prev)) {
      /* Skip blocks and nested loops */
      if (prev->type != nir_cf_node_if)
         continue;

      nir_if *nif = nir_cf_node_as_if(prev);
      nir_block *then_block = nir_if_last_then_block(nif);
      nir_block *else_block = nir_if_last_else_block(nif);
      if (!nir_block_ends_in_jump(then_block) && !nir_block_ends_in_jump(else_block))
         continue;

      bool merge_into_then = (has_continue && nir_block_ends_in_continue(else_block)) ||
                             (has_break && nir_block_ends_in_break(else_block));
      bool merge_into_else = (has_continue && nir_block_ends_in_continue(then_block)) ||
                             (has_break && nir_block_ends_in_break(then_block));

      if (!merge_into_then && !merge_into_else)
         continue;

      /* If there are single-source phis after the IF, get rid of them first */
      nir_remove_single_src_phis_block(nir_cf_node_cf_tree_next(prev));

      /* We are about to remove one predecessor. */
      nir_lower_phis_to_regs_block(block->successors[0], false);

      nir_cf_list tmp;
      nir_cf_extract(&tmp, nir_after_cf_node(prev), nir_after_block_before_jump(block));

      if (merge_into_then) {
         nir_cf_reinsert(&tmp, nir_after_block(then_block));
      } else {
         nir_cf_reinsert(&tmp, nir_after_block(else_block));
      }

      /* Because we split the current block, the pointer is not valid anymore. */
      block = nir_cf_node_cf_tree_next(prev);
      progress = true;
   }

   /* Revisit the predecessor blocks in order to remove implicit jump instructions. */
   if (is_block_singular(block)) {
      nir_cf_node *prev = nir_cf_node_prev(&block->cf_node);
      if (prev && prev->type == nir_cf_node_if) {
         nir_if *nif = nir_cf_node_as_if(prev);
         progress |= opt_loop_last_block(nir_if_last_then_block(nif), has_continue, has_break);
         progress |= opt_loop_last_block(nir_if_last_else_block(nif), has_continue, has_break);
      }
   }

   return progress;
}

static bool
can_constant_fold(nir_scalar scalar, nir_block *loop_header)
{
   if (nir_scalar_is_const(scalar))
      return true;

   if (nir_scalar_is_alu(scalar)) {
      for (unsigned i = 0; i < nir_op_infos[nir_scalar_alu_op(scalar)].num_inputs; i++) {
         if (nir_op_infos[nir_scalar_alu_op(scalar)].input_sizes[i] > 1 ||
             !can_constant_fold(nir_scalar_chase_alu_src(scalar, i), loop_header))
            return false;
      }
      return true;
   }

   if (nir_def_instr_type(scalar.def) == nir_instr_type_phi) {
      /* If this is a phi from anything but the loop header, we cannot constant-fold. */
      if (nir_def_block(scalar.def) != loop_header)
         return false;

      nir_block *preheader = nir_block_cf_tree_prev(loop_header);
      nir_phi_instr *phi = nir_def_as_phi(scalar.def);
      nir_phi_src *src = nir_phi_get_src_from_block(phi, preheader);
      return nir_src_is_const(src->src);
   }

   return false;
}

static bool
has_phi_with_constant_src(nir_block *block, nir_block *pred)
{
   nir_foreach_phi(phi, block) {
      nir_phi_src *src = nir_phi_get_src_from_block(phi, pred);
      if (nir_src_is_const(src->src))
         return true;
   }

   return false;
}

/* For now, we restrict this optimization to cases where the outer IF
 * can be constant-folded or where at least one phi at the loop-header
 * has a constant loop-carried source. If it can be constant-folded,
 * we additionally require that there is actual work to be done after
 * the initial break. This is to avoid unconditionally unrolling long
 * loops.
 *
 * Note: If this restriction is lifted, it might recurse infinitely.
 *       Prevent by e.g. restricting to single-exit loops.
 */
static bool
should_peel_initial_break(nir_loop *loop)
{
   nir_block *header_block = nir_loop_first_block(loop);
   nir_cf_node *if_node = nir_cf_node_next(&header_block->cf_node);
   nir_if *nif = nir_cf_node_as_if(if_node);

   /* If this loop is supposed to get unrolled, try doing so by peeling
    * one iteration at a time.  This works in cases where loop analysis
    * is not able to calculate the iteration count.
    */
   if (loop->control & nir_loop_control_unroll)
      return can_constant_fold(nir_get_scalar(nif->condition.ssa, 0), header_block);

   /* This loop is already in do-while form. */
   if (loop->do_while)
      return false;

   /* If there is a phi with a constant loop-carried value, we can remove
    * the phi by peeling one iteration.
    */
   if (has_phi_with_constant_src(header_block, nir_loop_last_block(loop)))
      return true;

   return nir_block_contains_work(nir_cf_node_cf_tree_next(if_node)) &&
          can_constant_fold(nir_get_scalar(nif->condition.ssa, 0), header_block);
}

/**
 * This optimization tries to peel the first loop break.
 *
 * This optimization turns:
 *
 *     loop {
 *        do_work_1();
 *        if (cond) {
 *           break;
 *        } else {
 *        }
 *        do_work_2();
 *     }
 *
 * into:
 *
 *     do_work_1();
 *     if (cond) {
 *     } else {
 *        loop {
 *           do_work_2();
 *           do_work_1();
 *           if (cond) {
 *              break;
 *           } else {
 *           }
 *        }
 *     }
 *
 * nir_opt_dead_cf() can later remove the outer IF statement, again.
 *
 */
static bool
opt_loop_peel_initial_break(nir_loop *loop)
{
   nir_block *header_block = nir_loop_first_block(loop);
   nir_block *prev_block = nir_cf_node_cf_tree_prev(&loop->cf_node);
   nir_block *exit_block = nir_cf_node_cf_tree_next(&loop->cf_node);

   /* If we immediately continue/break out from the header block,
    * we can't extract it since we'd be extracting the break as well.
    * Instead, let nir_opt_dead_cf clean up the rest of the loop. */
   if (nir_block_ends_in_jump(header_block))
      return false;

   /* The loop must have exactly one continue block. */
   if (nir_block_num_preds(header_block) != 2)
      return false;

   nir_cf_node *if_node = nir_cf_node_next(&header_block->cf_node);
   if (!if_node || if_node->type != nir_cf_node_if)
      return false;

   nir_if *nif = nir_cf_node_as_if(if_node);
   nir_block *last_then = nir_if_last_then_block(nif);
   if (!nir_block_ends_in_break(last_then) ||
       !is_block_empty(nir_if_first_else_block(nif)) ||
       !nir_is_trivial_loop_if(nif, last_then))
      return false;

   /* If do_work_2() ends in a break or other kind of jump then we can't move
    * it to the top of the loop ahead of do_work_1().
    */
   if (nir_block_ends_in_jump(nir_loop_last_block(loop)))
      return false;

   if (!should_peel_initial_break(loop))
      return false;

   /* Even though this if statement has a jump on one side, we may still have
    * phis afterwards.  Single-source phis can be produced by loop unrolling
    * or dead control-flow passes and are perfectly legal.  Run a quick phi
    * removal on the block after the if to clean up any such phis.
    */
   nir_remove_single_src_phis_block(nir_cf_node_cf_tree_next(if_node));

   /* We need LCSSA because we are going to wrap the loop into an IF. */
   nir_convert_loop_to_lcssa(loop);

   /* We can't lower some derefs to regs or create phis using them, so rematerialize them instead. */
   nir_foreach_instr_safe(instr, header_block) {
      if (instr->type == nir_instr_type_deref)
         nir_rematerialize_deref_in_use_blocks(nir_instr_as_deref(instr));
   }

   /* Lower loop header and LCSSA-phis to regs. */
   nir_lower_phis_to_regs_block(header_block, false);
   nir_lower_ssa_defs_to_regs_block(header_block);
   nir_lower_phis_to_regs_block(exit_block, false);

   /* Extract the loop header including the first break. */
   nir_cf_list tmp;
   nir_cf_extract(&tmp, nir_before_block(header_block),
                  nir_after_cf_node(if_node));
   header_block = nir_loop_first_block(loop);

   /* Clone and re-insert at the continue block. */
   nir_block *cont_block = nir_loop_last_block(loop);
   struct hash_table *remap_table = _mesa_pointer_hash_table_create(NULL);
   nir_cf_list_clone_and_reinsert(&tmp, &loop->cf_node, nir_after_block(cont_block), remap_table);
   _mesa_hash_table_destroy(remap_table, NULL);

   /* Remove the break and insert before the loop. */
   nir_cf_reinsert(&tmp, nir_after_block(prev_block));
   nir_instr_remove_v(nir_block_last_instr(last_then));

   /* Finally, extract the entire loop and insert into the else-branch. */
   nir_cf_extract(&tmp, nir_before_cf_node(&loop->cf_node),
                  nir_after_cf_node(&loop->cf_node));
   nir_cf_reinsert(&tmp, nir_after_block(nir_if_first_else_block(nif)));

   /* This loop is now in do-while form. */
   loop->do_while = true;

   return true;
}

struct merge_term_state {
   nir_shader *shader;
   nir_cursor after_src_if;
   nir_block *old_break_block;
   nir_block *continue_block;
};

static bool
insert_phis_after_terminator_merge(nir_def *def, void *state)
{
   struct merge_term_state *m_state = (struct merge_term_state *)state;

   bool phi_created = false;
   nir_phi_instr *phi_instr = NULL;

   nir_foreach_use_including_if_safe(src, def) {
      /* Don't reprocess the phi we just added */
      if (!nir_src_is_if(src) && phi_instr &&
          nir_src_use_instr(src) == &phi_instr->instr) {
         continue;
      }

      if (nir_src_is_if(src) ||
          (!nir_src_is_if(src) && nir_src_use_instr(src)->block != nir_def_block(def))) {
         if (!phi_created) {
            phi_instr = nir_phi_instr_create(m_state->shader);
            nir_def_init(&phi_instr->instr, &phi_instr->def, def->num_components,
                         def->bit_size);
            nir_instr_insert(nir_after_block(m_state->after_src_if.block),
                             &phi_instr->instr);

            nir_phi_src *phi_src =
               nir_phi_instr_add_src(phi_instr, m_state->continue_block, def);
            list_addtail(&phi_src->src.use_link, &def->uses);

            nir_undef_instr *undef =
               nir_undef_instr_create(m_state->shader,
                                      def->num_components,
                                      def->bit_size);
            nir_instr_insert(nir_after_block(m_state->old_break_block),
                             &undef->instr);
            phi_src = nir_phi_instr_add_src(phi_instr,
                                            m_state->old_break_block,
                                            &undef->def);
            list_addtail(&phi_src->src.use_link, &undef->def.uses);

            phi_created = true;
         }
         assert(phi_instr);
         nir_src_rewrite(src, &phi_instr->def);
      }
   }

   return true;
}

static void
merge_terminators(nir_builder *b, nir_if *dest_if, nir_if *src_if)
{
   /* Move instructions from the block between the ifs into the src
    * if-statements continue block and remove the break from the break block.
    * This helps avoid any potential out of bounds access after the merging
    * moves the break later.
    */
   bool then_break = nir_block_ends_in_break(nir_if_last_then_block(src_if));
   nir_cursor continue_blk_c = then_break ? nir_after_block(nir_if_last_else_block(src_if)) : nir_after_block(nir_if_last_then_block(src_if));

   nir_cf_list tmp;
   nir_cursor after_src_if = nir_after_cf_node(&src_if->cf_node);
   nir_cf_extract(&tmp, after_src_if, nir_before_cf_node(&dest_if->cf_node));
   nir_cf_reinsert(&tmp, continue_blk_c);

   /* Remove the break from the src if-statement */
   nir_block *break_blk = then_break ? nir_if_last_then_block(src_if) : nir_if_last_else_block(src_if);
   nir_instr_remove(nir_block_last_instr(break_blk));

   /* Add phis if needed after we moved instructions to the src if-statements
    * continue block.
    */
   struct merge_term_state m_state;
   m_state.shader = b->shader;
   m_state.after_src_if = nir_after_cf_node(&src_if->cf_node);
   m_state.old_break_block = break_blk;
   m_state.continue_block = continue_blk_c.block;
   /* Use _safe because nir_rematerialize_deref_in_use_blocks might remove dead derefs. */
   nir_foreach_instr_reverse_safe(instr, m_state.continue_block) {
      if (instr->type == nir_instr_type_deref)
         nir_rematerialize_deref_in_use_blocks(nir_instr_as_deref(instr));
      else
         nir_foreach_def(instr, insert_phis_after_terminator_merge, &m_state);
   }

   b->cursor = nir_before_src(&dest_if->condition);

   nir_def *new_c = NULL;
   if (then_break)
      new_c = nir_ior(b, dest_if->condition.ssa, src_if->condition.ssa);
   else
      new_c = nir_iand(b, dest_if->condition.ssa, src_if->condition.ssa);

   nir_src_rewrite(&dest_if->condition, new_c);
}

/* Checks to see if the if-statement is a basic terminator containing no
 * instructions in the branches other than a single break in one of the
 * branches.
 */
static bool
is_basic_terminator_if(nir_if *nif)
{
   nir_block *first_then = nir_if_first_then_block(nif);
   nir_block *first_else = nir_if_first_else_block(nif);
   nir_block *last_then = nir_if_last_then_block(nif);
   nir_block *last_else = nir_if_last_else_block(nif);

   if (first_then != last_then || first_else != last_else)
      return false;

   if (!nir_block_ends_in_break(last_then) &&
       !nir_block_ends_in_break(last_else))
      return false;

   if (nir_block_ends_in_break(last_then)) {
      if (!exec_list_is_empty(&last_else->instr_list) ||
          !exec_list_is_singular(&last_then->instr_list))
         return false;
   } else {
      assert(nir_block_ends_in_break(last_else));
      if (!exec_list_is_empty(&last_then->instr_list) ||
          !exec_list_is_singular(&last_else->instr_list))
         return false;
   }

   return true;
}

/*
 * Merge two consecutive loop terminators. For example:
 *
 *   int i;
 *   for(i = 0; i < n_stop; i++) {
 *      ...
 *
 *     if(0.0 < stops[i])
 *         break;
 *   }
 *
 * This loop checks if the value of stops[i] is greater than 0.0 and if untrue
 * immediately checks n_stop is less than i. If we combine these into a single
 * if the compiler has a greater chance of unrolling the loop.
 */
static bool
opt_loop_merge_terminators(nir_builder *b, nir_if *nif, nir_loop *loop)
{
   if (!loop)
      return false;

   /* If the loop has phis abort any merge attempt */
   nir_block *blk_after_lp = nir_cf_node_cf_tree_next(&loop->cf_node);
   nir_instr *instr_after_loop = nir_block_first_instr(blk_after_lp);
   if (instr_after_loop && instr_after_loop->type == nir_instr_type_phi)
      return false;

   /* Check if we have two consecutive basic terminators */
   if (!is_basic_terminator_if(nif))
      return false;

   nir_block *next_blk = nir_cf_node_cf_tree_next(&nif->cf_node);
   if (!next_blk)
      return false;

   nir_if *next_if = nir_block_get_following_if(next_blk);
   if (!next_if)
      return false;

   if (!is_basic_terminator_if(next_if))
      return false;

   /* If the terminators exit from different branches just abort for now.
    * After further if-statement optimisations are done we should get another
    * go at merging.
    */
   bool break_in_then_f = nir_block_ends_in_break(nir_if_last_then_block(nif));
   bool break_in_then_s = nir_block_ends_in_break(nir_if_last_then_block(next_if));
   if (break_in_then_f != break_in_then_s)
      return false;

   /* Allow some instructions that are acceptable between the terminators
    * these are expected to simply be used by the condition in the second
    * loop terminator.
    */
   nir_foreach_instr(instr, next_blk) {
      if (instr->type == nir_instr_type_phi)
         return false;

      if (instr->type != nir_instr_type_alu &&
          instr->type != nir_instr_type_load_const &&
          instr->type != nir_instr_type_deref &&
          (instr->type != nir_instr_type_intrinsic ||
           (instr->type == nir_instr_type_intrinsic &&
            nir_instr_as_intrinsic(instr)->intrinsic != nir_intrinsic_load_deref))) {
         return false;
      }
   }

   /* If either if-statement has phis abort */
   next_blk = nir_cf_node_cf_tree_next(&next_if->cf_node);
   if (next_blk) {
      nir_foreach_instr(instr, next_blk) {
         if (instr->type == nir_instr_type_phi)
            return false;
      }
   }

   merge_terminators(b, next_if, nif);
   return true;
}

static bool
opt_loop_cf_list(nir_builder *b, struct exec_list *cf_list,
                 nir_loop *current_loop)
{
   bool progress = false;
   foreach_list_typed_safe(nir_cf_node, cf_node, node, cf_list) {
      switch (cf_node->type) {
      case nir_cf_node_block: {
         nir_block *block = nir_cf_node_as_block(cf_node);
         progress |= opt_loop_last_block(block, false, false);
         break;
      }

      case nir_cf_node_if: {
         nir_if *nif = nir_cf_node_as_if(cf_node);
         progress |= opt_loop_cf_list(b, &nif->then_list, current_loop);
         progress |= opt_loop_cf_list(b, &nif->else_list, current_loop);
         progress |= opt_loop_merge_break_continue(nif);
         progress |= opt_loop_terminator(nif);
         progress |= opt_loop_merge_terminators(b, nif, current_loop);
         break;
      }

      case nir_cf_node_loop: {
         nir_loop *loop = nir_cf_node_as_loop(cf_node);
         assert(!nir_loop_has_continue_construct(loop));
         progress |= opt_loop_cf_list(b, &loop->body, loop);
         progress |= opt_loop_last_block(nir_loop_last_block(loop), true, false);
         progress |= opt_loop_peel_initial_break(loop);
         break;
      }

      case nir_cf_node_function:
         UNREACHABLE("Invalid cf type");
      }
   }

   return progress;
}

/**
 * This pass aims to simplify loop control-flow by reducing the number
 * of break and continue statements.
 */
bool
nir_opt_loop(nir_shader *shader)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      nir_builder b = nir_builder_create(impl);

      /* First we run the simple pass to get rid of pesky continues */
      if (opt_loop_cf_list(&b, &impl->body, NULL)) {
         nir_progress(true, impl, nir_metadata_none);

         /* If that made progress, we're no longer really in SSA form. */
         nir_lower_reg_intrinsics_to_ssa_impl(impl);
         progress = true;
      } else {
         nir_no_progress(impl);
      }
   }

   return progress;
}
