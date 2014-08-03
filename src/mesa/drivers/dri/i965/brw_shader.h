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
 */

#include <stdint.h>
#include "brw_reg.h"
#include "brw_defines.h"
#include "main/compiler.h"
#include "glsl/ir.h"

#pragma once

enum PACKED register_file {
   BAD_FILE,
   GRF,
   MRF,
   IMM,
   HW_REG, /* a struct brw_reg */
   ATTR,
   UNIFORM, /* prog_data->params[reg] */
};

struct backend_reg
{
#ifdef __cplusplus
   bool is_zero() const;
   bool is_one() const;
   bool is_null() const;
   bool is_accumulator() const;
#endif

   enum register_file file; /**< Register file: GRF, MRF, IMM. */
   enum brw_reg_type type;  /**< Register type: BRW_REGISTER_TYPE_* */

   /**
    * Register number.
    *
    * For GRF, it's a virtual register number until register allocation.
    *
    * For MRF, it's the hardware register.
    */
   uint16_t reg;

   /**
    * Offset within the virtual register.
    *
    * In the scalar backend, this is in units of a float per pixel for pre-
    * register allocation registers (i.e., one register in SIMD8 mode and two
    * registers in SIMD16 mode).
    *
    * For uniforms, this is in units of 1 float.
    */
   int reg_offset;

   struct brw_reg fixed_hw_reg;

   bool negate;
   bool abs;
};

struct cfg_t;

#ifdef __cplusplus
struct backend_instruction : public exec_node {
   bool is_tex() const;
   bool is_math() const;
   bool is_control_flow() const;
   bool can_do_source_mods() const;
   bool can_do_saturate() const;
   bool reads_accumulator_implicitly() const;
   bool writes_accumulator_implicitly(struct brw_context *brw) const;

   /**
    * True if the instruction has side effects other than writing to
    * its destination registers.  You are expected not to reorder or
    * optimize these out unless you know what you are doing.
    */
   bool has_side_effects() const;
#else
struct backend_instruction {
   struct exec_node link;
#endif
   /** @{
    * Annotation for the generated IR.  One of the two can be set.
    */
   const void *ir;
   const char *annotation;
   /** @} */

   uint32_t texture_offset; /**< Texture offset bitfield */
   uint32_t offset; /**< spill/unspill offset */
   uint8_t mlen; /**< SEND message length */
   int8_t base_mrf; /**< First MRF in the SEND message, if mlen is nonzero. */
   uint8_t target; /**< MRT target. */

   enum opcode opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   enum brw_conditional_mod conditional_mod; /**< BRW_CONDITIONAL_* */
   enum brw_predicate predicate;
   bool predicate_inverse:1;
   bool writes_accumulator:1; /**< instruction implicitly writes accumulator */
   bool force_writemask_all:1;
   bool no_dd_clear:1;
   bool no_dd_check:1;
   bool saturate:1;
};

#ifdef __cplusplus

enum instruction_scheduler_mode {
   SCHEDULE_PRE,
   SCHEDULE_PRE_NON_LIFO,
   SCHEDULE_PRE_LIFO,
   SCHEDULE_POST,
};

class backend_visitor : public ir_visitor {
protected:

   backend_visitor(struct brw_context *brw,
                   struct gl_shader_program *shader_prog,
                   struct gl_program *prog,
                   struct brw_stage_prog_data *stage_prog_data,
                   gl_shader_stage stage);

public:

   struct brw_context * const brw;
   struct gl_context * const ctx;
   struct brw_shader * const shader;
   struct gl_shader_program * const shader_prog;
   struct gl_program * const prog;
   struct brw_stage_prog_data * const stage_prog_data;

   /** ralloc context for temporary data used during compile */
   void *mem_ctx;

   /**
    * List of either fs_inst or vec4_instruction (inheriting from
    * backend_instruction)
    */
   exec_list instructions;

   cfg_t *cfg;

   gl_shader_stage stage;

   virtual void dump_instruction(backend_instruction *inst) = 0;
   virtual void dump_instruction(backend_instruction *inst, FILE *file) = 0;
   virtual void dump_instructions();
   virtual void dump_instructions(const char *name);

   void calculate_cfg();
   void invalidate_cfg();

   void assign_common_binding_table_offsets(uint32_t next_binding_table_offset);

   virtual void invalidate_live_intervals() = 0;
};

uint32_t brw_texture_offset(struct gl_context *ctx, ir_constant *offset);

#endif /* __cplusplus */

enum brw_reg_type brw_type_for_base_type(const struct glsl_type *type);
enum brw_conditional_mod brw_conditional_for_comparison(unsigned int op);
uint32_t brw_math_function(enum opcode op);
const char *brw_instruction_name(enum opcode op);
