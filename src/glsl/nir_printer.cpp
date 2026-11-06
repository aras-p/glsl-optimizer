#include "nir_printer.h"
#include <algorithm>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

std::string printer_fmt(const char* f, ...)
{
   char buf[4096];
   va_list ap;
   va_start(ap, f);
   vsnprintf(buf, sizeof(buf), f, ap);
   va_end(ap);
   return buf;
}
#define fmt printer_fmt

nir_printer::nir_printer(nir_shader* shader)
   : s(shader)
{
}

std::string nir_printer::print()
{
   impl = nir_shader_get_entrypoint(s);
   prologue();
   nir_index_ssa_defs(impl);
   nir_index_instrs(impl);
   infer_types();
   pi.infer(impl, false);
   expr.assign(impl->ssa_alloc, "");
   name.assign(impl->ssa_alloc, "");
   deps.assign(impl->ssa_alloc, value_deps());

   // registers
   int regn = 0;
   nir_foreach_reg_decl(decl, impl) {
      unsigned nc = nir_intrinsic_num_components(decl);
      std::string nm = fmt("r%d", regn++);
      name[decl->def.index] = nm;
      unsigned p = nir_intrinsic_bit_size(decl) == 16 ? GLSL_PRECISION_MEDIUM : eff(pi.regprec[decl->def.index]);
      BT bt = type[decl->def.index];
      pre_decl(decl_prec(p, bt) + vec_type(bt, nc, p) + " " + nm + local_initializer(bt) + ";");
   }
   // function-temp variables that survived; inlining leaves duplicate and
   // compiler-generated names, so give them unique valid ones
   nir_foreach_variable_in_shader(v, s)
      if (v->name)
         used_names.insert(v->name);
   nir_foreach_function_temp_variable(v, impl)
      local_names[v] = unique_name(v->name);
   nir_foreach_function_temp_variable(v, impl) {
      if (glsl_type_is_void(v->type))
         continue;
      unsigned p = eff(v->data.precision);
      const glsl_type* bare = glsl_without_array(v->type);
      std::string q = glsl_type_is_struct(bare) || glsl_type_is_boolean(bare) ? "" : decl_prec(p, bt_from_glsl(v->type));
      pre_decl(q + type_decl(v->type, v->data.precision, local_names[v]) + ";");
   }

   out.clear();
   emit_cf_list(&impl->body);
   return assemble(pre_decls, out);
}

// Identifiers that are keywords (or reserved) in GLSL or Metal.
static const char* const reserved_words[] = {
   "active", "asm", "atomic_uint", "attribute", "bool", "break", "buffer", "case", "cast", "centroid", "char", "class",
   "coherent", "common", "const", "constant", "continue", "default", "device", "discard", "do", "double", "else",
   "enum", "extern", "external", "false", "filter", "fixed", "flat", "float", "for", "fragment", "goto", "half",
   "highp", "if", "in", "inline", "inout", "input", "int", "interface", "invariant", "kernel", "layout", "long",
   "lowp", "mediump", "namespace", "noinline", "noperspective", "operator", "out", "output", "packed", "partition",
   "patch", "precise", "precision", "private", "protected", "public", "readonly", "resource", "restrict", "return",
   "sample", "sampler", "shared", "short", "signed", "sizeof", "smooth", "static", "struct", "subroutine", "superp",
   "switch", "template", "this", "thread", "threadgroup", "true", "typedef", "uniform", "union", "unsigned", "using",
   "varying", "vertex", "void", "volatile", "while", "writeonly", "texture", "uint", "vec2", "vec3", "vec4",
};

std::string nir_printer::unique_name(const char* base)
{
   std::string b = base && *base ? base : "tmpvar";
   for (char& c : b)
      if (!isalnum((unsigned char)c) && c != '_') c = '_';
   if (isdigit((unsigned char)b[0]))
      b = "v" + b;
   // reserved prefixes, double underscores, our own temporary names
   while (b.find("__") != std::string::npos)
      b.replace(b.find("__"), 2, "_");
   if (!strncmp(b.c_str(), "gl_", 3) || !strncmp(b.c_str(), "_mtl", 4) || b == "_")
      b = "xlat" + b;
   bool tmp_like = b.size() > 1 && (b[0] == 't' || b[0] == 'r');
   for (size_t i = 1; i < b.size() && tmp_like; i++)
      tmp_like = isdigit((unsigned char)b[i]);
   bool reserved = tmp_like;
   for (const char* w : reserved_words)
      reserved |= b == w;
   std::string nm = b;
   for (int n = 1; reserved || used_names.count(nm); n++) {
      nm = b + fmt("_%d", n);
      reserved = false;
   }
   used_names.insert(nm);
   return nm;
}

// ---------------------------------------------------------------------------
// precision

unsigned nir_printer::prec_of(nir_def* d) const
{
   if (d->bit_size == 16)
      return GLSL_PRECISION_MEDIUM;
   return eff(pi.prec[d->index]);
}

// ---------------------------------------------------------------------------
// types

nir_printer::BT nir_printer::bt_from_alu(nir_alu_type t)
{
   switch (nir_alu_type_get_base_type(t)) {
   case nir_type_float: return BT_FLOAT;
   case nir_type_int: return BT_INT;
   case nir_type_uint: return BT_UINT;
   case nir_type_bool: return BT_BOOL;
   default: return BT_UNK;
   }
}

nir_printer::BT nir_printer::bt_from_glsl(const glsl_type* t)
{
   t = glsl_without_array(t);
   if (glsl_type_is_matrix(t))
      return BT_FLOAT;
   switch (glsl_get_base_type(t)) {
   case GLSL_TYPE_FLOAT: case GLSL_TYPE_FLOAT16: case GLSL_TYPE_DOUBLE: return BT_FLOAT;
   case GLSL_TYPE_INT: case GLSL_TYPE_INT16: return BT_INT;
   case GLSL_TYPE_UINT: case GLSL_TYPE_UINT16: case GLSL_TYPE_ATOMIC_UINT: return BT_UINT;
   case GLSL_TYPE_BOOL: return BT_BOOL;
   default: return BT_UNK;
   }
}

bool nir_printer::alu_typeless(nir_op op)
{
   return op == nir_op_mov || op == nir_op_vec2 || op == nir_op_vec3 ||
          op == nir_op_vec4 || op == nir_op_bcsel;
}

nir_printer::BT nir_printer::intrinsic_type(nir_intrinsic_instr* in)
{
   if (glslopt_is_subgroup_value_op(in->intrinsic))
      return type[in->src[0].ssa->index];
   if (glslopt_is_derivative(in->intrinsic))
      return BT_FLOAT;
   switch (in->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
      return bt_from_glsl(nir_src_as_deref(in->src[0])->type);
   case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_point_coord:
      return BT_FLOAT;
   case nir_intrinsic_load_front_face:
   case nir_intrinsic_load_helper_invocation:
   case nir_intrinsic_elect:
      return BT_BOOL;
   case nir_intrinsic_load_vertex_id:
   case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_deref_buffer_array_length:
      return BT_INT;
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_local_invocation_index:
   case nir_intrinsic_load_base_global_invocation_id:
   case nir_intrinsic_load_base_workgroup_id:
   case nir_intrinsic_load_workgroup_size:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_load_subgroup_size:
   case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_num_subgroups:
   case nir_intrinsic_load_subgroup_eq_mask:
   case nir_intrinsic_load_subgroup_ge_mask:
   case nir_intrinsic_load_subgroup_gt_mask:
   case nir_intrinsic_load_subgroup_le_mask:
   case nir_intrinsic_load_subgroup_lt_mask:
   case nir_intrinsic_atomic_counter_read_deref:
   case nir_intrinsic_atomic_counter_inc_deref:
   case nir_intrinsic_atomic_counter_pre_dec_deref:
   case nir_intrinsic_atomic_counter_post_dec_deref:
   case nir_intrinsic_ballot:
   case nir_intrinsic_ballot_bit_count_reduce:
   case nir_intrinsic_ballot_bit_count_inclusive:
   case nir_intrinsic_ballot_bit_count_exclusive:
   case nir_intrinsic_ballot_find_lsb:
   case nir_intrinsic_ballot_find_msb:
      return BT_UINT;
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap: {
      const glsl_type* t = glsl_without_array(nir_src_as_deref(in->src[0])->type);
      return glsl_get_sampler_result_type(t) == GLSL_TYPE_INT ? BT_INT : BT_UINT;
   }
   default:
      if (nir_intrinsic_has_dest_type(in))
         return bt_from_alu(nir_intrinsic_dest_type(in));
      return BT_UNK;
   }
}

// expected type of a source as consumed by its user (UNK if unknown)
nir_printer::BT nir_printer::use_type(nir_src* src)
{
   if (src->ssa->bit_size == 1)
      return BT_BOOL;
   if (nir_src_is_if(src))
      return BT_BOOL;
   nir_instr* u = nir_src_use_instr(src);
   if (u->type == nir_instr_type_alu) {
      nir_alu_instr* alu = nir_instr_as_alu(u);
      return alu_src_type(alu, (nir_alu_src*)container_of(src, nir_alu_src, src) - alu->src);
   }
   if (u->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr* in = nir_instr_as_intrinsic(u);
      if (in->intrinsic == nir_intrinsic_store_deref && src == &in->src[1])
         return bt_from_glsl(nir_src_as_deref(in->src[0])->type);
      if (in->intrinsic == nir_intrinsic_store_reg && src == &in->src[0])
         return type[in->src[1].ssa->index];
      if (in->intrinsic == nir_intrinsic_glslopt_precision)
         return type[in->def.index];
      if (in->intrinsic == nir_intrinsic_deref_atomic || in->intrinsic == nir_intrinsic_deref_atomic_swap)
         return bt_from_glsl(nir_src_as_deref(in->src[0])->type);
      if (in->intrinsic == nir_intrinsic_image_deref_store && src == &in->src[3])
         return bt_from_alu(nir_intrinsic_src_type(in));
      if ((in->intrinsic == nir_intrinsic_image_deref_atomic || in->intrinsic == nir_intrinsic_image_deref_atomic_swap) &&
          (src == &in->src[3] || src == &in->src[4]))
         return type[in->def.index];
      if (src == &in->src[1] && (in->intrinsic == nir_intrinsic_image_deref_load || in->intrinsic == nir_intrinsic_image_deref_store ||
                                 in->intrinsic == nir_intrinsic_image_deref_atomic || in->intrinsic == nir_intrinsic_image_deref_atomic_swap))
         return BT_INT;
      if (glslopt_is_derivative(in->intrinsic))
         return BT_FLOAT;
      if (glslopt_is_subgroup_value_op(in->intrinsic))
         return src == &in->src[0] ? type[in->def.index] : BT_UINT;
      switch (in->intrinsic) {
      case nir_intrinsic_vote_feq:
         return BT_FLOAT;
      case nir_intrinsic_inverse_ballot: case nir_intrinsic_ballot_bitfield_extract:
      case nir_intrinsic_ballot_bit_count_reduce: case nir_intrinsic_ballot_bit_count_inclusive:
      case nir_intrinsic_ballot_bit_count_exclusive: case nir_intrinsic_ballot_find_lsb:
      case nir_intrinsic_ballot_find_msb:
         return BT_UINT;
      default:
         break;
      }
      if (nir_intrinsic_has_src_type(in))
         return bt_from_alu(nir_intrinsic_src_type(in));
      return BT_UNK;
   }
   if (u->type == nir_instr_type_tex) {
      nir_tex_instr* tex = nir_instr_as_tex(u);
      nir_tex_src* ts = (nir_tex_src*)container_of(src, nir_tex_src, src);
      return bt_from_alu(nir_tex_instr_src_type(tex, ts - tex->src));
   }
   if (u->type == nir_instr_type_deref)
      return BT_INT; // array index
   return BT_UNK;
}

nir_printer::BT nir_printer::producer_type(nir_def* def)
{
   if (def->bit_size == 1)
      return BT_BOOL;
   nir_instr* in = nir_def_instr(def);
   switch (in->type) {
   case nir_instr_type_alu: {
      nir_alu_instr* alu = nir_instr_as_alu(in);
      if (alu_typeless(alu->op)) {
         unsigned first = alu->op == nir_op_bcsel ? 1 : 0;
         for (unsigned i = first; i < nir_op_infos[alu->op].num_inputs; i++) {
            nir_def* sd = alu->src[i].src.ssa;
            if (nir_def_instr(sd)->type == nir_instr_type_load_const ||
                nir_def_instr(sd)->type == nir_instr_type_undef)
               continue;
            if (type[sd->index] != BT_UNK)
               return type[sd->index];
         }
         return BT_UNK;
      }
      // GLSL returns int from these, NIR says uint
      if (alu->op == nir_op_bit_count || alu->op == nir_op_ufind_msb ||
          alu->op == nir_op_ifind_msb || alu->op == nir_op_find_lsb)
         return BT_INT;
      return bt_from_alu(nir_op_infos[alu->op].output_type);
   }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr* ii = nir_instr_as_intrinsic(in);
      if (ii->intrinsic == nir_intrinsic_load_reg)
         return type[ii->src[0].ssa->index];
      if (ii->intrinsic == nir_intrinsic_glslopt_precision)
         return type[ii->src[0].ssa->index];
      if (ii->intrinsic == nir_intrinsic_decl_reg) {
         nir_foreach_use(u, def) {
            nir_instr* ui = nir_src_use_instr(u);
            if (ui->type != nir_instr_type_intrinsic) continue;
            nir_intrinsic_instr* st = nir_instr_as_intrinsic(ui);
            if (st->intrinsic == nir_intrinsic_store_reg && type[st->src[0].ssa->index] != BT_UNK &&
                nir_def_instr(st->src[0].ssa)->type != nir_instr_type_load_const)
               return type[st->src[0].ssa->index];
         }
         return BT_UNK;
      }
      return intrinsic_type(ii);
   }
   case nir_instr_type_tex:
      return bt_from_alu(nir_instr_as_tex(in)->dest_type);
   default:
      return BT_UNK;
   }
}

void nir_printer::infer_types()
{
   type.assign(impl->ssa_alloc, BT_UNK);
   // types from producers first; uses only for what that leaves open
   // (registers: the uses of their loads)
   auto pass = [&](bool from_uses) {
      bool changed = false;
      nir_foreach_block(block, impl) {
         nir_foreach_instr(instr, block) {
            nir_def* def = nir_instr_def(instr);
            if (!def || type[def->index] != BT_UNK)
               continue;
            BT t = producer_type(def);
            if (t == BT_UNK && from_uses) {
               nir_foreach_use_including_if(u, def) {
                  if ((t = use_type(u)) != BT_UNK) break;
                  nir_instr* ui = nir_src_is_if(u) ? NULL : nir_src_use_instr(u);
                  if (ui && ui->type == nir_instr_type_intrinsic && nir_instr_as_intrinsic(ui)->intrinsic == nir_intrinsic_load_reg) {
                     nir_foreach_use_including_if(u2, &nir_instr_as_intrinsic(ui)->def)
                        if ((t = use_type(u2)) != BT_UNK) break;
                     if (t != BT_UNK) break;
                  }
               }
            }
            if (t != BT_UNK) {
               type[def->index] = t;
               changed = true;
            }
         }
      }
      return changed;
   };
   for (int i = 0; i < 20 && (pass(false) || pass(true)); i++)
      ;
   for (auto& t : type)
      if (t == BT_UNK)
         t = BT_FLOAT;
}

// ---------------------------------------------------------------------------
// sources

bool nir_printer::is_atom(const std::string& s)
{
   // identifier, member access, array index, function call: can be swizzled
   // / used as operand without extra parens.
   if (s.empty())
      return false;
   if (s[0] == '(') {
      int depth = 0;
      for (size_t i = 0; i < s.size(); i++) {
         if (s[i] == '(') depth++;
         else if (s[i] == ')') { depth--; if (depth == 0 && i != s.size() - 1) return false; }
      }
      return true;
   }
   int depth = 0;
   for (char c : s) {
      if (c == '(' || c == '[') depth++;
      else if (c == ')' || c == ']') depth--;
      else if (depth == 0 && !(isalnum((unsigned char)c) || c == '_' || c == '.'))
         return false;
   }
   return true;
}

std::string nir_printer::float_special(double d)
{
   if (d != d) return "(0.0/0.0)";
   return d > 0 ? "(1.0/0.0)" : "(-1.0/0.0)";
}

std::string nir_printer::const_comp(nir_const_value v, unsigned bit_size, BT bt)
{
   switch (bt) {
   case BT_BOOL:
      return nir_const_value_as_bool(v, bit_size) ? "true" : "false";
   case BT_INT: {
      int64_t i = nir_const_value_as_int(v, bit_size);
      if ((int32_t)i == INT32_MIN)
         return "(-2147483647 - 1)";
      return fmt("%d", (int)i);
   }
   case BT_UINT:
      if (!has_uint())
         return fmt("%d", (int)nir_const_value_as_uint(v, bit_size));
      return fmt("%uu", (unsigned)nir_const_value_as_uint(v, bit_size));
   default: {
      double d = nir_const_value_as_float(v, bit_size);
      if (d != d || d == INFINITY || d == -INFINITY)
         return float_special(d);
      // shortest text that round-trips at the value's precision
      std::string r;
      for (int digits = 1; digits <= 17; digits++) {
         r = fmt("%.*g", digits, d);
         double back = strtod(r.c_str(), NULL);
         if (bit_size == 32 ? (float)back == (float)d : back == d)
            break;
      }
      if (r.find_first_of(".e") == std::string::npos)
         r += ".0";
      return r;
   }
   }
}

std::string nir_printer::const_vals_src(const nir_const_value* vals, unsigned bit_size, const uint8_t* swz, unsigned n,
                                        BT bt, bool scalar_ok, unsigned want_prec)
{
   unsigned p = want_prec ? want_prec : GLSL_PRECISION_HIGH;
   std::vector<std::string> c;
   for (unsigned i = 0; i < n; i++)
      c.push_back(const_comp(vals[swz[i]], bit_size, bt));
   bool same = true;
   for (unsigned i = 1; i < n; i++) same &= c[i] == c[0];
   if (n == 1 || (same && scalar_ok))
      return cast(c[0], bt, GLSL_PRECISION_HIGH, bt, p, 1);
   std::string r = vec_type(bt, n, p) + "(";
   for (unsigned i = 0; i < (same ? 1 : n); i++)
      r += (i ? ", " : "") + c[i];
   return r + ")";
}

std::string nir_printer::cast(const std::string& e, BT from, unsigned from_prec, BT to, unsigned to_prec, unsigned n)
{
   (void)from_prec;
   if (!has_uint()) {
      if (from == BT_UINT) from = BT_INT;
      if (to == BT_UINT) to = BT_INT;
   }
   if (from == to || to == BT_UNK || from == BT_UNK)
      return e;
   if (from == BT_FLOAT && to == BT_INT) return "floatBitsToInt(" + e + ")";
   if (from == BT_FLOAT && to == BT_UINT) return "floatBitsToUint(" + e + ")";
   if (from == BT_INT && to == BT_FLOAT) return "intBitsToFloat(" + e + ")";
   if (from == BT_UINT && to == BT_FLOAT) return "uintBitsToFloat(" + e + ")";
   return vec_type(to, n, to_prec ? to_prec : GLSL_PRECISION_HIGH) + "(" + e + ")";
}

std::string nir_printer::def_ref(nir_def* def)
{
   if (!expr[def->index].empty())
      return expr[def->index];
   if (!name[def->index].empty())
      return name[def->index];
   return fmt("/*undef t%u*/", def->index);
}

std::string nir_printer::src_swz(nir_src& src, const uint8_t* swz, unsigned n, BT want, bool allow_scalar_broadcast,
                                 unsigned want_prec)
{
   nir_def* def = src.ssa;
   nir_instr* pi_ = nir_def_instr(def);
   if (want == BT_UNK)
      want = type[def->index];
   if (pi_->type == nir_instr_type_load_const)
      return const_vals_src(nir_instr_as_load_const(pi_)->value, def->bit_size, swz, n, want, allow_scalar_broadcast,
                            want_prec);
   unsigned dp = prec_of(def);
   if (!want_prec)
      want_prec = dp;
   if (pi_->type == nir_instr_type_undef) {
      std::string z = const_comp(nir_const_value_for_uint(0, 64), 32, want);
      return n == 1 ? cast(z, want, GLSL_PRECISION_HIGH, want, want_prec, 1) : vec_type(want, n, want_prec) + "(" + z + ")";
   }

   std::string e = def_ref(def);
   bool identity = n == def->num_components;
   for (unsigned i = 0; i < n && identity; i++)
      identity = swz[i] == i;
   bool all_same = true;
   for (unsigned i = 1; i < n; i++) all_same &= swz[i] == swz[0];

   if (!identity) {
      if (def->num_components == 1) {
         // scalar replicated
         if (n > 1 && !allow_scalar_broadcast)
            e = vec_type(type[def->index], n, dp) + "(" + e + ")";
      } else {
         if (!is_atom(e)) e = "(" + e + ")";
         e += ".";
         unsigned cnt = (n > 1 && all_same && allow_scalar_broadcast) ? 1 : n;
         for (unsigned i = 0; i < cnt; i++)
            e += "xyzw"[swz[i]];
      }
   }
   unsigned outn = (n > 1 && (def->num_components == 1 || all_same) && allow_scalar_broadcast) ? 1 : n;
   return cast(e, type[def->index], dp, want, want_prec, outn);
}

std::string nir_printer::src_full(nir_src& src, BT want, unsigned want_prec)
{
   static const uint8_t id[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
   return src_swz(src, id, src.ssa->num_components, want, false, want_prec);
}

// type an ALU operand is consumed as
nir_printer::BT nir_printer::alu_src_type(nir_alu_instr* alu, unsigned i)
{
   if (alu->src[i].src.ssa->bit_size == 1)
      return BT_BOOL; // NIR uses "uint" input types for 1-bit iand/ior/inot
   if (alu_typeless(alu->op))
      return alu->op == nir_op_bcsel && i == 0 ? BT_BOOL : type[alu->def.index];
   return bt_from_alu(nir_op_infos[alu->op].input_types[i]);
}

// precision an ALU operand is consumed at: the operation's for math, its own
// for conversions
unsigned nir_printer::alu_src_prec(nir_alu_instr* alu, unsigned i)
{
   if (alu_typeless(alu->op))
      return alu->op == nir_op_bcsel && i == 0 ? 0 : prec_of(&alu->def);
   if (alu->src[i].src.ssa->bit_size == 1)
      return 0;
   BT in = bt_from_alu(nir_op_infos[alu->op].input_types[i]);
   BT outt = bt_from_alu(nir_op_infos[alu->op].output_type);
   // integer ops: NIR mixes int/uint inputs for the same operation
   bool same_kind = in == outt || (in != BT_FLOAT && outt != BT_FLOAT && in != BT_BOOL && outt != BT_BOOL);
   if (in != BT_BOOL && (same_kind || outt == BT_BOOL))
      return outt == BT_BOOL ? eff(pi.prec[alu->def.index]) : prec_of(&alu->def);
   return 0;
}

std::string nir_printer::alu_src(nir_alu_instr* alu, unsigned i, bool allow_broadcast)
{
   unsigned n = nir_ssa_alu_instr_src_components(alu, i);
   return src_swz(alu->src[i].src, alu->src[i].swizzle, n, alu_src_type(alu, i), allow_broadcast, alu_src_prec(alu, i));
}

// ---------------------------------------------------------------------------
// ALU

bool nir_printer::prints_scalar(nir_alu_instr* alu, unsigned i)
{
   unsigned n = nir_ssa_alu_instr_src_components(alu, i);
   if (n == 1)
      return true;
   nir_alu_src& sr = alu->src[i];
   if (sr.src.ssa->num_components == 1 || swz_broadcast(sr, n))
      return true;
   nir_instr* in = nir_def_instr(sr.src.ssa);
   if (in->type != nir_instr_type_load_const)
      return false;
   nir_load_const_instr* lc = nir_instr_as_load_const(in);
   for (unsigned c = 1; c < n; c++)
      if (lc->value[sr.swizzle[c]].u64 != lc->value[sr.swizzle[0]].u64) return false;
   return true;
}

// operands of a vector op, allowing scalar broadcast except that at least
// one must stay a vector (otherwise the result would be scalar-typed)
std::string nir_printer::vop_src(nir_alu_instr* alu, unsigned i, unsigned ninputs)
{
   bool all_scalar = alu->def.num_components > 1;
   for (unsigned k = 0; k < ninputs; k++)
      all_scalar &= prints_scalar(alu, k);
   return alu_src(alu, i, !(all_scalar && i == 0));
}

std::string nir_printer::binop(nir_alu_instr* alu, const char* op)
{
   return "(" + vop_src(alu, 0, 2) + " " + op + " " + vop_src(alu, 1, 2) + ")";
}

std::string nir_printer::call(nir_alu_instr* alu, const std::string& f)
{
   std::string r = f + "(";
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++)
      r += (i ? ", " : "") + alu_src(alu, i);
   return r + ")";
}

std::string nir_printer::cmp(nir_alu_instr* alu, const char* op, const char* vecfn)
{
   if (alu->def.num_components == 1 || !vecfn)
      return binop(alu, op);
   return call(alu, vecfn);
}

std::string nir_printer::componentwise(nir_alu_instr* alu, const char* op, bool is_ternary)
{
   unsigned n = alu->def.num_components;
   std::string r = def_type(&alu->def) + "(";
   for (unsigned c = 0; c < n; c++) {
      auto one = [&](unsigned i) {
         return src_swz(alu->src[i].src, &alu->src[i].swizzle[c], 1, alu_src_type(alu, i), false, alu_src_prec(alu, i));
      };
      if (c) r += ", ";
      if (is_ternary)
         r += "(" + one(0) + " ? " + one(1) + " : " + one(2) + ")";
      else
         r += "(" + one(0) + " " + op + " " + one(1) + ")";
   }
   return r + ")";
}

std::string nir_printer::conv(nir_alu_instr* alu, BT to)
{
   return vec_type(to, alu->def.num_components, prec_of(&alu->def)) + "(" + alu_src(alu, 0) + ")";
}

// does the (folded) expression for this value contain a texture lookup with
// implicit derivatives or a derivative op?
bool nir_printer::has_implicit_derivative(nir_def* d, int depth)
{
   if (depth > 64 || expr[d->index].empty())
      return false; // materialized values were evaluated unconditionally
   nir_instr* in = nir_def_instr(d);
   switch (in->type) {
   case nir_instr_type_tex: {
      nir_texop op = nir_instr_as_tex(in)->op;
      return op == nir_texop_tex || op == nir_texop_txb || op == nir_texop_lod;
   }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_op op = nir_instr_as_intrinsic(in)->intrinsic;
      return op == nir_intrinsic_ddx || op == nir_intrinsic_ddy || op == nir_intrinsic_ddx_fine ||
             op == nir_intrinsic_ddy_fine || op == nir_intrinsic_ddx_coarse || op == nir_intrinsic_ddy_coarse;
   }
   case nir_instr_type_alu: {
      nir_alu_instr* a = nir_instr_as_alu(in);
      for (unsigned i = 0; i < nir_op_infos[a->op].num_inputs; i++)
         if (has_implicit_derivative(a->src[i].src.ssa, depth + 1))
            return true;
      return false;
   }
   default:
      return false;
   }
}

// ---------------------------------------------------------------------------
// print-time pattern recovery
// These only fire when the inner values were folded into this expression
// (single use, same block, not materialized), so nothing is computed twice.

nir_alu_instr* nir_printer::folded_alu(nir_def* d, nir_op op)
{
   nir_instr* in = nir_def_instr(d);
   if (in->type != nir_instr_type_alu || nir_instr_as_alu(in)->op != op)
      return NULL;
   if (expr[d->index].empty() || !list_is_singular(&d->uses))
      return NULL;
   return nir_instr_as_alu(in);
}

bool nir_printer::swz_identity(const nir_alu_src& sr, unsigned n)
{
   for (unsigned i = 0; i < n; i++)
      if (sr.swizzle[i] != i) return false;
   return true;
}

bool nir_printer::swz_broadcast(const nir_alu_src& sr, unsigned n)
{
   for (unsigned i = 1; i < n; i++)
      if (sr.swizzle[i] != sr.swizzle[0]) return false;
   return true;
}

// same value: same SSA def, or two folded reads of the same register (no
// store can sit between them, since stores stop forwarding)
bool nir_printer::same_value(nir_def* a, nir_def* b)
{
   if (a == b)
      return true;
   nir_instr *ia = nir_def_instr(a), *ib = nir_def_instr(b);
   if (ia->type != nir_instr_type_intrinsic || ib->type != nir_instr_type_intrinsic)
      return false;
   nir_intrinsic_instr *la = nir_instr_as_intrinsic(ia), *lb = nir_instr_as_intrinsic(ib);
   return la->intrinsic == nir_intrinsic_load_reg && lb->intrinsic == nir_intrinsic_load_reg &&
          la->src[0].ssa == lb->src[0].ssa && la->instr.block == lb->instr.block &&
          !expr[a->index].empty() && !expr[b->index].empty();
}

bool nir_printer::same_operand(const nir_alu_src& a, const nir_alu_src& b, unsigned n)
{
   if (!same_value(a.src.ssa, b.src.ssa)) return false;
   for (unsigned i = 0; i < n; i++)
      if (a.swizzle[i] != b.swizzle[i]) return false;
   return true;
}

bool nir_printer::const_is(const nir_alu_src& sr, unsigned n, double v, double eps)
{
   nir_instr* in = nir_def_instr(sr.src.ssa);
   if (in->type != nir_instr_type_load_const)
      return false;
   nir_load_const_instr* lc = nir_instr_as_load_const(in);
   for (unsigned i = 0; i < n; i++) {
      double c = nir_const_value_as_float(lc->value[sr.swizzle[i]], lc->def.bit_size);
      if (fabs(c - v) > eps * MAX2(1.0, fabs(v))) return false;
   }
   return true;
}

std::string nir_printer::asrc(nir_alu_instr* alu, unsigned i, unsigned n)
{
   return src_swz(alu->src[i].src, alu->src[i].swizzle, n, BT_FLOAT, false, prec_of(&alu->def));
}

nir_alu_instr* nir_printer::folded_fdot(nir_def* d)
{
   for (nir_op op : {nir_op_fdot2, nir_op_fdot3, nir_op_fdot4})
      if (nir_alu_instr* a = folded_alu(d, op)) return a;
   return NULL;
}

// matrix column: load_deref(M[const k]) with an identity swizzle of n comps
bool nir_printer::mat_column(const nir_alu_src& sr, unsigned n, std::string& mat, const glsl_type*& mt, unsigned& col)
{
   if (!swz_identity(sr, n))
      return false;
   nir_instr* in = nir_def_instr(sr.src.ssa);
   if (in->type != nir_instr_type_intrinsic || nir_instr_as_intrinsic(in)->intrinsic != nir_intrinsic_load_deref)
      return false;
   if (expr[sr.src.ssa->index].empty())
      return false; // materialized; keep using the temp
   nir_deref_instr* d = nir_src_as_deref(nir_instr_as_intrinsic(in)->src[0]);
   if (d->deref_type != nir_deref_type_array || !nir_src_is_const(d->arr.index))
      return false;
   nir_deref_instr* p = nir_deref_instr_parent(d);
   if (!glsl_type_is_matrix(p->type))
      return false;
   // all columns of the matrix must print as plain column reads
   if (expr[sr.src.ssa->index] != deref_str(d))
      return false;
   mat = deref_str(p);
   mt = p->type;
   col = nir_src_as_uint(d->arr.index);
   return true;
}

void nir_printer::flatten_fadd(nir_alu_src& sr, unsigned n, std::vector<nir_alu_src*>& terms)
{
   nir_alu_instr* a = folded_alu(sr, nir_op_fadd);
   if (a && swz_identity(sr, n) && a->def.num_components == n) {
      flatten_fadd(a->src[0], n, terms);
      flatten_fadd(a->src[1], n, terms);
   } else {
      terms.push_back(&sr);
   }
}

// Build a K-component float vector from per-component (src, swizzle comp)
// pairs or literal strings, merging runs from the same value: vec4(v.xyz, 1.0)
std::string nir_printer::build_vec(const std::vector<Coef>& c, unsigned prec)
{
   unsigned K = c.size();
   if (c[0].src) {
      bool whole = true;
      for (unsigned i = 0; i < K && whole; i++)
         whole = c[i].src && c[i].src->src.ssa == c[0].src->src.ssa && c[i].comp == i;
      if (whole && c[0].src->src.ssa->num_components == K) {
         uint8_t sw[4] = {0, 1, 2, 3};
         return src_swz(c[0].src->src, sw, K, BT_FLOAT, false, prec);
      }
   }
   if (K == 1)
      return c[0].src ? src_swz(c[0].src->src, &c[0].comp, 1, BT_FLOAT, false, prec)
                      : cast(c[0].lit, BT_FLOAT, GLSL_PRECISION_HIGH, BT_FLOAT, prec, 1);
   std::string r = vec_type(BT_FLOAT, K, prec) + "(";
   for (unsigned i = 0; i < K;) {
      if (i) r += ", ";
      if (!c[i].src) { r += c[i].lit; i++; continue; }
      unsigned j = i + 1;
      uint8_t sw[4];
      sw[0] = c[i].comp;
      while (j < K && c[j].src && c[j].src->src.ssa == c[i].src->src.ssa) { sw[j - i] = c[j].comp; j++; }
      r += src_swz(c[i].src->src, sw, j - i, BT_FLOAT, false, prec);
      i = j;
   }
   return r + ")";
}

std::string nir_printer::array_dims(const glsl_type*& t, bool allow_unsized)
{
   std::string dims;
   while (glsl_type_is_array(t)) {
      dims += glsl_type_is_unsized_array(t) && allow_unsized ? std::string("[]") : fmt("[%u]", MAX2(glsl_get_length(t), 1u));
      t = glsl_get_array_element(t);
   }
   return dims;
}

std::string nir_printer::mat_ctor(unsigned cols, unsigned rows, unsigned prec)
{
   (void)prec;
   return cols == rows ? fmt("mat%u", cols) : fmt("mat%ux%u", cols, rows);
}

// sum of M[k] * v.k  ->  M * v
std::string nir_printer::match_mat_mul_vec(nir_alu_instr* alu)
{
   unsigned n = alu->def.num_components;
   if (n < 2)
      return "";
   std::vector<nir_alu_src*> terms;
   flatten_fadd(alu->src[0], n, terms);
   flatten_fadd(alu->src[1], n, terms);
   if (terms.size() < 2)
      return "";
   std::string mat;
   const glsl_type* mt = NULL;
   unsigned mprec = GLSL_PRECISION_HIGH;
   std::map<unsigned, Coef> coefs;
   for (nir_alu_src* t : terms) {
      std::string m; const glsl_type* tt; unsigned col;
      Coef c;
      unsigned cp;
      if (mat_column(*t, n, m, tt, col)) {
         c.lit = "1.0";
         cp = prec_of(t->src.ssa);
      } else {
         nir_alu_instr* mul = folded_alu(*t, nir_op_fmul);
         if (!mul || !swz_identity(*t, n))
            return "";
         int j = mat_column(mul->src[0], n, m, tt, col) ? 0 : (mat_column(mul->src[1], n, m, tt, col) ? 1 : -1);
         if (j < 0 || !swz_broadcast(mul->src[1 - j], n))
            return "";
         cp = prec_of(mul->src[j].src.ssa);
         nir_instr* ci = nir_def_instr(mul->src[1 - j].src.ssa);
         if (ci->type == nir_instr_type_load_const) {
            c.lit = const_comp(nir_instr_as_load_const(ci)->value[mul->src[1 - j].swizzle[0]],
                               mul->src[1 - j].src.ssa->bit_size, BT_FLOAT);
         } else {
            c.src = &mul->src[1 - j];
            c.comp = mul->src[1 - j].swizzle[0];
         }
      }
      if (mt && (tt != mt || m != mat))
         return "";
      if (coefs.count(col))
         return "";
      mat = m; mt = tt; mprec = cp;
      coefs[col] = c;
   }
   unsigned C = glsl_get_matrix_columns(mt), R = glsl_get_vector_elements(mt);
   unsigned K;
   std::string matexpr = mat, post;
   if (n == R) {
      K = C;
   } else if (C == R && coefs.rbegin()->first < n) {
      K = n; // mat3(M) * v
      std::string c = matrix_casts() ? mat_ctor(n, n, mprec) : "";
      if (c.empty())
         return "";
      matexpr = c + "(" + mat + ")";
   } else {
      K = C;
      post = std::string(".") + std::string("xyzw").substr(0, n);
   }
   for (auto& kv : coefs)
      if (kv.first >= K) return "";
   std::vector<Coef> v(K);
   for (unsigned k = 0; k < K; k++) {
      if (coefs.count(k)) v[k] = coefs[k];
      else v[k].lit = "0.0";
   }
   std::string r = "(" + matexpr + " * " + build_vec(v, mprec) + ")" + post;
   return cast(r, BT_FLOAT, mprec, BT_FLOAT, prec_of(&alu->def), n);
}

// c0 * v.x + c1 * v.y + c2 * v.z (+ c3 * v.w)  ->  matKxN(c0, c1, ...) * v
// for arbitrary column vectors (e.g. a transposed matrix built from rows)
std::string nir_printer::match_generic_mat_mul(nir_alu_instr* alu)
{
   unsigned n = alu->def.num_components;
   if (n < 2)
      return "";
   std::vector<nir_alu_src*> terms;
   flatten_fadd(alu->src[0], n, terms);
   flatten_fadd(alu->src[1], n, terms);
   unsigned K = terms.size();
   if (K < 2 || K > 4)
      return "";
   std::vector<std::string> cols(K);
   nir_alu_src* v = NULL;
   for (nir_alu_src* t : terms) {
      nir_alu_instr* mul = folded_alu(*t, nir_op_fmul);
      if (!mul || !swz_identity(*t, n))
         return "";
      int j = -1;
      for (int sd = 0; sd < 2 && j < 0; sd++) {
         nir_alu_src& cs = mul->src[1 - sd];
         if (swz_broadcast(cs, n) && nir_def_instr(cs.src.ssa)->type != nir_instr_type_load_const &&
             cs.src.ssa->num_components > 1 && (!v || v->src.ssa == cs.src.ssa))
            j = sd;
      }
      if (j < 0)
         return "";
      nir_alu_src& cs = mul->src[1 - j];
      unsigned k = cs.swizzle[0];
      if (k >= K || !cols[k].empty() || mul->src[j].src.ssa->num_components < n)
         return "";
      v = &cs;
      cols[k] = asrc(mul, j, n);
   }
   std::string r = mat_ctor(K, n, prec_of(&alu->def));
   if (r.empty())
      return "";
   r += "(";
   for (unsigned k = 0; k < K; k++)
      r += (k ? ", " : "") + cols[k];
   uint8_t sw[4] = {0, 1, 2, 3};
   return "(" + r + ") * " + src_swz(v->src, sw, K, BT_FLOAT, false, prec_of(&alu->def)) + ")";
}

// vecN(dot(v, M[0]), dot(v, M[1]), ...)  ->  v * M
std::string nir_printer::match_vec_mul_mat(nir_alu_instr* alu)
{
   unsigned n = alu->def.num_components;
   std::string mat;
   const glsl_type* mt = NULL;
   nir_alu_src* vsrc = NULL;
   unsigned rows = 0, mprec = GLSL_PRECISION_HIGH;
   for (unsigned k = 0; k < n; k++) {
      if (alu->src[k].swizzle[0] != 0)
         return "";
      nir_alu_instr* d = folded_fdot(alu->src[k].src.ssa);
      if (!d)
         return "";
      unsigned dn = nir_ssa_alu_instr_src_components(d, 0);
      std::string m; const glsl_type* tt; unsigned col;
      int j = mat_column(d->src[0], dn, m, tt, col) ? 0 : (mat_column(d->src[1], dn, m, tt, col) ? 1 : -1);
      if (j < 0 || col != k || (mt && (tt != mt || m != mat)))
         return "";
      if (vsrc && !same_operand(*vsrc, d->src[1 - j], dn))
         return "";
      mat = m; mt = tt; rows = dn;
      mprec = prec_of(d->src[j].src.ssa);
      vsrc = &d->src[1 - j];
   }
   if (!mt || n != glsl_get_matrix_columns(mt) || rows != glsl_get_vector_elements(mt))
      return "";
   std::string r = "(" + src_swz(vsrc->src, vsrc->swizzle, rows, BT_FLOAT, false, mprec) + " * " + mat + ")";
   return cast(r, BT_FLOAT, mprec, BT_FLOAT, prec_of(&alu->def), n);
}

// dot(x, x) with the given operand
bool nir_printer::is_self_dot(nir_alu_instr* d, const nir_alu_src* x, unsigned n)
{
   if (!d || nir_ssa_alu_instr_src_components(d, 0) != n)
      return false;
   if (!same_operand(d->src[0], d->src[1], n))
      return false;
   return !x || same_operand(d->src[0], *x, n);
}

std::string nir_printer::match_builtins(nir_alu_instr* alu)
{
   unsigned n = alu->def.num_components;
   unsigned p = prec_of(&alu->def);
   switch (alu->op) {
   case nir_op_fmul:
      // normalize: x * inversesqrt(dot(x, x))
      for (unsigned j = 0; j < 2 && n > 1; j++) {
         nir_alu_instr* rsq = folded_alu(alu->src[1 - j], nir_op_frsq);
         if (!rsq || !swz_broadcast(alu->src[1 - j], n) || alu->src[1 - j].swizzle[0] != 0)
            continue;
         if (is_self_dot(folded_fdot(rsq->src[0].src.ssa), &alu->src[j], n))
            return "normalize(" + asrc(alu, j, n) + ")";
      }
      // log(x): log2(x) * ln(2)
      for (unsigned j = 0; j < 2; j++) {
         nir_alu_instr* lg = folded_alu(alu->src[j], nir_op_flog2);
         if (lg && swz_identity(alu->src[j], n) && const_is(alu->src[1 - j], n, 0.69314718))
            return "log(" + asrc(lg, 0, n) + ")";
      }
      break;
   case nir_op_fsqrt: {
      // length(x)
      nir_alu_instr* d = folded_fdot(alu->src[0].src.ssa);
      if (d && is_self_dot(d, NULL, nir_ssa_alu_instr_src_components(d, 0))) {
         unsigned dn = nir_ssa_alu_instr_src_components(d, 0);
         return "length(" + src_swz(d->src[0].src, d->src[0].swizzle, dn, BT_FLOAT, false, p) + ")";
      }
      break;
   }
   case nir_op_fexp2: {
      // exp(x): exp2(x * log2(e))
      nir_alu_instr* m = folded_alu(alu->src[0], nir_op_fmul);
      if (m && swz_identity(alu->src[0], n))
         for (unsigned j = 0; j < 2; j++)
            if (const_is(m->src[1 - j], n, 1.44269504))
               return "exp(" + asrc(m, j, n) + ")";
      break;
   }
   case nir_op_fsub: {
      // reflect(I, N): I - 2 * dot(N, I) * N
      nir_alu_instr* m = folded_alu(alu->src[1], nir_op_fmul);
      if (!m || n < 2 || !swz_identity(alu->src[1], n))
         break;
      for (unsigned j = 0; j < 2; j++) {
         nir_alu_instr* m2 = folded_alu(m->src[1 - j], nir_op_fmul);
         if (!m2 || !swz_broadcast(m->src[1 - j], n))
            continue;
         for (unsigned k = 0; k < 2; k++) {
            if (!const_is(m2->src[1 - k], 1, 2.0))
               continue;
            nir_alu_instr* d = folded_fdot(m2->src[k].src.ssa);
            if (!d || nir_ssa_alu_instr_src_components(d, 0) != n)
               continue;
            const nir_alu_src &N = m->src[j], &I = alu->src[0];
            if ((same_operand(d->src[0], N, n) && same_operand(d->src[1], I, n)) ||
                (same_operand(d->src[1], N, n) && same_operand(d->src[0], I, n)))
               return "reflect(" + asrc(alu, 0, n) + ", " + asrc(m, j, n) + ")";
         }
      }
      break;
   }
   case nir_op_fmin:
   case nir_op_fmax: {
      // clamp(x, lo, hi)
      nir_op inner = alu->op == nir_op_fmin ? nir_op_fmax : nir_op_fmin;
      for (unsigned j = 0; j < 2; j++) {
         nir_alu_instr* a = folded_alu(alu->src[j], inner);
         if (!a || !swz_identity(alu->src[j], n))
            continue;
         for (unsigned k = 0; k < 2; k++) {
            nir_instr* ci = nir_def_instr(a->src[1 - k].src.ssa);
            nir_instr* co = nir_def_instr(alu->src[1 - j].src.ssa);
            // only with constant bounds, so lo/hi order is evident
            if (ci->type != nir_instr_type_load_const || co->type != nir_instr_type_load_const)
               continue;
            // clamp() is only defined for lo <= hi
            nir_load_const_instr *li = nir_instr_as_load_const(ci), *lo_ = nir_instr_as_load_const(co);
            bool ok = true;
            for (unsigned c = 0; c < n; c++) {
               double iv = nir_const_value_as_float(li->value[a->src[1 - k].swizzle[c]], li->def.bit_size);
               double ov = nir_const_value_as_float(lo_->value[alu->src[1 - j].swizzle[c]], lo_->def.bit_size);
               ok &= alu->op == nir_op_fmin ? iv <= ov : ov <= iv;
            }
            if (!ok)
               continue;
            std::string x = src_swz(a->src[k].src, a->src[k].swizzle, n, BT_FLOAT, false, p);
            std::string ib = src_swz(a->src[1 - k].src, a->src[1 - k].swizzle, n, BT_FLOAT, true, p);
            std::string ob = src_swz(alu->src[1 - j].src, alu->src[1 - j].swizzle, n, BT_FLOAT, true, p);
            return alu->op == nir_op_fmin ? "clamp(" + x + ", " + ib + ", " + ob + ")"
                                          : "clamp(" + x + ", " + ob + ", " + ib + ")";
         }
      }
      break;
   }
   case nir_op_flrp: {
      // mix(a, b, t) with constant a, b: NIR's rewrite of t*k+c; print it that way
      nir_instr *ai = nir_def_instr(alu->src[0].src.ssa), *bi = nir_def_instr(alu->src[1].src.ssa);
      if (ai->type != nir_instr_type_load_const || bi->type != nir_instr_type_load_const)
         break;
      nir_load_const_instr *la = nir_instr_as_load_const(ai), *lb = nir_instr_as_load_const(bi);
      if (la->def.bit_size != 32 || lb->def.bit_size != 32)
         break;
      double a0 = la->value[alu->src[0].swizzle[0]].f32, b0 = lb->value[alu->src[1].swizzle[0]].f32;
      bool uniform = true;
      for (unsigned c = 1; c < n; c++)
         uniform &= la->value[alu->src[0].swizzle[c]].f32 == a0 && lb->value[alu->src[1].swizzle[c]].f32 == b0;
      if (!uniform)
         break;
      std::string t = alu_src(alu, 2, !(n > 1 && prints_scalar(alu, 2)));
      double k = b0 - a0;
      auto lit = [&](double v) { return cast(const_comp(nir_const_value_for_float(v, 32), 32, BT_FLOAT),
                                             BT_FLOAT, GLSL_PRECISION_HIGH, BT_FLOAT, p, 1); };
      std::string r = k == 1.0 ? t : "(" + t + " * " + lit(k) + ")";
      if (a0 == 0.0) return r;
      return "(" + r + (a0 < 0 ? " - " : " + ") + lit(fabs(a0)) + ")";
   }
   case nir_op_b2f32: {
      // step(edge, x): float(x >= edge)
      nir_alu_instr* c = folded_alu(alu->src[0], nir_op_fge);
      if (c && swz_identity(alu->src[0], n))
         return "step(" + src_swz(c->src[1].src, c->src[1].swizzle, n, BT_FLOAT, true, p) + ", " +
                src_swz(c->src[0].src, c->src[0].swizzle, n, BT_FLOAT, false, p) + ")";
      break;
   }
   default:
      break;
   }
   return "";
}

std::string nir_printer::vec_ctor(nir_alu_instr* alu)
{
   unsigned n = alu->def.num_components;
   BT bt = type[alu->def.index];
   unsigned p = prec_of(&alu->def);
   std::string r = vec_type(bt, n, p) + "(";
   unsigned i = 0;
   bool first = true;
   while (i < n) {
      // merge run of components from the same non-constant def
      nir_def* d = alu->src[i].src.ssa;
      unsigned j = i + 1;
      bool isconst = nir_def_instr(d)->type == nir_instr_type_load_const;
      if (!isconst)
         while (j < n && alu->src[j].src.ssa == d) j++;
      uint8_t swz[4];
      for (unsigned k = i; k < j; k++) swz[k - i] = alu->src[k].swizzle[0];
      if (!first) r += ", ";
      first = false;
      std::string c = src_swz(alu->src[i].src, swz, j - i, bt, false, p);
      r += c;
      i = j;
   }
   return r + ")";
}

std::string nir_printer::alu_expr(nir_alu_instr* alu)
{
   unsigned n = alu->def.num_components;
   bool isbool = alu->def.bit_size == 1;
   if (type[alu->def.index] == BT_FLOAT) {
      std::string p;
      if (alu->op == nir_op_fadd) {
         p = match_mat_mul_vec(alu);
         if (p.empty())
            p = match_generic_mat_mul(alu);
      }
      else if (alu->op == nir_op_vec2 || alu->op == nir_op_vec3 || alu->op == nir_op_vec4)
         p = match_vec_mul_mat(alu);
      if (p.empty())
         p = match_builtins(alu);
      if (!p.empty())
         return p;
   }
   std::string d = alu_dialect(alu);
   if (!d.empty())
      return d;
   switch (alu->op) {
   case nir_op_mov: return alu_src(alu, 0);
   case nir_op_vec2: case nir_op_vec3: case nir_op_vec4: return vec_ctor(alu);
   case nir_op_fneg: case nir_op_ineg: return "(-" + alu_src(alu, 0) + ")";
   case nir_op_fabs: case nir_op_iabs: return call(alu, "abs");
   case nir_op_fsign: case nir_op_isign: return call(alu, "sign");
   case nir_op_ffloor: return call(alu, "floor");
   case nir_op_fceil: return call(alu, "ceil");
   case nir_op_ftrunc: return call(alu, fn("trunc"));
   case nir_op_fround_even: return call(alu, fn("roundEven"));
   case nir_op_ffract: return call(alu, "fract");
   case nir_op_frcp: {
      std::string one = cast("1.0", BT_FLOAT, GLSL_PRECISION_HIGH, BT_FLOAT, prec_of(&alu->def), 1);
      return "(" + one + " / " + alu_src(alu, 0) + ")";
   }
   case nir_op_frsq: return call(alu, fn("inversesqrt"));
   case nir_op_fsqrt: return call(alu, "sqrt");
   case nir_op_fexp2: return call(alu, "exp2");
   case nir_op_flog2: return call(alu, "log2");
   case nir_op_fsin: return call(alu, "sin");
   case nir_op_fcos: return call(alu, "cos");
   case nir_op_fpow: return call(alu, "pow");
   case nir_op_fmin: case nir_op_imin: case nir_op_umin: return call(alu, "min");
   case nir_op_fmax: case nir_op_imax: case nir_op_umax: return call(alu, "max");
   case nir_op_fsat: return call(alu, fn("saturate"));
   case nir_op_flrp: return call(alu, "mix");
   case nir_op_fmod: return call(alu, fn("mod"));
   case nir_op_fdot2: case nir_op_fdot3: case nir_op_fdot4: return call(alu, "dot");
   case nir_op_fadd: case nir_op_iadd: {
      // a + -c  ->  a - c
      nir_instr* ci = nir_def_instr(alu->src[1].src.ssa);
      if (ci->type == nir_instr_type_load_const) {
         nir_load_const_instr* lc = nir_instr_as_load_const(ci);
         bool neg = true;
         unsigned nc = nir_ssa_alu_instr_src_components(alu, 1);
         for (unsigned c = 0; c < nc && neg; c++) {
            nir_const_value v = lc->value[alu->src[1].swizzle[c]];
            neg = alu->op == nir_op_fadd ? nir_const_value_as_float(v, lc->def.bit_size) < 0
                                         : (lc->def.bit_size == 32 && type[alu->def.index] == BT_INT &&
                                            nir_const_value_as_int(v, 32) < 0 && nir_const_value_as_int(v, 32) != INT32_MIN);
         }
         if (neg) {
            std::vector<nir_const_value> negv(lc->def.num_components);
            for (unsigned c2 = 0; c2 < lc->def.num_components; c2++) {
               if (alu->op == nir_op_fadd)
                  negv[c2] = nir_const_value_for_float(-nir_const_value_as_float(lc->value[c2], lc->def.bit_size), lc->def.bit_size);
               else
                  negv[c2] = nir_const_value_for_int(-nir_const_value_as_int(lc->value[c2], lc->def.bit_size), lc->def.bit_size);
            }
            BT want = bt_from_alu(nir_op_infos[alu->op].input_types[1]);
            if (want == BT_INT && type[alu->def.index] != BT_INT) want = type[alu->def.index];
            std::string pos = const_vals_src(negv.data(), lc->def.bit_size, alu->src[1].swizzle, nc,
                                             want == BT_UNK ? type[alu->def.index] : want, true, alu_src_prec(alu, 1));
            return "(" + vop_src(alu, 0, 2) + " - " + pos + ")";
         }
      }
      return binop(alu, "+");
   }
   case nir_op_fsub: case nir_op_isub: return binop(alu, "-");
   case nir_op_fmul: case nir_op_imul: return binop(alu, "*");
   case nir_op_fdiv: case nir_op_idiv: case nir_op_udiv: return binop(alu, "/");
   case nir_op_irem: case nir_op_umod: return binop(alu, "%");
   case nir_op_ffma: case nir_op_fmad: case nir_op_ffma_weak:
      return "((" + vop_src(alu, 0, 3) + " * " + vop_src(alu, 1, 3) + ") + " + vop_src(alu, 2, 3) + ")";
   case nir_op_feq: case nir_op_ieq: return cmp(alu, "==", "equal");
   case nir_op_fneu: case nir_op_ine: return cmp(alu, "!=", "notEqual");
   case nir_op_flt: case nir_op_ilt: case nir_op_ult: return cmp(alu, "<", "lessThan");
   case nir_op_fge: case nir_op_ige: case nir_op_uge: return cmp(alu, ">=", "greaterThanEqual");
   case nir_op_ball_fequal2: case nir_op_ball_fequal3: case nir_op_ball_fequal4:
   case nir_op_ball_iequal2: case nir_op_ball_iequal3: case nir_op_ball_iequal4:
      return "(" + alu_src(alu, 0) + " == " + alu_src(alu, 1) + ")";
   case nir_op_bany_fnequal2: case nir_op_bany_fnequal3: case nir_op_bany_fnequal4:
   case nir_op_bany_inequal2: case nir_op_bany_inequal3: case nir_op_bany_inequal4:
      return "(" + alu_src(alu, 0) + " != " + alu_src(alu, 1) + ")";
   case nir_op_bcsel:
      if (alu->src[0].src.ssa->num_components == 1 || n == 1) {
         uint8_t s0 = alu->src[0].swizzle[0];
         std::string c = src_swz(alu->src[0].src, &s0, 1, BT_BOOL, false);
         // GLSL ?: evaluates only one side; NIR bcsel evaluates both. If a
         // side has an implicit-derivative op (texture(), dFdx) it must not
         // become conditional: use mix(), whose arguments are all evaluated.
         if (type[alu->def.index] == BT_FLOAT &&
             (has_implicit_derivative(alu->src[1].src.ssa) || has_implicit_derivative(alu->src[2].src.ssa))) {
            if (n > 1)
               c = vec_type(BT_BOOL, n, GLSL_PRECISION_HIGH) + "(" + c + ")";
            return "mix(" + alu_src(alu, 2) + ", " + alu_src(alu, 1) + ", " + c + ")";
         }
         return "(" + c + " ? " + alu_src(alu, 1) + " : " + alu_src(alu, 2) + ")";
      }
      if (type[alu->def.index] == BT_FLOAT)
         return "mix(" + alu_src(alu, 2) + ", " + alu_src(alu, 1) + ", " + alu_src(alu, 0) + ")";
      return componentwise(alu, "", true);
   case nir_op_iand:
      if (isbool) return n == 1 ? binop(alu, "&&") : componentwise(alu, "&&");
      return binop(alu, "&");
   case nir_op_ior:
      if (isbool) return n == 1 ? binop(alu, "||") : componentwise(alu, "||");
      return binop(alu, "|");
   case nir_op_ixor:
      if (isbool) return n == 1 ? binop(alu, "^^") : call(alu, "notEqual");
      return binop(alu, "^");
   case nir_op_inot:
      if (isbool) return n == 1 ? "(!" + alu_src(alu, 0) + ")" : call(alu, "not");
      return "(~" + alu_src(alu, 0) + ")";
   case nir_op_ishl: return binop(alu, "<<");
   case nir_op_ishr: case nir_op_ushr: return binop(alu, ">>");
   case nir_op_b2f32: case nir_op_b2f16: case nir_op_i2f32: case nir_op_u2f32:
   case nir_op_i2f16: case nir_op_u2f16:
      return conv(alu, BT_FLOAT);
   case nir_op_b2i32: case nir_op_f2i32: case nir_op_f2i16: case nir_op_u2u32: return conv(alu, BT_INT);
   case nir_op_f2u32: case nir_op_f2u16: return conv(alu, BT_UINT);
   case nir_op_f2f32: case nir_op_f2f16: case nir_op_f2fmp: case nir_op_f2f16_rtne: case nir_op_f2f16_rtz:
   case nir_op_i2i32: case nir_op_i2i16: case nir_op_u2u16: case nir_op_i2imp: case nir_op_b2b1:
      return alu_src(alu, 0);
   case nir_op_bit_count: return call(alu, "bitCount");
   case nir_op_ubitfield_extract: case nir_op_ibitfield_extract:
      // offset and bits are scalars in GLSL
      return fn("bitfieldExtract") + "(" + alu_src(alu, 0) + ", " + alu_src(alu, 1, true) + ", " + alu_src(alu, 2, true) + ")";
   case nir_op_bitfield_insert:
      return fn("bitfieldInsert") + "(" + alu_src(alu, 0) + ", " + alu_src(alu, 1) + ", " + alu_src(alu, 2, true) + ", " +
             alu_src(alu, 3, true) + ")";
   case nir_op_bitfield_reverse: return call(alu, "bitfieldReverse");
   case nir_op_ufind_msb: case nir_op_ifind_msb: return call(alu, "findMSB");
   case nir_op_find_lsb: return call(alu, "findLSB");
   case nir_op_pack_half_2x16: return call(alu, "packHalf2x16");
   case nir_op_unpack_half_2x16: return call(alu, "unpackHalf2x16");
   case nir_op_pack_unorm_2x16: return call(alu, "packUnorm2x16");
   case nir_op_pack_snorm_2x16: return call(alu, "packSnorm2x16");
   case nir_op_unpack_unorm_2x16: return call(alu, "unpackUnorm2x16");
   case nir_op_unpack_snorm_2x16: return call(alu, "unpackSnorm2x16");
   case nir_op_pack_unorm_4x8: return call(alu, "packUnorm4x8");
   case nir_op_pack_snorm_4x8: return call(alu, "packSnorm4x8");
   case nir_op_unpack_unorm_4x8: return call(alu, "unpackUnorm4x8");
   case nir_op_unpack_snorm_4x8: return call(alu, "unpackSnorm4x8");
   default:
      return std::string("/*UNHANDLED*/") + call(alu, nir_op_infos[alu->op].name);
   }
}

// ---------------------------------------------------------------------------
// derefs

std::string nir_printer::deref_str(nir_deref_instr* d)
{
   switch (d->deref_type) {
   case nir_deref_type_var:
      if (d->var->data.mode == nir_var_function_temp && local_names.count(d->var))
         return local_names[d->var];
      return var_ref(d->var);
   case nir_deref_type_struct: {
      nir_deref_instr* p = nir_deref_instr_parent(d);
      const glsl_type* pt = p->type;
      // unnamed interface block: the variable is the block, members are bare names
      if (p->deref_type == nir_deref_type_var && p->var->interface_type == pt &&
          strcmp(p->var->name, glsl_get_type_name(pt)) == 0)
         return glsl_get_struct_elem_name(pt, d->strct.index);
      return deref_str(p) + "." + glsl_get_struct_elem_name(pt, d->strct.index);
   }
   case nir_deref_type_array:
      return deref_str(nir_deref_instr_parent(d)) + "[" + src_full(d->arr.index, BT_INT) + "]";
   default:
      return "/*UNHANDLED deref*/";
   }
}

bool nir_printer::deref_is_mutable(nir_deref_instr* d)
{
   nir_variable* v = nir_deref_instr_get_variable(d);
   if (!v) return true;
   return !(v->data.mode & (nir_var_shader_in | nir_var_uniform | nir_var_mem_ubo | nir_var_system_value));
}

// ---------------------------------------------------------------------------
// statements

bool nir_printer::has_side_effect(nir_instr* in)
{
   switch (in->type) {
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr* ii = nir_instr_as_intrinsic(in);
      return !(nir_intrinsic_infos[ii->intrinsic].flags & NIR_INTRINSIC_CAN_ELIMINATE);
   }
   case nir_instr_type_call:
   case nir_instr_type_jump:
      return true;
   default:
      return false;
   }
}

// Can all uses of def see an expression evaluated now? (same block, no
// intervening side effects if the value reads mutable state)
// Derefs are printed where they are used: find the latest instruction that
// evaluates a use (in block blk; false if one is elsewhere).
static bool
latest_eval_point(nir_instr* ui, nir_block* blk, nir_instr** last)
{
   if (ui->type == nir_instr_type_deref) {
      nir_foreach_use(u, &nir_instr_as_deref(ui)->def)
         if (!latest_eval_point(nir_src_use_instr(u), blk, last))
            return false;
      return true;
   }
   if (ui->block != blk)
      return false;
   if (!*last || ui->index > (*last)->index)
      *last = ui;
   return true;
}

// dependencies of a source value; derefs depend on their array indices
void nir_printer::add_deps(nir_def* d, value_deps& out)
{
   nir_instr* in = nir_def_instr(d);
   if (in->type == nir_instr_type_deref) {
      for (nir_deref_instr* p = nir_instr_as_deref(in); p; p = nir_deref_instr_parent(p))
         if (p->deref_type == nir_deref_type_array)
            add_deps(p->arr.index.ssa, out);
      return;
   }
   const value_deps& s = deps[d->index];
   out.mem |= s.mem;
   for (unsigned r : s.regs)
      if (std::find(out.regs.begin(), out.regs.end(), r) == out.regs.end())
         out.regs.push_back(r);
}

// would executing `it` change what d reads?
bool nir_printer::conflicts(const value_deps& d, nir_instr* it)
{
   if (it->type == nir_instr_type_intrinsic && nir_instr_as_intrinsic(it)->intrinsic == nir_intrinsic_store_reg) {
      unsigned r = nir_instr_as_intrinsic(it)->src[1].ssa->index;
      return std::find(d.regs.begin(), d.regs.end(), r) != d.regs.end();
   }
   return d.mem && has_side_effect(it);
}

bool nir_printer::can_forward(nir_def* def, unsigned max_uses)
{
   unsigned count = 0;
   nir_instr* di = nir_def_instr(def);
   nir_instr* last = NULL;
   bool cond_use = false;
   nir_foreach_use_including_if(u, def) {
      count++;
      if (nir_src_get_block(u) != di->block)
         return false;
      if (nir_src_is_if(u)) {
         cond_use = true;
         continue;
      }
      if (!latest_eval_point(nir_src_use_instr(u), di->block, &last))
         return false;
   }
   if (count == 0 || count > max_uses)
      return false;
   // a use as an if condition is evaluated at the end of the block
   const value_deps& d = deps[def->index];
   if (d.mem || !d.regs.empty())
      for (nir_instr* it = nir_instr_next(di); it && (cond_use || it != last); it = nir_instr_next(it))
         if (conflicts(d, it)) return false;
   return true;
}

// is the def used in a scope outside the cf list where it is defined?
bool nir_printer::escapes_scope(nir_def* def)
{
   nir_cf_node* defp = nir_def_instr(def)->block->cf_node.parent;
   nir_foreach_use_including_if(u, def) {
      nir_cf_node* n = &nir_src_get_block(u)->cf_node;
      bool found = false;
      for (nir_cf_node* p = n->parent; p; p = p->parent)
         if (p == defp) { found = true; break; }
      if (!found) return true;
   }
   return false;
}

bool nir_printer::only_used_by_store(nir_def* def)
{
   nir_foreach_use_including_if(u, def) {
      if (nir_src_is_if(u)) return false;
      nir_instr* ui = nir_src_use_instr(u);
      if (ui->type != nir_instr_type_intrinsic) return false;
      nir_intrinsic_op op = nir_instr_as_intrinsic(ui)->intrinsic;
      if (op != nir_intrinsic_store_deref && op != nir_intrinsic_store_reg) return false;
   }
   return true;
}

std::string nir_printer::new_temp(nir_def* def, const std::string& e)
{
   std::string nm = fmt("t%d", tmp_counter++);
   name[def->index] = nm;
   unsigned p = prec_of(def);
   std::string decl = decl_prec(p, type[def->index]) + def_type(def);
   if (escapes_scope(def)) {
      pre_decl(decl + " " + nm + local_initializer(type[def->index]) + ";");
      line(nm + " = " + e + ";");
   } else {
      line(decl + " " + nm + " = " + e + ";");
   }
   return nm;
}

void nir_printer::define(nir_def* def, const std::string& e, bool reads_memory, bool cheap, bool force_temp)
{
   unsigned idx = def->index;
   value_deps d;
   d.mem = reads_memory;
   nir_instr* in = nir_def_instr(def);
   if (in->type == nir_instr_type_intrinsic && nir_instr_as_intrinsic(in)->intrinsic == nir_intrinsic_load_reg)
      d.regs.push_back(nir_instr_as_intrinsic(in)->src[0].ssa->index);
   else {
      std::pair<nir_printer*, value_deps*> state(this, &d);
      nir_foreach_src(in, [](nir_src* src, void* data) {
         auto* st = (std::pair<nir_printer*, value_deps*>*)data;
         st->first->add_deps(src->ssa, *st->second);
         return true;
      }, &state);
   }
   deps[idx] = d;
   bool reads_mutable = d.mem || !d.regs.empty();
   unsigned max_uses = cheap ? 1000 : 1;
   if (!force_temp && (e.size() < 160 || only_used_by_store(def)) && can_forward(def, max_uses)) {
      expr[idx] = e;
      return;
   }
   if (!force_temp && cheap && !reads_mutable) {
      // read-only names (uniforms/inputs) are fine to repeat anywhere
      expr[idx] = e;
      return;
   }
   new_temp(def, e);
}

void nir_printer::define_side_effect(nir_def* def, const std::string& e)
{
   if (nir_def_is_unused(def)) {
      line(e + ";");
      return;
   }
   new_temp(def, e);
}

std::string nir_printer::mask_str(unsigned mask, unsigned n)
{
   if (mask == (1u << n) - 1)
      return "";
   std::string r = ".";
   for (unsigned i = 0; i < 4; i++)
      if (mask & (1u << i)) r += "xyzw"[i];
   return r;
}

std::string nir_printer::masked_value(nir_src& val, unsigned mask, unsigned total, BT want, unsigned want_prec)
{
   if (mask == (1u << total) - 1 || total == 1)
      return src_full(val, want, want_prec);
   uint8_t swz[4];
   unsigned n = 0;
   for (unsigned i = 0; i < 4; i++)
      if (mask & (1u << i)) swz[n++] = i;
   return src_swz(val, swz, n, want, false, want_prec);
}

void nir_printer::emit_instr(nir_instr* instr)
{
   switch (instr->type) {
   case nir_instr_type_load_const:
   case nir_instr_type_undef:
   case nir_instr_type_deref:
      return; // printed at use sites
   case nir_instr_type_alu: {
      nir_alu_instr* alu = nir_instr_as_alu(instr);
      define(&alu->def, alu_expr(alu), false, false);
      return;
   }
   case nir_instr_type_tex: {
      nir_tex_instr* tex = nir_instr_as_tex(instr);
      define(&tex->def, tex_expr(tex), false, false);
      return;
   }
   case nir_instr_type_jump: {
      nir_jump_instr* j = nir_instr_as_jump(instr);
      switch (j->type) {
      case nir_jump_break: line("break;"); break;
      case nir_jump_continue: line("continue;"); break;
      case nir_jump_return: case nir_jump_halt: line(return_stmt()); break;
      default: line("/*UNHANDLED jump*/"); break;
      }
      return;
   }
   case nir_instr_type_intrinsic:
      emit_intrinsic(nir_instr_as_intrinsic(instr));
      return;
   default:
      line(fmt("/*UNHANDLED instr type %d*/", instr->type));
      return;
   }
}

const nir_printer::builtin_var* nir_printer::find_builtin(nir_intrinsic_op op)
{
   typedef builtin_var B;
   static const B builtins[] = {
      {nir_intrinsic_load_frag_coord, "gl_FragCoord", BT_FLOAT, 4, nullptr},
      {nir_intrinsic_load_front_face, "gl_FrontFacing", BT_BOOL, 1, "front_facing"},
      {nir_intrinsic_load_point_coord, "gl_PointCoord", BT_FLOAT, 2, "point_coord"},
      {nir_intrinsic_load_vertex_id, "gl_VertexID", BT_UINT, 1, "vertex_id"},
      {nir_intrinsic_load_instance_id, "gl_InstanceID", BT_UINT, 1, "instance_id"},
      {nir_intrinsic_load_sample_id, "gl_SampleID", BT_UINT, 1, "sample_id"},
      {nir_intrinsic_load_sample_pos, "gl_SamplePosition", BT_FLOAT, 2, nullptr},
      {nir_intrinsic_load_helper_invocation, "gl_HelperInvocation", BT_BOOL, 1, nullptr},
      {nir_intrinsic_load_local_invocation_id, "gl_LocalInvocationID", BT_UINT, 3, "thread_position_in_threadgroup"},
      {nir_intrinsic_load_global_invocation_id, "gl_GlobalInvocationID", BT_UINT, 3, "thread_position_in_grid"},
      {nir_intrinsic_load_workgroup_id, "gl_WorkGroupID", BT_UINT, 3, "threadgroup_position_in_grid"},
      {nir_intrinsic_load_num_workgroups, "gl_NumWorkGroups", BT_UINT, 3, "threadgroups_per_grid"},
      {nir_intrinsic_load_local_invocation_index, "gl_LocalInvocationIndex", BT_UINT, 1, "thread_index_in_threadgroup"},
      {nir_intrinsic_load_workgroup_size, "gl_WorkGroupSize", BT_UINT, 3, nullptr},
      {nir_intrinsic_load_subgroup_size, "gl_SubgroupSize", BT_UINT, 1, "threads_per_simdgroup"},
      {nir_intrinsic_load_subgroup_invocation, "gl_SubgroupInvocationID", BT_UINT, 1, "thread_index_in_simdgroup"},
      {nir_intrinsic_load_subgroup_id, "gl_SubgroupID", BT_UINT, 1, "simdgroup_index_in_threadgroup"},
      {nir_intrinsic_load_num_subgroups, "gl_NumSubgroups", BT_UINT, 1, "simdgroups_per_threadgroup"},
      {nir_intrinsic_load_subgroup_eq_mask, "gl_SubgroupEqMask", BT_UINT, 4, nullptr},
      {nir_intrinsic_load_subgroup_ge_mask, "gl_SubgroupGeMask", BT_UINT, 4, nullptr},
      {nir_intrinsic_load_subgroup_gt_mask, "gl_SubgroupGtMask", BT_UINT, 4, nullptr},
      {nir_intrinsic_load_subgroup_le_mask, "gl_SubgroupLeMask", BT_UINT, 4, nullptr},
      {nir_intrinsic_load_subgroup_lt_mask, "gl_SubgroupLtMask", BT_UINT, 4, nullptr},
   };
   for (const B& b : builtins)
      if (b.op == op)
         return &b;
   return nullptr;
}

void nir_printer::emit_intrinsic(nir_intrinsic_instr* in)
{
   if (intrinsic_dialect(in))
      return;
   if (const builtin_var* b = find_builtin(in->intrinsic)) {
      std::string e = builtin_ref(*b, &in->def);
      if (!e.empty()) {
         define(&in->def, e, false, true);
         return;
      }
   }
   if (glslopt_is_derivative(in->intrinsic)) {
      bool x = in->intrinsic == nir_intrinsic_ddx || in->intrinsic == nir_intrinsic_ddx_fine || in->intrinsic == nir_intrinsic_ddx_coarse;
      define(&in->def, fn(x ? "dFdx" : "dFdy") + "(" + src_full(in->src[0], BT_FLOAT) + ")", false, false);
      return;
   }
   std::string sg = subgroup_expr(in);
   if (!sg.empty()) {
      // subgroup operations depend on the active lanes: evaluate in place
      define_side_effect(&in->def, sg);
      return;
   }
   switch (in->intrinsic) {
   case nir_intrinsic_load_base_global_invocation_id:
      define(&in->def, vec_type(BT_UINT, 3, GLSL_PRECISION_HIGH) + "(0u)", false, true);
      return;
   case nir_intrinsic_decl_reg:
      return; // predeclared
   case nir_intrinsic_glslopt_precision: {
      // Precision conversion: must become a declared temporary, otherwise
      // the GLSL compiler would infer the operands' precision again. Not
      // needed if the value goes straight into a store, or if it already
      // has that precision.
      unsigned p = glslopt_precision_info::norm(nir_intrinsic_base(in));
      nir_def* sd = in->src[0].ssa;
      bool needed = glslopt_precision_info::norm(pi.prec[sd->index]) != p && !only_used_by_store(&in->def);
      define(&in->def, src_full(in->src[0], BT_UNK, prec_of(&in->def)), false, false, needed);
      return;
   }
   case nir_intrinsic_load_reg:
      define(&in->def, name[in->src[0].ssa->index], false, true);
      return;
   case nir_intrinsic_store_reg: {
      nir_def* reg = in->src[1].ssa;
      unsigned mask = nir_intrinsic_write_mask(in);
      nir_intrinsic_instr* decl = nir_instr_as_intrinsic(nir_def_instr(reg));
      unsigned n = nir_intrinsic_num_components(decl);
      unsigned p = nir_intrinsic_bit_size(decl) == 16 ? GLSL_PRECISION_MEDIUM : eff(pi.regprec[reg->index]);
      line(name[reg->index] + mask_str(mask, n) + " = " + masked_value(in->src[0], mask, n, type[reg->index], p) + ";");
      return;
   }
   case nir_intrinsic_load_deref: {
      nir_deref_instr* d = nir_src_as_deref(in->src[0]);
      bool mut = deref_is_mutable(d);
      bool simple = true;
      for (nir_deref_instr* p = d; p; p = nir_deref_instr_parent(p))
         if (p->deref_type == nir_deref_type_array && !nir_src_is_const(p->arr.index))
            simple = false;
      define(&in->def, deref_str(d), mut, simple && !mut);
      return;
   }
   case nir_intrinsic_store_deref: {
      nir_deref_instr* d = nir_src_as_deref(in->src[0]);
      unsigned mask = nir_intrinsic_write_mask(in);
      unsigned n = glsl_type_is_vector(d->type) ? glsl_get_vector_elements(d->type) : 1;
      unsigned p = eff(glslopt_precision_info::deref_prec(d));
      line(deref_str(d) + mask_str(mask, n) + " = " + masked_value(in->src[1], mask, n, bt_from_glsl(d->type), p) + ";");
      return;
   }
   case nir_intrinsic_copy_deref:
      line(deref_str(nir_src_as_deref(in->src[0])) + " = " + deref_str(nir_src_as_deref(in->src[1])) + ";");
      return;
   case nir_intrinsic_terminate: case nir_intrinsic_demote:
      line(discard_stmt());
      return;
   case nir_intrinsic_terminate_if: case nir_intrinsic_demote_if:
      line("if (" + src_full(in->src[0], BT_BOOL) + ") " + discard_stmt());
      return;
   default: {
      std::string e = std::string("/*UNHANDLED*/") + nir_intrinsic_infos[in->intrinsic].name + "()";
      if (nir_intrinsic_infos[in->intrinsic].has_dest)
         define(&in->def, e, true, false);
      else
         line(e + ";");
      return;
   }
   }
}

const char* nir_printer::atomic_name(nir_atomic_op op)
{
   switch (op) {
   case nir_atomic_op_iadd: return "atomicAdd";
   case nir_atomic_op_imin: case nir_atomic_op_umin: return "atomicMin";
   case nir_atomic_op_imax: case nir_atomic_op_umax: return "atomicMax";
   case nir_atomic_op_iand: return "atomicAnd";
   case nir_atomic_op_ior: return "atomicOr";
   case nir_atomic_op_ixor: return "atomicXor";
   case nir_atomic_op_xchg: return "atomicExchange";
   case nir_atomic_op_cmpxchg: return "atomicCompSwap";
   default: return "atomicUNHANDLED";
   }
}

// ---------------------------------------------------------------------------
// loops
// NIR loops are `loop { ... }` with explicit breaks. Recover source-like
// forms when the exit test is the first or the last thing in the body:
//   header test:  loop { if (c) break; body; i = i + k; }  -> for (init; !c; i = i + k) { body }
//   tail test:    loop { body; i = i + k; if (c) break; }  -> for (...) if the first test provably
//                 passes (constant init and bound), otherwise do { body } while (!c);
// Only for loops without `continue`: its meaning differs between the forms.

bool nir_printer::cf_has_continue(struct exec_list* list)
{
   foreach_list_typed(nir_cf_node, n, node, list) {
      switch (n->type) {
      case nir_cf_node_block: {
         nir_block* b = nir_cf_node_as_block(n);
         nir_instr* last = nir_block_last_instr(b);
         if (last && last->type == nir_instr_type_jump && nir_instr_as_jump(last)->type == nir_jump_continue)
            return true;
         break;
      }
      case nir_cf_node_if:
         if (cf_has_continue(&nir_cf_node_as_if(n)->then_list) || cf_has_continue(&nir_cf_node_as_if(n)->else_list))
            return true;
         break;
      default: // nested loop: its continues are its own
         break;
      }
   }
   return false;
}

static bool
block_is_just_break(struct exec_list* list)
{
   if (!exec_list_is_singular(list))
      return false;
   nir_cf_node* n = exec_node_data(nir_cf_node, exec_list_get_head(list), node);
   if (n->type != nir_cf_node_block)
      return false;
   nir_block* b = nir_cf_node_as_block(n);
   return nir_block_ends_in_break(b) && nir_block_first_instr(b) == nir_block_last_instr(b);
}

// `if (c) break;` (or `if (c) {} else break;`); *exit_when_true tells which
static bool
is_break_if(nir_cf_node* n, nir_if** out, bool* exit_when_true)
{
   if (!n || n->type != nir_cf_node_if)
      return false;
   nir_if* nif = nir_cf_node_as_if(n);
   if (block_is_just_break(&nif->then_list) && nir_cf_list_is_empty_block(&nif->else_list)) {
      *out = nif; *exit_when_true = true; return true;
   }
   if (block_is_just_break(&nif->else_list) && nir_cf_list_is_empty_block(&nif->then_list)) {
      *out = nif; *exit_when_true = false; return true;
   }
   return false;
}

// `rN = <expr>;` assigning register name
static bool
is_reg_assign(const std::string& l, std::string* reg)
{
   if (l.size() < 4 || l[0] != 'r' || l.back() != ';')
      return false;
   size_t sp = l.find(" = ");
   if (sp == std::string::npos)
      return false;
   for (size_t i = 1; i < sp; i++)
      if (!isdigit((unsigned char)l[i])) return false;
   *reg = l.substr(0, sp);
   return true;
}

static std::string
pretty_incr(const std::string& l, const std::string& reg)
{
   std::string body = l.substr(0, l.size() - 1); // drop ';'
   if (body == reg + " = (" + reg + " + 1)") return reg + "++";
   if (body == reg + " = (" + reg + " - 1)") return reg + "--";
   // rN = (rN + k)  ->  rN += k
   for (const char* op : {" + ", " - "}) {
      std::string pre = reg + " = (" + reg + op;
      if (body.compare(0, pre.size(), pre) == 0 && body.back() == ')' && body.find(' ', pre.size()) == std::string::npos)
         return reg + " " + op[1] + "= " + body.substr(pre.size(), body.size() - pre.size() - 1);
   }
   return body;
}

// does `text` use a temporary that is declared inside `body` (so it would be
// out of scope in a loop header / do-while condition)?
static bool
uses_body_decl(const std::string& text, const std::string& body)
{
   for (size_t i = 0; i < text.size(); i++) {
      if (text[i] != 't' || (i > 0 && (isalnum((unsigned char)text[i - 1]) || text[i - 1] == '_')))
         continue;
      size_t j = i + 1;
      while (j < text.size() && isdigit((unsigned char)text[j])) j++;
      if (j == i + 1 || (j < text.size() && (isalnum((unsigned char)text[j]) || text[j] == '_')))
         continue;
      std::string pat = " " + text.substr(i, j - i) + " = ";
      for (size_t p = body.find(pat); p != std::string::npos; p = body.find(pat, p + 1))
         if (p > 0 && (isalnum((unsigned char)body[p - 1]) || body[p - 1] == '_'))
            return true; // "<type> tN = " declaration
   }
   return false;
}

static bool
mentions(const std::string& sr, const std::string& id)
{
   for (size_t p = sr.find(id); p != std::string::npos; p = sr.find(id, p + 1)) {
      bool l = p == 0 || !(isalnum((unsigned char)sr[p - 1]) || sr[p - 1] == '_');
      bool r = p + id.size() >= sr.size() || !(isalnum((unsigned char)sr[p + id.size()]) || sr[p + id.size()] == '_');
      if (l && r) return true;
   }
   return false;
}

// loop condition text: the exit condition `c`, negated when exit_when_true
std::string nir_printer::loop_cond(nir_src& c, bool exit_when_true)
{
   if (!exit_when_true)
      return src_full(c, BT_BOOL);
   nir_alu_instr* a = NULL;
   if (c.ssa->num_components == 1 && (a = folded_alu(c.ssa, nir_op_inot)))
      return alu_src(a, 0);
   static const struct { nir_op op; const char* inv; } inv[] = {
      {nir_op_ilt, ">="}, {nir_op_ige, "<"}, {nir_op_ult, ">="}, {nir_op_uge, "<"},
      {nir_op_ieq, "!="}, {nir_op_ine, "=="}, {nir_op_flt, ">="}, {nir_op_fge, "<"},
      {nir_op_feq, "!="}, {nir_op_fneu, "=="},
   };
   for (auto& e : inv) {
      if (c.ssa->num_components == 1 && (a = folded_alu(c.ssa, e.op)))
         return std::string("(") + alu_src(a, 0, true) + " " + e.inv + " " + alu_src(a, 1, true) + ")";
   }
   std::string sr = src_full(c, BT_BOOL);
   return "!" + (is_atom(sr) ? sr : "(" + sr + ")");
}

// value of an integer/float comparison with load_reg(reg) := init; -1 if unknown
int nir_printer::eval_cond_at(nir_def* cond, nir_def* reg, nir_const_value init)
{
   nir_instr* ci = nir_def_instr(cond);
   if (ci->type != nir_instr_type_alu)
      return -1;
   nir_alu_instr* a = nir_instr_as_alu(ci);
   if (nir_op_infos[a->op].num_inputs != 2 || a->def.num_components != 1)
      return -1;
   nir_const_value v[2];
   for (unsigned i = 0; i < 2; i++) {
      nir_instr* si = nir_def_instr(a->src[i].src.ssa);
      if (si->type == nir_instr_type_load_const)
         v[i] = nir_instr_as_load_const(si)->value[a->src[i].swizzle[0]];
      else if (si->type == nir_instr_type_intrinsic && nir_instr_as_intrinsic(si)->intrinsic == nir_intrinsic_load_reg &&
               nir_instr_as_intrinsic(si)->src[0].ssa == reg && a->src[i].src.ssa->bit_size == 32)
         v[i] = init;
      else
         return -1;
   }
   switch (a->op) {
   case nir_op_ilt: return v[0].i32 < v[1].i32;
   case nir_op_ige: return v[0].i32 >= v[1].i32;
   case nir_op_ult: return v[0].u32 < v[1].u32;
   case nir_op_uge: return v[0].u32 >= v[1].u32;
   case nir_op_ieq: return v[0].u32 == v[1].u32;
   case nir_op_ine: return v[0].u32 != v[1].u32;
   case nir_op_flt: return v[0].f32 < v[1].f32;
   case nir_op_fge: return v[0].f32 >= v[1].f32;
   default: return -1;
   }
}

// Loop counters in printed text. NIR schedules the counter update anywhere in
// the body, and its initialization anywhere before the loop; both can move
// into a `for` header when nothing in between mentions the counter.

static std::vector<std::string>
split_lines(const std::string& t)
{
   std::vector<std::string> l;
   for (size_t p = 0; p < t.size();) {
      size_t e = t.find('\n', p);
      l.push_back(t.substr(p, e - p));
      p = e + 1;
   }
   return l;
}

static std::string
join_lines(const std::vector<std::string>& l)
{
   std::string t;
   for (const std::string& x : l)
      t += x + "\n";
   return t;
}

static bool
at_indent(const std::string& l, size_t n)
{
   return l.size() > n && l.compare(0, n, std::string(n, ' ')) == 0 && l[n] != ' ';
}

// the last statement `<reg> = ...;` at indentation n of text, for want_reg
// or else a register mentioned in `cond`, where nothing after it mentions reg
// (or breaks out of a loop, if no_break_after); removed from text and returned
// without ';'
static bool
take_reg_assign(std::string& text, size_t n, const std::string& want_reg, const std::string& cond,
                bool no_break_after, std::string* reg, std::string* stmt)
{
   std::vector<std::string> l = split_lines(text);
   for (size_t i = l.size(); i-- > 0;) {
      size_t f = l[i].find_first_not_of(' ');
      if (f != std::string::npos && f < n)
         return false; // start of the enclosing scope
      std::string r;
      if (!at_indent(l[i], n) || !is_reg_assign(l[i].substr(n), &r) ||
          (!want_reg.empty() ? r != want_reg : !mentions(cond, r))) {
         if (!want_reg.empty() ? mentions(l[i], want_reg) : false)
            return false;
         if (no_break_after && l[i].find("break;") != std::string::npos)
            return false;
         continue;
      }
      // later lines must not mention this register
      for (size_t j = i + 1; j < l.size(); j++)
         if (mentions(l[j], r))
            return false;
      *reg = r;
      *stmt = l[i].substr(n, l[i].size() - n - 1);
      l.erase(l.begin() + i);
      text = join_lines(l);
      return true;
   }
   return false;
}

// value of a literal printed by const_comp: int, uint or float
static bool
parse_literal(const std::string& t, nir_const_value* v)
{
   char* end;
   long i = strtol(t.c_str(), &end, 10);
   if (*end == 0 || (*end == 'u' && end[1] == 0)) {
      *v = nir_const_value_for_int(i, 32);
      return !t.empty();
   }
   double d = strtod(t.c_str(), &end);
   if (*end != 0 || t.empty())
      return false;
   *v = nir_const_value_for_float(d, 32);
   return true;
}

void nir_printer::emit_loop(nir_loop* loop)
{
   std::vector<nir_cf_node*> nodes;
   foreach_list_typed(nir_cf_node, n, node, &loop->body)
      nodes.push_back(n);
   // cf lists always end in a block; ignore a trailing empty one
   if (nodes.size() > 1 && nodes.back()->type == nir_cf_node_block &&
       exec_list_is_empty(&nir_cf_node_as_block(nodes.back())->instr_list))
      nodes.pop_back();
   nir_if* brk = NULL;
   bool exit_true = false;
   bool simple = !cf_has_continue(&loop->body) && nodes.size() >= 2;

   // header test (first block only computes the condition)
   bool pure_head = nodes[0]->type == nir_cf_node_block;
   if (pure_head) {
      nir_foreach_instr(in, nir_cf_node_as_block(nodes[0])) {
         if (has_side_effect(in) || (in->type == nir_instr_type_intrinsic &&
                                     (nir_instr_as_intrinsic(in)->intrinsic == nir_intrinsic_store_reg ||
                                      nir_instr_as_intrinsic(in)->intrinsic == nir_intrinsic_store_deref))) {
            pure_head = false;
            break;
         }
      }
   }
   if (simple && pure_head && is_break_if(nodes[1], &brk, &exit_true)) {
      std::string saved;
      saved.swap(out);
      ind++;
      emit_cf(nodes[0]);
      std::string hdr;
      hdr.swap(out);
      bool cond_inline = !expr[brk->condition.ssa->index].empty() ||
                         nir_def_instr(brk->condition.ssa)->type == nir_instr_type_load_const;
      if (hdr.empty() && cond_inline) {
         std::string cond = loop_cond(brk->condition, exit_true);
         for (size_t i = 2; i < nodes.size(); i++)
            emit_cf(nodes[i]);
         std::string body;
         body.swap(out);
         ind--;
         out.swap(saved);
         std::string reg, incr, init, r2;
         std::string body0 = body;
         size_t inner = (ind + 1) * 2;
         if (take_reg_assign(body, inner, "", cond, true, &reg, &incr) && mentions(cond, reg) && !uses_body_decl(incr, body0)) {
            take_reg_assign(out, ind * 2, reg, "", false, &r2, &init);
            line("for (" + init + "; " + cond + "; " + pretty_incr(incr + ";", reg) + ") {");
         } else {
            body = body0;
            line("while (" + cond + ") {");
         }
         out += body;
         line("}");
         return;
      }
      // not a clean header: print generically, reusing what was emitted
      ind--;
      out.swap(saved);
      line("while (true) {");
      out += hdr;
      ind++;
      for (size_t i = 1; i < nodes.size(); i++)
         emit_cf(nodes[i]);
      ind--;
      line("}");
      return;
   }

   // tail test
   if (simple && is_break_if(nodes.back(), &brk, &exit_true)) {
      std::string saved;
      saved.swap(out);
      ind++;
      for (size_t i = 0; i + 1 < nodes.size(); i++)
         emit_cf(nodes[i]);
      bool cond_inline = !expr[brk->condition.ssa->index].empty();
      std::string cond = cond_inline ? loop_cond(brk->condition, exit_true) : "";
      if (cond_inline && !uses_body_decl(cond, out)) {
         std::string body;
         body.swap(out);
         ind--;
         out.swap(saved);
         // for loop if the first iteration's test provably passes: the counter
         // starts at a constant for which the exit test is false
         std::string reg, incr, init, r2, body0 = body, out0 = out;
         size_t inner = (ind + 1) * 2;
         nir_const_value iv;
         if (take_reg_assign(body, inner, "", cond, true, &reg, &incr) && mentions(cond, reg) && !uses_body_decl(incr, body0) &&
             take_reg_assign(out, ind * 2, reg, "", false, &r2, &init) &&
             parse_literal(init.substr(reg.size() + 3), &iv)) {
            nir_def* rdef = NULL;
            nir_foreach_reg_decl(decl, impl)
               if (name[decl->def.index] == reg) rdef = &decl->def;
            if (rdef && eval_cond_at(brk->condition.ssa, rdef, iv) == (exit_true ? 0 : 1)) {
               line("for (" + init + "; " + cond + "; " + pretty_incr(incr + ";", reg) + ") {");
               out += body;
               line("}");
               return;
            }
         }
         body = body0;
         out = out0;
         line("do {");
         out += body;
         line("} while (" + cond + ");");
         return;
      }
      // condition was materialized inside the body; keep the break
      emit_cf(nodes.back());
      std::string body;
      body.swap(out);
      ind--;
      out.swap(saved);
      line("while (true) {");
      out += body;
      line("}");
      return;
   }

   line("while (true) {");
   ind++;
   emit_cf_list(&loop->body);
   ind--;
   line("}");
}

void nir_printer::emit_cf_list(struct exec_list* list)
{
   foreach_list_typed(nir_cf_node, node, node, list)
      emit_cf(node);
}

void nir_printer::emit_cf(nir_cf_node* node)
{
   switch (node->type) {
   case nir_cf_node_block: {
      nir_block* b = nir_cf_node_as_block(node);
      nir_foreach_instr(instr, b)
         emit_instr(instr);
      break;
   }
   case nir_cf_node_if: {
      nir_if* nif = nir_cf_node_as_if(node);
      line("if (" + src_full(nif->condition, BT_BOOL) + ") {");
      ind++;
      emit_cf_list(&nif->then_list);
      ind--;
      if (!nir_cf_list_is_empty_block(&nif->else_list)) {
         line("} else {");
         ind++;
         emit_cf_list(&nif->else_list);
         ind--;
      }
      line("}");
      break;
   }
   case nir_cf_node_loop:
      emit_loop(nir_cf_node_as_loop(node));
      break;
   default:
      line("/*UNHANDLED cf*/");
   }
}
