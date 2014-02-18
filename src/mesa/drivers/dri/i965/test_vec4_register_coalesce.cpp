/*
 * Copyright © 2012 Intel Corporation
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

#define register_coalesce(v) _register_coalesce(v, __FUNCTION__)

class register_coalesce_test : public ::testing::Test {
   virtual void SetUp();

public:
   struct brw_context *brw;
   struct gl_context *ctx;
   struct gl_shader_program *shader_prog;
   struct brw_vertex_program *vp;
   vec4_visitor *v;
};


class register_coalesce_vec4_visitor : public vec4_visitor
{
public:
   register_coalesce_vec4_visitor(struct brw_context *brw,
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


void register_coalesce_test::SetUp()
{
   brw = (struct brw_context *)calloc(1, sizeof(*brw));
   ctx = &brw->ctx;

   vp = ralloc(NULL, struct brw_vertex_program);

   shader_prog = ralloc(NULL, struct gl_shader_program);

   v = new register_coalesce_vec4_visitor(brw, shader_prog);

   _mesa_init_vertex_program(ctx, &vp->program, GL_VERTEX_SHADER, 0);

   brw->gen = 4;
}

static void
_register_coalesce(vec4_visitor *v, const char *func)
{
   bool print = false;

   if (print) {
      printf("%s: instructions before:\n", func);
      v->dump_instructions();
   }

   v->opt_register_coalesce();

   if (print) {
      printf("%s: instructions after:\n", func);
      v->dump_instructions();
   }
}

TEST_F(register_coalesce_test, test_compute_to_mrf)
{
   src_reg something = src_reg(v, glsl_type::float_type);
   dst_reg temp = dst_reg(v, glsl_type::float_type);
   dst_reg init;

   dst_reg m0 = dst_reg(MRF, 0);
   m0.writemask = WRITEMASK_X;
   m0.type = BRW_REGISTER_TYPE_F;

   vec4_instruction *mul = v->emit(v->MUL(temp, something, src_reg(1.0f)));
   v->emit(v->MOV(m0, src_reg(temp)));

   register_coalesce(v);

   EXPECT_EQ(mul->dst.file, MRF);
}


TEST_F(register_coalesce_test, test_multiple_use)
{
   src_reg something = src_reg(v, glsl_type::float_type);
   dst_reg temp = dst_reg(v, glsl_type::vec4_type);
   dst_reg init;

   dst_reg m0 = dst_reg(MRF, 0);
   m0.writemask = WRITEMASK_X;
   m0.type = BRW_REGISTER_TYPE_F;

   dst_reg m1 = dst_reg(MRF, 1);
   m1.writemask = WRITEMASK_XYZW;
   m1.type = BRW_REGISTER_TYPE_F;

   src_reg src = src_reg(temp);
   vec4_instruction *mul = v->emit(v->MUL(temp, something, src_reg(1.0f)));
   src.swizzle = BRW_SWIZZLE_XXXX;
   v->emit(v->MOV(m0, src));
   src.swizzle = BRW_SWIZZLE_XYZW;
   v->emit(v->MOV(m1, src));

   register_coalesce(v);

   EXPECT_NE(mul->dst.file, MRF);
}

TEST_F(register_coalesce_test, test_dp4_mrf)
{
   src_reg some_src_1 = src_reg(v, glsl_type::vec4_type);
   src_reg some_src_2 = src_reg(v, glsl_type::vec4_type);
   dst_reg init;

   dst_reg m0 = dst_reg(MRF, 0);
   m0.writemask = WRITEMASK_Y;
   m0.type = BRW_REGISTER_TYPE_F;

   dst_reg temp = dst_reg(v, glsl_type::float_type);

   vec4_instruction *dp4 = v->emit(v->DP4(temp, some_src_1, some_src_2));
   v->emit(v->MOV(m0, src_reg(temp)));

   register_coalesce(v);

   EXPECT_EQ(dp4->dst.file, MRF);
   EXPECT_EQ(dp4->dst.writemask, WRITEMASK_Y);
}

TEST_F(register_coalesce_test, test_dp4_grf)
{
   src_reg some_src_1 = src_reg(v, glsl_type::vec4_type);
   src_reg some_src_2 = src_reg(v, glsl_type::vec4_type);
   dst_reg init;

   dst_reg to = dst_reg(v, glsl_type::vec4_type);
   dst_reg temp = dst_reg(v, glsl_type::float_type);

   vec4_instruction *dp4 = v->emit(v->DP4(temp, some_src_1, some_src_2));
   to.writemask = WRITEMASK_Y;
   v->emit(v->MOV(to, src_reg(temp)));

   /* if we don't do something with the result, the automatic dead code
    * elimination will remove all our instructions.
    */
   src_reg src = src_reg(to);
   src.negate = true;
   v->emit(v->MOV(dst_reg(MRF, 0), src));

   register_coalesce(v);

   EXPECT_EQ(dp4->dst.reg, to.reg);
   EXPECT_EQ(dp4->dst.writemask, WRITEMASK_Y);
}

TEST_F(register_coalesce_test, test_channel_mul_grf)
{
   src_reg some_src_1 = src_reg(v, glsl_type::vec4_type);
   src_reg some_src_2 = src_reg(v, glsl_type::vec4_type);
   dst_reg init;

   dst_reg to = dst_reg(v, glsl_type::vec4_type);
   dst_reg temp = dst_reg(v, glsl_type::float_type);

   vec4_instruction *mul = v->emit(v->MUL(temp, some_src_1, some_src_2));
   to.writemask = WRITEMASK_Y;
   v->emit(v->MOV(to, src_reg(temp)));

   /* if we don't do something with the result, the automatic dead code
    * elimination will remove all our instructions.
    */
   src_reg src = src_reg(to);
   src.negate = true;
   v->emit(v->MOV(dst_reg(MRF, 0), src));

   register_coalesce(v);

   /* This path isn't supported yet in the reswizzling code, so we're checking
    * that we haven't done anything bad to scalar non-DP[234]s.
    */
   EXPECT_NE(mul->dst.reg, to.reg);
}
