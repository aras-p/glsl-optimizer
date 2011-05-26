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

   return GL_TRUE;
}
