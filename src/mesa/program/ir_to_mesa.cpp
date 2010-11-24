/*
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  VMware, Inc.   All Rights Reserved.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_to_mesa.cpp
 *
 * Translate GLSL IR to Mesa's gl_program representation.
 */

#include <stdio.h>
#include "main/compiler.h"
#include "ir.h"
#include "ir_visitor.h"
#include "ir_print_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "../glsl/program.h"
#include "ir_optimization.h"
#include "ast.h"

extern "C" {
#include "main/mtypes.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/hash_table.h"
#include "program/prog_instruction.h"
#include "program/prog_optimize.h"
#include "program/prog_print.h"
#include "program/program.h"
#include "program/prog_uniform.h"
#include "program/prog_parameter.h"
#include "program/sampler.h"
}

static int swizzle_for_size(int size);

/**
 * This struct is a corresponding struct to Mesa prog_src_register, with
 * wider fields.
 */
typedef struct ir_to_mesa_src_reg {
   ir_to_mesa_src_reg(int file, int index, const glsl_type *type)
   {
      this->file = (gl_register_file) file;
      this->index = index;
      if (type && (type->is_scalar() || type->is_vector() || type->is_matrix()))
	 this->swizzle = swizzle_for_size(type->vector_elements);
      else
	 this->swizzle = SWIZZLE_XYZW;
      this->negate = 0;
      this->reladdr = NULL;
   }

   ir_to_mesa_src_reg()
   {
      this->file = PROGRAM_UNDEFINED;
      this->index = 0;
      this->swizzle = 0;
      this->negate = 0;
      this->reladdr = NULL;
   }

   gl_register_file file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   GLuint swizzle; /**< SWIZZLE_XYZWONEZERO swizzles from Mesa. */
   int negate; /**< NEGATE_XYZW mask from mesa */
   /** Register index should be offset by the integer in this reg. */
   ir_to_mesa_src_reg *reladdr;
} ir_to_mesa_src_reg;

typedef struct ir_to_mesa_dst_reg {
   int file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   int writemask; /**< Bitfield of WRITEMASK_[XYZW] */
   GLuint cond_mask:4;
   /** Register index should be offset by the integer in this reg. */
   ir_to_mesa_src_reg *reladdr;
} ir_to_mesa_dst_reg;

extern ir_to_mesa_src_reg ir_to_mesa_undef;

class ir_to_mesa_instruction : public exec_node {
public:
   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = talloc_zero_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   enum prog_opcode op;
   ir_to_mesa_dst_reg dst_reg;
   ir_to_mesa_src_reg src_reg[3];
   /** Pointer to the ir source this tree came from for debugging */
   ir_instruction *ir;
   GLboolean cond_update;
   bool saturate;
   int sampler; /**< sampler index */
   int tex_target; /**< One of TEXTURE_*_INDEX */
   GLboolean tex_shadow;

   class function_entry *function; /* Set on OPCODE_CAL or OPCODE_BGNSUB */
};

class variable_storage : public exec_node {
public:
   variable_storage(ir_variable *var, gl_register_file file, int index)
      : file(file), index(index), var(var)
   {
      /* empty */
   }

   gl_register_file file;
   int index;
   ir_variable *var; /* variable that maps to this, if any */
};

class function_entry : public exec_node {
public:
   ir_function_signature *sig;

   /**
    * identifier of this function signature used by the program.
    *
    * At the point that Mesa instructions for function calls are
    * generated, we don't know the address of the first instruction of
    * the function body.  So we make the BranchTarget that is called a
    * small integer and rewrite them during set_branchtargets().
    */
   int sig_id;

   /**
    * Pointer to first instruction of the function body.
    *
    * Set during function body emits after main() is processed.
    */
   ir_to_mesa_instruction *bgn_inst;

   /**
    * Index of the first instruction of the function body in actual
    * Mesa IR.
    *
    * Set after convertion from ir_to_mesa_instruction to prog_instruction.
    */
   int inst;

   /** Storage for the return value. */
   ir_to_mesa_src_reg return_reg;
};

class ir_to_mesa_visitor : public ir_visitor {
public:
   ir_to_mesa_visitor();
   ~ir_to_mesa_visitor();

   function_entry *current_function;

   struct gl_context *ctx;
   struct gl_program *prog;
   struct gl_shader_program *shader_program;
   struct gl_shader_compiler_options *options;

   int next_temp;

   variable_storage *find_variable_storage(ir_variable *var);

   function_entry *get_function_signature(ir_function_signature *sig);

   ir_to_mesa_src_reg get_temp(const glsl_type *type);
   void reladdr_to_temp(ir_instruction *ir,
			ir_to_mesa_src_reg *reg, int *num_reladdr);

   struct ir_to_mesa_src_reg src_reg_for_float(float val);

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
   /*@}*/

   struct ir_to_mesa_src_reg result;

   /** List of variable_storage */
   exec_list variables;

   /** List of function_entry */
   exec_list function_signatures;
   int next_signature_id;

   /** List of ir_to_mesa_instruction */
   exec_list instructions;

   ir_to_mesa_instruction *ir_to_mesa_emit_op0(ir_instruction *ir,
					       enum prog_opcode op);

   ir_to_mesa_instruction *ir_to_mesa_emit_op1(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0);

   ir_to_mesa_instruction *ir_to_mesa_emit_op2(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0,
					       ir_to_mesa_src_reg src1);

   ir_to_mesa_instruction *ir_to_mesa_emit_op3(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0,
					       ir_to_mesa_src_reg src1,
					       ir_to_mesa_src_reg src2);

   /**
    * Emit the correct dot-product instruction for the type of arguments
    *
    * \sa ir_to_mesa_emit_op2
    */
   void ir_to_mesa_emit_dp(ir_instruction *ir,
			   ir_to_mesa_dst_reg dst,
			   ir_to_mesa_src_reg src0,
			   ir_to_mesa_src_reg src1,
			   unsigned elements);

   void ir_to_mesa_emit_scalar_op1(ir_instruction *ir,
				   enum prog_opcode op,
				   ir_to_mesa_dst_reg dst,
				   ir_to_mesa_src_reg src0);

   void ir_to_mesa_emit_scalar_op2(ir_instruction *ir,
				   enum prog_opcode op,
				   ir_to_mesa_dst_reg dst,
				   ir_to_mesa_src_reg src0,
				   ir_to_mesa_src_reg src1);

   void emit_scs(ir_instruction *ir, enum prog_opcode op,
		 ir_to_mesa_dst_reg dst,
		 const ir_to_mesa_src_reg &src);

   GLboolean try_emit_mad(ir_expression *ir,
			  int mul_operand);
   GLboolean try_emit_sat(ir_expression *ir);

   void emit_swz(ir_expression *ir);

   bool process_move_condition(ir_rvalue *ir);

   void *mem_ctx;
};

ir_to_mesa_src_reg ir_to_mesa_undef = ir_to_mesa_src_reg(PROGRAM_UNDEFINED, 0, NULL);

ir_to_mesa_dst_reg ir_to_mesa_undef_dst = {
   PROGRAM_UNDEFINED, 0, SWIZZLE_NOOP, COND_TR, NULL,
};

ir_to_mesa_dst_reg ir_to_mesa_address_reg = {
   PROGRAM_ADDRESS, 0, WRITEMASK_X, COND_TR, NULL
};

static void
fail_link(struct gl_shader_program *prog, const char *fmt, ...) PRINTFLIKE(2, 3);

static void
fail_link(struct gl_shader_program *prog, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   prog->InfoLog = talloc_vasprintf_append(prog->InfoLog, fmt, args);
   va_end(args);

   prog->LinkStatus = GL_FALSE;
}

static int
swizzle_for_size(int size)
{
   int size_swizzles[4] = {
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_Z),
      MAKE_SWIZZLE4(SWIZZLE_X, SWIZZLE_Y, SWIZZLE_Z, SWIZZLE_W),
   };

   assert((size >= 1) && (size <= 4));
   return size_swizzles[size - 1];
}

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op3(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0,
					ir_to_mesa_src_reg src1,
					ir_to_mesa_src_reg src2)
{
   ir_to_mesa_instruction *inst = new(mem_ctx) ir_to_mesa_instruction();
   int num_reladdr = 0;

   /* If we have to do relative addressing, we want to load the ARL
    * reg directly for one of the regs, and preload the other reladdr
    * sources into temps.
    */
   num_reladdr += dst.reladdr != NULL;
   num_reladdr += src0.reladdr != NULL;
   num_reladdr += src1.reladdr != NULL;
   num_reladdr += src2.reladdr != NULL;

   reladdr_to_temp(ir, &src2, &num_reladdr);
   reladdr_to_temp(ir, &src1, &num_reladdr);
   reladdr_to_temp(ir, &src0, &num_reladdr);

   if (dst.reladdr) {
      ir_to_mesa_emit_op1(ir, OPCODE_ARL, ir_to_mesa_address_reg,
                          *dst.reladdr);

      num_reladdr--;
   }
   assert(num_reladdr == 0);

   inst->op = op;
   inst->dst_reg = dst;
   inst->src_reg[0] = src0;
   inst->src_reg[1] = src1;
   inst->src_reg[2] = src2;
   inst->ir = ir;

   inst->function = NULL;

   this->instructions.push_tail(inst);

   return inst;
}


ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op2(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0,
					ir_to_mesa_src_reg src1)
{
   return ir_to_mesa_emit_op3(ir, op, dst, src0, src1, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op1(ir_instruction *ir,
					enum prog_opcode op,
					ir_to_mesa_dst_reg dst,
					ir_to_mesa_src_reg src0)
{
   assert(dst.writemask != 0);
   return ir_to_mesa_emit_op3(ir, op, dst,
			      src0, ir_to_mesa_undef, ir_to_mesa_undef);
}

ir_to_mesa_instruction *
ir_to_mesa_visitor::ir_to_mesa_emit_op0(ir_instruction *ir,
					enum prog_opcode op)
{
   return ir_to_mesa_emit_op3(ir, op, ir_to_mesa_undef_dst,
			      ir_to_mesa_undef,
			      ir_to_mesa_undef,
			      ir_to_mesa_undef);
}

void
ir_to_mesa_visitor::ir_to_mesa_emit_dp(ir_instruction *ir,
				       ir_to_mesa_dst_reg dst,
				       ir_to_mesa_src_reg src0,
				       ir_to_mesa_src_reg src1,
				       unsigned elements)
{
   static const gl_inst_opcode dot_opcodes[] = {
      OPCODE_DP2, OPCODE_DP3, OPCODE_DP4
   };

   ir_to_mesa_emit_op3(ir, dot_opcodes[elements - 2],
		       dst, src0, src1, ir_to_mesa_undef);
}

inline ir_to_mesa_dst_reg
ir_to_mesa_dst_reg_from_src(ir_to_mesa_src_reg reg)
{
   ir_to_mesa_dst_reg dst_reg;

   dst_reg.file = reg.file;
   dst_reg.index = reg.index;
   dst_reg.writemask = WRITEMASK_XYZW;
   dst_reg.cond_mask = COND_TR;
   dst_reg.reladdr = reg.reladdr;

   return dst_reg;
}

inline ir_to_mesa_src_reg
ir_to_mesa_src_reg_from_dst(ir_to_mesa_dst_reg reg)
{
   return ir_to_mesa_src_reg(reg.file, reg.index, NULL);
}

/**
 * Emits Mesa scalar opcodes to produce unique answers across channels.
 *
 * Some Mesa opcodes are scalar-only, like ARB_fp/vp.  The src X
 * channel determines the result across all channels.  So to do a vec4
 * of this operation, we want to emit a scalar per source channel used
 * to produce dest channels.
 */
void
ir_to_mesa_visitor::ir_to_mesa_emit_scalar_op2(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg orig_src0,
					       ir_to_mesa_src_reg orig_src1)
{
   int i, j;
   int done_mask = ~dst.writemask;

   /* Mesa RCP is a scalar operation splatting results to all channels,
    * like ARB_fp/vp.  So emit as many RCPs as necessary to cover our
    * dst channels.
    */
   for (i = 0; i < 4; i++) {
      GLuint this_mask = (1 << i);
      ir_to_mesa_instruction *inst;
      ir_to_mesa_src_reg src0 = orig_src0;
      ir_to_mesa_src_reg src1 = orig_src1;

      if (done_mask & this_mask)
	 continue;

      GLuint src0_swiz = GET_SWZ(src0.swizzle, i);
      GLuint src1_swiz = GET_SWZ(src1.swizzle, i);
      for (j = i + 1; j < 4; j++) {
	 /* If there is another enabled component in the destination that is
	  * derived from the same inputs, generate its value on this pass as
	  * well.
	  */
	 if (!(done_mask & (1 << j)) &&
	     GET_SWZ(src0.swizzle, j) == src0_swiz &&
	     GET_SWZ(src1.swizzle, j) == src1_swiz) {
	    this_mask |= (1 << j);
	 }
      }
      src0.swizzle = MAKE_SWIZZLE4(src0_swiz, src0_swiz,
				   src0_swiz, src0_swiz);
      src1.swizzle = MAKE_SWIZZLE4(src1_swiz, src1_swiz,
				  src1_swiz, src1_swiz);

      inst = ir_to_mesa_emit_op2(ir, op,
				 dst,
				 src0,
				 src1);
      inst->dst_reg.writemask = this_mask;
      done_mask |= this_mask;
   }
}

void
ir_to_mesa_visitor::ir_to_mesa_emit_scalar_op1(ir_instruction *ir,
					       enum prog_opcode op,
					       ir_to_mesa_dst_reg dst,
					       ir_to_mesa_src_reg src0)
{
   ir_to_mesa_src_reg undef = ir_to_mesa_undef;

   undef.swizzle = SWIZZLE_XXXX;

   ir_to_mesa_emit_scalar_op2(ir, op, dst, src0, undef);
}

/**
 * Emit an OPCODE_SCS instruction
 *
 * The \c SCS opcode functions a bit differently than the other Mesa (or
 * ARB_fragment_program) opcodes.  Instead of splatting its result across all
 * four components of the destination, it writes one value to the \c x
 * component and another value to the \c y component.
 *
 * \param ir        IR instruction being processed
 * \param op        Either \c OPCODE_SIN or \c OPCODE_COS depending on which
 *                  value is desired.
 * \param dst       Destination register
 * \param src       Source register
 */
void
ir_to_mesa_visitor::emit_scs(ir_instruction *ir, enum prog_opcode op,
			     ir_to_mesa_dst_reg dst,
			     const ir_to_mesa_src_reg &src)
{
   /* Vertex programs cannot use the SCS opcode.
    */
   if (this->prog->Target == GL_VERTEX_PROGRAM_ARB) {
      ir_to_mesa_emit_scalar_op1(ir, op, dst, src);
      return;
   }

   const unsigned component = (op == OPCODE_SIN) ? 0 : 1;
   const unsigned scs_mask = (1U << component);
   int done_mask = ~dst.writemask;
   ir_to_mesa_src_reg tmp;

   assert(op == OPCODE_SIN || op == OPCODE_COS);

   /* If there are compnents in the destination that differ from the component
    * that will be written by the SCS instrution, we'll need a temporary.
    */
   if (scs_mask != unsigned(dst.writemask)) {
      tmp = get_temp(glsl_type::vec4_type);
   }

   for (unsigned i = 0; i < 4; i++) {
      unsigned this_mask = (1U << i);
      ir_to_mesa_src_reg src0 = src;

      if ((done_mask & this_mask) != 0)
	 continue;

      /* The source swizzle specified which component of the source generates
       * sine / cosine for the current component in the destination.  The SCS
       * instruction requires that this value be swizzle to the X component.
       * Replace the current swizzle with a swizzle that puts the source in
       * the X component.
       */
      unsigned src0_swiz = GET_SWZ(src.swizzle, i);

      src0.swizzle = MAKE_SWIZZLE4(src0_swiz, src0_swiz,
				   src0_swiz, src0_swiz);
      for (unsigned j = i + 1; j < 4; j++) {
	 /* If there is another enabled component in the destination that is
	  * derived from the same inputs, generate its value on this pass as
	  * well.
	  */
	 if (!(done_mask & (1 << j)) &&
	     GET_SWZ(src0.swizzle, j) == src0_swiz) {
	    this_mask |= (1 << j);
	 }
      }

      if (this_mask != scs_mask) {
	 ir_to_mesa_instruction *inst;
	 ir_to_mesa_dst_reg tmp_dst = ir_to_mesa_dst_reg_from_src(tmp);

	 /* Emit the SCS instruction.
	  */
	 inst = ir_to_mesa_emit_op1(ir, OPCODE_SCS, tmp_dst, src0);
	 inst->dst_reg.writemask = scs_mask;

	 /* Move the result of the SCS instruction to the desired location in
	  * the destination.
	  */
	 tmp.swizzle = MAKE_SWIZZLE4(component, component,
				     component, component);
	 inst = ir_to_mesa_emit_op1(ir, OPCODE_SCS, dst, tmp);
	 inst->dst_reg.writemask = this_mask;
      } else {
	 /* Emit the SCS instruction to write directly to the destination.
	  */
	 ir_to_mesa_instruction *inst =
	    ir_to_mesa_emit_op1(ir, OPCODE_SCS, dst, src0);
	 inst->dst_reg.writemask = scs_mask;
      }

      done_mask |= this_mask;
   }
}

struct ir_to_mesa_src_reg
ir_to_mesa_visitor::src_reg_for_float(float val)
{
   ir_to_mesa_src_reg src_reg(PROGRAM_CONSTANT, -1, NULL);

   src_reg.index = _mesa_add_unnamed_constant(this->prog->Parameters,
					      &val, 1, &src_reg.swizzle);

   return src_reg;
}

static int
type_size(const struct glsl_type *type)
{
   unsigned int i;
   int size;

   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      if (type->is_matrix()) {
	 return type->matrix_columns;
      } else {
	 /* Regardless of size of vector, it gets a vec4. This is bad
	  * packing for things like floats, but otherwise arrays become a
	  * mess.  Hopefully a later pass over the code can pack scalars
	  * down if appropriate.
	  */
	 return 1;
      }
   case GLSL_TYPE_ARRAY:
      return type_size(type->fields.array) * type->length;
   case GLSL_TYPE_STRUCT:
      size = 0;
      for (i = 0; i < type->length; i++) {
	 size += type_size(type->fields.structure[i].type);
      }
      return size;
   case GLSL_TYPE_SAMPLER:
      /* Samplers take up one slot in UNIFORMS[], but they're baked in
       * at link time.
       */
      return 1;
   default:
      assert(0);
      return 0;
   }
}

/**
 * In the initial pass of codegen, we assign temporary numbers to
 * intermediate results.  (not SSA -- variable assignments will reuse
 * storage).  Actual register allocation for the Mesa VM occurs in a
 * pass over the Mesa IR later.
 */
ir_to_mesa_src_reg
ir_to_mesa_visitor::get_temp(const glsl_type *type)
{
   ir_to_mesa_src_reg src_reg;
   int swizzle[4];
   int i;

   src_reg.file = PROGRAM_TEMPORARY;
   src_reg.index = next_temp;
   src_reg.reladdr = NULL;
   next_temp += type_size(type);

   if (type->is_array() || type->is_record()) {
      src_reg.swizzle = SWIZZLE_NOOP;
   } else {
      for (i = 0; i < type->vector_elements; i++)
	 swizzle[i] = i;
      for (; i < 4; i++)
	 swizzle[i] = type->vector_elements - 1;
      src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
				      swizzle[2], swizzle[3]);
   }
   src_reg.negate = 0;

   return src_reg;
}

variable_storage *
ir_to_mesa_visitor::find_variable_storage(ir_variable *var)
{
   
   variable_storage *entry;

   foreach_iter(exec_list_iterator, iter, this->variables) {
      entry = (variable_storage *)iter.get();

      if (entry->var == var)
	 return entry;
   }

   return NULL;
}

void
ir_to_mesa_visitor::visit(ir_variable *ir)
{
   if (strcmp(ir->name, "gl_FragCoord") == 0) {
      struct gl_fragment_program *fp = (struct gl_fragment_program *)this->prog;

      fp->OriginUpperLeft = ir->origin_upper_left;
      fp->PixelCenterInteger = ir->pixel_center_integer;
   }

   if (ir->mode == ir_var_uniform && strncmp(ir->name, "gl_", 3) == 0) {
      unsigned int i;
      const struct gl_builtin_uniform_desc *statevar;

      for (i = 0; _mesa_builtin_uniform_desc[i].name; i++) {
	 if (strcmp(ir->name, _mesa_builtin_uniform_desc[i].name) == 0)
	    break;
      }

      if (!_mesa_builtin_uniform_desc[i].name) {
	 fail_link(this->shader_program,
		   "Failed to find builtin uniform `%s'\n", ir->name);
	 return;
      }

      statevar = &_mesa_builtin_uniform_desc[i];

      int array_count;
      if (ir->type->is_array()) {
	 array_count = ir->type->length;
      } else {
	 array_count = 1;
      }

      /* Check if this statevar's setup in the STATE file exactly
       * matches how we'll want to reference it as a
       * struct/array/whatever.  If not, then we need to move it into
       * temporary storage and hope that it'll get copy-propagated
       * out.
       */
      for (i = 0; i < statevar->num_elements; i++) {
	 if (statevar->elements[i].swizzle != SWIZZLE_XYZW) {
	    break;
	 }
      }

      struct variable_storage *storage;
      ir_to_mesa_dst_reg dst;
      if (i == statevar->num_elements) {
	 /* We'll set the index later. */
	 storage = new(mem_ctx) variable_storage(ir, PROGRAM_STATE_VAR, -1);
	 this->variables.push_tail(storage);

	 dst = ir_to_mesa_undef_dst;
      } else {
	 storage = new(mem_ctx) variable_storage(ir, PROGRAM_TEMPORARY,
						 this->next_temp);
	 this->variables.push_tail(storage);
	 this->next_temp += type_size(ir->type);

	 dst = ir_to_mesa_dst_reg_from_src(ir_to_mesa_src_reg(PROGRAM_TEMPORARY,
							      storage->index,
							      NULL));
      }


      for (int a = 0; a < array_count; a++) {
	 for (unsigned int i = 0; i < statevar->num_elements; i++) {
	    struct gl_builtin_uniform_element *element = &statevar->elements[i];
	    int tokens[STATE_LENGTH];

	    memcpy(tokens, element->tokens, sizeof(element->tokens));
	    if (ir->type->is_array()) {
	       tokens[1] = a;
	    }

	    int index = _mesa_add_state_reference(this->prog->Parameters,
						  (gl_state_index *)tokens);

	    if (storage->file == PROGRAM_STATE_VAR) {
	       if (storage->index == -1) {
		  storage->index = index;
	       } else {
		  assert(index ==
                         (int)(storage->index + a * statevar->num_elements + i));
	       }
	    } else {
	       ir_to_mesa_src_reg src(PROGRAM_STATE_VAR, index, NULL);
	       src.swizzle = element->swizzle;
	       ir_to_mesa_emit_op1(ir, OPCODE_MOV, dst, src);
	       /* even a float takes up a whole vec4 reg in a struct/array. */
	       dst.index++;
	    }
	 }
      }
      if (storage->file == PROGRAM_TEMPORARY &&
	  dst.index != storage->index + type_size(ir->type)) {
	 fail_link(this->shader_program,
		   "failed to load builtin uniform `%s'  (%d/%d regs loaded)\n",
		   ir->name, dst.index - storage->index,
		   type_size(ir->type));
      }
   }
}

void
ir_to_mesa_visitor::visit(ir_loop *ir)
{
   ir_dereference_variable *counter = NULL;

   if (ir->counter != NULL)
      counter = new(ir) ir_dereference_variable(ir->counter);

   if (ir->from != NULL) {
      assert(ir->counter != NULL);

      ir_assignment *a = new(ir) ir_assignment(counter, ir->from, NULL);

      a->accept(this);
      delete a;
   }

   ir_to_mesa_emit_op0(NULL, OPCODE_BGNLOOP);

   if (ir->to) {
      ir_expression *e =
	 new(ir) ir_expression(ir->cmp, glsl_type::bool_type,
			       counter, ir->to);
      ir_if *if_stmt =  new(ir) ir_if(e);

      ir_loop_jump *brk = new(ir) ir_loop_jump(ir_loop_jump::jump_break);

      if_stmt->then_instructions.push_tail(brk);

      if_stmt->accept(this);

      delete if_stmt;
      delete e;
      delete brk;
   }

   visit_exec_list(&ir->body_instructions, this);

   if (ir->increment) {
      ir_expression *e =
	 new(ir) ir_expression(ir_binop_add, counter->type,
			       counter, ir->increment);

      ir_assignment *a = new(ir) ir_assignment(counter, e, NULL);

      a->accept(this);
      delete a;
      delete e;
   }

   ir_to_mesa_emit_op0(NULL, OPCODE_ENDLOOP);
}

void
ir_to_mesa_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      ir_to_mesa_emit_op0(NULL, OPCODE_BRK);
      break;
   case ir_loop_jump::jump_continue:
      ir_to_mesa_emit_op0(NULL, OPCODE_CONT);
      break;
   }
}


void
ir_to_mesa_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
}

void
ir_to_mesa_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined before we get to ir_to_mesa.
    */
   if (strcmp(ir->name, "main") == 0) {
      const ir_function_signature *sig;
      exec_list empty;

      sig = ir->matching_signature(&empty);

      assert(sig);

      foreach_iter(exec_list_iterator, iter, sig->body) {
	 ir_instruction *ir = (ir_instruction *)iter.get();

	 ir->accept(this);
      }
   }
}

GLboolean
ir_to_mesa_visitor::try_emit_mad(ir_expression *ir, int mul_operand)
{
   int nonmul_operand = 1 - mul_operand;
   ir_to_mesa_src_reg a, b, c;

   ir_expression *expr = ir->operands[mul_operand]->as_expression();
   if (!expr || expr->operation != ir_binop_mul)
      return false;

   expr->operands[0]->accept(this);
   a = this->result;
   expr->operands[1]->accept(this);
   b = this->result;
   ir->operands[nonmul_operand]->accept(this);
   c = this->result;

   this->result = get_temp(ir->type);
   ir_to_mesa_emit_op3(ir, OPCODE_MAD,
		       ir_to_mesa_dst_reg_from_src(this->result), a, b, c);

   return true;
}

GLboolean
ir_to_mesa_visitor::try_emit_sat(ir_expression *ir)
{
   /* Saturates were only introduced to vertex programs in
    * NV_vertex_program3, so don't give them to drivers in the VP.
    */
   if (this->prog->Target == GL_VERTEX_PROGRAM_ARB)
      return false;

   ir_rvalue *sat_src = ir->as_rvalue_to_saturate();
   if (!sat_src)
      return false;

   sat_src->accept(this);
   ir_to_mesa_src_reg src = this->result;

   this->result = get_temp(ir->type);
   ir_to_mesa_instruction *inst;
   inst = ir_to_mesa_emit_op1(ir, OPCODE_MOV,
			      ir_to_mesa_dst_reg_from_src(this->result),
			      src);
   inst->saturate = true;

   return true;
}

void
ir_to_mesa_visitor::reladdr_to_temp(ir_instruction *ir,
				    ir_to_mesa_src_reg *reg, int *num_reladdr)
{
   if (!reg->reladdr)
      return;

   ir_to_mesa_emit_op1(ir, OPCODE_ARL, ir_to_mesa_address_reg, *reg->reladdr);

   if (*num_reladdr != 1) {
      ir_to_mesa_src_reg temp = get_temp(glsl_type::vec4_type);

      ir_to_mesa_emit_op1(ir, OPCODE_MOV,
			  ir_to_mesa_dst_reg_from_src(temp), *reg);
      *reg = temp;
   }

   (*num_reladdr)--;
}

void
ir_to_mesa_visitor::emit_swz(ir_expression *ir)
{
   /* Assume that the vector operator is in a form compatible with OPCODE_SWZ.
    * This means that each of the operands is either an immediate value of -1,
    * 0, or 1, or is a component from one source register (possibly with
    * negation).
    */
   uint8_t components[4] = { 0 };
   bool negate[4] = { false };
   ir_variable *var = NULL;

   for (unsigned i = 0; i < ir->type->vector_elements; i++) {
      ir_rvalue *op = ir->operands[i];

      assert(op->type->is_scalar());

      while (op != NULL) {
	 switch (op->ir_type) {
	 case ir_type_constant: {

	    assert(op->type->is_scalar());

	    const ir_constant *const c = op->as_constant();
	    if (c->is_one()) {
	       components[i] = SWIZZLE_ONE;
	    } else if (c->is_zero()) {
	       components[i] = SWIZZLE_ZERO;
	    } else if (c->is_negative_one()) {
	       components[i] = SWIZZLE_ONE;
	       negate[i] = true;
	    } else {
	       assert(!"SWZ constant must be 0.0 or 1.0.");
	    }

	    op = NULL;
	    break;
	 }

	 case ir_type_dereference_variable: {
	    ir_dereference_variable *const deref =
	       (ir_dereference_variable *) op;

	    assert((var == NULL) || (deref->var == var));
	    components[i] = SWIZZLE_X;
	    var = deref->var;
	    op = NULL;
	    break;
	 }

	 case ir_type_expression: {
	    ir_expression *const expr = (ir_expression *) op;

	    assert(expr->operation == ir_unop_neg);
	    negate[i] = true;

	    op = expr->operands[0];
	    break;
	 }

	 case ir_type_swizzle: {
	    ir_swizzle *const swiz = (ir_swizzle *) op;

	    components[i] = swiz->mask.x;
	    op = swiz->val;
	    break;
	 }

	 default:
	    assert(!"Should not get here.");
	    return;
	 }
      }
   }

   assert(var != NULL);

   ir_dereference_variable *const deref =
      new(mem_ctx) ir_dereference_variable(var);

   this->result.file = PROGRAM_UNDEFINED;
   deref->accept(this);
   if (this->result.file == PROGRAM_UNDEFINED) {
      ir_print_visitor v;
      printf("Failed to get tree for expression operand:\n");
      deref->accept(&v);
      exit(1);
   }

   ir_to_mesa_src_reg src;

   src = this->result;
   src.swizzle = MAKE_SWIZZLE4(components[0],
			       components[1],
			       components[2],
			       components[3]);
   src.negate = ((unsigned(negate[0]) << 0)
		 | (unsigned(negate[1]) << 1)
		 | (unsigned(negate[2]) << 2)
		 | (unsigned(negate[3]) << 3));

   /* Storage for our result.  Ideally for an assignment we'd be using the
    * actual storage for the result here, instead.
    */
   const ir_to_mesa_src_reg result_src = get_temp(ir->type);
   ir_to_mesa_dst_reg result_dst = ir_to_mesa_dst_reg_from_src(result_src);

   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   ir_to_mesa_emit_op1(ir, OPCODE_SWZ, result_dst, src);
   this->result = result_src;
}

void
ir_to_mesa_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   struct ir_to_mesa_src_reg op[Elements(ir->operands)];
   struct ir_to_mesa_src_reg result_src;
   struct ir_to_mesa_dst_reg result_dst;

   /* Quick peephole: Emit OPCODE_MAD(a, b, c) instead of ADD(MUL(a, b), c)
    */
   if (ir->operation == ir_binop_add) {
      if (try_emit_mad(ir, 1))
	 return;
      if (try_emit_mad(ir, 0))
	 return;
   }
   if (try_emit_sat(ir))
      return;

   if (ir->operation == ir_quadop_vector) {
      this->emit_swz(ir);
      return;
   }

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      this->result.file = PROGRAM_UNDEFINED;
      ir->operands[operand]->accept(this);
      if (this->result.file == PROGRAM_UNDEFINED) {
	 ir_print_visitor v;
	 printf("Failed to get tree for expression operand:\n");
	 ir->operands[operand]->accept(&v);
	 exit(1);
      }
      op[operand] = this->result;

      /* Matrix expression operands should have been broken down to vector
       * operations already.
       */
      assert(!ir->operands[operand]->type->is_matrix());
   }

   int vector_elements = ir->operands[0]->type->vector_elements;
   if (ir->operands[1]) {
      vector_elements = MAX2(vector_elements,
			     ir->operands[1]->type->vector_elements);
   }

   this->result.file = PROGRAM_UNDEFINED;

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = get_temp(ir->type);
   /* convenience for the emit functions below. */
   result_dst = ir_to_mesa_dst_reg_from_src(result_src);
   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   switch (ir->operation) {
   case ir_unop_logic_not:
      ir_to_mesa_emit_op2(ir, OPCODE_SEQ, result_dst,
			  op[0], src_reg_for_float(0.0));
      break;
   case ir_unop_neg:
      op[0].negate = ~op[0].negate;
      result_src = op[0];
      break;
   case ir_unop_abs:
      ir_to_mesa_emit_op1(ir, OPCODE_ABS, result_dst, op[0]);
      break;
   case ir_unop_sign:
      ir_to_mesa_emit_op1(ir, OPCODE_SSG, result_dst, op[0]);
      break;
   case ir_unop_rcp:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RCP, result_dst, op[0]);
      break;

   case ir_unop_exp2:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_EX2, result_dst, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_log2:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_LG2, result_dst, op[0]);
      break;
   case ir_unop_sin:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_COS, result_dst, op[0]);
      break;
   case ir_unop_sin_reduced:
      emit_scs(ir, OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos_reduced:
      emit_scs(ir, OPCODE_COS, result_dst, op[0]);
      break;

   case ir_unop_dFdx:
      ir_to_mesa_emit_op1(ir, OPCODE_DDX, result_dst, op[0]);
      break;
   case ir_unop_dFdy:
      ir_to_mesa_emit_op1(ir, OPCODE_DDY, result_dst, op[0]);
      break;

   case ir_unop_noise: {
      const enum prog_opcode opcode =
	 prog_opcode(OPCODE_NOISE1
		     + (ir->operands[0]->type->vector_elements) - 1);
      assert((opcode >= OPCODE_NOISE1) && (opcode <= OPCODE_NOISE4));

      ir_to_mesa_emit_op1(ir, opcode, result_dst, op[0]);
      break;
   }

   case ir_binop_add:
      ir_to_mesa_emit_op2(ir, OPCODE_ADD, result_dst, op[0], op[1]);
      break;
   case ir_binop_sub:
      ir_to_mesa_emit_op2(ir, OPCODE_SUB, result_dst, op[0], op[1]);
      break;

   case ir_binop_mul:
      ir_to_mesa_emit_op2(ir, OPCODE_MUL, result_dst, op[0], op[1]);
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
      ir_to_mesa_emit_op2(ir, OPCODE_SLT, result_dst, op[0], op[1]);
      break;
   case ir_binop_greater:
      ir_to_mesa_emit_op2(ir, OPCODE_SGT, result_dst, op[0], op[1]);
      break;
   case ir_binop_lequal:
      ir_to_mesa_emit_op2(ir, OPCODE_SLE, result_dst, op[0], op[1]);
      break;
   case ir_binop_gequal:
      ir_to_mesa_emit_op2(ir, OPCODE_SGE, result_dst, op[0], op[1]);
      break;
   case ir_binop_equal:
      ir_to_mesa_emit_op2(ir, OPCODE_SEQ, result_dst, op[0], op[1]);
      break;
   case ir_binop_nequal:
      ir_to_mesa_emit_op2(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      break;
   case ir_binop_all_equal:
      /* "==" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 ir_to_mesa_src_reg temp = get_temp(glsl_type::vec4_type);
	 ir_to_mesa_emit_op2(ir, OPCODE_SNE,
			     ir_to_mesa_dst_reg_from_src(temp), op[0], op[1]);
	 ir_to_mesa_emit_dp(ir, result_dst, temp, temp, vector_elements);
	 ir_to_mesa_emit_op2(ir, OPCODE_SEQ,
			     result_dst, result_src, src_reg_for_float(0.0));
      } else {
	 ir_to_mesa_emit_op2(ir, OPCODE_SEQ, result_dst, op[0], op[1]);
      }
      break;
   case ir_binop_any_nequal:
      /* "!=" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
	  ir->operands[1]->type->is_vector()) {
	 ir_to_mesa_src_reg temp = get_temp(glsl_type::vec4_type);
	 ir_to_mesa_emit_op2(ir, OPCODE_SNE,
			     ir_to_mesa_dst_reg_from_src(temp), op[0], op[1]);
	 ir_to_mesa_emit_dp(ir, result_dst, temp, temp, vector_elements);
	 ir_to_mesa_emit_op2(ir, OPCODE_SNE,
			     result_dst, result_src, src_reg_for_float(0.0));
      } else {
	 ir_to_mesa_emit_op2(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      }
      break;

   case ir_unop_any:
      assert(ir->operands[0]->type->is_vector());
      ir_to_mesa_emit_dp(ir, result_dst, op[0], op[0],
			 ir->operands[0]->type->vector_elements);
      ir_to_mesa_emit_op2(ir, OPCODE_SNE,
			  result_dst, result_src, src_reg_for_float(0.0));
      break;

   case ir_binop_logic_xor:
      ir_to_mesa_emit_op2(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      /* This could be a saturated add and skip the SNE. */
      ir_to_mesa_emit_op2(ir, OPCODE_ADD,
			  result_dst,
			  op[0], op[1]);

      ir_to_mesa_emit_op2(ir, OPCODE_SNE,
			  result_dst,
			  result_src, src_reg_for_float(0.0));
      break;

   case ir_binop_logic_and:
      /* the bool args are stored as float 0.0 or 1.0, so "mul" gives us "and". */
      ir_to_mesa_emit_op2(ir, OPCODE_MUL,
			  result_dst,
			  op[0], op[1]);
      break;

   case ir_binop_dot:
      assert(ir->operands[0]->type->is_vector());
      assert(ir->operands[0]->type == ir->operands[1]->type);
      ir_to_mesa_emit_dp(ir, result_dst, op[0], op[1],
			 ir->operands[0]->type->vector_elements);
      break;

   case ir_unop_sqrt:
      /* sqrt(x) = x * rsq(x). */
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RSQ, result_dst, op[0]);
      ir_to_mesa_emit_op2(ir, OPCODE_MUL, result_dst, result_src, op[0]);
      /* For incoming channels <= 0, set the result to 0. */
      op[0].negate = ~op[0].negate;
      ir_to_mesa_emit_op3(ir, OPCODE_CMP, result_dst,
			  op[0], result_src, src_reg_for_float(0.0));
      break;
   case ir_unop_rsq:
      ir_to_mesa_emit_scalar_op1(ir, OPCODE_RSQ, result_dst, op[0]);
      break;
   case ir_unop_i2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
      /* Mesa IR lacks types, ints are stored as truncated floats. */
      result_src = op[0];
      break;
   case ir_unop_f2i:
      ir_to_mesa_emit_op1(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_f2b:
   case ir_unop_i2b:
      ir_to_mesa_emit_op2(ir, OPCODE_SNE, result_dst,
			  op[0], src_reg_for_float(0.0));
      break;
   case ir_unop_trunc:
      ir_to_mesa_emit_op1(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_ceil:
      op[0].negate = ~op[0].negate;
      ir_to_mesa_emit_op1(ir, OPCODE_FLR, result_dst, op[0]);
      result_src.negate = ~result_src.negate;
      break;
   case ir_unop_floor:
      ir_to_mesa_emit_op1(ir, OPCODE_FLR, result_dst, op[0]);
      break;
   case ir_unop_fract:
      ir_to_mesa_emit_op1(ir, OPCODE_FRC, result_dst, op[0]);
      break;

   case ir_binop_min:
      ir_to_mesa_emit_op2(ir, OPCODE_MIN, result_dst, op[0], op[1]);
      break;
   case ir_binop_max:
      ir_to_mesa_emit_op2(ir, OPCODE_MAX, result_dst, op[0], op[1]);
      break;
   case ir_binop_pow:
      ir_to_mesa_emit_scalar_op2(ir, OPCODE_POW, result_dst, op[0], op[1]);
      break;

   case ir_unop_bit_not:
   case ir_unop_u2f:
   case ir_binop_lshift:
   case ir_binop_rshift:
   case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
   case ir_unop_round_even:
      assert(!"GLSL 1.30 features unsupported");
      break;

   case ir_quadop_vector:
      /* This operation should have already been handled.
       */
      assert(!"Should not get here.");
      break;
   }

   this->result = result_src;
}


void
ir_to_mesa_visitor::visit(ir_swizzle *ir)
{
   ir_to_mesa_src_reg src_reg;
   int i;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   src_reg = this->result;
   assert(src_reg.file != PROGRAM_UNDEFINED);

   for (i = 0; i < 4; i++) {
      if (i < ir->type->vector_elements) {
	 switch (i) {
	 case 0:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.x);
	    break;
	 case 1:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.y);
	    break;
	 case 2:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.z);
	    break;
	 case 3:
	    swizzle[i] = GET_SWZ(src_reg.swizzle, ir->mask.w);
	    break;
	 }
      } else {
	 /* If the type is smaller than a vec4, replicate the last
	  * channel out.
	  */
	 swizzle[i] = swizzle[ir->type->vector_elements - 1];
      }
   }

   src_reg.swizzle = MAKE_SWIZZLE4(swizzle[0],
				   swizzle[1],
				   swizzle[2],
				   swizzle[3]);

   this->result = src_reg;
}

void
ir_to_mesa_visitor::visit(ir_dereference_variable *ir)
{
   variable_storage *entry = find_variable_storage(ir->var);

   if (!entry) {
      switch (ir->var->mode) {
      case ir_var_uniform:
	 entry = new(mem_ctx) variable_storage(ir->var, PROGRAM_UNIFORM,
					       ir->var->location);
	 this->variables.push_tail(entry);
	 break;
      case ir_var_in:
      case ir_var_out:
      case ir_var_inout:
	 /* The linker assigns locations for varyings and attributes,
	  * including deprecated builtins (like gl_Color), user-assign
	  * generic attributes (glBindVertexLocation), and
	  * user-defined varyings.
	  *
	  * FINISHME: We would hit this path for function arguments.  Fix!
	  */
	 assert(ir->var->location != -1);
	 if (ir->var->mode == ir_var_in ||
	     ir->var->mode == ir_var_inout) {
	    entry = new(mem_ctx) variable_storage(ir->var,
						  PROGRAM_INPUT,
						  ir->var->location);

	    if (this->prog->Target == GL_VERTEX_PROGRAM_ARB &&
		ir->var->location >= VERT_ATTRIB_GENERIC0) {
	       _mesa_add_attribute(prog->Attributes,
				   ir->var->name,
				   _mesa_sizeof_glsl_type(ir->var->type->gl_type),
				   ir->var->type->gl_type,
				   ir->var->location - VERT_ATTRIB_GENERIC0);
	    }
	 } else {
	    entry = new(mem_ctx) variable_storage(ir->var,
						  PROGRAM_OUTPUT,
						  ir->var->location);
	 }

	 break;
      case ir_var_auto:
      case ir_var_temporary:
	 entry = new(mem_ctx) variable_storage(ir->var, PROGRAM_TEMPORARY,
					       this->next_temp);
	 this->variables.push_tail(entry);

	 next_temp += type_size(ir->var->type);
	 break;
      }

      if (!entry) {
	 printf("Failed to make storage for %s\n", ir->var->name);
	 exit(1);
      }
   }

   this->result = ir_to_mesa_src_reg(entry->file, entry->index, ir->var->type);
}

void
ir_to_mesa_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *index;
   ir_to_mesa_src_reg src_reg;
   int element_size = type_size(ir->type);

   index = ir->array_index->constant_expression_value();

   ir->array->accept(this);
   src_reg = this->result;

   if (index) {
      src_reg.index += index->value.i[0] * element_size;
   } else {
      ir_to_mesa_src_reg array_base = this->result;
      /* Variable index array dereference.  It eats the "vec4" of the
       * base of the array and an index that offsets the Mesa register
       * index.
       */
      ir->array_index->accept(this);

      ir_to_mesa_src_reg index_reg;

      if (element_size == 1) {
	 index_reg = this->result;
      } else {
	 index_reg = get_temp(glsl_type::float_type);

	 ir_to_mesa_emit_op2(ir, OPCODE_MUL,
			     ir_to_mesa_dst_reg_from_src(index_reg),
			     this->result, src_reg_for_float(element_size));
      }

      src_reg.reladdr = talloc(mem_ctx, ir_to_mesa_src_reg);
      memcpy(src_reg.reladdr, &index_reg, sizeof(index_reg));
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector())
      src_reg.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      src_reg.swizzle = SWIZZLE_NOOP;

   this->result = src_reg;
}

void
ir_to_mesa_visitor::visit(ir_dereference_record *ir)
{
   unsigned int i;
   const glsl_type *struct_type = ir->record->type;
   int offset = 0;

   ir->record->accept(this);

   for (i = 0; i < struct_type->length; i++) {
      if (strcmp(struct_type->fields.structure[i].name, ir->field) == 0)
	 break;
      offset += type_size(struct_type->fields.structure[i].type);
   }
   this->result.swizzle = swizzle_for_size(ir->type->vector_elements);
   this->result.index += offset;
}

/**
 * We want to be careful in assignment setup to hit the actual storage
 * instead of potentially using a temporary like we might with the
 * ir_dereference handler.
 */
static struct ir_to_mesa_dst_reg
get_assignment_lhs(ir_dereference *ir, ir_to_mesa_visitor *v)
{
   /* The LHS must be a dereference.  If the LHS is a variable indexed array
    * access of a vector, it must be separated into a series conditional moves
    * before reaching this point (see ir_vec_index_to_cond_assign).
    */
   assert(ir->as_dereference());
   ir_dereference_array *deref_array = ir->as_dereference_array();
   if (deref_array) {
      assert(!deref_array->array->type->is_vector());
   }

   /* Use the rvalue deref handler for the most part.  We'll ignore
    * swizzles in it and write swizzles using writemask, though.
    */
   ir->accept(v);
   return ir_to_mesa_dst_reg_from_src(v->result);
}

/**
 * Process the condition of a conditional assignment
 *
 * Examines the condition of a conditional assignment to generate the optimal
 * first operand of a \c CMP instruction.  If the condition is a relational
 * operator with 0 (e.g., \c ir_binop_less), the value being compared will be
 * used as the source for the \c CMP instruction.  Otherwise the comparison
 * is processed to a boolean result, and the boolean result is used as the
 * operand to the CMP instruction.
 */
bool
ir_to_mesa_visitor::process_move_condition(ir_rvalue *ir)
{
   ir_rvalue *src_ir = ir;
   bool negate = true;
   bool switch_order = false;

   ir_expression *const expr = ir->as_expression();
   if ((expr != NULL) && (expr->get_num_operands() == 2)) {
      bool zero_on_left = false;

      if (expr->operands[0]->is_zero()) {
	 src_ir = expr->operands[1];
	 zero_on_left = true;
      } else if (expr->operands[1]->is_zero()) {
	 src_ir = expr->operands[0];
	 zero_on_left = false;
      }

      /*      a is -  0  +            -  0  +
       * (a <  0)  T  F  F  ( a < 0)  T  F  F
       * (0 <  a)  F  F  T  (-a < 0)  F  F  T
       * (a <= 0)  T  T  F  (-a < 0)  F  F  T  (swap order of other operands)
       * (0 <= a)  F  T  T  ( a < 0)  T  F  F  (swap order of other operands)
       * (a >  0)  F  F  T  (-a < 0)  F  F  T
       * (0 >  a)  T  F  F  ( a < 0)  T  F  F
       * (a >= 0)  F  T  T  ( a < 0)  T  F  F  (swap order of other operands)
       * (0 >= a)  T  T  F  (-a < 0)  F  F  T  (swap order of other operands)
       *
       * Note that exchanging the order of 0 and 'a' in the comparison simply
       * means that the value of 'a' should be negated.
       */
      if (src_ir != ir) {
	 switch (expr->operation) {
	 case ir_binop_less:
	    switch_order = false;
	    negate = zero_on_left;
	    break;

	 case ir_binop_greater:
	    switch_order = false;
	    negate = !zero_on_left;
	    break;

	 case ir_binop_lequal:
	    switch_order = true;
	    negate = !zero_on_left;
	    break;

	 case ir_binop_gequal:
	    switch_order = true;
	    negate = zero_on_left;
	    break;

	 default:
	    /* This isn't the right kind of comparison afterall, so make sure
	     * the whole condition is visited.
	     */
	    src_ir = ir;
	    break;
	 }
      }
   }

   src_ir->accept(this);

   /* We use the OPCODE_CMP (a < 0 ? b : c) for conditional moves, and the
    * condition we produced is 0.0 or 1.0.  By flipping the sign, we can
    * choose which value OPCODE_CMP produces without an extra instruction
    * computing the condition.
    */
   if (negate)
      this->result.negate = ~this->result.negate;

   return switch_order;
}

void
ir_to_mesa_visitor::visit(ir_assignment *ir)
{
   struct ir_to_mesa_dst_reg l;
   struct ir_to_mesa_src_reg r;
   int i;

   ir->rhs->accept(this);
   r = this->result;

   l = get_assignment_lhs(ir->lhs, this);

   /* FINISHME: This should really set to the correct maximal writemask for each
    * FINISHME: component written (in the loops below).  This case can only
    * FINISHME: occur for matrices, arrays, and structures.
    */
   if (ir->write_mask == 0) {
      assert(!ir->lhs->type->is_scalar() && !ir->lhs->type->is_vector());
      l.writemask = WRITEMASK_XYZW;
   } else if (ir->lhs->type->is_scalar()) {
      /* FINISHME: This hack makes writing to gl_FragDepth, which lives in the
       * FINISHME: W component of fragment shader output zero, work correctly.
       */
      l.writemask = WRITEMASK_XYZW;
   } else {
      int swizzles[4];
      int first_enabled_chan = 0;
      int rhs_chan = 0;

      assert(ir->lhs->type->is_vector());
      l.writemask = ir->write_mask;

      for (int i = 0; i < 4; i++) {
	 if (l.writemask & (1 << i)) {
	    first_enabled_chan = GET_SWZ(r.swizzle, i);
	    break;
	 }
      }

      /* Swizzle a small RHS vector into the channels being written.
       *
       * glsl ir treats write_mask as dictating how many channels are
       * present on the RHS while Mesa IR treats write_mask as just
       * showing which channels of the vec4 RHS get written.
       */
      for (int i = 0; i < 4; i++) {
	 if (l.writemask & (1 << i))
	    swizzles[i] = GET_SWZ(r.swizzle, rhs_chan++);
	 else
	    swizzles[i] = first_enabled_chan;
      }
      r.swizzle = MAKE_SWIZZLE4(swizzles[0], swizzles[1],
				swizzles[2], swizzles[3]);
   }

   assert(l.file != PROGRAM_UNDEFINED);
   assert(r.file != PROGRAM_UNDEFINED);

   if (ir->condition) {
      const bool switch_order = this->process_move_condition(ir->condition);
      ir_to_mesa_src_reg condition = this->result;

      for (i = 0; i < type_size(ir->lhs->type); i++) {
	 if (switch_order) {
	    ir_to_mesa_emit_op3(ir, OPCODE_CMP, l,
				condition, ir_to_mesa_src_reg_from_dst(l), r);
	 } else {
	    ir_to_mesa_emit_op3(ir, OPCODE_CMP, l,
				condition, r, ir_to_mesa_src_reg_from_dst(l));
	 }

	 l.index++;
	 r.index++;
      }
   } else {
      for (i = 0; i < type_size(ir->lhs->type); i++) {
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	 l.index++;
	 r.index++;
      }
   }
}


void
ir_to_mesa_visitor::visit(ir_constant *ir)
{
   ir_to_mesa_src_reg src_reg;
   GLfloat stack_vals[4] = { 0 };
   GLfloat *values = stack_vals;
   unsigned int i;

   /* Unfortunately, 4 floats is all we can get into
    * _mesa_add_unnamed_constant.  So, make a temp to store an
    * aggregate constant and move each constant value into it.  If we
    * get lucky, copy propagation will eliminate the extra moves.
    */

   if (ir->type->base_type == GLSL_TYPE_STRUCT) {
      ir_to_mesa_src_reg temp_base = get_temp(ir->type);
      ir_to_mesa_dst_reg temp = ir_to_mesa_dst_reg_from_src(temp_base);

      foreach_iter(exec_list_iterator, iter, ir->components) {
	 ir_constant *field_value = (ir_constant *)iter.get();
	 int size = type_size(field_value->type);

	 assert(size > 0);

	 field_value->accept(this);
	 src_reg = this->result;

	 for (i = 0; i < (unsigned int)size; i++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, temp, src_reg);

	    src_reg.index++;
	    temp.index++;
	 }
      }
      this->result = temp_base;
      return;
   }

   if (ir->type->is_array()) {
      ir_to_mesa_src_reg temp_base = get_temp(ir->type);
      ir_to_mesa_dst_reg temp = ir_to_mesa_dst_reg_from_src(temp_base);
      int size = type_size(ir->type->fields.array);

      assert(size > 0);

      for (i = 0; i < ir->type->length; i++) {
	 ir->array_elements[i]->accept(this);
	 src_reg = this->result;
	 for (int j = 0; j < size; j++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, temp, src_reg);

	    src_reg.index++;
	    temp.index++;
	 }
      }
      this->result = temp_base;
      return;
   }

   if (ir->type->is_matrix()) {
      ir_to_mesa_src_reg mat = get_temp(ir->type);
      ir_to_mesa_dst_reg mat_column = ir_to_mesa_dst_reg_from_src(mat);

      for (i = 0; i < ir->type->matrix_columns; i++) {
	 assert(ir->type->base_type == GLSL_TYPE_FLOAT);
	 values = &ir->value.f[i * ir->type->vector_elements];

	 src_reg = ir_to_mesa_src_reg(PROGRAM_CONSTANT, -1, NULL);
	 src_reg.index = _mesa_add_unnamed_constant(this->prog->Parameters,
						values,
						ir->type->vector_elements,
						&src_reg.swizzle);
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, mat_column, src_reg);

	 mat_column.index++;
      }

      this->result = mat;
      return;
   }

   src_reg.file = PROGRAM_CONSTANT;
   switch (ir->type->base_type) {
   case GLSL_TYPE_FLOAT:
      values = &ir->value.f[0];
      break;
   case GLSL_TYPE_UINT:
      for (i = 0; i < ir->type->vector_elements; i++) {
	 values[i] = ir->value.u[i];
      }
      break;
   case GLSL_TYPE_INT:
      for (i = 0; i < ir->type->vector_elements; i++) {
	 values[i] = ir->value.i[i];
      }
      break;
   case GLSL_TYPE_BOOL:
      for (i = 0; i < ir->type->vector_elements; i++) {
	 values[i] = ir->value.b[i];
      }
      break;
   default:
      assert(!"Non-float/uint/int/bool constant");
   }

   this->result = ir_to_mesa_src_reg(PROGRAM_CONSTANT, -1, ir->type);
   this->result.index = _mesa_add_unnamed_constant(this->prog->Parameters,
						   values,
						   ir->type->vector_elements,
						   &this->result.swizzle);
}

function_entry *
ir_to_mesa_visitor::get_function_signature(ir_function_signature *sig)
{
   function_entry *entry;

   foreach_iter(exec_list_iterator, iter, this->function_signatures) {
      entry = (function_entry *)iter.get();

      if (entry->sig == sig)
	 return entry;
   }

   entry = talloc(mem_ctx, function_entry);
   entry->sig = sig;
   entry->sig_id = this->next_signature_id++;
   entry->bgn_inst = NULL;

   /* Allocate storage for all the parameters. */
   foreach_iter(exec_list_iterator, iter, sig->parameters) {
      ir_variable *param = (ir_variable *)iter.get();
      variable_storage *storage;

      storage = find_variable_storage(param);
      assert(!storage);

      storage = new(mem_ctx) variable_storage(param, PROGRAM_TEMPORARY,
					      this->next_temp);
      this->variables.push_tail(storage);

      this->next_temp += type_size(param->type);
   }

   if (!sig->return_type->is_void()) {
      entry->return_reg = get_temp(sig->return_type);
   } else {
      entry->return_reg = ir_to_mesa_undef;
   }

   this->function_signatures.push_tail(entry);
   return entry;
}

void
ir_to_mesa_visitor::visit(ir_call *ir)
{
   ir_to_mesa_instruction *call_inst;
   ir_function_signature *sig = ir->get_callee();
   function_entry *entry = get_function_signature(sig);
   int i;

   /* Process in parameters. */
   exec_list_iterator sig_iter = sig->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param_rval = (ir_rvalue *)iter.get();
      ir_variable *param = (ir_variable *)sig_iter.get();

      if (param->mode == ir_var_in ||
	  param->mode == ir_var_inout) {
	 variable_storage *storage = find_variable_storage(param);
	 assert(storage);

	 param_rval->accept(this);
	 ir_to_mesa_src_reg r = this->result;

	 ir_to_mesa_dst_reg l;
	 l.file = storage->file;
	 l.index = storage->index;
	 l.reladdr = NULL;
	 l.writemask = WRITEMASK_XYZW;
	 l.cond_mask = COND_TR;

	 for (i = 0; i < type_size(param->type); i++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	    l.index++;
	    r.index++;
	 }
      }

      sig_iter.next();
   }
   assert(!sig_iter.has_next());

   /* Emit call instruction */
   call_inst = ir_to_mesa_emit_op1(ir, OPCODE_CAL,
				   ir_to_mesa_undef_dst, ir_to_mesa_undef);
   call_inst->function = entry;

   /* Process out parameters. */
   sig_iter = sig->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param_rval = (ir_rvalue *)iter.get();
      ir_variable *param = (ir_variable *)sig_iter.get();

      if (param->mode == ir_var_out ||
	  param->mode == ir_var_inout) {
	 variable_storage *storage = find_variable_storage(param);
	 assert(storage);

	 ir_to_mesa_src_reg r;
	 r.file = storage->file;
	 r.index = storage->index;
	 r.reladdr = NULL;
	 r.swizzle = SWIZZLE_NOOP;
	 r.negate = 0;

	 param_rval->accept(this);
	 ir_to_mesa_dst_reg l = ir_to_mesa_dst_reg_from_src(this->result);

	 for (i = 0; i < type_size(param->type); i++) {
	    ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	    l.index++;
	    r.index++;
	 }
      }

      sig_iter.next();
   }
   assert(!sig_iter.has_next());

   /* Process return value. */
   this->result = entry->return_reg;
}

void
ir_to_mesa_visitor::visit(ir_texture *ir)
{
   ir_to_mesa_src_reg result_src, coord, lod_info, projector;
   ir_to_mesa_dst_reg result_dst, coord_dst;
   ir_to_mesa_instruction *inst = NULL;
   prog_opcode opcode = OPCODE_NOP;

   ir->coordinate->accept(this);

   /* Put our coords in a temp.  We'll need to modify them for shadow,
    * projection, or LOD, so the only case we'd use it as is is if
    * we're doing plain old texturing.  Mesa IR optimization should
    * handle cleaning up our mess in that case.
    */
   coord = get_temp(glsl_type::vec4_type);
   coord_dst = ir_to_mesa_dst_reg_from_src(coord);
   ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst,
		       this->result);

   if (ir->projector) {
      ir->projector->accept(this);
      projector = this->result;
   }

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = get_temp(glsl_type::vec4_type);
   result_dst = ir_to_mesa_dst_reg_from_src(result_src);

   switch (ir->op) {
   case ir_tex:
      opcode = OPCODE_TEX;
      break;
   case ir_txb:
      opcode = OPCODE_TXB;
      ir->lod_info.bias->accept(this);
      lod_info = this->result;
      break;
   case ir_txl:
      opcode = OPCODE_TXL;
      ir->lod_info.lod->accept(this);
      lod_info = this->result;
      break;
   case ir_txd:
   case ir_txf:
      assert(!"GLSL 1.30 features unsupported");
      break;
   }

   if (ir->projector) {
      if (opcode == OPCODE_TEX) {
	 /* Slot the projector in as the last component of the coord. */
	 coord_dst.writemask = WRITEMASK_W;
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst, projector);
	 coord_dst.writemask = WRITEMASK_XYZW;
	 opcode = OPCODE_TXP;
      } else {
	 ir_to_mesa_src_reg coord_w = coord;
	 coord_w.swizzle = SWIZZLE_WWWW;

	 /* For the other TEX opcodes there's no projective version
	  * since the last slot is taken up by lod info.  Do the
	  * projective divide now.
	  */
	 coord_dst.writemask = WRITEMASK_W;
	 ir_to_mesa_emit_op1(ir, OPCODE_RCP, coord_dst, projector);

	 coord_dst.writemask = WRITEMASK_XYZ;
	 ir_to_mesa_emit_op2(ir, OPCODE_MUL, coord_dst, coord, coord_w);

	 coord_dst.writemask = WRITEMASK_XYZW;
	 coord.swizzle = SWIZZLE_XYZW;
      }
   }

   if (ir->shadow_comparitor) {
      /* Slot the shadow value in as the second to last component of the
       * coord.
       */
      ir->shadow_comparitor->accept(this);
      coord_dst.writemask = WRITEMASK_Z;
      ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst, this->result);
      coord_dst.writemask = WRITEMASK_XYZW;
   }

   if (opcode == OPCODE_TXL || opcode == OPCODE_TXB) {
      /* Mesa IR stores lod or lod bias in the last channel of the coords. */
      coord_dst.writemask = WRITEMASK_W;
      ir_to_mesa_emit_op1(ir, OPCODE_MOV, coord_dst, lod_info);
      coord_dst.writemask = WRITEMASK_XYZW;
   }

   inst = ir_to_mesa_emit_op1(ir, opcode, result_dst, coord);

   if (ir->shadow_comparitor)
      inst->tex_shadow = GL_TRUE;

   inst->sampler = _mesa_get_sampler_uniform_value(ir->sampler,
						   this->shader_program,
						   this->prog);

   const glsl_type *sampler_type = ir->sampler->type;

   switch (sampler_type->sampler_dimensionality) {
   case GLSL_SAMPLER_DIM_1D:
      inst->tex_target = (sampler_type->sampler_array)
	 ? TEXTURE_1D_ARRAY_INDEX : TEXTURE_1D_INDEX;
      break;
   case GLSL_SAMPLER_DIM_2D:
      inst->tex_target = (sampler_type->sampler_array)
	 ? TEXTURE_2D_ARRAY_INDEX : TEXTURE_2D_INDEX;
      break;
   case GLSL_SAMPLER_DIM_3D:
      inst->tex_target = TEXTURE_3D_INDEX;
      break;
   case GLSL_SAMPLER_DIM_CUBE:
      inst->tex_target = TEXTURE_CUBE_INDEX;
      break;
   case GLSL_SAMPLER_DIM_RECT:
      inst->tex_target = TEXTURE_RECT_INDEX;
      break;
   case GLSL_SAMPLER_DIM_BUF:
      assert(!"FINISHME: Implement ARB_texture_buffer_object");
      break;
   default:
      assert(!"Should not get here.");
   }

   this->result = result_src;
}

void
ir_to_mesa_visitor::visit(ir_return *ir)
{
   if (ir->get_value()) {
      ir_to_mesa_dst_reg l;
      int i;

      assert(current_function);

      ir->get_value()->accept(this);
      ir_to_mesa_src_reg r = this->result;

      l = ir_to_mesa_dst_reg_from_src(current_function->return_reg);

      for (i = 0; i < type_size(current_function->sig->return_type); i++) {
	 ir_to_mesa_emit_op1(ir, OPCODE_MOV, l, r);
	 l.index++;
	 r.index++;
      }
   }

   ir_to_mesa_emit_op0(ir, OPCODE_RET);
}

void
ir_to_mesa_visitor::visit(ir_discard *ir)
{
   struct gl_fragment_program *fp = (struct gl_fragment_program *)this->prog;

   assert(ir->condition == NULL); /* FINISHME */

   ir_to_mesa_emit_op0(ir, OPCODE_KIL_NV);
   fp->UsesKill = GL_TRUE;
}

void
ir_to_mesa_visitor::visit(ir_if *ir)
{
   ir_to_mesa_instruction *cond_inst, *if_inst, *else_inst = NULL;
   ir_to_mesa_instruction *prev_inst;

   prev_inst = (ir_to_mesa_instruction *)this->instructions.get_tail();

   ir->condition->accept(this);
   assert(this->result.file != PROGRAM_UNDEFINED);

   if (this->options->EmitCondCodes) {
      cond_inst = (ir_to_mesa_instruction *)this->instructions.get_tail();

      /* See if we actually generated any instruction for generating
       * the condition.  If not, then cook up a move to a temp so we
       * have something to set cond_update on.
       */
      if (cond_inst == prev_inst) {
	 ir_to_mesa_src_reg temp = get_temp(glsl_type::bool_type);
	 cond_inst = ir_to_mesa_emit_op1(ir->condition, OPCODE_MOV,
					 ir_to_mesa_dst_reg_from_src(temp),
					 result);
      }
      cond_inst->cond_update = GL_TRUE;

      if_inst = ir_to_mesa_emit_op0(ir->condition, OPCODE_IF);
      if_inst->dst_reg.cond_mask = COND_NE;
   } else {
      if_inst = ir_to_mesa_emit_op1(ir->condition,
				    OPCODE_IF, ir_to_mesa_undef_dst,
				    this->result);
   }

   this->instructions.push_tail(if_inst);

   visit_exec_list(&ir->then_instructions, this);

   if (!ir->else_instructions.is_empty()) {
      else_inst = ir_to_mesa_emit_op0(ir->condition, OPCODE_ELSE);
      visit_exec_list(&ir->else_instructions, this);
   }

   if_inst = ir_to_mesa_emit_op1(ir->condition, OPCODE_ENDIF,
				 ir_to_mesa_undef_dst, ir_to_mesa_undef);
}

ir_to_mesa_visitor::ir_to_mesa_visitor()
{
   result.file = PROGRAM_UNDEFINED;
   next_temp = 1;
   next_signature_id = 1;
   current_function = NULL;
   mem_ctx = talloc_new(NULL);
}

ir_to_mesa_visitor::~ir_to_mesa_visitor()
{
   talloc_free(mem_ctx);
}

static struct prog_src_register
mesa_src_reg_from_ir_src_reg(ir_to_mesa_src_reg reg)
{
   struct prog_src_register mesa_reg;

   mesa_reg.File = reg.file;
   assert(reg.index < (1 << INST_INDEX_BITS));
   mesa_reg.Index = reg.index;
   mesa_reg.Swizzle = reg.swizzle;
   mesa_reg.RelAddr = reg.reladdr != NULL;
   mesa_reg.Negate = reg.negate;
   mesa_reg.Abs = 0;
   mesa_reg.HasIndex2 = GL_FALSE;
   mesa_reg.RelAddr2 = 0;
   mesa_reg.Index2 = 0;

   return mesa_reg;
}

static void
set_branchtargets(ir_to_mesa_visitor *v,
		  struct prog_instruction *mesa_instructions,
		  int num_instructions)
{
   int if_count = 0, loop_count = 0;
   int *if_stack, *loop_stack;
   int if_stack_pos = 0, loop_stack_pos = 0;
   int i, j;

   for (i = 0; i < num_instructions; i++) {
      switch (mesa_instructions[i].Opcode) {
      case OPCODE_IF:
	 if_count++;
	 break;
      case OPCODE_BGNLOOP:
	 loop_count++;
	 break;
      case OPCODE_BRK:
      case OPCODE_CONT:
	 mesa_instructions[i].BranchTarget = -1;
	 break;
      default:
	 break;
      }
   }

   if_stack = talloc_zero_array(v->mem_ctx, int, if_count);
   loop_stack = talloc_zero_array(v->mem_ctx, int, loop_count);

   for (i = 0; i < num_instructions; i++) {
      switch (mesa_instructions[i].Opcode) {
      case OPCODE_IF:
	 if_stack[if_stack_pos] = i;
	 if_stack_pos++;
	 break;
      case OPCODE_ELSE:
	 mesa_instructions[if_stack[if_stack_pos - 1]].BranchTarget = i;
	 if_stack[if_stack_pos - 1] = i;
	 break;
      case OPCODE_ENDIF:
	 mesa_instructions[if_stack[if_stack_pos - 1]].BranchTarget = i;
	 if_stack_pos--;
	 break;
      case OPCODE_BGNLOOP:
	 loop_stack[loop_stack_pos] = i;
	 loop_stack_pos++;
	 break;
      case OPCODE_ENDLOOP:
	 loop_stack_pos--;
	 /* Rewrite any breaks/conts at this nesting level (haven't
	  * already had a BranchTarget assigned) to point to the end
	  * of the loop.
	  */
	 for (j = loop_stack[loop_stack_pos]; j < i; j++) {
	    if (mesa_instructions[j].Opcode == OPCODE_BRK ||
		mesa_instructions[j].Opcode == OPCODE_CONT) {
	       if (mesa_instructions[j].BranchTarget == -1) {
		  mesa_instructions[j].BranchTarget = i;
	       }
	    }
	 }
	 /* The loop ends point at each other. */
	 mesa_instructions[i].BranchTarget = loop_stack[loop_stack_pos];
	 mesa_instructions[loop_stack[loop_stack_pos]].BranchTarget = i;
	 break;
      case OPCODE_CAL:
	 foreach_iter(exec_list_iterator, iter, v->function_signatures) {
	    function_entry *entry = (function_entry *)iter.get();

	    if (entry->sig_id == mesa_instructions[i].BranchTarget) {
	       mesa_instructions[i].BranchTarget = entry->inst;
	       break;
	    }
	 }
	 break;
      default:
	 break;
      }
   }
}

static void
print_program(struct prog_instruction *mesa_instructions,
	      ir_instruction **mesa_instruction_annotation,
	      int num_instructions)
{
   ir_instruction *last_ir = NULL;
   int i;
   int indent = 0;

   for (i = 0; i < num_instructions; i++) {
      struct prog_instruction *mesa_inst = mesa_instructions + i;
      ir_instruction *ir = mesa_instruction_annotation[i];

      fprintf(stdout, "%3d: ", i);

      if (last_ir != ir && ir) {
	 int j;

	 for (j = 0; j < indent; j++) {
	    fprintf(stdout, " ");
	 }
	 ir->print();
	 printf("\n");
	 last_ir = ir;

	 fprintf(stdout, "     "); /* line number spacing. */
      }

      indent = _mesa_fprint_instruction_opt(stdout, mesa_inst, indent,
					    PROG_PRINT_DEBUG, NULL);
   }
}

static void
count_resources(struct gl_program *prog)
{
   unsigned int i;

   prog->SamplersUsed = 0;

   for (i = 0; i < prog->NumInstructions; i++) {
      struct prog_instruction *inst = &prog->Instructions[i];

      if (_mesa_is_tex_instruction(inst->Opcode)) {
	 prog->SamplerTargets[inst->TexSrcUnit] =
	    (gl_texture_index)inst->TexSrcTarget;
	 prog->SamplersUsed |= 1 << inst->TexSrcUnit;
	 if (inst->TexShadow) {
	    prog->ShadowSamplers |= 1 << inst->TexSrcUnit;
	 }
      }
   }

   _mesa_update_shader_textures_used(prog);
}

struct uniform_sort {
   struct gl_uniform *u;
   int pos;
};

/* The shader_program->Uniforms list is almost sorted in increasing
 * uniform->{Frag,Vert}Pos locations, but not quite when there are
 * uniforms shared between targets.  We need to add parameters in
 * increasing order for the targets.
 */
static int
sort_uniforms(const void *a, const void *b)
{
   struct uniform_sort *u1 = (struct uniform_sort *)a;
   struct uniform_sort *u2 = (struct uniform_sort *)b;

   return u1->pos - u2->pos;
}

/* Add the uniforms to the parameters.  The linker chose locations
 * in our parameters lists (which weren't created yet), which the
 * uniforms code will use to poke values into our parameters list
 * when uniforms are updated.
 */
static void
add_uniforms_to_parameters_list(struct gl_shader_program *shader_program,
				struct gl_shader *shader,
				struct gl_program *prog)
{
   unsigned int i;
   unsigned int next_sampler = 0, num_uniforms = 0;
   struct uniform_sort *sorted_uniforms;

   sorted_uniforms = talloc_array(NULL, struct uniform_sort,
				  shader_program->Uniforms->NumUniforms);

   for (i = 0; i < shader_program->Uniforms->NumUniforms; i++) {
      struct gl_uniform *uniform = shader_program->Uniforms->Uniforms + i;
      int parameter_index = -1;

      switch (shader->Type) {
      case GL_VERTEX_SHADER:
	 parameter_index = uniform->VertPos;
	 break;
      case GL_FRAGMENT_SHADER:
	 parameter_index = uniform->FragPos;
	 break;
      case GL_GEOMETRY_SHADER:
	 parameter_index = uniform->GeomPos;
	 break;
      }

      /* Only add uniforms used in our target. */
      if (parameter_index != -1) {
	 sorted_uniforms[num_uniforms].pos = parameter_index;
	 sorted_uniforms[num_uniforms].u = uniform;
	 num_uniforms++;
      }
   }

   qsort(sorted_uniforms, num_uniforms, sizeof(struct uniform_sort),
	 sort_uniforms);

   for (i = 0; i < num_uniforms; i++) {
      struct gl_uniform *uniform = sorted_uniforms[i].u;
      int parameter_index = sorted_uniforms[i].pos;
      const glsl_type *type = uniform->Type;
      unsigned int size;

      if (type->is_vector() ||
	  type->is_scalar()) {
	 size = type->vector_elements;
      } else {
	 size = type_size(type) * 4;
      }

      gl_register_file file;
      if (type->is_sampler() ||
	  (type->is_array() && type->fields.array->is_sampler())) {
	 file = PROGRAM_SAMPLER;
      } else {
	 file = PROGRAM_UNIFORM;
      }

      GLint index = _mesa_lookup_parameter_index(prog->Parameters, -1,
						 uniform->Name);

      if (index < 0) {
	 index = _mesa_add_parameter(prog->Parameters, file,
				     uniform->Name, size, type->gl_type,
				     NULL, NULL, 0x0);

	 /* Sampler uniform values are stored in prog->SamplerUnits,
	  * and the entry in that array is selected by this index we
	  * store in ParameterValues[].
	  */
	 if (file == PROGRAM_SAMPLER) {
	    for (unsigned int j = 0; j < size / 4; j++)
	       prog->Parameters->ParameterValues[index + j][0] = next_sampler++;
	 }

	 /* The location chosen in the Parameters list here (returned
	  * from _mesa_add_uniform) has to match what the linker chose.
	  */
	 if (index != parameter_index) {
	    fail_link(shader_program, "Allocation of uniform `%s' to target "
		      "failed (%d vs %d)\n",
		      uniform->Name, index, parameter_index);
	 }
      }
   }

   talloc_free(sorted_uniforms);
}

static void
set_uniform_initializer(struct gl_context *ctx, void *mem_ctx,
			struct gl_shader_program *shader_program,
			const char *name, const glsl_type *type,
			ir_constant *val)
{
   if (type->is_record()) {
      ir_constant *field_constant;

      field_constant = (ir_constant *)val->components.get_head();

      for (unsigned int i = 0; i < type->length; i++) {
	 const glsl_type *field_type = type->fields.structure[i].type;
	 const char *field_name = talloc_asprintf(mem_ctx, "%s.%s", name,
					    type->fields.structure[i].name);
	 set_uniform_initializer(ctx, mem_ctx, shader_program, field_name,
				 field_type, field_constant);
	 field_constant = (ir_constant *)field_constant->next;
      }
      return;
   }

   int loc = _mesa_get_uniform_location(ctx, shader_program, name);

   if (loc == -1) {
      fail_link(shader_program,
		"Couldn't find uniform for initializer %s\n", name);
      return;
   }

   for (unsigned int i = 0; i < (type->is_array() ? type->length : 1); i++) {
      ir_constant *element;
      const glsl_type *element_type;
      if (type->is_array()) {
	 element = val->array_elements[i];
	 element_type = type->fields.array;
      } else {
	 element = val;
	 element_type = type;
      }

      void *values;

      if (element_type->base_type == GLSL_TYPE_BOOL) {
	 int *conv = talloc_array(mem_ctx, int, element_type->components());
	 for (unsigned int j = 0; j < element_type->components(); j++) {
	    conv[j] = element->value.b[j];
	 }
	 values = (void *)conv;
	 element_type = glsl_type::get_instance(GLSL_TYPE_INT,
						element_type->vector_elements,
						1);
      } else {
	 values = &element->value;
      }

      if (element_type->is_matrix()) {
	 _mesa_uniform_matrix(ctx, shader_program,
			      element_type->matrix_columns,
			      element_type->vector_elements,
			      loc, 1, GL_FALSE, (GLfloat *)values);
	 loc += element_type->matrix_columns;
      } else {
	 _mesa_uniform(ctx, shader_program, loc, element_type->matrix_columns,
		       values, element_type->gl_type);
	 loc += type_size(element_type);
      }
   }
}

static void
set_uniform_initializers(struct gl_context *ctx,
			 struct gl_shader_program *shader_program)
{
   void *mem_ctx = NULL;

   for (unsigned int i = 0; i < MESA_SHADER_TYPES; i++) {
      struct gl_shader *shader = shader_program->_LinkedShaders[i];

      if (shader == NULL)
	 continue;

      foreach_iter(exec_list_iterator, iter, *shader->ir) {
	 ir_instruction *ir = (ir_instruction *)iter.get();
	 ir_variable *var = ir->as_variable();

	 if (!var || var->mode != ir_var_uniform || !var->constant_value)
	    continue;

	 if (!mem_ctx)
	    mem_ctx = talloc_new(NULL);

	 set_uniform_initializer(ctx, mem_ctx, shader_program, var->name,
				 var->type, var->constant_value);
      }
   }

   talloc_free(mem_ctx);
}


/**
 * Convert a shader's GLSL IR into a Mesa gl_program.
 */
static struct gl_program *
get_mesa_program(struct gl_context *ctx,
                 struct gl_shader_program *shader_program,
		 struct gl_shader *shader)
{
   ir_to_mesa_visitor v;
   struct prog_instruction *mesa_instructions, *mesa_inst;
   ir_instruction **mesa_instruction_annotation;
   int i;
   struct gl_program *prog;
   GLenum target;
   const char *target_string;
   GLboolean progress;
   struct gl_shader_compiler_options *options =
         &ctx->ShaderCompilerOptions[_mesa_shader_type_to_index(shader->Type)];

   switch (shader->Type) {
   case GL_VERTEX_SHADER:
      target = GL_VERTEX_PROGRAM_ARB;
      target_string = "vertex";
      break;
   case GL_FRAGMENT_SHADER:
      target = GL_FRAGMENT_PROGRAM_ARB;
      target_string = "fragment";
      break;
   default:
      assert(!"should not be reached");
      return NULL;
   }

   validate_ir_tree(shader->ir);

   prog = ctx->Driver.NewProgram(ctx, target, shader_program->Name);
   if (!prog)
      return NULL;
   prog->Parameters = _mesa_new_parameter_list();
   prog->Varying = _mesa_new_parameter_list();
   prog->Attributes = _mesa_new_parameter_list();
   v.ctx = ctx;
   v.prog = prog;
   v.shader_program = shader_program;
   v.options = options;

   add_uniforms_to_parameters_list(shader_program, shader, prog);

   /* Emit Mesa IR for main(). */
   visit_exec_list(shader->ir, &v);
   v.ir_to_mesa_emit_op0(NULL, OPCODE_END);

   /* Now emit bodies for any functions that were used. */
   do {
      progress = GL_FALSE;

      foreach_iter(exec_list_iterator, iter, v.function_signatures) {
	 function_entry *entry = (function_entry *)iter.get();

	 if (!entry->bgn_inst) {
	    v.current_function = entry;

	    entry->bgn_inst = v.ir_to_mesa_emit_op0(NULL, OPCODE_BGNSUB);
	    entry->bgn_inst->function = entry;

	    visit_exec_list(&entry->sig->body, &v);

	    ir_to_mesa_instruction *last;
	    last = (ir_to_mesa_instruction *)v.instructions.get_tail();
	    if (last->op != OPCODE_RET)
	       v.ir_to_mesa_emit_op0(NULL, OPCODE_RET);

	    ir_to_mesa_instruction *end;
	    end = v.ir_to_mesa_emit_op0(NULL, OPCODE_ENDSUB);
	    end->function = entry;

	    progress = GL_TRUE;
	 }
      }
   } while (progress);

   prog->NumTemporaries = v.next_temp;

   int num_instructions = 0;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      num_instructions++;
   }

   mesa_instructions =
      (struct prog_instruction *)calloc(num_instructions,
					sizeof(*mesa_instructions));
   mesa_instruction_annotation = talloc_array(v.mem_ctx, ir_instruction *,
					      num_instructions);

   /* Convert ir_mesa_instructions into prog_instructions.
    */
   mesa_inst = mesa_instructions;
   i = 0;
   foreach_iter(exec_list_iterator, iter, v.instructions) {
      const ir_to_mesa_instruction *inst = (ir_to_mesa_instruction *)iter.get();

      mesa_inst->Opcode = inst->op;
      mesa_inst->CondUpdate = inst->cond_update;
      if (inst->saturate)
	 mesa_inst->SaturateMode = SATURATE_ZERO_ONE;
      mesa_inst->DstReg.File = inst->dst_reg.file;
      mesa_inst->DstReg.Index = inst->dst_reg.index;
      mesa_inst->DstReg.CondMask = inst->dst_reg.cond_mask;
      mesa_inst->DstReg.WriteMask = inst->dst_reg.writemask;
      mesa_inst->DstReg.RelAddr = inst->dst_reg.reladdr != NULL;
      mesa_inst->SrcReg[0] = mesa_src_reg_from_ir_src_reg(inst->src_reg[0]);
      mesa_inst->SrcReg[1] = mesa_src_reg_from_ir_src_reg(inst->src_reg[1]);
      mesa_inst->SrcReg[2] = mesa_src_reg_from_ir_src_reg(inst->src_reg[2]);
      mesa_inst->TexSrcUnit = inst->sampler;
      mesa_inst->TexSrcTarget = inst->tex_target;
      mesa_inst->TexShadow = inst->tex_shadow;
      mesa_instruction_annotation[i] = inst->ir;

      /* Set IndirectRegisterFiles. */
      if (mesa_inst->DstReg.RelAddr)
         prog->IndirectRegisterFiles |= 1 << mesa_inst->DstReg.File;

      /* Update program's bitmask of indirectly accessed register files */
      for (unsigned src = 0; src < 3; src++)
         if (mesa_inst->SrcReg[src].RelAddr)
            prog->IndirectRegisterFiles |= 1 << mesa_inst->SrcReg[src].File;

      if (options->EmitNoIfs && mesa_inst->Opcode == OPCODE_IF) {
	 fail_link(shader_program, "Couldn't flatten if statement\n");
      }

      switch (mesa_inst->Opcode) {
      case OPCODE_BGNSUB:
	 inst->function->inst = i;
	 mesa_inst->Comment = strdup(inst->function->sig->function_name());
	 break;
      case OPCODE_ENDSUB:
	 mesa_inst->Comment = strdup(inst->function->sig->function_name());
	 break;
      case OPCODE_CAL:
	 mesa_inst->BranchTarget = inst->function->sig_id; /* rewritten later */
	 break;
      case OPCODE_ARL:
	 prog->NumAddressRegs = 1;
	 break;
      default:
	 break;
      }

      mesa_inst++;
      i++;

      if (!shader_program->LinkStatus)
         break;
   }

   if (!shader_program->LinkStatus) {
      free(mesa_instructions);
      _mesa_reference_program(ctx, &shader->Program, NULL);
      return NULL;
   }

   set_branchtargets(&v, mesa_instructions, num_instructions);

   if (ctx->Shader.Flags & GLSL_DUMP) {
      printf("\n");
      printf("GLSL IR for linked %s program %d:\n", target_string,
	     shader_program->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n");
      printf("\n");
      printf("Mesa IR for linked %s program %d:\n", target_string,
	     shader_program->Name);
      print_program(mesa_instructions, mesa_instruction_annotation,
		    num_instructions);
   }

   prog->Instructions = mesa_instructions;
   prog->NumInstructions = num_instructions;

   do_set_program_inouts(shader->ir, prog);
   count_resources(prog);

   _mesa_reference_program(ctx, &shader->Program, prog);

   if ((ctx->Shader.Flags & GLSL_NO_OPT) == 0) {
      _mesa_optimize_program(ctx, prog);
   }

   return prog;
}

extern "C" {

/**
 * Called via ctx->Driver.CompilerShader().
 * This is a no-op.
 * XXX can we remove the ctx->Driver.CompileShader() hook?
 */
GLboolean
_mesa_ir_compile_shader(struct gl_context *ctx, struct gl_shader *shader)
{
   assert(shader->CompileStatus);
   (void) ctx;

   return GL_TRUE;
}


/**
 * Link a shader.
 * Called via ctx->Driver.LinkShader()
 * This actually involves converting GLSL IR into Mesa gl_programs with
 * code lowering and other optimizations.
 */
GLboolean
_mesa_ir_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
{
   assert(prog->LinkStatus);

   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      bool progress;
      exec_list *ir = prog->_LinkedShaders[i]->ir;
      const struct gl_shader_compiler_options *options =
            &ctx->ShaderCompilerOptions[_mesa_shader_type_to_index(prog->_LinkedShaders[i]->Type)];

      do {
	 progress = false;

	 /* Lowering */
	 do_mat_op_to_vec(ir);
	 lower_instructions(ir, MOD_TO_FRACT | DIV_TO_MUL_RCP | EXP_TO_EXP2
			      | LOG_TO_LOG2);

	 progress = do_lower_jumps(ir, true, true, options->EmitNoMainReturn, options->EmitNoCont, options->EmitNoLoops) || progress;

	 progress = do_common_optimization(ir, true, options->MaxUnrollIterations) || progress;

	 progress = lower_quadop_vector(ir, true) || progress;

	 if (options->EmitNoIfs)
	    progress = do_if_to_cond_assign(ir) || progress;

	 if (options->EmitNoNoise)
	    progress = lower_noise(ir) || progress;

	 /* If there are forms of indirect addressing that the driver
	  * cannot handle, perform the lowering pass.
	  */
	 if (options->EmitNoIndirectInput || options->EmitNoIndirectOutput
	     || options->EmitNoIndirectTemp || options->EmitNoIndirectUniform)
	   progress =
	     lower_variable_index_to_cond_assign(ir,
						 options->EmitNoIndirectInput,
						 options->EmitNoIndirectOutput,
						 options->EmitNoIndirectTemp,
						 options->EmitNoIndirectUniform)
	     || progress;

	 progress = do_vec_index_to_cond_assign(ir) || progress;
      } while (progress);

      validate_ir_tree(ir);
   }

   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      struct gl_program *linked_prog;

      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      linked_prog = get_mesa_program(ctx, prog, prog->_LinkedShaders[i]);

      if (linked_prog) {
         bool ok = true;

         switch (prog->_LinkedShaders[i]->Type) {
         case GL_VERTEX_SHADER:
            _mesa_reference_vertprog(ctx, &prog->VertexProgram,
                                     (struct gl_vertex_program *)linked_prog);
            ok = ctx->Driver.ProgramStringNotify(ctx, GL_VERTEX_PROGRAM_ARB,
                                                 linked_prog);
            break;
         case GL_FRAGMENT_SHADER:
            _mesa_reference_fragprog(ctx, &prog->FragmentProgram,
                                     (struct gl_fragment_program *)linked_prog);
            ok = ctx->Driver.ProgramStringNotify(ctx, GL_FRAGMENT_PROGRAM_ARB,
                                                 linked_prog);
            break;
         }
         if (!ok) {
            return GL_FALSE;
         }
      }

      _mesa_reference_program(ctx, &linked_prog, NULL);
   }

   return GL_TRUE;
}


/**
 * Compile a GLSL shader.  Called via glCompileShader().
 */
void
_mesa_glsl_compile_shader(struct gl_context *ctx, struct gl_shader *shader)
{
   struct _mesa_glsl_parse_state *state =
      new(shader) _mesa_glsl_parse_state(ctx, shader->Type, shader);

   const char *source = shader->Source;
   /* Check if the user called glCompileShader without first calling
    * glShaderSource.  This should fail to compile, but not raise a GL_ERROR.
    */
   if (source == NULL) {
      shader->CompileStatus = GL_FALSE;
      return;
   }

   state->error = preprocess(state, &source, &state->info_log,
			     &ctx->Extensions, ctx->API);

   if (ctx->Shader.Flags & GLSL_DUMP) {
      printf("GLSL source for shader %d:\n", shader->Name);
      printf("%s\n", shader->Source);
   }

   if (!state->error) {
     _mesa_glsl_lexer_ctor(state, source);
     _mesa_glsl_parse(state);
     _mesa_glsl_lexer_dtor(state);
   }

   talloc_free(shader->ir);
   shader->ir = new(shader) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
      _mesa_ast_to_hir(shader->ir, state);

   if (!state->error && !shader->ir->is_empty()) {
      validate_ir_tree(shader->ir);

      /* Do some optimization at compile time to reduce shader IR size
       * and reduce later work if the same shader is linked multiple times
       */
      while (do_common_optimization(shader->ir, false, 32))
	 ;

      validate_ir_tree(shader->ir);
   }

   shader->symbols = state->symbols;

   shader->CompileStatus = !state->error;
   shader->InfoLog = state->info_log;
   shader->Version = state->language_version;
   memcpy(shader->builtins_to_link, state->builtins_to_link,
	  sizeof(shader->builtins_to_link[0]) * state->num_builtins_to_link);
   shader->num_builtins_to_link = state->num_builtins_to_link;

   if (ctx->Shader.Flags & GLSL_LOG) {
      _mesa_write_shader_to_file(shader);
   }

   if (ctx->Shader.Flags & GLSL_DUMP) {
      if (shader->CompileStatus) {
	 printf("GLSL IR for shader %d:\n", shader->Name);
	 _mesa_print_ir(shader->ir, NULL);
	 printf("\n\n");
      } else {
	 printf("GLSL shader %d failed to compile.\n", shader->Name);
      }
      if (shader->InfoLog && shader->InfoLog[0] != 0) {
	 printf("GLSL shader %d info log:\n", shader->Name);
	 printf("%s\n", shader->InfoLog);
      }
   }

   /* Retain any live IR, but trash the rest. */
   reparent_ir(shader->ir, shader->ir);

   talloc_free(state);

   if (shader->CompileStatus) {
      if (!ctx->Driver.CompileShader(ctx, shader))
	 shader->CompileStatus = GL_FALSE;
   }
}


/**
 * Link a GLSL shader program.  Called via glLinkProgram().
 */
void
_mesa_glsl_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
{
   unsigned int i;

   _mesa_clear_shader_program_data(ctx, prog);

   prog->LinkStatus = GL_TRUE;

   for (i = 0; i < prog->NumShaders; i++) {
      if (!prog->Shaders[i]->CompileStatus) {
	 fail_link(prog, "linking with uncompiled shader");
	 prog->LinkStatus = GL_FALSE;
      }
   }

   prog->Varying = _mesa_new_parameter_list();
   _mesa_reference_vertprog(ctx, &prog->VertexProgram, NULL);
   _mesa_reference_fragprog(ctx, &prog->FragmentProgram, NULL);

   if (prog->LinkStatus) {
      link_shaders(ctx, prog);
   }

   if (prog->LinkStatus) {
      if (!ctx->Driver.LinkShader(ctx, prog)) {
	 prog->LinkStatus = GL_FALSE;
      }
   }

   set_uniform_initializers(ctx, prog);

   if (ctx->Shader.Flags & GLSL_DUMP) {
      if (!prog->LinkStatus) {
	 printf("GLSL shader program %d failed to link\n", prog->Name);
      }

      if (prog->InfoLog && prog->InfoLog[0] != 0) {
	 printf("GLSL shader program %d info log:\n", prog->Name);
	 printf("%s\n", prog->InfoLog);
      }
   }
}

} /* extern "C" */
