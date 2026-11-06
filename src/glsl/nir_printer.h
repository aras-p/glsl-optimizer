// NIR -> source printing core shared by the GLSL and Metal printers.
//
// Expects NIR out of SSA (nir_convert_from_ssa with registers) with a single
// entry point. Values are typed (NIR is mostly typeless), carry GLSL ES
// precision, and are folded into expressions when that does not change what is
// computed. Matrix/builtin/loop shapes that NIR lowered are recovered at print
// time when their pieces were folded.
#pragma once

#include "nir_optimize.h"
#include "compiler/glsl_types.h"
#include <string>
#include <vector>
#include <set>
#include <map>

enum printer_base_type { BT_UNK, BT_FLOAT, BT_INT, BT_UINT, BT_BOOL };

std::string printer_fmt(const char* f, ...)
#if defined(__GNUC__)
   __attribute__((format(printf, 1, 2)))
#endif
   ;

class nir_printer {
public:
   explicit nir_printer(nir_shader* shader);
   virtual ~nir_printer() {}
   std::string print();

protected:
   typedef printer_base_type BT;
   nir_shader* s;
   nir_function_impl* impl = nullptr;
   glslopt_precision_info pi;

   std::string out; // current statement stream
   int ind = 1;
   std::string pre_decls; // declarations at the top of the entry function
   int tmp_counter = 0;

   // per SSA def
   std::vector<BT> type;
   std::vector<std::string> expr;    // expression if folded
   std::vector<std::string> name;    // variable name if materialized
   // what a folded expression reads that later statements could change
   struct value_deps {
      bool mem = false;           // memory that stores/atomics/barriers can modify
      std::vector<unsigned> regs; // registers (decl_reg def indices)
   };
   std::vector<value_deps> deps;
   void add_deps(nir_def* d, value_deps& out);
   bool conflicts(const value_deps& d, nir_instr* it);

   std::map<const nir_variable*, std::string> local_names; // unique names of function temporaries
   std::set<std::string> used_names;
   std::string unique_name(const char* base);

   void line(const std::string& l) { out += std::string(ind * 2, ' ') + l + "\n"; }
   void pre_decl(const std::string& l) { pre_decls += "  " + l + "\n"; }

   // ------------------------------------------------------------------
   // dialect hooks
   virtual std::string vec_type(BT bt, unsigned n, unsigned prec) = 0;
   // type and name of a declaration (arrays get their dimensions)
   virtual std::string type_decl(const glsl_type* t, unsigned prec, const std::string& nm) = 0;
   // qualifier prefix for a declared temporary of the given precision
   virtual std::string decl_prec(unsigned prec, BT bt) = 0;
   virtual std::string var_ref(nir_variable* v) = 0;
   virtual std::string fn(const char* glsl_name) { return glsl_name; }
   virtual std::string alu_dialect(nir_alu_instr*) { return ""; }
   virtual std::string tex_expr(nir_tex_instr* tex) = 0;
   virtual bool intrinsic_dialect(nir_intrinsic_instr* in) = 0;
   virtual std::string cast(const std::string& e, BT from, unsigned from_prec, BT to, unsigned to_prec, unsigned n);
   virtual std::string float_special(double d); // nan/inf literals
   virtual std::string discard_stmt() { return "discard;"; }
   virtual std::string return_stmt() { return "return;"; }
   virtual std::string local_initializer(BT) { return ""; }
   // declarations that need names before the body is printed
   virtual void prologue() {}
   // the whole output, given the entry function's declarations and statements
   virtual std::string assemble(const std::string& decls, const std::string& stmts) = 0;

   // builtin inputs (system values); dialects return how to read one, "" if unsupported
   struct builtin_var {
      nir_intrinsic_op op;
      const char* name;
      BT bt;          // type of the Metal entry point parameter
      unsigned n;
      const char* metal_attr;
   };
   virtual std::string builtin_ref(const builtin_var& b, nir_def* d) = 0;
   static const builtin_var* find_builtin(nir_intrinsic_op op);
   // subgroup operation expression, "" if not one
   virtual std::string subgroup_expr(nir_intrinsic_instr*) { return ""; }

   // ------------------------------------------------------------------
   // precision
   unsigned prec_of(nir_def* d) const;   // HIGH or MEDIUM (lowp/none treated as such)
   static unsigned eff(unsigned p)
   {
      p = glslopt_precision_info::norm(p);
      return p == GLSL_PRECISION_MEDIUM ? GLSL_PRECISION_MEDIUM : GLSL_PRECISION_HIGH;
   }
   std::string def_type(nir_def* d) { return vec_type(type[d->index], d->num_components, prec_of(d)); }
   // expression e computes d's value as a 32-bit `from` (default: d's type): convert to d's type
   std::string as_def(const std::string& e, nir_def* d, BT from = BT_UNK)
   {
      return cast(e, from == BT_UNK ? type[d->index] : from, GLSL_PRECISION_HIGH, type[d->index], prec_of(d), d->num_components);
   }

   // ------------------------------------------------------------------
   // types
   static BT bt_from_alu(nir_alu_type t);
   static BT bt_from_glsl(const glsl_type* t);
   static bool alu_typeless(nir_op op);
   BT intrinsic_type(nir_intrinsic_instr* in);
   BT use_type(nir_src* src);
   BT producer_type(nir_def* def);
   void infer_types();

   // ------------------------------------------------------------------
   // sources
   static bool is_atom(const std::string& s);
   std::string const_comp(nir_const_value v, unsigned bit_size, BT bt);
   std::string const_vals_src(const nir_const_value* vals, unsigned bit_size, const uint8_t* swz, unsigned n, BT bt,
                              bool scalar_ok, unsigned want_prec);
   std::string def_ref(nir_def* def);
   // a source with the given swizzle (n components), converted to `want` type
   // and (for dialects where it matters) precision; 0 keeps the value's
   std::string src_swz(nir_src& src, const uint8_t* swz, unsigned n, BT want, bool allow_scalar_broadcast,
                       unsigned want_prec = 0);
   std::string src_full(nir_src& src, BT want = BT_UNK, unsigned want_prec = 0);
   BT alu_src_type(nir_alu_instr* alu, unsigned i);
   unsigned alu_src_prec(nir_alu_instr* alu, unsigned i);
   std::string alu_src(nir_alu_instr* alu, unsigned i, bool allow_broadcast = false);

   // ------------------------------------------------------------------
   // ALU
   bool prints_scalar(nir_alu_instr* alu, unsigned i);
   std::string vop_src(nir_alu_instr* alu, unsigned i, unsigned ninputs);
   std::string binop(nir_alu_instr* alu, const char* op);
   std::string call(nir_alu_instr* alu, const std::string& f);
   std::string cmp(nir_alu_instr* alu, const char* op, const char* vecfn);
   std::string componentwise(nir_alu_instr* alu, const char* op, bool is_ternary = false);
   std::string conv(nir_alu_instr* alu, BT to);
   std::string vec_ctor(nir_alu_instr* alu);
   std::string alu_expr(nir_alu_instr* alu);
   bool has_implicit_derivative(nir_def* d, int depth = 0);

   // print-time pattern recovery
   struct Coef { nir_alu_src* src = nullptr; uint8_t comp = 0; std::string lit; };
   nir_alu_instr* folded_alu(nir_def* d, nir_op op);
   nir_alu_instr* folded_alu(nir_alu_src& s, nir_op op) { return folded_alu(s.src.ssa, op); }
   static bool swz_identity(const nir_alu_src& s, unsigned n);
   static bool swz_broadcast(const nir_alu_src& s, unsigned n);
   bool same_value(nir_def* a, nir_def* b);
   bool same_operand(const nir_alu_src& a, const nir_alu_src& b, unsigned n);
   bool const_is(const nir_alu_src& s, unsigned n, double v, double eps = 1e-6);
   std::string asrc(nir_alu_instr* alu, unsigned i, unsigned n);
   nir_alu_instr* folded_fdot(nir_def* d);
   bool mat_column(const nir_alu_src& s, unsigned n, std::string& mat, const glsl_type*& mt, unsigned& col);
   void flatten_fadd(nir_alu_src& s, unsigned n, std::vector<nir_alu_src*>& terms);
   std::string build_vec(const std::vector<Coef>& c, unsigned prec);
   // "[2][3]", t becomes the element type; unsized arrays print "[]", or "[1]" if !allow_unsized
   static std::string array_dims(const glsl_type*& t, bool allow_unsized = true);
   // matrix constructor name; empty if the dialect has no such matrix
   virtual std::string mat_ctor(unsigned cols, unsigned rows, unsigned prec);
   virtual bool matrix_casts() { return true; } // matN(bigger matrix)
   virtual bool has_uint() { return true; }      // otherwise unsigned values print as int
   std::string match_mat_mul_vec(nir_alu_instr* alu);
   std::string match_generic_mat_mul(nir_alu_instr* alu);
   std::string match_vec_mul_mat(nir_alu_instr* alu);
   bool is_self_dot(nir_alu_instr* d, const nir_alu_src* x, unsigned n);
   std::string match_builtins(nir_alu_instr* alu);

   // ------------------------------------------------------------------
   // derefs
   virtual std::string deref_str(nir_deref_instr* d);
   bool deref_is_mutable(nir_deref_instr* d);

   // ------------------------------------------------------------------
   // statements
   bool has_side_effect(nir_instr* in);
   bool can_forward(nir_def* def, unsigned max_uses);
   bool escapes_scope(nir_def* def);
   bool only_used_by_store(nir_def* def);
   // e computes def; reads_memory: e itself reads modifiable memory (sources are tracked)
   void define(nir_def* def, const std::string& e, bool reads_memory, bool cheap, bool force_temp = false);
   void define_side_effect(nir_def* def, const std::string& e);
   std::string new_temp(nir_def* def, const std::string& e);
   static std::string mask_str(unsigned mask, unsigned n);
   std::string masked_value(nir_src& val, unsigned mask, unsigned total, BT want, unsigned want_prec);
   void emit_instr(nir_instr* instr);
   void emit_intrinsic(nir_intrinsic_instr* in);
   static const char* atomic_name(nir_atomic_op op);

   // ------------------------------------------------------------------
   // control flow
   bool cf_has_continue(struct exec_list* list);
   std::string loop_cond(nir_src& c, bool exit_when_true);
   int eval_cond_at(nir_def* cond, nir_def* reg, nir_const_value init);
   void emit_loop(nir_loop* loop);
   void emit_cf_list(struct exec_list* list);
   void emit_cf(nir_cf_node* node);
};

// What the printers need to know about the source shader (from the parse state).
struct glslopt_source_info {
   bool es = false;
   unsigned version = 110;
   bool had_version = false;
   std::vector<std::string> extensions; // enabled #extension names, in output order
   bool early_fragment_tests = false;
   bool local_size_specified = false;
   unsigned local_size[3] = {};
};

std::string glslopt_print_glsl(nir_shader* nir, const glslopt_source_info& info);

struct glslopt_metal_layout {
   std::map<const nir_variable*, int> uniform_offset; // byte offset in the uniform struct
   std::map<const nir_variable*, int> texture_slot;
   std::map<const nir_variable*, int> attribute_slot; // vertex inputs
   int uniforms_size = 0;
};
// Metal printer; fills uniform byte offsets / resource slots into layout.
std::string glslopt_print_metal(nir_shader* nir, const glslopt_source_info& info, glslopt_metal_layout& layout);
