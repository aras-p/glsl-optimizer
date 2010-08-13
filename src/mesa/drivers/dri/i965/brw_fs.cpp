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
   static int using_new_fs = -1;

   if (using_new_fs == -1)
      using_new_fs = getenv("INTEL_NEW_FS") != NULL;

   for (unsigned i = 0; i < prog->_NumLinkedShaders; i++) {
      struct gl_shader *shader = prog->_LinkedShaders[i];

      if (using_new_fs && shader->Type == GL_FRAGMENT_SHADER) {
	 do_mat_op_to_vec(shader->ir);
	 brw_do_channel_expressions(shader->ir);
      }
   }

   if (!_mesa_ir_link_shader(ctx, prog))
      return GL_FALSE;

   return GL_TRUE;
}
