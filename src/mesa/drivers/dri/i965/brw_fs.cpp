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

extern "C" {
#include "main/macros.h"
#include "main/shaderobj.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
#include "talloc.h"
}
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"

enum register_file {
   ARF = BRW_ARCHITECTURE_REGISTER_FILE,
   GRF = BRW_GENERAL_REGISTER_FILE,
   MRF = BRW_MESSAGE_REGISTER_FILE,
   IMM = BRW_IMMEDIATE_VALUE,
   BAD_FILE
};

enum fs_opcodes {
   FS_OPCODE_FB_WRITE = 256,
};

static int using_new_fs = -1;

struct gl_shader *
brw_new_shader(GLcontext *ctx, GLuint name, GLuint type)
{
   struct brw_shader *shader;

   shader = talloc_zero(NULL, struct brw_shader);
   shader->base.Type = type;
   shader->base.Name = name;
   if (shader) {
      _mesa_init_shader(ctx, &shader->base);
   }

   return &shader->base;
}

struct gl_shader_program *
brw_new_shader_program(GLcontext *ctx, GLuint name)
{
   struct brw_shader_program *prog;
   prog = talloc_zero(NULL, struct brw_shader_program);
   if (prog) {
      _mesa_init_shader_program(ctx, &prog->base);
   }
   return &prog->base;
}

GLboolean
brw_compile_shader(GLcontext *ctx, struct gl_shader *shader)
{
   if (!_mesa_ir_compile_shader(ctx, shader))
      return GL_FALSE;

   return GL_TRUE;
}

GLboolean
brw_link_shader(GLcontext *ctx, struct gl_shader_program *prog)
{
   if (using_new_fs == -1)
      using_new_fs = getenv("INTEL_NEW_FS") != NULL;

   for (unsigned i = 0; i < prog->_NumLinkedShaders; i++) {
      struct brw_shader *shader = (struct brw_shader *)prog->_LinkedShaders[i];

      if (using_new_fs && shader->base.Type == GL_FRAGMENT_SHADER) {
	 void *mem_ctx = talloc_new(NULL);
	 bool progress;

	 shader->ir = new(shader) exec_list;
	 clone_ir_list(mem_ctx, shader->ir, shader->base.ir);

	 do_mat_op_to_vec(shader->ir);
	 brw_do_channel_expressions(shader->ir);
	 brw_do_vector_splitting(shader->ir);

	 do {
	    progress = false;

	    progress = do_common_optimization(shader->ir, true) || progress;
	 } while (progress);

	 reparent_ir(shader->ir, shader);
	 talloc_free(mem_ctx);
      }
   }

   if (!_mesa_ir_link_shader(ctx, prog))
      return GL_FALSE;

   return GL_TRUE;
}

class fs_reg {
public:
   fs_reg()
   {
      this->file = BAD_FILE;
      this->reg = 0;
      this->hw_reg = -1;
   }

   fs_reg(float f)
   {
      this->file = IMM;
      this->reg = 0;
      this->hw_reg = 0;
      this->type = BRW_REGISTER_TYPE_F;
      this->imm.f = f;
   }

   fs_reg(int32_t i)
   {
      this->file = IMM;
      this->reg = 0;
      this->hw_reg = 0;
      this->type = BRW_REGISTER_TYPE_D;
      this->imm.i = i;
   }

   fs_reg(uint32_t u)
   {
      this->file = IMM;
      this->reg = 0;
      this->hw_reg = 0;
      this->type = BRW_REGISTER_TYPE_UD;
      this->imm.u = u;
   }

   fs_reg(enum register_file file, int hw_reg)
   {
      this->file = file;
      this->reg = 0;
      this->hw_reg = hw_reg;
      this->type = BRW_REGISTER_TYPE_F;
   }

   /** Register file: ARF, GRF, MRF, IMM. */
   enum register_file file;
   /** Abstract register number.  0 = fixed hw reg */
   int reg;
   /** HW register number.  Generally unset until register allocation. */
   int hw_reg;
   /** Register type.  BRW_REGISTER_TYPE_* */
   int type;

   /** Value for file == BRW_IMMMEDIATE_FILE */
   union {
      int32_t i;
      uint32_t u;
      float f;
   } imm;
};

static const fs_reg reg_undef(BAD_FILE, -1);
static const fs_reg reg_null(ARF, BRW_ARF_NULL);

class fs_inst : public exec_node {
public:
   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = talloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   fs_inst()
   {
      this->opcode = BRW_OPCODE_NOP;
      this->dst = reg_undef;
      this->src[0] = reg_undef;
      this->src[1] = reg_undef;
   }
   fs_inst(int opcode, fs_reg dst, fs_reg src0)
   {
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
      this->src[1] = reg_undef;
   }
   fs_inst(int opcode, fs_reg dst, fs_reg src0, fs_reg src1)
   {
      this->opcode = opcode;
      this->dst = dst;
      this->src[0] = src0;
      this->src[1] = src1;
   }

   int opcode; /* BRW_OPCODE_* or FS_OPCODE_* */
   fs_reg dst;
   fs_reg src[2];
};

class fs_visitor : public ir_hierarchical_visitor
{
public:

   fs_visitor(struct brw_wm_compile *c, struct brw_shader *shader)
   {
      this->c = c;
      this->p = &c->func;
      this->mem_ctx = talloc_new(NULL);
      this->shader = shader;
   }
   ~fs_visitor()
   {
      talloc_free(this->mem_ctx);
   }

   fs_inst *emit(fs_inst inst);
   void generate_code();
   void generate_fb_write(fs_inst *inst);

   void emit_dummy_fs();

   struct brw_wm_compile *c;
   struct brw_compile *p;
   struct brw_shader *shader;
   void *mem_ctx;
   exec_list instructions;

   int grf_used;

};

fs_inst *
fs_visitor::emit(fs_inst inst)
{
   fs_inst *list_inst = new(mem_ctx) fs_inst;
   *list_inst = inst;

   this->instructions.push_tail(list_inst);

   return list_inst;
}

/** Emits a dummy fragment shader consisting of magenta for bringup purposes. */
void
fs_visitor::emit_dummy_fs()
{
   /* Everyone's favorite color. */
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 2),
		fs_reg(1.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 3),
		fs_reg(0.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 4),
		fs_reg(1.0f)));
   emit(fs_inst(BRW_OPCODE_MOV,
		fs_reg(MRF, 5),
		fs_reg(0.0f)));

   fs_inst *write;
   write = emit(fs_inst(FS_OPCODE_FB_WRITE,
			fs_reg(0),
			fs_reg(0)));
}

void
fs_visitor::generate_fb_write(fs_inst *inst)
{
   GLboolean eot = 1; /* FINISHME: MRT */
   /* FINISHME: AADS */

   /* Header is 2 regs, g0 and g1 are the contents. g0 will be implied
    * move, here's g1.
    */
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_MOV(p,
	   brw_message_reg(1),
	   brw_vec8_grf(1, 0));
   brw_pop_insn_state(p);

   int nr = 2 + 4;

   brw_fb_WRITE(p,
		8, /* dispatch_width */
		retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW),
		0, /* base MRF */
		retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW),
		0, /* FINISHME: MRT target */
		nr,
		0,
		eot);
}

void
fs_visitor::generate_code()
{
   this->grf_used = 2; /* header */

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();
      struct brw_reg src[2], dst;

      for (unsigned int i = 0; i < 2; i++) {
	 switch (inst->src[i].file) {
	 case GRF:
	 case ARF:
	 case MRF:
	    src[i] = brw_vec8_reg(inst->src[i].file,
				  inst->src[i].hw_reg, 0);
	    src[i] = retype(src[i], inst->src[i].type);
	    break;
	 case IMM:
	    switch (inst->src[i].type) {
	    case BRW_REGISTER_TYPE_F:
	       src[i] = brw_imm_f(inst->src[i].imm.f);
	       break;
	    case BRW_REGISTER_TYPE_D:
	       src[i] = brw_imm_f(inst->src[i].imm.i);
	       break;
	    case BRW_REGISTER_TYPE_UD:
	       src[i] = brw_imm_f(inst->src[i].imm.u);
	       break;
	    default:
	       assert(!"not reached");
	       break;
	    }
	    break;
	 case BAD_FILE:
	    /* Probably unused. */
	    src[i] = brw_null_reg();
	 }
      }
      dst = brw_vec8_reg(inst->dst.file, inst->dst.hw_reg, 0);

      switch (inst->opcode) {
      case BRW_OPCODE_MOV:
	 brw_MOV(p, dst, src[0]);
	 break;
      case FS_OPCODE_FB_WRITE:
	 generate_fb_write(inst);
	 break;
      default:
	 assert(!"not reached");
      }
   }
}

GLboolean
brw_wm_fs_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
   struct brw_compile *p = &c->func;
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;
   struct brw_shader *shader = NULL;
   struct gl_shader_program *prog = ctx->Shader.CurrentProgram;

   if (!prog)
      return GL_FALSE;

   if (!using_new_fs)
      return GL_FALSE;

   for (unsigned int i = 0; i < prog->_NumLinkedShaders; i++) {
      if (prog->_LinkedShaders[i]->Type == GL_FRAGMENT_SHADER) {
	 shader = (struct brw_shader *)prog->_LinkedShaders[i];
	 break;
      }
   }
   if (!shader)
      return GL_FALSE;

   /* We always use 8-wide mode, at least for now.  For one, flow
    * control only works in 8-wide.  Also, when we're fragment shader
    * bound, we're almost always under register pressure as well, so
    * 8-wide would save us from the performance cliff of spilling
    * regs.
    */
   c->dispatch_width = 8;

   if (INTEL_DEBUG & DEBUG_WM) {
      printf("GLSL IR for native fragment shader %d:\n", prog->Name);
      _mesa_print_ir(shader->ir, NULL);
      printf("\n");
   }

   /* Now the main event: Visit the shader IR and generate our FS IR for it.
    */
   fs_visitor v(c, shader);
   visit_list_elements(&v, shader->ir);

   v.emit_dummy_fs();

   v.generate_code();

   if (INTEL_DEBUG & DEBUG_WM) {
      printf("Native code for fragment shader %d:\n", prog->Name);
      for (unsigned int i = 0; i < p->nr_insn; i++)
	 brw_disasm(stdout, &p->store[i], intel->gen);
      printf("\n");
   }

   c->prog_data.nr_params = 0; /* FINISHME */
   c->prog_data.first_curbe_grf = c->key.nr_payload_regs;
   c->prog_data.urb_read_length = 1; /* FINISHME: attrs */
   c->prog_data.curb_read_length = 0; /* FINISHME */
   c->prog_data.total_grf = v.grf_used;
   c->prog_data.total_scratch = 0;

   return GL_TRUE;
}
