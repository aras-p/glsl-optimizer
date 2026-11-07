/*
 * Copyright © 2015 Thomas Helland
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

#include "nir_loop_analyze.h"
#include "util/hash_table.h"
#include "util/ralloc.h"
#include "nir.h"
#include "nir_constant_expressions.h"
#include "nir_search_helpers.h"

#define IS_UNIFORM_UNKNOWN 0
#define IS_UNIFORM_TRUE    1
#define IS_UNIFORM_FALSE   2

typedef struct {
   /* The loop we store information for */
   nir_loop *loop;

   nir_variable_mode indirect_mask;

   bool force_unroll_sampler_indirect;

   struct hash_table *range_ht;
} loop_info_state;

static nir_loop_induction_variable *
get_loop_var(nir_def *value, loop_info_state *state)
{
   struct hash_entry *entry = _mesa_hash_table_search(state->loop->info->induction_vars, value);
   if (entry)
      return entry->data;
   else
      return NULL;
}

/* If an instruction only depends on basic induction variables
 * and constants, we know that it will be eliminated once the
 * loop is unrolled.
 */
static bool
is_const_after_unrolling(loop_info_state *state, nir_def *def)
{
   nir_instr *instr = nir_def_instr(def);
   if (instr->pass_flags == 0)
      return false;

   /* The pass flags are only correct within the loop. */
   return instr->block->index >= nir_loop_first_block(state->loop)->index;
}

/** Calculate an estimated cost in number of instructions
 *
 * We do this so that we don't unroll loops which will later get massively
 * inflated due to int64 or fp64 lowering.  The estimates provided here don't
 * have to be massively accurate; they just have to be good enough that loop
 * unrolling doesn't cause things to blow up too much.
 */
static unsigned
instr_cost(loop_info_state *state, nir_instr *instr,
           const nir_shader_compiler_options *options)
{
   if (instr->type == nir_instr_type_intrinsic ||
       instr->type == nir_instr_type_tex)
      return 1;

   if (instr->type != nir_instr_type_alu)
      return 0;

   nir_alu_instr *alu = nir_instr_as_alu(instr);
   const nir_op_info *info = &nir_op_infos[alu->op];
   unsigned cost = 1;

   /* Check if this instruction can be constant-folded after unrolling. */
   if (is_const_after_unrolling(state, &alu->def))
      return 0;

   if (nir_op_is_selection(alu->op) && is_const_after_unrolling(state, alu->src[0].src.ssa)) {
      /* If the condition can be constant folded after the loop is unrolled,
       * so can the selection.
       */
      return 0;
   } else if (nir_op_is_vec_or_mov(alu->op)) {
      /* movs and vecs are likely free. */
      return 0;
   } else if ((alu->op == nir_op_fmul || alu->op == nir_op_fmulz) &&
              is_only_used_by_fadd(alu)) {
      /* If we can fuse fma/mad, do not count the mul. */
      return 0;
   }

   if (alu->op == nir_op_flrp) {
      if ((options->lower_flrp16 && alu->def.bit_size == 16) ||
          (options->lower_flrp32 && alu->def.bit_size == 32) ||
          (options->lower_flrp64 && alu->def.bit_size == 64))
         cost *= 3;
   }

   /* Assume everything 16 or 32-bit is cheap.
    *
    * There are no 64-bit ops that don't have a 64-bit thing as their
    * destination or first source.
    */
   if (alu->def.bit_size < 64 &&
       nir_src_bit_size(alu->src[0].src) < 64)
      return cost;

   bool is_fp64 = alu->def.bit_size == 64 &&
                  nir_alu_type_get_base_type(info->output_type) == nir_type_float;
   for (unsigned i = 0; i < info->num_inputs; i++) {
      if (nir_src_bit_size(alu->src[i].src) == 64 &&
          nir_alu_type_get_base_type(info->input_types[i]) == nir_type_float)
         is_fp64 = true;
   }

   if (is_fp64) {
      /* If it's something lowered normally, it's expensive. */
      if (options->lower_doubles_options &
          nir_lower_doubles_op_to_options_mask(alu->op))
         cost *= 20;

      /* If it's full software, it's even more expensive */
      if (options->lower_doubles_options & nir_lower_fp64_full_software) {
         cost *= 100;
         state->loop->info->has_soft_fp64 = true;
      }

      return cost;
   } else {
      if (options->lower_int64_options &
          nir_lower_int64_op_to_options_mask(alu->op)) {
         /* These require a doing the division algorithm. */
         if (alu->op == nir_op_idiv || alu->op == nir_op_udiv ||
             alu->op == nir_op_imod || alu->op == nir_op_umod ||
             alu->op == nir_op_irem)
            return cost * 100;

         /* Other int64 lowering isn't usually all that expensive */
         return cost * 5;
      }

      return cost;
   }
}

/* If all of the instruction sources point to identical ALU instructions (as
 * per nir_instrs_equal), return one of the ALU instructions.  Otherwise,
 * return NULL.
 */
static nir_alu_instr *
phi_instr_as_alu(nir_phi_instr *phi)
{
   nir_alu_instr *first = NULL;
   nir_foreach_phi_src(src, phi) {
      nir_alu_instr *alu = nir_src_as_alu(src->src);
      if (!alu)
         return NULL;

      if (first == NULL) {
         first = alu;
      } else {
         if (!nir_instrs_equal(&first->instr, &alu->instr))
            return NULL;
      }
   }

   return first;
}

static bool
alu_src_has_identity_swizzle(nir_alu_instr *alu, unsigned src_idx)
{
   assert(nir_op_infos[alu->op].input_sizes[src_idx] == 0);
   for (unsigned i = 0; i < alu->def.num_components; i++) {
      if (alu->src[src_idx].swizzle[i] != i)
         return false;
   }

   return true;
}

static bool
is_only_uniform_src(nir_src *src)
{
   nir_instr *instr = nir_def_instr(src->ssa);

   /* Lookup memoized result */
   if (instr->pass_flags == IS_UNIFORM_TRUE)
      return true;
   if (instr->pass_flags == IS_UNIFORM_FALSE)
      return false;

   bool result = true;
   switch (instr->type) {
   case nir_instr_type_alu: {
      /* Return true if all sources return true. */
      nir_alu_instr *alu = nir_instr_as_alu(instr);
      for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
         if (!is_only_uniform_src(&alu->src[i].src)) {
            result = false;
            break;
         }
      }
      break;
   }

   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *inst = nir_instr_as_intrinsic(instr);
      /* current uniform inline only support load ubo */
      result = (inst->intrinsic == nir_intrinsic_load_ubo);
      break;
   }

   case nir_instr_type_load_const:
      /* Always return true for constants. */
      result = true;
      break;

   default:
      result = false;
      break;
   }
   /* Store memoized result */
   instr->pass_flags = result ? IS_UNIFORM_TRUE : IS_UNIFORM_FALSE;

   return result;
}

static bool
compute_induction_information(loop_info_state *state)
{
   bool progress = false;

   /* We are only interested in checking phis for the basic induction
    * variable case as its simple to detect. All basic induction variables
    * have a phi node
    */
   nir_block *header = nir_loop_first_block(state->loop);
   nir_block *preheader = nir_block_cf_tree_prev(header);

   nir_foreach_phi(phi, header) {
      nir_loop_induction_variable var = { .basis = &phi->def };

      nir_foreach_phi_src(phi_src, phi) {
         nir_def *src = phi_src->src.ssa;

         if (phi_src->pred == preheader) {
            var.init_src = &phi_src->src;
            continue;
         }

         /* If one of the sources is in an if branch or nested loop then don't
          * attempt to go any further.
          */
         if (nir_def_block(src)->cf_node.parent != &state->loop->cf_node)
            break;

         /* Detect inductions variables that are incremented in both branches
          * of an unnested if rather than in a loop block.
          */
         if (nir_def_is_phi(src)) {
            nir_phi_instr *src_phi = nir_def_as_phi(src);
            nir_alu_instr *src_phi_alu = phi_instr_as_alu(src_phi);
            if (src_phi_alu) {
               src = &src_phi_alu->def;
            }
         }

         if (nir_def_is_alu(src) && !var.update_src) {
            var.def = src;
            nir_alu_instr *alu = nir_def_as_alu(src);

            /* Check for unsupported alu operations */
            if (alu->op != nir_op_iadd && alu->op != nir_op_fadd &&
                alu->op != nir_op_imul && alu->op != nir_op_fmul &&
                alu->op != nir_op_ishl && alu->op != nir_op_ishr &&
                alu->op != nir_op_ushr)
               break;

            if (nir_op_infos[alu->op].num_inputs == 2) {
               for (unsigned i = 0; i < 2; i++) {
                  /* Is one of the operands const or uniform, and the other the phi.
                   * The phi source can't be swizzled in any way.
                   */
                  if (alu->src[1 - i].src.ssa == &phi->def &&
                      alu_src_has_identity_swizzle(alu, 1 - i)) {
                     if (is_only_uniform_src(&alu->src[i].src))
                        var.update_src = alu->src + i;
                  }
               }
            }

            if (!var.update_src)
               break;
         } else {
            var.update_src = NULL;
            break;
         }
      }

      if (var.update_src && var.init_src) {
         /* Insert induction variable into hash table. */
         struct hash_table *vars = state->loop->info->induction_vars;
         nir_loop_induction_variable *induction_var = ralloc(vars, nir_loop_induction_variable);
         *induction_var = var;
         _mesa_hash_table_insert(vars, induction_var->def, induction_var);
         _mesa_hash_table_insert(vars, induction_var->basis, induction_var);
         progress = true;
      }
   }

   return progress;
}

static bool
find_loop_terminators(loop_info_state *state)
{
   bool success = false;
   foreach_list_typed_safe(nir_cf_node, node, node, &state->loop->body) {
      if (node->type == nir_cf_node_if) {
         nir_if *nif = nir_cf_node_as_if(node);

         nir_block *break_blk = NULL;
         nir_block *continue_from_blk = NULL;
         bool continue_from_then = true;

         nir_block *last_then = nir_if_last_then_block(nif);
         nir_block *last_else = nir_if_last_else_block(nif);
         if (nir_block_ends_in_break(last_then)) {
            break_blk = last_then;
            continue_from_blk = last_else;
            continue_from_then = false;
         } else if (nir_block_ends_in_break(last_else)) {
            break_blk = last_else;
            continue_from_blk = last_then;
         }

         /* If there is a break then we should find a terminator. If we can
          * not find a loop terminator, but there is a break-statement then
          * we should return false so that we do not try to find trip-count
          */
         if (!nir_is_trivial_loop_if(nif, break_blk)) {
            state->loop->info->complex_loop = true;
            return false;
         }

         /* Continue if the if contained no jumps at all */
         if (!break_blk)
            continue;

         if (nir_src_is_phi(nif->condition)) {
            state->loop->info->complex_loop = true;
            return false;
         }

         nir_loop_terminator *terminator =
            rzalloc(state->loop->info, nir_loop_terminator);

         list_addtail(&terminator->loop_terminator_link,
                      &state->loop->info->loop_terminator_list);

         terminator->nif = nif;
         terminator->break_block = break_blk;
         terminator->continue_from_block = continue_from_blk;
         terminator->continue_from_then = continue_from_then;
         terminator->conditional_instr = nir_def_instr(nif->condition.ssa);

         success = true;
      }
   }

   return success;
}

/* This function looks for an array access within a loop that uses an
 * induction variable for the array index. If found it returns the size of the
 * array, otherwise 0 is returned. If we find an induction var we pass it back
 * to the caller via array_index_out.
 */
static unsigned
find_array_access_via_induction(loop_info_state *state,
                                nir_deref_instr *deref,
                                nir_loop_induction_variable **array_index_out)
{
   for (nir_deref_instr *d = deref; d; d = nir_deref_instr_parent(d)) {
      if (d->deref_type != nir_deref_type_array)
         continue;

      nir_loop_induction_variable *array_index = get_loop_var(d->arr.index.ssa, state);

      if (!array_index)
         continue;

      if (array_index_out)
         *array_index_out = array_index;

      nir_deref_instr *parent = nir_deref_instr_parent(d);

      if (glsl_type_is_array_or_matrix(parent->type)) {
         return glsl_get_length(parent->type);
      } else {
         assert(glsl_type_is_vector(parent->type));
         return glsl_get_vector_elements(parent->type);
      }
   }

   return 0;
}

static void
guess_loop_limit_by_intrinsic(loop_info_state *state, nir_intrinsic_instr *intrin,
                              nir_scalar *ind, unsigned *min_loop_count)
{
   /* Check for arrays variably-indexed by a loop induction variable. */
   if (intrin->intrinsic == nir_intrinsic_load_deref ||
       intrin->intrinsic == nir_intrinsic_store_deref ||
       intrin->intrinsic == nir_intrinsic_copy_deref) {

      nir_loop_induction_variable *array_idx = NULL;
      unsigned array_size =
         find_array_access_via_induction(state,
                                         nir_src_as_deref(intrin->src[0]),
                                         &array_idx);
      if (array_idx) {
         if (*min_loop_count > array_size) {
            nir_scalar lp_ind = { array_idx->basis, 0 };
            *ind = lp_ind;
            *min_loop_count = array_size;
         }
      }

      if (intrin->intrinsic == nir_intrinsic_copy_deref) {
         array_size =
            find_array_access_via_induction(state,
                                            nir_src_as_deref(intrin->src[1]),
                                            &array_idx);
         if (array_idx) {
            if (*min_loop_count > array_size) {
               nir_scalar lp_ind = { array_idx->basis, 0 };
               *ind = lp_ind;
               *min_loop_count = array_size;
            }
         }
      }
   }
}

static void
guess_loop_limit_by_tex(loop_info_state *state, nir_tex_instr *tex,
                        nir_scalar *ind, unsigned *min_loop_count)
{
   nir_def *sample = nir_get_tex_src(tex, nir_tex_src_ms_index);
   if (sample) {
      nir_loop_induction_variable *var = get_loop_var(sample, state);
      if (var) {
         nir_scalar lp_ind = { var->basis, 0 };
         *ind = lp_ind;
         const nir_function_impl *impl = nir_cf_node_get_function(&state->loop->cf_node);
         const nir_shader_compiler_options *options = impl->function->shader->options;
         *min_loop_count = MIN2(*min_loop_count, options->max_samples);
      }
   }
}

static unsigned
guess_loop_limit(loop_info_state *state, nir_scalar *ind)
{
   unsigned min_loop_count = UINT_MAX;

   nir_foreach_block_in_cf_node(block, &state->loop->cf_node) {
      nir_foreach_instr(instr, block) {
         switch (instr->type) {
         case nir_instr_type_intrinsic:
            guess_loop_limit_by_intrinsic(state, nir_instr_as_intrinsic(instr),
                                          ind, &min_loop_count);
            break;
         case nir_instr_type_tex:
            guess_loop_limit_by_tex(state, nir_instr_as_tex(instr), ind,
                                    &min_loop_count);
            break;
         default:
            break;
         }
      }
   }

   if (min_loop_count != UINT_MAX)
      return min_loop_count;
   else
      return 0;
}

static nir_op invert_comparison_if_needed(nir_op alu_op, bool invert);

/* Returns whether "limit_op(a, b) alu_op c" is equivalent to "(a alu_op c) || (b alu_op c)". */
static bool
is_minmax_compatible(nir_op limit_op, nir_op alu_op, bool limit_rhs, bool invert_cond)
{
   bool is_max;
   switch (limit_op) {
   case nir_op_imin:
   case nir_op_fmin:
   case nir_op_umin:
      is_max = false;
      break;
   case nir_op_imax:
   case nir_op_fmax:
   case nir_op_umax:
      is_max = true;
      break;
   default:
      return false;
   }

   if (nir_op_infos[limit_op].input_types[0] != nir_op_infos[alu_op].input_types[0])
      return false;

   /* Comparisons we can split are:
    * - min(a, b) < c
    * - c < max(a, b)
    * - max(a, b) >= c
    * - c >= min(a, b)
    */
   switch (alu_op) {
   case nir_op_ilt:
   case nir_op_flt:
   case nir_op_ult:
      return (!limit_rhs && !is_max) || (limit_rhs && is_max);
   case nir_op_ige:
   case nir_op_fge:
   case nir_op_uge:
      return (!limit_rhs && is_max) || (limit_rhs && !is_max);
   default:
      return false;
   }
}

static bool
try_find_limit(nir_scalar limit, nir_const_value *limit_val, nir_op alu_op,
               bool invert_cond, nir_loop_terminator *terminator,
               loop_info_state *state)
{

   alu_op = invert_comparison_if_needed(alu_op, invert_cond);

   if (nir_scalar_is_alu(limit)) {
      nir_op limit_op = nir_scalar_alu_op(limit);
      if (is_minmax_compatible(limit_op, alu_op, !terminator->induction_rhs, invert_cond)) {
         for (unsigned i = 0; i < 2; i++) {
            nir_scalar src = nir_scalar_chase_alu_src(limit, i);
            if (nir_scalar_is_const(src)) {
               *limit_val = nir_scalar_as_const_value(src);
               terminator->exact_trip_count_unknown = true;
               return true;
            }
         }
      }
   } else {
      /* Use NIR range analysis to find a limit of the trip count -- this covers
       * cases that minmax above doesn't, but doesn't cover its >= cases.
       */
      if ((alu_op == nir_op_ilt || alu_op == nir_op_ult) && limit.def->bit_size <= 32) {
         nir_function_impl *impl = nir_cf_node_get_function(&state->loop->cf_node);
         unsigned uub = nir_unsigned_upper_bound(impl->function->shader,
                                                 state->range_ht, limit);

         /* If the unsigned upper bound for our signed comparison is negative as
          * an integer, then we don't know the non-negative upper bound.
          */
         if (alu_op == nir_op_ilt && util_sign_extend(uub, limit.def->bit_size) < 0)
            return false;

         if (uub != ~0) {
            *limit_val = nir_const_value_for_uint(uub, limit.def->bit_size);
            terminator->exact_trip_count_unknown = true;
            return true;
         }
      }
   }

   return false;
}

static nir_const_value
eval_const_unop(nir_op op, unsigned bit_size, nir_const_value src0,
                unsigned execution_mode)
{
   assert(nir_op_infos[op].num_inputs == 1);
   nir_const_value dest;
   nir_const_value *src[1] = { &src0 };
   nir_eval_const_opcode(op, &dest, NULL, 1, bit_size, src, execution_mode);
   return dest;
}

static nir_const_value
eval_const_binop(nir_op op, unsigned bit_size,
                 nir_const_value src0, nir_const_value src1,
                 unsigned execution_mode)
{
   assert(nir_op_infos[op].num_inputs == 2);
   nir_const_value dest;
   nir_const_value *src[2] = { &src0, &src1 };
   nir_eval_const_opcode(op, &dest, NULL, 1, bit_size, src, execution_mode);
   return dest;
}

static int
find_replacement(const nir_scalar *originals, nir_scalar key,
                 unsigned num_replacements)
{
   for (int i = 0; i < num_replacements; i++) {
      if (nir_scalar_equal(originals[i], key))
         return i;
   }

   return -1;
}

/**
 * Try to evaluate an ALU instruction as a constant with a replacement
 *
 * Much like \c nir_opt_constant_folding.c:try_fold_alu, this method attempts
 * to evaluate an ALU instruction as a constant. There are two significant
 * differences.
 *
 * First, this method performs the evaluation recursively. If any source of
 * the ALU instruction is not itself a constant, it is first evaluated.
 *
 * Second, if the SSA value \c original is encountered as a source of the ALU
 * instruction, the value \c replacement is substituted.
 *
 * The intended purpose of this function is to evaluate an arbitrary
 * expression involving a loop induction variable. In this case, \c original
 * would be the phi node associated with the induction variable, and
 * \c replacement is the initial value of the induction variable.
 *
 * \returns true if the ALU instruction can be evaluated as constant (after
 * applying the previously described substitution) or false otherwise.
 */
static bool
try_eval_const_alu(nir_const_value *dest, nir_scalar alu_s, const nir_scalar *originals,
                   const nir_const_value *replacements,
                   unsigned num_replacements, unsigned execution_mode)
{
   nir_alu_instr *alu = nir_def_as_alu(alu_s.def);

   if (nir_op_infos[alu->op].output_size)
      return false;

   /* In the case that any outputs/inputs have unsized types, then we need to
    * guess the bit-size. In this case, the validator ensures that all
    * bit-sizes match so we can just take the bit-size from first
    * output/input with an unsized type. If all the outputs/inputs are sized
    * then we don't need to guess the bit-size at all because the code we
    * generate for constant opcodes in this case already knows the sizes of
    * the types involved and does not need the provided bit-size for anything
    * (although it still requires to receive a valid bit-size).
    */
   unsigned bit_size = 0;
   if (!nir_alu_type_get_type_size(nir_op_infos[alu->op].output_type)) {
      bit_size = alu->def.bit_size;
   } else {
      for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
         if (!nir_alu_type_get_type_size(nir_op_infos[alu->op].input_types[i]))
            bit_size = alu->src[i].src.ssa->bit_size;
      }

      if (bit_size == 0)
         bit_size = 32;
   }

   nir_const_value src[NIR_MAX_VEC_COMPONENTS];
   nir_const_value *src_ptrs[NIR_MAX_VEC_COMPONENTS];

   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      nir_scalar src_s = nir_scalar_chase_alu_src(alu_s, i);

      src_ptrs[i] = &src[i];
      if (nir_scalar_is_const(src_s)) {
         src[i] = nir_scalar_as_const_value(src_s);
         continue;
      }

      int r = find_replacement(originals, src_s, num_replacements);
      if (r >= 0) {
         src[i] = replacements[r];
      } else if (!nir_scalar_is_alu(src_s) ||
                 !try_eval_const_alu(&src[i], src_s,
                                     originals, replacements,
                                     num_replacements, execution_mode)) {
         return false;
      }
   }

   nir_eval_const_opcode(alu->op, dest, NULL, 1, bit_size, src_ptrs, execution_mode);

   return true;
}

static nir_op
invert_comparison_if_needed(nir_op alu_op, bool invert)
{
   if (!invert)
      return alu_op;

   switch (alu_op) {
   case nir_op_fge:
      return nir_op_flt;
   case nir_op_ige:
      return nir_op_ilt;
   case nir_op_uge:
      return nir_op_ult;
   case nir_op_flt:
      return nir_op_fge;
   case nir_op_ilt:
      return nir_op_ige;
   case nir_op_ult:
      return nir_op_uge;
   case nir_op_feq:
      return nir_op_fneu;
   case nir_op_ieq:
      return nir_op_ine;
   case nir_op_fneu:
      return nir_op_feq;
   case nir_op_ine:
      return nir_op_ieq;
   default:
      UNREACHABLE("Unsuported comparison!");
   }
}

static int32_t
get_iteration(nir_op cond_op, nir_const_value initial, nir_const_value step,
              nir_const_value limit, bool invert_cond, unsigned bit_size,
              unsigned execution_mode)
{
   nir_const_value span, iter;
   unsigned iter_bit_size = bit_size;

   switch (invert_comparison_if_needed(cond_op, invert_cond)) {
   case nir_op_ine:
      /* In order for execution to be here, limit must be the same as initial.
       * Otherwise will_break_on_first_iteration would have returned false.
       * If step is zero, the loop is infinite.  Otherwise the loop will
       * execute once.
       */
      return step.u64 == 0 ? -1 : 1;

   case nir_op_ige:
   case nir_op_ilt:
   case nir_op_ieq:
      span = eval_const_binop(nir_op_isub, bit_size, limit, initial,
                              execution_mode);
      iter = eval_const_binop(nir_op_idiv, bit_size, span, step,
                              execution_mode);
      break;

   case nir_op_uge:
   case nir_op_ult:
      span = eval_const_binop(nir_op_isub, bit_size, limit, initial,
                              execution_mode);
      iter = eval_const_binop(nir_op_udiv, bit_size, span, step,
                              execution_mode);
      break;

   case nir_op_fneu:
      /* In order for execution to be here, limit must be the same as initial.
       * Otherwise will_break_on_first_iteration would have returned false.
       * If step is zero, the loop is infinite.  Otherwise the loop will
       * execute once.
       *
       * This is a little more tricky for floating point since X-Y might still
       * be X even if Y is not zero.  Instead check that (initial + step) !=
       * initial.
       */
      span = eval_const_binop(nir_op_fadd, bit_size, initial, step,
                              execution_mode);
      iter = eval_const_binop(nir_op_feq, bit_size, initial,
                              span, execution_mode);

      /* return (initial + step) == initial ? -1 : 1 */
      return iter.b ? -1 : 1;

   case nir_op_fge:
   case nir_op_flt:
   case nir_op_feq:
      span = eval_const_binop(nir_op_fsub, bit_size, limit, initial,
                              execution_mode);
      iter = eval_const_binop(nir_op_fdiv, bit_size, span,
                              step, execution_mode);
      iter = eval_const_unop(nir_op_f2i64, bit_size, iter, execution_mode);
      iter_bit_size = 64;
      break;

   default:
      return -1;
   }

   uint64_t iter_u64 = nir_const_value_as_uint(iter, iter_bit_size);
   return iter_u64 > u_intN_max(iter_bit_size) ? -1 : (int)iter_u64;
}

static int32_t
get_iteration_empirical(nir_scalar cond, nir_alu_instr *incr_alu,
                        nir_scalar basis, nir_const_value initial,
                        nir_scalar limit_basis, nir_const_value limit,
                        bool invert_cond, unsigned execution_mode,
                        unsigned max_unroll_iterations)
{
   int iter_count = 0;
   nir_const_value result;

   const nir_scalar incr = nir_get_scalar(&incr_alu->def, basis.comp);

   const nir_scalar original[] = { basis, limit_basis };
   nir_const_value replacement[] = { initial, limit };

   while (iter_count <= max_unroll_iterations) {
      bool success;

      success = try_eval_const_alu(&result, cond, original, replacement,
                                   2, execution_mode);
      if (!success)
         return -1;

      const bool cond_succ = invert_cond ? !result.b : result.b;
      if (cond_succ)
         return iter_count;

      iter_count++;

      success = try_eval_const_alu(&result, incr, original, replacement,
                                   2, execution_mode);
      assert(success);

      replacement[0] = result;
   }

   return -1;
}

static bool
will_break_on_first_iteration(nir_scalar cond, nir_scalar basis,
                              nir_scalar limit_basis,
                              nir_const_value initial, nir_const_value limit,
                              bool invert_cond, unsigned execution_mode)
{
   nir_const_value result;

   const nir_scalar originals[2] = { basis, limit_basis };
   const nir_const_value replacements[2] = { initial, limit };

   ASSERTED bool success = try_eval_const_alu(&result, cond, originals,
                                              replacements, 2, execution_mode);

   assert(success);

   return invert_cond ? !result.b : result.b;
}

static bool
test_iterations(int32_t iter_int, nir_const_value step,
                nir_const_value limit, nir_op cond_op, unsigned bit_size,
                nir_alu_type induction_base_type,
                nir_const_value initial, bool limit_rhs, bool invert_cond,
                unsigned execution_mode)
{
   assert(nir_op_infos[cond_op].num_inputs == 2);

   nir_const_value iter_src;
   nir_op mul_op;
   nir_op add_op;
   switch (induction_base_type) {
   case nir_type_float:
      iter_src = nir_const_value_for_float(iter_int, bit_size);
      mul_op = nir_op_fmul;
      add_op = nir_op_fadd;
      break;
   case nir_type_int:
   case nir_type_uint:
      iter_src = nir_const_value_for_int(iter_int, bit_size);
      mul_op = nir_op_imul;
      add_op = nir_op_iadd;
      break;
   default:
      UNREACHABLE("Unhandled induction variable base type!");
   }

   /* Multiple the iteration count we are testing by the number of times we
    * step the induction variable each iteration.
    */
   nir_const_value mul_result =
      eval_const_binop(mul_op, bit_size, iter_src, step, execution_mode);

   /* Add the initial value to the accumulated induction variable total */
   nir_const_value add_result =
      eval_const_binop(add_op, bit_size, mul_result, initial, execution_mode);

   nir_const_value *src[2];
   src[limit_rhs ? 0 : 1] = &add_result;
   src[limit_rhs ? 1 : 0] = &limit;

   /* Evaluate the loop exit condition */
   nir_const_value result;
   nir_eval_const_opcode(cond_op, &result, NULL, 1, bit_size, src, execution_mode);

   return invert_cond ? !result.b : result.b;
}

static int
calculate_iterations(nir_scalar basis, nir_scalar limit_basis,
                     nir_const_value initial, nir_const_value step,
                     nir_const_value limit, nir_alu_instr *alu,
                     nir_scalar cond, nir_op alu_op, bool limit_rhs,
                     bool invert_cond, unsigned execution_mode,
                     unsigned max_unroll_iterations)
{
   /* nir_op_isub should have been lowered away by this point */
   assert(alu->op != nir_op_isub);

   /* Make sure the alu type for our induction variable is compatible with the
    * conditional alus input type. If its not something has gone really wrong.
    */
   nir_alu_type induction_base_type =
      nir_alu_type_get_base_type(nir_op_infos[alu->op].output_type);
   if (induction_base_type == nir_type_int || induction_base_type == nir_type_uint) {
      assert(nir_alu_type_get_base_type(nir_op_infos[alu_op].input_types[1]) == nir_type_int ||
             nir_alu_type_get_base_type(nir_op_infos[alu_op].input_types[1]) == nir_type_uint);
   } else {
      assert(nir_alu_type_get_base_type(nir_op_infos[alu_op].input_types[0]) ==
             induction_base_type);
   }

   /* do-while loops can increment the starting value before the condition is
    * checked. e.g.
    *
    *    do {
    *        ndx++;
    *     } while (ndx < 3);
    *
    * Here we check if the induction variable is used directly by the loop
    * condition and if so we assume we need to step the initial value.
    */
   unsigned trip_offset = 0;
   nir_alu_instr *cond_alu = nir_def_as_alu(cond.def);
   if (cond_alu->src[0].src.ssa == &alu->def ||
       cond_alu->src[1].src.ssa == &alu->def) {
      trip_offset = 1;
   }

   unsigned bit_size = nir_src_bit_size(alu->src[0].src);

   /* get_iteration works under assumption that iterator will be
    * incremented or decremented until it hits the limit,
    * however if the loop condition is false on the first iteration
    * get_iteration's assumption is broken. Handle such loops first.
    */
   if (will_break_on_first_iteration(cond, basis, limit_basis, initial,
                                     limit, invert_cond, execution_mode)) {
      return 0;
   }

   /* For loops incremented with addition operation, it's easy to
    * calculate the number of iterations theoretically. Even though it
    * is possible for other operations as well, it is much more error
    * prone, and doesn't cover all possible cases. So, we try to
    * emulate the loop.
    */
   int iter_int;
   switch (alu->op) {
   case nir_op_iadd:
   case nir_op_fadd:
      assert(nir_src_bit_size(alu->src[0].src) ==
             nir_src_bit_size(alu->src[1].src));

      iter_int = get_iteration(alu_op, initial, step, limit, invert_cond,
                               bit_size, execution_mode);
      break;
   case nir_op_fmul:
      /* Detecting non-zero loop counts when the loop increment is floating
       * point multiplication triggers a preexisting problem in
       * glsl-fs-loop-unroll-mul-fp64.shader_test. See
       * https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/3445#note_1779438.
       */
      return -1;
   case nir_op_imul:
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr:
      return get_iteration_empirical(cond, alu, basis, initial,
                                     limit_basis, limit, invert_cond,
                                     execution_mode, max_unroll_iterations);
   default:
      UNREACHABLE("Invalid induction variable increment operation.");
   }

   /* If iter_int is negative the loop is ill-formed or is the conditional is
    * unsigned with a huge iteration count so don't bother going any further.
    */
   if (iter_int < 0)
      return -1;

   nir_op actual_alu_op = invert_comparison_if_needed(alu_op, invert_cond);
   if (actual_alu_op == nir_op_ine || actual_alu_op == nir_op_fneu)
      return iter_int;

   /* An explanation from the GLSL unrolling pass:
    *
    * Make sure that the calculated number of iterations satisfies the exit
    * condition.  This is needed to catch off-by-one errors and some types of
    * ill-formed loops.  For example, we need to detect that the following
    * loop does not have a maximum iteration count.
    *
    *    for (float x = 0.0; x != 0.9; x += 0.2);
    */
   for (int bias = -1; bias <= 1; bias++) {
      const int iter_bias = iter_int + bias;
      if (iter_bias < 1)
         continue;

      if (test_iterations(iter_bias, step, limit, alu_op, bit_size,
                          induction_base_type, initial,
                          limit_rhs, invert_cond, execution_mode)) {
         return iter_bias - trip_offset;
      }
   }

   return -1;
}

static bool
get_induction_and_limit_vars(nir_scalar cond,
                             nir_scalar *ind,
                             nir_scalar *limit,
                             bool *limit_rhs,
                             loop_info_state *state)
{
   nir_scalar rhs, lhs;
   lhs = nir_scalar_chase_alu_src(cond, 0);
   rhs = nir_scalar_chase_alu_src(cond, 1);

   nir_loop_induction_variable *src0_lv = get_loop_var(lhs.def, state);
   nir_loop_induction_variable *src1_lv = get_loop_var(rhs.def, state);

   if (src0_lv) {
      *ind = lhs;
      *limit = rhs;
      *limit_rhs = true;
      return true;
   } else if (src1_lv) {
      *ind = rhs;
      *limit = lhs;
      *limit_rhs = false;
      return true;
   } else {
      return false;
   }
}

static bool
try_find_trip_count_vars_in_logical_op(nir_scalar *cond,
                                       nir_scalar *ind,
                                       nir_scalar *limit,
                                       bool *limit_rhs,
                                       loop_info_state *state)
{
   const nir_op alu_op = nir_scalar_alu_op(*cond);
   bool exit_loop_on_false = alu_op == nir_op_ieq || alu_op == nir_op_inot;
   nir_scalar logical_op = exit_loop_on_false ? nir_scalar_chase_alu_src(*cond, 0) : *cond;

   if (alu_op == nir_op_ieq) {
      nir_scalar zero = nir_scalar_chase_alu_src(*cond, 1);

      if (!nir_scalar_is_alu(logical_op) || !nir_scalar_is_const(zero)) {
         /* Maybe we had it the wrong way, flip things around */
         SWAP(zero, logical_op);

         /* If we still didn't find what we need then return */
         if (!nir_scalar_is_const(zero))
            return false;
      }

      /* If the loop is not breaking on (x && y) == 0 then return */
      if (nir_scalar_as_uint(zero) != 0)
         return false;
   }

   if (!nir_scalar_is_alu(logical_op))
      return false;

   if ((exit_loop_on_false && (nir_scalar_alu_op(logical_op) != nir_op_iand)) ||
       (!exit_loop_on_false && (nir_scalar_alu_op(logical_op) != nir_op_ior)))
      return false;

   /* Check if iand src is a terminator condition and try get induction var
    * and trip limit var.
    */
   bool found_induction_var = false;
   for (unsigned i = 0; i < 2; i++) {
      nir_scalar src = nir_scalar_chase_alu_src(logical_op, i);
      if (nir_is_terminator_condition_with_two_inputs(src) &&
          get_induction_and_limit_vars(src, ind, limit, limit_rhs, state)) {
         *cond = src;
         found_induction_var = true;

         /* If we've found one with a constant limit, stop. */
         if (nir_scalar_is_const(*limit))
            return true;
      }
   }

   return found_induction_var;
}

/* Run through each of the terminators of the loop and try to infer a possible
 * trip-count. We need to check them all, and set the lowest trip-count as the
 * trip-count of our loop. If one of the terminators has an undecidable
 * trip-count we can not safely assume anything about the duration of the
 * loop.
 */
static void
find_trip_count(loop_info_state *state, unsigned execution_mode,
                unsigned max_unroll_iterations)
{
   bool trip_count_known = true;
   bool guessed_trip_count = false;
   nir_loop_terminator *limiting_terminator = NULL;
   int max_trip_count = -1;

   list_for_each_entry(nir_loop_terminator, terminator,
                       &state->loop->info->loop_terminator_list,
                       loop_terminator_link) {
      nir_scalar cond = { terminator->nif->condition.ssa, 0 };

      if (!nir_scalar_is_alu(cond)) {
         /* If we get here the loop is dead and will get cleaned up by the
          * nir_opt_dead_cf pass.
          */
         trip_count_known = false;
         terminator->exact_trip_count_unknown = true;
         continue;
      }

      nir_op alu_op = nir_scalar_alu_op(cond);

      bool invert_cond = terminator->continue_from_then;

      bool limit_rhs;
      nir_scalar basic_ind = { NULL, 0 };
      nir_scalar terminator_ind = { NULL, 0 };
      nir_scalar limit;

      if ((alu_op == nir_op_inot || alu_op == nir_op_ieq || alu_op == nir_op_ior) &&
          try_find_trip_count_vars_in_logical_op(&cond, &basic_ind, &limit,
                                                 &limit_rhs, state)) {

         /* The loop is exiting on (x && y) == 0 so we need to get the
          * inverse of x or y (i.e. which ever contained the induction var) in
          * order to compute the trip count.
          */
         if (alu_op == nir_op_inot || alu_op == nir_op_ieq)
            invert_cond = !invert_cond;

         alu_op = nir_scalar_alu_op(cond);
         trip_count_known = false;
         terminator->conditional_instr = nir_def_instr(cond.def);
         terminator->exact_trip_count_unknown = true;
      }

      if (!basic_ind.def) {
         if (nir_is_supported_terminator_condition(cond)) {
            /* Extract and inverse the comparision if it is wrapped in an inot
             */
            if (alu_op == nir_op_inot) {
               cond = nir_scalar_chase_alu_src(cond, 0);
               alu_op = nir_scalar_alu_op(cond);
               invert_cond = !invert_cond;
            }

            get_induction_and_limit_vars(cond, &basic_ind,
                                         &limit, &limit_rhs, state);
         }
      }

      /* The comparison has to have a basic induction variable for us to be
       * able to find trip counts.
       */
      if (!basic_ind.def) {
         trip_count_known = false;
         terminator->exact_trip_count_unknown = true;
         continue;
      }

      terminator->induction_rhs = !limit_rhs;

      /* Attempt to find a constant limit for the loop */
      nir_const_value limit_val;
      if (nir_scalar_is_const(limit)) {
         limit_val = nir_scalar_as_const_value(limit);
      } else {
         trip_count_known = false;

         if (!try_find_limit(limit, &limit_val, alu_op, invert_cond, terminator, state)) {
            /* Guess loop limit based on array access, and get the induction
             * var from the array access which is not necessarily the same
             * induction var used by the loop terminator.
             */
            terminator_ind = basic_ind;
            unsigned guessed_loop_limit = guess_loop_limit(state, &basic_ind);
            if (guessed_loop_limit) {
               limit_val = nir_const_value_for_uint(guessed_loop_limit,
                                                    basic_ind.def->bit_size);
            } else {
               terminator->exact_trip_count_unknown = true;
               continue;
            }

            guessed_trip_count = true;
         }
      }

      /* We have determined that we have the following constants:
       * (With the typical int i = 0; i < x; i++; as an example)
       *    - Upper limit.
       *    - Starting value
       *    - Step / iteration size
       * Thats all thats needed to calculate the trip-count
       */

      nir_loop_induction_variable *lv = get_loop_var(basic_ind.def, state);
      terminator->init_src = lv->init_src;

      /* The basic induction var might be a vector but, because we guarantee
       * earlier that the phi source has a scalar swizzle, we can take the
       * component from basic_ind.
       */
      nir_scalar initial_s = { lv->init_src->ssa, basic_ind.comp };
      nir_scalar alu_s = {
         lv->update_src->src.ssa,
         lv->update_src->swizzle[basic_ind.comp]
      };

      nir_alu_instr *step_alu =
         nir_instr_as_alu(nir_src_use_instr(&lv->update_src->src));

      /* If the comparision is of unsigned type we don't necessarily need to
       * know the initial value to be able to calculate the max number of
       * iterations
       */
      bool can_find_max_trip_count = step_alu->op == nir_op_iadd &&
                                     ((alu_op == nir_op_uge && !invert_cond && limit_rhs) ||
                                      (alu_op == nir_op_ult && !invert_cond && !limit_rhs));

      /* nir_op_isub should have been lowered away by this point */
      assert(step_alu->op != nir_op_isub);

      /* For nir_op_uge as alu_op, the induction variable is [0,limit). For
       * nir_op_ult, it's [0,limit]. It must always be step_val larger in the
       * next iteration to use the can_find_max_trip_count=true path. This
       * check ensures that no unsigned overflow happens.
       * TODO: support for overflow could be added if a non-zero initial_val
       * is chosen.
       */
      if (can_find_max_trip_count && nir_scalar_is_const(alu_s)) {
         uint64_t uint_max = u_uintN_max(alu_s.def->bit_size);
         uint64_t max_step_val =
            uint_max - nir_const_value_as_uint(limit_val, alu_s.def->bit_size) +
            (alu_op == nir_op_uge ? 1 : 0);
         can_find_max_trip_count &= nir_scalar_as_uint(alu_s) <= max_step_val;
      }

      /* We are not guaranteed by that at one of these sources is a constant.
       * Try to find one.
       */
      if ((!nir_scalar_is_const(initial_s) && !can_find_max_trip_count) ||
          !nir_scalar_is_const(alu_s)) {
         trip_count_known = false;
         terminator->exact_trip_count_unknown = true;
         continue;
      }

      nir_const_value initial_val;
      if (nir_scalar_is_const(initial_s))
         initial_val = nir_scalar_as_const_value(initial_s);
      else {
         trip_count_known = false;
         terminator->exact_trip_count_unknown = true;
         initial_val = nir_const_value_for_uint(0, 32);
         assert(can_find_max_trip_count);
      }
      nir_const_value step_val = nir_scalar_as_const_value(alu_s);

      int iterations;
      if (guessed_trip_count && terminator_ind.def != basic_ind.def) {
         /* TODO: expand to support additional step combinations where needed */
         if (step_alu->op != nir_op_iadd ||
             (nir_const_value_as_int(step_val, basic_ind.def->bit_size) <= 0) ||
             (nir_const_value_as_int(initial_val, basic_ind.def->bit_size) < 0)) {
            iterations = -1;
         } else {
            iterations = get_iteration(nir_op_ilt, initial_val, step_val,
                                       limit_val, false,
                                       basic_ind.def->bit_size, execution_mode);
         }
      } else {
         iterations = calculate_iterations(nir_get_scalar(lv->basis, basic_ind.comp), limit,
                                           initial_val, step_val, limit_val,
                                           step_alu, cond,
                                           alu_op, limit_rhs,
                                           invert_cond,
                                           execution_mode,
                                           max_unroll_iterations);
      }

      /* Where we not able to calculate the iteration count */
      if (iterations == -1) {
         trip_count_known = false;
         guessed_trip_count = false;
         terminator->exact_trip_count_unknown = true;
         continue;
      }

      if (guessed_trip_count) {
         guessed_trip_count = false;
         terminator->exact_trip_count_unknown = true;
         if (state->loop->info->guessed_trip_count == 0 ||
             state->loop->info->guessed_trip_count > iterations)
            state->loop->info->guessed_trip_count = iterations;

         continue;
      }

      /* If this is the first run or we have found a smaller amount of
       * iterations than previously (we have identified a more limiting
       * terminator) set the trip count and limiting terminator.
       */
      if (max_trip_count == -1 || iterations < max_trip_count) {
         max_trip_count = iterations;
         limiting_terminator = terminator;
      }
   }

   state->loop->info->exact_trip_count_known = trip_count_known;
   if (max_trip_count > -1)
      state->loop->info->max_trip_count = max_trip_count;
   state->loop->info->limiting_terminator = limiting_terminator;
}

static bool
force_unroll_array_access(loop_info_state *state, nir_deref_instr *deref,
                          bool contains_sampler)
{
   unsigned array_size = find_array_access_via_induction(state, deref, NULL);
   if (array_size) {
      if ((array_size >= state->loop->info->max_trip_count) &&
          nir_deref_mode_must_be(deref, nir_var_shader_in |
                                           nir_var_shader_out |
                                           nir_var_shader_temp |
                                           nir_var_function_temp))
         return true;

      if (nir_deref_mode_must_be(deref, state->indirect_mask))
         return true;

      if (contains_sampler && state->force_unroll_sampler_indirect)
         return true;
   }

   return false;
}

static bool
force_unroll_heuristics(loop_info_state *state, nir_block *block)
{
   nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_tex) {
         nir_tex_instr *tex_instr = nir_instr_as_tex(instr);
         int sampler_idx =
            nir_tex_instr_src_index(tex_instr,
                                    nir_tex_src_sampler_deref);

         if (sampler_idx >= 0) {
            nir_deref_instr *deref =
               nir_def_as_deref(tex_instr->src[sampler_idx].src.ssa);
            if (force_unroll_array_access(state, deref, true))
               return true;
         }
      }

      if (instr->type != nir_instr_type_intrinsic)
         continue;

      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);

      /* Check for arrays variably-indexed by a loop induction variable.
       * Unrolling the loop may convert that access into constant-indexing.
       */
      if (intrin->intrinsic == nir_intrinsic_load_deref ||
          intrin->intrinsic == nir_intrinsic_store_deref ||
          intrin->intrinsic == nir_intrinsic_copy_deref) {
         if (force_unroll_array_access(state,
                                       nir_src_as_deref(intrin->src[0]),
                                       false))
            return true;

         if (intrin->intrinsic == nir_intrinsic_copy_deref &&
             force_unroll_array_access(state,
                                       nir_src_as_deref(intrin->src[1]),
                                       false))
            return true;
      }
   }

   return false;
}

static void
gather_constant_fold_info(loop_info_state *state, nir_instr *instr)
{
   instr->pass_flags = 0;

   /* Loop induction variables with constant initializer and constant
    * update source get constant-folded when the loop is being unrolled.
    */
   if (instr->type == nir_instr_type_phi &&
       instr->block == nir_loop_first_block(state->loop)) {
      nir_loop_induction_variable *var = get_loop_var(nir_instr_def(instr), state);

      instr->pass_flags = var && nir_def_is_const(var->init_src->ssa) &&
                          nir_def_is_const(var->update_src->src.ssa);
   }

   if (instr->type != nir_instr_type_alu)
      return;

   /* ALU instruction which only depend on constants and constant-foldable
    * sources, can also be constant-folded.
    */
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if(!nir_src_is_const(alu->src[i].src) &&
         !is_const_after_unrolling(state, alu->src[i].src.ssa))
         return;
   }

   instr->pass_flags = 1;
}

static void
gather_unroll_heuristic_info(loop_info_state *state, const nir_shader_compiler_options *options)
{
   state->loop->info->flattens_all_control_flow = state->loop->info->exact_trip_count_known;

   nir_foreach_block_in_cf_node(block, &state->loop->cf_node) {
      /* Calculate instruction cost. */
      nir_foreach_instr(instr, block) {
         gather_constant_fold_info(state, instr);
         state->loop->info->instr_cost += instr_cost(state, instr, options);
      }

      nir_if *nif = nir_block_get_following_if(block);
      if (nif) {
         /* If all IF statements can be constant-folded after unrolling,
          * the loop becomes a single large basic block.
          */
         state->loop->info->flattens_all_control_flow &=
            is_const_after_unrolling(state, nif->condition.ssa);
      }

      if (state->loop->info->force_unroll)
         continue;

      if (force_unroll_heuristics(state, block)) {
         state->loop->info->force_unroll = true;
      }
   }
}

static void
get_loop_info(loop_info_state *state, nir_function_impl *impl)
{
   nir_shader *shader = impl->function->shader;
   const nir_shader_compiler_options *options = shader->options;

   /* Try to find all simple terminators of the loop. If we can't find any,
    * or we find possible terminators that have side effects then bail.
    */
   if (!find_loop_terminators(state)) {
      list_for_each_entry_safe(nir_loop_terminator, terminator,
                               &state->loop->info->loop_terminator_list,
                               loop_terminator_link) {
         list_del(&terminator->loop_terminator_link);
         ralloc_free(terminator);
      }
      return;
   }

   if (!compute_induction_information(state))
      return;

   /* Run through each of the terminators and try to compute a trip-count */
   find_trip_count(state,
                   impl->function->shader->info.float_controls_execution_mode,
                   impl->function->shader->options->max_unroll_iterations);

   gather_unroll_heuristic_info(state, options);
}

static void
initialize_loop_info(nir_loop *loop)
{
   ralloc_free(loop->info);

   loop->info = rzalloc(loop, nir_loop_info);
   loop->info->induction_vars = _mesa_pointer_hash_table_create(loop->info);

   list_inithead(&loop->info->loop_terminator_list);
}

static void
process_loops(nir_cf_node *cf_node, nir_variable_mode indirect_mask,
              bool force_unroll_sampler_indirect, struct hash_table *range_ht)
{
   switch (cf_node->type) {
   case nir_cf_node_block:
      return;
   case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(cf_node);
      foreach_list_typed(nir_cf_node, nested_node, node, &if_stmt->then_list)
         process_loops(nested_node, indirect_mask, force_unroll_sampler_indirect, range_ht);
      foreach_list_typed(nir_cf_node, nested_node, node, &if_stmt->else_list)
         process_loops(nested_node, indirect_mask, force_unroll_sampler_indirect, range_ht);
      return;
   }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
      assert(!nir_loop_has_continue_construct(loop));

      foreach_list_typed(nir_cf_node, nested_node, node, &loop->body)
         process_loops(nested_node, indirect_mask, force_unroll_sampler_indirect, range_ht);
      break;
   }
   default:
      UNREACHABLE("unknown cf node type");
   }

   nir_loop *loop = nir_cf_node_as_loop(cf_node);
   nir_function_impl *impl = nir_cf_node_get_function(cf_node);
   loop_info_state state = {
      .loop = loop,
      .indirect_mask = indirect_mask,
      .force_unroll_sampler_indirect = force_unroll_sampler_indirect,
      .range_ht = range_ht,
   };

   initialize_loop_info(loop);
   get_loop_info(&state, impl);
}

void
nir_loop_analyze_impl(nir_function_impl *impl,
                      nir_variable_mode indirect_mask,
                      bool force_unroll_sampler_indirect)
{
   struct hash_table *range_ht = _mesa_pointer_hash_table_create(NULL);
   nir_metadata_require(impl, nir_metadata_block_index);

   /* Clear pass_flags before using it for memoization */
   nir_shader *shader = impl->function->shader;
   nir_shader_clear_pass_flags(shader);

   foreach_list_typed(nir_cf_node, node, node, &impl->body)
      process_loops(node, indirect_mask, force_unroll_sampler_indirect, range_ht);

   impl->loop_analysis_indirect_mask = indirect_mask;
   impl->loop_analysis_force_unroll_sampler_indirect = force_unroll_sampler_indirect;

   ralloc_free(range_ht);
}
