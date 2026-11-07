/*
 * Copyright © 2014 Connor Abbott
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

#include "nir_instr_set.h"
#include "util/half_float.h"
#include "nir.h"
#include "nir_vla.h"

/* glsl-optimizer change: no xxhash.h; use the hash_table.h data hash */
#include "util/hash_table.h"
#define XXH32(data, size, seed) _mesa_hash_data_with_seed((data), (size), (seed))

/* This function determines if uses of an instruction can safely be rewritten
 * to use another identical instruction instead. Note that this function must
 * be kept in sync with hash_instr() and nir_instrs_equal() -- only
 * instructions that pass this test will be handed on to those functions, and
 * conversely they must handle everything that this function returns true for.
 */
static bool
instr_can_rewrite(const nir_instr *instr)
{
   switch (instr->type) {
   case nir_instr_type_alu:
   case nir_instr_type_deref:
   case nir_instr_type_tex:
   case nir_instr_type_load_const:
   case nir_instr_type_phi:
      return true;
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
      switch (intr->intrinsic) {
      case nir_intrinsic_ddx:
      case nir_intrinsic_ddx_fine:
      case nir_intrinsic_ddx_coarse:
      case nir_intrinsic_ddy:
      case nir_intrinsic_ddy_fine:
      case nir_intrinsic_ddy_coarse:
         /* Derivatives are not CAN_REORDER, because we cannot move derivatives
          * across terminates if that would lose helper invocations. However,
          * they can be CSE'd as a special case - if it is legal to execute a
          * derivative at instruction A, then it is also legal to execute the
          * derivative from instruction B. So we can hoist up the derivatives as
          * CSE is inclined to without a problem.
          */
         return true;
      case nir_intrinsic_terminate:
      case nir_intrinsic_terminate_if:
      case nir_intrinsic_demote:
      case nir_intrinsic_demote_if:
         /* If a terminate/demote dominates another with the same source,
          * the second won't affect additional invocations.
          */
         return true;
      default:
         return nir_intrinsic_can_reorder(intr);
      }
   }
   case nir_instr_type_call:
   case nir_instr_type_cmat_call:
   case nir_instr_type_jump:
   case nir_instr_type_undef:
      return false;
   default:
      UNREACHABLE("Invalid instruction type");
   }

   return false;
}

#define HASH(hash, data) XXH32(&(data), sizeof(data), hash)

static uint32_t
hash_src(uint32_t hash, const nir_src *src)
{
   hash = HASH(hash, src->ssa);
   return hash;
}

static uint32_t
hash_alu_src(uint32_t hash, const nir_alu_src *src, unsigned num_components)
{
   for (unsigned i = 0; i < num_components; i++)
      hash = HASH(hash, src->swizzle[i]);

   hash = hash_src(hash, &src->src);
   return hash;
}

static uint32_t
hash_alu(uint32_t hash, const nir_alu_instr *instr)
{
   /* We explicitly don't hash instr->fp_math_ctrl. */
   uint8_t flags = instr->no_signed_wrap |
                   instr->no_unsigned_wrap << 1;
   uint8_t v[8];
   v[0] = flags;
   v[1] = instr->def.num_components;
   v[2] = instr->def.bit_size;
   v[3] = 0;
   uint32_t op = instr->op;
   memcpy(v + 4, &op, sizeof(op));
   hash = XXH32(v, sizeof(v), hash);

   if (nir_op_infos[instr->op].algebraic_properties & NIR_OP_IS_2SRC_COMMUTATIVE) {
      assert(nir_op_infos[instr->op].num_inputs >= 2);

      uint32_t hash0 = hash_alu_src(hash, &instr->src[0],
                                    nir_ssa_alu_instr_src_components(instr, 0));
      uint32_t hash1 = hash_alu_src(hash, &instr->src[1],
                                    nir_ssa_alu_instr_src_components(instr, 1));
      /* For commutative operations, we need some commutative way of
       * combining the hashes.  One option would be to XOR them but that
       * means that anything with two identical sources will hash to 0 and
       * that's common enough we probably don't want the guaranteed
       * collision.  Either addition or multiplication will also work.
       */
      hash = hash0 * hash1;

      for (unsigned i = 2; i < nir_op_infos[instr->op].num_inputs; i++) {
         hash = hash_alu_src(hash, &instr->src[i],
                             nir_ssa_alu_instr_src_components(instr, i));
      }
   } else {
      for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
         hash = hash_alu_src(hash, &instr->src[i],
                             nir_ssa_alu_instr_src_components(instr, i));
      }
   }

   return hash;
}

static uint32_t
hash_deref(uint32_t hash, const nir_deref_instr *instr)
{
   uint32_t v[4];
   v[0] = instr->deref_type;
   v[1] = instr->modes;
   uint64_t type = (uintptr_t)instr->type;
   memcpy(v + 2, &type, sizeof(type));
   hash = XXH32(v, sizeof(v), hash);

   if (instr->deref_type == nir_deref_type_var)
      return HASH(hash, instr->var);

   hash = hash_src(hash, &instr->parent);

   switch (instr->deref_type) {
   case nir_deref_type_struct:
      hash = HASH(hash, instr->strct.index);
      break;

   case nir_deref_type_array:
   case nir_deref_type_ptr_as_array:
      hash = hash_src(hash, &instr->arr.index);
      hash = HASH(hash, instr->arr.in_bounds);
      break;

   case nir_deref_type_cast:
      hash = HASH(hash, instr->cast.ptr_stride);
      hash = HASH(hash, instr->cast.align_mul);
      hash = HASH(hash, instr->cast.align_offset);
      break;

   case nir_deref_type_var:
   case nir_deref_type_array_wildcard:
      /* Nothing to do */
      break;

   default:
      UNREACHABLE("Invalid instruction deref type");
   }

   return hash;
}

static uint32_t
hash_load_const(uint32_t hash, const nir_load_const_instr *instr)
{
   hash = HASH(hash, instr->def.num_components);

   if (instr->def.bit_size == 1) {
      for (unsigned i = 0; i < instr->def.num_components; i++) {
         uint8_t b = instr->value[i].b;
         hash = HASH(hash, b);
      }
   } else {
      unsigned size = instr->def.num_components * sizeof(*instr->value);
      hash = XXH32(instr->value, size, hash);
   }

   return hash;
}

static int
cmp_phi_src(const void *data1, const void *data2)
{
   nir_phi_src *src1 = *(nir_phi_src **)data1;
   nir_phi_src *src2 = *(nir_phi_src **)data2;
   return src1->pred > src2->pred ? 1 : (src1->pred == src2->pred ? 0 : -1);
}

static uint32_t
hash_phi(uint32_t hash, const nir_phi_instr *instr)
{
   hash = HASH(hash, instr->instr.block);

   /* Similar to hash_alu(), combine the hashes commutatively. */
   nir_foreach_phi_src(src, instr)
      hash *= HASH(hash_src(0, &src->src), src->pred);

   return hash;
}

/* Returns either the original indices or the provided buffer, if some
 * change was necessary.
 */
static const int *
normalized_intrinsic_const_indices(const nir_intrinsic_instr *instr,
                                   int const_index[NIR_INTRINSIC_MAX_CONST_INDEX])
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];

   if (!nir_intrinsic_has_fp_math_ctrl(instr) &&
       !nir_intrinsic_has_io_semantics(instr))
      return instr->const_index;

   memcpy(const_index, instr->const_index,
          info->num_index_slots * sizeof(instr->const_index[0]));

   /* Keep this in sync with nir_instr_set_add_or_rewrite(): these bits are
    * merged into the rewritten instruction instead of preventing equality.
    */
   if (nir_intrinsic_has_fp_math_ctrl(instr)) {
      unsigned offset = info->index_map[NIR_INTRINSIC_FP_MATH_CTRL] - 1;
      const_index[offset] = 0;
   } else if (nir_intrinsic_has_io_semantics(instr)) {
      unsigned offset = info->index_map[NIR_INTRINSIC_IO_SEMANTICS] - 1;
      nir_io_semantics sem = nir_intrinsic_io_semantics(instr);
      sem.no_signed_zero = false;
      memcpy(&const_index[offset], &sem, sizeof(sem));
   }

   return const_index;
}

static uint32_t
hash_intrinsic(uint32_t hash, const nir_intrinsic_instr *instr)
{
   const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   hash = HASH(hash, instr->intrinsic);

   if (info->has_dest) {
      uint8_t v[4] = { instr->def.num_components, instr->def.bit_size, 0, 0 };
      hash = XXH32(v, sizeof(v), hash);
   }

   int const_index[NIR_INTRINSIC_MAX_CONST_INDEX];
   const int *normalized =
      normalized_intrinsic_const_indices(instr, const_index);
   hash = XXH32(normalized, info->num_index_slots * sizeof(normalized[0]), hash);

   for (unsigned i = 0; i < nir_intrinsic_infos[instr->intrinsic].num_srcs; i++)
      hash = hash_src(hash, &instr->src[i]);

   return hash;
}

/* Gathers all the small bits of nir_instr_tex as a uint32_t */
static uint32_t
pack_tex(const nir_tex_instr *instr)
{
   uint32_t packed = 0, bit = 0;

#define PACK(_val, _bits) do { \
   uint32_t u = (_val), bits = (_bits); \
   assert(bit + bits <= 32); \
   assert(u <= BITFIELD_MASK(bits)); \
   packed |= u << bit; \
   bit += bits; \
} while (0)

   PACK(instr->op, 6);
   PACK(instr->num_srcs, 5);
   PACK(instr->sampler_dim, 4);
   PACK(instr->coord_components, 3);
   PACK(instr->is_array, 1);
   PACK(instr->is_shadow, 1);
   PACK(instr->is_new_style_shadow, 1);
   PACK(instr->is_sparse, 1);
   PACK(instr->component, 2);
   PACK(instr->array_is_lowered_cube, 1);
   PACK(instr->is_gather_implicit_lod, 1);
   PACK(instr->skip_helpers, 1);
   PACK(instr->texture_non_uniform, 1);
   PACK(instr->sampler_non_uniform, 1);
   PACK(instr->embedded_sampler, 1);
   PACK(instr->offset_non_uniform, 1);

#undef PACK

   return packed;
}

static uint32_t
hash_tex(uint32_t hash, const nir_tex_instr *instr)
{
   uint8_t v[28];
   uint32_t packed = pack_tex(instr);
   memcpy(v, &packed, 4);
   STATIC_ASSERT(sizeof(instr->tg4_offsets) == 8);
   memcpy(v + 4, instr->tg4_offsets, 8);
   uint32_t texture_index = instr->texture_index;
   uint32_t sampler_index = instr->sampler_index;
   uint32_t backend_flags = instr->backend_flags;
   uint32_t dest = (instr->def.num_components << 8) | instr->dest_type;
   memcpy(v + 12, &texture_index, 4);
   memcpy(v + 16, &sampler_index, 4);
   memcpy(v + 20, &backend_flags, 4);
   memcpy(v + 24, &dest, 4);
   hash = XXH32(v, sizeof(v), hash);

   for (unsigned i = 0; i < instr->num_srcs; i++)
      hash *= hash_src(0, &instr->src[i].src);

   return hash;
}

/* Computes a hash of an instruction for use in a hash table. Note that this
 * will only work for instructions where instr_can_rewrite() returns true, and
 * it should return identical hashes for two instructions that are the same
 * according nir_instrs_equal().
 */

static uint32_t
hash_instr(const void *data)
{
   const nir_instr *instr = data;
   uint32_t hash = 0;

   switch (instr->type) {
   case nir_instr_type_alu:
      hash = hash_alu(hash, nir_instr_as_alu(instr));
      break;
   case nir_instr_type_deref:
      hash = hash_deref(hash, nir_instr_as_deref(instr));
      break;
   case nir_instr_type_load_const:
      hash = hash_load_const(hash, nir_instr_as_load_const(instr));
      break;
   case nir_instr_type_phi:
      hash = hash_phi(hash, nir_instr_as_phi(instr));
      break;
   case nir_instr_type_intrinsic:
      hash = hash_intrinsic(hash, nir_instr_as_intrinsic(instr));
      break;
   case nir_instr_type_tex:
      hash = hash_tex(hash, nir_instr_as_tex(instr));
      break;
   default:
      UNREACHABLE("Invalid instruction type");
   }

   return hash;
}

bool
nir_srcs_equal(nir_src src1, nir_src src2)
{
   return src1.ssa == src2.ssa;
}

/**
 * If the \p s is an SSA value that was generated by a negation instruction,
 * that instruction is returned as a \c nir_alu_instr.  Otherwise \c NULL is
 * returned.
 */
static nir_alu_instr *
get_neg_instr(nir_src s, nir_alu_type base_type)
{
   nir_alu_instr *alu = nir_src_as_alu(s);

   return alu != NULL && (alu->op == (base_type == nir_type_float ? nir_op_fneg : nir_op_ineg))
             ? alu
             : NULL;
}

bool
nir_const_value_negative_equal(nir_const_value c1,
                               nir_const_value c2,
                               nir_alu_type full_type)
{
   assert(nir_alu_type_get_base_type(full_type) != nir_type_invalid);
   assert(nir_alu_type_get_type_size(full_type) != 0);

   switch (full_type) {
   case nir_type_float16:
      return _mesa_half_to_float(c1.u16) == -_mesa_half_to_float(c2.u16);

   case nir_type_float32:
      return c1.f32 == -c2.f32;

   case nir_type_float64:
      return c1.f64 == -c2.f64;

   case nir_type_int8:
   case nir_type_uint8:
      return c1.i8 == -c2.i8;

   case nir_type_int16:
   case nir_type_uint16:
      return c1.i16 == -c2.i16;

   case nir_type_int32:
   case nir_type_uint32:
      return c1.i32 == -c2.i32;

   case nir_type_int64:
   case nir_type_uint64:
      return c1.i64 == -c2.i64;

   default:
      break;
   }

   return false;
}

bool
nir_alu_srcs_negative_equal_typed(const nir_alu_instr *alu1,
                                  const nir_alu_instr *alu2,
                                  unsigned src1, unsigned src2,
                                  nir_alu_type base_type)
{
#ifndef NDEBUG
   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      assert(nir_alu_instr_channel_used(alu1, src1, i) ==
             nir_alu_instr_channel_used(alu2, src2, i));
   }
#endif

   /* Handling load_const instructions is tricky. */

   const nir_const_value *const const1 =
      nir_src_as_const_value(alu1->src[src1].src);

   if (const1 != NULL) {
      const nir_const_value *const const2 =
         nir_src_as_const_value(alu2->src[src2].src);

      if (const2 == NULL)
         return false;

      if (nir_src_bit_size(alu1->src[src1].src) !=
          nir_src_bit_size(alu2->src[src2].src))
         return false;

      const nir_alu_type full_type = base_type | nir_src_bit_size(alu1->src[src1].src);
      for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
         if (nir_alu_instr_channel_used(alu1, src1, i) &&
             !nir_const_value_negative_equal(const1[alu1->src[src1].swizzle[i]],
                                             const2[alu2->src[src2].swizzle[i]],
                                             full_type))
            return false;
      }

      return true;
   }

   uint8_t alu1_swizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   nir_src alu1_actual_src;
   nir_alu_instr *neg1 = get_neg_instr(alu1->src[src1].src, base_type);
   bool parity = false;

   if (neg1) {
      parity = !parity;
      alu1_actual_src = neg1->src[0].src;

      for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(neg1, 0); i++)
         alu1_swizzle[i] = neg1->src[0].swizzle[i];
   } else {
      alu1_actual_src = alu1->src[src1].src;

      for (unsigned i = 0; i < nir_src_num_components(alu1_actual_src); i++)
         alu1_swizzle[i] = i;
   }

   uint8_t alu2_swizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   nir_src alu2_actual_src;
   nir_alu_instr *neg2 = get_neg_instr(alu2->src[src2].src, base_type);

   if (neg2) {
      parity = !parity;
      alu2_actual_src = neg2->src[0].src;

      for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(neg2, 0); i++)
         alu2_swizzle[i] = neg2->src[0].swizzle[i];
   } else {
      alu2_actual_src = alu2->src[src2].src;

      for (unsigned i = 0; i < nir_src_num_components(alu2_actual_src); i++)
         alu2_swizzle[i] = i;
   }

   /* Bail early if sources are not equal or we don't have parity. */
   if (!parity || !nir_srcs_equal(alu1_actual_src, alu2_actual_src))
      return false;

   for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(alu1, src1); i++) {
      if (alu1_swizzle[alu1->src[src1].swizzle[i]] !=
          alu2_swizzle[alu2->src[src2].swizzle[i]])
         return false;
   }

   return true;
}

/**
 * Shallow compare of ALU srcs to determine if one is the negation of the other
 *
 * This function detects cases where \p alu1 is a constant and \p alu2 is a
 * constant that is its negation.  It will also detect cases where \p alu2 is
 * an SSA value that is a \c nir_op_fneg applied to \p alu1 (and vice versa).
 *
 * This function does not detect the general case when \p alu1 and \p alu2 are
 * SSA values that are the negations of each other (e.g., \p alu1 represents
 * (a * b) and \p alu2 represents (-a * b)).
 *
 * \warning
 * It is the responsibility of the caller to ensure that the component counts,
 * write masks, and base types of the sources being compared are compatible.
 */
bool
nir_alu_srcs_negative_equal(const nir_alu_instr *alu1,
                            const nir_alu_instr *alu2,
                            unsigned src1, unsigned src2)
{

#ifndef NDEBUG
   if (nir_alu_type_get_base_type(nir_op_infos[alu1->op].input_types[src1]) == nir_type_float) {
      assert(nir_op_infos[alu1->op].input_types[src1] ==
             nir_op_infos[alu2->op].input_types[src2]);
   } else {
      assert(nir_op_infos[alu1->op].input_types[src1] == nir_type_int);
      assert(nir_op_infos[alu2->op].input_types[src2] == nir_type_int);
   }
#endif

   nir_alu_type type = nir_op_infos[alu1->op].input_types[src1];
   return nir_alu_srcs_negative_equal_typed(alu1, alu2, src1, src2, type);
}

bool
nir_alu_srcs_equal(const nir_alu_instr *alu1, const nir_alu_instr *alu2,
                   unsigned src1, unsigned src2)
{
   for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(alu1, src1); i++) {
      if (alu1->src[src1].swizzle[i] != alu2->src[src2].swizzle[i])
         return false;
   }

   return nir_srcs_equal(alu1->src[src1].src, alu2->src[src2].src);
}

/* Returns "true" if two instructions are equal. Note that this will only
 * work for the subset of instructions defined by instr_can_rewrite(). Also,
 * it should only return "true" for instructions that hash_instr() will return
 * the same hash for (ignoring collisions, of course).
 */

bool
nir_instrs_equal(const nir_instr *instr1, const nir_instr *instr2)
{
   assert(instr_can_rewrite(instr1) && instr_can_rewrite(instr2));

   if (instr1->type != instr2->type)
      return false;

   switch (instr1->type) {
   case nir_instr_type_alu: {
      nir_alu_instr *alu1 = nir_instr_as_alu(instr1);
      nir_alu_instr *alu2 = nir_instr_as_alu(instr2);

      if (alu1->op != alu2->op)
         return false;

      /* We explicitly don't compare instr->fp_math_ctrl. */

      if (alu1->no_signed_wrap != alu2->no_signed_wrap)
         return false;

      if (alu1->no_unsigned_wrap != alu2->no_unsigned_wrap)
         return false;

      /* TODO: We can probably acutally do something more inteligent such
       * as allowing different numbers and taking a maximum or something
       * here */
      if (alu1->def.num_components != alu2->def.num_components)
         return false;

      if (alu1->def.bit_size != alu2->def.bit_size)
         return false;

      if (nir_op_infos[alu1->op].algebraic_properties & NIR_OP_IS_2SRC_COMMUTATIVE) {
         if ((!nir_alu_srcs_equal(alu1, alu2, 0, 0) ||
              !nir_alu_srcs_equal(alu1, alu2, 1, 1)) &&
             (!nir_alu_srcs_equal(alu1, alu2, 0, 1) ||
              !nir_alu_srcs_equal(alu1, alu2, 1, 0)))
            return false;

         for (unsigned i = 2; i < nir_op_infos[alu1->op].num_inputs; i++) {
            if (!nir_alu_srcs_equal(alu1, alu2, i, i))
               return false;
         }
      } else {
         for (unsigned i = 0; i < nir_op_infos[alu1->op].num_inputs; i++) {
            if (!nir_alu_srcs_equal(alu1, alu2, i, i))
               return false;
         }
      }
      return true;
   }
   case nir_instr_type_deref: {
      nir_deref_instr *deref1 = nir_instr_as_deref(instr1);
      nir_deref_instr *deref2 = nir_instr_as_deref(instr2);

      if (deref1->deref_type != deref2->deref_type ||
          deref1->modes != deref2->modes ||
          deref1->type != deref2->type)
         return false;

      if (deref1->deref_type == nir_deref_type_var)
         return deref1->var == deref2->var;

      if (!nir_srcs_equal(deref1->parent, deref2->parent))
         return false;

      switch (deref1->deref_type) {
      case nir_deref_type_struct:
         if (deref1->strct.index != deref2->strct.index)
            return false;
         break;

      case nir_deref_type_array:
      case nir_deref_type_ptr_as_array:
         if (!nir_srcs_equal(deref1->arr.index, deref2->arr.index))
            return false;
         if (deref1->arr.in_bounds != deref2->arr.in_bounds)
            return false;
         break;

      case nir_deref_type_cast:
         if (deref1->cast.ptr_stride != deref2->cast.ptr_stride ||
             deref1->cast.align_mul != deref2->cast.align_mul ||
             deref1->cast.align_offset != deref2->cast.align_offset)
            return false;
         break;

      case nir_deref_type_var:
      case nir_deref_type_array_wildcard:
         /* Nothing to do */
         break;

      default:
         UNREACHABLE("Invalid instruction deref type");
      }
      return true;
   }
   case nir_instr_type_tex: {
      nir_tex_instr *tex1 = nir_instr_as_tex(instr1);
      nir_tex_instr *tex2 = nir_instr_as_tex(instr2);

      /* This covers the opcode and num_srcs */
      if (pack_tex(tex1) != pack_tex(tex2))
         return false;

      assert(tex1->num_srcs == tex2->num_srcs);
      for (unsigned i = 0; i < tex1->num_srcs; i++) {
         if (tex1->src[i].src_type != tex2->src[i].src_type ||
             !nir_srcs_equal(tex1->src[i].src, tex2->src[i].src)) {
            return false;
         }
      }

      if (memcmp(tex1->tg4_offsets, tex2->tg4_offsets,
                 sizeof(tex1->tg4_offsets)))
         return false;

      if (tex1->texture_index != tex2->texture_index ||
          tex1->sampler_index != tex2->sampler_index ||
          tex1->backend_flags != tex2->backend_flags ||
          tex1->def.num_components != tex2->def.num_components ||
          tex1->dest_type != tex2->dest_type)
         return false;

      return true;
   }
   case nir_instr_type_load_const: {
      nir_load_const_instr *load1 = nir_instr_as_load_const(instr1);
      nir_load_const_instr *load2 = nir_instr_as_load_const(instr2);

      if (load1->def.num_components != load2->def.num_components)
         return false;

      if (load1->def.bit_size != load2->def.bit_size)
         return false;

      if (load1->def.bit_size == 1) {
         for (unsigned i = 0; i < load1->def.num_components; ++i) {
            if (load1->value[i].b != load2->value[i].b)
               return false;
         }
      } else {
         unsigned size = load1->def.num_components * sizeof(*load1->value);
         if (memcmp(load1->value, load2->value, size) != 0)
            return false;
      }
      return true;
   }
   case nir_instr_type_phi: {
      nir_phi_instr *phi1 = nir_instr_as_phi(instr1);
      nir_phi_instr *phi2 = nir_instr_as_phi(instr2);

      if (phi1->instr.block != phi2->instr.block)
         return false;

      /* In case of phis with no sources, the dest needs to be checked
       * to ensure that phis with incompatible dests won't get merged
       * during CSE. */
      if (phi1->def.num_components != phi2->def.num_components)
         return false;
      if (phi1->def.bit_size != phi2->def.bit_size)
         return false;

      nir_foreach_phi_src(src1, phi1) {
         nir_foreach_phi_src(src2, phi2) {
            if (src1->pred == src2->pred) {
               if (!nir_srcs_equal(src1->src, src2->src))
                  return false;

               break;
            }
         }
      }

      return true;
   }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrinsic1 = nir_instr_as_intrinsic(instr1);
      nir_intrinsic_instr *intrinsic2 = nir_instr_as_intrinsic(instr2);
      const nir_intrinsic_info *info =
         &nir_intrinsic_infos[intrinsic1->intrinsic];

      if (intrinsic1->intrinsic != intrinsic2->intrinsic ||
          intrinsic1->num_components != intrinsic2->num_components)
         return false;

      if (info->has_dest && intrinsic1->def.num_components !=
                               intrinsic2->def.num_components)
         return false;

      if (info->has_dest && intrinsic1->def.bit_size !=
                               intrinsic2->def.bit_size)
         return false;

      for (unsigned i = 0; i < info->num_srcs; i++) {
         if (!nir_srcs_equal(intrinsic1->src[i], intrinsic2->src[i]))
            return false;
      }

      int const_index1[NIR_INTRINSIC_MAX_CONST_INDEX];
      int const_index2[NIR_INTRINSIC_MAX_CONST_INDEX];
      const int *normalized1 =
         normalized_intrinsic_const_indices(intrinsic1, const_index1);
      const int *normalized2 =
         normalized_intrinsic_const_indices(intrinsic2, const_index2);

      if (memcmp(normalized1, normalized2,
                 info->num_index_slots * sizeof(intrinsic1->const_index[0])))
         return false;

      return true;
   }
   case nir_instr_type_call:
   case nir_instr_type_cmat_call:
   case nir_instr_type_jump:
   case nir_instr_type_undef:
   default:
      UNREACHABLE("Invalid instruction type");
   }

   UNREACHABLE("All cases in the above switch should return");
}

static bool
cmp_func(const void *data1, const void *data2)
{
   return nir_instrs_equal(data1, data2);
}

void
nir_instr_set_init(struct set *s, void *mem_ctx)
{
   _mesa_set_init(s, mem_ctx, hash_instr, cmp_func);
}

void
nir_instr_set_fini(struct set *instr_set)
{
   _mesa_set_fini(instr_set, NULL);
}

nir_instr *
nir_instr_set_add_or_rewrite(struct set *instr_set, nir_instr *instr,
                             bool (*cond_function)(const nir_instr *a,
                                                   const nir_instr *b))
{
   if (!instr_can_rewrite(instr))
      return NULL;

   struct set_entry *e = _mesa_set_search_or_add(instr_set, instr, NULL);
   nir_instr *match = (nir_instr *)e->key;
   if (match == instr)
      return NULL;

   if (!cond_function || cond_function(match, instr)) {
      /* rewrite instruction if condition is matched */
      nir_def *def = nir_instr_def(instr);
      nir_def *new_def = nir_instr_def(match);

      /* It's safe to replace an instruction with an one with different fp_math_ctrl as
       * long as we take the fp_math_ctrl union. If we got here, the two instructions are
       * exactly identical in every other way.
       */
      if (instr->type == nir_instr_type_alu) {
         nir_instr_as_alu(match)->fp_math_ctrl |= nir_instr_as_alu(instr)->fp_math_ctrl;
      } else if (instr->type == nir_instr_type_intrinsic) {
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
         nir_intrinsic_instr *match_intr = nir_instr_as_intrinsic(match);
         if (nir_intrinsic_has_io_semantics(intr) &&
             !nir_intrinsic_io_semantics(intr).no_signed_zero) {
            nir_io_semantics sem = nir_intrinsic_io_semantics(match_intr);
            sem.no_signed_zero = false;
            nir_intrinsic_set_io_semantics(match_intr, sem);
         } else if (nir_intrinsic_has_fp_math_ctrl(intr)) {
            unsigned fp_math_ctrl = nir_intrinsic_fp_math_ctrl(match_intr);
            fp_math_ctrl |= nir_intrinsic_fp_math_ctrl(intr);
            nir_intrinsic_set_fp_math_ctrl(match_intr, fp_math_ctrl);
         }
      }

      assert(!def == !new_def);
      if (def)
         nir_def_rewrite_uses(def, new_def);

      return match;
   } else {
      /* otherwise, replace hashed instruction */
      e->key = instr;
      return NULL;
   }
}

void
nir_instr_set_remove(struct set *instr_set, nir_instr *instr)
{
   if (!instr_can_rewrite(instr))
      return;

   struct set_entry *entry = _mesa_set_search(instr_set, instr);
   if (entry)
      _mesa_set_remove(instr_set, entry);
}
