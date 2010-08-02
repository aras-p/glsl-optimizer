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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "ir_print_glsl_visitor.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"

static void print_type(const glsl_type *t);

void
_mesa_print_ir_glsl(exec_list *instructions,
	       struct _mesa_glsl_parse_state *state)
{
   if (state) {
      for (unsigned i = 0; i < state->num_user_structures; i++) {
	 const glsl_type *const s = state->user_structures[i];

	 printf("struct %s {\n",
		s->name);

	 for (unsigned j = 0; j < s->length; j++) {
	    printf("  ");
	    print_type(s->fields.structure[j].type);
	    printf(" %s;\n", s->fields.structure[j].name);
	 }

	 printf("};\n");
      }
   }

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
	  if (ir->ir_type == ir_type_variable) {
		ir_variable *var = static_cast<ir_variable*>(ir);
		if (strstr(var->name, "gl_") == var->name)
			continue;
	  }

	  ir_print_glsl_visitor v;
	  ir->accept(&v);
      if (ir->ir_type != ir_type_function)
		printf("\n");
   }
}


void ir_print_glsl_visitor::indent(void)
{
   for (int i = 0; i < indentation; i++)
      printf("  ");
}

static void
print_type(const glsl_type *t)
{
   if (t->base_type == GLSL_TYPE_ARRAY) {
      print_type(t->fields.array);
      printf("[%u]", t->length);
   } else if ((t->base_type == GLSL_TYPE_STRUCT)
	      && (strncmp("gl_", t->name, 3) != 0)) {
      printf("%s", t->name);
   } else {
      printf("%s", t->name);
   }
}


void ir_print_glsl_visitor::visit(ir_variable *ir)
{
   const char *const cent = (ir->centroid) ? "centroid " : "";
   const char *const inv = (ir->invariant) ? "invariant " : "";
   const char *const mode[] = { "", "uniform ", "in ", "out ", "inout ",
			        "" };
   const char *const interp[] = { "", "flat ", "noperspective " };

   printf("%s%s%s%s",
	  cent, inv, mode[ir->mode], interp[ir->interpolation]);
   print_type(ir->type);
   printf(" %s", ir->name);
   if (ir->mode == ir_var_temporary)
	   printf ("_%p", ir);
   printf(";");
}


void ir_print_glsl_visitor::visit(ir_function_signature *ir)
{
   print_type(ir->return_type);
   printf(" %s (\n", ir->function_name());

   indentation++;
   foreach_iter(exec_list_iterator, iter, ir->parameters) {
      ir_variable *const inst = (ir_variable *) iter.get();

      indent();
      inst->accept(this);
      printf("\n");
   }
   indentation--;

   indent();

   if (ir->body.is_empty())
   {
	   printf(");\n");
	   return;
   }

   printf(")\n");

   indent();
   printf("{\n");
   indentation++;

   foreach_iter(exec_list_iterator, iter, ir->body) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
	  printf("\n");
   }
   indentation--;
   indent();
   printf("}\n");
}


void ir_print_glsl_visitor::visit(ir_function *ir)
{
   bool found_non_builtin_proto = false;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();
      if (sig->is_defined || !sig->is_built_in)
	 found_non_builtin_proto = true;
   }
   if (!found_non_builtin_proto)
      return;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();

      indent();
      sig->accept(this);
      printf("\n");
   }
   indent();
}


void ir_print_glsl_visitor::visit(ir_expression *ir)
{
   if (ir->operands[0])
      ir->operands[0]->accept(this);

   printf(" %s ", ir->operator_string());

   if (ir->operands[1])
      ir->operands[1]->accept(this);
   printf(" ");
}


void ir_print_glsl_visitor::visit(ir_texture *ir)
{
   printf("(%s ", ir->opcode_string());

   ir->sampler->accept(this);
   printf(" ");

   ir->coordinate->accept(this);

   printf(" (%d %d %d) ", ir->offsets[0], ir->offsets[1], ir->offsets[2]);

   if (ir->op != ir_txf) {
      if (ir->projector)
	 ir->projector->accept(this);
      else
	 printf("1");

      if (ir->shadow_comparitor) {
	 printf(" ");
	 ir->shadow_comparitor->accept(this);
      } else {
	 printf(" ()");
      }
   }

   printf(" ");
   switch (ir->op)
   {
   case ir_tex:
      break;
   case ir_txb:
      ir->lod_info.bias->accept(this);
      break;
   case ir_txl:
   case ir_txf:
      ir->lod_info.lod->accept(this);
      break;
   case ir_txd:
      printf("(");
      ir->lod_info.grad.dPdx->accept(this);
      printf(" ");
      ir->lod_info.grad.dPdy->accept(this);
      printf(")");
      break;
   };
   printf(")");
}


void ir_print_glsl_visitor::visit(ir_swizzle *ir)
{
   const unsigned swiz[4] = {
      ir->mask.x,
      ir->mask.y,
      ir->mask.z,
      ir->mask.w,
   };

   ir->val->accept(this);
   printf(".");
   for (unsigned i = 0; i < ir->mask.num_components; i++) {
	   printf("%c", "xyzw"[swiz[i]]);
   }
}


void ir_print_glsl_visitor::visit(ir_dereference_variable *ir)
{
   ir_variable *var = ir->variable_referenced();
   printf("%s", var->name);
   if (var->mode == ir_var_temporary)
	   printf ("_%p", ir);
   printf(" ");
}


void ir_print_glsl_visitor::visit(ir_dereference_array *ir)
{
   ir->array->accept(this);
   printf("[");
   ir->array_index->accept(this);
   printf("] ");
}


void ir_print_glsl_visitor::visit(ir_dereference_record *ir)
{
   ir->record->accept(this);
   printf(".%s ", ir->field);
}


void ir_print_glsl_visitor::visit(ir_assignment *ir)
{
   if (ir->condition)
   {
      ir->condition->accept(this);
	  printf(" ");
   }

   ir->lhs->accept(this);

   printf(" = ");

   ir->rhs->accept(this);
   printf("; ");
}


void ir_print_glsl_visitor::visit(ir_constant *ir)
{
	if (ir->type == glsl_type::float_type)
	{
		printf("%f", ir->value.f[0]);
		return;
	}
	else if (ir->type == glsl_type::int_type)
	{
		printf("%d", ir->value.i[0]);
		return;
	}

   const glsl_type *const base_type = ir->type->get_base_type();

   print_type(ir->type);
   printf(" (");

   if (ir->type->is_array()) {
      for (unsigned i = 0; i < ir->type->length; i++)
	 ir->get_array_element(i)->accept(this);
   } else {
      for (unsigned i = 0; i < ir->type->components(); i++) {
	 if (i != 0)
	    printf(", ");
	 switch (base_type->base_type) {
	 case GLSL_TYPE_UINT:  printf("%u", ir->value.u[i]); break;
	 case GLSL_TYPE_INT:   printf("%d", ir->value.i[i]); break;
	 case GLSL_TYPE_FLOAT: printf("%f", ir->value.f[i]); break;
	 case GLSL_TYPE_BOOL:  printf("%d", ir->value.b[i]); break;
	 default: assert(0);
	 }
      }
   }
   printf(") ");
}


void
ir_print_glsl_visitor::visit(ir_call *ir)
{
   printf("%s (", ir->callee_name());
   bool first = true;
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_instruction *const inst = (ir_instruction *) iter.get();
	  if (!first)
		  printf (", ");
      inst->accept(this);
	  first = false;
   }
   printf(") ");
}


void
ir_print_glsl_visitor::visit(ir_return *ir)
{
   printf("return");

   ir_rvalue *const value = ir->get_value();
   if (value) {
      printf(" ");
      value->accept(this);
   }

   printf(";");
}


void
ir_print_glsl_visitor::visit(ir_discard *ir)
{
   printf("(discard ");

   if (ir->condition != NULL) {
      printf(" ");
      ir->condition->accept(this);
   }

   printf(")");
}


void
ir_print_glsl_visitor::visit(ir_if *ir)
{
   printf("if (");
   ir->condition->accept(this);

   printf(") {\n");
   indentation++;

   foreach_iter(exec_list_iterator, iter, ir->then_instructions) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
      printf("\n");
   }

   indentation--;
   indent();
   printf("}\n");

   if (!ir->else_instructions.is_empty())
   {
	   indent();
	   printf("else {\n");
	   indentation++;

	   foreach_iter(exec_list_iterator, iter, ir->else_instructions) {
		  ir_instruction *const inst = (ir_instruction *) iter.get();

		  indent();
		  inst->accept(this);
		  printf("\n");
	   }
	   indentation--;
	   indent();
	   printf("}\n");
   }
}


void
ir_print_glsl_visitor::visit(ir_loop *ir)
{
   printf("(loop (");
   if (ir->counter != NULL)
      ir->counter->accept(this);
   printf(") (");
   if (ir->from != NULL)
      ir->from->accept(this);
   printf(") (");
   if (ir->to != NULL)
      ir->to->accept(this);
   printf(") (");
   if (ir->increment != NULL)
      ir->increment->accept(this);
   printf(") (\n");
   indentation++;

   foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
      printf("\n");
   }
   indentation--;
   indent();
   printf("))\n");
}


void
ir_print_glsl_visitor::visit(ir_loop_jump *ir)
{
   printf("%s", ir->is_break() ? "break" : "continue");
}
