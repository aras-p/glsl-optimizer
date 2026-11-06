// NIR -> GLSL / GLSL ES source.
#include "nir_printer.h"
#include "image_formats.h"
#include "main/menums.h"
#include "compiler/shader_enums.h"
#include <string.h>

#define fmt printer_fmt

namespace {

class glsl_printer : public nir_printer {
public:
   glsl_printer(nir_shader* nir, const glslopt_source_info& info)
      : nir_printer(nir), info(info)
   {
      legacy = !info.es && info.version < 130;
   }

private:
   const glslopt_source_info& info;
   bool legacy; // desktop GLSL before 1.30: attribute/varying, texture2D etc.
   std::set<const glsl_type*> emitted_structs;
   std::string globals;

   // ------------------------------------------------------------------
   std::string vec_type(BT bt, unsigned n, unsigned) override
   {
      switch (bt) {
      case BT_INT: return glsl_get_type_name(glsl_ivec_type(n));
      case BT_UINT: return glsl_get_type_name(legacy ? glsl_ivec_type(n) : glsl_uvec_type(n));
      case BT_BOOL: return glsl_get_type_name(glsl_bvec_type(n));
      default: return glsl_get_type_name(glsl_vec_type(n));
      }
   }

   std::string type_decl(const glsl_type* t, unsigned, const std::string& nm) override
   {
      const glsl_type* e = t;
      std::string dims = array_dims(e);
      std::string tn = glsl_get_type_name(e);
      if (legacy && (tn == "uint" || !strncmp(tn.c_str(), "uvec", 4)))
         tn = tn == "uint" ? "int" : "i" + tn.substr(1);
      return tn + " " + nm + dims;
   }
   bool has_uint() override { return !legacy; }

   const char* prec_name(unsigned p)
   {
      if (!info.es)
         return "";
      switch (p) {
      case GLSL_PRECISION_HIGH: return "highp ";
      case GLSL_PRECISION_MEDIUM: case GLSL_PRECISION_LOW: return "mediump ";
      default: return "";
      }
   }

   std::string decl_prec(unsigned p, BT bt) override { return bt == BT_BOOL ? "" : prec_name(p); }

   // precision of a declaration from its declared qualifier (types that take one)
   std::string var_prec(const glsl_type* t, unsigned p)
   {
      const glsl_type* bare = glsl_without_array(t);
      if (glsl_type_is_struct(bare) || glsl_type_is_interface(bare) || glsl_get_base_type(bare) == GLSL_TYPE_BOOL ||
          glsl_get_base_type(bare) == GLSL_TYPE_ATOMIC_UINT)
         return "";
      return prec_name(p);
   }

   std::string var_ref(nir_variable* v) override { return v->name; }

   // GLSL 1.10 has neither non-square matrices nor matrix-from-matrix constructors
   bool pre120() const { return !info.es && info.version < 120; }
   std::string mat_ctor(unsigned cols, unsigned rows, unsigned prec) override
   {
      if (cols != rows && pre120())
         return "";
      return nir_printer::mat_ctor(cols, rows, prec);
   }
   bool matrix_casts() override { return !pre120(); }

   // ------------------------------------------------------------------
   std::string alu_dialect(nir_alu_instr* alu) override
   {
      unsigned n = alu->def.num_components;
      switch (alu->op) {
      case nir_op_fsat:
         return "clamp(" + alu_src(alu, 0) + ", 0.0, 1.0)";
      case nir_op_ftrunc:
         if (legacy) {
            std::string x = alu_src(alu, 0);
            return "(sign(" + x + ") * floor(abs(" + x + ")))";
         }
         return "";
      case nir_op_fround_even:
         if (legacy)
            return "floor(" + alu_src(alu, 0) + " + 0.5)";
         return "";
      case nir_op_bcsel: {
         // no mix(genType, genType, genBType) before GLSL 1.30
         if (!legacy || type[alu->def.index] != BT_FLOAT)
            return "";
         bool vec_cond = alu->src[0].src.ssa->num_components > 1 && n > 1;
         bool deriv = has_implicit_derivative(alu->src[1].src.ssa) || has_implicit_derivative(alu->src[2].src.ssa);
         if (deriv) {
            std::string c = vec_cond ? alu_src(alu, 0) : src_swz(alu->src[0].src, alu->src[0].swizzle, 1, BT_BOOL, false);
            return "mix(" + alu_src(alu, 2) + ", " + alu_src(alu, 1) + ", " + vec_type(BT_FLOAT, vec_cond ? n : 1, 0) + "(" + c + "))";
         }
         if (vec_cond)
            return componentwise(alu, "", true);
         return "";
      }
      case nir_op_ishl:
      case nir_op_ishr:
         // no shifts before GLSL 1.30; NIR turns multiplies by powers of two into them
         if (legacy && nir_src_is_const(alu->src[1].src)) {
            unsigned k = nir_alu_src_as_uint(alu->src[1]);
            std::string m = fmt("%u", 1u << (k & 31));
            return "(" + alu_src(alu, 0) + (alu->op == nir_op_ishl ? " * " : " / ") + m + ")";
         }
         return "";
      default:
         return "";
      }
   }

   // ------------------------------------------------------------------
   std::string tex_expr(nir_tex_instr* tex) override
   {
      std::string samp, coord, lod, bias, cmp, ddx, ddy, offset, ms, proj;
      for (unsigned i = 0; i < tex->num_srcs; i++) {
         nir_tex_src& ts = tex->src[i];
         BT want = bt_from_alu(nir_tex_instr_src_type(tex, i));
         switch (ts.src_type) {
         case nir_tex_src_texture_deref: samp = deref_str(nir_src_as_deref(ts.src)); break;
         case nir_tex_src_sampler_deref: if (samp.empty()) samp = deref_str(nir_src_as_deref(ts.src)); break;
         case nir_tex_src_coord: coord = src_full(ts.src, want); break;
         case nir_tex_src_lod: lod = src_full(ts.src, want); break;
         case nir_tex_src_bias: bias = src_full(ts.src, want); break;
         case nir_tex_src_comparator: cmp = src_full(ts.src, want); break;
         case nir_tex_src_ddx: ddx = src_full(ts.src, want); break;
         case nir_tex_src_ddy: ddy = src_full(ts.src, want); break;
         case nir_tex_src_offset: offset = src_full(ts.src, BT_INT); break;
         case nir_tex_src_ms_index: ms = src_full(ts.src, BT_INT); break;
         case nir_tex_src_projector: proj = src_full(ts.src, BT_FLOAT); break;
         default: coord += "/*UNHANDLED texsrc*/"; break;
         }
      }
      const glsl_sampler_dim dim = tex->sampler_dim;
      if (!cmp.empty() && dim == GLSL_SAMPLER_DIM_1D && !tex->is_array) {
         // 1D shadow coordinates are (s, unused, ref)
         coord = "vec3(" + coord + ", 0.0, " + cmp + ")";
         cmp.clear();
      }
      if (!cmp.empty() && tex->op != nir_texop_tg4 && !(dim == GLSL_SAMPLER_DIM_CUBE && tex->is_array)) {
         coord = fmt("vec%u(", tex->coord_components + 1) + coord + ", " + cmp + ")";
         cmp.clear();
      }
      if (!proj.empty()) {
         unsigned n = tex->coord_components + 1 + (tex->is_shadow ? 1 : 0);
         if (tex->is_shadow && dim == GLSL_SAMPLER_DIM_1D && !tex->is_array)
            n = 4;
         coord = fmt("vec%u(", n) + coord + ", " + proj + ")";
      }
      std::string ext = cmp.empty() ? "" : ", " + cmp;
      std::string off = offset.empty() ? "" : "Offset";
      std::string offa = offset.empty() ? "" : ", " + offset;
      std::string projs = proj.empty() ? "" : "Proj";
      // texelFetch on buffer, rectangle and multisample textures has no lod
      if (dim == GLSL_SAMPLER_DIM_BUF || dim == GLSL_SAMPLER_DIM_RECT || dim == GLSL_SAMPLER_DIM_MS)
         if (tex->op == nir_texop_txf || tex->op == nir_texop_txs)
            lod.clear();

      std::string base = "texture";
      if (legacy) {
         static const char* dims[] = { "1D", "2D", "3D", "Cube", "Rect", "Buffer", "2D", "2D", "2D" };
         base = std::string(tex->is_shadow ? "shadow" : "texture") + (dim < ARRAY_SIZE(dims) ? dims[dim] : "2D");
         if (tex->is_array)
            base += "Array";
      }
      switch (tex->op) {
      case nir_texop_tex:
         return base + projs + off + "(" + samp + ", " + coord + ext + offa + ")";
      case nir_texop_txb:
         return base + projs + off + "(" + samp + ", " + coord + ext + offa + ", " + bias + ")";
      case nir_texop_txl:
         return base + projs + "Lod" + off + "(" + samp + ", " + coord + ext + ", " + lod + offa + ")";
      case nir_texop_txd:
         return base + projs + "Grad" + off + (legacy ? "ARB" : "") + "(" + samp + ", " + coord + ext + ", " + ddx + ", " +
                ddy + offa + ")";
      case nir_texop_txf:
         return "texelFetch" + off + "(" + samp + ", " + coord + (lod.empty() ? "" : ", " + lod) + offa + ")";
      case nir_texop_txf_ms:
         return "texelFetch(" + samp + ", " + coord + ", " + ms + ")";
      case nir_texop_txs:
         return "textureSize(" + samp + (lod.empty() ? "" : ", " + lod) + ")";
      case nir_texop_tg4:
         return "textureGather" + off + "(" + samp + ", " + coord + ext + offa +
                (tex->is_shadow || tex->component == 0 ? "" : fmt(", %u", tex->component)) + ")";
      case nir_texop_query_levels:
         return "textureQueryLevels(" + samp + ")";
      case nir_texop_texture_samples:
         return "textureSamples(" + samp + ")";
      case nir_texop_lod:
         return "textureQueryLod(" + samp + ", " + coord + ")";
      default:
         return "/*UNHANDLED texop*/texture(" + samp + ", " + coord + ")";
      }
   }

   // ------------------------------------------------------------------
   std::string image_coord(nir_intrinsic_instr* in, nir_deref_instr* d)
   {
      unsigned dims = glsl_get_sampler_coordinate_components(glsl_without_array(d->type));
      uint8_t id[4] = {0, 1, 2, 3};
      std::string c = src_swz(in->src[1], id, dims, BT_INT, false);
      if (glsl_get_sampler_dim(glsl_without_array(d->type)) == GLSL_SAMPLER_DIM_MS)
         c += ", " + src_full(in->src[2], BT_INT);
      return c;
   }

   bool intrinsic_dialect(nir_intrinsic_instr* in) override
   {
      switch (in->intrinsic) {
      case nir_intrinsic_barrier: {
         mesa_scope es = nir_intrinsic_execution_scope(in);
         mesa_scope ms = nir_intrinsic_memory_scope(in);
         nir_variable_mode modes = (nir_variable_mode)nir_intrinsic_memory_modes(in);
         if (es == SCOPE_WORKGROUP)
            line("barrier();");
         else if (es == SCOPE_SUBGROUP)
            line("subgroupBarrier();");
         else if (ms == SCOPE_SUBGROUP)
            line(modes == nir_var_mem_shared ? "subgroupMemoryBarrierShared();" :
                 modes == nir_var_image ? "subgroupMemoryBarrierImage();" :
                 (modes & ~nir_var_mem_global) == nir_var_mem_ssbo ? "subgroupMemoryBarrierBuffer();" : "subgroupMemoryBarrier();");
         else if (modes == nir_var_mem_shared)
            line("memoryBarrierShared();");
         else if (ms == SCOPE_WORKGROUP)
            line("groupMemoryBarrier();");
         else if (modes == nir_var_mem_ssbo)
            line("memoryBarrierBuffer();");
         else if (modes == nir_var_image)
            line("memoryBarrierImage();");
         else if (ms != SCOPE_NONE)
            line("memoryBarrier();");
         return true;
      }
      case nir_intrinsic_deref_atomic:
      case nir_intrinsic_deref_atomic_swap: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         BT bt = bt_from_glsl(d->type);
         std::string e = std::string(atomic_name(nir_intrinsic_atomic_op(in))) + "(" + deref_str(d) + ", " + src_full(in->src[1], bt);
         if (in->intrinsic == nir_intrinsic_deref_atomic_swap)
            e += ", " + src_full(in->src[2], bt);
         define_side_effect(&in->def, e + ")");
         return true;
      }
      case nir_intrinsic_image_deref_load: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         define(&in->def, "imageLoad(" + deref_str(d) + ", " + image_coord(in, d) + ")", true, false);
         return true;
      }
      case nir_intrinsic_image_deref_store: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         line("imageStore(" + deref_str(d) + ", " + image_coord(in, d) + ", " +
              src_full(in->src[3], bt_from_alu(nir_intrinsic_src_type(in))) + ");");
         return true;
      }
      case nir_intrinsic_image_deref_atomic:
      case nir_intrinsic_image_deref_atomic_swap: {
         nir_deref_instr* d = nir_src_as_deref(in->src[0]);
         BT bt = type[in->def.index];
         std::string f = std::string("imageAtomic") + (atomic_name(nir_intrinsic_atomic_op(in)) + 6);
         std::string e = f + "(" + deref_str(d) + ", " + image_coord(in, d) + ", " + src_full(in->src[3], bt);
         if (in->intrinsic == nir_intrinsic_image_deref_atomic_swap)
            e += ", " + src_full(in->src[4], bt);
         define_side_effect(&in->def, e + ")");
         return true;
      }
      case nir_intrinsic_image_deref_size:
         define(&in->def, "imageSize(" + deref_str(nir_src_as_deref(in->src[0])) + ")", false, false);
         return true;
      case nir_intrinsic_image_deref_samples:
         define(&in->def, "imageSamples(" + deref_str(nir_src_as_deref(in->src[0])) + ")", false, false);
         return true;
      case nir_intrinsic_deref_buffer_array_length:
         define(&in->def, deref_str(nir_src_as_deref(in->src[0])) + ".length()", false, false);
         return true;
      case nir_intrinsic_atomic_counter_read_deref:
      case nir_intrinsic_atomic_counter_inc_deref:
      case nir_intrinsic_atomic_counter_pre_dec_deref:
      case nir_intrinsic_atomic_counter_post_dec_deref: {
         std::string c = deref_str(nir_src_as_deref(in->src[0]));
         define_side_effect(&in->def, in->intrinsic == nir_intrinsic_atomic_counter_read_deref ? "atomicCounter(" + c + ")" :
                                      in->intrinsic == nir_intrinsic_atomic_counter_inc_deref ? "atomicCounterIncrement(" + c + ")" :
                                      in->intrinsic == nir_intrinsic_atomic_counter_pre_dec_deref ? "atomicCounterDecrement(" + c + ")" :
                                      "(atomicCounterDecrement(" + c + ") + 1u)");
         return true;
      }
      default:
         return false;
      }
   }

   static const char* reduction_name(nir_op op)
   {
      switch (op) {
      case nir_op_iadd: case nir_op_fadd: return "Add";
      case nir_op_imul: case nir_op_fmul: return "Mul";
      case nir_op_imin: case nir_op_umin: case nir_op_fmin: return "Min";
      case nir_op_imax: case nir_op_umax: case nir_op_fmax: return "Max";
      case nir_op_iand: return "And";
      case nir_op_ior: return "Or";
      case nir_op_ixor: return "Xor";
      default: return "/*UNHANDLED reduction*/Add";
      }
   }

   std::string builtin_ref(const builtin_var& b, nir_def*) override { return b.name; }

   // KHR_shader_subgroup builtin for a subgroup intrinsic
   std::string subgroup_expr(nir_intrinsic_instr* in) override
   {
      BT vt = type[in->def.index];
      auto val = [&]() { return src_full(in->src[0], in->intrinsic == nir_intrinsic_vote_feq ? BT_FLOAT : BT_UNK); };
      auto arg1 = [&]() { return src_full(in->src[1], BT_UINT); };
      switch (in->intrinsic) {
      case nir_intrinsic_elect: return "subgroupElect()";
      case nir_intrinsic_read_first_invocation: return "subgroupBroadcastFirst(" + src_full(in->src[0], vt) + ")";
      case nir_intrinsic_read_invocation:
         // subgroupBroadcast needs a constant lane
         return std::string(nir_src_is_const(in->src[1]) ? "subgroupBroadcast(" : "subgroupShuffle(") +
                src_full(in->src[0], vt) + ", " + arg1() + ")";
      case nir_intrinsic_reduce:
      case nir_intrinsic_inclusive_scan:
      case nir_intrinsic_exclusive_scan: {
         const char* r = reduction_name((nir_op)nir_intrinsic_reduction_op(in));
         std::string v = src_full(in->src[0], vt);
         if (in->intrinsic == nir_intrinsic_reduce && nir_intrinsic_cluster_size(in))
            return std::string("subgroupClustered") + r + "(" + v + fmt(", %uu)", nir_intrinsic_cluster_size(in));
         const char* kind = in->intrinsic == nir_intrinsic_inclusive_scan ? "Inclusive" :
                            in->intrinsic == nir_intrinsic_exclusive_scan ? "Exclusive" : "";
         return std::string("subgroup") + kind + r + "(" + v + ")";
      }
      case nir_intrinsic_vote_any: return "subgroupAny(" + src_full(in->src[0], BT_BOOL) + ")";
      case nir_intrinsic_vote_all: return "subgroupAll(" + src_full(in->src[0], BT_BOOL) + ")";
      case nir_intrinsic_vote_ieq: case nir_intrinsic_vote_feq: return "subgroupAllEqual(" + val() + ")";
      case nir_intrinsic_ballot: return "subgroupBallot(" + src_full(in->src[0], BT_BOOL) + ")";
      case nir_intrinsic_inverse_ballot: return "subgroupInverseBallot(" + src_full(in->src[0], BT_UINT) + ")";
      case nir_intrinsic_ballot_bitfield_extract:
         return "subgroupBallotBitExtract(" + src_full(in->src[0], BT_UINT) + ", " + arg1() + ")";
      case nir_intrinsic_ballot_bit_count_reduce: return "subgroupBallotBitCount(" + src_full(in->src[0], BT_UINT) + ")";
      case nir_intrinsic_ballot_bit_count_inclusive: return "subgroupBallotInclusiveBitCount(" + src_full(in->src[0], BT_UINT) + ")";
      case nir_intrinsic_ballot_bit_count_exclusive: return "subgroupBallotExclusiveBitCount(" + src_full(in->src[0], BT_UINT) + ")";
      case nir_intrinsic_ballot_find_lsb: return "subgroupBallotFindLSB(" + src_full(in->src[0], BT_UINT) + ")";
      case nir_intrinsic_ballot_find_msb: return "subgroupBallotFindMSB(" + src_full(in->src[0], BT_UINT) + ")";
      case nir_intrinsic_shuffle: return "subgroupShuffle(" + src_full(in->src[0], vt) + ", " + arg1() + ")";
      case nir_intrinsic_shuffle_xor: return "subgroupShuffleXor(" + src_full(in->src[0], vt) + ", " + arg1() + ")";
      case nir_intrinsic_shuffle_up: return "subgroupShuffleUp(" + src_full(in->src[0], vt) + ", " + arg1() + ")";
      case nir_intrinsic_shuffle_down: return "subgroupShuffleDown(" + src_full(in->src[0], vt) + ", " + arg1() + ")";
      case nir_intrinsic_quad_broadcast: return "subgroupQuadBroadcast(" + src_full(in->src[0], vt) + ", " + arg1() + ")";
      case nir_intrinsic_quad_swap_horizontal: return "subgroupQuadSwapHorizontal(" + src_full(in->src[0], vt) + ")";
      case nir_intrinsic_quad_swap_vertical: return "subgroupQuadSwapVertical(" + src_full(in->src[0], vt) + ")";
      case nir_intrinsic_quad_swap_diagonal: return "subgroupQuadSwapDiagonal(" + src_full(in->src[0], vt) + ")";
      default: return "";
      }
   }

   // ------------------------------------------------------------------
   // declarations

   void emit_struct(const glsl_type* t)
   {
      t = glsl_without_array(t);
      if (!glsl_type_is_struct(t) || emitted_structs.count(t) || !strncmp(glsl_get_type_name(t), "gl_", 3))
         return;
      for (unsigned i = 0; i < glsl_get_length(t); i++)
         emit_struct(glsl_get_struct_field(t, i));
      emitted_structs.insert(t);
      globals += fmt("struct %s {\n", glsl_get_type_name(t));
      for (unsigned i = 0; i < glsl_get_length(t); i++) {
         const glsl_struct_field* f = glsl_get_struct_field_data(t, i);
         unsigned p = f->precision;
         if (info.es && p == GLSL_PRECISION_NONE)
            p = GLSL_PRECISION_HIGH;
         globals += "  " + var_prec(f->type, p) + type_decl(f->type, p, f->name) + ";\n";
      }
      globals += "};\n";
   }

   std::string location_layout(nir_variable* v)
   {
      if (!v->data.explicit_location || (legacy && !info.es) || (!info.es && info.version < 330 && v->data.mode != nir_var_uniform))
         return "";
      int loc = v->data.location;
      if (v->data.mode == nir_var_uniform)
         return fmt("location = %d", loc);
      if (s->info.stage == MESA_SHADER_VERTEX && v->data.mode == nir_var_shader_in)
         loc -= VERT_ATTRIB_GENERIC0;
      else if (s->info.stage == MESA_SHADER_FRAGMENT && v->data.mode == nir_var_shader_out)
         loc -= FRAG_RESULT_DATA0;
      else
         loc -= v->data.patch ? VARYING_SLOT_PATCH0 : VARYING_SLOT_VAR0;
      if (loc < 0)
         return "";
      std::string r = fmt("location = %d", loc);
      if (s->info.stage == MESA_SHADER_FRAGMENT && v->data.mode == nir_var_shader_out && v->data.index)
         r += fmt(", index = %d", v->data.index);
      return r;
   }

   static std::string layout(const std::vector<std::string>& q)
   {
      if (q.empty())
         return "";
      std::string r = "layout(";
      for (size_t i = 0; i < q.size(); i++)
         r += (i ? ", " : "") + q[i];
      return r + ") ";
   }

   static std::string access_qualifiers(unsigned access)
   {
      std::string q;
      if (access & ACCESS_COHERENT) q += "coherent ";
      if (access & ACCESS_VOLATILE) q += "volatile ";
      if (access & ACCESS_RESTRICT) q += "restrict ";
      if (access & ACCESS_NON_WRITEABLE) q += "readonly ";
      if (access & ACCESS_NON_READABLE) q += "writeonly ";
      return q;
   }

   static std::string field_memory_qualifiers(const glsl_struct_field* f)
   {
      return access_qualifiers((f->memory_coherent ? ACCESS_COHERENT : 0) | (f->memory_volatile ? ACCESS_VOLATILE : 0) |
                               (f->memory_restrict ? ACCESS_RESTRICT : 0) | (f->memory_read_only ? ACCESS_NON_WRITEABLE : 0) |
                               (f->memory_write_only ? ACCESS_NON_READABLE : 0));
   }

   std::string interp_qualifiers(unsigned interp, bool centroid, bool sample)
   {
      std::string q;
      if (centroid) q += "centroid ";
      if (sample) q += "sample ";
      if (interp == INTERP_MODE_FLAT) q += "flat ";
      else if (interp == INTERP_MODE_NOPERSPECTIVE) q += "noperspective ";
      else if (interp == INTERP_MODE_SMOOTH) q += "smooth ";
      return q;
   }

   const char* in_out_keyword(nir_variable* v)
   {
      if (v->data.mode == nir_var_shader_in)
         return legacy ? (s->info.stage == MESA_SHADER_VERTEX ? "attribute " : "varying ") : "in ";
      if (v->data.fb_fetch_output)
         return "inout ";
      return legacy ? "varying " : "out ";
   }

   void emit_block(nir_variable* v, std::set<const glsl_type*>& done)
   {
      const glsl_type* it = v->interface_type;
      bool named = glsl_without_array(v->type) == it;
      if (done.count(it))
         return;
      done.insert(it);
      std::vector<std::string> q;
      const char* kw;
      bool buffer = v->data.mode == nir_var_mem_ubo || v->data.mode == nir_var_mem_ssbo;
      if (buffer) {
         switch (glsl_get_ifc_packing(it)) {
         case GLSL_INTERFACE_PACKING_STD140: q.push_back("std140"); break;
         case GLSL_INTERFACE_PACKING_STD430: q.push_back("std430"); break;
         case GLSL_INTERFACE_PACKING_PACKED: q.push_back("packed"); break;
         default: q.push_back("shared"); break;
         }
         if (glsl_matrix_type_is_row_major(it))
            q.push_back("row_major");
         if (v->data.explicit_binding)
            q.push_back(fmt("binding = %u", v->data.binding));
         kw = v->data.mode == nir_var_mem_ubo ? "uniform" : "buffer";
      } else {
         std::string l = location_layout(v);
         if (!l.empty())
            q.push_back(l);
         kw = v->data.mode == nir_var_shader_in ? "in" : "out";
      }
      std::string mem;
      if (v->data.mode == nir_var_mem_ssbo && named)
         mem = access_qualifiers(v->data.access & ~(ACCESS_NON_WRITEABLE | ACCESS_NON_READABLE));
      globals += layout(q) + mem + kw + " " + glsl_get_type_name(it) + " {\n";
      for (unsigned i = 0; i < glsl_get_length(it); i++) {
         const glsl_struct_field* f = glsl_get_struct_field_data(it, i);
         std::vector<std::string> fq;
         if (buffer && f->offset >= 0)
            fq.push_back(fmt("offset = %d", f->offset));
         if (buffer && glsl_type_is_matrix(glsl_without_array(f->type))) {
            if (f->matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR && !glsl_matrix_type_is_row_major(it))
               fq.push_back("row_major");
            else if (f->matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR && glsl_matrix_type_is_row_major(it))
               fq.push_back("column_major");
         }
         std::string fl = buffer ? field_memory_qualifiers(f) : interp_qualifiers(f->interpolation, f->centroid, f->sample);
         unsigned p = f->precision;
         if (info.es && p == GLSL_PRECISION_NONE && !buffer)
            p = v->data.precision;
         globals += "  " + layout(fq) + fl + var_prec(f->type, p) + type_decl(f->type, p, f->name) + ";\n";
      }
      const glsl_type* t = v->type;
      if (named)
         globals += "} " + std::string(v->name) + array_dims(t) + ";\n";
      else
         globals += "};\n";
   }

   void emit_globals()
   {
      if (info.had_version)
         globals += fmt("#version %u%s\n", info.version, info.es ? " es" : "");
      for (const std::string& e : info.extensions)
         globals += "#extension " + e + " : enable\n";
      if (info.es && s->info.stage == MESA_SHADER_FRAGMENT)
         globals += "precision highp float;\nprecision highp int;\n";
      if (s->info.stage == MESA_SHADER_FRAGMENT && info.early_fragment_tests)
         globals += "layout(early_fragment_tests) in;\n";
      if (s->info.stage == MESA_SHADER_COMPUTE && info.local_size_specified) {
         globals += fmt("layout(local_size_x = %u", info.local_size[0]);
         if (info.local_size[1] != 1) globals += fmt(", local_size_y = %u", info.local_size[1]);
         if (info.local_size[2] != 1) globals += fmt(", local_size_z = %u", info.local_size[2]);
         globals += ") in;\n";
      }

      nir_foreach_variable_in_shader(v, s) {
         emit_struct(v->type);
         if (v->interface_type)
            for (unsigned i = 0; i < glsl_get_length(v->interface_type); i++)
               emit_struct(glsl_get_struct_field(v->interface_type, i));
      }
      nir_foreach_function_temp_variable(v, impl)
         emit_struct(v->type);

      std::set<const glsl_type*> done_blocks;
      nir_foreach_variable_in_shader(v, s) {
         if (v->name && !strncmp(v->name, "gl_", 3)) {
            // keep invariance of builtin outputs
            if (v->data.mode == nir_var_shader_out && v->data.explicit_invariant)
               globals += std::string("invariant ") + v->name + ";\n";
            continue;
         }
         switch (v->data.mode) {
         case nir_var_shader_in:
         case nir_var_shader_out: {
            if (v->interface_type) {
               emit_block(v, done_blocks);
               break;
            }
            std::vector<std::string> q;
            std::string l = location_layout(v);
            if (!l.empty()) q.push_back(l);
            std::string inv = v->data.explicit_invariant ? "invariant " : "";
            globals += layout(q) + inv + interp_qualifiers(v->data.interpolation, v->data.centroid, v->data.sample) +
                       in_out_keyword(v) + var_prec(v->type, v->data.precision) +
                       type_decl(v->type, v->data.precision, v->name) + ";\n";
            break;
         }
         case nir_var_uniform:
         case nir_var_image: {
            const glsl_type* bare = glsl_without_array(v->type);
            std::vector<std::string> q;
            std::string mem;
            if (glsl_type_is_image(bare)) {
               if (const char* f = glslopt_image_format_name((enum pipe_format)v->data.image.format))
                  q.push_back(f);
               mem = access_qualifiers(v->data.access);
            }
            if (glsl_contains_atomic(v->type)) {
               q.push_back(fmt("binding = %u", v->data.binding));
               q.push_back(fmt("offset = %u", v->data.offset));
            } else if (v->data.explicit_binding) {
               q.push_back(fmt("binding = %u", v->data.binding));
            } else {
               std::string l = location_layout(v);
               if (!l.empty() && info.version >= 300) q.push_back(l);
            }
            globals += layout(q) + mem + "uniform " + var_prec(v->type, v->data.precision) +
                       type_decl(v->type, v->data.precision, v->name) + ";\n";
            break;
         }
         case nir_var_mem_ubo:
         case nir_var_mem_ssbo:
            if (v->interface_type)
               emit_block(v, done_blocks);
            break;
         case nir_var_mem_shared:
            globals += "shared " + var_prec(v->type, v->data.precision) + type_decl(v->type, v->data.precision, v->name) + ";\n";
            break;
         default:
            break;
         }
      }
   }

   std::string assemble(const std::string& decls, const std::string& stmts) override
   {
      emit_globals();
      return globals + "void main ()\n{\n" + decls + stmts + "}\n";
   }
};

} // namespace

std::string glslopt_print_glsl(nir_shader* nir, const glslopt_source_info& info)
{
   glsl_printer p(nir, info);
   return p.print();
}
