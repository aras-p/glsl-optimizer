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
#include "linker.h"

/**
 * \file varyings_test.cpp
 *
 * Test various aspects of linking shader stage inputs and outputs.
 */

class invalidate_locations : public ::testing::Test {
public:
   virtual void SetUp();
   virtual void TearDown();

   void *mem_ctx;
   exec_list ir;
};

void
invalidate_locations::SetUp()
{
   this->mem_ctx = ralloc_context(NULL);
   this->ir.make_empty();
}

void
invalidate_locations::TearDown()
{
   ralloc_free(this->mem_ctx);
   this->mem_ctx = NULL;
}

TEST_F(invalidate_locations, simple_vertex_in_generic)
{
   ir_variable *const var =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "a",
                               ir_var_shader_in);

   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_EQ(-1, var->data.location);

   var->data.location = VERT_ATTRIB_GENERIC0;
   var->data.location_frac = 2;

   ir.push_tail(var);

   link_invalidate_variable_locations(&ir);

   EXPECT_EQ(-1, var->data.location);
   EXPECT_EQ(0u, var->data.location_frac);
   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_TRUE(var->data.is_unmatched_generic_inout);
}

TEST_F(invalidate_locations, explicit_location_vertex_in_generic)
{
   ir_variable *const var =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "a",
                               ir_var_shader_in);

   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_EQ(-1, var->data.location);

   var->data.location = VERT_ATTRIB_GENERIC0;
   var->data.explicit_location = true;

   ir.push_tail(var);

   link_invalidate_variable_locations(&ir);

   EXPECT_EQ(VERT_ATTRIB_GENERIC0, var->data.location);
   EXPECT_EQ(0u, var->data.location_frac);
   EXPECT_TRUE(var->data.explicit_location);
   EXPECT_FALSE(var->data.is_unmatched_generic_inout);
}

TEST_F(invalidate_locations, explicit_location_frac_vertex_in_generic)
{
   ir_variable *const var =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "a",
                               ir_var_shader_in);

   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_EQ(-1, var->data.location);

   var->data.location = VERT_ATTRIB_GENERIC0;
   var->data.location_frac = 2;
   var->data.explicit_location = true;

   ir.push_tail(var);

   link_invalidate_variable_locations(&ir);

   EXPECT_EQ(VERT_ATTRIB_GENERIC0, var->data.location);
   EXPECT_EQ(2u, var->data.location_frac);
   EXPECT_TRUE(var->data.explicit_location);
   EXPECT_FALSE(var->data.is_unmatched_generic_inout);
}

TEST_F(invalidate_locations, vertex_in_builtin)
{
   ir_variable *const var =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "gl_Vertex",
                               ir_var_shader_in);

   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_EQ(-1, var->data.location);

   var->data.location = VERT_ATTRIB_POS;
   var->data.explicit_location = true;

   ir.push_tail(var);

   link_invalidate_variable_locations(&ir);

   EXPECT_EQ(VERT_ATTRIB_POS, var->data.location);
   EXPECT_EQ(0u, var->data.location_frac);
   EXPECT_TRUE(var->data.explicit_location);
   EXPECT_FALSE(var->data.is_unmatched_generic_inout);
}

TEST_F(invalidate_locations, simple_vertex_out_generic)
{
   ir_variable *const var =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "a",
                               ir_var_shader_out);

   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_EQ(-1, var->data.location);

   var->data.location = VARYING_SLOT_VAR0;

   ir.push_tail(var);

   link_invalidate_variable_locations(&ir);

   EXPECT_EQ(-1, var->data.location);
   EXPECT_EQ(0u, var->data.location_frac);
   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_TRUE(var->data.is_unmatched_generic_inout);
}

TEST_F(invalidate_locations, vertex_out_builtin)
{
   ir_variable *const var =
      new(mem_ctx) ir_variable(glsl_type::vec(4),
                               "gl_FrontColor",
                               ir_var_shader_out);

   EXPECT_FALSE(var->data.explicit_location);
   EXPECT_EQ(-1, var->data.location);

   var->data.location = VARYING_SLOT_COL0;
   var->data.explicit_location = true;

   ir.push_tail(var);

   link_invalidate_variable_locations(&ir);

   EXPECT_EQ(VARYING_SLOT_COL0, var->data.location);
   EXPECT_EQ(0u, var->data.location_frac);
   EXPECT_TRUE(var->data.explicit_location);
   EXPECT_FALSE(var->data.is_unmatched_generic_inout);
}
