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

extern "C" {
#include "main/macros.h"
#include "brw_context.h"
}
#include "brw_fs.h"
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"

struct gl_shader *
brw_new_shader(struct gl_context *ctx, GLuint name, GLuint type)
{
   struct brw_shader *shader;

   shader = rzalloc(NULL, struct brw_shader);
   if (shader) {
      shader->base.Type = type;
      shader->base.Name = name;
      _mesa_init_shader(ctx, &shader->base);
   }

   return &shader->base;
}

struct gl_shader_program *
brw_new_shader_program(struct gl_context *ctx, GLuint name)
{
   struct brw_shader_program *prog;
   prog = rzalloc(NULL, struct brw_shader_program);
   if (prog) {
      prog->base.Name = name;
      _mesa_init_shader_program(ctx, &prog->base);
   }
   return &prog->base;
}

/**
 * Performs a compile of the shader stages even when we don't know
 * what non-orthogonal state will be set, in the hope that it reflects
 * the eventual NOS used, and thus allows us to produce link failures.
 */
bool
brw_shader_precompile(struct gl_context *ctx, struct gl_shader_program *prog)
{
   if (!brw_fs_precompile(ctx, prog))
      return false;

   return true;
}

GLboolean
brw_link_shader(struct gl_context *ctx, struct gl_shader_program *prog)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = &brw->intel;

   struct brw_shader *shader =
      (struct brw_shader *)prog->_LinkedShaders[MESA_SHADER_FRAGMENT];
   if (shader != NULL) {
      void *mem_ctx = ralloc_context(NULL);
      bool progress;

      if (shader->ir)
	 ralloc_free(shader->ir);
      shader->ir = new(shader) exec_list;
      clone_ir_list(mem_ctx, shader->ir, shader->base.ir);

      do_mat_op_to_vec(shader->ir);
      lower_instructions(shader->ir,
			 MOD_TO_FRACT |
			 DIV_TO_MUL_RCP |
			 SUB_TO_ADD_NEG |
			 EXP_TO_EXP2 |
			 LOG_TO_LOG2);

      /* Pre-gen6 HW can only nest if-statements 16 deep.  Beyond this,
       * if-statements need to be flattened.
       */
      if (intel->gen < 6)
	 lower_if_to_cond_assign(shader->ir, 16);

      do_lower_texture_projection(shader->ir);
      do_vec_index_to_cond_assign(shader->ir);
      brw_do_cubemap_normalize(shader->ir);
      lower_noise(shader->ir);
      lower_quadop_vector(shader->ir, false);
      lower_variable_index_to_cond_assign(shader->ir,
					  GL_TRUE, /* input */
					  GL_TRUE, /* output */
					  GL_TRUE, /* temp */
					  GL_TRUE /* uniform */
					  );

      do {
	 progress = false;

	 brw_do_channel_expressions(shader->ir);
	 brw_do_vector_splitting(shader->ir);

	 progress = do_lower_jumps(shader->ir, true, true,
				   true, /* main return */
				   false, /* continue */
				   false /* loops */
				   ) || progress;

	 progress = do_common_optimization(shader->ir, true, 32) || progress;
      } while (progress);

      validate_ir_tree(shader->ir);

      reparent_ir(shader->ir, shader->ir);
      ralloc_free(mem_ctx);
   }

   if (!_mesa_ir_link_shader(ctx, prog))
      return GL_FALSE;

   if (!brw_shader_precompile(ctx, prog))
      return GL_FALSE;

   return GL_TRUE;
}


int
brw_type_for_base_type(const struct glsl_type *type)
{
   switch (type->base_type) {
   case GLSL_TYPE_FLOAT:
      return BRW_REGISTER_TYPE_F;
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      return BRW_REGISTER_TYPE_D;
   case GLSL_TYPE_UINT:
      return BRW_REGISTER_TYPE_UD;
   case GLSL_TYPE_ARRAY:
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_SAMPLER:
      /* These should be overridden with the type of the member when
       * dereferenced into.  BRW_REGISTER_TYPE_UD seems like a likely
       * way to trip up if we don't.
       */
      return BRW_REGISTER_TYPE_UD;
   default:
      assert(!"not reached");
      return BRW_REGISTER_TYPE_F;
   }
}

uint32_t
brw_conditional_for_comparison(unsigned int op)
{
   switch (op) {
   case ir_binop_less:
      return BRW_CONDITIONAL_L;
   case ir_binop_greater:
      return BRW_CONDITIONAL_G;
   case ir_binop_lequal:
      return BRW_CONDITIONAL_LE;
   case ir_binop_gequal:
      return BRW_CONDITIONAL_GE;
   case ir_binop_equal:
   case ir_binop_all_equal: /* same as equal for scalars */
      return BRW_CONDITIONAL_Z;
   case ir_binop_nequal:
   case ir_binop_any_nequal: /* same as nequal for scalars */
      return BRW_CONDITIONAL_NZ;
   default:
      assert(!"not reached: bad operation for comparison");
      return BRW_CONDITIONAL_NZ;
   }
}
