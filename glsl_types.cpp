/*
 * Copyright © 2009 Intel Corporation
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

#include <stdlib.h>
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "builtin_types.h"


static void
add_types_to_symbol_table(glsl_symbol_table *symtab,
			  const struct glsl_type *types,
			  unsigned num_types)
{
   unsigned i;

   for (i = 0; i < num_types; i++) {
      symtab->add_type(types[i].name, & types[i]);
   }
}


static void
generate_110_types(glsl_symbol_table *symtab)
{
   add_types_to_symbol_table(symtab, builtin_core_types,
			     Elements(builtin_core_types));
   add_types_to_symbol_table(symtab, builtin_structure_types,
			     Elements(builtin_structure_types));
   add_types_to_symbol_table(symtab, builtin_110_deprecated_structure_types,
			     Elements(builtin_110_deprecated_structure_types));
   add_types_to_symbol_table(symtab, & void_type, 1);
}


static void
generate_120_types(glsl_symbol_table *symtab)
{
   generate_110_types(symtab);

   add_types_to_symbol_table(symtab, builtin_120_types,
			     Elements(builtin_120_types));
}


static void
generate_130_types(glsl_symbol_table *symtab)
{
   generate_120_types(symtab);

   add_types_to_symbol_table(symtab, builtin_130_types,
			     Elements(builtin_130_types));
}


void
_mesa_glsl_initialize_types(struct _mesa_glsl_parse_state *state)
{
   switch (state->language_version) {
   case 110:
      generate_110_types(state->symbols);
      break;
   case 120:
      generate_120_types(state->symbols);
      break;
   case 130:
      generate_130_types(state->symbols);
      break;
   default:
      /* error */
      break;
   }
}


const glsl_type *glsl_type::get_base_type() const
{
   switch (base_type) {
   case GLSL_TYPE_UINT:
      return uint_type;
   case GLSL_TYPE_INT:
      return int_type;
   case GLSL_TYPE_FLOAT:
      return float_type;
   case GLSL_TYPE_BOOL:
      return bool_type;
   default:
      return error_type;
   }
}


/**
 * Generate the function intro for a constructor
 *
 * \param type         Data type to be constructed
 * \param count        Number of parameters to this concrete constructor.  Most
 *                     types have at least two constructors.  One will take a
 *                     single scalar parameter and the other will take "N"
 *                     scalar parameters.
 * \param parameters   Storage for the list of parameters.  These are
 *                     typically stored in an \c ir_function_signature.
 * \param instructions Storage for the preamble and body of the function.
 * \param declarations Pointers to the variable declarations for the function
 *                     parameters.  These are used later to avoid having to use
 *                     the symbol table.
 */
static ir_label *
generate_constructor_intro(const glsl_type *type, unsigned parameter_count,
			   exec_list *parameters, exec_list *instructions,
			   ir_variable **declarations)
{
   /* Names of parameters used in vector and matrix constructors
    */
   static const char *const names[] = {
      "a", "b", "c", "d", "e", "f", "g", "h",
      "i", "j", "k", "l", "m", "n", "o", "p",
   };

   assert(parameter_count <= Elements(names));

   const glsl_type *const parameter_type = type->get_base_type();

   ir_label *const label = new ir_label(type->name);
   instructions->push_tail(label);

   for (unsigned i = 0; i < parameter_count; i++) {
      ir_variable *var = new ir_variable(parameter_type, names[i]);

      var->mode = ir_var_in;
      parameters->push_tail(var);

      var = new ir_variable(parameter_type, names[i]);

      var->mode = ir_var_in;
      instructions->push_tail(var);

      declarations[i] = var;
   }

   ir_variable *retval = new ir_variable(type, "__retval");
   instructions->push_tail(retval);

   declarations[16] = retval;
   return label;
}


/**
 * Generate the body of a vector constructor that takes a single scalar
 */
static void
generate_vec_body_from_scalar(exec_list *instructions,
			      ir_variable **declarations)
{
   ir_instruction *inst;

   /* Generate a single assignment of the parameter to __retval.x and return
    * __retval.xxxx for however many vector components there are.
    */
   ir_dereference *const lhs_ref = new ir_dereference(declarations[16]);
   ir_dereference *const rhs = new ir_dereference(declarations[0]);

   ir_swizzle *lhs = new ir_swizzle(lhs_ref, 0, 0, 0, 0, 1);

   inst = new ir_assignment(lhs, rhs, NULL);
   instructions->push_tail(inst);

   ir_dereference *const retref = new ir_dereference(declarations[16]);

   ir_swizzle *retval = new ir_swizzle(retref, 0, 0, 0, 0,
                                       declarations[16]->type->vector_elements);

   inst = new ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the body of a vector constructor that takes multiple scalars
 */
static void
generate_vec_body_from_N_scalars(exec_list *instructions,
				 ir_variable **declarations)
{
   ir_instruction *inst;
   const glsl_type *const vec_type = declarations[16]->type;


   /* Generate an assignment of each parameter to a single component of
    * __retval.x and return __retval.
    */
   for (unsigned i = 0; i < vec_type->vector_elements; i++) {
      ir_dereference *const lhs_ref = new ir_dereference(declarations[16]);
      ir_dereference *const rhs = new ir_dereference(declarations[i]);

      ir_swizzle *lhs = new ir_swizzle(lhs_ref, 1, 0, 0, 0, 1);

      inst = new ir_assignment(lhs, rhs, NULL);
      instructions->push_tail(inst);
   }

   ir_dereference *retval = new ir_dereference(declarations[16]);

   inst = new ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the body of a matrix constructor that takes a single scalar
 */
static void
generate_mat_body_from_scalar(exec_list *instructions,
			      ir_variable **declarations)
{
   ir_instruction *inst;

   /* Generate an assignment of the parameter to the X component of a
    * temporary vector.  Set the remaining fields of the vector to 0.  The
    * size of the vector is equal to the number of rows of the matrix.
    *
    * Set each column of the matrix to a successive "rotation" of the
    * temporary vector.  This fills the matrix with 0s, but writes the single
    * scalar along the matrix's diagonal.
    *
    * For a mat4x3, this is equivalent to:
    *
    *   vec3 tmp;
    *   mat4x3 __retval;
    *   tmp.x = a;
    *   tmp.y = 0.0;
    *   tmp.z = 0.0;
    *   __retval[0] = tmp.xyy;
    *   __retval[1] = tmp.yxy;
    *   __retval[2] = tmp.yyx;
    *   __retval[3] = tmp.yyy;
    */
   const glsl_type *const column_type = declarations[16]->type->column_type();
   const glsl_type *const row_type = declarations[16]->type->row_type();
   ir_variable *const column = new ir_variable(column_type, "v");

   instructions->push_tail(column);

   ir_dereference *const lhs_ref = new ir_dereference(column);
   ir_dereference *const rhs = new ir_dereference(declarations[0]);

   ir_swizzle *lhs = new ir_swizzle(lhs_ref, 0, 0, 0, 0, 1);

   inst = new ir_assignment(lhs, rhs, NULL);
   instructions->push_tail(inst);

   const float z = 0.0f;
   ir_constant *const zero = new ir_constant(glsl_type::float_type, &z);

   for (unsigned i = 1; i < column_type->vector_elements; i++) {
      ir_dereference *const lhs_ref = new ir_dereference(column);

      ir_swizzle *lhs = new ir_swizzle(lhs_ref, i, 0, 0, 0, 1);

      inst = new ir_assignment(lhs, zero, NULL);
      instructions->push_tail(inst);
   }


   for (unsigned i = 0; i < row_type->vector_elements; i++) {
      static const unsigned swiz[] = { 1, 1, 1, 0, 1, 1, 1 };
      ir_dereference *const rhs_ref = new ir_dereference(column);

      /* This will be .xyyy when i=0, .yxyy when i=1, etc.
       */
      ir_swizzle *rhs = new ir_swizzle(rhs_ref, swiz[3 - i], swiz[4 - i],
                                       swiz[5 - i], swiz[6 - i],
				       column_type->vector_elements);

      ir_constant *const idx = new ir_constant(glsl_type::int_type, &i);
      ir_dereference *const lhs = new ir_dereference(declarations[16], idx);

      inst = new ir_assignment(lhs, rhs, NULL);
      instructions->push_tail(inst);
   }

   ir_dereference *const retval = new ir_dereference(declarations[16]);
   inst = new ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the body of a vector constructor that takes multiple scalars
 */
static void
generate_mat_body_from_N_scalars(exec_list *instructions,
				 ir_variable **declarations)
{
   ir_instruction *inst;
   const glsl_type *const row_type = declarations[16]->type->row_type();
   const glsl_type *const column_type = declarations[16]->type->column_type();


   /* Generate an assignment of each parameter to a single component of
    * of a particular column of __retval and return __retval.
    */
   for (unsigned i = 0; i < column_type->vector_elements; i++) {
      for (unsigned j = 0; j < row_type->vector_elements; j++) {
	 ir_constant *row_index = new ir_constant(glsl_type::int_type, &i);
	 ir_dereference *const row_access =
	    new ir_dereference(declarations[16], row_index);

	 ir_dereference *const component_access_ref =
	    new ir_dereference(row_access);

	 ir_swizzle *component_access = new ir_swizzle(component_access_ref,
	                                               j, 0, 0, 0, 1);

	 const unsigned param = (i * row_type->vector_elements) + j;
	 ir_dereference *const rhs = new ir_dereference(declarations[param]);

	 inst = new ir_assignment(component_access, rhs, NULL);
	 instructions->push_tail(inst);
      }
   }

   ir_dereference *retval = new ir_dereference(declarations[16]);

   inst = new ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the constructors for a set of GLSL types
 *
 * Constructor implementations are added to \c instructions, and the symbols
 * are added to \c symtab.
 */
static void
generate_constructor(glsl_symbol_table *symtab, const struct glsl_type *types,
		     unsigned num_types, exec_list *instructions)
{
   ir_variable *declarations[17];

   for (unsigned i = 0; i < num_types; i++) {
      /* Only numeric and boolean vectors and matrices get constructors here.
       * Structures need to be handled elsewhere.  It is expected that scalar
       * constructors are never actually called, so they are not generated.
       */
      if (!types[i].is_numeric() && !types[i].is_boolean())
	 continue;

      if (types[i].is_scalar())
	 continue;

      /* Generate the function name and add it to the symbol table.
       */
      ir_function *const f = new ir_function(types[i].name);

      bool added = symtab->add_function(types[i].name, f);
      assert(added);


      /* Each type has several basic constructors.  The total number of forms
       * depends on the derived type.
       *
       * Vectors:  1 scalar, N scalars
       * Matrices: 1 scalar, NxM scalars
       *
       * Several possible types of constructors are not included in this list.
       *
       * Scalar constructors are not included.  The expectation is that the
       * IR generator won't actually generate these as constructor calls.  The
       * expectation is that it will just generate the necessary type
       * conversion.
       *
       * Matrix contructors from matrices are also not included.  The
       * expectation is that the IR generator will generate a call to the
       * appropriate from-scalars constructor.
       */
      ir_function_signature *const sig = new ir_function_signature(& types[i]);
      f->signatures.push_tail(sig);

      sig->definition =
	 generate_constructor_intro(& types[i], 1, & sig->parameters,
				    instructions, declarations);

      if (types[i].is_vector()) {
	 generate_vec_body_from_scalar(instructions, declarations);

	 ir_function_signature *const vec_sig =
	    new ir_function_signature(& types[i]);
	 f->signatures.push_tail(vec_sig);

	 vec_sig->definition =
	    generate_constructor_intro(& types[i], types[i].vector_elements,
				       & vec_sig->parameters, instructions,
				       declarations);
	 generate_vec_body_from_N_scalars(instructions, declarations);
      } else {
	 assert(types[i].is_matrix());

	 generate_mat_body_from_scalar(instructions, declarations);

	 ir_function_signature *const mat_sig =
	    new ir_function_signature(& types[i]);
	 f->signatures.push_tail(mat_sig);

	 mat_sig->definition =
	    generate_constructor_intro(& types[i],
				       (types[i].vector_elements
					* types[i].matrix_columns),
				       & mat_sig->parameters, instructions,
				       declarations);
	 generate_mat_body_from_N_scalars(instructions, declarations);
      }
   }
}


void
generate_110_constructors(glsl_symbol_table *symtab, exec_list *instructions)
{
   generate_constructor(symtab, builtin_core_types,
			Elements(builtin_core_types), instructions);
}


void
generate_120_constructors(glsl_symbol_table *symtab, exec_list *instructions)
{
   generate_110_constructors(symtab, instructions);

   generate_constructor(symtab, builtin_120_types,
			Elements(builtin_120_types), instructions);
}


void
generate_130_constructors(glsl_symbol_table *symtab, exec_list *instructions)
{
   generate_120_constructors(symtab, instructions);

   generate_constructor(symtab, builtin_130_types,
			Elements(builtin_130_types), instructions);
}


void
_mesa_glsl_initialize_constructors(exec_list *instructions,
				   struct _mesa_glsl_parse_state *state)
{
   switch (state->language_version) {
   case 110:
      generate_110_constructors(state->symbols, instructions);
      break;
   case 120:
      generate_120_constructors(state->symbols, instructions);
      break;
   case 130:
      generate_130_constructors(state->symbols, instructions);
      break;
   default:
      /* error */
      break;
   }
}


const glsl_type *
glsl_type::get_instance(unsigned base_type, unsigned rows, unsigned columns)
{
   if (base_type == GLSL_TYPE_VOID)
      return &void_type;

   if ((rows < 1) || (rows > 4) || (columns < 1) || (columns > 4))
      return error_type;

   /* Treat GLSL vectors as Nx1 matrices.
    */
   if (columns == 1) {
      switch (base_type) {
      case GLSL_TYPE_UINT:
	 return uint_type + (rows - 1);
      case GLSL_TYPE_INT:
	 return int_type + (rows - 1);
      case GLSL_TYPE_FLOAT:
	 return float_type + (rows - 1);
      case GLSL_TYPE_BOOL:
	 return bool_type + (rows - 1);
      default:
	 return error_type;
      }
   } else {
      if ((base_type != GLSL_TYPE_FLOAT) || (rows == 1))
	 return error_type;

      /* GLSL matrix types are named mat{COLUMNS}x{ROWS}.  Only the following
       * combinations are valid:
       *
       *   1 2 3 4
       * 1
       * 2   x x x
       * 3   x x x
       * 4   x x x
       */
#define IDX(c,r) (((c-1)*3) + (r-1))

      switch (IDX(columns, rows)) {
      case IDX(2,2): return mat2_type;
      case IDX(2,3): return mat2x3_type;
      case IDX(2,4): return mat2x4_type;
      case IDX(3,2): return mat3x2_type;
      case IDX(3,3): return mat3_type;
      case IDX(3,4): return mat3x4_type;
      case IDX(4,2): return mat4x2_type;
      case IDX(4,3): return mat4x3_type;
      case IDX(4,4): return mat4_type;
      default: return error_type;
      }
   }

   assert(!"Should not get here.");
   return error_type;
}
