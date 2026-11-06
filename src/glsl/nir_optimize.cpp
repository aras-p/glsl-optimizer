#include "nir_optimize.h"
#include "compiler/nir/nir_builder.h"
#include "compiler/glsl_types.h"
#include <string.h>

void glslopt_init_nir_options(nir_shader_compiler_options* o, unsigned max_unroll_iterations)
{
   memset(o, 0, sizeof(*o));
   o->has_fsub = true;
   o->has_isub = true;
   o->lower_fdph = true;
   o->lower_scmp = true;
   o->lower_extract_byte = true;
   o->lower_extract_word = true;
   o->lower_insert_byte = true;
   o->lower_insert_word = true;
   o->lower_uadd_carry = true;
   o->lower_usub_borrow = true;
   o->lower_hadd = true;
   o->lower_mul_high = true;
   o->lower_bitfield_extract = true;
   o->lower_bitfield_insert = true;
   o->lower_int64_options = (nir_lower_int64_options)~0;
   o->max_unroll_iterations = max_unroll_iterations;
   o->has_cs_global_id = true;
}

// ---------------------------------------------------------------------------
// precision analysis

unsigned glslopt_precision_info::deref_prec(nir_deref_instr* d)
{
   for (nir_deref_instr* p = d; p; p = nir_deref_instr_parent(p)) {
      if (p->deref_type == nir_deref_type_struct) {
         const glsl_type* pt = nir_deref_instr_parent(p)->type;
         unsigned fp = glsl_get_struct_field_data(pt, p->strct.index)->precision;
         if (fp != GLSL_PRECISION_NONE)
            return norm(fp);
      } else if (p->deref_type == nir_deref_type_var) {
         return norm(p->var->data.precision);
      }
   }
   return GLSL_PRECISION_NONE;
}

unsigned glslopt_precision_info::instr_prec(nir_instr* instr)
{
   switch (instr->type) {
   case nir_instr_type_alu: {
      nir_alu_instr* alu = nir_instr_as_alu(instr);
      if (alu->def.bit_size == 16)
         return GLSL_PRECISION_MEDIUM;
      if (alu->op == nir_op_b2f32 || alu->op == nir_op_b2f16)
         return GLSL_PRECISION_MEDIUM;
      unsigned p = GLSL_PRECISION_NONE;
      for (unsigned i = alu->op == nir_op_bcsel ? 1 : 0; i < nir_op_infos[alu->op].num_inputs; i++)
         p = higher(p, prec[alu->src[i].src.ssa->index]);
      return p;
   }
   case nir_instr_type_tex: {
      nir_tex_instr* tex = nir_instr_as_tex(instr);
      if (tex->op == nir_texop_txs || tex->op == nir_texop_query_levels || tex->op == nir_texop_lod ||
          tex->op == nir_texop_texture_samples)
         return GLSL_PRECISION_HIGH;
      int i = nir_tex_instr_src_index(tex, nir_tex_src_texture_deref);
      if (i < 0) i = nir_tex_instr_src_index(tex, nir_tex_src_sampler_deref);
      return i >= 0 ? deref_prec(nir_src_as_deref(tex->src[i].src)) : GLSL_PRECISION_NONE;
   }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr* in = nir_instr_as_intrinsic(instr);
      switch (in->intrinsic) {
      case nir_intrinsic_glslopt_precision:
         return nir_intrinsic_base(in);
      case nir_intrinsic_load_deref:
      case nir_intrinsic_image_deref_load:
      case nir_intrinsic_deref_atomic:
      case nir_intrinsic_deref_atomic_swap:
      case nir_intrinsic_image_deref_atomic:
      case nir_intrinsic_image_deref_atomic_swap: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         unsigned p = deref_prec(d);
         if (p == GLSL_PRECISION_NONE) {
            nir_variable* v = nir_deref_instr_get_variable(d);
            auto it = v ? var_inferred.find(v) : var_inferred.end();
            if (it != var_inferred.end())
               p = it->second;
         }
         return p;
      }
      case nir_intrinsic_load_reg:
         return regprec[in->src[0].ssa->index];
      case nir_intrinsic_load_point_coord:
         return GLSL_PRECISION_MEDIUM;
      default:
         if (glslopt_is_derivative(in->intrinsic) || glslopt_is_subgroup_value_op(in->intrinsic))
            return prec[in->src[0].ssa->index];
         return nir_intrinsic_infos[in->intrinsic].has_dest ? GLSL_PRECISION_HIGH : GLSL_PRECISION_NONE;
      }
   }
   default:
      return GLSL_PRECISION_NONE;
   }
}

static bool
is_local_store(nir_intrinsic_instr* in, nir_deref_instr** out)
{
   if (in->intrinsic != nir_intrinsic_store_deref)
      return false;
   nir_deref_instr* d = nir_src_as_deref(in->src[0]);
   if (!nir_deref_mode_is_one_of(d, (nir_variable_mode)(nir_var_function_temp | nir_var_shader_temp)))
      return false;
   *out = d;
   return true;
}

void glslopt_precision_info::infer(nir_function_impl* impl, bool track_vars)
{
   prec.assign(impl->ssa_alloc, GLSL_PRECISION_NONE);
   regprec.assign(impl->ssa_alloc, GLSL_PRECISION_NONE);
   var_inferred.clear();
   bool changed = true;
   for (int pass = 0; changed && pass < 20; pass++) {
      changed = false;
      nir_foreach_block(block, impl) {
         nir_foreach_instr(instr, block) {
            if (instr->type == nir_instr_type_intrinsic) {
               nir_intrinsic_instr* in = nir_instr_as_intrinsic(instr);
               if (in->intrinsic == nir_intrinsic_store_reg) {
                  unsigned r = in->src[1].ssa->index;
                  unsigned p = higher(regprec[r], prec[in->src[0].ssa->index]);
                  if (p != regprec[r]) { regprec[r] = p; changed = true; }
                  continue;
               }
               nir_deref_instr* d;
               if (track_vars && is_local_store(in, &d) && deref_prec(d) == GLSL_PRECISION_NONE) {
                  if (nir_variable* v = nir_deref_instr_get_variable(d)) {
                     unsigned old = var_inferred.count(v) ? var_inferred[v] : GLSL_PRECISION_NONE;
                     unsigned p = higher(old, prec[in->src[1].ssa->index]);
                     if (p != old) { var_inferred[v] = p; changed = true; }
                  }
                  continue;
               }
            }
            nir_def* def = nir_instr_def(instr);
            if (!def)
               continue;
            unsigned p = higher(prec[def->index], instr_prec(instr));
            if (p != prec[def->index]) { prec[def->index] = p; changed = true; }
         }
      }
   }
}

// ---------------------------------------------------------------------------
// Precision markers (see the glslopt_precision intrinsic in nir_intrinsics.py).
//
// vars_to_ssa erases stores into local variables, and with them the GLSL
// precision conversion that happens when e.g. a highp value is assigned to a
// mediump local (or vice versa). Before going to SSA, find such stores and wrap
// the stored value in glslopt_precision(value, precision): an opaque identity
// that keeps the optimizer from folding across the conversion, and that the
// printers turn into a temporary declared with that precision. Same condition
// as the old ir_precision_conversion(): both sides have a precision and differ.

static void
insert_precision_markers(nir_shader* nir)
{
   nir_function_impl* impl = nir_shader_get_entrypoint(nir);
   nir_index_ssa_defs(impl);
   glslopt_precision_info pi;
   pi.infer(impl, true);

   nir_builder b = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         if (instr->type != nir_instr_type_intrinsic)
            continue;
         nir_intrinsic_instr* st = nir_instr_as_intrinsic(instr);
         nir_deref_instr* d;
         if (!is_local_store(st, &d))
            continue;
         nir_def* val = st->src[1].ssa;
         if (val->bit_size == 1)
            continue;
         unsigned vp = glslopt_precision_info::deref_prec(d);
         unsigned sp = pi.prec[val->index];
         if (vp == GLSL_PRECISION_NONE || sp == GLSL_PRECISION_NONE || vp == sp)
            continue;
         b.cursor = nir_before_instr(instr);
         nir_intrinsic_instr* m = nir_intrinsic_instr_create(nir, nir_intrinsic_glslopt_precision);
         m->num_components = val->num_components;
         m->src[0] = nir_src_for_ssa(val);
         nir_def_init(&m->instr, &m->def, val->num_components, val->bit_size);
         nir_intrinsic_set_base(m, vp);
         nir_builder_instr_insert(&b, &m->instr);
         nir_src_rewrite(&st->src[1], &m->def);
      }
   }
   nir_progress(true, impl, nir_metadata_control_flow);
}

// Drop markers that no longer mean anything: on constants/undefs, or nested.
static bool
opt_precision_markers(nir_shader* nir)
{
   bool progress = false;
   nir_function_impl* impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         if (instr->type != nir_instr_type_intrinsic)
            continue;
         nir_intrinsic_instr* m = nir_instr_as_intrinsic(instr);
         if (m->intrinsic != nir_intrinsic_glslopt_precision)
            continue;
         nir_instr* si = nir_def_instr(m->src[0].ssa);
         if (si->type == nir_instr_type_load_const || si->type == nir_instr_type_undef) {
            nir_def_rewrite_uses(&m->def, m->src[0].ssa);
            nir_instr_remove(instr);
            progress = true;
         } else if (si->type == nir_instr_type_intrinsic &&
                    nir_instr_as_intrinsic(si)->intrinsic == nir_intrinsic_glslopt_precision) {
            nir_src_rewrite(&m->src[0], nir_instr_as_intrinsic(si)->src[0].ssa);
            progress = true;
         }
      }
   }
   if (progress)
      nir_progress(true, impl, nir_metadata_control_flow);
   else
      nir_no_progress(impl);
   return progress;
}

// ---------------------------------------------------------------------------
// pipeline

// Upstream inserts the call to the global-initializer wrapper function during
// linking (gl_nir_link_shaders). We don't link, so do the same here.
static void
call_globals_wrapper(nir_shader* nir)
{
   nir_function_impl* main_impl = nir_shader_get_entrypoint(nir);
   nir_foreach_function(func, nir) {
      if (!func->is_tmp_globals_wrapper || !func->impl)
         continue;
      nir_builder b = nir_builder_at(nir_before_impl(main_impl));
      nir_call(&b, func);
   }
}

// After inlining, replace deref_texture_src/deref_sampler_src intrinsics with
// plain derefs and fold implicit array lengths (both done by upstream's linker).
static void
resolve_texture_srcs(nir_shader* nir)
{
   nir_function_impl* impl = nir_shader_get_entrypoint(nir);
   nir_builder b = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         if (instr->type == nir_instr_type_intrinsic) {
            nir_intrinsic_instr* in = nir_instr_as_intrinsic(instr);
            if (in->intrinsic == nir_intrinsic_deref_implicit_array_length) {
               b.cursor = nir_before_instr(instr);
               nir_deref_instr* d = nir_src_as_deref(in->src[0]);
               nir_def_rewrite_uses(&in->def, nir_imm_int(&b, glsl_get_length(d->type)));
               nir_instr_remove(instr);
            }
            continue;
         }
         if (instr->type != nir_instr_type_tex)
            continue;
         nir_tex_instr* tex = nir_instr_as_tex(instr);
         for (unsigned i = 0; i < tex->num_srcs; i++) {
            nir_tex_src_type t = tex->src[i].src_type;
            if (t != nir_tex_src_sampler_deref_intrinsic && t != nir_tex_src_texture_deref_intrinsic)
               continue;
            nir_intrinsic_instr* in = nir_def_as_intrinsic(tex->src[i].src.ssa);
            nir_def* deref = in->src[0].ssa;
            tex->src[i].src_type = t == nir_tex_src_sampler_deref_intrinsic ? nir_tex_src_sampler_deref : nir_tex_src_texture_deref;
            nir_src_rewrite(&tex->src[i].src, deref);
            if (nir_def_is_unused(&in->def))
               nir_instr_remove(&in->instr);
         }
      }
   }
   nir_progress(true, impl, nir_metadata_control_flow);
   NIR_PASS(_, nir, nir_opt_dce);
}

void glslopt_lower_nir(nir_shader* nir)
{
   NIR_PASS(_, nir, nir_lower_variable_initializers, nir_var_shader_temp | nir_var_function_temp);
   call_globals_wrapper(nir);
   NIR_PASS(_, nir, nir_lower_returns);
   NIR_PASS(_, nir, nir_inline_functions);
   NIR_PASS(_, nir, nir_opt_deref);
   nir_remove_non_entrypoints(nir);
   resolve_texture_srcs(nir);
   NIR_PASS(_, nir, nir_split_var_copies);
   NIR_PASS(_, nir, nir_lower_var_copies);
   NIR_PASS(_, nir, nir_lower_system_values);
   nir_lower_compute_system_values_options csv = {}; // no base workgroup/invocation ids
   NIR_PASS(_, nir, nir_lower_compute_system_values, &csv);
}

void glslopt_optimize_nir(nir_shader* nir)
{
   insert_precision_markers(nir);

   NIR_PASS(_, nir, nir_lower_global_vars_to_local);
   NIR_PASS(_, nir, nir_lower_continue_constructs);

   nir_opt_peephole_select_options peep = {};
   peep.limit = 8;
   peep.expensive_alu_ok = true;

   bool progress;
   int iter = 0;
   do {
      progress = false;
      NIR_PASS(progress, nir, nir_split_var_copies);
      NIR_PASS(progress, nir, nir_lower_var_copies);
      NIR_PASS(progress, nir, nir_split_struct_vars, nir_var_function_temp);
      NIR_PASS(progress, nir, nir_split_array_vars, nir_var_function_temp);
      NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
      NIR_PASS(progress, nir, nir_opt_copy_prop);
      NIR_PASS(progress, nir, nir_opt_remove_phis);
      NIR_PASS(progress, nir, nir_opt_dce);
      NIR_PASS(progress, nir, nir_opt_dead_cf);
      NIR_PASS(progress, nir, nir_opt_cse);
      NIR_PASS(progress, nir, nir_opt_if, (nir_opt_if_options)0);
      NIR_PASS(progress, nir, nir_opt_loop);
      NIR_PASS(progress, nir, nir_opt_peephole_select, &peep);
      NIR_PASS(progress, nir, nir_opt_algebraic);
      NIR_PASS(progress, nir, nir_opt_constant_folding);
      NIR_PASS(progress, nir, nir_opt_undef);
      NIR_PASS(progress, nir, nir_opt_deref);
      NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
      NIR_PASS(progress, nir, nir_opt_dead_write_vars);
      NIR_PASS(progress, nir, nir_opt_combine_stores, nir_var_all);
      NIR_PASS(progress, nir, nir_opt_loop_unroll);
      NIR_PASS(progress, nir, opt_precision_markers);
      NIR_PASS(progress, nir, nir_remove_dead_variables, nir_var_function_temp, NULL);
   } while (progress && ++iter < 50);

   NIR_PASS(_, nir, nir_opt_algebraic_late);
   NIR_PASS(_, nir, nir_opt_constant_folding);
   NIR_PASS(_, nir, nir_opt_copy_prop);
   NIR_PASS(_, nir, nir_opt_dce);
   NIR_PASS(_, nir, nir_opt_cse);
   NIR_PASS(_, nir, nir_opt_shrink_stores, false);
   NIR_PASS(_, nir, nir_remove_dead_variables,
            nir_var_shader_in | nir_var_uniform | nir_var_image | nir_var_mem_ubo | nir_var_mem_ssbo |
            nir_var_mem_shared | nir_var_function_temp, NULL);

   // Move values next to their uses: more single-use, same-block values that
   // the printers can fold into expressions.
   nir_move_options move = (nir_move_options)(nir_move_const_undef | nir_move_load_ubo |
      nir_move_load_input | nir_move_comparisons | nir_move_copies | nir_move_alu);
   NIR_PASS(_, nir, nir_opt_sink, move);
   NIR_PASS(_, nir, nir_opt_move, move);

   NIR_PASS(_, nir, nir_convert_from_ssa, true, false);
   NIR_PASS(_, nir, nir_opt_dce);
   nir_validate_shader(nir, "glsl-optimizer: before printing");
}

std::set<const nir_variable*> glslopt_referenced_vars(nir_shader* nir)
{
   std::set<const nir_variable*> vars;
   nir_foreach_block(block, nir_shader_get_entrypoint(nir))
      nir_foreach_instr(instr, block)
         if (instr->type == nir_instr_type_deref && nir_instr_as_deref(instr)->deref_type == nir_deref_type_var)
            vars.insert(nir_instr_as_deref(instr)->var);
   return vars;
}
