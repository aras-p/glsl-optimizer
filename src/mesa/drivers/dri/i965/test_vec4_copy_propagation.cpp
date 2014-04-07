/*
 * Copyright © 2014 Intel Corporation
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

#include <gtest/gtest.h>
#include "brw_vec4.h"
#include "brw_vs.h"

using namespace brw;

int ret = 0;

class copy_propagation_test : public ::testing::Test {
   virtual void SetUp();

public:
   struct brw_context *brw;
   struct gl_context *ctx;
   struct gl_shader_program *shader_prog;
   struct brw_vertex_program *vp;
   vec4_visitor *v;
};

class copy_propagation_vec4_visitor : public vec4_visitor
{
public:
   copy_propagation_vec4_visitor(struct brw_context *brw,
                                  struct gl_shader_program *shader_prog)
      : vec4_visitor(brw, NULL, NULL, NULL, NULL, shader_prog,
                     MESA_SHADER_VERTEX, NULL,
                     false, false /* no_spills */,
                     ST_NONE, ST_NONE, ST_NONE)
   {
   }

protected:
   virtual dst_reg *make_reg_for_system_value(ir_variable *ir)
   {
      assert(!"Not reached");
      return NULL;
   }

   virtual void setup_payload()
   {
      assert(!"Not reached");
   }

   virtual void emit_prolog()
   {
      assert(!"Not reached");
   }

   virtual void emit_program_code()
   {
      assert(!"Not reached");
   }

   virtual void emit_thread_end()
   {
      assert(!"Not reached");
   }

   virtual void emit_urb_write_header(int mrf)
   {
      assert(!"Not reached");
   }

   virtual vec4_instruction *emit_urb_write_opcode(bool complete)
   {
      assert(!"Not reached");
      unreachable();
   }
};


void copy_propagation_test::SetUp()
{
   brw = (struct brw_context *)calloc(1, sizeof(*brw));
   ctx = &brw->ctx;

   vp = ralloc(NULL, struct brw_vertex_program);

   shader_prog = ralloc(NULL, struct gl_shader_program);

   v = new copy_propagation_vec4_visitor(brw, shader_prog);

   _mesa_init_vertex_program(ctx, &vp->program, GL_VERTEX_SHADER, 0);

   brw->gen = 4;
}

static void
copy_propagation(vec4_visitor *v)
{
   bool print = false;

   if (print) {
      fprintf(stderr, "instructions before:\n");
      v->dump_instructions();
   }

   v->opt_copy_propagation();

   if (print) {
      fprintf(stderr, "instructions after:\n");
      v->dump_instructions();
   }
}

TEST_F(copy_propagation_test, test_swizzle_swizzle)
{
   dst_reg a = dst_reg(v, glsl_type::vec4_type);
   dst_reg b = dst_reg(v, glsl_type::vec4_type);
   dst_reg c = dst_reg(v, glsl_type::vec4_type);

   v->emit(v->ADD(a, src_reg(a), src_reg(a)));

   v->emit(v->MOV(b, swizzle(src_reg(a), BRW_SWIZZLE4(SWIZZLE_Y,
                                                      SWIZZLE_Z,
                                                      SWIZZLE_W,
                                                      SWIZZLE_X))));

   vec4_instruction *test_mov =
      v->MOV(c, swizzle(src_reg(b), BRW_SWIZZLE4(SWIZZLE_Y,
                                                 SWIZZLE_Z,
                                                 SWIZZLE_W,
                                                 SWIZZLE_X)));
   v->emit(test_mov);

   copy_propagation(v);

   EXPECT_EQ(test_mov->src[0].reg, a.reg);
   EXPECT_EQ(test_mov->src[0].swizzle, BRW_SWIZZLE4(SWIZZLE_Z,
                                                    SWIZZLE_W,
                                                    SWIZZLE_X,
                                                    SWIZZLE_Y));
}
