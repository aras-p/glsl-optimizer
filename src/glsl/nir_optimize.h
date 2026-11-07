// NIR side of glsl-optimizer: the optimization pipeline, and GLSL ES precision
// analysis shared by the pipeline (precision markers) and the printers.
#pragma once

#include "compiler/nir/nir.h"
#include <map>
#include <set>
#include <functional>
#include <vector>

// Compiler options: keep what GLSL/Metal can express, lower what they can't.
void glslopt_init_nir_options(nir_shader_compiler_options* options, unsigned max_unroll_iterations);

// Bring the NIR from glsl_to_nir into the form the printers handle: a single
// entry point with everything inlined.
void glslopt_lower_nir(nir_shader* nir);

// Full optimization pipeline; ends out of SSA (registers) for printing.
void glslopt_optimize_nir(nir_shader* nir);

// Turn runs of leaf-by-leaf copies (store_deref(d.x, load_deref(s.x)) for
// every leaf x of an aggregate) back into copy_deref(d, s), so they print as
// one assignment. allow() can reject copies a dialect cannot express.
void glslopt_combine_copies(nir_shader* nir, const std::function<bool(nir_deref_instr* dst, nir_deref_instr* src)>& allow);

// Variables referenced from the entry point.
std::set<const nir_variable*> glslopt_referenced_vars(nir_shader* nir);

static inline bool glslopt_is_derivative(nir_intrinsic_op op)
{
   return op == nir_intrinsic_ddx || op == nir_intrinsic_ddy || op == nir_intrinsic_ddx_fine ||
          op == nir_intrinsic_ddy_fine || op == nir_intrinsic_ddx_coarse || op == nir_intrinsic_ddy_coarse;
}

// subgroup operations whose result is (a function of) their first source
static inline bool glslopt_is_subgroup_value_op(nir_intrinsic_op op)
{
   switch (op) {
   case nir_intrinsic_reduce: case nir_intrinsic_inclusive_scan: case nir_intrinsic_exclusive_scan:
   case nir_intrinsic_read_first_invocation: case nir_intrinsic_read_invocation:
   case nir_intrinsic_shuffle: case nir_intrinsic_shuffle_xor: case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down: case nir_intrinsic_quad_broadcast: case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical: case nir_intrinsic_quad_swap_diagonal:
      return true;
   default:
      return false;
   }
}

// Source-level GLSL ES precision rules (what the old propagate_precision did
// on GLSL IR), computed as metadata on SSA values:
//  - leaves: variables/struct fields have declared precision, textures follow
//    the sampler (size queries are highp), constants have none
//  - an operation has the highest precision of its operands (ignoring select
//    conditions); bool->float is mediump
//  - registers merge the precision of everything stored to them
// lowp is treated as mediump.
struct glslopt_precision_info {
   std::vector<unsigned char> prec;    // per SSA def index (GLSL_PRECISION_*)
   std::vector<unsigned char> regprec; // per decl_reg def index
   // precision inferred for variables declared without one (compiler temps),
   // from what is stored to them; only used before vars_to_ssa
   std::map<nir_variable*, unsigned> var_inferred;

   static unsigned norm(unsigned p) { return p == GLSL_PRECISION_LOW ? GLSL_PRECISION_MEDIUM : p; }
   static unsigned higher(unsigned a, unsigned b)
   {
      a = norm(a);
      b = norm(b);
      if (a == GLSL_PRECISION_NONE) return b;
      if (b == GLSL_PRECISION_NONE) return a;
      return a < b ? a : b; // HIGH(1) < MEDIUM(2)
   }
   static unsigned deref_prec(nir_deref_instr* d);
   unsigned instr_prec(nir_instr* instr);
   // fixpoint over the function; track_vars also infers var_inferred
   void infer(nir_function_impl* impl, bool track_vars);
};
