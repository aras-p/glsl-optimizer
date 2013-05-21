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

/**
 * \file link_interface_blocks.cpp
 * Linker support for GLSL's interface blocks.
 */

#include "ir.h"
#include "glsl_symbol_table.h"
#include "linker.h"
#include "main/macros.h"

bool
validate_intrastage_interface_blocks(const gl_shader **shader_list,
                                     unsigned num_shaders)
{
   glsl_symbol_table interfaces;

   for (unsigned int i = 0; i < num_shaders; i++) {
      if (shader_list[i] == NULL)
         continue;

      foreach_list(node, shader_list[i]->ir) {
         ir_variable *var = ((ir_instruction *) node)->as_variable();
         if (!var)
            continue;

         const glsl_type *iface_type = var->interface_type;

         if (iface_type == NULL)
            continue;

         const glsl_type *old_iface_type =
            interfaces.get_interface(iface_type->name,
                                     (enum ir_variable_mode) var->mode);

         if (old_iface_type == NULL) {
            /* This is the first time we've seen the interface, so save
             * it into our symbol table.
             */
            interfaces.add_interface(iface_type->name, iface_type,
                                     (enum ir_variable_mode) var->mode);
         } else if (old_iface_type != iface_type) {
            return false;
         }
      }
   }

   return true;
}

bool
validate_interstage_interface_blocks(const gl_shader *producer,
                                     const gl_shader *consumer)
{
   glsl_symbol_table interfaces;

   /* Add non-output interfaces from the consumer to the symbol table. */
   foreach_list(node, consumer->ir) {
      ir_variable *var = ((ir_instruction *) node)->as_variable();
      if (!var || !var->interface_type || var->mode == ir_var_shader_out)
         continue;

      interfaces.add_interface(var->interface_type->name,
                               var->interface_type,
                               (enum ir_variable_mode) var->mode);
   }

   /* Verify that the producer's interfaces match. */
   foreach_list(node, producer->ir) {
      ir_variable *var = ((ir_instruction *) node)->as_variable();
      if (!var || !var->interface_type || var->mode == ir_var_shader_in)
         continue;

      enum ir_variable_mode consumer_mode =
         var->mode == ir_var_uniform ? ir_var_uniform : ir_var_shader_in;
      const glsl_type *expected_type =
         interfaces.get_interface(var->interface_type->name, consumer_mode);

      /* The consumer doesn't use this output block.  Ignore it. */
      if (expected_type == NULL)
         continue;

      if (var->interface_type != expected_type)
         return false;
   }

   return true;
}
