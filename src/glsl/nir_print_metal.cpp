// NIR -> Metal Shading Language.
//
// Conventions (kept from the old glsl-optimizer Metal printer):
//  - stage inputs in `xlatMtlShaderInput _mtl_i [[stage_in]]`, outputs in
//    `xlatMtlShaderOutput _mtl_o`, ordinary uniforms in
//    `constant xlatMtlShaderUniform& _mtl_u [[buffer(0)]]`
//  - each sampled texture T has `sampler _mtlsmp_T` at the same index; images
//    take the texture slots after all sampled textures
//  - uniform/storage blocks, the optional buffer size table and atomic
//    counter buffers take buffer slots 1.. (see buffer_layout.h)
//  - mediump floats are half; values convert explicitly between precisions
#include "nir_printer.h"
#include "buffer_layout.h"
#include "compiler/nir/nir_deref.h"
#include "image_formats.h"
#include "main/menums.h"
#include "compiler/shader_enums.h"
#include <string.h>

#define fmt printer_fmt

namespace {

class metal_printer : public nir_printer {
public:
   metal_printer(nir_shader* nir, const glslopt_source_info& info, glslopt_metal_layout& layout)
      : nir_printer(nir), info(info), layout(layout)
   {
   }

private:
   const glslopt_source_info& info;
   glslopt_metal_layout& layout;

   std::string prefix;           // helpers, struct declarations
   std::set<std::string> helpers_done;
   std::set<const glsl_type*> structs_done;
   std::string inputs, outputs, uniforms, params, block_params, copy_in;
   std::vector<std::string> copy_out; // statements before every return
   std::map<const nir_variable*, std::string> var_names; // how a variable is referenced
   std::set<std::string> sysvals_done;
   int varying_counter = 0;
   unsigned min_metal_version = 0;
   std::vector<glslopt_buffer_block> blocks;
   std::vector<glslopt_atomic_counter_buffer> counters;
   unsigned size_table_index = 0;

   bool is_compute() const { return s->info.stage == MESA_SHADER_COMPUTE; }
   bool is_fragment() const { return s->info.stage == MESA_SHADER_FRAGMENT; }
   bool is_vertex() const { return s->info.stage == MESA_SHADER_VERTEX; }

   void need_version(unsigned v) { min_metal_version = MAX2(min_metal_version, v); }

   // ------------------------------------------------------------------
   // types

   std::string vec_type(BT bt, unsigned n, unsigned prec) override
   {
      std::string b;
      bool medium = eff(prec) == GLSL_PRECISION_MEDIUM;
      switch (bt) {
      case BT_INT: b = medium ? "short" : "int"; break;
      case BT_UINT: b = medium ? "ushort" : "uint"; break;
      case BT_BOOL: b = "bool"; break;
      default: b = medium ? "half" : "float"; break;
      }
      return n > 1 ? b + fmt("%u", n) : b;
   }

   std::string texture_type(const glsl_type* t, unsigned prec, unsigned access)
   {
      const char* sampled = glsl_get_sampler_result_type(t) == GLSL_TYPE_INT ? "int" :
                            glsl_get_sampler_result_type(t) == GLSL_TYPE_UINT ? "uint" :
                            eff(prec) == GLSL_PRECISION_MEDIUM && !glsl_type_is_image(t) ? "half" : "float";
      bool array = glsl_sampler_type_is_array(t);
      if (glsl_type_is_image(t)) {
         bool ms = glsl_get_sampler_dim(t) == GLSL_SAMPLER_DIM_MS;
         const char* acc = ms || (access & ACCESS_NON_WRITEABLE) ? "read" : (access & ACCESS_NON_READABLE) ? "write" : "read_write";
         const char* img = glslopt_metal_image_type_name(t);
         if (glsl_get_sampler_dim(t) == GLSL_SAMPLER_DIM_BUF)
            need_version(210);
         return fmt("%s<%s, access::%s%s>", img ? img : "texture2d", sampled, acc,
                    (access & ACCESS_COHERENT) ? ", memory_coherence_device" : "");
      }
      if (glsl_sampler_type_is_shadow(t)) {
         switch (glsl_get_sampler_dim(t)) {
         case GLSL_SAMPLER_DIM_CUBE: return array ? "depthcube_array<float>" : "depthcube<float>";
         default: return array ? "depth2d_array<float>" : "depth2d<float>";
         }
      }
      switch (glsl_get_sampler_dim(t)) {
      case GLSL_SAMPLER_DIM_CUBE: return fmt(array ? "texturecube_array<%s>" : "texturecube<%s>", sampled);
      case GLSL_SAMPLER_DIM_3D: return fmt("texture3d<%s>", sampled);
      default: return fmt(array ? "texture2d_array<%s>" : "texture2d<%s>", sampled);
      }
   }

   // Metal type of a value type (no array dimensions)
   std::string base_type_name(const glsl_type* t, unsigned prec)
   {
      if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
         emit_struct(t);
         return glsl_get_type_name(t);
      }
      if (glsl_type_is_sampler(t) || glsl_type_is_image(t))
         return texture_type(t, prec, 0);
      if (glsl_type_is_matrix(t))
         return mat_ctor(glsl_get_matrix_columns(t), glsl_get_vector_elements(t), eff(prec));
      return vec_type(bt_from_glsl(t), glsl_get_vector_elements(t), prec);
   }

   std::string type_decl(const glsl_type* t, unsigned prec, const std::string& nm) override
   {
      std::string dims = array_dims(t, false);
      return base_type_name(t, prec) + " " + nm + dims;
   }

   std::string decl_prec(unsigned, BT) override { return ""; }

   void emit_struct(const glsl_type* t)
   {
      t = glsl_without_array(t);
      if (!(glsl_type_is_struct(t) || glsl_type_is_interface(t)) || structs_done.count(t))
         return;
      structs_done.insert(t);
      std::string body;
      for (unsigned i = 0; i < glsl_get_length(t); i++) {
         const glsl_struct_field* f = glsl_get_struct_field_data(t, i);
         body += "  " + type_decl(f->type, f->precision, f->name) + ";\n";
      }
      prefix += fmt("struct %s {\n", glsl_get_type_name(t)) + body + "};\n";
   }

   // ------------------------------------------------------------------
   // values

   std::string cast(const std::string& e, BT from, unsigned from_prec, BT to, unsigned to_prec, unsigned n) override
   {
      if (from == BT_UNK || to == BT_UNK)
         return e;
      from_prec = from_prec ? from_prec : GLSL_PRECISION_HIGH;
      to_prec = to_prec ? to_prec : GLSL_PRECISION_HIGH;
      // precision matters for everything but bool: mediump is half/short/ushort
      if (from == BT_BOOL) from_prec = GLSL_PRECISION_HIGH;
      if (to == BT_BOOL) to_prec = GLSL_PRECISION_HIGH;
      if (from == to) {
         if (eff(from_prec) != eff(to_prec))
            return vec_type(to, n, to_prec) + "(" + e + ")";
         return e;
      }
      if ((from == BT_FLOAT) != (to == BT_FLOAT) && from != BT_BOOL && to != BT_BOOL) {
         // bit casts are between 32-bit types: 16-bit values widen first
         std::string v = eff(from_prec) == GLSL_PRECISION_MEDIUM ? vec_type(from, n, GLSL_PRECISION_HIGH) + "(" + e + ")" : e;
         std::string r = "as_type<" + vec_type(to, n, GLSL_PRECISION_HIGH) + ">(" + v + ")";
         if (eff(to_prec) == GLSL_PRECISION_MEDIUM)
            r = vec_type(to, n, to_prec) + "(" + r + ")";
         return r;
      }
      return vec_type(to, n, to_prec) + "(" + e + ")";
   }

   std::string float_special(double d) override
   {
      if (d != d) return "NAN";
      return d > 0 ? "INFINITY" : "(-INFINITY)";
   }

   std::string fn(const char* g) override
   {
      static const struct { const char* glsl; const char* metal; } map[] = {
         {"inversesqrt", "rsqrt"}, {"roundEven", "rint"}, {"saturate", "saturate"}, {"trunc", "trunc"},
         {"bitfieldExtract", "extract_bits"}, {"bitfieldInsert", "insert_bits"}, {"dFdx", "dfdx"}, {"dFdy", "dfdy"},
      };
      for (auto& m : map)
         if (!strcmp(g, m.glsl)) return m.metal;
      if (!strcmp(g, "mod")) {
         helper("_xlat_mod", "template <typename T> inline T _xlat_mod(T x, T y) { return x - y * floor(x / y); }\n");
         return "_xlat_mod";
      }
      return g;
   }

   void helper(const std::string& nm, const std::string& code)
   {
      if (helpers_done.insert(nm).second)
         prefix += code;
   }

   std::string mat_ctor(unsigned cols, unsigned rows, unsigned prec) override
   {
      return std::string(prec == GLSL_PRECISION_MEDIUM ? "half" : "float") + fmt("%ux%u", cols, rows);
   }
   bool matrix_casts() override { return false; }

   std::string local_initializer(BT bt) override { return bt == BT_BOOL ? " = false" : " = 0"; }
   std::string discard_stmt() override { return "discard_fragment();"; }
   std::string return_stmt() override
   {
      if (is_compute())
         return "return;";
      std::string r;
      for (const std::string& c : copy_out)
         r += c + " ";
      return r + "return _mtl_o;";
   }

   std::string alu_dialect(nir_alu_instr* alu) override
   {
      unsigned n = alu->def.num_components;
      bool isbool = alu->def.bit_size == 1;
      switch (alu->op) {
      case nir_op_bcsel: {
         bool vec_cond = alu->src[0].src.ssa->num_components > 1 && n > 1;
         bool deriv = type[alu->def.index] == BT_FLOAT &&
                      (has_implicit_derivative(alu->src[1].src.ssa) || has_implicit_derivative(alu->src[2].src.ssa));
         if (!vec_cond && !deriv)
            return "";
         std::string c = vec_cond ? alu_src(alu, 0) : src_swz(alu->src[0].src, alu->src[0].swizzle, 1, BT_BOOL, false);
         if (!vec_cond && n > 1)
            c = vec_type(BT_BOOL, n, 0) + "(" + c + ")";
         return "select(" + alu_src(alu, 2) + ", " + alu_src(alu, 1) + ", " + c + ")";
      }
      case nir_op_feq: case nir_op_ieq: return binop(alu, "==");
      case nir_op_fneu: case nir_op_ine: return binop(alu, "!=");
      case nir_op_flt: case nir_op_ilt: case nir_op_ult: return binop(alu, "<");
      case nir_op_fge: case nir_op_ige: case nir_op_uge: return binop(alu, ">=");
      case nir_op_ball_fequal2: case nir_op_ball_fequal3: case nir_op_ball_fequal4:
      case nir_op_ball_iequal2: case nir_op_ball_iequal3: case nir_op_ball_iequal4:
         return "all(" + alu_src(alu, 0) + " == " + alu_src(alu, 1) + ")";
      case nir_op_bany_fnequal2: case nir_op_bany_fnequal3: case nir_op_bany_fnequal4:
      case nir_op_bany_inequal2: case nir_op_bany_inequal3: case nir_op_bany_inequal4:
         return "any(" + alu_src(alu, 0) + " != " + alu_src(alu, 1) + ")";
      case nir_op_ixor:
         if (isbool) return binop(alu, "!=");
         return "";
      case nir_op_ishl: case nir_op_ishr: case nir_op_ushr: {
         // vector shifts need both operands of the same type
         bool sc = n > 1 && prints_scalar(alu, 1);
         std::string amount = alu_src(alu, 1, sc);
         std::string t = vec_type(type[alu->def.index], sc ? 1 : n, prec_of(&alu->def));
         const char* op = alu->op == nir_op_ishl ? " << " : " >> ";
         return "(" + alu_src(alu, 0) + op + t + "(" + amount + "))";
      }
      case nir_op_inot:
         if (isbool && n > 1) return "(!" + alu_src(alu, 0) + ")";
         return "";
      case nir_op_iand:
         if (isbool && n > 1) return binop(alu, "&&");
         return "";
      case nir_op_ior:
         if (isbool && n > 1) return binop(alu, "||");
         return "";
      case nir_op_bit_count: return vec_type(BT_INT, n, prec_of(&alu->def)) + "(popcount(" + alu_src(alu, 0) + "))";
      case nir_op_bitfield_reverse: return call(alu, "reverse_bits");
      case nir_op_find_lsb:
         helper("_xlat_findLSB", "template <typename T> inline T _xlat_findLSB(T x) { return select(T(ctz(x)), T(-1), x == T(0)); }\n");
         return vec_type(BT_INT, n, prec_of(&alu->def)) + "(_xlat_findLSB(" + alu_src(alu, 0) + "))";
      case nir_op_ufind_msb:
         helper("_xlat_findMSBu", "template <typename T> inline T _xlat_findMSBu(T x) { return select(T(31) - T(clz(x)), T(-1), x == T(0)); }\n");
         return vec_type(BT_INT, n, prec_of(&alu->def)) + "(_xlat_findMSBu(" + alu_src(alu, 0) + "))";
      case nir_op_ifind_msb:
         helper("_xlat_findMSBi", "template <typename T> inline T _xlat_findMSBi(T x) { T y = select(x, ~x, x < T(0)); return select(T(31) - T(clz(y)), T(-1), y == T(0)); }\n");
         return "_xlat_findMSBi(" + alu_src(alu, 0) + ")";
      case nir_op_ubitfield_extract: case nir_op_ibitfield_extract:
         return "extract_bits(" + alu_src(alu, 0) + ", uint(" + alu_src(alu, 1, true) + "), uint(" + alu_src(alu, 2, true) + "))";
      case nir_op_bitfield_insert:
         return "insert_bits(" + alu_src(alu, 0) + ", " + alu_src(alu, 1) + ", uint(" + alu_src(alu, 2, true) + "), uint(" +
                alu_src(alu, 3, true) + "))";
      case nir_op_pack_half_2x16: return "as_type<uint>(half2(" + alu_src(alu, 0) + "))";
      case nir_op_unpack_half_2x16: return as_def("float2(as_type<half2>(" + alu_src(alu, 0) + "))", &alu->def);
      case nir_op_pack_unorm_2x16: return call(alu, "pack_float_to_unorm2x16");
      case nir_op_pack_snorm_2x16: return call(alu, "pack_float_to_snorm2x16");
      case nir_op_pack_unorm_4x8: return call(alu, "pack_float_to_unorm4x8");
      case nir_op_pack_snorm_4x8: return call(alu, "pack_float_to_snorm4x8");
      case nir_op_unpack_unorm_2x16: return as_def(call(alu, "unpack_unorm2x16_to_float"), &alu->def);
      case nir_op_unpack_snorm_2x16: return as_def(call(alu, "unpack_snorm2x16_to_float"), &alu->def);
      case nir_op_unpack_unorm_4x8: return as_def(call(alu, "unpack_unorm4x8_to_float"), &alu->def);
      case nir_op_unpack_snorm_4x8: return as_def(call(alu, "unpack_snorm4x8_to_float"), &alu->def);
      default:
         return "";
      }
   }

   // ------------------------------------------------------------------
   // variable references

   std::string var_ref(nir_variable* v) override
   {
      auto it = var_names.find(v);
      if (it != var_names.end())
         return it->second;
      return v->name ? v->name : "/*unnamed*/";
   }

   std::string sysval(const char* type_name, const char* nm, const char* attr)
   {
      if (sysvals_done.insert(nm).second)
         params += fmt("\n  , %s %s [[%s]]", type_name, nm, attr);
      return nm;
   }

   // ------------------------------------------------------------------
   // buffer storage (uniform/storage blocks): Metal structs that match the
   // std140/std430 layout byte for byte

   std::map<const glsl_type*, unsigned> block_index; // interface type -> blocks index

   static const char* storage_scalar(const glsl_type* t)
   {
      switch (glsl_get_base_type(t)) {
      case GLSL_TYPE_INT: return "int";
      case GLSL_TYPE_UINT: case GLSL_TYPE_BOOL: return "uint";
      default: return "float";
      }
   }

   static std::string storage_vector(const glsl_type* t)
   {
      unsigned n = glsl_get_vector_elements(t);
      std::string sc = storage_scalar(t);
      if (n == 1) return sc;
      if (n == 3) return "packed_" + sc + "3";
      return sc + fmt("%u", n);
   }

   // array elements smaller than the array stride are padded with a struct
   // and accessed via .value
   static bool storage_wrapped(const glsl_type* elem, bool row, bool std430, unsigned stride)
   {
      return glslopt_buffer_size(elem, row, std430) != stride;
   }

   std::string storage_element(const glsl_type* elem, bool row, bool std430, unsigned stride)
   {
      std::string et = storage_type(elem, row, std430);
      unsigned sz = glslopt_buffer_size(elem, row, std430);
      if (sz == stride)
         return et;
      std::string dims;
      size_t b = et.find('[');
      std::string base = b == std::string::npos ? et : et.substr(0, b);
      dims = b == std::string::npos ? "" : et.substr(b);
      std::string nm = "_xlat_element_" + base + (dims.empty() ? "" : "_a") + fmt("_%u", stride);
      for (char& c : nm) if (!isalnum((unsigned char)c) && c != '_') c = '_';
      helper(nm, "struct " + nm + " { " + base + " value" + dims + fmt("; char _pad[%u]; };\n", stride - sz));
      return nm;
   }

   // storage type of a field; arrays come back as "T[N]" suffix form "T" + dims
   std::string storage_type(const glsl_type* t, bool row, bool std430)
   {
      if (glsl_type_is_array(t)) {
         const glsl_type* e = glsl_get_array_element(t);
         return storage_element(e, row, std430, glslopt_buffer_array_stride(e, row, std430)) + fmt("[%u]", MAX2(glsl_get_length(t), 1u));
      }
      if (glsl_type_is_struct(t))
         return storage_struct(t, row, std430);
      if (glsl_type_is_matrix(t)) {
         const glsl_type* vt = row ? glsl_get_row_type(t) : glsl_get_column_type(t);
         unsigned count = row ? glsl_get_vector_elements(t) : glsl_get_matrix_columns(t);
         return storage_element(vt, false, std430, glslopt_buffer_array_stride(vt, false, std430)) + fmt("[%u]", count);
      }
      return storage_vector(t);
   }

   std::string storage_struct(const glsl_type* t, bool row, bool std430)
   {
      std::string nm = std::string("_xlat_") + (glsl_type_is_interface(t) ? "buf_" : "") + glsl_get_type_name(t) +
                       (std430 ? "_std430" : "_std140") + (row ? "_row" : "");
      if (helpers_done.count(nm))
         return nm;
      std::string body;
      unsigned cursor = 0, pad = 0;
      std::vector<unsigned> offsets = glslopt_buffer_field_offsets(t, row, std430);
      for (unsigned i = 0; i < glsl_get_length(t); i++) {
         const glsl_struct_field& f = *glsl_get_struct_field_data(t, i);
         unsigned off = offsets[i];
         if (off > cursor)
            body += fmt("  char _pad%u[%u];\n", pad++, off - cursor);
         bool frow = glslopt_field_row_major(f, row);
         std::string st = storage_type(f.type, frow, std430);
         size_t b = st.find('[');
         body += "  " + (b == std::string::npos ? st + " " + f.name : st.substr(0, b) + " " + f.name + st.substr(b)) + ";\n";
         cursor = off + glslopt_buffer_size(f.type, frow, std430);
      }
      unsigned size = glsl_type_is_interface(t) ? glslopt_buffer_block_size(t, std430) : glslopt_buffer_size(t, row, std430);
      bool runtime = glsl_type_is_interface(t) && glslopt_buffer_has_runtime_array(t);
      if (!runtime && size > cursor)
         body += fmt("  char _pad%u[%u];\n", pad++, size - cursor);
      helper(nm, "struct " + nm + " {\n" + body + "};\n");
      if (!runtime)
         helper(nm + "_assert", fmt("static_assert(sizeof(%s) == %u, \"%s size\");\n", nm.c_str(), size, std430 ? "std430" : "std140"));
      return nm;
   }

   // walk a deref chain into a block; returns the storage expression of the
   // final deref, and describes what it points at
   struct buffer_ref {
      std::string expr;            // storage lvalue
      std::string row_col;         // for a column of a row-major matrix: column index text
      std::string row_base;        // ... and the matrix storage
      unsigned row_count = 0;
      bool row_rows_wrapped = false;
   };

   bool is_buffer_var(nir_variable* v) const
   {
      return v && (v->data.mode == nir_var_mem_ubo || v->data.mode == nir_var_mem_ssbo) && v->interface_type;
   }

   buffer_ref buffer_deref(nir_deref_instr* d)
   {
      nir_deref_path path;
      nir_deref_path_init(&path, d, NULL);
      nir_variable* v = path.path[0]->var;
      const glslopt_buffer_block& b = blocks[block_index[v->interface_type]];
      bool std430 = glslopt_buffer_uses_std430(b, true);
      bool row = glsl_matrix_type_is_row_major(b.type);
      buffer_ref r;
      unsigned i = 1;
      std::string root = var_names[b.variable];
      const glsl_type* t;
      if (b.named) {
         if (b.array_size > 1) {
            // array of blocks: a local array of pointers
            std::string idx;
            while (path.path[i] && path.path[i]->deref_type == nir_deref_type_array &&
                   glsl_type_is_array(nir_deref_instr_parent(path.path[i])->type)) {
               idx += "[" + src_full(path.path[i]->arr.index, BT_INT) + "]";
               i++;
            }
            root = "(*" + root + idx + ")";
         }
         t = b.type;
      } else {
         // a member of an unnamed block
         const glsl_struct_field& f = *glsl_get_struct_field_data(b.type, glsl_get_field_index(b.type, v->name));
         row = glslopt_field_row_major(f, row);
         root += std::string(".") + f.name;
         t = f.type;
      }
      r.expr = root;
      for (; path.path[i]; i++) {
         nir_deref_instr* p = path.path[i];
         if (p->deref_type == nir_deref_type_struct) {
            const glsl_struct_field& f = *glsl_get_struct_field_data(t, p->strct.index);
            row = glslopt_field_row_major(f, row);
            r.expr += std::string(".") + f.name;
            t = f.type;
         } else if (p->deref_type == nir_deref_type_array) {
            std::string idx = src_full(p->arr.index, BT_INT);
            if (glsl_type_is_array(t)) {
               const glsl_type* e = glsl_get_array_element(t);
               bool w = storage_wrapped(e, row, std430, glslopt_buffer_array_stride(e, row, std430));
               r.expr += "[" + idx + "]" + (w ? ".value" : "");
               t = e;
            } else if (glsl_type_is_matrix(t)) {
               const glsl_type* vt = row ? glsl_get_row_type(t) : glsl_get_column_type(t);
               bool w = storage_wrapped(vt, false, std430, glslopt_buffer_array_stride(vt, false, std430));
               if (row) {
                  r.row_base = r.expr;
                  r.row_col = idx;
                  r.row_count = glsl_get_vector_elements(t);
                  r.row_rows_wrapped = w;
               } else {
                  r.expr += "[" + idx + "]" + (w ? ".value" : "");
               }
               t = glsl_get_column_type(t);
            } else {
               // vector component
               if (!r.row_col.empty()) {
                  // row-major element: row = component, column = row_col
                  r.expr = r.row_base + "[" + idx + "]" + (r.row_rows_wrapped ? ".value" : "") + "[" + r.row_col + "]";
                  r.row_col.clear();
               } else {
                  r.expr += "[" + idx + "]";
               }
               t = glsl_get_array_element(t);
            }
         }
      }
      nir_deref_path_finish(&path);
      return r;
   }

   std::string buffer_load(nir_intrinsic_instr* in)
   {
      buffer_ref r = buffer_deref(nir_src_as_deref(in->src[0]));
      unsigned n = in->def.num_components;
      std::string e;
      if (!r.row_col.empty()) {
         e = vec_type(type[in->def.index], n, GLSL_PRECISION_HIGH) + "(";
         for (unsigned k = 0; k < r.row_count; k++)
            e += (k ? ", " : "") + r.row_base + fmt("[%u]", k) + (r.row_rows_wrapped ? ".value" : "") + "[" + r.row_col + "]";
         e += ")";
      } else {
         e = r.expr;
      }
      return vec_type(type[in->def.index], n, prec_of(&in->def)) + "(" + e + ")";
   }

   void buffer_store(nir_intrinsic_instr* in)
   {
      nir_deref_instr* d = nir_src_as_deref(in->src[0]);
      buffer_ref r = buffer_deref(d);
      unsigned mask = nir_intrinsic_write_mask(in);
      unsigned n = in->src[1].ssa->num_components;
      BT bt = bt_from_glsl(d->type);
      bool full = mask == (1u << n) - 1;
      if (r.row_col.empty() && (full || n == 1)) {
         std::string v = src_full(in->src[1], bt, GLSL_PRECISION_HIGH);
         if (bt == BT_BOOL)
            v = vec_type(BT_UINT, n, 0) + "(" + v + ")";
         line(r.expr + " = " + v + ";");
         return;
      }
      // per component (partial writes, row-major columns)
      for (unsigned c = 0; c < n; c++) {
         if (!(mask & (1u << c)))
            continue;
         uint8_t sw = c;
         std::string v = src_swz(in->src[1], &sw, 1, bt, false, GLSL_PRECISION_HIGH);
         if (bt == BT_BOOL)
            v = "uint(" + v + ")";
         std::string dst = r.row_col.empty() ? r.expr + fmt("[%u]", c)
                                             : r.row_base + fmt("[%u]", c) + (r.row_rows_wrapped ? ".value" : "") + "[" + r.row_col + "]";
         line(dst + " = " + v + ";");
      }
   }

   // ------------------------------------------------------------------
   // textures

   std::string tex_expr(nir_tex_instr* tex) override
   {
      std::string samp, samp_name, coord, lod, bias, cmp, ddx, ddy, offset, ms, proj;
      unsigned sprec = GLSL_PRECISION_HIGH;
      for (unsigned i = 0; i < tex->num_srcs; i++) {
         nir_tex_src& ts = tex->src[i];
         BT want = bt_from_alu(nir_tex_instr_src_type(tex, i));
         unsigned hp = want == BT_FLOAT ? GLSL_PRECISION_HIGH : 0;
         switch (ts.src_type) {
         case nir_tex_src_texture_deref: {
            nir_deref_instr* d = nir_src_as_deref(ts.src);
            samp = deref_str(d);
            nir_variable* v = nir_deref_instr_get_variable(d);
            samp_name = v ? std::string("_mtlsmp_") + v->name : "/*sampler*/";
            sprec = eff(glslopt_precision_info::deref_prec(d));
            break;
         }
         case nir_tex_src_sampler_deref: break;
         case nir_tex_src_coord: coord = src_full(ts.src, want, hp); break;
         case nir_tex_src_lod: lod = src_full(ts.src, want, hp); break;
         case nir_tex_src_bias: bias = src_full(ts.src, want, hp); break;
         case nir_tex_src_comparator: cmp = src_full(ts.src, want, hp); break;
         case nir_tex_src_ddx: ddx = src_full(ts.src, want, hp); break;
         case nir_tex_src_ddy: ddy = src_full(ts.src, want, hp); break;
         case nir_tex_src_offset: offset = src_full(ts.src, BT_INT); break;
         case nir_tex_src_ms_index: ms = src_full(ts.src, BT_INT); break;
         case nir_tex_src_projector: proj = src_full(ts.src, BT_FLOAT, GLSL_PRECISION_HIGH); break;
         default: coord += "/*UNHANDLED texsrc*/"; break;
         }
      }
      const glsl_sampler_dim dim = tex->sampler_dim;
      const bool cube = dim == GLSL_SAMPLER_DIM_CUBE, three = dim == GLSL_SAMPLER_DIM_3D;
      const unsigned n = cube || three ? 3 : 2;
      unsigned res_prec = prec_of(&tex->def);
      BT rt = bt_from_alu(tex->dest_type);
      unsigned tex_prec = rt != BT_FLOAT ? 0 : tex->is_shadow ? GLSL_PRECISION_HIGH : sprec;
      auto finish = [&](const std::string& e, unsigned comps, unsigned prec) {
         std::string r = e;
         if (comps < tex->def.num_components)
            r = vec_type(rt, tex->def.num_components, prec) + "(" + r + ")";
         return cast(r, rt, prec, rt, res_prec, tex->def.num_components);
      };
      std::string swz = fmt(".%.*s", n, "xyz");
      std::string uv = coord;
      std::string layer;
      if (tex->is_array && tex->op != nir_texop_txs && tex->op != nir_texop_query_levels) {
         std::string c = is_atom(coord) ? coord : "(" + coord + ")";
         uv = c + swz;
         if (tex->op == nir_texop_txf)
            layer = "uint(" + c + (cube ? ".w" : ".z") + ")";
         else
            layer = "uint(rint(" + c + (cube ? ".w" : ".z") + "))";
      }
      if (!proj.empty()) {
         uv = "(" + uv + " / " + proj + ")";
         if (!cmp.empty())
            cmp = "(" + cmp + " / " + proj + ")";
      }
      std::string arr = layer.empty() ? "" : ", " + layer;
      std::string offa = offset.empty() ? "" : ", " + offset;
      switch (tex->op) {
      case nir_texop_tex:
      case nir_texop_txb:
      case nir_texop_txl:
      case nir_texop_txd: {
         std::string opt;
         if (tex->op == nir_texop_txb) opt = ", bias(" + bias + ")";
         if (tex->op == nir_texop_txl) opt = ", level(" + lod + ")";
         if (tex->op == nir_texop_txd)
            opt = std::string(cube ? ", gradientcube(" : three ? ", gradient3d(" : ", gradient2d(") + ddx + ", " + ddy + ")";
         if (tex->is_shadow) {
            helper("_mtl_xl_shadow_sampler", "constexpr sampler _mtl_xl_shadow_sampler(address::clamp_to_edge, filter::linear, compare_func::less_equal);\n");
            return finish(samp + ".sample_compare(_mtl_xl_shadow_sampler, " + uv + arr + ", saturate(" + cmp + ")" + opt + offa + ")",
                          1, GLSL_PRECISION_HIGH);
         }
         return finish(samp + ".sample(" + samp_name + ", " + uv + arr + opt + offa + ")", 4, tex_prec);
      }
      case nir_texop_txf: {
         std::string c = std::string(three ? "uint3(" : dim == GLSL_SAMPLER_DIM_BUF ? "uint(" : "uint2(") + uv +
                         (offset.empty() ? "" : " + " + offset) + ")";
         bool mip = !lod.empty() && dim != GLSL_SAMPLER_DIM_BUF && dim != GLSL_SAMPLER_DIM_RECT;
         return finish(samp + ".read(" + c + arr + (mip ? ", uint(" + lod + ")" : "") + ")", 4, tex_prec);
      }
      case nir_texop_txf_ms:
         return finish(samp + ".read(uint2(" + uv + ")" + arr + ", uint(" + ms + "))", 4, tex_prec);
      case nir_texop_txs: {
         std::string l = lod.empty() ? "" : "uint(" + lod + ")";
         std::vector<std::string> c = {samp + ".get_width(" + l + ")", samp + ".get_height(" + l + ")",
                                       samp + (tex->is_array ? ".get_array_size()" : ".get_depth(" + l + ")")};
         c.resize(tex->def.num_components);
         return int_vec(c);
      }
      case nir_texop_tg4: {
         if (tex->is_shadow)
            return finish(samp + ".gather_compare(" + samp_name + ", " + uv + arr + ", " + cmp + offa + ")", 4, GLSL_PRECISION_HIGH);
         std::string comp;
         if (tex->component) {
            if (offset.empty() && !cube) offa = ", int2(0)";
            comp = fmt(", component::%c", "xyzw"[tex->component & 3]);
         }
         return finish(samp + ".gather(" + samp_name + ", " + uv + arr + offa + comp + ")", 4, tex_prec);
      }
      case nir_texop_query_levels:
         return "int(" + samp + ".get_num_mip_levels())";
      case nir_texop_texture_samples:
         return "int(" + samp + ".get_num_samples())";
      case nir_texop_lod:
         return as_def("float2(" + samp + ".calculate_clamped_lod(" + samp_name + ", " + uv + "), " + samp +
                       ".calculate_unclamped_lod(" + samp_name + ", " + uv + "))", &tex->def);
      default:
         return "/*UNHANDLED texop*/float4(0.0)";
      }
   }

   // ------------------------------------------------------------------
   // intrinsics

   std::string image_coord(nir_intrinsic_instr* in, nir_deref_instr* d, std::string* extra)
   {
      const glsl_type* t = glsl_without_array(d->type);
      unsigned dims = glsl_get_sampler_coordinate_components(t);
      bool array = glsl_sampler_type_is_array(t);
      glsl_sampler_dim dim = glsl_get_sampler_dim(t);
      unsigned spatial = dim == GLSL_SAMPLER_DIM_CUBE ? 2 : dims - (array ? 1 : 0);
      std::string c = src_full(in->src[1], BT_INT);
      std::string ca = is_atom(c) ? c : "(" + c + ")";
      std::string r = spatial == 1 ? "uint(" + ca + ".x)" : fmt("uint%u(", spatial) + ca + "." + std::string("xyz").substr(0, spatial) + ")";
      *extra = "";
      if (dim == GLSL_SAMPLER_DIM_CUBE) {
         // face (and layer for arrays) from z
         *extra = array ? ", uint(" + ca + ".z) % 6u, uint(" + ca + ".z) / 6u" : ", uint(" + ca + ".z)";
      } else if (array) {
         *extra = ", uint(" + ca + "." + "xyzw"[spatial] + ")";
      }
      if (dim == GLSL_SAMPLER_DIM_MS)
         *extra += ", uint(" + src_full(in->src[2], BT_INT) + ")";
      return r;
   }

   std::string atomic_ptr(nir_deref_instr* d, BT bt)
   {
      const char* at = bt == BT_INT ? "atomic_int" : "atomic_uint";
      std::string space = nir_deref_mode_is(d, nir_var_mem_shared) ? "threadgroup" : "device";
      // a vector component has no address of its own: offset from the vector
      nir_deref_instr* p = nir_deref_instr_parent(d);
      if (d->deref_type == nir_deref_type_array && p && glsl_type_is_vector(p->type)) {
         std::string lv = nir_deref_mode_is(d, nir_var_mem_shared) ? nir_printer::deref_str(p) : buffer_deref(p).expr;
         return "((" + space + " " + at + "*)&" + lv + " + " + src_full(d->arr.index, BT_INT) + ")";
      }
      std::string lv = nir_deref_mode_is(d, nir_var_mem_shared) ? nir_printer::deref_str(d) : buffer_deref(d).expr;
      return "(" + space + " " + at + "*)&" + lv;
   }

   std::string counter_ref(nir_deref_instr* d)
   {
      nir_variable* v = nir_deref_instr_get_variable(d);
      unsigned slot = 0;
      for (auto& c : counters)
         if (c.binding == v->data.binding) slot = c.metal_index;
      std::string idx = fmt("%u", v->data.offset / 4);
      // counter arrays: element index
      for (nir_deref_instr* p = d; p && p->deref_type == nir_deref_type_array; p = nir_deref_instr_parent(p)) {
         unsigned stride = glsl_atomic_size(p->type) / 4;
         idx += " + " + (stride == 1 ? "" : fmt("%u * ", stride)) + src_full(p->arr.index, BT_UINT);
      }
      return fmt("&_mtl_atomic%u[", slot) + idx + "]";
   }

   static std::string mem_flags(nir_variable_mode modes)
   {
      std::string f;
      if (modes & (nir_var_mem_ssbo | nir_var_mem_global)) f += "mem_flags::mem_device";
      if (modes & nir_var_image) f += std::string(f.empty() ? "" : " | ") + "mem_flags::mem_texture";
      if (modes & nir_var_mem_shared) f += std::string(f.empty() ? "" : " | ") + "mem_flags::mem_threadgroup";
      return f.empty() ? "mem_flags::mem_none" : f;
   }

   bool intrinsic_dialect(nir_intrinsic_instr* in) override
   {
      switch (in->intrinsic) {
      case nir_intrinsic_load_deref: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         if (!is_buffer_var(nir_deref_instr_get_variable(d)))
            return false;
         define(&in->def, buffer_load(in), d->modes == nir_var_mem_ssbo, false);
         return true;
      }
      case nir_intrinsic_store_deref: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         if (!is_buffer_var(nir_deref_instr_get_variable(d)))
            return false;
         buffer_store(in);
         return true;
      }
      case nir_intrinsic_copy_deref: {
         // blocks of the same layout: copy the storage structs
         nir_deref_instr *d = nir_src_as_deref(in->src[0]), *sr = nir_src_as_deref(in->src[1]);
         if (!is_buffer_var(nir_deref_instr_get_variable(d)))
            return false;
         line(buffer_deref(d).expr + " = " + buffer_deref(sr).expr + ";");
         return true;
      }
      case nir_intrinsic_load_frag_coord:
         define(&in->def, frag_coord(), false, true);
         return true;
      case nir_intrinsic_load_helper_invocation:
         need_version(230);
         define(&in->def, "simd_is_helper_thread()", false, true);
         return true;
      case nir_intrinsic_load_workgroup_size:
         define(&in->def, fmt("uint3(%uu, %uu, %uu)", info.local_size[0], info.local_size[1], info.local_size[2]), false, true);
         return true;
      case nir_intrinsic_load_subgroup_eq_mask: case nir_intrinsic_load_subgroup_ge_mask:
      case nir_intrinsic_load_subgroup_gt_mask: case nir_intrinsic_load_subgroup_le_mask:
      case nir_intrinsic_load_subgroup_lt_mask: {
         helper("_xlat_lane_mask", "inline ulong _xlat_lanes_below(uint n) { return n >= 64u ? ~0ul : ((1ul << n) - 1ul); }\n"
                "inline uint4 _xlat_lane_mask(uint lo, uint hi) { ulong m = _xlat_lanes_below(hi) & ~_xlat_lanes_below(lo); return uint4(uint(m), uint(m >> 32), 0u, 0u); }\n");
         std::string l = lane();
         const char* r = in->intrinsic == nir_intrinsic_load_subgroup_eq_mask ? "%s, %s + 1u" :
                         in->intrinsic == nir_intrinsic_load_subgroup_ge_mask ? "%s, 64u" :
                         in->intrinsic == nir_intrinsic_load_subgroup_gt_mask ? "%s + 1u, 64u" :
                         in->intrinsic == nir_intrinsic_load_subgroup_le_mask ? "0u, %s + 1u" : "0u, %s";
         std::string args = fmt(r, l.c_str(), l.c_str());
         define(&in->def, "_xlat_lane_mask(" + args + ")", false, true);
         return true;
      }
      case nir_intrinsic_barrier: {
         mesa_scope es = nir_intrinsic_execution_scope(in);
         mesa_scope ms = nir_intrinsic_memory_scope(in);
         nir_variable_mode modes = (nir_variable_mode)nir_intrinsic_memory_modes(in);
         if (es == SCOPE_NONE && ms == SCOPE_NONE)
            return true;
         if (es == SCOPE_NONE) {
            // a shared-memory barrier right before an execution barrier is part of it
            if (shared_fence_before_barrier(in))
               return true;
            need_version(320);
            line("atomic_thread_fence(" + mem_flags(modes) + ", memory_order_seq_cst" +
                 (ms == SCOPE_SUBGROUP ? ", thread_scope_simdgroup" : ms == SCOPE_WORKGROUP ? ", thread_scope_threadgroup" : ", thread_scope_device") + ");");
            return true;
         }
         nir_instr* prev = nir_instr_prev(&in->instr);
         if (prev && prev->type == nir_instr_type_intrinsic && shared_fence_before_barrier(nir_instr_as_intrinsic(prev)))
            modes = (nir_variable_mode)(modes | nir_var_mem_shared);
         line(std::string(es == SCOPE_SUBGROUP ? "simdgroup_barrier(" : "threadgroup_barrier(") + mem_flags(modes) + ");");
         return true;
      }
      case nir_intrinsic_deref_atomic:
      case nir_intrinsic_deref_atomic_swap: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         BT bt = bt_from_glsl(d->type);
         std::string ptr = atomic_ptr(d, bt);
         std::string v = src_full(in->src[1], bt, GLSL_PRECISION_HIGH);
         if (in->intrinsic == nir_intrinsic_deref_atomic_swap) {
            helper("_xlat_atomic_comp_swap",
                   "template <typename P, typename T> inline T _xlat_atomic_comp_swap(P p, T compare, T value) { T expected = compare; atomic_compare_exchange_weak_explicit(p, &expected, value, memory_order_relaxed, memory_order_relaxed); return expected; }\n");
            define_side_effect(&in->def, as_def("_xlat_atomic_comp_swap(" + ptr + ", " + v + ", " + src_full(in->src[2], bt, GLSL_PRECISION_HIGH) + ")", &in->def));
            return true;
         }
         define_side_effect(&in->def, as_def(std::string(metal_atomic(nir_intrinsic_atomic_op(in))) + "_explicit(" + ptr + ", " + v + ", memory_order_relaxed)", &in->def));
         return true;
      }
      case nir_intrinsic_image_deref_load: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         std::string extra;
         std::string c = image_coord(in, d, &extra);
         define(&in->def, as_def(deref_str(d) + ".read(" + c + extra + ")", &in->def), true, false);
         return true;
      }
      case nir_intrinsic_image_deref_store: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         std::string extra;
         std::string c = image_coord(in, d, &extra);
         BT bt = bt_from_alu(nir_intrinsic_src_type(in));
         std::string val = src_full(in->src[3], bt, GLSL_PRECISION_HIGH);
         if (in->src[3].ssa->num_components < 4)
            val = vec_type(bt, 4, GLSL_PRECISION_HIGH) + "(" + val + ")";
         line(deref_str(d) + ".write(" + val + ", " + c + extra + ");");
         return true;
      }
      case nir_intrinsic_image_deref_atomic:
      case nir_intrinsic_image_deref_atomic_swap: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         std::string extra;
         std::string c = image_coord(in, d, &extra);
         BT bt = type[in->def.index];
         need_version(310);
         if (glsl_get_sampler_dim(glsl_without_array(d->type)) == GLSL_SAMPLER_DIM_CUBE)
            need_version(400);
         std::string v = src_full(in->src[3], bt, GLSL_PRECISION_HIGH);
         std::string vec = vec_type(bt, 4, GLSL_PRECISION_HIGH);
         if (in->intrinsic == nir_intrinsic_image_deref_atomic_swap) {
            helper("_xlat_image_comp_swap",
                   "template <typename Tex, typename C, typename T> inline T _xlat_image_comp_swap(Tex tex, C coord, T compare, T value) { vec<T, 4> expected(compare); tex.atomic_compare_exchange_weak(coord, &expected, vec<T, 4>(value)); return expected.x; }\n"
                   "template <typename Tex, typename C, typename T> inline T _xlat_image_comp_swap(Tex tex, C coord, uint slice, T compare, T value) { vec<T, 4> expected(compare); tex.atomic_compare_exchange_weak(coord, slice, &expected, vec<T, 4>(value)); return expected.x; }\n"
                   "template <typename Tex, typename C, typename T> inline T _xlat_image_comp_swap(Tex tex, C coord, uint face, uint slice, T compare, T value) { vec<T, 4> expected(compare); tex.atomic_compare_exchange_weak(coord, face, slice, &expected, vec<T, 4>(value)); return expected.x; }\n");
            define_side_effect(&in->def, as_def("_xlat_image_comp_swap(" + deref_str(d) + ", " + c + extra + ", " + v + ", " + src_full(in->src[4], bt, GLSL_PRECISION_HIGH) + ")", &in->def));
            return true;
         }
         define_side_effect(&in->def, as_def(deref_str(d) + "." + metal_atomic(nir_intrinsic_atomic_op(in)) + "(" + c + extra + ", " + vec + "(" + v + ")).x", &in->def));
         return true;
      }
      case nir_intrinsic_image_deref_size: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         const glsl_type* t = glsl_without_array(d->type);
         std::string img = deref_str(d);
         std::vector<std::string> c = {img + ".get_width()"};
         switch (glsl_get_sampler_dim(t)) {
         case GLSL_SAMPLER_DIM_1D: case GLSL_SAMPLER_DIM_BUF: break;
         case GLSL_SAMPLER_DIM_3D: c.push_back(img + ".get_height()"); c.push_back(img + ".get_depth()"); break;
         default: c.push_back(img + ".get_height()"); break;
         }
         if (glsl_sampler_type_is_array(t)) c.push_back(img + ".get_array_size()");
         c.resize(in->def.num_components, "1");
         define(&in->def, int_vec(c), false, false);
         return true;
      }
      case nir_intrinsic_image_deref_samples:
         define(&in->def, "int(" + deref_str(nir_src_as_deref(in->src[0])) + ".get_num_samples())", false, false);
         return true;
      case nir_intrinsic_deref_buffer_array_length: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         nir_variable* v = nir_deref_instr_get_variable(d);
         const glslopt_buffer_block& b = blocks[block_index[v->interface_type]];
         bool std430 = glslopt_buffer_uses_std430(b, true);
         unsigned last = glsl_get_length(b.type) - 1;
         bool row = glsl_matrix_type_is_row_major(b.type);
         unsigned off = glslopt_buffer_field_offsets(b.type, row, std430)[last];
         const glsl_struct_field& lf = *glsl_get_struct_field_data(b.type, last);
         unsigned stride = glslopt_buffer_array_stride(glsl_get_array_element(lf.type), glslopt_field_row_major(lf, row), std430);
         std::string idx = fmt("%u", b.metal_index);
         if (b.named && b.array_size > 1) {
            nir_deref_instr* p = d;
            while (p->deref_type != nir_deref_type_array && nir_deref_instr_parent(p)) p = nir_deref_instr_parent(p);
            if (p->deref_type == nir_deref_type_array)
               idx += " + " + src_full(p->arr.index, BT_UINT);
         }
         define(&in->def, fmt("(max(int(_mtl_buffer_sizes[%s]) - %u, 0) / %u)", idx.c_str(), off, stride), false, false);
         return true;
      }
      case nir_intrinsic_atomic_counter_read_deref:
         define_side_effect(&in->def, "atomic_load_explicit(" + counter_ref(nir_src_as_deref(in->src[0])) + ", memory_order_relaxed)");
         return true;
      case nir_intrinsic_atomic_counter_inc_deref:
         define_side_effect(&in->def, "atomic_fetch_add_explicit(" + counter_ref(nir_src_as_deref(in->src[0])) + ", 1u, memory_order_relaxed)");
         return true;
      case nir_intrinsic_atomic_counter_pre_dec_deref:
         define_side_effect(&in->def, "(atomic_fetch_sub_explicit(" + counter_ref(nir_src_as_deref(in->src[0])) + ", 1u, memory_order_relaxed) - 1u)");
         return true;
      case nir_intrinsic_atomic_counter_post_dec_deref:
         define_side_effect(&in->def, "atomic_fetch_sub_explicit(" + counter_ref(nir_src_as_deref(in->src[0])) + ", 1u, memory_order_relaxed)");
         return true;
      default:
         return false;
      }
   }

   static bool shared_fence_before_barrier(nir_intrinsic_instr* in)
   {
      nir_instr* next = nir_instr_next(&in->instr);
      return in->intrinsic == nir_intrinsic_barrier && nir_intrinsic_execution_scope(in) == SCOPE_NONE &&
             nir_intrinsic_memory_modes(in) == nir_var_mem_shared &&
             next && next->type == nir_instr_type_intrinsic &&
             nir_instr_as_intrinsic(next)->intrinsic == nir_intrinsic_barrier &&
             nir_intrinsic_execution_scope(nir_instr_as_intrinsic(next)) >= MIN2(nir_intrinsic_memory_scope(in), SCOPE_WORKGROUP);
   }

   static std::string int_vec(const std::vector<std::string>& c)
   {
      std::string e = c.size() == 1 ? "int(" : fmt("int%u(", (unsigned)c.size());
      for (size_t i = 0; i < c.size(); i++)
         e += (i ? ", " : "") + c[i];
      return e + ")";
   }

   std::string builtin_ref(const builtin_var& b, nir_def* d) override
   {
      if (!b.metal_attr)
         return "";
      return as_def(sysval(vec_type(b.bt, b.n, GLSL_PRECISION_HIGH).c_str(), b.name, b.metal_attr), d, b.bt);
   }

   static const char* metal_atomic(nir_atomic_op op)
   {
      switch (op) {
      case nir_atomic_op_iadd: return "atomic_fetch_add";
      case nir_atomic_op_imin: case nir_atomic_op_umin: return "atomic_fetch_min";
      case nir_atomic_op_imax: case nir_atomic_op_umax: return "atomic_fetch_max";
      case nir_atomic_op_iand: return "atomic_fetch_and";
      case nir_atomic_op_ior: return "atomic_fetch_or";
      case nir_atomic_op_ixor: return "atomic_fetch_xor";
      case nir_atomic_op_xchg: return "atomic_exchange";
      default: return "/*UNHANDLED atomic*/atomic_fetch_add";
      }
   }

   std::string frag_coord()
   {
      if (sysvals_done.insert("gl_FragCoord").second)
         inputs += "  float4 gl_FragCoord [[position]];\n";
      return "_mtl_i.gl_FragCoord";
   }

   std::string lane() { return sysval("uint", "gl_SubgroupInvocationID", "thread_index_in_simdgroup"); }

   // ------------------------------------------------------------------
   // subgroups -> SIMD-group functions

   std::string subgroup_expr(nir_intrinsic_instr* in) override
   {
      std::string r = subgroup_op(in);
      if (!r.empty())
         need_version(230);
      return r;
   }

   std::string subgroup_op(nir_intrinsic_instr* in)
   {
      // SIMD-group functions take no bool vectors: pass those through uint
      bool via_uint = type[in->def.index] == BT_BOOL && glslopt_is_subgroup_value_op(in->intrinsic) &&
                      in->intrinsic != nir_intrinsic_reduce && in->intrinsic != nir_intrinsic_inclusive_scan &&
                      in->intrinsic != nir_intrinsic_exclusive_scan;
      std::string r = subgroup_value_op(in, via_uint);
      return via_uint ? vec_type(BT_BOOL, in->def.num_components, 0) + "(" + r + ")" : r;
   }

   std::string subgroup_value_op(nir_intrinsic_instr* in, bool via_uint)
   {
      auto v0 = [&]() {
         return via_uint ? vec_type(BT_UINT, in->def.num_components, 0) + "(" + src_full(in->src[0], BT_BOOL) + ")"
                         : src_full(in->src[0], type[in->def.index]);
      };
      auto u1 = [&]() { return "ushort(" + src_full(in->src[1], BT_UINT) + ")"; };
      auto ballot_helpers = [&]() {
         helper("_xlat_ballot",
                "inline uint4 _xlat_ballot(bool b) { ulong m = ulong(simd_ballot(b)); return uint4(uint(m), uint(m >> 32), 0u, 0u); }\n"
                "inline ulong _xlat_ballot_bits(uint4 b) { return ulong(b.x) | (ulong(b.y) << 32); }\n");
      };
      switch (in->intrinsic) {
      case nir_intrinsic_elect: return "simd_is_first()";
      case nir_intrinsic_read_first_invocation: return "simd_broadcast_first(" + v0() + ")";
      case nir_intrinsic_read_invocation:
         return std::string(nir_src_is_const(in->src[1]) ? "simd_broadcast(" : "simd_shuffle(") + v0() + ", " + u1() + ")";
      case nir_intrinsic_vote_any: return "simd_any(" + src_full(in->src[0], BT_BOOL) + ")";
      case nir_intrinsic_vote_all: return "simd_all(" + src_full(in->src[0], BT_BOOL) + ")";
      case nir_intrinsic_vote_ieq: case nir_intrinsic_vote_feq: {
         BT t = in->intrinsic == nir_intrinsic_vote_feq ? BT_FLOAT : type[in->src[0].ssa->index];
         std::string v = src_full(in->src[0], t);
         std::string eq = "(" + v + " == simd_broadcast_first(" + v + "))";
         return "simd_all(" + (in->src[0].ssa->num_components > 1 ? "all" + eq : eq) + ")";
      }
      case nir_intrinsic_ballot:
         ballot_helpers();
         return "_xlat_ballot(" + src_full(in->src[0], BT_BOOL) + ")";
      case nir_intrinsic_inverse_ballot:
         ballot_helpers();
         return "(((_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ") >> " + lane() + ") & 1ul) != 0ul)";
      case nir_intrinsic_ballot_bitfield_extract:
         ballot_helpers();
         return "(((_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ") >> " + src_full(in->src[1], BT_UINT) + ") & 1ul) != 0ul)";
      case nir_intrinsic_ballot_bit_count_reduce:
         ballot_helpers();
         return "uint(popcount(_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ")))";
      case nir_intrinsic_ballot_bit_count_inclusive:
         ballot_helpers();
         return "uint(popcount(_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ") & ((2ul << " + lane() + ") - 1ul)))";
      case nir_intrinsic_ballot_bit_count_exclusive:
         ballot_helpers();
         return "uint(popcount(_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ") & ((1ul << " + lane() + ") - 1ul)))";
      case nir_intrinsic_ballot_find_lsb:
         ballot_helpers();
         return "uint(ctz(_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ")))";
      case nir_intrinsic_ballot_find_msb:
         ballot_helpers();
         return "uint(63 - clz(_xlat_ballot_bits(" + src_full(in->src[0], BT_UINT) + ")))";
      case nir_intrinsic_shuffle: return "simd_shuffle(" + v0() + ", " + u1() + ")";
      case nir_intrinsic_shuffle_xor: return "simd_shuffle_xor(" + v0() + ", " + u1() + ")";
      case nir_intrinsic_shuffle_up: return "simd_shuffle_up(" + v0() + ", " + u1() + ")";
      case nir_intrinsic_shuffle_down: return "simd_shuffle_down(" + v0() + ", " + u1() + ")";
      case nir_intrinsic_quad_broadcast: return "quad_broadcast(" + v0() + ", " + u1() + ")";
      case nir_intrinsic_quad_swap_horizontal: return "quad_shuffle_xor(" + v0() + ", 1)";
      case nir_intrinsic_quad_swap_vertical: return "quad_shuffle_xor(" + v0() + ", 2)";
      case nir_intrinsic_quad_swap_diagonal: return "quad_shuffle_xor(" + v0() + ", 3)";
      case nir_intrinsic_reduce:
      case nir_intrinsic_inclusive_scan:
      case nir_intrinsic_exclusive_scan:
         return subgroup_reduce(in);
      default:
         return "";
      }
   }

   std::string subgroup_reduce(nir_intrinsic_instr* in)
   {
      nir_op op = (nir_op)nir_intrinsic_reduction_op(in);
      BT vt = type[in->def.index];
      std::string v = src_full(in->src[0], vt);
      const char* name;
      const char* combine; // binary form for emulated scans/clusters
      switch (op) {
      case nir_op_iadd: case nir_op_fadd: name = "sum"; combine = "a + b"; break;
      case nir_op_imul: case nir_op_fmul: name = "product"; combine = "a * b"; break;
      case nir_op_imin: case nir_op_umin: case nir_op_fmin: name = "min"; combine = "min(a, b)"; break;
      case nir_op_imax: case nir_op_umax: case nir_op_fmax: name = "max"; combine = "max(a, b)"; break;
      case nir_op_iand: name = "and"; combine = vt == BT_BOOL ? "a && b" : "a & b"; break;
      case nir_op_ior: name = "or"; combine = vt == BT_BOOL ? "a || b" : "a | b"; break;
      case nir_op_ixor: name = "xor"; combine = vt == BT_BOOL ? "a != b" : "a ^ b"; break;
      default: return "/*UNHANDLED reduction*/" + v;
      }
      // identity value of the operation
      unsigned n = in->def.num_components;
      std::string tn = vec_type(vt, n, prec_of(&in->def));
      std::string ident;
      switch (op) {
      case nir_op_iadd: case nir_op_fadd: case nir_op_ior: case nir_op_ixor: case nir_op_umax: ident = tn + "(0)"; break;
      case nir_op_imul: case nir_op_fmul: ident = tn + "(1)"; break;
      case nir_op_iand: case nir_op_umin: ident = vt == BT_BOOL ? tn + "(true)" : tn + "(~0u)"; break;
      case nir_op_imin: ident = tn + "(INT_MAX)"; break;
      case nir_op_imax: ident = tn + "(INT_MIN)"; break;
      case nir_op_fmin: ident = tn + "(INFINITY)"; break;
      case nir_op_fmax: ident = tn + "(-INFINITY)"; break;
      default: ident = tn + "(0)"; break;
      }
      bool boolv = vt == BT_BOOL;
      if (in->intrinsic == nir_intrinsic_reduce) {
         unsigned cluster = nir_intrinsic_cluster_size(in);
         if (!cluster) {
            if (boolv) // no bool SIMD reductions: go through uint
               return vec_type(BT_BOOL, n, 0) + "(simd_" + name + "(" + vec_type(BT_UINT, n, 0) + "(" + v + ")))";
            return std::string("simd_") + name + "(" + v + ")";
         }
         // clustered: butterfly over the cluster with shuffle_xor
         std::string hn = std::string("_xlat_clustered_") + name;
         helper(hn, fmt("template <typename T> inline T %s(T x, uint size) { for (ushort i = 1; i < size; i <<= 1) { T a = x; T b = simd_shuffle_xor(x, i); x = %s; } return x; }\n",
                        hn.c_str(), combine));
         return hn + "(" + v + fmt(", %uu)", cluster);
      }
      bool inclusive = in->intrinsic == nir_intrinsic_inclusive_scan;
      if (!boolv && (op == nir_op_iadd || op == nir_op_fadd || op == nir_op_imul || op == nir_op_fmul))
         return std::string(inclusive ? "simd_prefix_inclusive_" : "simd_prefix_exclusive_") + name + "(" + v + ")";
      // other scans: Hillis-Steele with shuffle_up; lanes below the shift keep x
      std::string hn = std::string("_xlat_scan_") + name;
      helper(hn, fmt("template <typename T> inline T %s(T x, uint lane, T identity, bool inclusive) { if (!inclusive) { x = simd_shuffle_up(x, 1); if (lane == 0) x = identity; } for (ushort i = 1; i < 64; i <<= 1) { T a = x; T b = simd_shuffle_up(x, i); if (lane >= i) x = %s; } return x; }\n",
                     hn.c_str(), combine));
      return hn + "(" + v + ", " + lane() + ", " + ident + (inclusive ? ", true)" : ", false)");
   }

   // ------------------------------------------------------------------
   // declarations

   void get_metal_type_size(const glsl_type* type, unsigned prec, int& size, int& alignment)
   {
      if (glsl_type_is_array(type)) {
         get_metal_type_size(glsl_get_array_element(type), prec, size, alignment);
         size = align(size, alignment) * glsl_get_length(type);
         return;
      }
      if (glsl_type_is_struct(type)) {
         size = 0; alignment = 1;
         for (unsigned i = 0; i < glsl_get_length(type); ++i) {
            int fs, fa;
            const glsl_struct_field* f = glsl_get_struct_field_data(type, i);
            get_metal_type_size(f->type, f->precision, fs, fa);
            size = align(size, fa) + fs;
            alignment = MAX2(alignment, fa);
         }
         size = align(size, alignment);
         return;
      }
      int scalar = glsl_type_is_boolean(type) ? 1 : eff(prec) == GLSL_PRECISION_MEDIUM ? 2 : 4;
      int vector = glsl_get_vector_elements(type) == 3 ? 4 : glsl_get_vector_elements(type);
      alignment = scalar * vector;
      size = alignment * glsl_get_matrix_columns(type);
   }

   static std::string interpolation(unsigned mode, bool centroid, bool sample)
   {
      if (mode == INTERP_MODE_FLAT)
         return " [[flat]]";
      if (centroid || sample || mode != INTERP_MODE_NONE)
         return fmt(" [[%s_%s]]", sample ? "sample" : centroid ? "centroid" : "center",
                    mode == INTERP_MODE_NOPERSPECTIVE ? "no_perspective" : "perspective");
      return "";
   }

   std::set<int> explicit_attribute_slots;
   int attribute_counter = 0, color_counter = 0;

   int next_attribute_slot(int slots)
   {
      for (int slot = attribute_counter; slot < attribute_counter + slots; ++slot)
         if (explicit_attribute_slots.count(slot))
            attribute_counter = slot + 1;
      int r = attribute_counter;
      attribute_counter += slots;
      return r;
   }

   // stage interface field(s) for a value of type t; aggregates flatten into
   // one field per vector, copied to/from a local of the original type
   void stage_fields(nir_variable* v, const glsl_type* t, unsigned prec, const std::string& local, bool input, int location,
                     unsigned interp, bool centroid, bool sample, int* attribute, int element_offset = 0)
   {
      if (glsl_type_is_struct(t) || glsl_type_is_interface(t)) {
         bool block = glsl_type_is_interface(t);
         for (unsigned i = 0; i < glsl_get_length(t); ++i) {
            const glsl_struct_field& f = *glsl_get_struct_field_data(t, i);
            // member locations are for the first element of a block array
            int fl = block && f.location >= 0 ? f.location - VARYING_SLOT_VAR0 + element_offset : location;
            stage_fields(v, f.type, f.precision ? f.precision : prec, local + "." + f.name, input, fl,
                         block ? f.interpolation : interp, block ? f.centroid : centroid, block ? f.sample : sample, attribute);
            if (!(block && f.location >= 0) && location >= 0) location += glsl_count_attribute_slots(f.type, false);
         }
         return;
      }
      if (glsl_type_is_array(t) || glsl_type_is_matrix(t)) {
         const glsl_type* e = glsl_type_is_array(t) ? glsl_get_array_element(t) : glsl_get_column_type(t);
         unsigned count = glsl_type_is_array(t) ? glsl_get_length(t) : glsl_get_matrix_columns(t);
         unsigned slots = glsl_count_attribute_slots(e, false);
         for (unsigned i = 0; i < count; ++i) {
            stage_fields(v, e, prec, local + fmt("[%u]", i), input, location, interp, centroid, sample, attribute,
                         element_offset + (int)(i * slots));
            if (location >= 0) location += slots;
         }
         return;
      }
      std::string field = fmt("_xlat_varying%d", varying_counter++);
      if (input)
         copy_in += "  " + local + " = _mtl_i." + field + ";\n";
      else
         copy_out.push_back("_mtl_o." + field + " = " + local + ";");
      (input ? inputs : outputs) += stage_field(v, t, prec, field, input, location, interp, centroid, sample, attribute);
   }

   // declaration of one stage interface field
   std::string stage_field(nir_variable* v, const glsl_type* t, unsigned prec, const std::string& field, bool input,
                           int location, unsigned interp, bool centroid, bool sample, int* attribute)
   {
      std::string decl = "  " + base_type_name(t, prec) + " " + field + stage_attribute(v, input, location, attribute);
      if (stage_is_varying(v, input))
         decl += interpolation(glsl_type_is_integer(t) ? (unsigned)INTERP_MODE_FLAT : interp, centroid, sample);
      return decl + ";\n";
   }

   bool stage_is_varying(nir_variable* v, bool input)
   {
      return ((is_fragment() && input) || (is_vertex() && !input)) && strncmp(v->name, "gl_", 3) != 0;
   }

   std::string stage_attribute(nir_variable* v, bool input, int location, int* attribute)
   {
      const char* nm = v->name;
      if (is_vertex() && input) {
         int slot = (*attribute)++;
         return fmt(" [[attribute(%d)]]", slot);
      }
      if (!strcmp(nm, "gl_Position")) return v->data.invariant ? " [[position]] [[invariant]]" : " [[position]]";
      if (!strcmp(nm, "gl_PointSize")) return " [[point_size]]";
      if (!strcmp(nm, "gl_FragDepth")) return " [[depth(any)]]";
      if (!strcmp(nm, "gl_FragCoord")) return " [[position]]";
      if (is_fragment() && !input) {
         int loc = location >= 0 ? location : color_counter;
         color_counter = MAX2(color_counter, loc + 1);
         return fmt(" [[color(%d)]]", loc);
      }
      if (stage_is_varying(v, input) && location >= 0)
         return fmt(" [[user(locn%d)]]", location);
      return "";
   }

   void declare_stage_var(nir_variable* v, const std::set<const nir_variable*>& referenced)
   {
      bool input = v->data.mode == nir_var_shader_in;
      if (!strncmp(v->name, "gl_", 3) && !referenced.count(v))
         return;
      if (is_compute())
         return;
      int location = -1;
      if (v->data.explicit_location) {
         if (is_vertex() && input)
            location = v->data.location - VERT_ATTRIB_GENERIC0;
         else if (is_fragment() && !input)
            location = v->data.location - FRAG_RESULT_DATA0;
         else
            location = v->data.location - VARYING_SLOT_VAR0;
      }
      if (!strcmp(v->name, "gl_FragColor") || !strcmp(v->name, "gl_FragData"))
         location = 0;
      int attribute = 0;
      if (is_vertex() && input) {
         int slots = glsl_count_attribute_slots(v->type, true);
         attribute = v->data.explicit_location && location >= 0 ? location : next_attribute_slot(slots);
         layout.attribute_slot[v] = attribute;
      }
      const glsl_type* t = v->type;
      bool simple = glsl_type_is_vector_or_scalar(t);
      if (simple) {
         std::string field = v->name;
         std::string decl = stage_field(v, t, v->data.precision, field, input, location, v->data.interpolation,
                                        v->data.centroid, v->data.sample, &attribute);
         if (v->data.fb_fetch_output) {
            inputs += decl;
            outputs += decl;
            copy_in += "  _mtl_o." + field + " = _mtl_i." + field + ";\n";
         } else {
            (input ? inputs : outputs) += decl;
         }
         var_names[v] = std::string(input ? "_mtl_i." : "_mtl_o.") + field;
         return;
      }
      std::string local = unique_name(v->name);
      var_names[v] = local;
      pre_decl(type_decl(t, v->data.precision, local) + ";");
      stage_fields(v, t, v->data.precision, local, input, location, v->data.interpolation, v->data.centroid, v->data.sample, &attribute);
   }

   // declarations need names before the body is printed
   void prologue() override
   {
      std::set<const nir_variable*> referenced = glslopt_referenced_vars(s);

      // names visible in the entry function: parameters and declared locals
      // get unique names; stage/uniform fields live in their structs
      used_names.insert("_mtl_i");
      used_names.insert("_mtl_o");
      used_names.insert("_mtl_u");
      nir_foreach_variable_with_modes(v, s, nir_var_uniform | nir_var_image)
         if (v->name && (glsl_type_is_sampler(glsl_without_array(v->type)) || glsl_type_is_image(glsl_without_array(v->type))))
            used_names.insert(v->name);

      blocks = glslopt_collect_buffer_blocks(s);
      for (unsigned i = 0; i < blocks.size(); i++)
         block_index[blocks[i].type] = i;
      size_table_index = glslopt_buffer_size_table_index(blocks);
      counters = glslopt_collect_atomic_counter_buffers(s, blocks);

      nir_foreach_variable_with_modes(v, s, nir_var_shader_in)
         if (is_vertex() && v->data.explicit_location && strncmp(v->name, "gl_", 3)) {
            int base = v->data.location - VERT_ATTRIB_GENERIC0;
            for (int i = 0; i < (int)glsl_count_attribute_slots(v->type, true); i++)
               explicit_attribute_slots.insert(base + i);
         }

      int sampler_count = 0;
      nir_foreach_variable_with_modes(v, s, nir_var_uniform)
         if (glsl_type_is_sampler(glsl_without_array(v->type)))
            sampler_count++;
      int texture_slot = 0, image_slot = sampler_count;
      int uniform_offset = 0;

      std::string shared_decls;
      nir_foreach_variable_in_shader(v, s) {
         const glsl_type* bare = glsl_without_array(v->type);
         switch (v->data.mode) {
         case nir_var_shader_in:
         case nir_var_shader_out:
            if (!strcmp(v->name, "gl_FragCoord") && v->data.mode == nir_var_shader_in) {
               var_names[v] = frag_coord();
               break;
            }
            declare_stage_var(v, referenced);
            break;
         case nir_var_uniform:
         case nir_var_image:
            if (glsl_contains_atomic(v->type))
               break;
            if (glsl_type_is_sampler(bare)) {
               int slot = texture_slot++;
               params += "\n  , " + texture_type(bare, v->data.precision, 0) + " " + v->name + fmt(" [[texture(%d)]], sampler _mtlsmp_", slot) + v->name + fmt(" [[sampler(%d)]]", slot);
               layout.texture_slot[v] = slot;
            } else if (glsl_type_is_image(bare)) {
               int slot = image_slot;
               int count = glsl_type_is_array(v->type) ? (int)glsl_get_length(v->type) : 1;
               image_slot += count;
               std::string tt = texture_type(bare, v->data.precision, v->data.access);
               if (v->data.access & ACCESS_COHERENT)
                  need_version(320);
               if (glsl_type_is_array(v->type))
                  params += "\n  , array<" + tt + fmt(", %d> ", count) + v->name + fmt(" [[texture(%d)]]", slot);
               else
                  params += "\n  , " + tt + " " + v->name + fmt(" [[texture(%d)]]", slot);
               layout.texture_slot[v] = slot;
            } else {
               int size, align;
               get_metal_type_size(v->type, v->data.precision, size, align);
               uniform_offset = ::align(uniform_offset, align);
               layout.uniform_offset[v] = uniform_offset;
               uniform_offset += size;
               uniforms += "  " + type_decl(v->type, v->data.precision, v->name) + ";\n";
               var_names[v] = std::string("_mtl_u.") + v->name;
            }
            break;
         case nir_var_mem_shared: {
            // matrices as arrays of columns: threadgroup memory has no matrix constructors
            const glsl_type* t = v->type;
            std::string dims = array_dims(t, false);
            std::string nm = unique_name(v->name);
            var_names[v] = nm;
            if (glsl_type_is_matrix(t))
               shared_decls += "  threadgroup " + base_type_name(glsl_get_column_type(t), v->data.precision) + " " + nm + dims +
                               fmt("[%u];\n", glsl_get_matrix_columns(t));
            else
               shared_decls += "  threadgroup " + base_type_name(t, v->data.precision) + " " + nm + dims + ";\n";
            break;
         }
         default:
            break;
         }
      }
      // blocks
      for (const auto& b : blocks) {
         bool std430 = glslopt_buffer_uses_std430(b, true);
         std::string st = storage_struct(b.type, glsl_matrix_type_is_row_major(b.type), std430);
         bool storage = glslopt_block_is_storage(b);
         bool readonly = storage && (b.variable->data.access & ACCESS_NON_WRITEABLE);
         if (storage && b.variable->data.access & ACCESS_COHERENT)
            need_version(320);
         std::string space = !storage ? "constant" : readonly ? "const device" : "device";
         std::string coherent = storage && (b.variable->data.access & ACCESS_COHERENT) ? " coherent(device)" : "";
         std::string nm = b.named ? unique_name(b.variable->name) : unique_name((std::string("_xlat_blk_") + glsl_get_type_name(b.type)).c_str());
         if (b.array_size > 1) {
            std::string list;
            for (unsigned i = 0; i < b.array_size; i++) {
               block_params += "\n  , " + space + coherent + " " + st + "& " + nm + fmt("_%u [[buffer(%u)]]", i, b.metal_index + i);
               list += (i ? ", &" : "&") + nm + fmt("_%u", i);
            }
            pre_decl(space + coherent + " " + st + "* " + nm + fmt("[%u] = { ", b.array_size) + list + " };");
         } else {
            block_params += "\n  , " + space + coherent + " " + st + "& " + nm + fmt(" [[buffer(%u)]]", b.metal_index);
         }
         var_names[b.variable] = nm;
      }
      // Metal vertex functions must output a position
      if (is_vertex() && outputs.find("[[position]]") == std::string::npos) {
         outputs += "  float4 gl_Position [[position]];\n";
         copy_in += "  _mtl_o.gl_Position = float4(0.0);\n";
      }
      if (size_table_index)
         block_params += fmt("\n  , constant uint* _mtl_buffer_sizes [[buffer(%u)]]", size_table_index);
      for (const auto& c : counters)
         block_params += fmt("\n  , device atomic_uint* _mtl_atomic%u [[buffer(%u)]]", c.metal_index, c.metal_index);
      layout.uniforms_size = uniform_offset;
      pre_decls = shared_decls + pre_decls;
   }

   std::string assemble(const std::string& decls, const std::string& stmts) override
   {
      std::string head = "#include <metal_stdlib>\n#pragma clang diagnostic ignored \"-Wparentheses-equality\"\nusing namespace metal;\n";
      if (min_metal_version)
         head += fmt("#if __METAL_VERSION__ < %u\n#error This shader requires Metal %u.%u or newer\n#endif\n",
                     min_metal_version, min_metal_version / 100, (min_metal_version / 10) % 10);
      std::string r = head + prefix;
      if (!is_compute()) {
         r += "struct xlatMtlShaderInput {\n" + inputs + "};\n";
         r += "struct xlatMtlShaderOutput {\n" + outputs + "};\n";
      }
      r += "struct xlatMtlShaderUniform {\n" + uniforms + "};\n";
      if (is_compute()) {
         r += fmt("// threadsPerThreadgroup: %u, %u, %u\n", info.local_size[0], info.local_size[1], info.local_size[2]);
         r += "kernel void xlatMtlMain (constant xlatMtlShaderUniform& _mtl_u [[buffer(0)]]" + block_params + params + ")\n{\n";
         r += decls + stmts + "}\n";
      } else {
         r += std::string(is_vertex() ? "vertex" : "fragment") +
              " xlatMtlShaderOutput xlatMtlMain (xlatMtlShaderInput _mtl_i [[stage_in]], constant xlatMtlShaderUniform& _mtl_u [[buffer(0)]]" +
              block_params + params + ")\n{\n";
         r += "  xlatMtlShaderOutput _mtl_o;\n" + decls + copy_in + stmts;
         for (const std::string& c : copy_out)
            r += "  " + c + "\n";
         r += "  return _mtl_o;\n}\n";
      }
      return r;
   }
};

} // namespace

std::string glslopt_print_metal(nir_shader* nir, const glslopt_source_info& info, glslopt_metal_layout& layout)
{
   metal_printer p(nir, info, layout);
   return p.print();
}
