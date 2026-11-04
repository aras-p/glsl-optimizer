/*
 * Copyright © 2016 Intel Corporation
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

#ifndef OPT_ADD_NEG_TO_SUB_H
#define OPT_ADD_NEG_TO_SUB_H

#include "ir.h"
#include <cmath>
#include <climits>
#include "ir_hierarchical_visitor.h"

class add_neg_to_sub_visitor : public ir_hierarchical_visitor {
public:
   add_neg_to_sub_visitor()
   {
      /* empty */
   }

   ir_visitor_status visit_leave(ir_expression *ir)
   {
      /* glsl-optimizer change: recover a - b from -(b - a) after the
       * children have had their additions converted to subtraction. */
      if (ir->operation == ir_unop_neg) {
         ir_expression *op = ir->operands[0]->as_expression();
         if (op && op->operation == ir_binop_sub) {
            ir->operation = ir_binop_sub;
            ir->operands[0] = op->operands[1];
            ir->operands[1] = op->operands[0];
            ir->init_num_operands();
         }
      }
      if (ir->operation != ir_binop_add)
         return visit_continue;

      for (unsigned i = 0; i < 2; i++) {
         ir_expression *const op = ir->operands[i]->as_expression();

         /* glsl-optimizer change: constant folding turns -(1.0) into -1.0.
          * Restore subtraction for entirely negative constants too. Do not
          * negate INT_MIN or non-finite values, or change signed-zero cases. */
         ir_constant *constant = ir->operands[i]->as_constant();
         if (constant && (constant->type->is_scalar() || constant->type->is_vector())) {
            bool negative = true;
            for (unsigned c = 0; c < constant->type->components(); ++c) {
               switch (constant->type->base_type) {
               case GLSL_TYPE_FLOAT:
                  negative &= std::isfinite(constant->value.f[c]) && constant->value.f[c] < 0.0f;
                  break;
               case GLSL_TYPE_DOUBLE:
                  negative &= std::isfinite(constant->value.d[c]) && constant->value.d[c] < 0.0;
                  break;
               case GLSL_TYPE_INT:
                  negative &= constant->value.i[c] < 0 && constant->value.i[c] != INT_MIN;
                  break;
               default: negative = false; break;
               }
            }
            if (negative) {
               ir_constant *positive = constant->clone(ralloc_parent(ir), NULL);
               for (unsigned c = 0; c < positive->type->components(); ++c) {
                  switch (positive->type->base_type) {
                  case GLSL_TYPE_FLOAT: positive->value.f[c] = -positive->value.f[c]; break;
                  case GLSL_TYPE_DOUBLE: positive->value.d[c] = -positive->value.d[c]; break;
                  case GLSL_TYPE_INT: positive->value.i[c] = -positive->value.i[c]; break;
                  default: break;
                  }
               }
               ir->operation = ir_binop_sub;
               if (i == 0) ir->operands[0] = ir->operands[1];
               ir->operands[1] = positive;
               break;
            }
         }

         if (op != NULL && op->operation == ir_unop_neg) {
            ir->operation = ir_binop_sub;

            /* This ensures that -a + b becomes b - a. */
            if (i == 0)
               ir->operands[0] = ir->operands[1];

            ir->operands[1] = op->operands[0];
            break;
         }
      }

      return visit_continue;
   }
};

#endif /* OPT_ADD_NEG_TO_SUB_H */
