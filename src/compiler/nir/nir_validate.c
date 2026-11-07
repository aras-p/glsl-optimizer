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
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include <assert.h>
#include "c11/threads.h"
#include "util/hash_table.h"
#include "util/simple_mtx.h"
#include "nir.h"
#include "nir_xfb_info.h"

/*
 * This file checks for invalid IR indicating a bug somewhere in the compiler.
 */

/* Since this file is just a pile of asserts, don't bother compiling it if
 * we're not building a debug build.
 */
#ifndef NDEBUG

typedef struct {
   void *mem_ctx;

   /* the current shader being validated */
   nir_shader *shader;

   /* the current instruction being validated */
   nir_instr *instr;

   /* the current variable being validated */
   nir_variable *var;

   /* the current basic block being validated */
   nir_block *block;

   /* the current if statement being validated */
   nir_if *if_stmt;

   /* the current loop being visited */
   nir_loop *loop;

   /* weather the loop continue construct is being visited */
   bool in_loop_continue_construct;

   /* the parent of the current cf node being visited */
   nir_cf_node *parent_node;

   /* the current function implementation being validated */
   nir_function_impl *impl;

   /* Set of all blocks in the list */
   struct set *blocks;

   /* Number of tagged nir_src's. This is implicitly the cardinality of the set
    * of pending nir_src's.
    */
   uint32_t nr_tagged_srcs;

   /* bitset of ssa definitions we have found; used to check uniqueness */
   BITSET_WORD *ssa_defs_found;

   /* map of variable -> function implementation where it is defined or NULL
    * if it is a global variable
    */
   struct hash_table *var_defs;

   /* map of instruction/var/etc to failed assert string */
   struct hash_table *errors;
} validate_state;

static void
log_error(validate_state *state, const char *cond, const char *file, int line)
{
   const void *obj;

   if (state->instr)
      obj = state->instr;
   else if (state->var)
      obj = state->var;
   else
      obj = cond;

   char *msg = ralloc_asprintf(state->errors, "error: %s (%s:%d)",
                               cond, file, line);

   _mesa_hash_table_insert(state->errors, obj, msg);
}

static bool
validate_assert_impl(validate_state *state, bool cond, const char *str,
                     const char *file, unsigned line)
{
   if (unlikely(!cond))
      log_error(state, str, file, line);
   return cond;
}

#define validate_assert(state, cond) \
   validate_assert_impl(state, (cond), #cond, __FILE__, __LINE__)

static void
validate_num_components(validate_state *state, unsigned num_components)
{
   validate_assert(state, nir_num_components_valid(num_components));
}

/* Tag used in nir_src::_parent to indicate that a source has been seen. */
#define SRC_TAG_SEEN (0x2)

static_assert(SRC_TAG_SEEN == (~NIR_SRC_PARENT_MASK + 1),
              "Parent pointer tags chosen not to collide");

static void
tag_src(nir_src *src, validate_state *state)
{
   /* nir_src only appears once and only in one SSA def use list, since we
    * mark nir_src's as we go by tagging this pointer.
    */
   if (validate_assert(state, (src->_parent & SRC_TAG_SEEN) == 0)) {
      src->_parent |= SRC_TAG_SEEN;
      state->nr_tagged_srcs++;
   }
}

/* Due to tagging, it's not safe to use nir_src_use_instr during the main
 * validate loop. This is a tagging-aware version.
 */
static nir_instr *
src_parent_instr_safe(nir_src *src)
{
   uintptr_t untagged = (src->_parent & ~SRC_TAG_SEEN);
   assert(!(untagged & NIR_SRC_PARENT_IS_IF) && "precondition");
   return (nir_instr *)untagged;
}

/*
 * As we walk SSA defs, we mark every use as seen by tagging the parent pointer.
 * We need to make sure our use is seen in a use list.
 *
 * Then we unmark when we hit the source. This will let us prove that we've
 * seen all the sources.
 */
static void
validate_src_tag(nir_src *src, validate_state *state)
{
   if (validate_assert(state, src->_parent & SRC_TAG_SEEN)) {
      src->_parent &= ~SRC_TAG_SEEN;
      state->nr_tagged_srcs--;
   }
}

static void
validate_if_src(nir_src *src, validate_state *state)
{
   validate_src_tag(src, state);
   validate_assert(state, nir_src_use_if(src) == state->if_stmt);
   validate_assert(state, src->ssa != NULL);
   validate_assert(state, src->ssa->num_components == 1);
}

static void
validate_src(nir_src *src, validate_state *state)
{
   /* Validate the tag first, so that nir_src_use_instr is valid */
   validate_src_tag(src, state);

   /* Source assumed to be instruction, use validate_if_src for if */
   validate_assert(state, nir_src_use_instr(src) == state->instr);

   validate_assert(state, src->ssa != NULL);
}

static void
validate_sized_src(nir_src *src, validate_state *state,
                   unsigned bit_sizes, unsigned num_components)
{
   validate_src(src, state);

   if (bit_sizes)
      validate_assert(state, src->ssa->bit_size & bit_sizes);
   if (num_components)
      validate_assert(state, src->ssa->num_components == num_components);
}

static void
validate_alu_src(nir_alu_instr *instr, unsigned index, validate_state *state)
{
   nir_alu_src *src = &instr->src[index];

   unsigned num_instr_channels = nir_ssa_alu_instr_src_components(instr, index);
   unsigned num_components = nir_src_num_components(src->src);

   for (unsigned i = 0; i < num_instr_channels; i++) {
      validate_assert(state, src->swizzle[i] < num_components);
   }

   validate_src(&src->src, state);
}

static void
validate_def(nir_def *def, validate_state *state)
{
   validate_assert(state, def->index < state->impl->ssa_alloc);
   validate_assert(state, !BITSET_TEST(state->ssa_defs_found, def->index));
   BITSET_SET(state->ssa_defs_found, def->index);

   validate_num_components(state, def->num_components);

   list_validate(&def->uses);
   nir_foreach_use_including_if(src, def) {
      /* Check that the def matches. */
      validate_assert(state, src->ssa == def);

      /* Check that nir_src's are unique */
      tag_src(src, state);
   }
}

static void
validate_alu_instr(nir_alu_instr *instr, validate_state *state)
{
   validate_assert(state, instr->op < nir_num_opcodes);

   unsigned instr_bit_size = 0;
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      nir_alu_type src_type = nir_op_infos[instr->op].input_types[i];
      unsigned src_bit_size = nir_src_bit_size(instr->src[i].src);
      if (nir_alu_type_get_type_size(src_type)) {
         validate_assert(state, src_bit_size == nir_alu_type_get_type_size(src_type));
      } else if (instr_bit_size) {
         validate_assert(state, src_bit_size == instr_bit_size);
      } else {
         instr_bit_size = src_bit_size;
      }

      if (nir_alu_type_get_base_type(src_type) == nir_type_float) {
         /* 8-bit float isn't a thing */
         validate_assert(state, src_bit_size == 16 || src_bit_size == 32 ||
                                   src_bit_size == 64);
      }

      /* In nir_opcodes.py, these are defined to take general uint or int
       * sources.  However, they're really only defined for 32-bit or 64-bit
       * sources.  This seems to be the only place to enforce this
       * restriction.
       */
      switch (instr->op) {
      case nir_op_ufind_msb:
      case nir_op_ufind_msb_rev:
         validate_assert(state, src_bit_size == 32 || src_bit_size == 64);
         break;

      default:
         break;
      }

      validate_alu_src(instr, i, state);
   }

   nir_alu_type dest_type = nir_op_infos[instr->op].output_type;
   unsigned dest_bit_size = instr->def.bit_size;
   if (nir_alu_type_get_type_size(dest_type)) {
      validate_assert(state, dest_bit_size == nir_alu_type_get_type_size(dest_type));
   } else if (instr_bit_size) {
      validate_assert(state, dest_bit_size == instr_bit_size);
   } else {
      /* The only unsized thing is the destination so it's vacuously valid */
   }

   if (nir_alu_type_get_base_type(dest_type) == nir_type_float) {
      /* 8-bit float isn't a thing */
      validate_assert(state, dest_bit_size == 16 || dest_bit_size == 32 ||
                                dest_bit_size == 64);
   }

   validate_def(&instr->def, state);

   validate_assert(state, (instr->fp_math_ctrl & ~nir_op_infos[instr->op].valid_fp_math_ctrl) == 0);
   validate_assert(state, nir_alu_instr_no_transform(instr) || !nir_alu_instr_no_contract(instr));
   validate_assert(state, nir_alu_instr_no_transform(instr) || !nir_alu_instr_no_reassoc(instr));
}

static void
validate_var_use(nir_variable *var, validate_state *state)
{
   struct hash_entry *entry = _mesa_hash_table_search(state->var_defs, var);
   validate_assert(state, entry);
   if (entry && var->data.mode == nir_var_function_temp)
      validate_assert(state, (nir_function_impl *)entry->data == state->impl);
}

static void
validate_deref_instr(nir_deref_instr *instr, validate_state *state)
{
   if (instr->deref_type == nir_deref_type_var) {
      /* Variable dereferences are stupid simple. */
      validate_assert(state, instr->modes == instr->var->data.mode);
      validate_assert(state, instr->type == instr->var->type);
      validate_var_use(instr->var, state);
   } else if (instr->deref_type == nir_deref_type_cast) {
      /* For cast, we simply have to trust the instruction.  It's up to
       * lowering passes and front/back-ends to make them sane.
       */
      validate_src(&instr->parent, state);

      /* Most variable modes in NIR can only exist by themselves. */
      if (instr->modes & ~nir_var_mem_generic)
         validate_assert(state, util_bitcount(instr->modes) == 1);

      nir_deref_instr *parent = nir_src_as_deref(instr->parent);
      if (parent) {
         if (parent->modes & nir_var_resource_heap) {
            /* Some casts from the resource heap pointer are allowed. */
            validate_assert(state, instr->modes & (nir_var_resource_heap |
                                                   nir_var_image |
                                                   nir_var_uniform |
                                                   nir_var_mem_ubo |
                                                   nir_var_mem_ssbo));
         } else if (parent->modes & nir_var_sampler_heap) {
            /* Some casts from the sampler heap pointer are allowed. */
            validate_assert(state, instr->modes & (nir_var_sampler_heap |
                                                   nir_var_uniform));
         } else {
            /* Casts can change the mode but it can't change completely. The
             * new mode must have some bits in common with the old.
             */
            validate_assert(state, instr->modes & parent->modes);
         }
      } else {
         /* If our parent isn't a deref, just assert the mode is there */
         validate_assert(state, instr->modes != 0);
      }

      /* We just validate that the type is there */
      validate_assert(state, instr->type);
      if (instr->cast.align_mul > 0) {
         validate_assert(state, util_is_power_of_two_nonzero(instr->cast.align_mul));
         validate_assert(state, instr->cast.align_offset < instr->cast.align_mul);
      } else {
         validate_assert(state, instr->cast.align_offset == 0);
      }
   } else {
      /* The parent pointer value must have the same number of components
       * as the destination.
       */
      validate_sized_src(&instr->parent, state, instr->def.bit_size,
                         instr->def.num_components);

      nir_instr *parent_instr = nir_def_instr(instr->parent.ssa);

      /* The parent must come from another deref instruction */
      validate_assert(state, parent_instr->type == nir_instr_type_deref);

      nir_deref_instr *parent = nir_instr_as_deref(parent_instr);

      validate_assert(state, instr->modes == parent->modes);

      switch (instr->deref_type) {
      case nir_deref_type_struct:
         validate_assert(state, glsl_type_is_struct_or_ifc(parent->type));
         validate_assert(state,
                         instr->strct.index < glsl_get_length(parent->type));
         validate_assert(state, instr->type ==
                                   glsl_get_struct_field(parent->type, instr->strct.index));
         break;

      case nir_deref_type_array:
      case nir_deref_type_array_wildcard:
         if (instr->modes & nir_var_vec_indexable_modes) {
            /* Shared variables and UBO/SSBOs have a bit more relaxed rules
             * because we need to be able to handle array derefs on vectors.
             * Fortunately, nir_lower_io handles these just fine.
             */
            validate_assert(state, glsl_type_is_array(parent->type) ||
                                      glsl_type_is_matrix(parent->type) ||
                                      glsl_type_is_vector(parent->type));
         } else {
            /* Most of NIR cannot handle array derefs on vectors */
            validate_assert(state, glsl_type_is_array(parent->type) ||
                                      glsl_type_is_matrix(parent->type));
         }
         validate_assert(state,
                         instr->type == glsl_get_array_element(parent->type));

         if (instr->deref_type == nir_deref_type_array) {
            validate_sized_src(&instr->arr.index, state,
                               instr->def.bit_size, 1);
         }
         break;

      case nir_deref_type_ptr_as_array:
         /* ptr_as_array derefs must have a parent that is either an array,
          * ptr_as_array, or cast.  If the parent is a cast, we get the stride
          * information (if any) from the cast deref.
          */
         validate_assert(state,
                         parent->deref_type == nir_deref_type_array ||
                            parent->deref_type == nir_deref_type_ptr_as_array ||
                            parent->deref_type == nir_deref_type_cast);
         validate_sized_src(&instr->arr.index, state,
                            instr->def.bit_size, 1);
         break;

      default:
         UNREACHABLE("Invalid deref instruction type");
      }
   }

   /* We intentionally don't validate the size of the destination because we
    * want to let other compiler components such as SPIR-V decide how big
    * pointers should be.
    */
   validate_def(&instr->def, state);

   /* Certain modes cannot be used as sources for phi instructions because
    * way too many passes assume that they can always chase deref chains.
    */
   nir_foreach_use_including_if(use, &instr->def) {
      /* Deref instructions as if conditions don't make sense because if
       * conditions expect well-formed Booleans.  If you want to compare with
       * NULL, an explicit comparison operation should be used.
       */
      if (!validate_assert(state, !nir_src_is_if(use)))
         continue;

      if (src_parent_instr_safe(use)->type == nir_instr_type_phi) {
         validate_assert(state, !(instr->modes & (nir_var_shader_in |
                                                  nir_var_shader_out |
                                                  nir_var_shader_out |
                                                  nir_var_uniform)));
      }
   }
}

static bool
vectorized_intrinsic(nir_intrinsic_instr *intr)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[intr->intrinsic];

   if (info->dest_components == 0)
      return true;

   for (unsigned i = 0; i < info->num_srcs; i++)
      if (info->src_components[i] == 0)
         return true;

   return false;
}

/** Returns the image format or PIPE_FORMAT_COUNT for incomplete derefs
 *
 * We use PIPE_FORMAT_COUNT for incomplete derefs because PIPE_FORMAT_NONE
 * indicates that we found the variable but it has no format specified.
 */
static enum pipe_format
image_intrin_format(nir_intrinsic_instr *instr)
{
   if (nir_intrinsic_format(instr) != PIPE_FORMAT_NONE)
      return nir_intrinsic_format(instr);

   /* If this not a deref intrinsic, PIPE_FORMAT_NONE is the best we can do */
   if (nir_intrinsic_infos[instr->intrinsic].src_components[0] != -1)
      return PIPE_FORMAT_NONE;

   nir_variable *var = nir_intrinsic_get_var(instr, 0);
   if (var == NULL)
      return PIPE_FORMAT_COUNT;

   return var->data.image.format;
}

static void
validate_register_handle(nir_src handle_src,
                         unsigned num_components,
                         unsigned bit_size,
                         validate_state *state)
{
   nir_def *handle = handle_src.ssa;
   nir_instr *parent = nir_def_instr(handle);

   if (!validate_assert(state, parent->type == nir_instr_type_intrinsic))
      return;

   nir_intrinsic_instr *intr = nir_instr_as_intrinsic(parent);
   if (!validate_assert(state, intr->intrinsic == nir_intrinsic_decl_reg))
      return;

   validate_assert(state, nir_intrinsic_num_components(intr) == num_components);
   validate_assert(state, nir_intrinsic_bit_size(intr) == bit_size);
}

static void
validate_intrinsic_instr(nir_intrinsic_instr *instr, validate_state *state)
{
   unsigned dest_bit_size = 0;
   unsigned src_bit_sizes[NIR_INTRINSIC_MAX_INPUTS] = {
      0,
   };
   switch (instr->intrinsic) {
   case nir_intrinsic_decl_reg:
      assert(state->block == nir_start_block(state->impl));
      break;

   case nir_intrinsic_load_reg:
   case nir_intrinsic_load_reg_indirect:
      validate_register_handle(instr->src[0],
                               instr->def.num_components,
                               instr->def.bit_size, state);
      break;

   case nir_intrinsic_store_reg:
   case nir_intrinsic_store_reg_indirect:
      validate_register_handle(instr->src[1],
                               nir_src_num_components(instr->src[0]),
                               nir_src_bit_size(instr->src[0]), state);
      break;

   case nir_intrinsic_convert_alu_types: {
      nir_alu_type src_type = nir_intrinsic_src_type(instr);
      nir_alu_type dest_type = nir_intrinsic_dest_type(instr);
      dest_bit_size = nir_alu_type_get_type_size(dest_type);
      src_bit_sizes[0] = nir_alu_type_get_type_size(src_type);
      validate_assert(state, dest_bit_size != 0);
      validate_assert(state, src_bit_sizes[0] != 0);
      break;
   }

   case nir_intrinsic_load_param: {
      unsigned param_idx = nir_intrinsic_param_idx(instr);
      validate_assert(state, param_idx < state->impl->function->num_params);
      nir_parameter *param = &state->impl->function->params[param_idx];
      validate_assert(state, instr->num_components == param->num_components);
      dest_bit_size = param->bit_size;
      break;
   }

   case nir_intrinsic_load_deref: {
      nir_deref_instr *src = nir_src_as_deref(instr->src[0]);
      assert(src);
      validate_assert(state, glsl_type_is_vector_or_scalar(src->type) ||
                                (src->modes == nir_var_uniform &&
                                 glsl_get_base_type(src->type) == GLSL_TYPE_SUBROUTINE));
      validate_assert(state, instr->num_components ==
                                glsl_get_vector_elements(src->type));
      dest_bit_size = glsl_get_bit_size(src->type);
      /* Also allow 32-bit boolean load operations */
      if (glsl_type_is_boolean(src->type))
         dest_bit_size |= 32;
      break;
   }

   case nir_intrinsic_store_deref: {
      nir_deref_instr *dst = nir_src_as_deref(instr->src[0]);
      assert(dst);
      validate_assert(state, glsl_type_is_vector_or_scalar(dst->type));
      validate_assert(state, instr->num_components ==
                                glsl_get_vector_elements(dst->type));
      src_bit_sizes[1] = glsl_get_bit_size(dst->type);
      /* Also allow 32-bit boolean store operations */
      if (glsl_type_is_boolean(dst->type))
         src_bit_sizes[1] |= 32;
      validate_assert(state, !nir_deref_mode_may_be(dst, nir_var_read_only_modes));
      break;
   }

   case nir_intrinsic_copy_deref: {
      nir_deref_instr *dst = nir_src_as_deref(instr->src[0]);
      nir_deref_instr *src = nir_src_as_deref(instr->src[1]);
      validate_assert(state, glsl_get_bare_type(dst->type) ==
                                glsl_get_bare_type(src->type));
      validate_assert(state, !nir_deref_mode_may_be(dst, nir_var_read_only_modes));
      /* FIXME: now that we track if the var copies were lowered, it would be
       * good to validate here that no new copy derefs were added. Right now
       * we can't as there are some specific cases where copies are added even
       * after the lowering. One example is the Intel compiler, that calls
       * nir_lower_io_vars_to_temporaries when linking some shader stages.
       */
      break;
   }

   case nir_intrinsic_load_ubo_vec4: {
      int bit_size = instr->def.bit_size;
      validate_assert(state, bit_size >= 8);
      validate_assert(state, (nir_intrinsic_component(instr) +
                              instr->num_components) *
                                   (bit_size / 8) <=
                                16);
      break;
   }

   case nir_intrinsic_load_ubo:
      /* Make sure that the creator didn't forget to set the range_base+range. */
      validate_assert(state, nir_intrinsic_range(instr) != 0);
      FALLTHROUGH;
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_constant:
      /* These memory load operations must have alignments */
      validate_assert(state,
                      util_is_power_of_two_nonzero(nir_intrinsic_align_mul(instr)));
      validate_assert(state, nir_intrinsic_align_offset(instr) <
                                nir_intrinsic_align_mul(instr));
      FALLTHROUGH;

   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_primitive_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_view_output:
   case nir_intrinsic_load_per_primitive_output:
   case nir_intrinsic_load_push_constant:
   case nir_intrinsic_load_attribute_pan:
      /* All memory load operations must load at least a byte */
      validate_assert(state, instr->def.bit_size >= 8);
      break;

   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_barycentric_at_sample: {
      enum glsl_interp_mode mode = nir_intrinsic_interp_mode(instr);
      validate_assert(state,
                      mode == INTERP_MODE_NONE ||
                         mode == INTERP_MODE_SMOOTH ||
                         mode == INTERP_MODE_NOPERSPECTIVE);
      break;
   }

   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
      /* These memory store operations must also have alignments */
      validate_assert(state,
                      util_is_power_of_two_nonzero(nir_intrinsic_align_mul(instr)));
      validate_assert(state, nir_intrinsic_align_offset(instr) <
                                nir_intrinsic_align_mul(instr));
      /* All memory store operations must store at least a byte */
      validate_assert(state, nir_src_bit_size(instr->src[0]) >= 8);
      break;

   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_view_output:
      validate_assert(state, nir_src_bit_size(instr->src[0]) >= 8);
      validate_assert(state,
                      nir_src_bit_size(instr->src[0]) ==
                         nir_alu_type_get_type_size(nir_intrinsic_src_type(instr)));
      break;

   case nir_intrinsic_deref_mode_is:
   case nir_intrinsic_addr_mode_is:
      validate_assert(state,
                      util_bitcount(nir_intrinsic_memory_modes(instr)) == 1);
      break;

   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_heap_atomic:
   case nir_intrinsic_image_heap_atomic_swap: {
      nir_atomic_op op = nir_intrinsic_atomic_op(instr);

      enum pipe_format format = image_intrin_format(instr);
      if (format != PIPE_FORMAT_COUNT) {
         bool allowed = false;
         bool is_float = (nir_atomic_op_type(op) == nir_type_float);

         switch (format) {
         case PIPE_FORMAT_R32_FLOAT:
         case PIPE_FORMAT_R16G16_FLOAT:
         case PIPE_FORMAT_R16G16B16A16_FLOAT:
            allowed = is_float || op == nir_atomic_op_xchg;
            break;
         case PIPE_FORMAT_R16_FLOAT:
         case PIPE_FORMAT_R64_FLOAT:
            allowed = op == nir_atomic_op_fmin || op == nir_atomic_op_fmax;
            break;
         case PIPE_FORMAT_R32_UINT:
         case PIPE_FORMAT_R32_SINT:
         case PIPE_FORMAT_R64_UINT:
         case PIPE_FORMAT_R64_SINT:
            allowed = !is_float;
            break;
         default:
            break;
         }

         validate_assert(state, allowed);
         const struct util_format_description *fmt_desc =
            util_format_description(format);
         validate_assert(state, instr->def.bit_size ==
                                fmt_desc->channel[0].size);
         validate_assert(state,
                         instr->num_components >= 1 &&
                         instr->num_components <= 4);
      }
      break;
   }

   case nir_intrinsic_load_buffer_amd:
      if (nir_intrinsic_access(instr) & ACCESS_USES_FORMAT_AMD) {
         nir_alu_type dest_type = nir_intrinsic_dest_type(instr);
         validate_assert(state, nir_alu_type_get_type_size(dest_type) &&
                                nir_alu_type_get_base_type(dest_type));
         validate_assert(state, nir_alu_type_get_type_size(dest_type) ==
                                instr->def.bit_size);
      }
      break;

   case nir_intrinsic_store_buffer_amd:
      if (nir_intrinsic_access(instr) & ACCESS_USES_FORMAT_AMD) {
         unsigned writemask = nir_intrinsic_write_mask(instr);

         /* Make sure the writemask is derived from the component count. */
         validate_assert(state,
                         writemask ==
                            BITFIELD_MASK(nir_src_num_components(instr->src[0])));
      }
      break;

   case nir_intrinsic_load_deref_transpose_amd: {
      nir_deref_instr *src = nir_src_as_deref(instr->src[0]);
      assert(src);
      unsigned disallow_access = ACCESS_ATOMIC | ACCESS_SKIP_HELPERS | ACCESS_SMEM_AMD;
      validate_assert(state, !(nir_intrinsic_access(instr) & disallow_access));
      validate_assert(state, glsl_type_is_scalar(src->type));
      validate_assert(state, instr->num_components == 8 || instr->num_components == 4);
      dest_bit_size = glsl_get_bit_size(src->type);
      src_bit_sizes[0] = 64;
      break;
   }

   case nir_intrinsic_load_global_transpose_amd:
   case nir_intrinsic_load_global_tr_amd: {
      unsigned disallow_access = ACCESS_ATOMIC | ACCESS_SKIP_HELPERS | ACCESS_SMEM_AMD;
      validate_assert(state, !(nir_intrinsic_access(instr) & disallow_access));
      validate_assert(state, instr->num_components == 8 || instr->num_components == 4);
      src_bit_sizes[0] = 64;
      src_bit_sizes[1] = 32;
      break;
   }

   case nir_intrinsic_global_atomic_nv:
   case nir_intrinsic_global_atomic_swap_nv:
   case nir_intrinsic_shared_atomic_nv:
   case nir_intrinsic_shared_atomic_swap_nv:
   case nir_intrinsic_ldc_nv:
   case nir_intrinsic_ldcx_nv:
   case nir_intrinsic_load_global_nv:
   case nir_intrinsic_load_scratch_nv:
   case nir_intrinsic_load_shared_nv:
   case nir_intrinsic_store_global_nv:
   case nir_intrinsic_store_scratch_nv:
   case nir_intrinsic_store_shared_nv:
   case nir_intrinsic_store_shared_unlock_nv:
   case nir_intrinsic_vild_nv: {
      int base = nir_intrinsic_base(instr);
      nir_src src = *nir_get_io_offset_src(instr);
      nir_src *uniform_src = nir_get_io_uniform_offset_src(instr);
      unsigned const_bits = nir_get_io_base_size_nv(instr);

      if (nir_src_is_const(src) && nir_src_as_int(src) == 0 &&
          (!uniform_src || (nir_src_is_const(*uniform_src) && nir_src_as_int(*uniform_src) == 0))) {
         validate_assert(state, base >= 0 && base < BITFIELD_MASK(const_bits));
      } else {
         int32_t max = BITFIELD_MASK(const_bits - 1);
         int32_t min = ~BITFIELD_MASK(const_bits - 1);
         validate_assert(state, base >= min && base < max);
      }

      if (uniform_src)
         validate_assert(state, uniform_src->ssa->bit_size >= src.ssa->bit_size);

      if (instr->intrinsic == nir_intrinsic_load_global_nv) {
         validate_assert(state, instr->src[2].ssa->bit_size == 1);
      }

      break;
   }

   case nir_intrinsic_barrier: {
      unsigned semantics = nir_intrinsic_memory_semantics(instr);
      bool is_arrive = semantics & NIR_MEMORY_CONTROL_ARRIVE;
      bool is_wait = semantics & NIR_MEMORY_CONTROL_WAIT;
      if (nir_intrinsic_execution_scope(instr) != SCOPE_NONE) {
         if (is_wait)
            validate_assert(state, is_arrive || !(semantics & NIR_MEMORY_RELEASE));
         if (is_arrive)
            validate_assert(state, is_wait || !(semantics & NIR_MEMORY_ACQUIRE));
      } else {
         validate_assert(state, !is_arrive && !is_wait);
      }
      break;
   }

   default:
      break;
   }

   if (instr->num_components > 0)
      validate_num_components(state, instr->num_components);

   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   unsigned num_srcs = info->num_srcs;
   for (unsigned i = 0; i < num_srcs; i++) {
      unsigned components_read = nir_intrinsic_src_components(instr, i);

      validate_num_components(state, components_read);

      validate_sized_src(&instr->src[i], state, src_bit_sizes[i], components_read);
   }

   if (nir_intrinsic_infos[instr->intrinsic].has_dest) {
      unsigned components_written = nir_intrinsic_dest_components(instr);
      unsigned bit_sizes = info->dest_bit_sizes;
      if (!bit_sizes && info->bit_size_src >= 0)
         bit_sizes = nir_src_bit_size(instr->src[info->bit_size_src]);

      validate_num_components(state, components_written);
      if (dest_bit_size && bit_sizes)
         validate_assert(state, dest_bit_size & bit_sizes);
      else
         dest_bit_size = dest_bit_size ? dest_bit_size : bit_sizes;

      validate_def(&instr->def, state);
      validate_assert(state, instr->def.num_components == components_written);

      if (dest_bit_size)
         validate_assert(state, instr->def.bit_size & dest_bit_size);
   }

   if (!vectorized_intrinsic(instr))
      validate_assert(state, instr->num_components == 0);

   if (nir_intrinsic_has_write_mask(instr)) {
      unsigned component_mask = BITFIELD_MASK(instr->num_components);
      validate_assert(state, (nir_intrinsic_write_mask(instr) & ~component_mask) == 0);
   }

   if (nir_intrinsic_has_io_xfb(instr)) {
      unsigned used_mask = 0;

      nir_io_xfb xfb = nir_intrinsic_io_xfb(instr);
      for (unsigned i = 0; i < 4; i++) {
         unsigned xfb_mask = BITFIELD_RANGE(i, xfb.out[i].num_components);

         /* Each component can be used only once by transform feedback info. */
         validate_assert(state, (xfb_mask & used_mask) == 0);
         used_mask |= xfb_mask;
      }
   }

   if (nir_intrinsic_has_io_semantics(instr) &&
       !nir_intrinsic_io_semantics(instr).no_validate) {
      bool is_input = instr->intrinsic == nir_intrinsic_load_input ||
                      instr->intrinsic == nir_intrinsic_load_per_vertex_input ||
                      instr->intrinsic == nir_intrinsic_load_per_primitive_input ||
                      instr->intrinsic == nir_intrinsic_load_interpolated_input ||
                      instr->intrinsic == nir_intrinsic_load_input_vertex;
      bool is_output = instr->intrinsic == nir_intrinsic_load_output ||
                       instr->intrinsic == nir_intrinsic_load_per_vertex_output ||
                       instr->intrinsic == nir_intrinsic_load_per_primitive_output ||
                       instr->intrinsic == nir_intrinsic_load_per_view_output ||
                       instr->intrinsic == nir_intrinsic_store_output ||
                       instr->intrinsic == nir_intrinsic_store_per_vertex_output ||
                       instr->intrinsic == nir_intrinsic_store_per_primitive_output ||
                       instr->intrinsic == nir_intrinsic_store_per_view_output;
      /* Driver-specific intrinsics with IO semantics are not validated. */
      bool is_core_intrinsic = is_input || is_output;

      if (is_core_intrinsic) {
         nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
         unsigned max_num_components;

         /* Validate that a single intrinsic doesn't access past the maximum
          * number of components per slot.
          */
         /* TODO: remove nir_io_radv_intrinsic_component_workaround */
         if (state->shader->options->io_options &
             nir_io_radv_intrinsic_component_workaround && sem.high_16bits)
            max_num_components = 8;
         else
            max_num_components = 4;

         if (nir_intrinsic_infos[instr->intrinsic].has_dest) {
            /* Input and output loads shouldn't load from multiple slots at once. */
            validate_assert(state, nir_intrinsic_component(instr) +
                            instr->def.num_components <= max_num_components);
         } else {
            /* Output stores shouldn't write to multiple slots at once. */
            validate_assert(state, nir_intrinsic_component(instr) +
                            instr->num_components <= max_num_components);
         }

         if (is_input) {
            validate_assert(state, !sem.no_sysval_output);
            validate_assert(state, !sem.no_varying);
            validate_assert(state, !sem.gs_streams);
         } else {
            /* An output that has no effect shouldn't be present in the IR. */
            validate_assert(state,
                            (nir_slot_is_sysval_output(sem.location, MESA_SHADER_NONE) &&
                             !sem.no_sysval_output) ||
                            (nir_slot_is_varying(sem.location, MESA_SHADER_NONE) &&
                             !sem.no_varying) ||
                            nir_instr_xfb_write_mask(instr) ||
                            /* TCS can set no_varying and no_sysval_output, meaning
                             * that the output is only read by TCS and not TES.
                             */
                            state->shader->info.stage == MESA_SHADER_TESS_CTRL);

            validate_assert(state,
                            !sem.gs_streams ||
                            state->shader->info.stage == MESA_SHADER_GEOMETRY);
         }

         validate_assert(state,
                         (!sem.dual_source_blend_index &&
                          !sem.fb_fetch_output &&
                          !sem.fb_fetch_output_coherent) ||
                         (state->shader->info.stage == MESA_SHADER_FRAGMENT &&
                          is_output));

         validate_assert(state,
                         !sem.high_dvec2 ||
                         (state->shader->info.stage == MESA_SHADER_VERTEX &&
                          is_input));

         validate_assert(state,
                         !sem.interp_explicit_strict ||
                            (state->shader->info.stage == MESA_SHADER_FRAGMENT &&
                             instr->intrinsic == nir_intrinsic_load_input_vertex));

         /* Non-constant src offset with num_slots == 1 is disallowed. */
         if (sem.num_slots == 1) {
            nir_src *offset_src = nir_get_io_offset_src(instr);

            /* TODO: nir_opt_loop produces phi 0, 0 for store_output as follows,
             * which isn't incorrect, but also isn't useful:
             *
             * Before:
             *   %1 = load_const 0
             *   loop {
             *      if {
             *      } else {
             *      }
             *   }
             *   store_output(, %1)
             *
             * After:
             *   %1 = load_const 0
             *   if {
             *   } else {
             *      loop {
             *         %38 = load_const 0
             *      }
             *   }
             *   %52 = phi %1, %38
             *   store_output(, %52)
             *
             * Test: KHR-GLES3.shaders.indexing.uniform_array.float_dynamic_loop_read_fragment
             *
             * So allow phis in offset_src with num_slots == 1 for now.
             */
            validate_assert(state,
                            nir_src_is_const(*offset_src) ||
                            nir_def_is_phi(offset_src->ssa));
         }
      }
   }

   if (nir_intrinsic_has_offset_shift(instr) &&
       nir_intrinsic_has_align(instr)) {
      unsigned min_align = 1 << nir_intrinsic_offset_shift(instr);
      validate_assert(state, nir_intrinsic_align(instr) >= min_align);
   }

   if (nir_intrinsic_has_offset_shift_nv(instr)) {
      unsigned shift = nir_intrinsic_offset_shift_nv(instr);
      validate_assert(state, shift == 0 || (shift >= 2 && shift <= 4));
   }
}

static void
validate_tex_src_texture_deref(nir_tex_instr *instr, validate_state *state,
                               nir_deref_instr *deref)
{
   validate_assert(state, glsl_type_is_image(deref->type) ||
                             glsl_type_is_texture(deref->type) ||
                             glsl_type_is_sampler(deref->type));

   switch (instr->op) {
   case nir_texop_descriptor_amd:
   case nir_texop_sampler_descriptor_amd:
   case nir_texop_custom_border_color_agx:
      break;
   case nir_texop_lod:
   case nir_texop_lod_bias:
      validate_assert(state, nir_alu_type_get_base_type(instr->dest_type) == nir_type_float);
      break;
   case nir_texop_image_min_lod_agx:
      validate_assert(state, instr->dest_type == nir_type_float16 ||
                                instr->dest_type == nir_type_uint16);
      break;
   case nir_texop_samples_identical:
   case nir_texop_has_custom_border_color_agx:
      validate_assert(state, nir_alu_type_get_base_type(instr->dest_type) == nir_type_bool);
      break;
   case nir_texop_txs:
   case nir_texop_texture_samples:
   case nir_texop_query_levels:
   case nir_texop_fragment_mask_fetch_amd:
   case nir_texop_txf_ms_mcs_intel:
      validate_assert(state, nir_alu_type_get_base_type(instr->dest_type) == nir_type_int ||
                                nir_alu_type_get_base_type(instr->dest_type) == nir_type_uint);
      break;
   default:
      validate_assert(state,
                      glsl_get_sampler_result_type(deref->type) == GLSL_TYPE_VOID ||
                         glsl_base_type_is_integer(glsl_get_sampler_result_type(deref->type)) ==
                            (nir_alu_type_get_base_type(instr->dest_type) == nir_type_int ||
                             nir_alu_type_get_base_type(instr->dest_type) == nir_type_uint));
   }
}

static void
validate_tex_instr(nir_tex_instr *instr, validate_state *state)
{
   bool src_type_seen[nir_num_tex_src_types];
   for (unsigned i = 0; i < nir_num_tex_src_types; i++)
      src_type_seen[i] = false;

   for (unsigned i = 0; i < instr->num_srcs; i++) {
      validate_assert(state, !src_type_seen[instr->src[i].src_type]);
      src_type_seen[instr->src[i].src_type] = true;
      validate_sized_src(&instr->src[i].src, state,
                         0, nir_tex_instr_src_size(instr, i));

      switch (instr->src[i].src_type) {

      case nir_tex_src_comparator:
         validate_assert(state, instr->is_shadow);
         break;

      case nir_tex_src_bias:
         validate_assert(state, instr->op == nir_texop_txb ||
                                   instr->op == nir_texop_tg4 ||
                                   instr->op == nir_texop_lod ||
                                   instr->op == nir_texop_sparse_residency_intel);
         break;

      case nir_tex_src_lod:
         validate_assert(state, instr->op != nir_texop_tex &&
                                   instr->op != nir_texop_txb &&
                                   instr->op != nir_texop_txd &&
                                   instr->op != nir_texop_lod);
         break;

      case nir_tex_src_ddx:
      case nir_tex_src_ddy:
         validate_assert(state,
                         instr->op == nir_texop_txd ||
                            instr->op == nir_texop_sparse_residency_intel);
         break;

      case nir_tex_src_texture_deref:
      case nir_tex_src_texture_2_deref: {
         nir_deref_instr *deref = nir_src_as_deref(instr->src[i].src);
         if (!validate_assert(state, deref))
            break;

         validate_tex_src_texture_deref(instr, state, deref);
         break;
      }

      case nir_tex_src_sampler_deref:
      case nir_tex_src_sampler_2_deref: {
         nir_deref_instr *deref = nir_src_as_deref(instr->src[i].src);
         if (!validate_assert(state, deref))
            break;

         validate_assert(state, glsl_type_is_sampler(deref->type));
         break;
      }

      case nir_tex_src_sampler_deref_intrinsic:
      case nir_tex_src_texture_deref_intrinsic: {
         nir_intrinsic_instr *intrin =
            nir_def_as_intrinsic(instr->src[i].src.ssa);
         nir_deref_instr *deref =
            nir_def_as_deref(intrin->src[0].ssa);
         if (!validate_assert(state, deref))
            break;

         if (instr->src[i].src_type == nir_tex_src_sampler_deref_intrinsic)
            validate_assert(state, glsl_type_is_sampler(deref->type));
         else
            validate_tex_src_texture_deref(instr, state, deref);

         break;
      }

      case nir_tex_src_block_size:
         validate_assert(state,
                         instr->op == nir_texop_block_match_sad_qcom ||
                         instr->op == nir_texop_block_match_ssd_qcom);
         break;

      case nir_tex_src_box_size:
         validate_assert(state, instr->op == nir_texop_box_filter_qcom);
         break;

      case nir_tex_src_coord:
      case nir_tex_src_ref_coord:
      case nir_tex_src_projector:
      case nir_tex_src_offset:
      case nir_tex_src_min_lod:
      case nir_tex_src_ms_index:
      case nir_tex_src_texture_offset:
      case nir_tex_src_sampler_offset:
      case nir_tex_src_plane:
      case nir_tex_src_texture_handle:
      case nir_tex_src_sampler_handle:
      case nir_tex_src_texture_2_handle:
      case nir_tex_src_sampler_2_handle:
         break;

      default:
         break;
      }
   }

   bool msaa = (instr->sampler_dim == GLSL_SAMPLER_DIM_MS ||
                instr->sampler_dim == GLSL_SAMPLER_DIM_SUBPASS_MS);

   if (msaa)
      validate_assert(state, instr->op != nir_texop_txf);
   else
      validate_assert(state, instr->op != nir_texop_txf_ms);

   if (instr->op != nir_texop_tg4)
      validate_assert(state, instr->component == 0);

   if (nir_tex_instr_has_explicit_tg4_offsets(instr)) {
      validate_assert(state, instr->op == nir_texop_tg4);
      validate_assert(state, !src_type_seen[nir_tex_src_offset]);
   }

   if (instr->is_gather_implicit_lod)
      validate_assert(state, instr->op == nir_texop_tg4);

   validate_def(&instr->def, state);
   validate_assert(state, instr->def.num_components ==
                             nir_tex_instr_dest_size(instr));

   unsigned bit_size = nir_alu_type_get_type_size(instr->dest_type);
   validate_assert(state,
                   (bit_size ? bit_size : 32) ==
                      instr->def.bit_size);
}

static void
validate_call_instr(nir_call_instr *instr, validate_state *state)
{
   validate_assert(state, instr->num_params == instr->callee->num_params);

   if (instr->indirect_callee.ssa) {
      validate_assert(state, !instr->callee->impl);
      validate_src(&instr->indirect_callee, state);
   }

   for (unsigned i = 0; i < instr->num_params; i++) {
      validate_sized_src(&instr->params[i], state,
                         instr->callee->params[i].bit_size,
                         instr->callee->params[i].num_components);
   }
}

static void
validate_cmat_call_instr(nir_cmat_call_instr *instr, validate_state *state)
{
   validate_assert(state, instr->num_params == nir_cmat_call_op_params(instr->op, instr->callee));

   for (unsigned i = 0; i < instr->num_params; i++) {
      validate_src(&instr->params[i], state);
   }
}

static void
validate_const_value(nir_const_value *val, unsigned bit_size,
                     bool is_null_constant, validate_state *state)
{
   /* In order for block copies to work properly for things like instruction
    * comparisons and [de]serialization, we require the unused bits of the
    * nir_const_value to be zero.
    */
   nir_const_value cmp_val;
   memset(&cmp_val, 0, sizeof(cmp_val));
   if (!is_null_constant) {
      switch (bit_size) {
      case 1:
         cmp_val.b = val->b;
         break;
      case 8:
         cmp_val.u8 = val->u8;
         break;
      case 16:
         cmp_val.u16 = val->u16;
         break;
      case 32:
         cmp_val.u32 = val->u32;
         break;
      case 64:
         cmp_val.u64 = val->u64;
         break;
      default:
         validate_assert(state, !"Invalid load_const bit size");
      }
   }
   validate_assert(state, memcmp(val, &cmp_val, sizeof(cmp_val)) == 0);
}

static void
validate_load_const_instr(nir_load_const_instr *instr, validate_state *state)
{
   validate_def(&instr->def, state);

   for (unsigned i = 0; i < instr->def.num_components; i++)
      validate_const_value(&instr->value[i], instr->def.bit_size, false, state);
}

static void
validate_ssa_undef_instr(nir_undef_instr *instr, validate_state *state)
{
   validate_def(&instr->def, state);
}

static void
validate_phi_instr(nir_phi_instr *instr, validate_state *state)
{
   /*
    * don't validate the sources until we get to them from their predecessor
    * basic blocks, to avoid validating an SSA use before its definition.
    */

   validate_def(&instr->def, state);

   exec_list_validate(&instr->srcs);
   validate_assert(state, exec_list_length(&instr->srcs) ==
                             nir_block_num_preds(state->block));
}

static void
validate_jump_instr(nir_jump_instr *instr, validate_state *state)
{
   nir_block *block = state->block;
   validate_assert(state, &instr->instr == nir_block_last_instr(block));

   switch (instr->type) {
   case nir_jump_return:
   case nir_jump_halt:
   case nir_jump_abort:
      validate_assert(state, block->successors[0] == state->impl->end_block);
      validate_assert(state, block->successors[1] == NULL);
      validate_assert(state, instr->target == NULL);
      validate_assert(state, instr->else_target == NULL);
      validate_assert(state, !state->in_loop_continue_construct);
      break;

   case nir_jump_break:
      validate_assert(state, state->impl->structured);
      validate_assert(state, state->loop != NULL);
      if (state->loop) {
         nir_block *after =
            nir_cf_node_as_block(nir_cf_node_next(&state->loop->cf_node));
         validate_assert(state, block->successors[0] == after);
      }
      validate_assert(state, block->successors[1] == NULL);
      validate_assert(state, instr->target == NULL);
      validate_assert(state, instr->else_target == NULL);
      break;

   case nir_jump_continue:
      validate_assert(state, state->impl->structured);
      validate_assert(state, state->loop != NULL);
      if (state->loop) {
         validate_assert(state, nir_loop_has_continue_construct(state->loop));
         if (nir_loop_has_continue_construct(state->loop)) {
            nir_block *cont_block = nir_loop_first_continue_block(state->loop);
            validate_assert(state, block->successors[0] == cont_block);
         }
      }
      validate_assert(state, block->successors[1] == NULL);
      validate_assert(state, instr->target == NULL);
      validate_assert(state, instr->else_target == NULL);
      validate_assert(state, !state->in_loop_continue_construct);
      break;

   case nir_jump_goto:
      validate_assert(state, !state->impl->structured);
      validate_assert(state, instr->target == block->successors[0]);
      validate_assert(state, instr->target != NULL);
      validate_assert(state, instr->else_target == NULL);
      break;

   case nir_jump_goto_if:
      validate_assert(state, !state->impl->structured);
      validate_assert(state, instr->target == block->successors[1]);
      validate_assert(state, instr->else_target == block->successors[0]);
      validate_sized_src(&instr->condition, state, 0, 1);
      validate_assert(state, instr->target != NULL);
      validate_assert(state, instr->else_target != NULL);
      break;

   default:
      validate_assert(state, !"Invalid jump instruction type");
      break;
   }
}

static void
validate_instr(nir_instr *instr, validate_state *state)
{
   validate_assert(state, instr->block == state->block);

   state->instr = instr;

   switch (instr->type) {
   case nir_instr_type_alu:
      validate_alu_instr(nir_instr_as_alu(instr), state);
      break;

   case nir_instr_type_deref:
      validate_deref_instr(nir_instr_as_deref(instr), state);
      break;

   case nir_instr_type_call:
      validate_call_instr(nir_instr_as_call(instr), state);
      break;

   case nir_instr_type_cmat_call:
      validate_cmat_call_instr(nir_instr_as_cmat_call(instr), state);
      break;

   case nir_instr_type_intrinsic:
      validate_intrinsic_instr(nir_instr_as_intrinsic(instr), state);
      break;

   case nir_instr_type_tex:
      validate_tex_instr(nir_instr_as_tex(instr), state);
      break;

   case nir_instr_type_load_const:
      validate_load_const_instr(nir_instr_as_load_const(instr), state);
      break;

   case nir_instr_type_phi:
      validate_phi_instr(nir_instr_as_phi(instr), state);
      break;

   case nir_instr_type_undef:
      validate_ssa_undef_instr(nir_instr_as_undef(instr), state);
      break;

   case nir_instr_type_jump:
      validate_jump_instr(nir_instr_as_jump(instr), state);
      break;

   default:
      validate_assert(state, !"Invalid ALU instruction type");
      break;
   }

   state->instr = NULL;
}

static void
validate_phi_src(nir_phi_instr *instr, nir_block *pred, validate_state *state)
{
   state->instr = &instr->instr;

   exec_list_validate(&instr->srcs);
   nir_foreach_phi_src(src, instr) {
      if (src->pred == pred) {
         validate_sized_src(&src->src, state, instr->def.bit_size,
                            instr->def.num_components);
         state->instr = NULL;
         return;
      }
   }
   validate_assert(state, !"Phi does not have a source corresponding to one "
                           "of its predecessor blocks");
}

static void
validate_phi_srcs(nir_block *block, nir_block *succ, validate_state *state)
{
   nir_foreach_phi(phi, succ) {
      validate_phi_src(phi, block, state);
   }
}

static void
collect_blocks(struct exec_list *cf_list, validate_state *state)
{
   /* We walk the blocks manually here rather than using nir_foreach_block for
    * a few reasons:
    *
    *  1. We want to call exec_list_validate() on every linked list in the IR
    *     which means we need to touch every linked and just walking blocks
    *     with nir_foreach_block() would make that difficult.  In particular,
    *     we want to validate each list before the first time we walk it so
    *     that we catch broken lists in exec_list_validate() instead of
    *     getting stuck in a hard-to-debug infinite loop in the validator.
    *
    *  2. nir_foreach_block() depends on several invariants of the CF node
    *     hierarchy which nir_validate_shader() is responsible for verifying.
    *     If we used nir_foreach_block() in nir_validate_shader(), we could
    *     end up blowing up on a bad list walk instead of throwing the much
    *     easier to debug validation error.
    */
   exec_list_validate(cf_list);
   foreach_list_typed(nir_cf_node, node, node, cf_list) {
      switch (node->type) {
      case nir_cf_node_block:
         _mesa_set_add(state->blocks, nir_cf_node_as_block(node));
         break;

      case nir_cf_node_if:
         collect_blocks(&nir_cf_node_as_if(node)->then_list, state);
         collect_blocks(&nir_cf_node_as_if(node)->else_list, state);
         break;

      case nir_cf_node_loop:
         collect_blocks(&nir_cf_node_as_loop(node)->body, state);
         collect_blocks(&nir_cf_node_as_loop(node)->continue_list, state);
         break;

      default:
         UNREACHABLE("Invalid CF node type");
      }
   }
}

static void
collect_blocks_pdfs(nir_function_impl *impl, nir_block *block,
                    uint32_t *count, validate_state *state)
{
   if (block == impl->end_block)
      return;

   if (_mesa_set_search(state->blocks, block))
      return;

   _mesa_set_add(state->blocks, block);

   for (uint32_t i = 0; i < ARRAY_SIZE(block->successors); i++) {
      if (block->successors[i] != NULL)
         collect_blocks_pdfs(impl, block->successors[i], count, state);
   }

   /* Assert that the blocks are indexed in reverse PDFS order */
   validate_assert(state, block->index == --(*count));
}

static void
collect_unstructured_blocks(nir_function_impl *impl, validate_state *state)
{
   exec_list_validate(&impl->body);

   /* Assert that the blocks are properly indexed */
   uint32_t count = 0;
   foreach_list_typed(nir_cf_node, node, node, &impl->body) {
      nir_block *block = nir_cf_node_as_block(node);
      validate_assert(state, block->index == count++);
   }
   validate_assert(state, impl->end_block->index == count);

   collect_blocks_pdfs(impl, nir_start_block(impl), &count, state);
}

static void validate_cf_node(nir_cf_node *node, validate_state *state);

static void
validate_block_predecessors(nir_block *block, validate_state *state)
{
   for (unsigned i = 0; i < 2; i++) {
      if (block->successors[i] == NULL)
         continue;

      /* The block has to exist in the nir_function_impl */
      validate_assert(state, _mesa_set_search(state->blocks,
                                              block->successors[i]));

      /* And we have to be in our successor's predecessors set */
      bool has_pred = false;
      nir_foreach_pred(pred, block->successors[i])
         has_pred |= pred == block;
      validate_assert(state, has_pred);

      validate_phi_srcs(block, block->successors[i], state);
   }

   /* The start block cannot have any predecessors */
   if (block == nir_start_block(state->impl))
      validate_assert(state, nir_block_num_preds(block) == 0);

   /* Check for duplicate predecessors. */
   nir_foreach_pred(pred, block) {
      bool found = false;
      nir_foreach_pred(pred2, block) {
         if (pred == pred2) {
            validate_assert(state, !found);
            found = true;
         }
      }
   }

   nir_foreach_pred(pred, block) {
      validate_assert(state, _mesa_set_search(state->blocks, pred));
      validate_assert(state, pred->successors[0] == block ||
                                pred->successors[1] == block);
   }
}

static void
validate_block(nir_block *block, validate_state *state)
{
   validate_assert(state, block->cf_node.parent == state->parent_node);

   validate_assert(state, block->impl == state->impl);
   state->block = block;

   exec_list_validate(&block->instr_list);
   nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_phi) {
         validate_assert(state, instr == nir_block_first_instr(block) ||
                                   nir_instr_prev(instr)->type == nir_instr_type_phi);
      }

      validate_instr(instr, state);
   }

   validate_assert(state, block->successors[0] != NULL);
   validate_assert(state, block->successors[0] != block->successors[1]);
   validate_block_predecessors(block, state);

   if (!state->impl->structured) {
      validate_assert(state, nir_block_ends_in_jump(block));
   } else if (!nir_block_ends_in_jump(block)) {
      nir_cf_node *next = nir_cf_node_next(&block->cf_node);
      if (next == NULL) {
         switch (state->parent_node->type) {
         case nir_cf_node_loop: {
            if (!nir_loop_has_continue_construct(state->loop) ||
                block == nir_loop_last_continue_block(state->loop)) {
               nir_block *header = nir_loop_first_block(state->loop);
               validate_assert(state, block->successors[0] == header);
            } else {
               nir_block *cont = nir_loop_first_continue_block(state->loop);
               validate_assert(state, block->successors[0] == cont);
            }
            /* due to the hack for infinite loops, block->successors[1] may
             * point to the block after the loop.
             */
            break;
         }

         case nir_cf_node_if: {
            nir_block *after =
               nir_cf_node_as_block(nir_cf_node_next(state->parent_node));
            validate_assert(state, block->successors[0] == after);
            validate_assert(state, block->successors[1] == NULL);
            break;
         }

         case nir_cf_node_function:
            validate_assert(state, block->successors[0] == state->impl->end_block);
            validate_assert(state, block->successors[1] == NULL);
            break;

         default:
            UNREACHABLE("unknown control flow node type");
         }
      } else {
         if (next->type == nir_cf_node_if) {
            nir_if *if_stmt = nir_cf_node_as_if(next);
            validate_assert(state, block->successors[0] ==
                                      nir_if_first_then_block(if_stmt));
            validate_assert(state, block->successors[1] ==
                                      nir_if_first_else_block(if_stmt));
         } else if (next->type == nir_cf_node_loop) {
            nir_loop *loop = nir_cf_node_as_loop(next);
            validate_assert(state, block->successors[0] ==
                                      nir_loop_first_block(loop));
            validate_assert(state, block->successors[1] == NULL);
         } else {
            validate_assert(state,
                            !"Structured NIR cannot have consecutive blocks");
         }
      }
   }
}

static void
validate_end_block(nir_block *block, validate_state *state)
{
   validate_assert(state, block->cf_node.parent == &state->impl->cf_node);

   exec_list_validate(&block->instr_list);
   validate_assert(state, exec_list_is_empty(&block->instr_list));

   validate_assert(state, block->successors[0] == NULL);
   validate_assert(state, block->successors[1] == NULL);
   validate_block_predecessors(block, state);
}

static void
validate_if(nir_if *if_stmt, validate_state *state)
{
   validate_assert(state, state->impl->structured);

   state->if_stmt = if_stmt;

   validate_assert(state, !exec_node_is_head_sentinel(if_stmt->cf_node.node.prev));
   nir_cf_node *prev_node = nir_cf_node_prev(&if_stmt->cf_node);
   validate_assert(state, prev_node->type == nir_cf_node_block);

   validate_assert(state, !exec_node_is_tail_sentinel(if_stmt->cf_node.node.next));
   nir_cf_node *next_node = nir_cf_node_next(&if_stmt->cf_node);
   validate_assert(state, next_node->type == nir_cf_node_block);

   validate_assert(state, nir_src_is_if(&if_stmt->condition));
   validate_if_src(&if_stmt->condition, state);

   validate_assert(state, !exec_list_is_empty(&if_stmt->then_list));
   validate_assert(state, !exec_list_is_empty(&if_stmt->else_list));

   nir_cf_node *old_parent = state->parent_node;
   state->parent_node = &if_stmt->cf_node;

   foreach_list_typed(nir_cf_node, cf_node, node, &if_stmt->then_list) {
      validate_cf_node(cf_node, state);
   }

   foreach_list_typed(nir_cf_node, cf_node, node, &if_stmt->else_list) {
      validate_cf_node(cf_node, state);
   }

   state->parent_node = old_parent;
   state->if_stmt = NULL;
}

static void
validate_loop(nir_loop *loop, validate_state *state)
{
   validate_assert(state, state->impl->structured);

   validate_assert(state, !exec_node_is_head_sentinel(loop->cf_node.node.prev));
   nir_cf_node *prev_node = nir_cf_node_prev(&loop->cf_node);
   validate_assert(state, prev_node->type == nir_cf_node_block);

   validate_assert(state, !exec_node_is_tail_sentinel(loop->cf_node.node.next));
   nir_cf_node *next_node = nir_cf_node_next(&loop->cf_node);
   validate_assert(state, next_node->type == nir_cf_node_block);

   validate_assert(state, !exec_list_is_empty(&loop->body));
   validate_assert(state, nir_block_num_preds(nir_loop_first_block(loop)) <= 2);

   nir_cf_node *old_parent = state->parent_node;
   state->parent_node = &loop->cf_node;
   nir_loop *old_loop = state->loop;
   bool old_continue_construct = state->in_loop_continue_construct;
   state->loop = loop;
   state->in_loop_continue_construct = false;

   foreach_list_typed(nir_cf_node, cf_node, node, &loop->body) {
      validate_cf_node(cf_node, state);
   }
   state->in_loop_continue_construct = true;
   foreach_list_typed(nir_cf_node, cf_node, node, &loop->continue_list) {
      validate_cf_node(cf_node, state);
   }
   state->in_loop_continue_construct = false;
   state->parent_node = old_parent;
   state->loop = old_loop;
   state->in_loop_continue_construct = old_continue_construct;
}

static void
validate_cf_node(nir_cf_node *node, validate_state *state)
{
   validate_assert(state, node->parent == state->parent_node);

   switch (node->type) {
   case nir_cf_node_block:
      validate_block(nir_cf_node_as_block(node), state);
      break;

   case nir_cf_node_if:
      validate_if(nir_cf_node_as_if(node), state);
      break;

   case nir_cf_node_loop:
      validate_loop(nir_cf_node_as_loop(node), state);
      break;

   default:
      UNREACHABLE("Invalid CF node type");
   }
}

static void
validate_constant(nir_constant *c, const struct glsl_type *type,
                  validate_state *state)
{
   if (glsl_type_is_vector_or_scalar(type)) {
      unsigned num_components = glsl_get_vector_elements(type);
      unsigned bit_size = glsl_get_bit_size(type);
      for (unsigned i = 0; i < num_components; i++)
         validate_const_value(&c->values[i], bit_size, c->is_null_constant, state);
      for (unsigned i = num_components; i < NIR_MAX_VEC_COMPONENTS; i++)
         validate_assert(state, c->values[i].u64 == 0);
   } else {
      validate_assert(state, c->num_elements == glsl_get_length(type));
      if (glsl_type_is_struct_or_ifc(type)) {
         for (unsigned i = 0; i < c->num_elements; i++) {
            const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
            validate_constant(c->elements[i], elem_type, state);
            validate_assert(state, !c->is_null_constant || c->elements[i]->is_null_constant);
         }
      } else if (glsl_type_is_array_or_matrix(type)) {
         const struct glsl_type *elem_type = glsl_get_array_element(type);
         for (unsigned i = 0; i < c->num_elements; i++) {
            validate_constant(c->elements[i], elem_type, state);
            validate_assert(state, !c->is_null_constant || c->elements[i]->is_null_constant);
         }
      } else {
         validate_assert(state, !"Invalid type for nir_constant");
      }
   }
}

static void
validate_var_decl(nir_variable *var, nir_variable_mode valid_modes,
                  validate_state *state)
{
   state->var = var;

   /* Must have exactly one mode set */
   validate_assert(state, util_is_power_of_two_nonzero(var->data.mode));
   validate_assert(state, var->data.mode & valid_modes);

   if (var->data.compact) {
      /* The "compact" flag is only valid on arrays of scalars. */
      assert(glsl_type_is_array(var->type));

      const struct glsl_type *type = glsl_get_array_element(var->type);
      if (nir_is_arrayed_io(var, state->shader->info.stage)) {
         assert(glsl_type_is_array(type));
         assert(glsl_type_is_scalar(glsl_get_array_element(type)));
      } else {
         assert(glsl_type_is_scalar(type));
      }
   }

   if (var->num_members > 0) {
      const struct glsl_type *without_array = glsl_without_array(var->type);
      validate_assert(state, glsl_type_is_struct_or_ifc(without_array));
      validate_assert(state, var->num_members == glsl_get_length(without_array));
      validate_assert(state, var->members != NULL);
   }

   if (var->data.per_view)
      validate_assert(state, glsl_type_is_array(var->type));

   if (var->constant_initializer)
      validate_constant(var->constant_initializer, var->type, state);

   if (var->data.mode == nir_var_image) {
      validate_assert(state, !var->data.bindless);
      validate_assert(state, glsl_type_is_image(glsl_without_array(var->type)));
   }

   if (var->data.per_vertex)
      validate_assert(state, state->shader->info.stage == MESA_SHADER_FRAGMENT);

   /*
    * TODO validate some things ir_validate.cpp does (requires more GLSL type
    * support)
    */

   _mesa_hash_table_insert(state->var_defs, var,
                           valid_modes == nir_var_function_temp ? state->impl : NULL);

   state->var = NULL;
}

static bool
validate_ssa_def_dominance(nir_def *def, void *_state)
{
   validate_state *state = _state;

   validate_assert(state, def->index < state->impl->ssa_alloc);
   validate_assert(state, !BITSET_TEST(state->ssa_defs_found, def->index));
   BITSET_SET(state->ssa_defs_found, def->index);

   return true;
}

static bool
validate_src_dominance(nir_src *src, void *_state)
{
   validate_state *state = _state;

   if (nir_def_block(src->ssa) == nir_src_use_instr(src)->block) {
      validate_assert(state, src->ssa->index < state->impl->ssa_alloc);
      validate_assert(state, BITSET_TEST(state->ssa_defs_found,
                                         src->ssa->index));
   } else {
      validate_assert(state, nir_block_dominates(nir_def_block(src->ssa),
                                                 nir_src_use_instr(src)->block));
   }
   return true;
}

static void
validate_ssa_dominance(nir_function_impl *impl, validate_state *state)
{
   nir_metadata_require(impl, nir_metadata_dominance);

   nir_foreach_block_unstructured(block, impl) {
      state->block = block;
      nir_foreach_instr(instr, block) {
         state->instr = instr;
         if (instr->type == nir_instr_type_phi) {
            nir_phi_instr *phi = nir_instr_as_phi(instr);
            nir_foreach_phi_src(src, phi) {
               validate_assert(state,
                               nir_block_dominates(nir_def_block(src->src.ssa),
                                                   src->pred));
            }
         } else {
            nir_foreach_src(instr, validate_src_dominance, state);
         }
         nir_foreach_def(instr, validate_ssa_def_dominance, state);
      }

      nir_if *nif = nir_block_get_following_if(block);
      if (nif) {
         validate_assert(state, nir_block_dominates(nir_def_block(nif->condition.ssa),
                                                    block));
      }
   }
}

static void
validate_block_index(nir_function_impl *impl, validate_state *state)
{
   unsigned index = 0;
   nir_foreach_block_unstructured(block, impl) {
      state->block = block;
      validate_assert(state, block->index == index);
      index++;
   }
   state->block = NULL;
   validate_assert(state, impl->num_blocks == index);
   validate_assert(state, impl->end_block->index == impl->num_blocks);
}

static void
validate_instr_index(nir_function_impl *impl, validate_state *state)
{
   int index = -1;
   nir_foreach_block(block, impl) {
      state->block = block;

      validate_assert(state, (int)block->start_ip > index);
      index = block->start_ip;

      nir_foreach_instr(instr, block) {
         state->instr = instr;
         validate_assert(state, instr->index > index);
         index = instr->index;
      }
      state->instr = NULL;

      validate_assert(state, block->end_ip > index);
      index = block->end_ip;
   }
   state->block = NULL;
}

typedef struct {
   uint32_t index;
   unsigned num_dom_children;
   struct nir_block **dom_children;
   struct set dom_frontier;
   uint32_t dom_pre_index, dom_post_index;
} block_dom_metadata;

static void
validate_dominance(nir_function_impl *impl, validate_state *state)
{
   nir_metadata valid_metadata = impl->valid_metadata;

   /* Preserve dominance */
   block_dom_metadata *blocks = ralloc_array(state->mem_ctx, block_dom_metadata,
                                             state->blocks->size);
   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_dom_metadata *md = &blocks[entry - state->blocks->table];
      md->index = block->index;
      md->num_dom_children = block->num_dom_children;
      md->dom_pre_index = block->dom_pre_index;
      md->dom_post_index = block->dom_post_index;
      md->dom_children = block->dom_children;
      md->dom_frontier = block->dom_frontier;
      memset(&block->dom_frontier, 0, sizeof(struct set));

      block->dom_children = NULL;
      _mesa_pointer_set_init(&block->dom_frontier, block);
   }

   /* Call metadata passes and compare it against the preserved metadata and call SSA dominance
    * validation. Dominance requires block indices, but we should ignore the current ones, since
    * we don't trust them.
    */
   impl->valid_metadata &= ~(nir_metadata_block_index | nir_metadata_dominance);
   nir_metadata_require(impl, nir_metadata_block_index | nir_metadata_dominance);
   assert(impl->valid_metadata == (valid_metadata | nir_metadata_block_index | nir_metadata_dominance));

   if (valid_metadata & nir_metadata_dominance) {
      set_foreach(state->blocks, entry) {
         nir_block *block = (nir_block *)entry->key;
         block_dom_metadata *md = &blocks[entry - state->blocks->table];
         state->block = (nir_block *)block;

         validate_assert(state, block->num_dom_children == md->num_dom_children);
         validate_assert(state, block->dom_pre_index == md->dom_pre_index);
         validate_assert(state, block->dom_post_index == md->dom_post_index);

         if (block->num_dom_children == md->num_dom_children && block->num_dom_children) {
            validate_assert(state, !memcmp(block->dom_children, md->dom_children,
                                           block->num_dom_children * sizeof(md->dom_children[0])));
         }

         validate_assert(state, block->dom_frontier.entries == md->dom_frontier.entries);
         set_foreach(&block->dom_frontier, entry) {
            validate_assert(state, _mesa_set_search_pre_hashed(&md->dom_frontier,
                                                               entry->hash, entry->key));
         }
      }
      state->block = NULL;
   }

   memset(state->ssa_defs_found, 0, BITSET_BYTES(impl->ssa_alloc));
   validate_ssa_dominance(impl, state);

   /* Restore the old dominance metadata */
   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_dom_metadata *md = &blocks[entry - state->blocks->table];

      if (block->dom_children != block->_dom_children_storage)
         ralloc_free(block->dom_children);
      _mesa_set_fini(&block->dom_frontier, NULL);

      block->index = md->index;
      block->num_dom_children = md->num_dom_children;
      block->dom_pre_index = md->dom_pre_index;
      block->dom_post_index = md->dom_post_index;
      block->dom_children = md->dom_children;
      block->dom_frontier = md->dom_frontier;
   }

   ralloc_free(blocks);
   impl->valid_metadata = valid_metadata;
}

typedef struct {
   struct u_sparse_bitset live_in;
   struct u_sparse_bitset live_out;
} block_liveness_metadata;

static void
validate_live_defs(nir_function_impl *impl, validate_state *state)
{
   nir_metadata valid_metadata = impl->valid_metadata;

   /* Preserve live defs */
   block_liveness_metadata *blocks = ralloc_array(state->mem_ctx, block_liveness_metadata,
                                                  state->blocks->size);
   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_liveness_metadata *md = &blocks[entry - state->blocks->table];
      md->live_in = block->live_in;
      md->live_out = block->live_out;

      u_sparse_bitset_init(&block->live_in, impl->ssa_alloc, NULL);
      u_sparse_bitset_init(&block->live_out, impl->ssa_alloc, NULL);
   }

   /* Call metadata passes and compare it against the preserved metadata */
   impl->valid_metadata &= ~nir_metadata_live_defs;
   nir_metadata_require(impl, nir_metadata_live_defs);
   assert(impl->valid_metadata == valid_metadata);

   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_liveness_metadata *md = &blocks[entry - state->blocks->table];
      state->block = (nir_block *)block;

      if (block == impl->end_block)
         continue;

      size_t bitset_words = BITSET_WORDS(impl->ssa_alloc);
      if (bitset_words) {
         validate_assert(state, !u_sparse_bitset_cmp(&md->live_in, &block->live_in));
         validate_assert(state, !u_sparse_bitset_cmp(&md->live_out, &block->live_out));
      }
   }
   state->block = NULL;

   /* Restore the old live defs metadata */
   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_liveness_metadata *md = &blocks[entry - state->blocks->table];

      u_sparse_bitset_free(&block->live_in);
      u_sparse_bitset_free(&block->live_out);

      block->live_in = md->live_in;
      block->live_out = md->live_out;
   }

   ralloc_free(blocks);
}

typedef struct {
   uint32_t index;
   bool divergent;
   bool divergent_break;
   bool divergent_continue;
   bool is_loop_header;
} block_divergence_metadata;

static void
validate_divergence(nir_function_impl *impl, validate_state *state)
{
   nir_metadata valid_metadata = impl->valid_metadata;

   /* Preserve divergence information. */
   unsigned num_blocks = impl->num_blocks;
   block_divergence_metadata *blocks = ralloc_array(state->mem_ctx,
                                                    block_divergence_metadata,
                                                    state->blocks->size);
   BITSET_WORD *ssa_divergence = BITSET_RZALLOC(state->mem_ctx,
                                                impl->ssa_alloc);
   BITSET_WORD *loop_invariance = BITSET_RZALLOC(state->mem_ctx,
                                                 impl->ssa_alloc);

   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_divergence_metadata *md = &blocks[entry - state->blocks->table];
      md->index = block->index;
      md->divergent = block->divergent;
      md->is_loop_header = false;

      if (block->cf_node.parent->type == nir_cf_node_loop &&
          nir_cf_node_is_first(&block->cf_node)) {
         md->is_loop_header = true;
         nir_loop *loop = nir_cf_node_as_loop(block->cf_node.parent);
         md->divergent_break = loop->divergent_break;
         md->divergent_continue = loop->divergent_continue;
      }

      nir_foreach_instr(instr, block) {
         nir_def *def = nir_instr_def(instr);
         if (def && def->divergent)
            BITSET_SET(ssa_divergence, def->index);
         if (def && def->loop_invariant)
            BITSET_SET(loop_invariance, def->index);
      }
   }

   /* Call metadata passes and compare it against the preserved metadata */
   impl->valid_metadata &= ~nir_metadata_divergence;
   nir_metadata_require(impl, nir_metadata_divergence);
   assert(impl->valid_metadata == (valid_metadata | nir_metadata_block_index));

   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_divergence_metadata *md = &blocks[entry - state->blocks->table];
      state->block = (nir_block *)block;

      validate_assert(state, block->divergent == md->divergent);
      if (md->is_loop_header) {
         nir_loop *loop = nir_cf_node_as_loop(block->cf_node.parent);
         validate_assert(state, loop->divergent_break == md->divergent_break);
         validate_assert(state, loop->divergent_continue == md->divergent_continue);
      }

      nir_foreach_instr(instr, block) {
         nir_def *def = nir_instr_def(instr);
         if (def) {
            state->instr = instr;
            validate_assert(state, def->divergent == BITSET_TEST(ssa_divergence, def->index));
            validate_assert(state, def->loop_invariant == BITSET_TEST(loop_invariance, def->index));
         }
      }
      state->instr = NULL;
   }
   state->block = NULL;

   /* Restore the old divergence metadata */
   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      block_divergence_metadata *md = &blocks[entry - state->blocks->table];
      block->index = md->index;
      block->divergent = md->divergent;

      if (md->is_loop_header) {
         nir_loop *loop = nir_cf_node_as_loop(block->cf_node.parent);
         loop->divergent_break = md->divergent_break;
         loop->divergent_continue = md->divergent_continue;
      }

      nir_foreach_instr(instr, block) {
         nir_def *def = nir_instr_def(instr);
         if (def) {
            def->divergent = BITSET_TEST(ssa_divergence, def->index);
            def->loop_invariant = BITSET_TEST(loop_invariance, def->index);
         }
      }
   }
   impl->num_blocks = num_blocks;

   ralloc_free(blocks);
   ralloc_free(ssa_divergence);
   ralloc_free(loop_invariance);
   impl->valid_metadata = valid_metadata;
}

static bool
are_loop_terminators_equal(const nir_loop_terminator *a, const nir_loop_terminator *b)
{
   if (!a || !b)
      return !a && !b;

   return a->nif == b->nif &&
          a->conditional_instr == b->conditional_instr &&
          a->break_block == b->break_block &&
          a->continue_from_block == b->continue_from_block &&
          a->continue_from_then == b->continue_from_then &&
          a->induction_rhs == b->induction_rhs &&
          a->exact_trip_count_unknown == b->exact_trip_count_unknown;
}

static void
validate_loop_info(nir_function_impl *impl, validate_state *state)
{
   nir_metadata valid_metadata = impl->valid_metadata;

   /* Preserve loop info */
   struct hash_table *loops = _mesa_pointer_hash_table_create(state->mem_ctx);
   set_foreach(state->blocks, entry) {
      nir_block *block = (nir_block *)entry->key;
      if (block->cf_node.parent->type == nir_cf_node_loop &&
          nir_cf_node_is_first(&block->cf_node)) {
         nir_loop *loop = nir_cf_node_as_loop(block->cf_node.parent);
         _mesa_hash_table_insert(loops, loop, loop->info);
         loop->info = NULL;
      }
   }

   /* Call metadata passes and compare it against the preserved metadata */
   impl->valid_metadata &= ~nir_metadata_loop_analysis;
   nir_metadata_require(impl, nir_metadata_loop_analysis, impl->loop_analysis_indirect_mask,
                        impl->loop_analysis_force_unroll_sampler_indirect);
   assert(impl->valid_metadata == valid_metadata);

   hash_table_foreach(loops, entry) {
      const nir_loop *loop = entry->key;
      const nir_loop_info *md = entry->data;

      validate_assert(state, loop->info->instr_cost == md->instr_cost);
      validate_assert(state, loop->info->has_soft_fp64 == md->has_soft_fp64);
      validate_assert(state, loop->info->guessed_trip_count == md->guessed_trip_count);
      validate_assert(state, loop->info->max_trip_count == md->max_trip_count);
      validate_assert(state, loop->info->exact_trip_count_known == md->exact_trip_count_known);
      validate_assert(state, loop->info->force_unroll == md->force_unroll);
      validate_assert(state, loop->info->complex_loop == md->complex_loop);

      validate_assert(state, are_loop_terminators_equal(loop->info->limiting_terminator,
                                                        md->limiting_terminator));

      validate_assert(state, list_length(&loop->info->loop_terminator_list) ==
                                list_length(&md->loop_terminator_list));
      list_pair_for_each_entry(nir_loop_terminator, a, b, &loop->info->loop_terminator_list,
                               &md->loop_terminator_list, loop_terminator_link) {
         validate_assert(state, are_loop_terminators_equal(a, b));
      }

      validate_assert(state, _mesa_hash_table_num_entries(loop->info->induction_vars) ==
                                _mesa_hash_table_num_entries(md->induction_vars));
      hash_table_foreach(loop->info->induction_vars, var) {
         struct hash_entry *prev_var = _mesa_hash_table_search(md->induction_vars, var->key);
         validate_assert(state, prev_var != NULL);
         if (prev_var) {
            nir_loop_induction_variable *a = var->data;
            nir_loop_induction_variable *b = prev_var->data;
            validate_assert(state, a->basis == b->basis);
            validate_assert(state, a->def == b->def);
            validate_assert(state, a->init_src == b->init_src);
            validate_assert(state, a->update_src == b->update_src);
         }
      }
   }

   /* Restore the old loop info */
   hash_table_foreach(loops, entry) {
      nir_loop *loop = (nir_loop *)entry->key;
      nir_loop_info *md = entry->data;

      ralloc_free(loop->info);
      loop->info = md;
   }
}

static void
validate_metadata_and_ssa_dominance(nir_function_impl *impl, validate_state *state)
{
   /* We should preserve and restore metadata when necessary, so that passes do not accidentally
    * depend on nir_validate_shader().
    */

   if (impl->valid_metadata & nir_metadata_block_index)
      validate_block_index(impl, state);

   if (impl->valid_metadata & nir_metadata_instr_index)
      validate_instr_index(impl, state);

   /* This validates both metadata and SSA dominance. */
   validate_dominance(impl, state);

   if (impl->valid_metadata & nir_metadata_live_defs)
      validate_live_defs(impl, state);

   if (!impl->structured)
      return;

   if (impl->valid_metadata & nir_metadata_divergence)
      validate_divergence(impl, state);

   if (impl->valid_metadata & nir_metadata_loop_analysis)
      validate_loop_info(impl, state);
}

static void
validate_function_impl(nir_function_impl *impl, validate_state *state)
{
   validate_assert(state, impl->function->impl == impl);
   validate_assert(state, impl->cf_node.parent == NULL);

   if (impl->preamble) {
      validate_assert(state, impl->function->is_entrypoint);
      validate_assert(state, impl->preamble->is_preamble);
   }

   validate_assert(state, exec_list_is_empty(&impl->end_block->instr_list));
   validate_assert(state, impl->end_block->successors[0] == NULL);
   validate_assert(state, impl->end_block->successors[1] == NULL);

   state->impl = impl;
   state->parent_node = &impl->cf_node;

   exec_list_validate(&impl->locals);
   nir_foreach_function_temp_variable(var, impl) {
      validate_var_decl(var, nir_var_function_temp, state);
   }

   state->ssa_defs_found = reralloc(state->mem_ctx, state->ssa_defs_found,
                                    BITSET_WORD, BITSET_WORDS(impl->ssa_alloc));
   memset(state->ssa_defs_found, 0, BITSET_BYTES(impl->ssa_alloc));

   _mesa_set_clear(state->blocks, NULL);
   _mesa_set_resize(state->blocks, impl->num_blocks);
   if (impl->structured)
      collect_blocks(&impl->body, state);
   else
      collect_unstructured_blocks(impl, state);
   _mesa_set_add(state->blocks, impl->end_block);
   validate_assert(state, !exec_list_is_empty(&impl->body));
   foreach_list_typed(nir_cf_node, node, node, &impl->body) {
      validate_cf_node(node, state);
   }
   validate_end_block(impl->end_block, state);

   /* We must have seen every source by now. This also means that we've untagged
    * every source, so we have valid (unaugmented) NIR once again.
    */
   validate_assert(state, state->nr_tagged_srcs == 0);

   /* Metadata validation assumes a valid NIR shader. */
   if (_mesa_hash_table_num_entries(state->errors) == 0)
      validate_metadata_and_ssa_dominance(impl, state);
}

static void
validate_function(nir_function *func, validate_state *state)
{
   if (func->impl != NULL) {
      validate_assert(state, func->impl->function == func);
      validate_function_impl(func->impl, state);
   }
}

static void
validate_float_mul_add(nir_float_muladd_support muladd_support, validate_state *state)
{
   if (muladd_support & nir_float_muladd_support_fuse) {
      validate_assert(state, muladd_support &
         (nir_float_muladd_support_has_ffma | nir_float_muladd_support_has_fmad));
   }

   if (muladd_support & nir_float_muladd_support_prefers_split)
      validate_assert(state, muladd_support & nir_float_muladd_support_has_ffma);
}

static void
validate_options(const nir_shader_compiler_options *options, validate_state *state)
{
   if (!options)
      return;

   validate_float_mul_add(options->float_mul_add16, state);
   validate_float_mul_add(options->float_mul_add32, state);
   validate_float_mul_add(options->float_mul_add64, state);
}

static void
init_validate_state(validate_state *state)
{
   state->mem_ctx = ralloc_context(NULL);
   state->ssa_defs_found = NULL;
   state->blocks = _mesa_pointer_set_create(state->mem_ctx);
   state->var_defs = _mesa_pointer_hash_table_create(state->mem_ctx);
   state->errors = _mesa_pointer_hash_table_create(state->mem_ctx);
   state->nr_tagged_srcs = 0;

   state->loop = NULL;
   state->in_loop_continue_construct = false;
   state->instr = NULL;
   state->var = NULL;
}

static void
destroy_validate_state(validate_state *state)
{
   ralloc_free(state->mem_ctx);
}

simple_mtx_t fail_dump_mutex = SIMPLE_MTX_INITIALIZER;

static void
dump_errors(validate_state *state, const char *when)
{
   struct hash_table *errors = state->errors;

   /* Lock around dumping so that we get clean dumps in a multi-threaded
    * scenario
    */
   simple_mtx_lock(&fail_dump_mutex);

   if (when) {
      fprintf(stderr, "NIR validation failed %s\n", when);
      fprintf(stderr, "%d errors:\n", _mesa_hash_table_num_entries(errors));
   } else {
      fprintf(stderr, "NIR validation failed with %d errors:\n",
              _mesa_hash_table_num_entries(errors));
   }

   nir_print_shader_annotated(state->shader, stderr, errors);

   if (_mesa_hash_table_num_entries(errors) > 0) {
      fprintf(stderr, "%d additional errors:\n",
              _mesa_hash_table_num_entries(errors));
      hash_table_foreach(errors, entry) {
         fprintf(stderr, "%s\n", (char *)entry->data);
      }
   }
   fflush(stderr);

   simple_mtx_unlock(&fail_dump_mutex);

   abort();
}

void
nir_validate_shader(nir_shader *shader, const char *when)
{
   if (NIR_DEBUG(NOVALIDATE))
      return;

   validate_state state;
   init_validate_state(&state);

   state.shader = shader;

   nir_variable_mode valid_modes =
      nir_var_shader_in |
      nir_var_shader_out |
      nir_var_shader_temp |
      nir_var_uniform |
      nir_var_mem_ubo |
      nir_var_system_value |
      nir_var_mem_ssbo |
      nir_var_mem_shared |
      nir_var_mem_global |
      nir_var_mem_push_const |
      nir_var_mem_constant |
      nir_var_mem_pixel_local_in |
      nir_var_mem_pixel_local_out |
      nir_var_mem_pixel_local_inout |
      nir_var_image |
      nir_var_resource_heap |
      nir_var_sampler_heap;

   if (mesa_shader_stage_is_callable(shader->info.stage))
      valid_modes |= nir_var_shader_call_data;

   if (shader->info.stage == MESA_SHADER_ANY_HIT ||
       shader->info.stage == MESA_SHADER_CLOSEST_HIT ||
       shader->info.stage == MESA_SHADER_INTERSECTION)
      valid_modes |= nir_var_ray_hit_attrib;

   if (shader->info.stage == MESA_SHADER_TASK ||
       shader->info.stage == MESA_SHADER_MESH)
      valid_modes |= nir_var_mem_task_payload;

   if (shader->info.stage == MESA_SHADER_COMPUTE)
      valid_modes |= nir_var_mem_node_payload |
                     nir_var_mem_node_payload_in;

   validate_options(shader->options, &state);

   exec_list_validate(&shader->variables);
   nir_foreach_variable_in_shader(var, shader)
      validate_var_decl(var, valid_modes, &state);

   exec_list_validate(&shader->functions);
   foreach_list_typed(nir_function, func, node, &shader->functions) {
      validate_function(func, &state);
   }

   if (shader->xfb_info != NULL) {
      /* At least validate that, if nir_shader::xfb_info exists, the shader
       * has real transform feedback going on.
       */
      validate_assert(&state, shader->info.stage == MESA_SHADER_VERTEX ||
                                 shader->info.stage == MESA_SHADER_TESS_EVAL ||
                                 shader->info.stage == MESA_SHADER_GEOMETRY);
      validate_assert(&state, shader->xfb_info->buffers_written != 0);
      validate_assert(&state, shader->xfb_info->streams_written != 0);
      validate_assert(&state, shader->xfb_info->output_count > 0);
   }

   validate_assert(&state,
                   util_is_power_of_two_or_zero(shader->info.api_subgroup_size));
   validate_assert(&state,
                   util_is_power_of_two_nonzero(shader->info.max_subgroup_size));
   validate_assert(&state,
                   util_is_power_of_two_nonzero(shader->info.min_subgroup_size));
   validate_assert(&state, shader->info.max_subgroup_size <= 128);
   validate_assert(&state, shader->info.min_subgroup_size <= 128);
   validate_assert(&state, shader->info.api_subgroup_size == 0 ||
                              shader->info.api_subgroup_size >= shader->info.max_subgroup_size);
   validate_assert(&state,
                   shader->info.min_subgroup_size <= shader->info.max_subgroup_size);

   if (_mesa_hash_table_num_entries(state.errors) > 0)
      dump_errors(&state, when);

   destroy_validate_state(&state);
}

void
nir_validate_ssa_dominance(nir_shader *shader, const char *when)
{
   if (NIR_DEBUG(NOVALIDATE))
      return;

   validate_state state;
   init_validate_state(&state);

   state.shader = shader;

   nir_foreach_function_impl(impl, shader) {
      state.ssa_defs_found = reralloc(state.mem_ctx, state.ssa_defs_found,
                                      BITSET_WORD,
                                      BITSET_WORDS(impl->ssa_alloc));
      memset(state.ssa_defs_found, 0, BITSET_BYTES(impl->ssa_alloc));

      state.impl = impl;
      validate_ssa_dominance(impl, &state);
   }

   if (_mesa_hash_table_num_entries(state.errors) > 0)
      dump_errors(&state, when);

   destroy_validate_state(&state);
}

void
_nir_assert_no_progress(bool progress, const char *when)
{
   if (!progress)
      return;

   /* Lock around dumping so that we get clean dumps in a multi-threaded
    * scenario.
    */
   simple_mtx_lock(&fail_dump_mutex);

   fprintf(stderr, "NIR assertion failed: Expected no progress from %s.\n",
           when);
   fflush(stderr);

   simple_mtx_unlock(&fail_dump_mutex);
   abort();
}

#endif /* NDEBUG */
