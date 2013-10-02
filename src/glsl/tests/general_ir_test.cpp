/*
 * Copyright © 2013 Intel Corporation
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
#include <gtest/gtest.h>
#include "main/compiler.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "ralloc.h"
#include "ir.h"

TEST(ir_variable_constructor, interface)
{
   void *mem_ctx = ralloc_context(NULL);

   static const glsl_struct_field f[] = {
      {
         glsl_type::vec(4),
         "v",
         false
      }
   };

   const glsl_type *const interface =
      glsl_type::get_interface_instance(f,
                                        ARRAY_SIZE(f),
                                        GLSL_INTERFACE_PACKING_STD140,
                                        "simple_interface");

   static const char name[] = "named_instance";

   ir_variable *const v =
      new(mem_ctx) ir_variable(interface, name, ir_var_uniform);

   EXPECT_STREQ(name, v->name);
   EXPECT_NE(name, v->name);
   EXPECT_EQ(interface, v->type);
   EXPECT_EQ(interface, v->get_interface_type());
}

TEST(ir_variable_constructor, interface_array)
{
   void *mem_ctx = ralloc_context(NULL);

   static const glsl_struct_field f[] = {
      {
         glsl_type::vec(4),
         "v",
         false
      }
   };

   const glsl_type *const interface =
      glsl_type::get_interface_instance(f,
                                        ARRAY_SIZE(f),
                                        GLSL_INTERFACE_PACKING_STD140,
                                        "simple_interface");

   const glsl_type *const interface_array =
      glsl_type::get_array_instance(interface, 2);

   static const char name[] = "array_instance";

   ir_variable *const v =
      new(mem_ctx) ir_variable(interface_array, name, ir_var_uniform);

   EXPECT_STREQ(name, v->name);
   EXPECT_NE(name, v->name);
   EXPECT_EQ(interface_array, v->type);
   EXPECT_EQ(interface, v->get_interface_type());
}
