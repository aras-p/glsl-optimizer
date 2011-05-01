/*
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008  VMware, Inc.   All Rights Reserved.
 * Copyright © 2010 Intel Corporation
 * Copyright © 2011 Bryan Cain
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
 * \file glsl_to_tgsi.cpp
 *
 * Translate GLSL IR to Mesa's gl_program representation and to TGSI.
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

#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "util/u_math.h"
#include "tgsi/tgsi_ureg.h"
#include "tgsi/tgsi_dump.h"
#include "st_context.h"
#include "st_program.h"
#include "st_glsl_to_tgsi.h"
#include "st_mesa_to_tgsi.h"

#define PROGRAM_ANY_CONST ((1 << PROGRAM_LOCAL_PARAM) |  \
                           (1 << PROGRAM_ENV_PARAM) |    \
                           (1 << PROGRAM_STATE_VAR) |    \
                           (1 << PROGRAM_NAMED_PARAM) |  \
                           (1 << PROGRAM_CONSTANT) |     \
                           (1 << PROGRAM_UNIFORM))
}

class st_src_reg;
class st_dst_reg;

static int swizzle_for_size(int size);

/**
 * This struct is a corresponding struct to Mesa prog_src_register, with
 * wider fields.
 */
class st_src_reg {
public:
   st_src_reg(gl_register_file file, int index, const glsl_type *type)
   {
      this->file = file;
      this->index = index;
      if (type && (type->is_scalar() || type->is_vector() || type->is_matrix()))
         this->swizzle = swizzle_for_size(type->vector_elements);
      else
         this->swizzle = SWIZZLE_XYZW;
      this->negate = 0;
      this->reladdr = NULL;
   }

   st_src_reg(gl_register_file file, int index)
   {
      this->file = file;
      this->index = index;
      this->swizzle = SWIZZLE_XYZW;
      this->negate = 0;
      this->reladdr = NULL;
   }

   st_src_reg()
   {
      this->file = PROGRAM_UNDEFINED;
      this->index = 0;
      this->swizzle = 0;
      this->negate = 0;
      this->reladdr = NULL;
   }

   explicit st_src_reg(st_dst_reg reg);

   gl_register_file file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   GLuint swizzle; /**< SWIZZLE_XYZWONEZERO swizzles from Mesa. */
   int negate; /**< NEGATE_XYZW mask from mesa */
   /** Register index should be offset by the integer in this reg. */
   st_src_reg *reladdr;
};

class st_dst_reg {
public:
   st_dst_reg(gl_register_file file, int writemask)
   {
      this->file = file;
      this->index = 0;
      this->writemask = writemask;
      this->cond_mask = COND_TR;
      this->reladdr = NULL;
   }

   st_dst_reg()
   {
      this->file = PROGRAM_UNDEFINED;
      this->index = 0;
      this->writemask = 0;
      this->cond_mask = COND_TR;
      this->reladdr = NULL;
   }

   explicit st_dst_reg(st_src_reg reg);

   gl_register_file file; /**< PROGRAM_* from Mesa */
   int index; /**< temporary index, VERT_ATTRIB_*, FRAG_ATTRIB_*, etc. */
   int writemask; /**< Bitfield of WRITEMASK_[XYZW] */
   GLuint cond_mask:4;
   /** Register index should be offset by the integer in this reg. */
   st_src_reg *reladdr;
};

st_src_reg::st_src_reg(st_dst_reg reg)
{
   this->file = reg.file;
   this->index = reg.index;
   this->swizzle = SWIZZLE_XYZW;
   this->negate = 0;
   this->reladdr = NULL;
}

st_dst_reg::st_dst_reg(st_src_reg reg)
{
   this->file = reg.file;
   this->index = reg.index;
   this->writemask = WRITEMASK_XYZW;
   this->cond_mask = COND_TR;
   this->reladdr = reg.reladdr;
}

class glsl_to_tgsi_instruction : public exec_node {
public:
   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   enum prog_opcode op;
   st_dst_reg dst;
   st_src_reg src[3];
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
   glsl_to_tgsi_instruction *bgn_inst;

   /**
    * Index of the first instruction of the function body in actual
    * Mesa IR.
    *
    * Set after convertion from glsl_to_tgsi_instruction to prog_instruction.
    */
   int inst;

   /** Storage for the return value. */
   st_src_reg return_reg;
};

class glsl_to_tgsi_visitor : public ir_visitor {
public:
   glsl_to_tgsi_visitor();
   ~glsl_to_tgsi_visitor();

   function_entry *current_function;

   struct gl_context *ctx;
   struct gl_program *prog;
   struct gl_shader_program *shader_program;
   struct gl_shader_compiler_options *options;

   int next_temp;

   int num_address_regs;
   int samplers_used;
   bool indirect_addr_temps;
   bool indirect_addr_consts;

   variable_storage *find_variable_storage(ir_variable *var);

   function_entry *get_function_signature(ir_function_signature *sig);

   st_src_reg get_temp(const glsl_type *type);
   void reladdr_to_temp(ir_instruction *ir, st_src_reg *reg, int *num_reladdr);

   st_src_reg st_src_reg_for_float(float val);

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

   st_src_reg result;

   /** List of variable_storage */
   exec_list variables;

   /** List of function_entry */
   exec_list function_signatures;
   int next_signature_id;

   /** List of glsl_to_tgsi_instruction */
   exec_list instructions;

   glsl_to_tgsi_instruction *emit(ir_instruction *ir, enum prog_opcode op);

   glsl_to_tgsi_instruction *emit(ir_instruction *ir, enum prog_opcode op,
        		        st_dst_reg dst, st_src_reg src0);

   glsl_to_tgsi_instruction *emit(ir_instruction *ir, enum prog_opcode op,
        		        st_dst_reg dst, st_src_reg src0, st_src_reg src1);

   glsl_to_tgsi_instruction *emit(ir_instruction *ir, enum prog_opcode op,
        		        st_dst_reg dst,
        		        st_src_reg src0, st_src_reg src1, st_src_reg src2);

   /**
    * Emit the correct dot-product instruction for the type of arguments
    */
   void emit_dp(ir_instruction *ir,
                st_dst_reg dst,
                st_src_reg src0,
                st_src_reg src1,
                unsigned elements);

   void emit_scalar(ir_instruction *ir, enum prog_opcode op,
        	    st_dst_reg dst, st_src_reg src0);

   void emit_scalar(ir_instruction *ir, enum prog_opcode op,
        	    st_dst_reg dst, st_src_reg src0, st_src_reg src1);

   void emit_scs(ir_instruction *ir, enum prog_opcode op,
        	 st_dst_reg dst, const st_src_reg &src);

   GLboolean try_emit_mad(ir_expression *ir,
        		  int mul_operand);
   GLboolean try_emit_sat(ir_expression *ir);

   void emit_swz(ir_expression *ir);

   bool process_move_condition(ir_rvalue *ir);

   void remove_output_reads(gl_register_file type);

   void rename_temp_register(int index, int new_index);
   int get_first_temp_read(int index);
   int get_first_temp_write(int index);
   int get_last_temp_read(int index);
   int get_last_temp_write(int index);

   void copy_propagate(void);
   void eliminate_dead_code(void);
   void merge_registers(void);
   void renumber_registers(void);

   void *mem_ctx;
};

static st_src_reg undef_src = st_src_reg(PROGRAM_UNDEFINED, 0, NULL);

static st_dst_reg undef_dst = st_dst_reg(PROGRAM_UNDEFINED, SWIZZLE_NOOP);

static st_dst_reg address_reg = st_dst_reg(PROGRAM_ADDRESS, WRITEMASK_X);

static void
fail_link(struct gl_shader_program *prog, const char *fmt, ...) PRINTFLIKE(2, 3);

static void
fail_link(struct gl_shader_program *prog, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   ralloc_vasprintf_append(&prog->InfoLog, fmt, args);
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

glsl_to_tgsi_instruction *
glsl_to_tgsi_visitor::emit(ir_instruction *ir, enum prog_opcode op,
        		 st_dst_reg dst,
        		 st_src_reg src0, st_src_reg src1, st_src_reg src2)
{
   glsl_to_tgsi_instruction *inst = new(mem_ctx) glsl_to_tgsi_instruction();
   int num_reladdr = 0, i;

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
      emit(ir, OPCODE_ARL, address_reg, *dst.reladdr);
      num_reladdr--;
   }
   assert(num_reladdr == 0);

   inst->op = op;
   inst->dst = dst;
   inst->src[0] = src0;
   inst->src[1] = src1;
   inst->src[2] = src2;
   inst->ir = ir;

   inst->function = NULL;
   
   if (op == OPCODE_ARL)
      this->num_address_regs = 1;
   
   /* Update indirect addressing status used by TGSI */
   if (dst.reladdr) {
      switch(dst.file) {
      case PROGRAM_TEMPORARY:
         this->indirect_addr_temps = true;
         break;
      case PROGRAM_LOCAL_PARAM:
      case PROGRAM_ENV_PARAM:
      case PROGRAM_STATE_VAR:
      case PROGRAM_NAMED_PARAM:
      case PROGRAM_CONSTANT:
      case PROGRAM_UNIFORM:
         this->indirect_addr_consts = true;
         break;
      default:
         break;
      }
   }
   else {
      for (i=0; i<3; i++) {
         if(inst->src[i].reladdr) {
            switch(dst.file) {
            case PROGRAM_TEMPORARY:
               this->indirect_addr_temps = true;
               break;
            case PROGRAM_LOCAL_PARAM:
            case PROGRAM_ENV_PARAM:
            case PROGRAM_STATE_VAR:
            case PROGRAM_NAMED_PARAM:
            case PROGRAM_CONSTANT:
            case PROGRAM_UNIFORM:
               this->indirect_addr_consts = true;
               break;
            default:
               break;
            }
         }
      }
   }

   this->instructions.push_tail(inst);
   
   return inst;
}


glsl_to_tgsi_instruction *
glsl_to_tgsi_visitor::emit(ir_instruction *ir, enum prog_opcode op,
        		 st_dst_reg dst, st_src_reg src0, st_src_reg src1)
{
   return emit(ir, op, dst, src0, src1, undef_src);
}

glsl_to_tgsi_instruction *
glsl_to_tgsi_visitor::emit(ir_instruction *ir, enum prog_opcode op,
        		 st_dst_reg dst, st_src_reg src0)
{
   assert(dst.writemask != 0);
   return emit(ir, op, dst, src0, undef_src, undef_src);
}

glsl_to_tgsi_instruction *
glsl_to_tgsi_visitor::emit(ir_instruction *ir, enum prog_opcode op)
{
   return emit(ir, op, undef_dst, undef_src, undef_src, undef_src);
}

void
glsl_to_tgsi_visitor::emit_dp(ir_instruction *ir,
        		    st_dst_reg dst, st_src_reg src0, st_src_reg src1,
        		    unsigned elements)
{
   static const gl_inst_opcode dot_opcodes[] = {
      OPCODE_DP2, OPCODE_DP3, OPCODE_DP4
   };

   emit(ir, dot_opcodes[elements - 2], dst, src0, src1);
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
glsl_to_tgsi_visitor::emit_scalar(ir_instruction *ir, enum prog_opcode op,
        		        st_dst_reg dst,
        			st_src_reg orig_src0, st_src_reg orig_src1)
{
   int i, j;
   int done_mask = ~dst.writemask;

   /* Mesa RCP is a scalar operation splatting results to all channels,
    * like ARB_fp/vp.  So emit as many RCPs as necessary to cover our
    * dst channels.
    */
   for (i = 0; i < 4; i++) {
      GLuint this_mask = (1 << i);
      glsl_to_tgsi_instruction *inst;
      st_src_reg src0 = orig_src0;
      st_src_reg src1 = orig_src1;

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

      inst = emit(ir, op, dst, src0, src1);
      inst->dst.writemask = this_mask;
      done_mask |= this_mask;
   }
}

void
glsl_to_tgsi_visitor::emit_scalar(ir_instruction *ir, enum prog_opcode op,
        		        st_dst_reg dst, st_src_reg src0)
{
   st_src_reg undef = undef_src;

   undef.swizzle = SWIZZLE_XXXX;

   emit_scalar(ir, op, dst, src0, undef);
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
glsl_to_tgsi_visitor::emit_scs(ir_instruction *ir, enum prog_opcode op,
        		     st_dst_reg dst,
        		     const st_src_reg &src)
{
   /* Vertex programs cannot use the SCS opcode.
    */
   if (this->prog->Target == GL_VERTEX_PROGRAM_ARB) {
      emit_scalar(ir, op, dst, src);
      return;
   }

   const unsigned component = (op == OPCODE_SIN) ? 0 : 1;
   const unsigned scs_mask = (1U << component);
   int done_mask = ~dst.writemask;
   st_src_reg tmp;

   assert(op == OPCODE_SIN || op == OPCODE_COS);

   /* If there are compnents in the destination that differ from the component
    * that will be written by the SCS instrution, we'll need a temporary.
    */
   if (scs_mask != unsigned(dst.writemask)) {
      tmp = get_temp(glsl_type::vec4_type);
   }

   for (unsigned i = 0; i < 4; i++) {
      unsigned this_mask = (1U << i);
      st_src_reg src0 = src;

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
         glsl_to_tgsi_instruction *inst;
         st_dst_reg tmp_dst = st_dst_reg(tmp);

         /* Emit the SCS instruction.
          */
         inst = emit(ir, OPCODE_SCS, tmp_dst, src0);
         inst->dst.writemask = scs_mask;

         /* Move the result of the SCS instruction to the desired location in
          * the destination.
          */
         tmp.swizzle = MAKE_SWIZZLE4(component, component,
        			     component, component);
         inst = emit(ir, OPCODE_SCS, dst, tmp);
         inst->dst.writemask = this_mask;
      } else {
         /* Emit the SCS instruction to write directly to the destination.
          */
         glsl_to_tgsi_instruction *inst = emit(ir, OPCODE_SCS, dst, src0);
         inst->dst.writemask = scs_mask;
      }

      done_mask |= this_mask;
   }
}

struct st_src_reg
glsl_to_tgsi_visitor::st_src_reg_for_float(float val)
{
   st_src_reg src(PROGRAM_CONSTANT, -1, NULL);

   src.index = _mesa_add_unnamed_constant(this->prog->Parameters,
        				  &val, 1, &src.swizzle);

   return src;
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
      assert(type->length > 0);
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
st_src_reg
glsl_to_tgsi_visitor::get_temp(const glsl_type *type)
{
   st_src_reg src;
   int swizzle[4];
   int i;

   src.file = PROGRAM_TEMPORARY;
   src.index = next_temp;
   src.reladdr = NULL;
   next_temp += type_size(type);

   if (type->is_array() || type->is_record()) {
      src.swizzle = SWIZZLE_NOOP;
   } else {
      for (i = 0; i < type->vector_elements; i++)
         swizzle[i] = i;
      for (; i < 4; i++)
         swizzle[i] = type->vector_elements - 1;
      src.swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1],
        			  swizzle[2], swizzle[3]);
   }
   src.negate = 0;

   return src;
}

variable_storage *
glsl_to_tgsi_visitor::find_variable_storage(ir_variable *var)
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
glsl_to_tgsi_visitor::visit(ir_variable *ir)
{
   if (strcmp(ir->name, "gl_FragCoord") == 0) {
      struct gl_fragment_program *fp = (struct gl_fragment_program *)this->prog;

      fp->OriginUpperLeft = ir->origin_upper_left;
      fp->PixelCenterInteger = ir->pixel_center_integer;

   } else if (strcmp(ir->name, "gl_FragDepth") == 0) {
      struct gl_fragment_program *fp = (struct gl_fragment_program *)this->prog;
      switch (ir->depth_layout) {
      case ir_depth_layout_none:
         fp->FragDepthLayout = FRAG_DEPTH_LAYOUT_NONE;
         break;
      case ir_depth_layout_any:
         fp->FragDepthLayout = FRAG_DEPTH_LAYOUT_ANY;
         break;
      case ir_depth_layout_greater:
         fp->FragDepthLayout = FRAG_DEPTH_LAYOUT_GREATER;
         break;
      case ir_depth_layout_less:
         fp->FragDepthLayout = FRAG_DEPTH_LAYOUT_LESS;
         break;
      case ir_depth_layout_unchanged:
         fp->FragDepthLayout = FRAG_DEPTH_LAYOUT_UNCHANGED;
         break;
      default:
         assert(0);
         break;
      }
   }

   if (ir->mode == ir_var_uniform && strncmp(ir->name, "gl_", 3) == 0) {
      unsigned int i;
      const ir_state_slot *const slots = ir->state_slots;
      assert(ir->state_slots != NULL);

      /* Check if this statevar's setup in the STATE file exactly
       * matches how we'll want to reference it as a
       * struct/array/whatever.  If not, then we need to move it into
       * temporary storage and hope that it'll get copy-propagated
       * out.
       */
      for (i = 0; i < ir->num_state_slots; i++) {
         if (slots[i].swizzle != SWIZZLE_XYZW) {
            break;
         }
      }

      struct variable_storage *storage;
      st_dst_reg dst;
      if (i == ir->num_state_slots) {
         /* We'll set the index later. */
         storage = new(mem_ctx) variable_storage(ir, PROGRAM_STATE_VAR, -1);
         this->variables.push_tail(storage);

         dst = undef_dst;
      } else {
         /* The variable_storage constructor allocates slots based on the size
          * of the type.  However, this had better match the number of state
          * elements that we're going to copy into the new temporary.
          */
         assert((int) ir->num_state_slots == type_size(ir->type));

         storage = new(mem_ctx) variable_storage(ir, PROGRAM_TEMPORARY,
        					 this->next_temp);
         this->variables.push_tail(storage);
         this->next_temp += type_size(ir->type);

         dst = st_dst_reg(st_src_reg(PROGRAM_TEMPORARY, storage->index, NULL));
      }


      for (unsigned int i = 0; i < ir->num_state_slots; i++) {
         int index = _mesa_add_state_reference(this->prog->Parameters,
        				       (gl_state_index *)slots[i].tokens);

         if (storage->file == PROGRAM_STATE_VAR) {
            if (storage->index == -1) {
               storage->index = index;
            } else {
               assert(index == storage->index + (int)i);
            }
         } else {
            st_src_reg src(PROGRAM_STATE_VAR, index, NULL);
            src.swizzle = slots[i].swizzle;
            emit(ir, OPCODE_MOV, dst, src);
            /* even a float takes up a whole vec4 reg in a struct/array. */
            dst.index++;
         }
      }

      if (storage->file == PROGRAM_TEMPORARY &&
          dst.index != storage->index + (int) ir->num_state_slots) {
         fail_link(this->shader_program,
        	   "failed to load builtin uniform `%s'  (%d/%d regs loaded)\n",
        	   ir->name, dst.index - storage->index,
        	   type_size(ir->type));
      }
   }
}

void
glsl_to_tgsi_visitor::visit(ir_loop *ir)
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

   emit(NULL, OPCODE_BGNLOOP);

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

   emit(NULL, OPCODE_ENDLOOP);
}

void
glsl_to_tgsi_visitor::visit(ir_loop_jump *ir)
{
   switch (ir->mode) {
   case ir_loop_jump::jump_break:
      emit(NULL, OPCODE_BRK);
      break;
   case ir_loop_jump::jump_continue:
      emit(NULL, OPCODE_CONT);
      break;
   }
}


void
glsl_to_tgsi_visitor::visit(ir_function_signature *ir)
{
   assert(0);
   (void)ir;
}

void
glsl_to_tgsi_visitor::visit(ir_function *ir)
{
   /* Ignore function bodies other than main() -- we shouldn't see calls to
    * them since they should all be inlined before we get to glsl_to_tgsi.
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
glsl_to_tgsi_visitor::try_emit_mad(ir_expression *ir, int mul_operand)
{
   int nonmul_operand = 1 - mul_operand;
   st_src_reg a, b, c;

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
   emit(ir, OPCODE_MAD, st_dst_reg(this->result), a, b, c);

   return true;
}

GLboolean
glsl_to_tgsi_visitor::try_emit_sat(ir_expression *ir)
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
   st_src_reg src = this->result;

   this->result = get_temp(ir->type);
   glsl_to_tgsi_instruction *inst;
   inst = emit(ir, OPCODE_MOV, st_dst_reg(this->result), src);
   inst->saturate = true;

   return true;
}

void
glsl_to_tgsi_visitor::reladdr_to_temp(ir_instruction *ir,
        			    st_src_reg *reg, int *num_reladdr)
{
   if (!reg->reladdr)
      return;

   emit(ir, OPCODE_ARL, address_reg, *reg->reladdr);

   if (*num_reladdr != 1) {
      st_src_reg temp = get_temp(glsl_type::vec4_type);

      emit(ir, OPCODE_MOV, st_dst_reg(temp), *reg);
      *reg = temp;
   }

   (*num_reladdr)--;
}

void
glsl_to_tgsi_visitor::emit_swz(ir_expression *ir)
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

   st_src_reg src;

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
   const st_src_reg result_src = get_temp(ir->type);
   st_dst_reg result_dst = st_dst_reg(result_src);

   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   emit(ir, OPCODE_SWZ, result_dst, src);
   this->result = result_src;
}

void
glsl_to_tgsi_visitor::visit(ir_expression *ir)
{
   unsigned int operand;
   st_src_reg op[Elements(ir->operands)];
   st_src_reg result_src;
   st_dst_reg result_dst;

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
   result_dst = st_dst_reg(result_src);
   /* Limit writes to the channels that will be used by result_src later.
    * This does limit this temp's use as a temporary for multi-instruction
    * sequences.
    */
   result_dst.writemask = (1 << ir->type->vector_elements) - 1;

   switch (ir->operation) {
   case ir_unop_logic_not:
      emit(ir, OPCODE_SEQ, result_dst, op[0], st_src_reg_for_float(0.0));
      break;
   case ir_unop_neg:
      op[0].negate = ~op[0].negate;
      result_src = op[0];
      break;
   case ir_unop_abs:
      emit(ir, OPCODE_ABS, result_dst, op[0]);
      break;
   case ir_unop_sign:
      emit(ir, OPCODE_SSG, result_dst, op[0]);
      break;
   case ir_unop_rcp:
      emit_scalar(ir, OPCODE_RCP, result_dst, op[0]);
      break;

   case ir_unop_exp2:
      emit_scalar(ir, OPCODE_EX2, result_dst, op[0]);
      break;
   case ir_unop_exp:
   case ir_unop_log:
      assert(!"not reached: should be handled by ir_explog_to_explog2");
      break;
   case ir_unop_log2:
      emit_scalar(ir, OPCODE_LG2, result_dst, op[0]);
      break;
   case ir_unop_sin:
      emit_scalar(ir, OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos:
      emit_scalar(ir, OPCODE_COS, result_dst, op[0]);
      break;
   case ir_unop_sin_reduced:
      emit_scs(ir, OPCODE_SIN, result_dst, op[0]);
      break;
   case ir_unop_cos_reduced:
      emit_scs(ir, OPCODE_COS, result_dst, op[0]);
      break;

   case ir_unop_dFdx:
      emit(ir, OPCODE_DDX, result_dst, op[0]);
      break;
   case ir_unop_dFdy:
      emit(ir, OPCODE_DDY, result_dst, op[0]);
      break;

   case ir_unop_noise: {
      const enum prog_opcode opcode =
         prog_opcode(OPCODE_NOISE1
        	     + (ir->operands[0]->type->vector_elements) - 1);
      assert((opcode >= OPCODE_NOISE1) && (opcode <= OPCODE_NOISE4));

      emit(ir, opcode, result_dst, op[0]);
      break;
   }

   case ir_binop_add:
      emit(ir, OPCODE_ADD, result_dst, op[0], op[1]);
      break;
   case ir_binop_sub:
      emit(ir, OPCODE_SUB, result_dst, op[0], op[1]);
      break;

   case ir_binop_mul:
      emit(ir, OPCODE_MUL, result_dst, op[0], op[1]);
      break;
   case ir_binop_div:
      assert(!"not reached: should be handled by ir_div_to_mul_rcp");
   case ir_binop_mod:
      assert(!"ir_binop_mod should have been converted to b * fract(a/b)");
      break;

   case ir_binop_less:
      emit(ir, OPCODE_SLT, result_dst, op[0], op[1]);
      break;
   case ir_binop_greater:
      emit(ir, OPCODE_SGT, result_dst, op[0], op[1]);
      break;
   case ir_binop_lequal:
      emit(ir, OPCODE_SLE, result_dst, op[0], op[1]);
      break;
   case ir_binop_gequal:
      emit(ir, OPCODE_SGE, result_dst, op[0], op[1]);
      break;
   case ir_binop_equal:
      emit(ir, OPCODE_SEQ, result_dst, op[0], op[1]);
      break;
   case ir_binop_nequal:
      emit(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      break;
   case ir_binop_all_equal:
      /* "==" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
          ir->operands[1]->type->is_vector()) {
         st_src_reg temp = get_temp(glsl_type::vec4_type);
         emit(ir, OPCODE_SNE, st_dst_reg(temp), op[0], op[1]);
         emit_dp(ir, result_dst, temp, temp, vector_elements);
         emit(ir, OPCODE_SEQ, result_dst, result_src, st_src_reg_for_float(0.0));
      } else {
         emit(ir, OPCODE_SEQ, result_dst, op[0], op[1]);
      }
      break;
   case ir_binop_any_nequal:
      /* "!=" operator producing a scalar boolean. */
      if (ir->operands[0]->type->is_vector() ||
          ir->operands[1]->type->is_vector()) {
         st_src_reg temp = get_temp(glsl_type::vec4_type);
         emit(ir, OPCODE_SNE, st_dst_reg(temp), op[0], op[1]);
         emit_dp(ir, result_dst, temp, temp, vector_elements);
         emit(ir, OPCODE_SNE, result_dst, result_src, st_src_reg_for_float(0.0));
      } else {
         emit(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      }
      break;

   case ir_unop_any:
      assert(ir->operands[0]->type->is_vector());
      emit_dp(ir, result_dst, op[0], op[0],
              ir->operands[0]->type->vector_elements);
      emit(ir, OPCODE_SNE, result_dst, result_src, st_src_reg_for_float(0.0));
      break;

   case ir_binop_logic_xor:
      emit(ir, OPCODE_SNE, result_dst, op[0], op[1]);
      break;

   case ir_binop_logic_or:
      /* This could be a saturated add and skip the SNE. */
      emit(ir, OPCODE_ADD, result_dst, op[0], op[1]);
      emit(ir, OPCODE_SNE, result_dst, result_src, st_src_reg_for_float(0.0));
      break;

   case ir_binop_logic_and:
      /* the bool args are stored as float 0.0 or 1.0, so "mul" gives us "and". */
      emit(ir, OPCODE_MUL, result_dst, op[0], op[1]);
      break;

   case ir_binop_dot:
      assert(ir->operands[0]->type->is_vector());
      assert(ir->operands[0]->type == ir->operands[1]->type);
      emit_dp(ir, result_dst, op[0], op[1],
              ir->operands[0]->type->vector_elements);
      break;

   case ir_unop_sqrt:
      /* sqrt(x) = x * rsq(x). */
      emit_scalar(ir, OPCODE_RSQ, result_dst, op[0]);
      emit(ir, OPCODE_MUL, result_dst, result_src, op[0]);
      /* For incoming channels <= 0, set the result to 0. */
      op[0].negate = ~op[0].negate;
      emit(ir, OPCODE_CMP, result_dst,
        		  op[0], result_src, st_src_reg_for_float(0.0));
      break;
   case ir_unop_rsq:
      emit_scalar(ir, OPCODE_RSQ, result_dst, op[0]);
      break;
   case ir_unop_i2f:
   case ir_unop_b2f:
   case ir_unop_b2i:
      /* Mesa IR lacks types, ints are stored as truncated floats. */
      result_src = op[0];
      break;
   case ir_unop_f2i:
      emit(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_f2b:
   case ir_unop_i2b:
      emit(ir, OPCODE_SNE, result_dst,
        		  op[0], st_src_reg_for_float(0.0));
      break;
   case ir_unop_trunc:
      emit(ir, OPCODE_TRUNC, result_dst, op[0]);
      break;
   case ir_unop_ceil:
      op[0].negate = ~op[0].negate;
      emit(ir, OPCODE_FLR, result_dst, op[0]);
      result_src.negate = ~result_src.negate;
      break;
   case ir_unop_floor:
      emit(ir, OPCODE_FLR, result_dst, op[0]);
      break;
   case ir_unop_fract:
      emit(ir, OPCODE_FRC, result_dst, op[0]);
      break;

   case ir_binop_min:
      emit(ir, OPCODE_MIN, result_dst, op[0], op[1]);
      break;
   case ir_binop_max:
      emit(ir, OPCODE_MAX, result_dst, op[0], op[1]);
      break;
   case ir_binop_pow:
      emit_scalar(ir, OPCODE_POW, result_dst, op[0], op[1]);
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
glsl_to_tgsi_visitor::visit(ir_swizzle *ir)
{
   st_src_reg src;
   int i;
   int swizzle[4];

   /* Note that this is only swizzles in expressions, not those on the left
    * hand side of an assignment, which do write masking.  See ir_assignment
    * for that.
    */

   ir->val->accept(this);
   src = this->result;
   assert(src.file != PROGRAM_UNDEFINED);

   for (i = 0; i < 4; i++) {
      if (i < ir->type->vector_elements) {
         switch (i) {
         case 0:
            swizzle[i] = GET_SWZ(src.swizzle, ir->mask.x);
            break;
         case 1:
            swizzle[i] = GET_SWZ(src.swizzle, ir->mask.y);
            break;
         case 2:
            swizzle[i] = GET_SWZ(src.swizzle, ir->mask.z);
            break;
         case 3:
            swizzle[i] = GET_SWZ(src.swizzle, ir->mask.w);
            break;
         }
      } else {
         /* If the type is smaller than a vec4, replicate the last
          * channel out.
          */
         swizzle[i] = swizzle[ir->type->vector_elements - 1];
      }
   }

   src.swizzle = MAKE_SWIZZLE4(swizzle[0], swizzle[1], swizzle[2], swizzle[3]);

   this->result = src;
}

void
glsl_to_tgsi_visitor::visit(ir_dereference_variable *ir)
{
   variable_storage *entry = find_variable_storage(ir->var);
   ir_variable *var = ir->var;

   if (!entry) {
      switch (var->mode) {
      case ir_var_uniform:
         entry = new(mem_ctx) variable_storage(var, PROGRAM_UNIFORM,
        				       var->location);
         this->variables.push_tail(entry);
         break;
      case ir_var_in:
      case ir_var_inout:
         /* The linker assigns locations for varyings and attributes,
          * including deprecated builtins (like gl_Color), user-assign
          * generic attributes (glBindVertexLocation), and
          * user-defined varyings.
          *
          * FINISHME: We would hit this path for function arguments.  Fix!
          */
         assert(var->location != -1);
         entry = new(mem_ctx) variable_storage(var,
                                               PROGRAM_INPUT,
                                               var->location);
         if (this->prog->Target == GL_VERTEX_PROGRAM_ARB &&
             var->location >= VERT_ATTRIB_GENERIC0) {
            _mesa_add_attribute(this->prog->Attributes,
                                var->name,
                                _mesa_sizeof_glsl_type(var->type->gl_type),
                                var->type->gl_type,
                                var->location - VERT_ATTRIB_GENERIC0);
         }
         break;
      case ir_var_out:
         assert(var->location != -1);
         entry = new(mem_ctx) variable_storage(var,
                                               PROGRAM_OUTPUT,
                                               var->location);
         break;
      case ir_var_system_value:
         entry = new(mem_ctx) variable_storage(var,
                                               PROGRAM_SYSTEM_VALUE,
                                               var->location);
         break;
      case ir_var_auto:
      case ir_var_temporary:
         entry = new(mem_ctx) variable_storage(var, PROGRAM_TEMPORARY,
        				       this->next_temp);
         this->variables.push_tail(entry);

         next_temp += type_size(var->type);
         break;
      }

      if (!entry) {
         printf("Failed to make storage for %s\n", var->name);
         exit(1);
      }
   }

   this->result = st_src_reg(entry->file, entry->index, var->type);
}

void
glsl_to_tgsi_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *index;
   st_src_reg src;
   int element_size = type_size(ir->type);

   index = ir->array_index->constant_expression_value();

   ir->array->accept(this);
   src = this->result;

   if (index) {
      src.index += index->value.i[0] * element_size;
   } else {
      st_src_reg array_base = this->result;
      /* Variable index array dereference.  It eats the "vec4" of the
       * base of the array and an index that offsets the Mesa register
       * index.
       */
      ir->array_index->accept(this);

      st_src_reg index_reg;

      if (element_size == 1) {
         index_reg = this->result;
      } else {
         index_reg = get_temp(glsl_type::float_type);

         emit(ir, OPCODE_MUL, st_dst_reg(index_reg),
              this->result, st_src_reg_for_float(element_size));
      }

      src.reladdr = ralloc(mem_ctx, st_src_reg);
      memcpy(src.reladdr, &index_reg, sizeof(index_reg));
   }

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector())
      src.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      src.swizzle = SWIZZLE_NOOP;

   this->result = src;
}

void
glsl_to_tgsi_visitor::visit(ir_dereference_record *ir)
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

   /* If the type is smaller than a vec4, replicate the last channel out. */
   if (ir->type->is_scalar() || ir->type->is_vector())
      this->result.swizzle = swizzle_for_size(ir->type->vector_elements);
   else
      this->result.swizzle = SWIZZLE_NOOP;

   this->result.index += offset;
}

/**
 * We want to be careful in assignment setup to hit the actual storage
 * instead of potentially using a temporary like we might with the
 * ir_dereference handler.
 */
static st_dst_reg
get_assignment_lhs(ir_dereference *ir, glsl_to_tgsi_visitor *v)
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
   return st_dst_reg(v->result);
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
glsl_to_tgsi_visitor::process_move_condition(ir_rvalue *ir)
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
glsl_to_tgsi_visitor::visit(ir_assignment *ir)
{
   st_dst_reg l;
   st_src_reg r;
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
      st_src_reg condition = this->result;

      for (i = 0; i < type_size(ir->lhs->type); i++) {
         if (switch_order) {
            emit(ir, OPCODE_CMP, l, condition, st_src_reg(l), r);
         } else {
            emit(ir, OPCODE_CMP, l, condition, r, st_src_reg(l));
         }

         l.index++;
         r.index++;
      }
   } else {
      for (i = 0; i < type_size(ir->lhs->type); i++) {
         emit(ir, OPCODE_MOV, l, r);
         l.index++;
         r.index++;
      }
   }
}


void
glsl_to_tgsi_visitor::visit(ir_constant *ir)
{
   st_src_reg src;
   GLfloat stack_vals[4] = { 0 };
   GLfloat *values = stack_vals;
   unsigned int i;

   /* Unfortunately, 4 floats is all we can get into
    * _mesa_add_unnamed_constant.  So, make a temp to store an
    * aggregate constant and move each constant value into it.  If we
    * get lucky, copy propagation will eliminate the extra moves.
    */

   if (ir->type->base_type == GLSL_TYPE_STRUCT) {
      st_src_reg temp_base = get_temp(ir->type);
      st_dst_reg temp = st_dst_reg(temp_base);

      foreach_iter(exec_list_iterator, iter, ir->components) {
         ir_constant *field_value = (ir_constant *)iter.get();
         int size = type_size(field_value->type);

         assert(size > 0);

         field_value->accept(this);
         src = this->result;

         for (i = 0; i < (unsigned int)size; i++) {
            emit(ir, OPCODE_MOV, temp, src);

            src.index++;
            temp.index++;
         }
      }
      this->result = temp_base;
      return;
   }

   if (ir->type->is_array()) {
      st_src_reg temp_base = get_temp(ir->type);
      st_dst_reg temp = st_dst_reg(temp_base);
      int size = type_size(ir->type->fields.array);

      assert(size > 0);

      for (i = 0; i < ir->type->length; i++) {
         ir->array_elements[i]->accept(this);
         src = this->result;
         for (int j = 0; j < size; j++) {
            emit(ir, OPCODE_MOV, temp, src);

            src.index++;
            temp.index++;
         }
      }
      this->result = temp_base;
      return;
   }

   if (ir->type->is_matrix()) {
      st_src_reg mat = get_temp(ir->type);
      st_dst_reg mat_column = st_dst_reg(mat);

      for (i = 0; i < ir->type->matrix_columns; i++) {
         assert(ir->type->base_type == GLSL_TYPE_FLOAT);
         values = &ir->value.f[i * ir->type->vector_elements];

         src = st_src_reg(PROGRAM_CONSTANT, -1, NULL);
         src.index = _mesa_add_unnamed_constant(this->prog->Parameters,
        					values,
        					ir->type->vector_elements,
        					&src.swizzle);
         emit(ir, OPCODE_MOV, mat_column, src);

         mat_column.index++;
      }

      this->result = mat;
      return;
   }

   src.file = PROGRAM_CONSTANT;
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

   this->result = st_src_reg(PROGRAM_CONSTANT, -1, ir->type);
   this->result.index = _mesa_add_unnamed_constant(this->prog->Parameters,
        					   values,
        					   ir->type->vector_elements,
        					   &this->result.swizzle);
}

function_entry *
glsl_to_tgsi_visitor::get_function_signature(ir_function_signature *sig)
{
   function_entry *entry;

   foreach_iter(exec_list_iterator, iter, this->function_signatures) {
      entry = (function_entry *)iter.get();

      if (entry->sig == sig)
         return entry;
   }

   entry = ralloc(mem_ctx, function_entry);
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
      entry->return_reg = undef_src;
   }

   this->function_signatures.push_tail(entry);
   return entry;
}

void
glsl_to_tgsi_visitor::visit(ir_call *ir)
{
   glsl_to_tgsi_instruction *call_inst;
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
         st_src_reg r = this->result;

         st_dst_reg l;
         l.file = storage->file;
         l.index = storage->index;
         l.reladdr = NULL;
         l.writemask = WRITEMASK_XYZW;
         l.cond_mask = COND_TR;

         for (i = 0; i < type_size(param->type); i++) {
            emit(ir, OPCODE_MOV, l, r);
            l.index++;
            r.index++;
         }
      }

      sig_iter.next();
   }
   assert(!sig_iter.has_next());

   /* Emit call instruction */
   call_inst = emit(ir, OPCODE_CAL);
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

         st_src_reg r;
         r.file = storage->file;
         r.index = storage->index;
         r.reladdr = NULL;
         r.swizzle = SWIZZLE_NOOP;
         r.negate = 0;

         param_rval->accept(this);
         st_dst_reg l = st_dst_reg(this->result);

         for (i = 0; i < type_size(param->type); i++) {
            emit(ir, OPCODE_MOV, l, r);
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
glsl_to_tgsi_visitor::visit(ir_texture *ir)
{
   st_src_reg result_src, coord, lod_info, projector, dx, dy;
   st_dst_reg result_dst, coord_dst;
   glsl_to_tgsi_instruction *inst = NULL;
   prog_opcode opcode = OPCODE_NOP;

   ir->coordinate->accept(this);

   /* Put our coords in a temp.  We'll need to modify them for shadow,
    * projection, or LOD, so the only case we'd use it as is is if
    * we're doing plain old texturing.  Mesa IR optimization should
    * handle cleaning up our mess in that case.
    */
   coord = get_temp(glsl_type::vec4_type);
   coord_dst = st_dst_reg(coord);
   emit(ir, OPCODE_MOV, coord_dst, this->result);

   if (ir->projector) {
      ir->projector->accept(this);
      projector = this->result;
   }

   /* Storage for our result.  Ideally for an assignment we'd be using
    * the actual storage for the result here, instead.
    */
   result_src = get_temp(glsl_type::vec4_type);
   result_dst = st_dst_reg(result_src);

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
      opcode = OPCODE_TXD;
      ir->lod_info.grad.dPdx->accept(this);
      dx = this->result;
      ir->lod_info.grad.dPdy->accept(this);
      dy = this->result;
      break;
   case ir_txf: // TODO: use TGSI_OPCODE_TXF here
      assert(!"GLSL 1.30 features unsupported");
      break;
   }

   if (ir->projector) {
      if (opcode == OPCODE_TEX) {
         /* Slot the projector in as the last component of the coord. */
         coord_dst.writemask = WRITEMASK_W;
         emit(ir, OPCODE_MOV, coord_dst, projector);
         coord_dst.writemask = WRITEMASK_XYZW;
         opcode = OPCODE_TXP;
      } else {
         st_src_reg coord_w = coord;
         coord_w.swizzle = SWIZZLE_WWWW;

         /* For the other TEX opcodes there's no projective version
          * since the last slot is taken up by lod info.  Do the
          * projective divide now.
          */
         coord_dst.writemask = WRITEMASK_W;
         emit(ir, OPCODE_RCP, coord_dst, projector);

         /* In the case where we have to project the coordinates "by hand,"
          * the shadow comparitor value must also be projected.
          */
         st_src_reg tmp_src = coord;
         if (ir->shadow_comparitor) {
            /* Slot the shadow value in as the second to last component of the
             * coord.
             */
            ir->shadow_comparitor->accept(this);

            tmp_src = get_temp(glsl_type::vec4_type);
            st_dst_reg tmp_dst = st_dst_reg(tmp_src);

            tmp_dst.writemask = WRITEMASK_Z;
            emit(ir, OPCODE_MOV, tmp_dst, this->result);

            tmp_dst.writemask = WRITEMASK_XY;
            emit(ir, OPCODE_MOV, tmp_dst, coord);
         }

         coord_dst.writemask = WRITEMASK_XYZ;
         emit(ir, OPCODE_MUL, coord_dst, tmp_src, coord_w);

         coord_dst.writemask = WRITEMASK_XYZW;
         coord.swizzle = SWIZZLE_XYZW;
      }
   }

   /* If projection is done and the opcode is not OPCODE_TXP, then the shadow
    * comparitor was put in the correct place (and projected) by the code,
    * above, that handles by-hand projection.
    */
   if (ir->shadow_comparitor && (!ir->projector || opcode == OPCODE_TXP)) {
      /* Slot the shadow value in as the second to last component of the
       * coord.
       */
      ir->shadow_comparitor->accept(this);
      coord_dst.writemask = WRITEMASK_Z;
      emit(ir, OPCODE_MOV, coord_dst, this->result);
      coord_dst.writemask = WRITEMASK_XYZW;
   }

   if (opcode == OPCODE_TXL || opcode == OPCODE_TXB) {
      /* Mesa IR stores lod or lod bias in the last channel of the coords. */
      coord_dst.writemask = WRITEMASK_W;
      emit(ir, OPCODE_MOV, coord_dst, lod_info);
      coord_dst.writemask = WRITEMASK_XYZW;
   }

   if (opcode == OPCODE_TXD)
      inst = emit(ir, opcode, result_dst, coord, dx, dy);
   else
      inst = emit(ir, opcode, result_dst, coord);

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
glsl_to_tgsi_visitor::visit(ir_return *ir)
{
   if (ir->get_value()) {
      st_dst_reg l;
      int i;

      assert(current_function);

      ir->get_value()->accept(this);
      st_src_reg r = this->result;

      l = st_dst_reg(current_function->return_reg);

      for (i = 0; i < type_size(current_function->sig->return_type); i++) {
         emit(ir, OPCODE_MOV, l, r);
         l.index++;
         r.index++;
      }
   }

   emit(ir, OPCODE_RET);
}

void
glsl_to_tgsi_visitor::visit(ir_discard *ir)
{
   struct gl_fragment_program *fp = (struct gl_fragment_program *)this->prog;

   if (ir->condition) {
      ir->condition->accept(this);
      this->result.negate = ~this->result.negate;
      emit(ir, OPCODE_KIL, undef_dst, this->result);
   } else {
      emit(ir, OPCODE_KIL_NV);
   }

   fp->UsesKill = GL_TRUE;
}

void
glsl_to_tgsi_visitor::visit(ir_if *ir)
{
   glsl_to_tgsi_instruction *cond_inst, *if_inst, *else_inst = NULL;
   glsl_to_tgsi_instruction *prev_inst;

   prev_inst = (glsl_to_tgsi_instruction *)this->instructions.get_tail();

   ir->condition->accept(this);
   assert(this->result.file != PROGRAM_UNDEFINED);

   if (this->options->EmitCondCodes) {
      cond_inst = (glsl_to_tgsi_instruction *)this->instructions.get_tail();

      /* See if we actually generated any instruction for generating
       * the condition.  If not, then cook up a move to a temp so we
       * have something to set cond_update on.
       */
      if (cond_inst == prev_inst) {
         st_src_reg temp = get_temp(glsl_type::bool_type);
         cond_inst = emit(ir->condition, OPCODE_MOV, st_dst_reg(temp), result);
      }
      cond_inst->cond_update = GL_TRUE;

      if_inst = emit(ir->condition, OPCODE_IF);
      if_inst->dst.cond_mask = COND_NE;
   } else {
      if_inst = emit(ir->condition, OPCODE_IF, undef_dst, this->result);
   }

   this->instructions.push_tail(if_inst);

   visit_exec_list(&ir->then_instructions, this);

   if (!ir->else_instructions.is_empty()) {
      else_inst = emit(ir->condition, OPCODE_ELSE);
      visit_exec_list(&ir->else_instructions, this);
   }

   if_inst = emit(ir->condition, OPCODE_ENDIF);
}

glsl_to_tgsi_visitor::glsl_to_tgsi_visitor()
{
   result.file = PROGRAM_UNDEFINED;
   next_temp = 1;
   next_signature_id = 1;
   current_function = NULL;
   num_address_regs = 0;
   indirect_addr_temps = false;
   indirect_addr_consts = false;
   mem_ctx = ralloc_context(NULL);
}

glsl_to_tgsi_visitor::~glsl_to_tgsi_visitor()
{
   ralloc_free(mem_ctx);
}

extern "C" void free_glsl_to_tgsi_visitor(glsl_to_tgsi_visitor *v)
{
   delete v;
}


/**
 * Count resources used by the given gpu program (number of texture
 * samplers, etc).
 */
static void
count_resources(glsl_to_tgsi_visitor *v, gl_program *prog)
{
   v->samplers_used = 0;

   foreach_iter(exec_list_iterator, iter, v->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();

      if (_mesa_is_tex_instruction(inst->op)) {
         v->samplers_used |= 1 << inst->sampler;

         prog->SamplerTargets[inst->sampler] =
            (gl_texture_index)inst->tex_target;
         if (inst->tex_shadow) {
            prog->ShadowSamplers |= 1 << inst->sampler;
         }
      }
   }
   
   prog->SamplersUsed = v->samplers_used;
   _mesa_update_shader_textures_used(prog);
}


/**
 * Check if the given vertex/fragment/shader program is within the
 * resource limits of the context (number of texture units, etc).
 * If any of those checks fail, record a linker error.
 *
 * XXX more checks are needed...
 */
static void
check_resources(const struct gl_context *ctx,
                struct gl_shader_program *shader_program,
                glsl_to_tgsi_visitor *prog,
                struct gl_program *proginfo)
{
   switch (proginfo->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      if (_mesa_bitcount(prog->samplers_used) >
          ctx->Const.MaxVertexTextureImageUnits) {
         fail_link(shader_program, "Too many vertex shader texture samplers");
      }
      if (proginfo->Parameters->NumParameters > MAX_UNIFORMS) {
         fail_link(shader_program, "Too many vertex shader constants");
      }
      break;
   case MESA_GEOMETRY_PROGRAM:
      if (_mesa_bitcount(prog->samplers_used) >
          ctx->Const.MaxGeometryTextureImageUnits) {
         fail_link(shader_program, "Too many geometry shader texture samplers");
      }
      if (proginfo->Parameters->NumParameters >
          MAX_GEOMETRY_UNIFORM_COMPONENTS / 4) {
         fail_link(shader_program, "Too many geometry shader constants");
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      if (_mesa_bitcount(prog->samplers_used) >
          ctx->Const.MaxTextureImageUnits) {
         fail_link(shader_program, "Too many fragment shader texture samplers");
      }
      if (proginfo->Parameters->NumParameters > MAX_UNIFORMS) {
         fail_link(shader_program, "Too many fragment shader constants");
      }
      break;
   default:
      _mesa_problem(ctx, "unexpected program type in check_resources()");
   }
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

   sorted_uniforms = ralloc_array(NULL, struct uniform_sort,
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

   ralloc_free(sorted_uniforms);
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
         const char *field_name = ralloc_asprintf(mem_ctx, "%s.%s", name,
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
         int *conv = ralloc_array(mem_ctx, int, element_type->components());
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
            mem_ctx = ralloc_context(NULL);

         set_uniform_initializer(ctx, mem_ctx, shader_program, var->name,
        			 var->type, var->constant_value);
      }
   }

   ralloc_free(mem_ctx);
}

/*
 * Scan/rewrite program to remove reads of custom (output) registers.
 * The passed type has to be either PROGRAM_OUTPUT or PROGRAM_VARYING
 * (for vertex shaders).
 * In GLSL shaders, varying vars can be read and written.
 * On some hardware, trying to read an output register causes trouble.
 * So, rewrite the program to use a temporary register in this case.
 * 
 * Based on _mesa_remove_output_reads from programopt.c.
 */
void
glsl_to_tgsi_visitor::remove_output_reads(gl_register_file type)
{
   GLuint i;
   GLint outputMap[VERT_RESULT_MAX];
   GLuint numVaryingReads = 0;
   GLboolean usedTemps[MAX_PROGRAM_TEMPS];
   GLuint firstTemp = 0;

   _mesa_find_used_registers(prog, PROGRAM_TEMPORARY,
                             usedTemps, MAX_PROGRAM_TEMPS);

   assert(type == PROGRAM_VARYING || type == PROGRAM_OUTPUT);
   assert(prog->Target == GL_VERTEX_PROGRAM_ARB || type != PROGRAM_VARYING);

   for (i = 0; i < VERT_RESULT_MAX; i++)
      outputMap[i] = -1;

   /* look for instructions which read from varying vars */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      const GLuint numSrc = _mesa_num_inst_src_regs(inst->op);
      GLuint j;
      for (j = 0; j < numSrc; j++) {
         if (inst->src[j].file == type) {
            /* replace the read with a temp reg */
            const GLuint var = inst->src[j].index;
            if (outputMap[var] == -1) {
               numVaryingReads++;
               outputMap[var] = _mesa_find_free_register(usedTemps,
                                                         MAX_PROGRAM_TEMPS,
                                                         firstTemp);
               firstTemp = outputMap[var] + 1;
            }
            inst->src[j].file = PROGRAM_TEMPORARY;
            inst->src[j].index = outputMap[var];
         }
      }
   }

   if (numVaryingReads == 0)
      return; /* nothing to be done */

   /* look for instructions which write to the varying vars identified above */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      if (inst->dst.file == type && outputMap[inst->dst.index] >= 0) {
         /* change inst to write to the temp reg, instead of the varying */
         inst->dst.file = PROGRAM_TEMPORARY;
         inst->dst.index = outputMap[inst->dst.index];
      }
   }
   
   /* insert new MOV instructions at the end */
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (outputMap[i] >= 0) {
         /* MOV VAR[i], TEMP[tmp]; */
         st_src_reg src = st_src_reg(PROGRAM_TEMPORARY, outputMap[i]);
         st_dst_reg dst = st_dst_reg(type, WRITEMASK_XYZW);
         dst.index = i;
         this->emit(NULL, OPCODE_MOV, dst, src);
      }
   }
}

/* Replaces all references to a temporary register index with another index. */
void
glsl_to_tgsi_visitor::rename_temp_register(int index, int new_index)
{
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      unsigned j;
      
      for (j=0; j < _mesa_num_inst_src_regs(inst->op); j++) {
         if (inst->src[j].file == PROGRAM_TEMPORARY && 
             inst->src[j].index == index) {
            inst->src[j].index = new_index;
         }
      }
      
      if (inst->dst.file == PROGRAM_TEMPORARY && inst->dst.index == index) {
         inst->dst.index = new_index;
      }
   }
}

int
glsl_to_tgsi_visitor::get_first_temp_read(int index)
{
   int depth = 0; /* loop depth */
   int loop_start = -1; /* index of the first active BGNLOOP (if any) */
   unsigned i = 0, j;
   
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      
      for (j=0; j < _mesa_num_inst_src_regs(inst->op); j++) {
         if (inst->src[j].file == PROGRAM_TEMPORARY && 
             inst->src[j].index == index) {
            return (depth == 0) ? i : loop_start;
         }
      }
      
      if (inst->op == OPCODE_BGNLOOP) {
         if(depth++ == 0)
            loop_start = i;
      } else if (inst->op == OPCODE_ENDLOOP) {
         if (--depth == 0)
            loop_start = -1;
      }
      assert(depth >= 0);
      
      i++;
   }
   
   return -1;
}

int
glsl_to_tgsi_visitor::get_first_temp_write(int index)
{
   int depth = 0; /* loop depth */
   int loop_start = -1; /* index of the first active BGNLOOP (if any) */
   int i = 0;
   
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      
      if (inst->dst.file == PROGRAM_TEMPORARY && inst->dst.index == index) {
         return (depth == 0) ? i : loop_start;
      }
      
      if (inst->op == OPCODE_BGNLOOP) {
         if(depth++ == 0)
            loop_start = i;
      } else if (inst->op == OPCODE_ENDLOOP) {
         if (--depth == 0)
            loop_start = -1;
      }
      assert(depth >= 0);
      
      i++;
   }
   
   return -1;
}

int
glsl_to_tgsi_visitor::get_last_temp_read(int index)
{
   int depth = 0; /* loop depth */
   int last = -1; /* index of last instruction that reads the temporary */
   unsigned i = 0, j;
   
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      
      for (j=0; j < _mesa_num_inst_src_regs(inst->op); j++) {
         if (inst->src[j].file == PROGRAM_TEMPORARY && 
             inst->src[j].index == index) {
            last = (depth == 0) ? i : -2;
         }
      }
      
      if (inst->op == OPCODE_BGNLOOP)
         depth++;
      else if (inst->op == OPCODE_ENDLOOP)
         if (--depth == 0 && last == -2)
            last = i;
      assert(depth >= 0);
      
      i++;
   }
   
   assert(last >= -1);
   return last;
}

int
glsl_to_tgsi_visitor::get_last_temp_write(int index)
{
   int depth = 0; /* loop depth */
   int last = -1; /* index of last instruction that writes to the temporary */
   int i = 0;
   
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();
      
      if (inst->dst.file == PROGRAM_TEMPORARY && inst->dst.index == index)
         last = (depth == 0) ? i : -2;
      
      if (inst->op == OPCODE_BGNLOOP)
         depth++;
      else if (inst->op == OPCODE_ENDLOOP)
         if (--depth == 0 && last == -2)
            last = i;
      assert(depth >= 0);
      
      i++;
   }
   
   assert(last >= -1);
   return last;
}

/*
 * On a basic block basis, tracks available PROGRAM_TEMPORARY register
 * channels for copy propagation and updates following instructions to
 * use the original versions.
 *
 * The glsl_to_tgsi_visitor lazily produces code assuming that this pass
 * will occur.  As an example, a TXP production before this pass:
 *
 * 0: MOV TEMP[1], INPUT[4].xyyy;
 * 1: MOV TEMP[1].w, INPUT[4].wwww;
 * 2: TXP TEMP[2], TEMP[1], texture[0], 2D;
 *
 * and after:
 *
 * 0: MOV TEMP[1], INPUT[4].xyyy;
 * 1: MOV TEMP[1].w, INPUT[4].wwww;
 * 2: TXP TEMP[2], INPUT[4].xyyw, texture[0], 2D;
 *
 * which allows for dead code elimination on TEMP[1]'s writes.
 */
void
glsl_to_tgsi_visitor::copy_propagate(void)
{
   glsl_to_tgsi_instruction **acp = rzalloc_array(mem_ctx,
        					    glsl_to_tgsi_instruction *,
        					    this->next_temp * 4);
   int *acp_level = rzalloc_array(mem_ctx, int, this->next_temp * 4);
   int level = 0;

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();

      assert(inst->dst.file != PROGRAM_TEMPORARY
             || inst->dst.index < this->next_temp);

      /* First, do any copy propagation possible into the src regs. */
      for (int r = 0; r < 3; r++) {
         glsl_to_tgsi_instruction *first = NULL;
         bool good = true;
         int acp_base = inst->src[r].index * 4;

         if (inst->src[r].file != PROGRAM_TEMPORARY ||
             inst->src[r].reladdr)
            continue;

         /* See if we can find entries in the ACP consisting of MOVs
          * from the same src register for all the swizzled channels
          * of this src register reference.
          */
         for (int i = 0; i < 4; i++) {
            int src_chan = GET_SWZ(inst->src[r].swizzle, i);
            glsl_to_tgsi_instruction *copy_chan = acp[acp_base + src_chan];

            if (!copy_chan) {
               good = false;
               break;
            }

            assert(acp_level[acp_base + src_chan] <= level);

            if (!first) {
               first = copy_chan;
            } else {
               if (first->src[0].file != copy_chan->src[0].file ||
        	   first->src[0].index != copy_chan->src[0].index) {
        	  good = false;
        	  break;
               }
            }
         }

         if (good) {
            /* We've now validated that we can copy-propagate to
             * replace this src register reference.  Do it.
             */
            inst->src[r].file = first->src[0].file;
            inst->src[r].index = first->src[0].index;

            int swizzle = 0;
            for (int i = 0; i < 4; i++) {
               int src_chan = GET_SWZ(inst->src[r].swizzle, i);
               glsl_to_tgsi_instruction *copy_inst = acp[acp_base + src_chan];
               swizzle |= (GET_SWZ(copy_inst->src[0].swizzle, src_chan) <<
        		   (3 * i));
            }
            inst->src[r].swizzle = swizzle;
         }
      }

      switch (inst->op) {
      case OPCODE_BGNLOOP:
      case OPCODE_ENDLOOP:
         /* End of a basic block, clear the ACP entirely. */
         memset(acp, 0, sizeof(*acp) * this->next_temp * 4);
         break;

      case OPCODE_IF:
         ++level;
         break;

      case OPCODE_ENDIF:
      case OPCODE_ELSE:
         /* Clear all channels written inside the block from the ACP, but
          * leaving those that were not touched.
          */
         for (int r = 0; r < this->next_temp; r++) {
            for (int c = 0; c < 4; c++) {
               if (!acp[4 * r + c])
        	  continue;

               if (acp_level[4 * r + c] >= level)
        	  acp[4 * r + c] = NULL;
            }
         }
         if (inst->op == OPCODE_ENDIF)
            --level;
         break;

      default:
         /* Continuing the block, clear any written channels from
          * the ACP.
          */
         if (inst->dst.file == PROGRAM_TEMPORARY && inst->dst.reladdr) {
            /* Any temporary might be written, so no copy propagation
             * across this instruction.
             */
            memset(acp, 0, sizeof(*acp) * this->next_temp * 4);
         } else if (inst->dst.file == PROGRAM_OUTPUT &&
        	    inst->dst.reladdr) {
            /* Any output might be written, so no copy propagation
             * from outputs across this instruction.
             */
            for (int r = 0; r < this->next_temp; r++) {
               for (int c = 0; c < 4; c++) {
        	  if (!acp[4 * r + c])
        	     continue;

        	  if (acp[4 * r + c]->src[0].file == PROGRAM_OUTPUT)
        	     acp[4 * r + c] = NULL;
               }
            }
         } else if (inst->dst.file == PROGRAM_TEMPORARY ||
        	    inst->dst.file == PROGRAM_OUTPUT) {
            /* Clear where it's used as dst. */
            if (inst->dst.file == PROGRAM_TEMPORARY) {
               for (int c = 0; c < 4; c++) {
        	  if (inst->dst.writemask & (1 << c)) {
        	     acp[4 * inst->dst.index + c] = NULL;
        	  }
               }
            }

            /* Clear where it's used as src. */
            for (int r = 0; r < this->next_temp; r++) {
               for (int c = 0; c < 4; c++) {
        	  if (!acp[4 * r + c])
        	     continue;

        	  int src_chan = GET_SWZ(acp[4 * r + c]->src[0].swizzle, c);

        	  if (acp[4 * r + c]->src[0].file == inst->dst.file &&
        	      acp[4 * r + c]->src[0].index == inst->dst.index &&
        	      inst->dst.writemask & (1 << src_chan))
        	  {
        	     acp[4 * r + c] = NULL;
        	  }
               }
            }
         }
         break;
      }

      /* If this is a copy, add it to the ACP. */
      if (inst->op == OPCODE_MOV &&
          inst->dst.file == PROGRAM_TEMPORARY &&
          !inst->dst.reladdr &&
          !inst->saturate &&
          !inst->src[0].reladdr &&
          !inst->src[0].negate) {
         for (int i = 0; i < 4; i++) {
            if (inst->dst.writemask & (1 << i)) {
               acp[4 * inst->dst.index + i] = inst;
               acp_level[4 * inst->dst.index + i] = level;
            }
         }
      }
   }

   ralloc_free(acp_level);
   ralloc_free(acp);
}

/*
 * Tracks available PROGRAM_TEMPORARY registers for dead code elimination.
 *
 * The glsl_to_tgsi_visitor lazily produces code assuming that this pass
 * will occur.  As an example, a TXP production after copy propagation but 
 * before this pass:
 *
 * 0: MOV TEMP[1], INPUT[4].xyyy;
 * 1: MOV TEMP[1].w, INPUT[4].wwww;
 * 2: TXP TEMP[2], INPUT[4].xyyw, texture[0], 2D;
 *
 * and after this pass:
 *
 * 0: TXP TEMP[2], INPUT[4].xyyw, texture[0], 2D;
 * 
 * FIXME: assumes that all functions are inlined (no support for BGNSUB/ENDSUB)
 * FIXME: doesn't eliminate all dead code inside of loops; it steps around them
 */
void
glsl_to_tgsi_visitor::eliminate_dead_code(void)
{
   int i;
   
   for (i=0; i < this->next_temp; i++) {
      int last_read = get_last_temp_read(i);
      int j = 0;
      
      foreach_iter(exec_list_iterator, iter, this->instructions) {
         glsl_to_tgsi_instruction *inst = (glsl_to_tgsi_instruction *)iter.get();

         if (inst->dst.file == PROGRAM_TEMPORARY && inst->dst.index == i &&
             j > last_read)
         {
            iter.remove();
            delete inst;
         }
         
         j++;
      }
   }
}

/* Merges temporary registers together where possible to reduce the number of 
 * registers needed to run a program.
 * 
 * Produces optimal code only after copy propagation and dead code elimination 
 * have been run. */
void
glsl_to_tgsi_visitor::merge_registers(void)
{
   int *last_reads = rzalloc_array(mem_ctx, int, this->next_temp);
   int *first_writes = rzalloc_array(mem_ctx, int, this->next_temp);
   int i, j;
   
   /* Read the indices of the last read and first write to each temp register
    * into an array so that we don't have to traverse the instruction list as 
    * much. */
   for (i=0; i < this->next_temp; i++) {
      last_reads[i] = get_last_temp_read(i);
      first_writes[i] = get_first_temp_write(i);
   }
   
   /* Start looking for registers with non-overlapping usages that can be 
    * merged together. */
   for (i=0; i < this->next_temp; i++) {
      /* Don't touch unused registers. */
      if (last_reads[i] < 0 || first_writes[i] < 0) continue;
      
      for (j=0; j < this->next_temp; j++) {
         /* Don't touch unused registers. */
         if (last_reads[j] < 0 || first_writes[j] < 0) continue;
         
         /* We can merge the two registers if the first write to j is after or 
          * in the same instruction as the last read from i.  Note that the 
          * register at index i will always be used earlier or at the same time 
          * as the register at index j. */
         if (first_writes[i] <= first_writes[j] && 
             last_reads[i] <= first_writes[j])
         {
            rename_temp_register(j, i); /* Replace all references to j with i.*/
            
            /* Update the first_writes and last_reads arrays with the new 
             * values for the merged register index, and mark the newly unused 
             * register index as such. */
            last_reads[i] = last_reads[j];
            first_writes[j] = -1;
            last_reads[j] = -1;
         }
      }
   }
   
   ralloc_free(last_reads);
   ralloc_free(first_writes);
}

/* Reassign indices to temporary registers by reusing unused indices created 
 * by optimization passes. */
void
glsl_to_tgsi_visitor::renumber_registers(void)
{
   int i = 0;
   int new_index = 0;
   
   for (i=0; i < this->next_temp; i++) {
      if (get_first_temp_read(i) < 0) continue;
      if (i != new_index)
         rename_temp_register(i, new_index);
      new_index++;
   }
   
   this->next_temp = new_index;
}

/* ------------------------- TGSI conversion stuff -------------------------- */
struct label {
   unsigned branch_target;
   unsigned token;
};

/**
 * Intermediate state used during shader translation.
 */
struct st_translate {
   struct ureg_program *ureg;

   struct ureg_dst temps[MAX_PROGRAM_TEMPS];
   struct ureg_src *constants;
   struct ureg_dst outputs[PIPE_MAX_SHADER_OUTPUTS];
   struct ureg_src inputs[PIPE_MAX_SHADER_INPUTS];
   struct ureg_dst address[1];
   struct ureg_src samplers[PIPE_MAX_SAMPLERS];
   struct ureg_src systemValues[SYSTEM_VALUE_MAX];

   /* Extra info for handling point size clamping in vertex shader */
   struct ureg_dst pointSizeResult; /**< Actual point size output register */
   struct ureg_src pointSizeConst;  /**< Point size range constant register */
   GLint pointSizeOutIndex;         /**< Temp point size output register */
   GLboolean prevInstWrotePointSize;

   const GLuint *inputMapping;
   const GLuint *outputMapping;

   /* For every instruction that contains a label (eg CALL), keep
    * details so that we can go back afterwards and emit the correct
    * tgsi instruction number for each label.
    */
   struct label *labels;
   unsigned labels_size;
   unsigned labels_count;

   /* Keep a record of the tgsi instruction number that each mesa
    * instruction starts at, will be used to fix up labels after
    * translation.
    */
   unsigned *insn;
   unsigned insn_size;
   unsigned insn_count;

   unsigned procType;  /**< TGSI_PROCESSOR_VERTEX/FRAGMENT */

   boolean error;
};

/** Map Mesa's SYSTEM_VALUE_x to TGSI_SEMANTIC_x */
static unsigned mesa_sysval_to_semantic[SYSTEM_VALUE_MAX] = {
   TGSI_SEMANTIC_FACE,
   TGSI_SEMANTIC_INSTANCEID
};

/**
 * Make note of a branch to a label in the TGSI code.
 * After we've emitted all instructions, we'll go over the list
 * of labels built here and patch the TGSI code with the actual
 * location of each label.
 */
static unsigned *get_label( struct st_translate *t,
                            unsigned branch_target )
{
   unsigned i;

   if (t->labels_count + 1 >= t->labels_size) {
      t->labels_size = 1 << (util_logbase2(t->labels_size) + 1);
      t->labels = (struct label *)realloc(t->labels, 
                                          t->labels_size * sizeof t->labels[0]);
      if (t->labels == NULL) {
         static unsigned dummy;
         t->error = TRUE;
         return &dummy;
      }
   }

   i = t->labels_count++;
   t->labels[i].branch_target = branch_target;
   return &t->labels[i].token;
}

/**
 * Called prior to emitting the TGSI code for each Mesa instruction.
 * Allocate additional space for instructions if needed.
 * Update the insn[] array so the next Mesa instruction points to
 * the next TGSI instruction.
 */
static void set_insn_start( struct st_translate *t,
                            unsigned start )
{
   if (t->insn_count + 1 >= t->insn_size) {
      t->insn_size = 1 << (util_logbase2(t->insn_size) + 1);
      t->insn = (unsigned *)realloc(t->insn, t->insn_size * sizeof t->insn[0]);
      if (t->insn == NULL) {
         t->error = TRUE;
         return;
      }
   }

   t->insn[t->insn_count++] = start;
}

/**
 * Map a Mesa dst register to a TGSI ureg_dst register.
 */
static struct ureg_dst
dst_register( struct st_translate *t,
              gl_register_file file,
              GLuint index )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return ureg_dst_undef();

   case PROGRAM_TEMPORARY:
      if (ureg_dst_is_undef(t->temps[index]))
         t->temps[index] = ureg_DECL_temporary( t->ureg );

      return t->temps[index];

   case PROGRAM_OUTPUT:
      if (t->procType == TGSI_PROCESSOR_VERTEX && index == VERT_RESULT_PSIZ)
         t->prevInstWrotePointSize = GL_TRUE;

      if (t->procType == TGSI_PROCESSOR_VERTEX)
         assert(index < VERT_RESULT_MAX);
      else if (t->procType == TGSI_PROCESSOR_FRAGMENT)
         assert(index < FRAG_RESULT_MAX);
      else
         assert(index < GEOM_RESULT_MAX);

      assert(t->outputMapping[index] < Elements(t->outputs));

      return t->outputs[t->outputMapping[index]];

   case PROGRAM_ADDRESS:
      return t->address[index];

   default:
      debug_assert( 0 );
      return ureg_dst_undef();
   }
}

/**
 * Map a Mesa src register to a TGSI ureg_src register.
 */
static struct ureg_src
src_register( struct st_translate *t,
              gl_register_file file,
              GLuint index )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return ureg_src_undef();

   case PROGRAM_TEMPORARY:
      assert(index >= 0);
      assert(index < Elements(t->temps));
      if (ureg_dst_is_undef(t->temps[index]))
         t->temps[index] = ureg_DECL_temporary( t->ureg );
      return ureg_src(t->temps[index]);

   case PROGRAM_NAMED_PARAM:
   case PROGRAM_ENV_PARAM:
   case PROGRAM_LOCAL_PARAM:
   case PROGRAM_UNIFORM:
      assert(index >= 0);
      return t->constants[index];
   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:       /* ie, immediate */
      if (index < 0)
         return ureg_DECL_constant( t->ureg, 0 );
      else
         return t->constants[index];

   case PROGRAM_INPUT:
      assert(t->inputMapping[index] < Elements(t->inputs));
      return t->inputs[t->inputMapping[index]];

   case PROGRAM_OUTPUT:
      assert(t->outputMapping[index] < Elements(t->outputs));
      return ureg_src(t->outputs[t->outputMapping[index]]); /* not needed? */

   case PROGRAM_ADDRESS:
      return ureg_src(t->address[index]);

   case PROGRAM_SYSTEM_VALUE:
      assert(index < Elements(t->systemValues));
      return t->systemValues[index];

   default:
      debug_assert( 0 );
      return ureg_src_undef();
   }
}

/**
 * Create a TGSI ureg_dst register from a Mesa dest register.
 */
static struct ureg_dst
translate_dst( struct st_translate *t,
               const st_dst_reg *dst_reg, //const struct prog_dst_register *DstReg,
               boolean saturate )
{
   struct ureg_dst dst = dst_register( t, 
                                       dst_reg->file,
                                       dst_reg->index );

   dst = ureg_writemask( dst, 
                         dst_reg->writemask );
   
   if (saturate)
      dst = ureg_saturate( dst );

   if (dst_reg->reladdr != NULL)
      dst = ureg_dst_indirect( dst, ureg_src(t->address[0]) );

   return dst;
}

/**
 * Create a TGSI ureg_src register from a Mesa src register.
 */
static struct ureg_src
translate_src( struct st_translate *t,
               const st_src_reg *src_reg )
{
   struct ureg_src src = src_register( t, src_reg->file, src_reg->index );

   src = ureg_swizzle( src,
                       GET_SWZ( src_reg->swizzle, 0 ) & 0x3,
                       GET_SWZ( src_reg->swizzle, 1 ) & 0x3,
                       GET_SWZ( src_reg->swizzle, 2 ) & 0x3,
                       GET_SWZ( src_reg->swizzle, 3 ) & 0x3);

   if ((src_reg->negate & 0xf) == NEGATE_XYZW)
      src = ureg_negate(src);

#if 0
   // src_reg currently does not have an equivalent to SrcReg->Abs in Mesa IR
   if (src_reg->abs) 
      src = ureg_abs(src);
#endif

   if (src_reg->reladdr != NULL) {
      /* Normally ureg_src_indirect() would be used here, but a stupid compiler 
       * bug in g++ makes ureg_src_indirect (an inline C function) erroneously 
       * set the bit for src.Negate.  So we have to do the operation manually
       * here to work around the compiler's problems. */
      /*src = ureg_src_indirect(src, ureg_src(t->address[0]));*/
      struct ureg_src addr = ureg_src(t->address[0]);
      src.Indirect = 1;
      src.IndirectFile = addr.File;
      src.IndirectIndex = addr.Index;
      src.IndirectSwizzle = addr.SwizzleX;
      
      if (src_reg->file != PROGRAM_INPUT &&
          src_reg->file != PROGRAM_OUTPUT) {
         /* If src_reg->index was negative, it was set to zero in
          * src_register().  Reassign it now.  But don't do this
          * for input/output regs since they get remapped while
          * const buffers don't.
          */
         src.Index = src_reg->index;
      }
   }

   return src;
}

static void
compile_tgsi_instruction(struct st_translate *t, 
        			     const struct glsl_to_tgsi_instruction *inst)
{
   struct ureg_program *ureg = t->ureg;
   GLuint i;
   struct ureg_dst dst[1];
   struct ureg_src src[4];
   unsigned num_dst;
   unsigned num_src;

   num_dst = _mesa_num_inst_dst_regs( inst->op );
   num_src = _mesa_num_inst_src_regs( inst->op );

   if (num_dst) 
      dst[0] = translate_dst( t, 
                              &inst->dst,
                              inst->saturate); // inst->SaturateMode

   for (i = 0; i < num_src; i++) 
      src[i] = translate_src( t, &inst->src[i] );

   switch( inst->op ) {
   case OPCODE_SWZ:
      // TODO: copy emit_swz function from st_mesa_to_tgsi.c
      //emit_swz( t, dst[0], &inst->src[0] );
      assert(!"OPCODE_SWZ");
      return;

   case OPCODE_BGNLOOP:
   case OPCODE_CAL:
   case OPCODE_ELSE:
   case OPCODE_ENDLOOP:
   case OPCODE_IF:
      debug_assert(num_dst == 0);
      ureg_label_insn( ureg,
                       translate_opcode( inst->op ),
                       src, num_src,
                       get_label( t, 
                                  inst->op == OPCODE_CAL ? inst->function->sig_id : 0 ));
      return;

   case OPCODE_TEX:
   case OPCODE_TXB:
   case OPCODE_TXD:
   case OPCODE_TXL:
   case OPCODE_TXP:
      src[num_src++] = t->samplers[inst->sampler];
      ureg_tex_insn( ureg,
                     translate_opcode( inst->op ),
                     dst, num_dst, 
                     translate_texture_target( inst->tex_target,
                                               inst->tex_shadow ),
                     src, num_src );
      return;

   case OPCODE_SCS:
      dst[0] = ureg_writemask(dst[0], TGSI_WRITEMASK_XY );
      ureg_insn( ureg, 
                 translate_opcode( inst->op ), 
                 dst, num_dst, 
                 src, num_src );
      break;

   case OPCODE_XPD:
      dst[0] = ureg_writemask(dst[0], TGSI_WRITEMASK_XYZ );
      ureg_insn( ureg, 
                 translate_opcode( inst->op ), 
                 dst, num_dst, 
                 src, num_src );
      break;

   case OPCODE_NOISE1:
   case OPCODE_NOISE2:
   case OPCODE_NOISE3:
   case OPCODE_NOISE4:
      /* At some point, a motivated person could add a better
       * implementation of noise.  Currently not even the nvidia
       * binary drivers do anything more than this.  In any case, the
       * place to do this is in the GL state tracker, not the poor
       * driver.
       */
      ureg_MOV( ureg, dst[0], ureg_imm1f(ureg, 0.5) );
      break;
		 
   case OPCODE_DDY:
      // TODO: copy emit_ddy() function from st_mesa_to_tgsi.c
      assert(!"OPCODE_DDY");
      //emit_ddy( t, dst[0], &inst->src[0] );
      break;

   default:
      ureg_insn( ureg, 
                 translate_opcode( inst->op ), 
                 dst, num_dst, 
                 src, num_src );
      break;
   }
}

/**
 * Emit the TGSI instructions to adjust the WPOS pixel center convention
 * Basically, add (adjX, adjY) to the fragment position.
 */
static void
emit_adjusted_wpos( struct st_translate *t,
                    const struct gl_program *program,
                    GLfloat adjX, GLfloat adjY)
{
   struct ureg_program *ureg = t->ureg;
   struct ureg_dst wpos_temp = ureg_DECL_temporary(ureg);
   struct ureg_src wpos_input = t->inputs[t->inputMapping[FRAG_ATTRIB_WPOS]];

   /* Note that we bias X and Y and pass Z and W through unchanged.
    * The shader might also use gl_FragCoord.w and .z.
    */
   ureg_ADD(ureg, wpos_temp, wpos_input,
            ureg_imm4f(ureg, adjX, adjY, 0.0f, 0.0f));

   t->inputs[t->inputMapping[FRAG_ATTRIB_WPOS]] = ureg_src(wpos_temp);
}


/**
 * Emit the TGSI instructions for inverting the WPOS y coordinate.
 * This code is unavoidable because it also depends on whether
 * a FBO is bound (STATE_FB_WPOS_Y_TRANSFORM).
 */
static void
emit_wpos_inversion( struct st_translate *t,
                     const struct gl_program *program,
                     boolean invert)
{
   struct ureg_program *ureg = t->ureg;

   /* Fragment program uses fragment position input.
    * Need to replace instances of INPUT[WPOS] with temp T
    * where T = INPUT[WPOS] by y is inverted.
    */
   static const gl_state_index wposTransformState[STATE_LENGTH]
      = { STATE_INTERNAL, STATE_FB_WPOS_Y_TRANSFORM, 
          (gl_state_index)0, (gl_state_index)0, (gl_state_index)0 };
   
   /* XXX: note we are modifying the incoming shader here!  Need to
    * do this before emitting the constant decls below, or this
    * will be missed:
    */
   unsigned wposTransConst = _mesa_add_state_reference(program->Parameters,
                                                       wposTransformState);

   struct ureg_src wpostrans = ureg_DECL_constant( ureg, wposTransConst );
   struct ureg_dst wpos_temp;
   struct ureg_src wpos_input = t->inputs[t->inputMapping[FRAG_ATTRIB_WPOS]];

   /* MOV wpos_temp, input[wpos]
    */
   if (wpos_input.File == TGSI_FILE_TEMPORARY)
      wpos_temp = ureg_dst(wpos_input);
   else {
      wpos_temp = ureg_DECL_temporary( ureg );
      ureg_MOV( ureg, wpos_temp, wpos_input );
   }

   if (invert) {
      /* MAD wpos_temp.y, wpos_input, wpostrans.xxxx, wpostrans.yyyy
       */
      ureg_MAD( ureg,
                ureg_writemask(wpos_temp, TGSI_WRITEMASK_Y ),
                wpos_input,
                ureg_scalar(wpostrans, 0),
                ureg_scalar(wpostrans, 1));
   } else {
      /* MAD wpos_temp.y, wpos_input, wpostrans.zzzz, wpostrans.wwww
       */
      ureg_MAD( ureg,
                ureg_writemask(wpos_temp, TGSI_WRITEMASK_Y ),
                wpos_input,
                ureg_scalar(wpostrans, 2),
                ureg_scalar(wpostrans, 3));
   }

   /* Use wpos_temp as position input from here on:
    */
   t->inputs[t->inputMapping[FRAG_ATTRIB_WPOS]] = ureg_src(wpos_temp);
}


/**
 * Emit fragment position/ooordinate code.
 */
static void
emit_wpos(struct st_context *st,
          struct st_translate *t,
          const struct gl_program *program,
          struct ureg_program *ureg)
{
   const struct gl_fragment_program *fp =
      (const struct gl_fragment_program *) program;
   struct pipe_screen *pscreen = st->pipe->screen;
   boolean invert = FALSE;

   if (fp->OriginUpperLeft) {
      /* Fragment shader wants origin in upper-left */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT)) {
         /* the driver supports upper-left origin */
      }
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT)) {
         /* the driver supports lower-left origin, need to invert Y */
         ureg_property_fs_coord_origin(ureg, TGSI_FS_COORD_ORIGIN_LOWER_LEFT);
         invert = TRUE;
      }
      else
         assert(0);
   }
   else {
      /* Fragment shader wants origin in lower-left */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT))
         /* the driver supports lower-left origin */
         ureg_property_fs_coord_origin(ureg, TGSI_FS_COORD_ORIGIN_LOWER_LEFT);
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT))
         /* the driver supports upper-left origin, need to invert Y */
         invert = TRUE;
      else
         assert(0);
   }
   
   if (fp->PixelCenterInteger) {
      /* Fragment shader wants pixel center integer */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER))
         /* the driver supports pixel center integer */
         ureg_property_fs_coord_pixel_center(ureg, TGSI_FS_COORD_PIXEL_CENTER_INTEGER);
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER))
         /* the driver supports pixel center half integer, need to bias X,Y */
         emit_adjusted_wpos(t, program, 0.5f, invert ? 0.5f : -0.5f);
      else
         assert(0);
   }
   else {
      /* Fragment shader wants pixel center half integer */
      if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER)) {
         /* the driver supports pixel center half integer */
      }
      else if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER)) {
         /* the driver supports pixel center integer, need to bias X,Y */
         ureg_property_fs_coord_pixel_center(ureg, TGSI_FS_COORD_PIXEL_CENTER_INTEGER);
         emit_adjusted_wpos(t, program, 0.5f, invert ? -0.5f : 0.5f);
      }
      else
         assert(0);
   }

   /* we invert after adjustment so that we avoid the MOV to temporary,
    * and reuse the adjustment ADD instead */
   emit_wpos_inversion(t, program, invert);
}

/**
 * Translate intermediate IR (glsl_to_tgsi_instruction) to TGSI format.
 * \param program  the program to translate
 * \param numInputs  number of input registers used
 * \param inputMapping  maps Mesa fragment program inputs to TGSI generic
 *                      input indexes
 * \param inputSemanticName  the TGSI_SEMANTIC flag for each input
 * \param inputSemanticIndex  the semantic index (ex: which texcoord) for
 *                            each input
 * \param interpMode  the TGSI_INTERPOLATE_LINEAR/PERSP mode for each input
 * \param numOutputs  number of output registers used
 * \param outputMapping  maps Mesa fragment program outputs to TGSI
 *                       generic outputs
 * \param outputSemanticName  the TGSI_SEMANTIC flag for each output
 * \param outputSemanticIndex  the semantic index (ex: which texcoord) for
 *                             each output
 *
 * \return  PIPE_OK or PIPE_ERROR_OUT_OF_MEMORY
 */
extern "C" enum pipe_error
st_translate_program(
   struct gl_context *ctx,
   uint procType,
   struct ureg_program *ureg,
   glsl_to_tgsi_visitor *program,
   const struct gl_program *proginfo,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   boolean passthrough_edgeflags )
{
   struct st_translate translate, *t;
   unsigned i;
   enum pipe_error ret = PIPE_OK;

   assert(numInputs <= Elements(t->inputs));
   assert(numOutputs <= Elements(t->outputs));

   t = &translate;
   memset(t, 0, sizeof *t);

   t->procType = procType;
   t->inputMapping = inputMapping;
   t->outputMapping = outputMapping;
   t->ureg = ureg;
   t->pointSizeOutIndex = -1;
   t->prevInstWrotePointSize = GL_FALSE;

   /*
    * Declare input attributes.
    */
   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      for (i = 0; i < numInputs; i++) {
         t->inputs[i] = ureg_DECL_fs_input(ureg,
                                           inputSemanticName[i],
                                           inputSemanticIndex[i],
                                           interpMode[i]);
      }

      if (proginfo->InputsRead & FRAG_BIT_WPOS) {
         /* Must do this after setting up t->inputs, and before
          * emitting constant references, below:
          */
          printf("FRAG_BIT_WPOS\n");
          emit_wpos(st_context(ctx), t, proginfo, ureg);
      }

      if (proginfo->InputsRead & FRAG_BIT_FACE) {
         // TODO: uncomment
         printf("FRAG_BIT_FACE\n");
         //emit_face_var( t, program );
      }

      /*
       * Declare output attributes.
       */
      for (i = 0; i < numOutputs; i++) {
         switch (outputSemanticName[i]) {
         case TGSI_SEMANTIC_POSITION:
            t->outputs[i] = ureg_DECL_output( ureg,
                                              TGSI_SEMANTIC_POSITION, /* Z / Depth */
                                              outputSemanticIndex[i] );

            t->outputs[i] = ureg_writemask( t->outputs[i],
                                            TGSI_WRITEMASK_Z );
            break;
         case TGSI_SEMANTIC_STENCIL:
            t->outputs[i] = ureg_DECL_output( ureg,
                                              TGSI_SEMANTIC_STENCIL, /* Stencil */
                                              outputSemanticIndex[i] );
            t->outputs[i] = ureg_writemask( t->outputs[i],
                                            TGSI_WRITEMASK_Y );
            break;
         case TGSI_SEMANTIC_COLOR:
            t->outputs[i] = ureg_DECL_output( ureg,
                                              TGSI_SEMANTIC_COLOR,
                                              outputSemanticIndex[i] );
            break;
         default:
            debug_assert(0);
            return PIPE_ERROR_BAD_INPUT;
         }
      }
   }
   else if (procType == TGSI_PROCESSOR_GEOMETRY) {
      for (i = 0; i < numInputs; i++) {
         t->inputs[i] = ureg_DECL_gs_input(ureg,
                                           i,
                                           inputSemanticName[i],
                                           inputSemanticIndex[i]);
      }

      for (i = 0; i < numOutputs; i++) {
         t->outputs[i] = ureg_DECL_output( ureg,
                                           outputSemanticName[i],
                                           outputSemanticIndex[i] );
      }
   }
   else {
      assert(procType == TGSI_PROCESSOR_VERTEX);

      for (i = 0; i < numInputs; i++) {
         t->inputs[i] = ureg_DECL_vs_input(ureg, i);
      }

      for (i = 0; i < numOutputs; i++) {
         t->outputs[i] = ureg_DECL_output( ureg,
                                           outputSemanticName[i],
                                           outputSemanticIndex[i] );
         if ((outputSemanticName[i] == TGSI_SEMANTIC_PSIZE) && proginfo->Id) {
            /* Writing to the point size result register requires special
             * handling to implement clamping.
             */
            static const gl_state_index pointSizeClampState[STATE_LENGTH]
               = { STATE_INTERNAL, STATE_POINT_SIZE_IMPL_CLAMP, (gl_state_index)0, (gl_state_index)0, (gl_state_index)0 };
               /* XXX: note we are modifying the incoming shader here!  Need to
               * do this before emitting the constant decls below, or this
               * will be missed.
               * XXX: depends on "Parameters" field specific to Mesa IR
               */
            unsigned pointSizeClampConst =
               _mesa_add_state_reference(proginfo->Parameters,
                                         pointSizeClampState);
            struct ureg_dst psizregtemp = ureg_DECL_temporary( ureg );
            t->pointSizeConst = ureg_DECL_constant( ureg, pointSizeClampConst );
            t->pointSizeResult = t->outputs[i];
            t->pointSizeOutIndex = i;
            t->outputs[i] = psizregtemp;
         }
      }
      /*if (passthrough_edgeflags)
         emit_edgeflags( t, program ); */ // TODO: uncomment
   }

   /* Declare address register.
    */
   if (program->num_address_regs > 0) {
      debug_assert( program->num_address_regs == 1 );
      t->address[0] = ureg_DECL_address( ureg );
   }

   /* Declare misc input registers
    */
   {
      GLbitfield sysInputs = proginfo->SystemValuesRead;
      unsigned numSys = 0;
      for (i = 0; sysInputs; i++) {
         if (sysInputs & (1 << i)) {
            unsigned semName = mesa_sysval_to_semantic[i];
            t->systemValues[i] = ureg_DECL_system_value(ureg, numSys, semName, 0);
            numSys++;
            sysInputs &= ~(1 << i);
         }
      }
   }

   if (program->indirect_addr_temps) {
      /* If temps are accessed with indirect addressing, declare temporaries
       * in sequential order.  Else, we declare them on demand elsewhere.
       * (Note: the number of temporaries is equal to program->next_temp)
       */
      for (i = 0; i < (unsigned)program->next_temp; i++) {
         /* XXX use TGSI_FILE_TEMPORARY_ARRAY when it's supported by ureg */
         t->temps[i] = ureg_DECL_temporary( t->ureg );
      }
   }

   /* Emit constants and immediates.  Mesa uses a single index space
    * for these, so we put all the translated regs in t->constants.
    * XXX: this entire if block depends on proginfo->Parameters from Mesa IR
    */
   if (proginfo->Parameters) {
      t->constants = (struct ureg_src *)CALLOC( proginfo->Parameters->NumParameters * sizeof t->constants[0] );
      if (t->constants == NULL) {
         ret = PIPE_ERROR_OUT_OF_MEMORY;
         goto out;
      }

      for (i = 0; i < proginfo->Parameters->NumParameters; i++) {
         switch (proginfo->Parameters->Parameters[i].Type) {
         case PROGRAM_ENV_PARAM:
         case PROGRAM_LOCAL_PARAM:
         case PROGRAM_STATE_VAR:
         case PROGRAM_NAMED_PARAM:
         case PROGRAM_UNIFORM:
            t->constants[i] = ureg_DECL_constant( ureg, i );
            break;

            /* Emit immediates only when there's no indirect addressing of
             * the const buffer.
             * FIXME: Be smarter and recognize param arrays:
             * indirect addressing is only valid within the referenced
             * array.
             */
         case PROGRAM_CONSTANT:
            if (program->indirect_addr_consts)
               t->constants[i] = ureg_DECL_constant( ureg, i );
            else
               t->constants[i] = 
                  ureg_DECL_immediate( ureg,
                                       proginfo->Parameters->ParameterValues[i],
                                       4 );
            break;
         default:
            break;
         }
      }
   }

   /* texture samplers */
   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (program->samplers_used & (1 << i)) {
         t->samplers[i] = ureg_DECL_sampler( ureg, i );
      }
   }

   /* Emit each instruction in turn:
    */
   foreach_iter(exec_list_iterator, iter, program->instructions) {
      set_insn_start( t, ureg_get_instruction_number( ureg ));
      compile_tgsi_instruction( t, (glsl_to_tgsi_instruction *)iter.get() );

      if (t->prevInstWrotePointSize && proginfo->Id) {
         /* The previous instruction wrote to the (fake) vertex point size
          * result register.  Now we need to clamp that value to the min/max
          * point size range, putting the result into the real point size
          * register.
          * Note that we can't do this easily at the end of program due to
          * possible early return.
          */
         set_insn_start( t, ureg_get_instruction_number( ureg ));
         ureg_MAX( t->ureg,
                   ureg_writemask(t->outputs[t->pointSizeOutIndex], WRITEMASK_X),
                   ureg_src(t->outputs[t->pointSizeOutIndex]),
                   ureg_swizzle(t->pointSizeConst, 1,1,1,1));
         ureg_MIN( t->ureg, ureg_writemask(t->pointSizeResult, WRITEMASK_X),
                   ureg_src(t->outputs[t->pointSizeOutIndex]),
                   ureg_swizzle(t->pointSizeConst, 2,2,2,2));
      }
      t->prevInstWrotePointSize = GL_FALSE;
   }

   /* Fix up all emitted labels:
    */
   for (i = 0; i < t->labels_count; i++) {
      ureg_fixup_label( ureg,
                        t->labels[i].token,
                        t->insn[t->labels[i].branch_target] );
   }

out:
   FREE(t->insn);
   FREE(t->labels);
   FREE(t->constants);

   if (t->error) {
      debug_printf("%s: translate error flag set\n", __FUNCTION__);
   }

   return ret;
}
/* ----------------------------- End TGSI code ------------------------------ */

/**
 * Convert a shader's GLSL IR into a Mesa gl_program, although without 
 * generating Mesa IR.
 */
static struct gl_program *
get_mesa_program(struct gl_context *ctx,
                 struct gl_shader_program *shader_program,
        	 struct gl_shader *shader)
{
   glsl_to_tgsi_visitor* v = new glsl_to_tgsi_visitor();
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
   case GL_GEOMETRY_SHADER:
      target = GL_GEOMETRY_PROGRAM_NV;
      target_string = "geometry";
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
   v->ctx = ctx;
   v->prog = prog;
   v->shader_program = shader_program;
   v->options = options;

   add_uniforms_to_parameters_list(shader_program, shader, prog);

   /* Emit Mesa IR for main(). */
   visit_exec_list(shader->ir, v);
   v->emit(NULL, OPCODE_END);

   /* Now emit bodies for any functions that were used. */
   do {
      progress = GL_FALSE;

      foreach_iter(exec_list_iterator, iter, v->function_signatures) {
         function_entry *entry = (function_entry *)iter.get();

         if (!entry->bgn_inst) {
            v->current_function = entry;

            entry->bgn_inst = v->emit(NULL, OPCODE_BGNSUB);
            entry->bgn_inst->function = entry;

            visit_exec_list(&entry->sig->body, v);

            glsl_to_tgsi_instruction *last;
            last = (glsl_to_tgsi_instruction *)v->instructions.get_tail();
            if (last->op != OPCODE_RET)
               v->emit(NULL, OPCODE_RET);

            glsl_to_tgsi_instruction *end;
            end = v->emit(NULL, OPCODE_ENDSUB);
            end->function = entry;

            progress = GL_TRUE;
         }
      }
   } while (progress);

#if 0
   /* Print out some information (for debugging purposes) used by the 
    * optimization passes. */
   for (i=0; i < v->next_temp; i++) {
      int fr = v->get_first_temp_read(i);
      int fw = v->get_first_temp_write(i);
      int lr = v->get_last_temp_read(i);
      int lw = v->get_last_temp_write(i);
      
      printf("Temp %d: FR=%3d FW=%3d LR=%3d LW=%3d\n", i, fr, fw, lr, lw);
      assert(fw <= fr);
   }
#endif

   /* Remove reads to output registers, and to varyings in vertex shaders. */
   v->remove_output_reads(PROGRAM_OUTPUT);
   if (target == GL_VERTEX_PROGRAM_ARB)
      v->remove_output_reads(PROGRAM_VARYING);

   /* Perform optimizations on the instructions in the glsl_to_tgsi_visitor. */
   v->copy_propagate();
   v->eliminate_dead_code();
   v->merge_registers();
   v->renumber_registers();

   if (ctx->Shader.Flags & GLSL_DUMP) {
      printf("\n");
      printf("GLSL IR for linked %s program %d:\n", target_string,
             shader_program->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n");
      printf("\n");
   }

   prog->Instructions = NULL;
   prog->NumInstructions = 0;

   do_set_program_inouts(shader->ir, prog);
   count_resources(v, prog);

   check_resources(ctx, shader_program, v, prog);

   _mesa_reference_program(ctx, &shader->Program, prog);
   
   struct st_vertex_program *stvp;
   struct st_fragment_program *stfp;
   struct st_geometry_program *stgp;
   
   switch (shader->Type) {
   case GL_VERTEX_SHADER:
      stvp = (struct st_vertex_program *)prog;
      stvp->glsl_to_tgsi = v;
      break;
   case GL_FRAGMENT_SHADER:
      stfp = (struct st_fragment_program *)prog;
      stfp->glsl_to_tgsi = v;
      break;
   case GL_GEOMETRY_SHADER:
      stgp = (struct st_geometry_program *)prog;
      stgp->glsl_to_tgsi = v;
      break;
   default:
      assert(!"should not be reached");
      return NULL;
   }

   return prog;
}

extern "C" {

struct gl_shader *
st_new_shader(struct gl_context *ctx, GLuint name, GLuint type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER ||
          type == GL_GEOMETRY_SHADER_ARB);
   shader = rzalloc(NULL, struct gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      _mesa_init_shader(ctx, shader);
   }
   return shader;
}

struct gl_shader_program *
st_new_shader_program(struct gl_context *ctx, GLuint name)
{
   struct gl_shader_program *shProg;
   shProg = rzalloc(NULL, struct gl_shader_program);
   if (shProg) {
      shProg->Name = name;
      _mesa_init_shader_program(ctx, shProg);
   }
   return shProg;
}

/**
 * Link a shader.
 * Called via ctx->Driver.LinkShader()
 * This actually involves converting GLSL IR into Mesa gl_programs with
 * code lowering and other optimizations.
 */
GLboolean
st_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
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
         lower_instructions(ir, (MOD_TO_FRACT | DIV_TO_MUL_RCP | EXP_TO_EXP2
        			 | LOG_TO_LOG2
        			 | ((options->EmitNoPow) ? POW_TO_EXP2 : 0)));

         progress = do_lower_jumps(ir, true, true, options->EmitNoMainReturn, options->EmitNoCont, options->EmitNoLoops) || progress;

         progress = do_common_optimization(ir, true, options->MaxUnrollIterations) || progress;

         progress = lower_quadop_vector(ir, true) || progress;

         if (options->EmitNoIfs) {
            progress = lower_discard(ir) || progress;
            progress = lower_if_to_cond_assign(ir) || progress;
         }

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
         case GL_GEOMETRY_SHADER:
            _mesa_reference_geomprog(ctx, &prog->GeometryProgram,
                                     (struct gl_geometry_program *)linked_prog);
            ok = ctx->Driver.ProgramStringNotify(ctx, GL_GEOMETRY_PROGRAM_NV,
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
 * Link a GLSL shader program.  Called via glLinkProgram().
 */
void
st_glsl_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
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
   _mesa_reference_geomprog(ctx, &prog->GeometryProgram, NULL);

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
