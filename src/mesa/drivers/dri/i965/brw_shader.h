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

#ifdef __cplusplus

class backend_instruction : public exec_node {
public:
   bool is_tex() const;
   bool is_math() const;
   bool is_control_flow() const;
   bool can_do_source_mods() const;
   bool can_do_saturate() const;
   bool reads_accumulator_implicitly() const;

   /**
    * True if the instruction has side effects other than writing to
    * its destination registers.  You are expected not to reorder or
    * optimize these out unless you know what you are doing.
    */
   bool has_side_effects() const;

   enum opcode opcode; /* BRW_OPCODE_* or FS_OPCODE_* */

   uint8_t predicate;
   bool predicate_inverse;
   bool writes_accumulator; /**< instruction implicitly writes accumulator */
};

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

   virtual void dump_instruction(backend_instruction *inst) = 0;
   virtual void dump_instructions();

   void assign_common_binding_table_offsets(uint32_t next_binding_table_offset);

   virtual void invalidate_live_intervals() = 0;
};

uint32_t brw_texture_offset(struct gl_context *ctx, ir_constant *offset);

#endif /* __cplusplus */

int brw_type_for_base_type(const struct glsl_type *type);
uint32_t brw_conditional_for_comparison(unsigned int op);
uint32_t brw_math_function(enum opcode op);
const char *brw_instruction_name(enum opcode op);
