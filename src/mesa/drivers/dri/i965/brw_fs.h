/*
 * Copyright © 2010 Intel Corporation
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
 *    Eric Anholt <eric@anholt.net>
 *
 */

#pragma once

#include "brw_shader.h"

extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
#include "brw_shader.h"
#include "intel_asm_annotation.h"
}
#include "glsl/glsl_types.h"
#include "glsl/ir.h"

#define MAX_SAMPLER_MESSAGE_SIZE 11

struct bblock_t;
namespace {
   struct acp_entry;
}

namespace brw {
   class fs_live_variables;
}

class fs_reg : public backend_reg {
public:
   DECLARE_RALLOC_CXX_OPERATORS(fs_reg)

   void init();

   fs_reg();
   fs_reg(float f);
   fs_reg(int32_t i);
   fs_reg(uint32_t u);
   fs_reg(struct brw_reg fixed_hw_reg);
   fs_reg(enum register_file file, int reg);
   fs_reg(enum register_file file, int reg, enum brw_reg_type type);
   fs_reg(class fs_visitor *v, const struct glsl_type *type);

   bool equals(const fs_reg &r) const;
   bool is_valid_3src() const;
   bool is_contiguous() const;

   fs_reg &apply_stride(unsigned stride);
   /** Smear a channel of the reg to all channels. */
   fs_reg &set_smear(unsigned subreg);

   /**
    * Offset in bytes from the start of the register.  Values up to a
    * backend_reg::reg_offset unit are valid.
    */
   int subreg_offset;

   fs_reg *reladdr;

   /** Register region horizontal stride */
   uint8_t stride;
};

static inline fs_reg
retype(fs_reg reg, enum brw_reg_type type)
{
   reg.fixed_hw_reg.type = reg.type = type;
   return reg;
}

static inline fs_reg
offset(fs_reg reg, unsigned delta)
{
   assert(delta == 0 || (reg.file != HW_REG && reg.file != IMM));
   reg.reg_offset += delta;
   return reg;
}

static inline fs_reg
byte_offset(fs_reg reg, unsigned delta)
{
   assert(delta == 0 || (reg.file != HW_REG && reg.file != IMM));
   reg.subreg_offset += delta;
   return reg;
}

/**
 * Get either of the 8-component halves of a 16-component register.
 *
 * Note: this also works if \c reg represents a SIMD16 pair of registers.
 */
static inline fs_reg
half(const fs_reg &reg, unsigned idx)
{
   assert(idx < 2);
   assert(idx == 0 || (reg.file != HW_REG && reg.file != IMM));
   return byte_offset(reg, 8 * idx * reg.stride * type_sz(reg.type));
}

static const fs_reg reg_undef;
static const fs_reg reg_null_f(retype(brw_null_reg(), BRW_REGISTER_TYPE_F));
static const fs_reg reg_null_d(retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
static const fs_reg reg_null_ud(retype(brw_null_reg(), BRW_REGISTER_TYPE_UD));

class ip_record : public exec_node {
public:
   DECLARE_RALLOC_CXX_OPERATORS(ip_record)

   ip_record(int ip)
   {
      this->ip = ip;
   }

   int ip;
};

class fs_inst : public backend_instruction {
   fs_inst &operator=(const fs_inst &);

public:
   DECLARE_RALLOC_CXX_OPERATORS(fs_inst)

   void init(enum opcode opcode, const fs_reg &dst, fs_reg *src, int sources);

   fs_inst(enum opcode opcode = BRW_OPCODE_NOP, const fs_reg &dst = reg_undef);
   fs_inst(enum opcode opcode, const fs_reg &dst, const fs_reg &src0);
   fs_inst(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
           const fs_reg &src1);
   fs_inst(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
           const fs_reg &src1, const fs_reg &src2);
   fs_inst(enum opcode opcode, const fs_reg &dst, fs_reg src[], int sources);
   fs_inst(const fs_inst &that);

   void resize_sources(uint8_t num_sources);

   bool equals(fs_inst *inst) const;
   bool overwrites_reg(const fs_reg &reg) const;
   bool is_send_from_grf() const;
   bool is_partial_write() const;
   int regs_read(fs_visitor *v, int arg) const;
   bool can_do_source_mods(struct brw_context *brw);

   bool reads_flag() const;
   bool writes_flag() const;

   fs_reg dst;
   fs_reg *src;

   uint8_t sources; /**< Number of fs_reg sources. */

   /* Chooses which flag subregister (f0.0 or f0.1) is used for conditional
    * mod and predication.
    */
   uint8_t flag_subreg;

   uint8_t regs_written; /**< Number of vgrfs written by a SEND message, or 1 */
   bool eot:1;
   bool header_present:1;
   bool shadow_compare:1;
   bool force_uncompressed:1;
   bool force_sechalf:1;
   bool pi_noperspective:1;   /**< Pixel interpolator noperspective flag */
};

/**
 * The fragment shader front-end.
 *
 * Translates either GLSL IR or Mesa IR (for ARB_fragment_program) into FS IR.
 */
class fs_visitor : public backend_visitor
{
public:

   fs_visitor(struct brw_context *brw,
              void *mem_ctx,
              const struct brw_wm_prog_key *key,
              struct brw_wm_prog_data *prog_data,
              struct gl_shader_program *shader_prog,
              struct gl_fragment_program *fp,
              unsigned dispatch_width);
   ~fs_visitor();

   fs_reg *variable_storage(ir_variable *var);
   int virtual_grf_alloc(int size);
   void import_uniforms(fs_visitor *v);

   void visit(ir_variable *ir);
   void visit(ir_assignment *ir);
   void visit(ir_dereference_variable *ir);
   void visit(ir_dereference_record *ir);
   void visit(ir_dereference_array *ir);
   void visit(ir_expression *ir);
   void visit(ir_texture *ir);
   void visit(ir_if *ir);
   void visit(ir_constant *ir);
   void visit(ir_swizzle *ir);
   void visit(ir_return *ir);
   void visit(ir_loop *ir);
   void visit(ir_loop_jump *ir);
   void visit(ir_discard *ir);
   void visit(ir_call *ir);
   void visit(ir_function *ir);
   void visit(ir_function_signature *ir);
   void visit(ir_emit_vertex *);
   void visit(ir_end_primitive *);

   uint32_t gather_channel(ir_texture *ir, uint32_t sampler);
   void swizzle_result(ir_texture *ir, fs_reg orig_val, uint32_t sampler);

   fs_inst *emit(fs_inst *inst);
   void emit(exec_list list);

   fs_inst *emit(enum opcode opcode);
   fs_inst *emit(enum opcode opcode, const fs_reg &dst);
   fs_inst *emit(enum opcode opcode, const fs_reg &dst, const fs_reg &src0);
   fs_inst *emit(enum opcode opcode, const fs_reg &dst, const fs_reg &src0,
                 const fs_reg &src1);
   fs_inst *emit(enum opcode opcode, const fs_reg &dst,
                 const fs_reg &src0, const fs_reg &src1, const fs_reg &src2);
   fs_inst *emit(enum opcode opcode, const fs_reg &dst,
                 fs_reg src[], int sources);

   fs_inst *MOV(const fs_reg &dst, const fs_reg &src);
   fs_inst *NOT(const fs_reg &dst, const fs_reg &src);
   fs_inst *RNDD(const fs_reg &dst, const fs_reg &src);
   fs_inst *RNDE(const fs_reg &dst, const fs_reg &src);
   fs_inst *RNDZ(const fs_reg &dst, const fs_reg &src);
   fs_inst *FRC(const fs_reg &dst, const fs_reg &src);
   fs_inst *ADD(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *MUL(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *MACH(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *MAC(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *SHL(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *SHR(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *ASR(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *AND(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *OR(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *XOR(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *IF(enum brw_predicate predicate);
   fs_inst *IF(const fs_reg &src0, const fs_reg &src1,
               enum brw_conditional_mod condition);
   fs_inst *CMP(fs_reg dst, fs_reg src0, fs_reg src1,
                enum brw_conditional_mod condition);
   fs_inst *LRP(const fs_reg &dst, const fs_reg &a, const fs_reg &y,
                const fs_reg &x);
   fs_inst *DEP_RESOLVE_MOV(int grf);
   fs_inst *BFREV(const fs_reg &dst, const fs_reg &value);
   fs_inst *BFE(const fs_reg &dst, const fs_reg &bits, const fs_reg &offset,
                const fs_reg &value);
   fs_inst *BFI1(const fs_reg &dst, const fs_reg &bits, const fs_reg &offset);
   fs_inst *BFI2(const fs_reg &dst, const fs_reg &bfi1_dst,
                 const fs_reg &insert, const fs_reg &base);
   fs_inst *FBH(const fs_reg &dst, const fs_reg &value);
   fs_inst *FBL(const fs_reg &dst, const fs_reg &value);
   fs_inst *CBIT(const fs_reg &dst, const fs_reg &value);
   fs_inst *MAD(const fs_reg &dst, const fs_reg &c, const fs_reg &b,
                const fs_reg &a);
   fs_inst *ADDC(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *SUBB(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);
   fs_inst *SEL(const fs_reg &dst, const fs_reg &src0, const fs_reg &src1);

   int type_size(const struct glsl_type *type);
   fs_inst *get_instruction_generating_reg(fs_inst *start,
					   fs_inst *end,
					   const fs_reg &reg);

   fs_inst *LOAD_PAYLOAD(const fs_reg &dst, fs_reg *src, int sources);

   exec_list VARYING_PULL_CONSTANT_LOAD(const fs_reg &dst,
                                        const fs_reg &surf_index,
                                        const fs_reg &varying_offset,
                                        uint32_t const_offset);

   bool run();
   void assign_binding_table_offsets();
   void setup_payload_gen4();
   void setup_payload_gen6();
   void assign_curb_setup();
   void calculate_urb_setup();
   void assign_urb_setup();
   bool assign_regs(bool allow_spilling);
   void assign_regs_trivial();
   void get_used_mrfs(bool *mrf_used);
   void setup_payload_interference(struct ra_graph *g, int payload_reg_count,
                                   int first_payload_node);
   void setup_mrf_hack_interference(struct ra_graph *g,
                                    int first_mrf_hack_node);
   int choose_spill_reg(struct ra_graph *g);
   void spill_reg(int spill_reg);
   void split_virtual_grfs();
   void compact_virtual_grfs();
   void move_uniform_array_access_to_pull_constants();
   void assign_constant_locations();
   void demote_pull_constants();
   void invalidate_live_intervals();
   void calculate_live_intervals();
   void calculate_register_pressure();
   bool opt_algebraic();
   bool opt_cse();
   bool opt_cse_local(bblock_t *block);
   bool opt_copy_propagate();
   bool try_copy_propagate(fs_inst *inst, int arg, acp_entry *entry);
   bool opt_copy_propagate_local(void *mem_ctx, bblock_t *block,
                                 exec_list *acp);
   void opt_drop_redundant_mov_to_flags();
   bool opt_register_renaming();
   bool register_coalesce();
   bool compute_to_mrf();
   bool dead_code_eliminate();
   bool remove_duplicate_mrf_writes();
   bool virtual_grf_interferes(int a, int b);
   void schedule_instructions(instruction_scheduler_mode mode);
   void insert_gen4_send_dependency_workarounds();
   void insert_gen4_pre_send_dependency_workarounds(fs_inst *inst);
   void insert_gen4_post_send_dependency_workarounds(fs_inst *inst);
   void vfail(const char *msg, va_list args);
   void fail(const char *msg, ...);
   void no16(const char *msg, ...);
   void lower_uniform_pull_constant_loads();
   bool lower_load_payload();

   void try_rep_send();

   void push_force_uncompressed();
   void pop_force_uncompressed();

   void emit_dummy_fs();
   fs_reg *emit_fragcoord_interpolation(ir_variable *ir);
   fs_inst *emit_linterp(const fs_reg &attr, const fs_reg &interp,
                         glsl_interp_qualifier interpolation_mode,
                         bool is_centroid, bool is_sample);
   fs_reg *emit_frontfacing_interpolation(ir_variable *ir);
   fs_reg *emit_samplepos_setup(ir_variable *ir);
   fs_reg *emit_sampleid_setup(ir_variable *ir);
   fs_reg *emit_general_interpolation(ir_variable *ir);
   void emit_interpolation_setup_gen4();
   void emit_interpolation_setup_gen6();
   void compute_sample_position(fs_reg dst, fs_reg int_sample_pos);
   fs_reg rescale_texcoord(ir_texture *ir, fs_reg coordinate,
                           bool is_rect, uint32_t sampler, int texunit);
   fs_inst *emit_texture_gen4(ir_texture *ir, fs_reg dst, fs_reg coordinate,
                              fs_reg shadow_comp, fs_reg lod, fs_reg lod2,
                              uint32_t sampler);
   fs_inst *emit_texture_gen5(ir_texture *ir, fs_reg dst, fs_reg coordinate,
                              fs_reg shadow_comp, fs_reg lod, fs_reg lod2,
                              fs_reg sample_index, uint32_t sampler);
   fs_inst *emit_texture_gen7(ir_texture *ir, fs_reg dst, fs_reg coordinate,
                              fs_reg shadow_comp, fs_reg lod, fs_reg lod2,
                              fs_reg sample_index, fs_reg mcs, fs_reg sampler);
   fs_reg emit_mcs_fetch(ir_texture *ir, fs_reg coordinate, fs_reg sampler);
   void emit_gen6_gather_wa(uint8_t wa, fs_reg dst);
   fs_reg fix_math_operand(fs_reg src);
   fs_inst *emit_math(enum opcode op, fs_reg dst, fs_reg src0);
   fs_inst *emit_math(enum opcode op, fs_reg dst, fs_reg src0, fs_reg src1);
   void emit_lrp(const fs_reg &dst, const fs_reg &x, const fs_reg &y,
                 const fs_reg &a);
   void emit_minmax(enum brw_conditional_mod conditionalmod, const fs_reg &dst,
                    const fs_reg &src0, const fs_reg &src1);
   bool try_emit_saturate(ir_expression *ir);
   bool try_emit_mad(ir_expression *ir);
   void try_replace_with_sel();
   bool opt_peephole_sel();
   bool opt_peephole_predicated_break();
   bool opt_saturate_propagation();
   void emit_bool_to_cond_code(ir_rvalue *condition);
   void emit_if_gen6(ir_if *ir);
   void emit_unspill(fs_inst *inst, fs_reg reg, uint32_t spill_offset,
                     int count);

   void emit_fragment_program_code();
   void setup_fp_regs();
   fs_reg get_fp_src_reg(const prog_src_register *src);
   fs_reg get_fp_dst_reg(const prog_dst_register *dst);
   void emit_fp_alu1(enum opcode opcode,
                     const struct prog_instruction *fpi,
                     fs_reg dst, fs_reg src);
   void emit_fp_alu2(enum opcode opcode,
                     const struct prog_instruction *fpi,
                     fs_reg dst, fs_reg src0, fs_reg src1);
   void emit_fp_scalar_write(const struct prog_instruction *fpi,
                             fs_reg dst, fs_reg src);
   void emit_fp_scalar_math(enum opcode opcode,
                            const struct prog_instruction *fpi,
                            fs_reg dst, fs_reg src);

   void emit_fp_minmax(const struct prog_instruction *fpi,
                       fs_reg dst, fs_reg src0, fs_reg src1);

   void emit_fp_sop(enum brw_conditional_mod conditional_mod,
                    const struct prog_instruction *fpi,
                    fs_reg dst, fs_reg src0, fs_reg src1, fs_reg one);

   void emit_color_write(int target, int index, int first_color_mrf);
   void emit_alpha_test();
   void emit_fb_writes();

   void emit_shader_time_begin();
   void emit_shader_time_end();
   void emit_shader_time_write(enum shader_time_shader_type type,
                               fs_reg value);

   void emit_untyped_atomic(unsigned atomic_op, unsigned surf_index,
                            fs_reg dst, fs_reg offset, fs_reg src0,
                            fs_reg src1);

   void emit_untyped_surface_read(unsigned surf_index, fs_reg dst,
                                  fs_reg offset);

   void emit_interpolate_expression(ir_expression *ir);

   bool try_rewrite_rhs_to_dst(ir_assignment *ir,
			       fs_reg dst,
			       fs_reg src,
			       fs_inst *pre_rhs_inst,
			       fs_inst *last_rhs_inst);
   void emit_assignment_writes(fs_reg &l, fs_reg &r,
			       const glsl_type *type, bool predicated);
   void resolve_ud_negate(fs_reg *reg);
   void resolve_bool_comparison(ir_rvalue *rvalue, fs_reg *reg);

   fs_reg get_timestamp();

   struct brw_reg interp_reg(int location, int channel);
   void setup_uniform_values(ir_variable *ir);
   void setup_builtin_uniform_values(ir_variable *ir);
   int implied_mrf_writes(fs_inst *inst);

   virtual void dump_instructions();
   virtual void dump_instructions(const char *name);
   void dump_instruction(backend_instruction *inst);
   void dump_instruction(backend_instruction *inst, FILE *file);

   void visit_atomic_counter_intrinsic(ir_call *ir);

   struct gl_fragment_program *fp;
   const struct brw_wm_prog_key *const key;
   struct brw_wm_prog_data *prog_data;
   unsigned int sanity_param_count;

   int *param_size;

   int *virtual_grf_sizes;
   int virtual_grf_count;
   int virtual_grf_array_size;
   int *virtual_grf_start;
   int *virtual_grf_end;
   brw::fs_live_variables *live_intervals;

   int *regs_live_at_ip;

   /** Number of uniform variable components visited. */
   unsigned uniforms;

   /** Byte-offset for the next available spot in the scratch space buffer. */
   unsigned last_scratch;

   /**
    * Array mapping UNIFORM register numbers to the pull parameter index,
    * or -1 if this uniform register isn't being uploaded as a pull constant.
    */
   int *pull_constant_loc;

   /**
    * Array mapping UNIFORM register numbers to the push parameter index,
    * or -1 if this uniform register isn't being uploaded as a push constant.
    */
   int *push_constant_loc;

   struct hash_table *variable_ht;
   fs_reg frag_depth;
   fs_reg sample_mask;
   fs_reg outputs[BRW_MAX_DRAW_BUFFERS];
   unsigned output_components[BRW_MAX_DRAW_BUFFERS];
   fs_reg dual_src_output;
   bool do_dual_src;
   int first_non_payload_grf;
   /** Either BRW_MAX_GRF or GEN7_MRF_HACK_START */
   int max_grf;

   fs_reg *fp_temp_regs;
   fs_reg *fp_input_regs;

   /** @{ debug annotation info */
   const char *current_annotation;
   const void *base_ir;
   /** @} */

   bool failed;
   char *fail_msg;
   bool simd16_unsupported;
   char *no16_msg;

   /* Result of last visit() method. */
   fs_reg result;

   /** Register numbers for thread payload fields. */
   struct {
      uint8_t source_depth_reg;
      uint8_t source_w_reg;
      uint8_t aa_dest_stencil_reg;
      uint8_t dest_depth_reg;
      uint8_t sample_pos_reg;
      uint8_t sample_mask_in_reg;
      uint8_t barycentric_coord_reg[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];

      /** The number of thread payload registers the hardware will supply. */
      uint8_t num_regs;
   } payload;

   bool source_depth_to_render_target;
   bool runtime_check_aads_emit;

   fs_reg pixel_x;
   fs_reg pixel_y;
   fs_reg wpos_w;
   fs_reg pixel_w;
   fs_reg delta_x[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];
   fs_reg delta_y[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];
   fs_reg shader_start_time;

   int grf_used;
   bool spilled_any_registers;

   const unsigned dispatch_width; /**< 8 or 16 */

   int force_uncompressed_stack;
};

/**
 * The fragment shader code generator.
 *
 * Translates FS IR to actual i965 assembly code.
 */
class fs_generator
{
public:
   fs_generator(struct brw_context *brw,
                void *mem_ctx,
                const struct brw_wm_prog_key *key,
                struct brw_wm_prog_data *prog_data,
                struct gl_shader_program *prog,
                struct gl_fragment_program *fp,
                bool runtime_check_aads_emit,
                bool debug_flag);
   ~fs_generator();

   const unsigned *generate_assembly(const cfg_t *simd8_cfg,
                                     const cfg_t *simd16_cfg,
                                     unsigned *assembly_size);

private:
   void generate_code(const cfg_t *cfg);
   void fire_fb_write(fs_inst *inst,
                      GLuint base_reg,
                      struct brw_reg implied_header,
                      GLuint nr);
   void generate_fb_write(fs_inst *inst);
   void generate_blorp_fb_write(fs_inst *inst);
   void generate_rep_fb_write(fs_inst *inst);
   void generate_pixel_xy(struct brw_reg dst, bool is_x);
   void generate_linterp(fs_inst *inst, struct brw_reg dst,
			 struct brw_reg *src);
   void generate_tex(fs_inst *inst, struct brw_reg dst, struct brw_reg src,
                     struct brw_reg sampler_index);
   void generate_math_gen6(fs_inst *inst,
                           struct brw_reg dst,
                           struct brw_reg src0,
                           struct brw_reg src1);
   void generate_math_gen4(fs_inst *inst,
			   struct brw_reg dst,
			   struct brw_reg src);
   void generate_math_g45(fs_inst *inst,
			  struct brw_reg dst,
			  struct brw_reg src);
   void generate_ddx(fs_inst *inst, struct brw_reg dst, struct brw_reg src, struct brw_reg quality);
   void generate_ddy(fs_inst *inst, struct brw_reg dst, struct brw_reg src,
                     struct brw_reg quality, bool negate_value);
   void generate_scratch_write(fs_inst *inst, struct brw_reg src);
   void generate_scratch_read(fs_inst *inst, struct brw_reg dst);
   void generate_scratch_read_gen7(fs_inst *inst, struct brw_reg dst);
   void generate_uniform_pull_constant_load(fs_inst *inst, struct brw_reg dst,
                                            struct brw_reg index,
                                            struct brw_reg offset);
   void generate_uniform_pull_constant_load_gen7(fs_inst *inst,
                                                 struct brw_reg dst,
                                                 struct brw_reg surf_index,
                                                 struct brw_reg offset);
   void generate_varying_pull_constant_load(fs_inst *inst, struct brw_reg dst,
                                            struct brw_reg index,
                                            struct brw_reg offset);
   void generate_varying_pull_constant_load_gen7(fs_inst *inst,
                                                 struct brw_reg dst,
                                                 struct brw_reg index,
                                                 struct brw_reg offset);
   void generate_mov_dispatch_to_flags(fs_inst *inst);

   void generate_pixel_interpolator_query(fs_inst *inst,
                                          struct brw_reg dst,
                                          struct brw_reg src,
                                          struct brw_reg msg_data,
                                          unsigned msg_type);

   void generate_set_omask(fs_inst *inst,
                           struct brw_reg dst,
                           struct brw_reg sample_mask);

   void generate_set_sample_id(fs_inst *inst,
                               struct brw_reg dst,
                               struct brw_reg src0,
                               struct brw_reg src1);

   void generate_set_simd4x2_offset(fs_inst *inst,
                                    struct brw_reg dst,
                                    struct brw_reg offset);
   void generate_discard_jump(fs_inst *inst);

   void generate_pack_half_2x16_split(fs_inst *inst,
                                      struct brw_reg dst,
                                      struct brw_reg x,
                                      struct brw_reg y);
   void generate_unpack_half_2x16_split(fs_inst *inst,
                                        struct brw_reg dst,
                                        struct brw_reg src);

   void generate_shader_time_add(fs_inst *inst,
                                 struct brw_reg payload,
                                 struct brw_reg offset,
                                 struct brw_reg value);

   void generate_untyped_atomic(fs_inst *inst,
                                struct brw_reg dst,
                                struct brw_reg atomic_op,
                                struct brw_reg surf_index);

   void generate_untyped_surface_read(fs_inst *inst,
                                      struct brw_reg dst,
                                      struct brw_reg surf_index);

   bool patch_discard_jumps_to_fb_writes();

   struct brw_context *brw;
   struct gl_context *ctx;

   struct brw_compile *p;
   const struct brw_wm_prog_key *const key;
   struct brw_wm_prog_data *prog_data;

   struct gl_shader_program *prog;
   const struct gl_fragment_program *fp;

   unsigned dispatch_width; /**< 8 or 16 */

   exec_list discard_halt_patches;
   bool runtime_check_aads_emit;
   const bool debug_flag;
   void *mem_ctx;
};

bool brw_do_channel_expressions(struct exec_list *instructions);
bool brw_do_vector_splitting(struct exec_list *instructions);
bool brw_fs_precompile(struct gl_context *ctx, struct gl_shader_program *prog);

struct brw_reg brw_reg_from_fs_reg(fs_reg *reg);
