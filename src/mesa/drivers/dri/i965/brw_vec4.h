/*
 * Copyright © 2011 Intel Corporation
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

#ifndef BRW_VEC4_H
#define BRW_VEC4_H

#include <stdint.h>
#include "brw_shader.h"
#include "main/compiler.h"
#include "program/hash_table.h"
#include "brw_program.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "brw_context.h"
#include "brw_eu.h"
#include "intel_asm_annotation.h"

#ifdef __cplusplus
}; /* extern "C" */
#endif

#include "glsl/ir.h"


struct brw_vec4_compile {
   GLuint last_scratch; /**< measured in 32-byte (register size) units */
};


struct brw_vec4_prog_key {
   GLuint program_string_id;

   /**
    * True if at least one clip flag is enabled, regardless of whether the
    * shader uses clip planes or gl_ClipDistance.
    */
   GLuint userclip_active:1;

   /**
    * How many user clipping planes are being uploaded to the vertex shader as
    * push constants.
    */
   GLuint nr_userclip_plane_consts:4;

   GLuint clamp_vertex_color:1;

   struct brw_sampler_prog_key_data tex;
};


#ifdef __cplusplus
extern "C" {
#endif

void
brw_vec4_setup_prog_key_for_precompile(struct gl_context *ctx,
                                       struct brw_vec4_prog_key *key,
                                       GLuint id, struct gl_program *prog);

#ifdef __cplusplus
} /* extern "C" */

namespace brw {

class dst_reg;

unsigned
swizzle_for_size(int size);

class src_reg : public backend_reg
{
public:
   DECLARE_RALLOC_CXX_OPERATORS(src_reg)

   void init();

   src_reg(register_file file, int reg, const glsl_type *type);
   src_reg();
   src_reg(float f);
   src_reg(uint32_t u);
   src_reg(int32_t i);
   src_reg(struct brw_reg reg);

   bool equals(const src_reg &r) const;

   src_reg(class vec4_visitor *v, const struct glsl_type *type);
   src_reg(class vec4_visitor *v, const struct glsl_type *type, int size);

   explicit src_reg(dst_reg reg);

   GLuint swizzle; /**< BRW_SWIZZLE_XYZW macros from brw_reg.h. */

   src_reg *reladdr;
};

static inline src_reg
retype(src_reg reg, enum brw_reg_type type)
{
   reg.fixed_hw_reg.type = reg.type = type;
   return reg;
}

static inline src_reg
offset(src_reg reg, unsigned delta)
{
   assert(delta == 0 || (reg.file != HW_REG && reg.file != IMM));
   reg.reg_offset += delta;
   return reg;
}

/**
 * Reswizzle a given source register.
 * \sa brw_swizzle().
 */
static inline src_reg
swizzle(src_reg reg, unsigned swizzle)
{
   assert(reg.file != HW_REG);
   reg.swizzle = BRW_SWIZZLE4(
      BRW_GET_SWZ(reg.swizzle, BRW_GET_SWZ(swizzle, 0)),
      BRW_GET_SWZ(reg.swizzle, BRW_GET_SWZ(swizzle, 1)),
      BRW_GET_SWZ(reg.swizzle, BRW_GET_SWZ(swizzle, 2)),
      BRW_GET_SWZ(reg.swizzle, BRW_GET_SWZ(swizzle, 3)));
   return reg;
}

static inline src_reg
negate(src_reg reg)
{
   assert(reg.file != HW_REG && reg.file != IMM);
   reg.negate = !reg.negate;
   return reg;
}

class dst_reg : public backend_reg
{
public:
   DECLARE_RALLOC_CXX_OPERATORS(dst_reg)

   void init();

   dst_reg();
   dst_reg(register_file file, int reg);
   dst_reg(register_file file, int reg, const glsl_type *type, int writemask);
   dst_reg(struct brw_reg reg);
   dst_reg(class vec4_visitor *v, const struct glsl_type *type);

   explicit dst_reg(src_reg reg);

   int writemask; /**< Bitfield of WRITEMASK_[XYZW] */

   src_reg *reladdr;
};

static inline dst_reg
retype(dst_reg reg, enum brw_reg_type type)
{
   reg.fixed_hw_reg.type = reg.type = type;
   return reg;
}

static inline dst_reg
offset(dst_reg reg, unsigned delta)
{
   assert(delta == 0 || (reg.file != HW_REG && reg.file != IMM));
   reg.reg_offset += delta;
   return reg;
}

static inline dst_reg
writemask(dst_reg reg, unsigned mask)
{
   assert(reg.file != HW_REG && reg.file != IMM);
   assert((reg.writemask & mask) != 0);
   reg.writemask &= mask;
   return reg;
}

class vec4_instruction : public backend_instruction {
public:
   DECLARE_RALLOC_CXX_OPERATORS(vec4_instruction)

   vec4_instruction(vec4_visitor *v, enum opcode opcode,
                    const dst_reg &dst = dst_reg(),
                    const src_reg &src0 = src_reg(),
                    const src_reg &src1 = src_reg(),
                    const src_reg &src2 = src_reg());

   struct brw_reg get_dst(void);
   struct brw_reg get_src(const struct brw_vec4_prog_data *prog_data, int i);

   dst_reg dst;
   src_reg src[3];

   bool shadow_compare;

   enum brw_urb_write_flags urb_write_flags;
   bool header_present;

   unsigned sol_binding; /**< gen6: SOL binding table index */
   bool sol_final_write; /**< gen6: send commit message */
   unsigned sol_vertex; /**< gen6: used for setting dst index in SVB header */

   bool is_send_from_grf();
   bool can_reswizzle(int dst_writemask, int swizzle, int swizzle_mask);
   void reswizzle(int dst_writemask, int swizzle);
   bool can_do_source_mods(struct brw_context *brw);

   bool reads_flag()
   {
      return predicate || opcode == VS_OPCODE_UNPACK_FLAGS_SIMD4X2;
   }

   bool writes_flag()
   {
      return conditional_mod && opcode != BRW_OPCODE_SEL;
   }
};

/**
 * The vertex shader front-end.
 *
 * Translates either GLSL IR or Mesa IR (for ARB_vertex_program and
 * fixed-function) into VS IR.
 */
class vec4_visitor : public backend_visitor
{
public:
   vec4_visitor(struct brw_context *brw,
                struct brw_vec4_compile *c,
                struct gl_program *prog,
                const struct brw_vec4_prog_key *key,
                struct brw_vec4_prog_data *prog_data,
		struct gl_shader_program *shader_prog,
                gl_shader_stage stage,
		void *mem_ctx,
                bool debug_flag,
                bool no_spills,
                shader_time_shader_type st_base,
                shader_time_shader_type st_written,
                shader_time_shader_type st_reset);
   ~vec4_visitor();

   dst_reg dst_null_f()
   {
      return dst_reg(brw_null_reg());
   }

   dst_reg dst_null_d()
   {
      return dst_reg(retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   }

   dst_reg dst_null_ud()
   {
      return dst_reg(retype(brw_null_reg(), BRW_REGISTER_TYPE_UD));
   }

   struct brw_vec4_compile * const c;
   const struct brw_vec4_prog_key * const key;
   struct brw_vec4_prog_data * const prog_data;
   unsigned int sanity_param_count;

   char *fail_msg;
   bool failed;

   /**
    * GLSL IR currently being processed, which is associated with our
    * driver IR instructions for debugging purposes.
    */
   const void *base_ir;
   const char *current_annotation;

   int *virtual_grf_sizes;
   int virtual_grf_count;
   int virtual_grf_array_size;
   int first_non_payload_grf;
   unsigned int max_grf;
   int *virtual_grf_start;
   int *virtual_grf_end;
   dst_reg userplane[MAX_CLIP_PLANES];

   /**
    * This is the size to be used for an array with an element per
    * reg_offset
    */
   int virtual_grf_reg_count;
   /** Per-virtual-grf indices into an array of size virtual_grf_reg_count */
   int *virtual_grf_reg_map;

   bool live_intervals_valid;

   dst_reg *variable_storage(ir_variable *var);

   void reladdr_to_temp(ir_instruction *ir, src_reg *reg, int *num_reladdr);

   bool need_all_constants_in_pull_buffer;

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_variable *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference_variable  *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_if *);
   virtual void visit(ir_emit_vertex *);
   virtual void visit(ir_end_primitive *);
   /*@}*/

   src_reg result;

   /* Regs for vertex results.  Generated at ir_variable visiting time
    * for the ir->location's used.
    */
   dst_reg output_reg[BRW_VARYING_SLOT_COUNT];
   const char *output_reg_annotation[BRW_VARYING_SLOT_COUNT];
   int *uniform_size;
   int *uniform_vector_size;
   int uniform_array_size; /*< Size of uniform_[vector_]size arrays */
   int uniforms;

   src_reg shader_start_time;

   struct hash_table *variable_ht;

   bool run(void);
   void fail(const char *msg, ...);

   int virtual_grf_alloc(int size);
   void setup_uniform_clipplane_values();
   void setup_uniform_values(ir_variable *ir);
   void setup_builtin_uniform_values(ir_variable *ir);
   int setup_uniforms(int payload_reg);
   bool reg_allocate_trivial();
   bool reg_allocate();
   void evaluate_spill_costs(float *spill_costs, bool *no_spill);
   int choose_spill_reg(struct ra_graph *g);
   void spill_reg(int spill_reg);
   void move_grf_array_access_to_scratch();
   void move_uniform_array_access_to_pull_constants();
   void move_push_constants_to_pull_constants();
   void split_uniform_registers();
   void pack_uniform_registers();
   void calculate_live_intervals();
   void invalidate_live_intervals(bool invalidate_cfg = true);
   void split_virtual_grfs();
   bool opt_reduce_swizzle();
   bool dead_code_eliminate();
   bool virtual_grf_interferes(int a, int b);
   bool opt_copy_propagation();
   bool opt_cse_local(bblock_t *block);
   bool opt_cse();
   bool opt_algebraic();
   bool opt_register_coalesce();
   void opt_set_dependency_control();
   void opt_schedule_instructions();

   vec4_instruction *emit(vec4_instruction *inst);

   vec4_instruction *emit(enum opcode opcode);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst, src_reg src0);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst,
			  src_reg src0, src_reg src1);

   vec4_instruction *emit(enum opcode opcode, dst_reg dst,
			  src_reg src0, src_reg src1, src_reg src2);

   vec4_instruction *emit_before(bblock_t *block,
                                 vec4_instruction *inst,
				 vec4_instruction *new_inst);

   vec4_instruction *MOV(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *NOT(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *RNDD(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *RNDE(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *RNDZ(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *FRC(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *F32TO16(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *F16TO32(const dst_reg &dst, const src_reg &src0);
   vec4_instruction *ADD(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *MUL(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *MACH(const dst_reg &dst, const src_reg &src0,
                          const src_reg &src1);
   vec4_instruction *MAC(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *AND(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *OR(const dst_reg &dst, const src_reg &src0,
                        const src_reg &src1);
   vec4_instruction *XOR(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *DP3(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *DP4(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *DPH(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *SHL(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *SHR(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *ASR(const dst_reg &dst, const src_reg &src0,
                         const src_reg &src1);
   vec4_instruction *CMP(dst_reg dst, src_reg src0, src_reg src1,
			 enum brw_conditional_mod condition);
   vec4_instruction *IF(src_reg src0, src_reg src1,
                        enum brw_conditional_mod condition);
   vec4_instruction *IF(enum brw_predicate predicate);
   vec4_instruction *PULL_CONSTANT_LOAD(const dst_reg &dst,
                                        const src_reg &index);
   vec4_instruction *SCRATCH_READ(const dst_reg &dst, const src_reg &index);
   vec4_instruction *SCRATCH_WRITE(const dst_reg &dst, const src_reg &src,
                                   const src_reg &index);
   vec4_instruction *LRP(const dst_reg &dst, const src_reg &a,
                         const src_reg &y, const src_reg &x);
   vec4_instruction *BFREV(const dst_reg &dst, const src_reg &value);
   vec4_instruction *BFE(const dst_reg &dst, const src_reg &bits,
                         const src_reg &offset, const src_reg &value);
   vec4_instruction *BFI1(const dst_reg &dst, const src_reg &bits,
                          const src_reg &offset);
   vec4_instruction *BFI2(const dst_reg &dst, const src_reg &bfi1_dst,
                          const src_reg &insert, const src_reg &base);
   vec4_instruction *FBH(const dst_reg &dst, const src_reg &value);
   vec4_instruction *FBL(const dst_reg &dst, const src_reg &value);
   vec4_instruction *CBIT(const dst_reg &dst, const src_reg &value);
   vec4_instruction *MAD(const dst_reg &dst, const src_reg &c,
                         const src_reg &b, const src_reg &a);
   vec4_instruction *ADDC(const dst_reg &dst, const src_reg &src0,
                          const src_reg &src1);
   vec4_instruction *SUBB(const dst_reg &dst, const src_reg &src0,
                          const src_reg &src1);

   int implied_mrf_writes(vec4_instruction *inst);

   bool try_rewrite_rhs_to_dst(ir_assignment *ir,
			       dst_reg dst,
			       src_reg src,
			       vec4_instruction *pre_rhs_inst,
			       vec4_instruction *last_rhs_inst);

   /** Walks an exec_list of ir_instruction and sends it through this visitor. */
   void visit_instructions(const exec_list *list);

   void emit_vp_sop(enum brw_conditional_mod condmod, dst_reg dst,
                    src_reg src0, src_reg src1, src_reg one);

   void emit_bool_to_cond_code(ir_rvalue *ir, enum brw_predicate *predicate);
   void emit_if_gen6(ir_if *ir);

   void emit_minmax(enum brw_conditional_mod conditionalmod, dst_reg dst,
                    src_reg src0, src_reg src1);

   void emit_lrp(const dst_reg &dst,
                 const src_reg &x, const src_reg &y, const src_reg &a);

   void emit_block_move(dst_reg *dst, src_reg *src,
                        const struct glsl_type *type, brw_predicate predicate);

   void emit_constant_values(dst_reg *dst, ir_constant *value);

   /**
    * Emit the correct dot-product instruction for the type of arguments
    */
   void emit_dp(dst_reg dst, src_reg src0, src_reg src1, unsigned elements);

   void emit_scalar(ir_instruction *ir, enum prog_opcode op,
		    dst_reg dst, src_reg src0);

   void emit_scalar(ir_instruction *ir, enum prog_opcode op,
		    dst_reg dst, src_reg src0, src_reg src1);

   void emit_scs(ir_instruction *ir, enum prog_opcode op,
		 dst_reg dst, const src_reg &src);

   src_reg fix_3src_operand(src_reg src);

   void emit_math1_gen6(enum opcode opcode, dst_reg dst, src_reg src);
   void emit_math1_gen4(enum opcode opcode, dst_reg dst, src_reg src);
   void emit_math(enum opcode opcode, dst_reg dst, src_reg src);
   void emit_math2_gen6(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1);
   void emit_math2_gen4(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1);
   void emit_math(enum opcode opcode, dst_reg dst, src_reg src0, src_reg src1);
   src_reg fix_math_operand(src_reg src);

   void emit_pack_half_2x16(dst_reg dst, src_reg src0);
   void emit_unpack_half_2x16(dst_reg dst, src_reg src0);

   uint32_t gather_channel(ir_texture *ir, uint32_t sampler);
   src_reg emit_mcs_fetch(ir_texture *ir, src_reg coordinate, src_reg sampler);
   void emit_gen6_gather_wa(uint8_t wa, dst_reg dst);
   void swizzle_result(ir_texture *ir, src_reg orig_val, uint32_t sampler);

   void emit_ndc_computation();
   void emit_psiz_and_flags(dst_reg reg);
   void emit_clip_distances(dst_reg reg, int offset);
   void emit_generic_urb_slot(dst_reg reg, int varying);
   void emit_urb_slot(dst_reg reg, int varying);

   void emit_shader_time_begin();
   void emit_shader_time_end();
   void emit_shader_time_write(enum shader_time_shader_type type,
                               src_reg value);

   void emit_untyped_atomic(unsigned atomic_op, unsigned surf_index,
                            dst_reg dst, src_reg offset, src_reg src0,
                            src_reg src1);

   void emit_untyped_surface_read(unsigned surf_index, dst_reg dst,
                                  src_reg offset);

   src_reg get_scratch_offset(bblock_t *block, vec4_instruction *inst,
			      src_reg *reladdr, int reg_offset);
   src_reg get_pull_constant_offset(bblock_t *block, vec4_instruction *inst,
				    src_reg *reladdr, int reg_offset);
   void emit_scratch_read(bblock_t *block, vec4_instruction *inst,
			  dst_reg dst,
			  src_reg orig_src,
			  int base_offset);
   void emit_scratch_write(bblock_t *block, vec4_instruction *inst,
			   int base_offset);
   void emit_pull_constant_load(bblock_t *block, vec4_instruction *inst,
				dst_reg dst,
				src_reg orig_src,
				int base_offset);

   bool try_emit_mad(ir_expression *ir);
   bool try_emit_b2f_of_compare(ir_expression *ir);
   void resolve_ud_negate(src_reg *reg);

   src_reg get_timestamp();

   bool process_move_condition(ir_rvalue *ir);

   void dump_instruction(backend_instruction *inst);
   void dump_instruction(backend_instruction *inst, FILE *file);

   void visit_atomic_counter_intrinsic(ir_call *ir);

protected:
   void emit_vertex();
   void lower_attributes_to_hw_regs(const int *attribute_map,
                                    bool interleaved);
   void setup_payload_interference(struct ra_graph *g, int first_payload_node,
                                   int reg_node_count);
   virtual dst_reg *make_reg_for_system_value(ir_variable *ir) = 0;
   virtual void assign_binding_table_offsets();
   virtual void setup_payload() = 0;
   virtual void emit_prolog() = 0;
   virtual void emit_program_code() = 0;
   virtual void emit_thread_end() = 0;
   virtual void emit_urb_write_header(int mrf) = 0;
   virtual vec4_instruction *emit_urb_write_opcode(bool complete) = 0;
   virtual int compute_array_stride(ir_dereference_array *ir);

   const bool debug_flag;

private:
   /**
    * If true, then register allocation should fail instead of spilling.
    */
   const bool no_spills;

   const shader_time_shader_type st_base;
   const shader_time_shader_type st_written;
   const shader_time_shader_type st_reset;
};


/**
 * The vertex shader code generator.
 *
 * Translates VS IR to actual i965 assembly code.
 */
class vec4_generator
{
public:
   vec4_generator(struct brw_context *brw,
                  struct gl_shader_program *shader_prog,
                  struct gl_program *prog,
                  struct brw_vec4_prog_data *prog_data,
                  void *mem_ctx,
                  bool debug_flag);
   ~vec4_generator();

   const unsigned *generate_assembly(const cfg_t *cfg, unsigned *asm_size);

private:
   void generate_code(const cfg_t *cfg);

   void generate_math1_gen4(vec4_instruction *inst,
			    struct brw_reg dst,
			    struct brw_reg src);
   void generate_math2_gen4(vec4_instruction *inst,
			    struct brw_reg dst,
			    struct brw_reg src0,
			    struct brw_reg src1);
   void generate_math_gen6(vec4_instruction *inst,
                           struct brw_reg dst,
                           struct brw_reg src0,
                           struct brw_reg src1);

   void generate_tex(vec4_instruction *inst,
                     struct brw_reg dst,
                     struct brw_reg src,
                     struct brw_reg sampler_index);

   void generate_vs_urb_write(vec4_instruction *inst);
   void generate_gs_urb_write(vec4_instruction *inst);
   void generate_gs_urb_write_allocate(vec4_instruction *inst);
   void generate_gs_thread_end(vec4_instruction *inst);
   void generate_gs_set_write_offset(struct brw_reg dst,
                                     struct brw_reg src0,
                                     struct brw_reg src1);
   void generate_gs_set_vertex_count(struct brw_reg dst,
                                     struct brw_reg src);
   void generate_gs_svb_write(vec4_instruction *inst,
                              struct brw_reg dst,
                              struct brw_reg src0,
                              struct brw_reg src1);
   void generate_gs_svb_set_destination_index(vec4_instruction *inst,
                                              struct brw_reg dst,
                                              struct brw_reg src);
   void generate_gs_set_dword_2(struct brw_reg dst, struct brw_reg src);
   void generate_gs_prepare_channel_masks(struct brw_reg dst);
   void generate_gs_set_channel_masks(struct brw_reg dst, struct brw_reg src);
   void generate_gs_get_instance_id(struct brw_reg dst);
   void generate_gs_ff_sync_set_primitives(struct brw_reg dst,
                                           struct brw_reg src0,
                                           struct brw_reg src1,
                                           struct brw_reg src2);
   void generate_gs_ff_sync(vec4_instruction *inst,
                            struct brw_reg dst,
                            struct brw_reg src0,
                            struct brw_reg src1);
   void generate_gs_set_primitive_id(struct brw_reg dst);
   void generate_oword_dual_block_offsets(struct brw_reg m1,
					  struct brw_reg index);
   void generate_scratch_write(vec4_instruction *inst,
			       struct brw_reg dst,
			       struct brw_reg src,
			       struct brw_reg index);
   void generate_scratch_read(vec4_instruction *inst,
			      struct brw_reg dst,
			      struct brw_reg index);
   void generate_pull_constant_load(vec4_instruction *inst,
				    struct brw_reg dst,
				    struct brw_reg index,
				    struct brw_reg offset);
   void generate_pull_constant_load_gen7(vec4_instruction *inst,
                                         struct brw_reg dst,
                                         struct brw_reg surf_index,
                                         struct brw_reg offset);
   void generate_unpack_flags(vec4_instruction *inst,
                              struct brw_reg dst);

   void generate_untyped_atomic(vec4_instruction *inst,
                                struct brw_reg dst,
                                struct brw_reg atomic_op,
                                struct brw_reg surf_index);

   void generate_untyped_surface_read(vec4_instruction *inst,
                                      struct brw_reg dst,
                                      struct brw_reg surf_index);

   struct brw_context *brw;

   struct brw_compile *p;

   struct gl_shader_program *shader_prog;
   const struct gl_program *prog;

   struct brw_vec4_prog_data *prog_data;

   void *mem_ctx;
   const bool debug_flag;
};

} /* namespace brw */
#endif /* __cplusplus */

#endif /* BRW_VEC4_H */
