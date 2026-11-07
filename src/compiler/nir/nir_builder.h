/*
 * Copyright © 2014-2015 Broadcom
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

#ifndef NIR_BUILDER_H
#define NIR_BUILDER_H

#include "util/bitscan.h"
#include "util/half_float.h"
#include "util/u_endian.h"
#include "nir_control_flow.h"

#ifdef __cplusplus
extern "C" {
#endif

struct exec_list;

typedef struct nir_builder {
   nir_cursor cursor;

   /* Whether new ALU instruction will be constanst-folded if possible. */
   bool constant_fold_alu;

   /* Float_controls2 bits. See nir_alu_instr for details. */
   uint32_t fp_math_ctrl;

   nir_shader *shader;
   nir_function_impl *impl;
} nir_builder;

static inline nir_builder
nir_builder_create(nir_function_impl *impl)
{
   nir_builder b;
   memset(&b, 0, sizeof(b));
   b.fp_math_ctrl = nir_fp_fast_math;
   b.impl = impl;
   b.shader = impl->function->shader;
   return b;
}

/* Requires the cursor to be inside a nir_function_impl. */
static inline nir_builder
nir_builder_at(nir_cursor cursor)
{
   nir_block *current_block = nir_cursor_current_block(cursor);

   nir_builder b = nir_builder_create(current_block->impl);
   b.cursor = cursor;
   return b;
}

nir_builder MUST_CHECK PRINTFLIKE(3, 4)
   nir_builder_init_simple_shader(mesa_shader_stage stage,
                                  const nir_shader_compiler_options *options,
                                  const char *name, ...);

typedef bool (*nir_instr_pass_cb)(struct nir_builder *, nir_instr *, void *);
typedef bool (*nir_intrinsic_pass_cb)(struct nir_builder *,
                                      nir_intrinsic_instr *, void *);
typedef bool (*nir_alu_pass_cb)(struct nir_builder *,
                                nir_alu_instr *, void *);
typedef bool (*nir_tex_pass_cb)(struct nir_builder *,
                                nir_tex_instr *, void *);
typedef bool (*nir_phi_pass_cb)(struct nir_builder *,
                                nir_phi_instr *, void *);
typedef bool (*nir_deref_pass_cb)(struct nir_builder *,
                                  nir_deref_instr *, void *);

/**
 * Iterates over all the instructions in a NIR function and calls the given pass
 * on them.
 *
 * The pass should return true if it modified the function.  In that case, only
 * the preserved metadata flags will be preserved in the function impl.
 *
 * The builder will be initialized to point at the function impl, but its
 * cursor is unset.
 */
static inline bool
nir_function_instructions_pass(nir_function_impl *impl,
                               nir_instr_pass_cb pass,
                               nir_metadata preserved,
                               void *cb_data)
{
   bool progress = false;
   nir_builder b = nir_builder_create(impl);

   nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         progress |= pass(&b, instr, cb_data);
      }
   }

   return nir_progress(progress, impl, preserved);
}

/**
 * Iterates over all the instructions in a NIR shader and calls the given pass
 * on them.
 *
 * The pass should return true if it modified the shader.  In that case, only
 * the preserved metadata flags will be preserved in the function impl.
 *
 * The builder will be initialized to point at the function impl, but its
 * cursor is unset.
 */
static inline bool
nir_shader_instructions_pass(nir_shader *shader,
                             nir_instr_pass_cb pass,
                             nir_metadata preserved,
                             void *cb_data)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      progress |= nir_function_instructions_pass(impl, pass,
                                                 preserved, cb_data);
   }

   return progress;
}

/**
 * Iterates over all the intrinsics in a NIR function and calls the given pass
 * on them.
 *
 * The pass should return true if it modified the shader.  In that case, only
 * the preserved metadata flags will be preserved in the function impl.
 *
 * The builder will be initialized to point at the function impl, but its
 * cursor is unset.
 */
static inline bool
nir_function_intrinsics_pass(nir_function_impl *impl,
                             nir_intrinsic_pass_cb pass,
                             nir_metadata preserved,
                             void *cb_data)
{
   bool progress = false;
   nir_builder b = nir_builder_create(impl);

   nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         if (instr->type == nir_instr_type_intrinsic) {
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            progress |= pass(&b, intr, cb_data);
         }
      }
   }

   return nir_progress(progress, impl, preserved);
}

/**
 * Iterates over all the intrinsics in a NIR shader and calls the given pass on
 * them.
 *
 * The pass should return true if it modified the shader.  In that case, only
 * the preserved metadata flags will be preserved in the function impl.
 *
 * The builder will be initialized to point at the function impl, but its
 * cursor is unset.
 */
static inline bool
nir_shader_intrinsics_pass(nir_shader *shader,
                           nir_intrinsic_pass_cb pass,
                           nir_metadata preserved,
                           void *cb_data)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      progress |= nir_function_intrinsics_pass(impl, pass, preserved, cb_data);
   }

   return progress;
}

/* As above, but for ALU */
static inline bool
nir_shader_alu_pass(nir_shader *shader,
                    nir_alu_pass_cb pass,
                    nir_metadata preserved,
                    void *cb_data)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      bool func_progress = false;
      nir_builder b = nir_builder_create(impl);

      nir_foreach_block_safe(block, impl) {
         nir_foreach_instr_safe(instr, block) {
            if (instr->type == nir_instr_type_alu) {
               nir_alu_instr *intr = nir_instr_as_alu(instr);
               func_progress |= pass(&b, intr, cb_data);
            }
         }
      }

      progress |= nir_progress(func_progress, impl, preserved);
   }

   return progress;
}

/* As above, but for textures */
static inline bool
nir_shader_tex_pass(nir_shader *shader, nir_tex_pass_cb pass,
                    nir_metadata preserved, void *cb_data)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      bool func_progress = false;
      nir_builder b = nir_builder_create(impl);

      nir_foreach_block_safe(block, impl) {
         nir_foreach_instr_safe(instr, block) {
            if (instr->type == nir_instr_type_tex) {
               nir_tex_instr *tex = nir_instr_as_tex(instr);
               func_progress |= pass(&b, tex, cb_data);
            }
         }
      }

      progress |= nir_progress(func_progress, impl, preserved);
   }

   return progress;
}

/* As above, but for phis */
static inline bool
nir_shader_phi_pass(nir_shader *shader,
                    nir_phi_pass_cb pass,
                    nir_metadata preserved,
                    void *cb_data)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      bool func_progress = false;
      nir_builder b = nir_builder_create(impl);

      nir_foreach_block_safe(block, impl) {
         nir_foreach_phi_safe(phi, block) {
            func_progress |= pass(&b, phi, cb_data);
         }
      }

      progress |= nir_progress(func_progress, impl, preserved);
   }

   return progress;
}

/* As above, but for derefs */
static inline bool
nir_shader_deref_pass(nir_shader *shader, nir_deref_pass_cb pass,
                      nir_metadata preserved, void *cb_data)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      bool func_progress = false;
      nir_builder b = nir_builder_create(impl);

      nir_foreach_block_safe(block, impl) {
         nir_foreach_instr_safe(instr, block) {
            if (instr->type == nir_instr_type_deref) {
               nir_deref_instr *deref = nir_instr_as_deref(instr);
               func_progress |= pass(&b, deref, cb_data);
            }
         }
      }

      progress |= nir_progress(func_progress, impl, preserved);
   }

   return progress;
}

void nir_builder_instr_insert(nir_builder *build, nir_instr *instr);
void nir_builder_instr_insert_at_top(nir_builder *build, nir_instr *instr);

static inline nir_instr *
nir_builder_last_instr(nir_builder *build)
{
   assert(build->cursor.option == nir_cursor_after_instr);
   return build->cursor.instr;
}

/* General nir_build_alu() taking a variable arg count with NULLs for the rest. */
nir_def *
nir_build_alu(nir_builder *build, nir_op op, nir_def *src0,
              nir_def *src1, nir_def *src2, nir_def *src3);

/* Fixed-arg-count variants to reduce size of codegen. */
nir_def *
nir_build_alu1(nir_builder *build, nir_op op, nir_def *src0);
nir_def *
nir_build_alu2(nir_builder *build, nir_op op, nir_def *src0,
               nir_def *src1);
nir_def *
nir_build_alu3(nir_builder *build, nir_op op, nir_def *src0,
               nir_def *src1, nir_def *src2);
nir_def *
nir_build_alu4(nir_builder *build, nir_op op, nir_def *src0,
               nir_def *src1, nir_def *src2, nir_def *src3);

nir_def *nir_build_alu_src_arr(nir_builder *build, nir_op op, nir_def **srcs);

nir_instr *nir_builder_last_instr(nir_builder *build);

void nir_builder_cf_insert(nir_builder *build, nir_cf_node *cf);

bool nir_builder_is_inside_cf(nir_builder *build, nir_cf_node *cf_node);

nir_if *
nir_push_if(nir_builder *build, nir_def *condition);

nir_if *
nir_push_else(nir_builder *build, nir_if *nif);

void nir_pop_if(nir_builder *build, nir_if *nif);

nir_def *
nir_if_phi(nir_builder *build, nir_def *then_def, nir_def *else_def);

nir_loop *
nir_push_loop(nir_builder *build);

nir_loop *
nir_push_continue(nir_builder *build, nir_loop *loop);

void nir_pop_loop(nir_builder *build, nir_loop *loop);

static inline nir_def *
nir_undef(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_undef_instr *undef =
      nir_undef_instr_create(build->shader, num_components, bit_size);
   if (!undef)
      return NULL;

   nir_builder_instr_insert_at_top(build, &undef->instr);

   return &undef->def;
}

static inline nir_def *
nir_build_imm(nir_builder *build, unsigned num_components,
              unsigned bit_size, const nir_const_value *value)
{
   nir_load_const_instr *load_const =
      nir_load_const_instr_create(build->shader, num_components, bit_size);
   if (!load_const)
      return NULL;

   memcpy(load_const->value, value, sizeof(nir_const_value) * num_components);

   nir_builder_instr_insert(build, &load_const->instr);

   return &load_const->def;
}

static inline nir_def *
nir_imm_zero(nir_builder *build, unsigned num_components, unsigned bit_size)
{
   nir_load_const_instr *load_const =
      nir_load_const_instr_create(build->shader, num_components, bit_size);

   /* nir_load_const_instr_create uses rzalloc so it's already zero */

   nir_builder_instr_insert(build, &load_const->instr);

   return &load_const->def;
}

static inline nir_def *
nir_imm_boolN_t(nir_builder *build, bool x, unsigned bit_size)
{
   nir_const_value v = nir_const_value_for_bool(x, bit_size);
   return nir_build_imm(build, 1, bit_size, &v);
}

static inline nir_def *
nir_imm_bool(nir_builder *build, bool x)
{
   return nir_imm_boolN_t(build, x, 1);
}

static inline nir_def *
nir_imm_true(nir_builder *build)
{
   return nir_imm_bool(build, true);
}

static inline nir_def *
nir_imm_false(nir_builder *build)
{
   return nir_imm_bool(build, false);
}

static inline nir_def *
nir_imm_floatN_t(nir_builder *build, double x, unsigned bit_size)
{
   nir_const_value v = nir_const_value_for_float(x, bit_size);
   return nir_build_imm(build, 1, bit_size, &v);
}

static inline nir_def *
nir_imm_float16(nir_builder *build, float x)
{
   return nir_imm_floatN_t(build, x, 16);
}

static inline nir_def *
nir_imm_float(nir_builder *build, float x)
{
   return nir_imm_floatN_t(build, x, 32);
}

static inline nir_def *
nir_imm_double(nir_builder *build, double x)
{
   return nir_imm_floatN_t(build, x, 64);
}

static inline nir_def *
nir_imm_vec2(nir_builder *build, float x, float y)
{
   nir_const_value v[2] = {
      nir_const_value_for_float(x, 32),
      nir_const_value_for_float(y, 32),
   };
   return nir_build_imm(build, 2, 32, v);
}

static inline nir_def *
nir_imm_vec3(nir_builder *build, float x, float y, float z)
{
   nir_const_value v[3] = {
      nir_const_value_for_float(x, 32),
      nir_const_value_for_float(y, 32),
      nir_const_value_for_float(z, 32),
   };
   return nir_build_imm(build, 3, 32, v);
}

static inline nir_def *
nir_imm_vec4(nir_builder *build, float x, float y, float z, float w)
{
   nir_const_value v[4] = {
      nir_const_value_for_float(x, 32),
      nir_const_value_for_float(y, 32),
      nir_const_value_for_float(z, 32),
      nir_const_value_for_float(w, 32),
   };

   return nir_build_imm(build, 4, 32, v);
}

static inline nir_def *
nir_imm_vec4_16(nir_builder *build, float x, float y, float z, float w)
{
   nir_const_value v[4] = {
      nir_const_value_for_float(x, 16),
      nir_const_value_for_float(y, 16),
      nir_const_value_for_float(z, 16),
      nir_const_value_for_float(w, 16),
   };

   return nir_build_imm(build, 4, 16, v);
}

static inline nir_def *
nir_imm_intN_t(nir_builder *build, uint64_t x, unsigned bit_size)
{
   nir_const_value v = nir_const_value_for_raw_uint(x, bit_size);
   return nir_build_imm(build, 1, bit_size, &v);
}

static inline nir_def *
nir_imm_int(nir_builder *build, int x)
{
   return nir_imm_intN_t(build, x, 32);
}

static inline nir_def *
nir_imm_int64(nir_builder *build, int64_t x)
{
   return nir_imm_intN_t(build, x, 64);
}

static inline nir_def *
nir_imm_ivec2(nir_builder *build, int x, int y)
{
   nir_const_value v[2] = {
      nir_const_value_for_int(x, 32),
      nir_const_value_for_int(y, 32),
   };

   return nir_build_imm(build, 2, 32, v);
}

static inline nir_def *
nir_imm_ivec3_intN(nir_builder *build, int x, int y, int z, unsigned bit_size)
{
   nir_const_value v[3] = {
      nir_const_value_for_int(x, bit_size),
      nir_const_value_for_int(y, bit_size),
      nir_const_value_for_int(z, bit_size),
   };

   return nir_build_imm(build, 3, bit_size, v);
}

static inline nir_def *
nir_imm_uvec2_intN(nir_builder *build, unsigned x, unsigned y,
                   unsigned bit_size)
{
   nir_const_value v[2] = {
      nir_const_value_for_uint(x, bit_size),
      nir_const_value_for_uint(y, bit_size),
   };

   return nir_build_imm(build, 2, bit_size, v);
}

static inline nir_def *
nir_imm_uvec3_intN(nir_builder *build, unsigned x, unsigned y, unsigned z,
                   unsigned bit_size)
{
   nir_const_value v[3] = {
      nir_const_value_for_uint(x, bit_size),
      nir_const_value_for_uint(y, bit_size),
      nir_const_value_for_uint(z, bit_size),
   };

   return nir_build_imm(build, 3, bit_size, v);
}

static inline nir_def *
nir_imm_ivec3(nir_builder *build, int x, int y, int z)
{
   return nir_imm_ivec3_intN(build, x, y, z, 32);
}

static inline nir_def *
nir_imm_ivec4_intN(nir_builder *build, int x, int y, int z, int w,
                   unsigned bit_size)
{
   nir_const_value v[4] = {
      nir_const_value_for_int(x, bit_size),
      nir_const_value_for_int(y, bit_size),
      nir_const_value_for_int(z, bit_size),
      nir_const_value_for_int(w, bit_size),
   };

   return nir_build_imm(build, 4, bit_size, v);
}

static inline nir_def *
nir_imm_ivec4(nir_builder *build, int x, int y, int z, int w)
{
   return nir_imm_ivec4_intN(build, x, y, z, w, 32);
}

static inline nir_def *
nir_imm_uvec8_intN(nir_builder *build, unsigned a, unsigned b, unsigned c,
                   unsigned d, unsigned e, unsigned f, unsigned g, unsigned h,
                   unsigned bit_size)
{
   nir_const_value v[8] = {
      nir_const_value_for_uint(a, bit_size),
      nir_const_value_for_uint(b, bit_size),
      nir_const_value_for_uint(c, bit_size),
      nir_const_value_for_uint(d, bit_size),
      nir_const_value_for_uint(e, bit_size),
      nir_const_value_for_uint(f, bit_size),
      nir_const_value_for_uint(g, bit_size),
      nir_const_value_for_uint(h, bit_size),
   };

   return nir_build_imm(build, 8, bit_size, v);
}

static inline nir_def *
nir_imm_uvec8(nir_builder *build, unsigned a, unsigned b, unsigned c,
              unsigned d, unsigned e, unsigned f, unsigned g, unsigned h)
{
   return nir_imm_uvec8_intN(build, a, b, c, d, e, f, g, h, 32);
}

nir_def *
nir_builder_alu_instr_finish_and_insert(nir_builder *build, nir_alu_instr *instr);

/* for the couple special cases with more than 4 src args: */
nir_def *
nir_build_alu_src_arr(nir_builder *build, nir_op op, nir_def **srcs);

/* Generic builder for system values. */
nir_def *
nir_load_system_value(nir_builder *build, nir_intrinsic_op op, int index,
                      unsigned num_components, unsigned bit_size);

#include "nir_builder_opcodes.h"
#undef nir_deref_mode_is

nir_def *
nir_type_convert(nir_builder *b,
                 nir_def *src,
                 nir_alu_type src_type,
                 nir_alu_type dest_type,
                 nir_rounding_mode rnd);

static inline nir_def *
nir_convert_to_bit_size(nir_builder *b,
                        nir_def *src,
                        nir_alu_type type,
                        unsigned bit_size)
{
   return nir_type_convert(b, src, type, (nir_alu_type)(type | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_i2iN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_convert_to_bit_size(b, src, nir_type_int, bit_size);
}

static inline nir_def *
nir_u2uN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_convert_to_bit_size(b, src, nir_type_uint, bit_size);
}

static inline nir_def *
nir_b2bN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_convert_to_bit_size(b, src, nir_type_bool, bit_size);
}

static inline nir_def *
nir_f2fN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_convert_to_bit_size(b, src, nir_type_float, bit_size);
}

static inline nir_def *
nir_i2b(nir_builder *b, nir_def *src)
{
   return nir_ine_imm(b, src, 0);
}

static inline nir_def *
nir_b2iN(nir_builder *b, nir_def *src, uint32_t bit_size)
{
   return nir_type_convert(b, src, nir_type_bool,
                           (nir_alu_type)(nir_type_int | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_b2fN(nir_builder *b, nir_def *src, uint32_t bit_size)
{
   return nir_type_convert(b, src, nir_type_bool,
                           (nir_alu_type)(nir_type_float | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_i2fN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_type_convert(b, src, nir_type_int,
                           (nir_alu_type)(nir_type_float | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_u2fN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_type_convert(b, src, nir_type_uint,
                           (nir_alu_type)(nir_type_float | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_f2uN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_type_convert(b, src, nir_type_float,
                           (nir_alu_type)(nir_type_uint | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_f2iN(nir_builder *b, nir_def *src, unsigned bit_size)
{
   return nir_type_convert(b, src, nir_type_float,
                           (nir_alu_type)(nir_type_int | bit_size),
                           nir_rounding_mode_undef);
}

static inline nir_def *
nir_vec(nir_builder *build, nir_def **comp, unsigned num_components)
{
   return nir_build_alu_src_arr(build, nir_op_vec(num_components), comp);
}

nir_def *
nir_vec_scalars(nir_builder *build, nir_scalar *comp, unsigned num_components);

static inline nir_def *
nir_mov_alu(nir_builder *build, nir_alu_src src, unsigned num_components)
{
   if (src.src.ssa->num_components == num_components) {
      bool any_swizzles = false;
      for (unsigned i = 0; i < num_components; i++) {
         if (src.swizzle[i] != i)
            any_swizzles = true;
      }
      if (!any_swizzles)
         return src.src.ssa;
   }

   if (build->constant_fold_alu && nir_src_is_const(src.src)) {
      nir_const_value dest[NIR_MAX_VEC_COMPONENTS];
      nir_load_const_instr *load_const = nir_src_as_load_const(src.src);
      for (unsigned i = 0; i < num_components; i++)
         dest[i] = load_const->value[src.swizzle[i]];

      return nir_build_imm(build, num_components,
                           nir_src_bit_size(src.src),
                           dest);
   }

   nir_alu_instr *mov = nir_alu_instr_create(build->shader, nir_op_mov);
   nir_def_init(&mov->instr, &mov->def, num_components,
                nir_src_bit_size(src.src));
   assert(nir_op_infos[nir_op_mov].valid_fp_math_ctrl == 0);
   mov->src[0] = src;
   nir_builder_instr_insert(build, &mov->instr);

   return &mov->def;
}

/* Tries to avoid inserting a mov for the replacement,
 * but if it has to insert one to handle non-alu, it's return instead of NULL.
 */
nir_def *
nir_def_rewrite_uses_with_alu_src(nir_builder *build, nir_def *def, nir_alu_src src);

/**
 * Construct a mov that reswizzles the source's components.
 */
static inline nir_def *
nir_swizzle(nir_builder *build, nir_def *src, const unsigned *swiz,
            unsigned num_components)
{
   assert(num_components <= NIR_MAX_VEC_COMPONENTS);
   nir_alu_src alu_src = { NIR_SRC_INIT };
   alu_src.src = nir_src_for_ssa(src);

   bool is_identity_swizzle = true;
   for (unsigned i = 0; i < num_components && i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (swiz[i] != i)
         is_identity_swizzle = false;
      alu_src.swizzle[i] = (uint8_t)swiz[i];
   }

   if (num_components == src->num_components && is_identity_swizzle)
      return src;

   return nir_mov_alu(build, alu_src, num_components);
}

/**
 * Convert 64-bit values split into pairs of 32-bit components between the
 * low-half-first order of nir_unpack_64_2x32() and memory order, which puts
 * the high half first on big-endian hosts. Lowered 64-bit IO uses memory
 * order.
 */
static inline nir_def *
nir_64_2x32_memory_order(nir_builder *build, nir_def *src)
{
   static const unsigned swap[] = { 1, 0, 3, 2 };

   assert(src->num_components == 2 || src->num_components == 4);
   if (UTIL_ARCH_LITTLE_ENDIAN)
      return src;

   return nir_swizzle(build, src, swap, src->num_components);
}

/* Selects the right fdot given the number of components in each source. */
static inline nir_def *
nir_fdot(nir_builder *build, nir_def *src0, nir_def *src1)
{
   assert(src0->num_components == src1->num_components);
   switch (src0->num_components) {
   case 1:
      return nir_fmul(build, src0, src1);
   case 2:
      return nir_fdot2(build, src0, src1);
   case 3:
      return nir_fdot3(build, src0, src1);
   case 4:
      return nir_fdot4(build, src0, src1);
   case 5:
      return nir_fdot5(build, src0, src1);
   case 8:
      return nir_fdot8(build, src0, src1);
   case 16:
      return nir_fdot16(build, src0, src1);
   default:
      UNREACHABLE("bad component size");
   }

   return NULL;
}

static inline nir_def *
nir_bfdot(nir_builder *build, nir_def *src0, nir_def *src1)
{
   assert(src0->num_components == src1->num_components);
   switch (src0->num_components) {
   case 1:
      return nir_bfmul(build, src0, src1);
   case 2:
      return nir_bfdot2(build, src0, src1);
   case 3:
      return nir_bfdot3(build, src0, src1);
   case 4:
      return nir_bfdot4(build, src0, src1);
   case 5:
      return nir_bfdot5(build, src0, src1);
   case 8:
      return nir_bfdot8(build, src0, src1);
   case 16:
      return nir_bfdot16(build, src0, src1);
   default:
      UNREACHABLE("bad component size");
   }

   return NULL;
}

static inline nir_def *
nir_ball_iequal(nir_builder *b, nir_def *src0, nir_def *src1)
{
   switch (src0->num_components) {
   case 1:
      return nir_ieq(b, src0, src1);
   case 2:
      return nir_ball_iequal2(b, src0, src1);
   case 3:
      return nir_ball_iequal3(b, src0, src1);
   case 4:
      return nir_ball_iequal4(b, src0, src1);
   case 5:
      return nir_ball_iequal5(b, src0, src1);
   case 8:
      return nir_ball_iequal8(b, src0, src1);
   case 16:
      return nir_ball_iequal16(b, src0, src1);
   default:
      UNREACHABLE("bad component size");
   }
}

static inline nir_def *
nir_ball(nir_builder *b, nir_def *src)
{
   return nir_ball_iequal(b, src, nir_imm_true(b));
}

static inline nir_def *
nir_bany_inequal(nir_builder *b, nir_def *src0, nir_def *src1)
{
   switch (src0->num_components) {
   case 1:
      return nir_ine(b, src0, src1);
   case 2:
      return nir_bany_inequal2(b, src0, src1);
   case 3:
      return nir_bany_inequal3(b, src0, src1);
   case 4:
      return nir_bany_inequal4(b, src0, src1);
   case 5:
      return nir_bany_inequal5(b, src0, src1);
   case 8:
      return nir_bany_inequal8(b, src0, src1);
   case 16:
      return nir_bany_inequal16(b, src0, src1);
   default:
      UNREACHABLE("bad component size");
   }
}

static inline nir_def *
nir_bany(nir_builder *b, nir_def *src)
{
   return nir_bany_inequal(b, src, nir_imm_false(b));
}

static inline nir_def *
nir_channel(nir_builder *b, nir_def *def, unsigned c)
{
   return nir_swizzle(b, def, &c, 1);
}

static inline nir_def *
nir_mov_scalar(nir_builder *b, nir_scalar scalar)
{
   return nir_channel(b, scalar.def, scalar.comp);
}

static inline nir_def *
nir_channel_or_undef(nir_builder *b, nir_def *def, signed int channel)
{
   if (channel >= 0 && channel < def->num_components)
      return nir_channel(b, def, channel);
   else
      return nir_undef(b, 1, def->bit_size);
}

static inline nir_def *
nir_channels(nir_builder *b, nir_def *def, nir_component_mask_t mask)
{
   unsigned num_channels = 0, swizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };

   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if ((mask & (1 << i)) == 0)
         continue;
      swizzle[num_channels++] = i;
   }

   return nir_swizzle(b, def, swizzle, num_channels);
}

static inline nir_def *
_nir_select_from_array_helper(nir_builder *b, nir_def **arr,
                              nir_def *idx,
                              unsigned start, unsigned end)
{
   if (start == end - 1) {
      return arr[start];
   } else {
      unsigned mid = start + (end - start) / 2;
      return nir_bcsel(b, nir_ilt_imm(b, idx, mid),
                       _nir_select_from_array_helper(b, arr, idx, start, mid),
                       _nir_select_from_array_helper(b, arr, idx, mid, end));
   }
}

static inline nir_def *
nir_select_from_ssa_def_array(nir_builder *b, nir_def **arr,
                              unsigned arr_len, nir_def *idx)
{
   return _nir_select_from_array_helper(b, arr, idx, 0, arr_len);
}

static inline nir_def *
nir_vector_extract(nir_builder *b, nir_def *vec, nir_def *c)
{
   nir_src c_src = nir_src_for_ssa(c);
   if (nir_src_is_const(c_src)) {
      uint64_t c_const = nir_src_as_uint(c_src);
      if (c_const < vec->num_components)
         return nir_channel(b, vec, (unsigned)c_const);
      else
         return nir_undef(b, 1, vec->bit_size);
   } else {
      nir_def *comps[NIR_MAX_VEC_COMPONENTS];
      for (unsigned i = 0; i < vec->num_components; i++)
         comps[i] = nir_channel(b, vec, i);
      return nir_select_from_ssa_def_array(b, comps, vec->num_components, c);
   }
}

/** Replaces the component of `vec` specified by `c` with `scalar` */
static inline nir_def *
nir_vector_insert_imm(nir_builder *b, nir_def *vec,
                      nir_def *scalar, unsigned c)
{
   assert(scalar->num_components == 1);
   assert(c < vec->num_components);

   nir_op vec_op = nir_op_vec(vec->num_components);
   nir_alu_instr *vec_instr = nir_alu_instr_create(b->shader, vec_op);

   for (unsigned i = 0; i < vec->num_components; i++) {
      if (i == c) {
         vec_instr->src[i].src = nir_src_for_ssa(scalar);
         vec_instr->src[i].swizzle[0] = 0;
      } else {
         vec_instr->src[i].src = nir_src_for_ssa(vec);
         vec_instr->src[i].swizzle[0] = (uint8_t)i;
      }
   }

   return nir_builder_alu_instr_finish_and_insert(b, vec_instr);
}

/** Replaces the component of `vec` specified by `c` with `scalar` */
static inline nir_def *
nir_vector_insert(nir_builder *b, nir_def *vec, nir_def *scalar,
                  nir_def *c)
{
   assert(scalar->num_components == 1);
   assert(c->num_components == 1);

   nir_src c_src = nir_src_for_ssa(c);
   if (nir_src_is_const(c_src)) {
      uint64_t c_const = nir_src_as_uint(c_src);
      if (c_const < vec->num_components)
         return nir_vector_insert_imm(b, vec, scalar, (unsigned )c_const);
      else
         return vec;
   } else {
      nir_const_value per_comp_idx_const[NIR_MAX_VEC_COMPONENTS];
      for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++)
         per_comp_idx_const[i] = nir_const_value_for_int(i, c->bit_size);
      nir_def *per_comp_idx =
         nir_build_imm(b, vec->num_components,
                       c->bit_size, per_comp_idx_const);

      /* nir_builder will automatically splat out scalars to vectors so an
       * insert is as simple as "if I'm the channel, replace me with the
       * scalar."
       */
      return nir_bcsel(b, nir_ieq(b, c, per_comp_idx), scalar, vec);
   }
}

static inline nir_def *
nir_replicate(nir_builder *b, nir_def *scalar, unsigned num_components)
{
   assert(scalar->num_components == 1);
   assert(num_components <= NIR_MAX_VEC_COMPONENTS);

   nir_def *copies[NIR_MAX_VEC_COMPONENTS] = { NULL };
   for (unsigned i = 0; i < num_components; ++i)
      copies[i] = scalar;

   return nir_vec(b, copies, num_components);
}

static inline nir_def *
nir_iadd_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   assert(x->bit_size <= 64);
   y &= BITFIELD64_MASK(x->bit_size);

   if (y == 0) {
      return x;
   } else {
      return nir_iadd(build, x, nir_imm_intN_t(build, y, x->bit_size));
   }
}

static inline nir_def *
nir_iadd_imm_nuw(nir_builder *b, nir_def *x, uint64_t y)
{
   nir_def *d = nir_iadd_imm(b, x, y);
   if (d != x && nir_def_is_alu(d))
      nir_def_as_alu(d)->no_unsigned_wrap = true;
   return d;
}

static inline nir_def *
nir_iadd_nuw(nir_builder *b, nir_def *x, nir_def *y)
{
   nir_def *d = nir_iadd(b, x, y);
   if (nir_def_is_alu(d))
      nir_def_as_alu(d)->no_unsigned_wrap = true;
   return d;
}

static inline nir_def *
nir_fgt_imm(nir_builder *build, nir_def *src1, double src2)
{
   return nir_flt(build, nir_imm_floatN_t(build, src2, src1->bit_size), src1);
}

static inline nir_def *
nir_fle_imm(nir_builder *build, nir_def *src1, double src2)
{
   return nir_fge(build, nir_imm_floatN_t(build, src2, src1->bit_size), src1);
}

/* Use nir_iadd(x, -y) for reversing parameter ordering */
static inline nir_def *
nir_isub_imm(nir_builder *build, uint64_t y, nir_def *x)
{
   return nir_isub(build, nir_imm_intN_t(build, y, x->bit_size), x);
}

static inline nir_def *
nir_imax_imm(nir_builder *build, nir_def *x, int64_t y)
{
   return nir_imax(build, x, nir_imm_intN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_imin_imm(nir_builder *build, nir_def *x, int64_t y)
{
   return nir_imin(build, x, nir_imm_intN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_umax_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   return nir_umax(build, x, nir_imm_intN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_umin_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   return nir_umin(build, x, nir_imm_intN_t(build, y, x->bit_size));
}

static inline nir_def *
_nir_mul_imm(nir_builder *build, nir_def *x, uint64_t y, bool amul)
{
   assert(x->bit_size <= 64);
   y &= BITFIELD64_MASK(x->bit_size);

   if (amul && build->shader->options)
      amul &= build->shader->options->has_amul;

   if (y == 0) {
      return nir_imm_intN_t(build, 0, x->bit_size);
   } else if (y == 1) {
      return x;
   } else if (amul) {
      return nir_amul(build, x, nir_imm_intN_t(build, y, x->bit_size));
   } else if ((!build->shader->options ||
               !build->shader->options->lower_bitops) &&
              util_is_power_of_two_or_zero64(y)) {
      return nir_ishl(build, x, nir_imm_int(build, ffsll(y) - 1));
   } else {
      return nir_imul(build, x, nir_imm_intN_t(build, y, x->bit_size));
   }
}

static inline nir_def *
nir_imul_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   return _nir_mul_imm(build, x, y, false);
}

static inline nir_def *
nir_imul_imm_nuw(nir_builder *build, nir_def *x, uint64_t y)
{
   nir_def *d = nir_imul_imm(build, x, y);
   if (d != x && nir_def_is_alu(d))
      nir_def_as_alu(d)->no_unsigned_wrap = true;
   return d;
}

static inline nir_def *
nir_imul_nuw(nir_builder *build, nir_def *x, nir_def *y)
{
   nir_def *d = nir_imul(build, x, y);
   if (nir_def_is_alu(d))
      nir_def_as_alu(d)->no_unsigned_wrap = true;
   return d;
}

static inline nir_def *
nir_amul_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   return _nir_mul_imm(build, x, y, true);
}

static inline nir_def *
nir_fadd_imm(nir_builder *build, nir_def *x, double y)
{
   return nir_fadd(build, x, nir_imm_floatN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_fsub_imm(nir_builder *build, double x, nir_def *y)
{
   return nir_fsub(build, nir_imm_floatN_t(build, x, y->bit_size), y);
}

static inline nir_def *
nir_fmul_imm(nir_builder *build, nir_def *x, double y)
{
   return nir_fmul(build, x, nir_imm_floatN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_fdiv_imm(nir_builder *build, nir_def *x, double y)
{
   return nir_fdiv(build, x, nir_imm_floatN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_fpow_imm(nir_builder *build, nir_def *x, double y)
{
   return nir_fpow(build, x, nir_imm_floatN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_iand_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   assert(x->bit_size <= 64);
   y &= BITFIELD64_MASK(x->bit_size);

   if (y == 0) {
      return nir_imm_intN_t(build, 0, x->bit_size);
   } else if (y == BITFIELD64_MASK(x->bit_size)) {
      return x;
   } else {
      return nir_iand(build, x, nir_imm_intN_t(build, y, x->bit_size));
   }
}

static inline nir_def *
nir_test_mask(nir_builder *build, nir_def *x, uint64_t mask)
{
   assert(mask <= BITFIELD64_MASK(x->bit_size));
   return nir_ine_imm(build, nir_iand_imm(build, x, mask), 0);
}

static inline nir_def *
nir_ior_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   assert(x->bit_size <= 64);
   y &= BITFIELD64_MASK(x->bit_size);

   if (y == 0) {
      return x;
   } else if (y == BITFIELD64_MASK(x->bit_size)) {
      return nir_imm_intN_t(build, y, x->bit_size);
   } else
      return nir_ior(build, x, nir_imm_intN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_ishl_imm(nir_builder *build, nir_def *x, uint32_t y)
{
   if (y == 0) {
      return x;
   } else {
      assert(y < x->bit_size);
      return nir_ishl(build, x, nir_imm_int(build, y));
   }
}

static inline nir_def *
nir_ishr_imm(nir_builder *build, nir_def *x, uint32_t y)
{
   if (y == 0) {
      return x;
   } else {
      return nir_ishr(build, x, nir_imm_int(build, y));
   }
}

static inline nir_def *
nir_ushr_imm(nir_builder *build, nir_def *x, uint32_t y)
{
   if (y == 0) {
      return x;
   } else {
      return nir_ushr(build, x, nir_imm_int(build, y));
   }
}

static inline nir_def *
nir_shr(nir_builder *build, bool is_signed, nir_def *x, nir_def *y)
{
   return is_signed ? nir_ishr(build, x, y) : nir_ushr(build, x, y);
}

static inline nir_def *
nir_imod_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   return nir_imod(build, x, nir_imm_intN_t(build, y, x->bit_size));
}

static inline nir_def *
nir_udiv_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   assert(x->bit_size <= 64);
   y &= BITFIELD64_MASK(x->bit_size);

   if (y == 1) {
      return x;
   } else if (util_is_power_of_two_nonzero64(y)) {
      return nir_ushr_imm(build, x, ffsll(y) - 1);
   } else {
      return nir_udiv(build, x, nir_imm_intN_t(build, y, x->bit_size));
   }
}

static inline nir_def *
nir_umod_imm(nir_builder *build, nir_def *x, uint64_t y)
{
   assert(y > 0 && y <= u_uintN_max(x->bit_size));

   if (util_is_power_of_two_nonzero64(y)) {
      return nir_iand_imm(build, x, y - 1);
   } else {
      return nir_umod(build, x, nir_imm_intN_t(build, y, x->bit_size));
   }
}

static inline nir_def *
nir_align_imm(nir_builder *b, nir_def *x, uint64_t align)
{
   if (align == 1)
      return x;

   assert(util_is_power_of_two_nonzero64(align));
   return nir_iand_imm(b, nir_iadd_imm(b, x, align - 1), ~(align - 1));
}

static inline nir_def *
nir_ibfe_imm(nir_builder *build, nir_def *x, uint32_t offset, uint32_t size)
{
   return nir_ibfe(build, x, nir_imm_int(build, offset), nir_imm_int(build, size));
}

static inline nir_def *
nir_ubfe_imm(nir_builder *build, nir_def *x, uint32_t offset, uint32_t size)
{
   return nir_ubfe(build, x, nir_imm_int(build, offset), nir_imm_int(build, size));
}

static inline nir_def *
nir_ubitfield_extract_imm(nir_builder *build, nir_def *x, uint32_t offset, uint32_t size)
{
   return nir_ubitfield_extract(build, x, nir_imm_int(build, offset), nir_imm_int(build, size));
}

static inline nir_def *
nir_ibitfield_extract_imm(nir_builder *build, nir_def *x, uint32_t offset, uint32_t size)
{
   return nir_ibitfield_extract(build, x, nir_imm_int(build, offset), nir_imm_int(build, size));
}

static inline nir_def *
nir_bitfield_insert_imm(nir_builder *build, nir_def *x, nir_def *insert, uint32_t offset, uint32_t size)
{
   return nir_bitfield_insert(build, x, insert, nir_imm_int(build, offset), nir_imm_int(build, size));
}

static inline nir_def *
nir_extract_u8_imm(nir_builder *b, nir_def *a, unsigned i)
{
   return nir_extract_u8(b, a, nir_imm_intN_t(b, i, a->bit_size));
}

static inline nir_def *
nir_extract_i8_imm(nir_builder *b, nir_def *a, unsigned i)
{
   return nir_extract_i8(b, a, nir_imm_intN_t(b, i, a->bit_size));
}

static inline nir_def *
nir_fclamp(nir_builder *b,
           nir_def *x, nir_def *min_val, nir_def *max_val)
{
   return nir_fmin(b, nir_fmax(b, x, min_val), max_val);
}

static inline nir_def *
nir_iclamp(nir_builder *b,
           nir_def *x, nir_def *min_val, nir_def *max_val)
{
   return nir_imin(b, nir_imax(b, x, min_val), max_val);
}

static inline nir_def *
nir_uclamp(nir_builder *b,
           nir_def *x, nir_def *min_val, nir_def *max_val)
{
   return nir_umin(b, nir_umax(b, x, min_val), max_val);
}

static inline nir_def *
nir_ffma_weak_imm12(nir_builder *build, nir_def *src0, double src1, double src2)
{
   if (build->shader->options &&
       build->shader->options->avoid_ternary_with_two_constants)
      return nir_fadd_imm(build, nir_fmul_imm(build, src0, src1), src2);
   else
      return nir_ffma_weak(build, src0, nir_imm_floatN_t(build, src1, src0->bit_size),
                                  nir_imm_floatN_t(build, src2, src0->bit_size));
}

static inline nir_def *
nir_ffma_weak_imm1(nir_builder *build, nir_def *src0, double src1, nir_def *src2)
{
   return nir_ffma_weak(build, src0, nir_imm_floatN_t(build, src1, src0->bit_size), src2);
}

static inline nir_def *
nir_ffma_weak_imm2(nir_builder *build, nir_def *src0, nir_def *src1, double src2)
{
   return nir_ffma_weak(build, src0, src1, nir_imm_floatN_t(build, src2, src0->bit_size));
}

static inline nir_def *
nir_a_minus_bc(nir_builder *build, nir_def *src0, nir_def *src1,
               nir_def *src2)
{
   return nir_ffma_weak(build, nir_fneg(build, src1), src2, src0);
}

static inline nir_def *
nir_pack_bits(nir_builder *b, nir_def *src, unsigned dest_bit_size)
{
   assert((unsigned)(src->num_components * src->bit_size) == dest_bit_size);

   switch (dest_bit_size) {
   case 64:
      switch (src->bit_size) {
      case 32:
         return nir_pack_64_2x32(b, src);
      case 16:
         return nir_pack_64_4x16(b, src);
      case 8: {
         nir_def *lo = nir_pack_32_4x8(b, nir_channels(b, src, 0x0f));
         nir_def *hi = nir_pack_32_4x8(b, nir_channels(b, src, 0xf0));
         return nir_pack_64_2x32(b, nir_vec2(b, lo, hi));
      }
      default:
         break;
      }
      break;

   case 32:
      switch (src->bit_size) {
      case 32: return src;
      case 16: return nir_pack_32_2x16(b, src);
      case 8: return nir_pack_32_4x8(b, src);
      default: break;
      }

      break;

   default:
      break;
   }

   /* If we got here, we have no dedicated unpack opcode. */
   nir_def *dest = nir_imm_intN_t(b, 0, dest_bit_size);
   for (unsigned i = 0; i < src->num_components; i++) {
      nir_def *val = nir_u2uN(b, nir_channel(b, src, i), dest_bit_size);
      val = nir_ishl(b, val, nir_imm_int(b, i * src->bit_size));
      dest = nir_ior(b, dest, val);
   }
   return dest;
}

static inline nir_def *
nir_unpack_bits(nir_builder *b, nir_def *src, unsigned dest_bit_size)
{
   assert(src->num_components == 1);
   assert(src->bit_size >= dest_bit_size);
   const unsigned dest_num_components = src->bit_size / dest_bit_size;
   assert(dest_num_components <= NIR_MAX_VEC_COMPONENTS);

   switch (src->bit_size) {
   case 64:
      switch (dest_bit_size) {
      case 32:
         return nir_unpack_64_2x32(b, src);
      case 16:
         return nir_unpack_64_4x16(b, src);
      case 8: {
         nir_def *split = nir_unpack_64_2x32(b, src);
         nir_def *lo = nir_unpack_32_4x8(b, nir_channel(b, split, 0));
         nir_def *hi = nir_unpack_32_4x8(b, nir_channel(b, split, 1));
         return nir_vec8(b, nir_channel(b, lo, 0), nir_channel(b, lo, 1),
                         nir_channel(b, lo, 2), nir_channel(b, lo, 3),
                         nir_channel(b, hi, 0), nir_channel(b, hi, 1),
                         nir_channel(b, hi, 2), nir_channel(b, hi, 3));
      }
      default:
         break;
      }
      break;

   case 32:
      switch (dest_bit_size) {
      case 32: return src;
      case 16: return nir_unpack_32_2x16(b, src);
      case 8: return nir_unpack_32_4x8(b, src);
      default: break;
      }

      break;

   default:
      break;
   }

   /* If we got here, we have no dedicated unpack opcode. */
   nir_def *dest_comps[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < dest_num_components; i++) {
      nir_def *val = nir_ushr_imm(b, src, i * dest_bit_size);
      dest_comps[i] = nir_u2uN(b, val, dest_bit_size);
   }
   return nir_vec(b, dest_comps, dest_num_components);
}

/**
 * Treats srcs as if it's one big blob of bits and extracts the range of bits
 * given by
 *
 *       [first_bit, first_bit + dest_num_components * dest_bit_size)
 *
 * The range can have any alignment or size as long as it's an integer number
 * of destination components and fits inside the concatenated sources.
 *
 * TODO: The one caveat here is that we can't handle byte alignment if 64-bit
 * values are involved because that would require pack/unpack to/from a vec8
 * which NIR currently does not support.
 */
static inline nir_def *
nir_extract_bits(nir_builder *b, nir_def **srcs, unsigned num_srcs,
                 unsigned first_bit,
                 unsigned dest_num_components, unsigned dest_bit_size)
{
   const unsigned num_bits = dest_num_components * dest_bit_size;

   /* Figure out the common bit size */
   unsigned common_bit_size = dest_bit_size;
   for (unsigned i = 0; i < num_srcs; i++)
      common_bit_size = MIN2(common_bit_size, srcs[i]->bit_size);
   if (first_bit > 0)
      common_bit_size = MIN2(common_bit_size, (1u << (ffs(first_bit) - 1)));

   /* We don't want to have to deal with 1-bit values */
   assert(common_bit_size >= 8);

   nir_def *common_comps[NIR_MAX_VEC_COMPONENTS * sizeof(uint64_t)];
   assert(num_bits / common_bit_size <= ARRAY_SIZE(common_comps));

   /* First, unpack to the common bit size and select the components from the
    * source.
    */
   int src_idx = -1;
   unsigned src_start_bit = 0;
   unsigned src_end_bit = 0;
   for (unsigned i = 0; i < num_bits / common_bit_size; i++) {
      const unsigned bit = first_bit + (i * common_bit_size);
      while (bit >= src_end_bit) {
         src_idx++;
         assert(src_idx < (int)num_srcs);
         src_start_bit = src_end_bit;
         src_end_bit += srcs[src_idx]->bit_size *
                        srcs[src_idx]->num_components;
      }
      assert(bit >= src_start_bit);
      assert(bit + common_bit_size <= src_end_bit);
      const unsigned rel_bit = bit - src_start_bit;
      const unsigned src_bit_size = srcs[src_idx]->bit_size;

      nir_def *comp = nir_channel(b, srcs[src_idx],
                                  rel_bit / src_bit_size);
      if (srcs[src_idx]->bit_size > common_bit_size) {
         nir_def *unpacked = nir_unpack_bits(b, comp, common_bit_size);
         comp = nir_channel(b, unpacked, (rel_bit % src_bit_size) / common_bit_size);
      }
      common_comps[i] = comp;
   }

   /* Now, re-pack the destination if we have to */
   if (dest_bit_size > common_bit_size) {
      unsigned common_per_dest = dest_bit_size / common_bit_size;
      nir_def *dest_comps[NIR_MAX_VEC_COMPONENTS];
      for (unsigned i = 0; i < dest_num_components; i++) {
         nir_def *unpacked = nir_vec(b, common_comps + i * common_per_dest,
                                     common_per_dest);
         dest_comps[i] = nir_pack_bits(b, unpacked, dest_bit_size);
      }
      return nir_vec(b, dest_comps, dest_num_components);
   } else {
      assert(dest_bit_size == common_bit_size);
      return nir_vec(b, common_comps, dest_num_components);
   }
}

static inline nir_def *
nir_bitcast_vector(nir_builder *b, nir_def *src, unsigned dest_bit_size)
{
   assert((src->bit_size * src->num_components) % dest_bit_size == 0);
   const unsigned dest_num_components =
      (src->bit_size * src->num_components) / dest_bit_size;
   assert(dest_num_components <= NIR_MAX_VEC_COMPONENTS);

   return nir_extract_bits(b, &src, 1, 0, dest_num_components, dest_bit_size);
}

static inline nir_def *
nir_trim_vector(nir_builder *b, nir_def *src, unsigned num_components)
{
   assert(src->num_components >= num_components);
   if (src->num_components == num_components)
      return src;

   return nir_channels(b, src, nir_component_mask(num_components));
}

/**
 * Pad a value to N components with undefs of matching bit size.
 * If the value already contains >= num_components, it is returned without change.
 */
static inline nir_def *
nir_pad_vector(nir_builder *b, nir_def *src, unsigned num_components)
{
   assert(src->num_components <= num_components);
   if (src->num_components == num_components)
      return src;

   nir_scalar components[NIR_MAX_VEC_COMPONENTS];
   nir_scalar undef = nir_get_scalar(nir_undef(b, 1, src->bit_size), 0);
   unsigned i = 0;
   for (; i < src->num_components; i++)
      components[i] = nir_get_scalar(src, i);
   for (; i < num_components; i++)
      components[i] = undef;

   return nir_vec_scalars(b, components, num_components);
}

/**
 * Pad a value to N components with copies of the given immediate of matching
 * bit size. If the value already contains >= num_components, it is returned
 * without change.
 */
static inline nir_def *
nir_pad_vector_imm_int(nir_builder *b, nir_def *src, uint64_t imm_val,
                       unsigned num_components)
{
   assert(src->num_components <= num_components);
   if (src->num_components == num_components)
      return src;

   nir_scalar components[NIR_MAX_VEC_COMPONENTS];
   nir_scalar imm = nir_get_scalar(nir_imm_intN_t(b, imm_val, src->bit_size), 0);
   unsigned i = 0;
   for (; i < src->num_components; i++)
      components[i] = nir_get_scalar(src, i);
   for (; i < num_components; i++)
      components[i] = imm;

   return nir_vec_scalars(b, components, num_components);
}

/**
 * Pad a value to 4 components with undefs of matching bit size.
 * If the value already contains >= 4 components, it is returned without change.
 */
static inline nir_def *
nir_pad_vec4(nir_builder *b, nir_def *src)
{
   return nir_pad_vector(b, src, 4);
}

/**
 * Resizes a vector by either trimming off components or adding undef
 * components, as needed.  Only use this helper if it's actually what you
 * need.  Prefer nir_pad_vector() or nir_trim_vector() instead if you know a
 * priori which direction you're resizing.
 */
static inline nir_def *
nir_resize_vector(nir_builder *b, nir_def *src, unsigned num_components)
{
   if (src->num_components < num_components)
      return nir_pad_vector(b, src, num_components);
   else
      return nir_trim_vector(b, src, num_components);
}

/* Shift channels to the left or right. Fill undefined components with undef.
 * Examples:
 *    channel_shift =  1, new_num_components = 4: .xyzw -> ._xyz
 *    channel_shift = -1, new_num_components = 3: .xyzw -> .yzw
 */
static inline nir_def *
nir_shift_channels(nir_builder *b, nir_def *def, int channel_shift,
                   unsigned new_num_components)
{
   if (channel_shift == 0)
      return nir_resize_vector(b, def, new_num_components);

   nir_def *chan[NIR_MAX_VEC_COMPONENTS];

   for (int i = 0; i < (int)new_num_components; i++) {
      int src_index = i - channel_shift;

      if (src_index >= 0 && src_index < def->num_components)
         chan[i] = nir_channel(b, def, src_index);
      else
         chan[i] = nir_undef(b, 1, def->bit_size);
   }

   return nir_vec(b, chan, new_num_components);
}

nir_def *
nir_ssa_for_alu_src(nir_builder *build, nir_alu_instr *instr, unsigned srcn);

static inline unsigned
nir_get_ptr_bitsize(nir_shader *shader)
{
   if (shader->info.stage == MESA_SHADER_KERNEL)
      return shader->info.cs.ptr_size;
   return 32;
}

static inline nir_deref_instr *
nir_build_deref_var(nir_builder *build, nir_variable *var)
{
   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_var);

   deref->modes = (nir_variable_mode)var->data.mode;
   deref->type = var->type;
   deref->var = var;

   nir_def_init(&deref->instr, &deref->def, 1,
                nir_get_ptr_bitsize(build->shader));

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

static inline nir_deref_instr *
nir_build_deref_array(nir_builder *build, nir_deref_instr *parent,
                      nir_def *index)
{
   assert(glsl_type_is_array(parent->type) ||
          glsl_type_is_matrix(parent->type) ||
          glsl_type_is_vector(parent->type));

   assert(index->bit_size == parent->def.bit_size);

   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_array);

   deref->modes = parent->modes;
   deref->type = glsl_get_array_element(parent->type);
   deref->parent = nir_src_for_ssa(&parent->def);
   deref->arr.index = nir_src_for_ssa(index);

   nir_def_init(&deref->instr, &deref->def,
                parent->def.num_components, parent->def.bit_size);

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

static inline nir_deref_instr *
nir_build_deref_array_imm(nir_builder *build, nir_deref_instr *parent,
                          int64_t index)
{
   nir_def *idx_ssa = nir_imm_intN_t(build, index,
                                     parent->def.bit_size);

   return nir_build_deref_array(build, parent, idx_ssa);
}

static inline nir_deref_instr *
nir_build_deref_ptr_as_array(nir_builder *build, nir_deref_instr *parent,
                             nir_def *index)
{
   assert(parent->deref_type == nir_deref_type_array ||
          parent->deref_type == nir_deref_type_ptr_as_array ||
          parent->deref_type == nir_deref_type_cast);

   assert(index->bit_size == parent->def.bit_size);

   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_ptr_as_array);

   deref->modes = parent->modes;
   deref->type = parent->type;
   deref->parent = nir_src_for_ssa(&parent->def);
   deref->arr.index = nir_src_for_ssa(index);

   nir_def_init(&deref->instr, &deref->def,
                parent->def.num_components, parent->def.bit_size);

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

static inline nir_deref_instr *
nir_build_deref_array_wildcard(nir_builder *build, nir_deref_instr *parent)
{
   assert(glsl_type_is_array(parent->type) ||
          glsl_type_is_matrix(parent->type));

   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_array_wildcard);

   deref->modes = parent->modes;
   deref->type = glsl_get_array_element(parent->type);
   deref->parent = nir_src_for_ssa(&parent->def);

   nir_def_init(&deref->instr, &deref->def,
                parent->def.num_components, parent->def.bit_size);

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

static inline nir_deref_instr *
nir_build_deref_struct(nir_builder *build, nir_deref_instr *parent,
                       unsigned index)
{
   assert(glsl_type_is_struct_or_ifc(parent->type));

   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_struct);

   deref->modes = parent->modes;
   deref->type = glsl_get_struct_field(parent->type, index);
   deref->parent = nir_src_for_ssa(&parent->def);
   deref->strct.index = index;

   nir_def_init(&deref->instr, &deref->def,
                parent->def.num_components, parent->def.bit_size);

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

static inline nir_deref_instr *
nir_build_deref_cast_with_alignment(nir_builder *build, nir_def *parent,
                                    nir_variable_mode modes,
                                    const struct glsl_type *type,
                                    unsigned ptr_stride,
                                    unsigned align_mul,
                                    unsigned align_offset)
{
   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_cast);

   deref->modes = modes;
   deref->type = type;
   deref->parent = nir_src_for_ssa(parent);
   deref->cast.align_mul = align_mul;
   deref->cast.align_offset = align_offset;
   deref->cast.ptr_stride = ptr_stride;

   nir_def_init(&deref->instr, &deref->def, parent->num_components,
                parent->bit_size);

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

static inline nir_deref_instr *
nir_build_deref_cast(nir_builder *build, nir_def *parent,
                     nir_variable_mode modes, const struct glsl_type *type,
                     unsigned ptr_stride)
{
   return nir_build_deref_cast_with_alignment(build, parent, modes, type,
                                              ptr_stride, 0, 0);
}

static inline nir_deref_instr *
nir_alignment_deref_cast(nir_builder *build, nir_deref_instr *parent,
                         uint32_t align_mul, uint32_t align_offset)
{
   nir_deref_instr *deref =
      nir_deref_instr_create(build->shader, nir_deref_type_cast);

   deref->modes = parent->modes;
   deref->type = parent->type;
   deref->parent = nir_src_for_ssa(&parent->def);
   deref->cast.ptr_stride = nir_deref_instr_array_stride(deref);
   deref->cast.align_mul = align_mul;
   deref->cast.align_offset = align_offset;

   nir_def_init(&deref->instr, &deref->def,
                parent->def.num_components, parent->def.bit_size);

   nir_builder_instr_insert(build, &deref->instr);

   return deref;
}

/** Returns a deref that follows another but starting from the given parent
 *
 * The new deref will be the same type and take the same array or struct index
 * as the leader deref but it may have a different parent.  This is very
 * useful for walking deref paths.
 */
static inline nir_deref_instr *
nir_build_deref_follower(nir_builder *b, nir_deref_instr *parent,
                         nir_deref_instr *leader)
{
   /* If the derefs would have the same parent, don't make a new one */
   if (leader->parent.ssa == &parent->def)
      return leader;

   UNUSED nir_deref_instr *leader_parent = nir_src_as_deref(leader->parent);

   switch (leader->deref_type) {
   case nir_deref_type_var:
      UNREACHABLE("A var dereference cannot have a parent");
      break;

   case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
      assert(glsl_type_is_matrix(parent->type) ||
             glsl_type_is_array(parent->type) ||
             (leader->deref_type == nir_deref_type_array &&
              glsl_type_is_vector(parent->type)));
      assert(glsl_get_length(parent->type) ==
             glsl_get_length(leader_parent->type));

      if (leader->deref_type == nir_deref_type_array) {
         nir_def *index = nir_i2iN(b, leader->arr.index.ssa,
                                   parent->def.bit_size);
         return nir_build_deref_array(b, parent, index);
      } else {
         return nir_build_deref_array_wildcard(b, parent);
      }

   case nir_deref_type_struct:
      assert(glsl_type_is_struct_or_ifc(parent->type));
      assert(glsl_get_length(parent->type) ==
             glsl_get_length(leader_parent->type));

      return nir_build_deref_struct(b, parent, leader->strct.index);

   case nir_deref_type_cast:
      return nir_build_deref_cast_with_alignment(b, &parent->def,
                                                 leader->modes,
                                                 leader->type,
                                                 leader->cast.ptr_stride,
                                                 leader->cast.align_mul,
                                                 leader->cast.align_offset);

   case nir_deref_type_ptr_as_array: {
      assert(parent->deref_type == nir_deref_type_array ||
             parent->deref_type == nir_deref_type_ptr_as_array ||
             parent->deref_type == nir_deref_type_cast);
      nir_def *index = nir_i2iN(b, leader->arr.index.ssa,
                                parent->def.bit_size);
      return nir_build_deref_ptr_as_array(b, parent, index);
   }

   default:
      UNREACHABLE("Invalid deref instruction type");
   }
   return NULL;
}

static inline nir_def *
nir_load_deref_with_access(nir_builder *build, nir_deref_instr *deref,
                           enum gl_access_qualifier access)
{
   return nir_build_load_deref(build, glsl_get_vector_elements(deref->type),
                               glsl_get_bit_size(deref->type), &deref->def,
                               access);
}

#undef nir_load_deref
static inline nir_def *
nir_load_deref(nir_builder *build, nir_deref_instr *deref)
{
   return nir_load_deref_with_access(build, deref, (enum gl_access_qualifier)0);
}

static inline void
nir_store_deref_with_access(nir_builder *build, nir_deref_instr *deref,
                            nir_def *value, unsigned writemask,
                            enum gl_access_qualifier access)
{
   writemask &= (1u << value->num_components) - 1u;
   nir_build_store_deref(build, &deref->def, value, writemask, access);
}

#undef nir_store_deref
static inline void
nir_store_deref(nir_builder *build, nir_deref_instr *deref,
                nir_def *value, unsigned writemask)
{
   nir_store_deref_with_access(build, deref, value, writemask,
                               (enum gl_access_qualifier)0);
}

static inline void
nir_build_write_masked_store(nir_builder *b, nir_deref_instr *vec_deref,
                             nir_def *value, unsigned component)
{
   assert(value->num_components == 1);
   unsigned num_components = glsl_get_components(vec_deref->type);
   assert(num_components > 1 && num_components <= NIR_MAX_VEC_COMPONENTS);

   nir_def *vec =
      nir_vector_insert_imm(b, nir_undef(b, num_components, value->bit_size),
                            value, component);
   nir_store_deref(b, vec_deref, vec, (1u << component));
}

static inline void
nir_build_write_masked_stores(nir_builder *b, nir_deref_instr *vec_deref,
                              nir_def *value, nir_def *index,
                              unsigned start, unsigned end)
{
   if (start == end - 1) {
      nir_build_write_masked_store(b, vec_deref, value, start);
   } else {
      unsigned mid = start + (end - start) / 2;
      nir_push_if(b, nir_ilt_imm(b, index, mid));
      nir_build_write_masked_stores(b, vec_deref, value, index, start, mid);
      nir_push_else(b, NULL);
      nir_build_write_masked_stores(b, vec_deref, value, index, mid, end);
      nir_pop_if(b, NULL);
   }
}

static inline void
nir_copy_deref_with_access(nir_builder *build, nir_deref_instr *dest,
                           nir_deref_instr *src,
                           enum gl_access_qualifier dest_access,
                           enum gl_access_qualifier src_access)
{
   nir_build_copy_deref(build, &dest->def, &src->def, dest_access, src_access);
}

#undef nir_copy_deref
static inline void
nir_copy_deref(nir_builder *build, nir_deref_instr *dest, nir_deref_instr *src)
{
   nir_copy_deref_with_access(build, dest, src,
                              (enum gl_access_qualifier)0,
                              (enum gl_access_qualifier)0);
}

static inline void
nir_memcpy_deref_with_access(nir_builder *build, nir_deref_instr *dest,
                             nir_deref_instr *src, nir_def *size,
                             enum gl_access_qualifier dest_access,
                             enum gl_access_qualifier src_access)
{
   nir_build_memcpy_deref(build, &dest->def, &src->def,
                          size, dest_access, src_access);
}

#undef nir_memcpy_deref
static inline void
nir_memcpy_deref(nir_builder *build, nir_deref_instr *dest,
                 nir_deref_instr *src, nir_def *size)
{
   nir_memcpy_deref_with_access(build, dest, src, size,
                                (enum gl_access_qualifier)0,
                                (enum gl_access_qualifier)0);
}

static inline nir_def *
nir_load_var(nir_builder *build, nir_variable *var)
{
   return nir_load_deref(build, nir_build_deref_var(build, var));
}

static inline void
nir_store_var(nir_builder *build, nir_variable *var, nir_def *value,
              unsigned writemask)
{
   nir_store_deref(build, nir_build_deref_var(build, var), value, writemask);
}

static inline void
nir_copy_var(nir_builder *build, nir_variable *dest, nir_variable *src)
{
   nir_copy_deref(build, nir_build_deref_var(build, dest),
                  nir_build_deref_var(build, src));
}

static inline nir_def *
nir_load_array_var(nir_builder *build, nir_variable *var, nir_def *index)
{
   nir_deref_instr *deref =
      nir_build_deref_array(build, nir_build_deref_var(build, var), index);
   return nir_load_deref(build, deref);
}

static inline nir_def *
nir_load_array_var_imm(nir_builder *build, nir_variable *var, int64_t index)
{
   nir_deref_instr *deref =
      nir_build_deref_array_imm(build, nir_build_deref_var(build, var), index);
   return nir_load_deref(build, deref);
}

static inline void
nir_store_array_var(nir_builder *build, nir_variable *var, nir_def *index,
                    nir_def *value, unsigned writemask)
{
   nir_deref_instr *deref =
      nir_build_deref_array(build, nir_build_deref_var(build, var), index);
   nir_store_deref(build, deref, value, writemask);
}

static inline void
nir_store_array_var_imm(nir_builder *build, nir_variable *var, int64_t index,
                        nir_def *value, unsigned writemask)
{
   nir_deref_instr *deref =
      nir_build_deref_array_imm(build, nir_build_deref_var(build, var), index);
   nir_store_deref(build, deref, value, writemask);
}

#undef nir_load_param
static inline nir_def *
nir_load_param(nir_builder *build, uint32_t param_idx)
{
   assert(param_idx < build->impl->function->num_params);
   nir_parameter *param = &build->impl->function->params[param_idx];
   return nir_build_load_param(build, param->num_components, param->bit_size, param_idx);
}

#undef nir_decl_reg
static inline nir_def *
nir_decl_reg(nir_builder *b, unsigned num_components, unsigned bit_size,
             unsigned num_array_elems)
{
   nir_intrinsic_instr *decl =
      nir_intrinsic_instr_create(b->shader, nir_intrinsic_decl_reg);
   nir_intrinsic_set_num_components(decl, num_components);
   nir_intrinsic_set_bit_size(decl, bit_size);
   nir_intrinsic_set_num_array_elems(decl, num_array_elems);
   nir_intrinsic_set_divergent(decl, true);
   nir_def_init(&decl->instr, &decl->def, 1, 32);

   nir_builder_instr_insert_at_top(b, &decl->instr);

   return &decl->def;
}

#undef nir_load_reg
static inline nir_def *
nir_load_reg(nir_builder *b, nir_def *reg)
{
   nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
   unsigned num_components = nir_intrinsic_num_components(decl);
   unsigned bit_size = nir_intrinsic_bit_size(decl);

   nir_def *res = nir_build_load_reg(b, num_components, bit_size, reg);
   res->divergent = nir_intrinsic_divergent(decl);

   return res;
}

#undef nir_store_reg
static inline void
nir_store_reg(nir_builder *b, nir_def *value, nir_def *reg)
{
   ASSERTED nir_intrinsic_instr *decl = nir_reg_get_decl(reg);
   ASSERTED unsigned num_components = nir_intrinsic_num_components(decl);
   ASSERTED unsigned bit_size = nir_intrinsic_bit_size(decl);

   assert(value->num_components == num_components);
   assert(value->bit_size == bit_size);

   nir_build_store_reg(b, value, reg);
}

static inline nir_tex_src
nir_tex_src_for_ssa(nir_tex_src_type src_type, nir_def *def)
{
   nir_tex_src src;
   src.src = nir_src_for_ssa(def);
   src.src_type = src_type;
   return src;
}

#undef nir_ddx
#undef nir_ddx_fine
#undef nir_ddx_coarse
#undef nir_ddy
#undef nir_ddy_fine
#undef nir_ddy_coarse

static inline nir_def *
nir_build_deriv(nir_builder *b, nir_def *x, nir_intrinsic_op intrin)
{
   struct _nir_ddx_indices indices = { 0 };
   if (b->shader->options->scalarize_ddx && x->num_components > 1) {
      nir_def *res[NIR_MAX_VEC_COMPONENTS] = { NULL };

      for (unsigned i = 0; i < x->num_components; ++i) {
         res[i] = _nir_build_ddx(b, x->bit_size, nir_channel(b, x, i), indices);
         nir_def_as_intrinsic(res[i])->intrinsic = intrin;
      }

      return nir_vec(b, res, x->num_components);
   } else {
      nir_def *res = _nir_build_ddx(b, x->bit_size, x, indices);
      nir_def_as_intrinsic(res)->intrinsic = intrin;
      return res;
   }
}

#define DEF_DERIV(op)                                                        \
   static inline nir_def *                                                   \
      nir_##op(nir_builder *build, nir_def *src0)                            \
   {                                                                         \
      return nir_build_deriv(build, src0, nir_intrinsic_##op);               \
   }

DEF_DERIV(ddx)
DEF_DERIV(ddx_fine)
DEF_DERIV(ddx_coarse)
DEF_DERIV(ddy)
DEF_DERIV(ddy_fine)
DEF_DERIV(ddy_coarse)

struct nir_tex_builder {
   nir_def *coord, *ms_index, *lod, *bias, *comparator;
   nir_def *backend1, *backend2;
   unsigned texture_index, sampler_index;
   nir_def *texture_offset, *sampler_offset;
   nir_def *texture_heap_offset, *sampler_heap_offset;
   nir_def *texture_handle, *sampler_handle;
   nir_deref_instr *texture_deref, *sampler_deref;
   enum glsl_sampler_dim dim;
   nir_alu_type dest_type;
   bool is_array;
   bool is_sparse;
   bool can_speculate;
   uint32_t backend_flags;
};

nir_def *nir_build_tex_struct(nir_builder *build, nir_texop op,
                              struct nir_tex_builder fields);

#define nir_build_tex(build, op, ...)                                          \
   nir_build_tex_struct(build, op, (struct nir_tex_builder){__VA_ARGS__})

#define nir_tex(build, coord_, ...)                                            \
   nir_build_tex(build, nir_texop_tex, .coord = coord_, __VA_ARGS__)

#define nir_txf(build, coord_, ...)                                            \
   nir_build_tex(build, nir_texop_txf, .coord = coord_, __VA_ARGS__)

#define nir_txf_ms(build, coord_, ms_index_, ...)       \
   nir_build_tex(build, nir_texop_txf, .coord = coord_, \
                 .ms_index = ms_index_, __VA_ARGS__)

#define nir_txs(build, ...) nir_build_tex(build, nir_texop_txs, __VA_ARGS__)

#define nir_texture_samples(build, ...)                                        \
   nir_build_tex(build, nir_texop_texture_samples, __VA_ARGS__)

#define nir_samples_identical(build, coord_, ...)                              \
   nir_build_tex(build, nir_texop_samples_identical, .coord = coord_,          \
                 __VA_ARGS__)

/* calculate a `(1 << value) - 1` in ssa without overflows */
static inline nir_def *
nir_mask(nir_builder *b, nir_def *bits, unsigned dst_bit_size)
{
   return nir_ushr(b, nir_imm_intN_t(b, -1, dst_bit_size),
                   nir_isub_imm(b, dst_bit_size, nir_u2u32(b, bits)));
}

static inline nir_def *
nir_load_barycentric(nir_builder *build, nir_intrinsic_op op,
                     unsigned interp_mode)
{
   unsigned num_components = op == nir_intrinsic_load_barycentric_model ? 3 : 2;
   nir_intrinsic_instr *bary = nir_intrinsic_instr_create(build->shader, op);
   nir_def_init(&bary->instr, &bary->def, num_components, 32);
   nir_intrinsic_set_interp_mode(bary, interp_mode);
   nir_builder_instr_insert(build, &bary->instr);
   return &bary->def;
}

static inline void
nir_jump(nir_builder *build, nir_jump_type jump_type)
{
   assert(jump_type != nir_jump_goto && jump_type != nir_jump_goto_if);
   nir_jump_instr *jump = nir_jump_instr_create(build->shader, jump_type);
   nir_builder_instr_insert(build, &jump->instr);
}

static inline void
nir_goto(nir_builder *build, struct nir_block *target)
{
   assert(!build->impl->structured);
   nir_jump_instr *jump = nir_jump_instr_create(build->shader, nir_jump_goto);
   jump->target = target;
   nir_builder_instr_insert(build, &jump->instr);
}

static inline void
nir_goto_if(nir_builder *build, struct nir_block *target, nir_def *cond,
            struct nir_block *else_target)
{
   assert(!build->impl->structured);
   nir_jump_instr *jump = nir_jump_instr_create(build->shader, nir_jump_goto_if);
   jump->condition = nir_src_for_ssa(cond);
   jump->target = target;
   jump->else_target = else_target;
   nir_builder_instr_insert(build, &jump->instr);
}

static inline void
nir_break_if(nir_builder *build, nir_def *cond)
{
   nir_if *nif = nir_push_if(build, cond);
   {
      nir_jump(build, nir_jump_break);
   }
   nir_pop_if(build, nif);
}

static inline void
nir_build_call(nir_builder *build, nir_function *func, size_t count,
               nir_def **args)
{
   assert(count == func->num_params && "parameter count must match");
   nir_call_instr *call = nir_call_instr_create(build->shader, func);

   for (unsigned i = 0; i < count; ++i) {
      call->params[i] = nir_src_for_ssa(args[i]);
   }

   nir_builder_instr_insert(build, &call->instr);
}

static inline void
nir_build_indirect_call(nir_builder *build, nir_function *func, nir_def *callee,
                        size_t count, nir_def **args)
{
   assert(count == func->num_params && "parameter count must match");
   assert(!func->impl && "cannot call directly defined functions indirectly");
   nir_call_instr *call = nir_call_instr_create(build->shader, func);

   for (unsigned i = 0; i < func->num_params; ++i) {
      call->params[i] = nir_src_for_ssa(args[i]);
   }
   call->indirect_callee = nir_src_for_ssa(callee);

   nir_builder_instr_insert(build, &call->instr);
}

static inline void
nir_discard(nir_builder *build)
{
   if (build->shader->options->discard_is_demote)
      nir_demote(build);
   else
      nir_terminate(build);
}

static inline void
nir_discard_if(nir_builder *build, nir_def *src)
{
   if (build->shader->options->discard_is_demote)
      nir_demote_if(build, src);
   else
      nir_terminate_if(build, src);
}

static inline nir_def *
nir_inverse_ballot_imm(nir_builder *build, uint64_t imm, unsigned bit_size)
{
   return nir_inverse_ballot(build, nir_imm_intN_t(build, imm, bit_size));
}

nir_def *
nir_build_frag_coord(nir_builder *b, unsigned num_components);

nir_def *
nir_build_string(nir_builder *build, const char *value);

/*
 * Call a given nir_function * with a variadic number of nir_def * arguments.
 *
 * Defined with __VA_ARGS__ instead of va_list so we can assert the correct
 * number of parameters are passed in.
 */
#define nir_call(build, func, ...)                         \
   do {                                                    \
      nir_def *args[] = { __VA_ARGS__ };                   \
      nir_build_call(build, func, ARRAY_SIZE(args), args); \
   } while (0)

#define nir_call_indirect(build, func, callee, ...)                           \
   do {                                                                       \
      nir_def *_args[] = { __VA_ARGS__ };                                     \
      nir_build_indirect_call(build, func, callee, ARRAY_SIZE(_args), _args); \
   } while (0)

nir_def *
nir_compare_func(nir_builder *b, enum compare_func func,
                 nir_def *src0, nir_def *src1);

static inline void
nir_scoped_memory_barrier(nir_builder *b,
                          mesa_scope scope,
                          nir_memory_semantics semantics,
                          nir_variable_mode modes)
{
   nir_barrier(b, SCOPE_NONE, scope, semantics, modes);
}

nir_def *
nir_gen_rect_vertices(nir_builder *b, nir_def *z, nir_def *w);

/* Emits a printf in the same way nir_lower_printf(). Each of the variadic
 * argument is a pointer to a nir_def value.
 */
void nir_printf_fmt(nir_builder *b, unsigned ptr_bit_size,
                    const char *fmt, ...);
void nir_printf_fmt_at_px(nir_builder *b, unsigned ptr_bit_size,
                          unsigned x, unsigned y, const char *fmt, ...);

/* Call a serialized function. This is used internally by vtn_bindgen, it is not
 * intended for end-users of NIR.
 */
/* glsl-optimizer change: no nir_call_serialized() */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* NIR_BUILDER_H */
