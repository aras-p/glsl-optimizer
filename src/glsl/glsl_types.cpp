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

#include <cstdio>
#include <stdlib.h>
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "builtin_types.h"
#include "hash_table.h"


hash_table *glsl_type::array_types = NULL;

static void
add_types_to_symbol_table(glsl_symbol_table *symtab,
			  const struct glsl_type *types,
			  unsigned num_types, bool warn)
{
   (void) warn;

   for (unsigned i = 0; i < num_types; i++) {
      symtab->add_type(types[i].name, & types[i]);
   }
}


static void
generate_110_types(glsl_symbol_table *symtab)
{
   add_types_to_symbol_table(symtab, builtin_core_types,
			     Elements(builtin_core_types),
			     false);
   add_types_to_symbol_table(symtab, builtin_structure_types,
			     Elements(builtin_structure_types),
			     false);
   add_types_to_symbol_table(symtab, builtin_110_deprecated_structure_types,
			     Elements(builtin_110_deprecated_structure_types),
			     false);
   add_types_to_symbol_table(symtab, & void_type, 1, false);
}


static void
generate_120_types(glsl_symbol_table *symtab)
{
   generate_110_types(symtab);

   add_types_to_symbol_table(symtab, builtin_120_types,
			     Elements(builtin_120_types), false);
}


static void
generate_130_types(glsl_symbol_table *symtab)
{
   generate_120_types(symtab);

   add_types_to_symbol_table(symtab, builtin_130_types,
			     Elements(builtin_130_types), false);
}


static void
generate_ARB_texture_rectangle_types(glsl_symbol_table *symtab, bool warn)
{
   add_types_to_symbol_table(symtab, builtin_ARB_texture_rectangle_types,
			     Elements(builtin_ARB_texture_rectangle_types),
			     warn);
}


static void
generate_EXT_texture_array_types(glsl_symbol_table *symtab, bool warn)
{
   add_types_to_symbol_table(symtab, builtin_EXT_texture_array_types,
			     Elements(builtin_EXT_texture_array_types),
			     warn);
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

   if (state->ARB_texture_rectangle_enable) {
      generate_ARB_texture_rectangle_types(state->symbols,
					   state->ARB_texture_rectangle_warn);
   }

   if (state->EXT_texture_array_enable && state->language_version < 130) {
      // These are already included in 130; don't create twice.
      generate_EXT_texture_array_types(state->symbols,
				       state->EXT_texture_array_warn);
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


ir_function *
glsl_type::generate_constructor(glsl_symbol_table *symtab) const
{
   void *ctx = symtab;

   /* Generate the function name and add it to the symbol table.
    */
   ir_function *const f = new(ctx) ir_function(name);

   bool added = symtab->add_function(name, f);
   assert(added);

   ir_function_signature *const sig = new(ctx) ir_function_signature(this);
   f->add_signature(sig);

   ir_variable **declarations =
      (ir_variable **) malloc(sizeof(ir_variable *) * this->length);
   for (unsigned i = 0; i < length; i++) {
      char *const param_name = (char *) malloc(10);

      snprintf(param_name, 10, "p%08X", i);

      ir_variable *var = (this->base_type == GLSL_TYPE_ARRAY)
	 ? new(ctx) ir_variable(fields.array, param_name)
	 : new(ctx) ir_variable(fields.structure[i].type, param_name);

      var->mode = ir_var_in;
      declarations[i] = var;
      sig->parameters.push_tail(var);
   }

   /* Generate the body of the constructor.  The body assigns each of the
    * parameters to a portion of a local variable called __retval that has
    * the same type as the constructor.  After initializing __retval,
    * __retval is returned.
    */
   ir_variable *retval = new(ctx) ir_variable(this, "__retval");
   sig->body.push_tail(retval);

   for (unsigned i = 0; i < length; i++) {
      ir_dereference *const lhs = (this->base_type == GLSL_TYPE_ARRAY)
	 ? (ir_dereference *) new(ctx) ir_dereference_array(retval,
							    new(ctx) ir_constant(i))
	 : (ir_dereference *) new(ctx) ir_dereference_record(retval,
							     fields.structure[i].name);

      ir_dereference *const rhs = new(ctx) ir_dereference_variable(declarations[i]);
      ir_instruction *const assign = new(ctx) ir_assignment(lhs, rhs, NULL);

      sig->body.push_tail(assign);
   }

   free(declarations);

   ir_dereference *const retref = new(ctx) ir_dereference_variable(retval);
   ir_instruction *const inst = new(ctx) ir_return(retref);
   sig->body.push_tail(inst);

   return f;
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
 * \param declarations Pointers to the variable declarations for the function
 *                     parameters.  These are used later to avoid having to use
 *                     the symbol table.
 */
static ir_function_signature *
generate_constructor_intro(void *ctx,
			   const glsl_type *type, unsigned parameter_count,
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

   ir_function_signature *const signature = new(ctx) ir_function_signature(type);

   for (unsigned i = 0; i < parameter_count; i++) {
      ir_variable *var = new(ctx) ir_variable(parameter_type, names[i]);

      var->mode = ir_var_in;
      signature->parameters.push_tail(var);

      declarations[i] = var;
   }

   ir_variable *retval = new(ctx) ir_variable(type, "__retval");
   signature->body.push_tail(retval);

   declarations[16] = retval;
   return signature;
}


/**
 * Generate the body of a vector constructor that takes a single scalar
 */
static void
generate_vec_body_from_scalar(void *ctx,
			      exec_list *instructions,
			      ir_variable **declarations)
{
   ir_instruction *inst;

   /* Generate a single assignment of the parameter to __retval.x and return
    * __retval.xxxx for however many vector components there are.
    */
   ir_dereference *const lhs_ref =
      new(ctx) ir_dereference_variable(declarations[16]);
   ir_dereference *const rhs = new(ctx) ir_dereference_variable(declarations[0]);

   ir_swizzle *lhs = new(ctx) ir_swizzle(lhs_ref, 0, 0, 0, 0, 1);

   inst = new(ctx) ir_assignment(lhs, rhs, NULL);
   instructions->push_tail(inst);

   ir_dereference *const retref = new(ctx) ir_dereference_variable(declarations[16]);

   ir_swizzle *retval = new(ctx) ir_swizzle(retref, 0, 0, 0, 0,
					    declarations[16]->type->vector_elements);

   inst = new(ctx) ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the body of a vector constructor that takes multiple scalars
 */
static void
generate_vec_body_from_N_scalars(void *ctx,
				 exec_list *instructions,
				 ir_variable **declarations)
{
   ir_instruction *inst;
   const glsl_type *const vec_type = declarations[16]->type;

   /* Generate an assignment of each parameter to a single component of
    * __retval.x and return __retval.
    */
   for (unsigned i = 0; i < vec_type->vector_elements; i++) {
      ir_dereference *const lhs_ref =
	 new(ctx) ir_dereference_variable(declarations[16]);
      ir_dereference *const rhs = new(ctx) ir_dereference_variable(declarations[i]);

      ir_swizzle *lhs = new(ctx) ir_swizzle(lhs_ref, i, 0, 0, 0, 1);

      inst = new(ctx) ir_assignment(lhs, rhs, NULL);
      instructions->push_tail(inst);
   }

   ir_dereference *retval = new(ctx) ir_dereference_variable(declarations[16]);

   inst = new(ctx) ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the body of a matrix constructor that takes a single scalar
 */
static void
generate_mat_body_from_scalar(void *ctx,
			      exec_list *instructions,
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

   ir_variable *const column = new(ctx) ir_variable(column_type, "v");

   instructions->push_tail(column);

   ir_dereference *const lhs_ref = new(ctx) ir_dereference_variable(column);
   ir_dereference *const rhs = new(ctx) ir_dereference_variable(declarations[0]);

   ir_swizzle *lhs = new(ctx) ir_swizzle(lhs_ref, 0, 0, 0, 0, 1);

   inst = new(ctx) ir_assignment(lhs, rhs, NULL);
   instructions->push_tail(inst);

   for (unsigned i = 1; i < column_type->vector_elements; i++) {
      ir_dereference *const lhs_ref = new(ctx) ir_dereference_variable(column);
      ir_constant *const zero = new(ctx) ir_constant(0.0f);

      ir_swizzle *lhs = new(ctx) ir_swizzle(lhs_ref, i, 0, 0, 0, 1);

      inst = new(ctx) ir_assignment(lhs, zero, NULL);
      instructions->push_tail(inst);
   }


   for (unsigned i = 0; i < row_type->vector_elements; i++) {
      static const unsigned swiz[] = { 1, 1, 1, 0, 1, 1, 1 };
      ir_dereference *const rhs_ref = new(ctx) ir_dereference_variable(column);

      /* This will be .xyyy when i=0, .yxyy when i=1, etc.
       */
      ir_swizzle *rhs = new(ctx) ir_swizzle(rhs_ref, swiz[3 - i], swiz[4 - i],
					    swiz[5 - i], swiz[6 - i],
					    column_type->vector_elements);

      ir_constant *const idx = new(ctx) ir_constant(int(i));
      ir_dereference *const lhs =
	 new(ctx) ir_dereference_array(declarations[16], idx);

      inst = new(ctx) ir_assignment(lhs, rhs, NULL);
      instructions->push_tail(inst);
   }

   ir_dereference *const retval = new(ctx) ir_dereference_variable(declarations[16]);
   inst = new(ctx) ir_return(retval);
   instructions->push_tail(inst);
}


/**
 * Generate the body of a vector constructor that takes multiple scalars
 */
static void
generate_mat_body_from_N_scalars(void *ctx,
				 exec_list *instructions,
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
	 ir_constant *row_index = new(ctx) ir_constant(int(i));
	 ir_dereference *const row_access =
	    new(ctx) ir_dereference_array(declarations[16], row_index);

	 ir_swizzle *component_access = new(ctx) ir_swizzle(row_access,
							    j, 0, 0, 0, 1);

	 const unsigned param = (i * row_type->vector_elements) + j;
	 ir_dereference *const rhs =
	    new(ctx) ir_dereference_variable(declarations[param]);

	 inst = new(ctx) ir_assignment(component_access, rhs, NULL);
	 instructions->push_tail(inst);
      }
   }

   ir_dereference *retval = new(ctx) ir_dereference_variable(declarations[16]);

   inst = new(ctx) ir_return(retval);
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
   void *ctx = symtab;
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

      /* Generate the function block, add it to the symbol table, and emit it.
       */
      ir_function *const f = new(ctx) ir_function(types[i].name);

      bool added = symtab->add_function(types[i].name, f);
      assert(added);

      instructions->push_tail(f);

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
      ir_function_signature *const sig =
         generate_constructor_intro(ctx, &types[i], 1, declarations);
      f->add_signature(sig);

      if (types[i].is_vector()) {
	 generate_vec_body_from_scalar(ctx, &sig->body, declarations);

	 ir_function_signature *const vec_sig =
	    generate_constructor_intro(ctx,
				       &types[i], types[i].vector_elements,
				       declarations);
	 f->add_signature(vec_sig);

	 generate_vec_body_from_N_scalars(ctx, &vec_sig->body, declarations);
      } else {
	 assert(types[i].is_matrix());

	 generate_mat_body_from_scalar(ctx, &sig->body, declarations);

	 ir_function_signature *const mat_sig =
	    generate_constructor_intro(ctx,
				       &types[i],
				       (types[i].vector_elements
					* types[i].matrix_columns),
				       declarations);
	 f->add_signature(mat_sig);

	 generate_mat_body_from_N_scalars(ctx, &mat_sig->body, declarations);
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


glsl_type::glsl_type(void *ctx, const glsl_type *array, unsigned length) :
   base_type(GLSL_TYPE_ARRAY),
   sampler_dimensionality(0), sampler_shadow(0), sampler_array(0),
   sampler_type(0),
   vector_elements(0), matrix_columns(0),
   name(NULL), length(length)
{
   this->fields.array = array;

   /* Allow a maximum of 10 characters for the array size.  This is enough
    * for 32-bits of ~0.  The extra 3 are for the '[', ']', and terminating
    * NUL.
    */
   const unsigned name_length = strlen(array->name) + 10 + 3;
   char *const n = (char *) talloc_size(ctx, name_length);

   if (length == 0)
      snprintf(n, name_length, "%s[]", array->name);
   else
      snprintf(n, name_length, "%s[%u]", array->name, length);

   this->name = n;
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


int
glsl_type::array_key_compare(const void *a, const void *b)
{
   const glsl_type *const key1 = (glsl_type *) a;
   const glsl_type *const key2 = (glsl_type *) b;

   /* Return zero is the types match (there is zero difference) or non-zero
    * otherwise.
    */
   return ((key1->fields.array == key2->fields.array)
	   && (key1->length == key2->length)) ? 0 : 1;
}


unsigned
glsl_type::array_key_hash(const void *a)
{
   const glsl_type *const key = (glsl_type *) a;

   const struct {
      const glsl_type *t;
      unsigned l;
      char nul;
   } hash_key = {
      key->fields.array,
      key->length,
      '\0'
   };

   return hash_table_string_hash(& hash_key);
}


const glsl_type *
glsl_type::get_array_instance(void *ctx, const glsl_type *base,
			      unsigned array_size)
{
   const glsl_type key(ctx, base, array_size);

   if (array_types == NULL) {
      array_types = hash_table_ctor(64, array_key_hash, array_key_compare);
   }

   const glsl_type *t = (glsl_type *) hash_table_find(array_types, & key);
   if (t == NULL) {
      t = new(ctx) glsl_type(ctx, base, array_size);

      hash_table_insert(array_types, (void *) t, t);
   }

   assert(t->base_type == GLSL_TYPE_ARRAY);
   assert(t->length == array_size);
   assert(t->fields.array == base);

   return t;
}


const glsl_type *
glsl_type::field_type(const char *name) const
{
   if (this->base_type != GLSL_TYPE_STRUCT)
      return error_type;

   for (unsigned i = 0; i < this->length; i++) {
      if (strcmp(name, this->fields.structure[i].name) == 0)
	 return this->fields.structure[i].type;
   }

   return error_type;
}


int
glsl_type::field_index(const char *name) const
{
   if (this->base_type != GLSL_TYPE_STRUCT)
      return -1;

   for (unsigned i = 0; i < this->length; i++) {
      if (strcmp(name, this->fields.structure[i].name) == 0)
	 return i;
   }

   return -1;
}


unsigned
glsl_type::component_slots() const
{
   switch (this->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return this->components();

   case GLSL_TYPE_STRUCT: {
      unsigned size = 0;

      for (unsigned i = 0; i < this->length; i++)
	 size += this->fields.structure[i].type->component_slots();

      return size;
   }

   case GLSL_TYPE_ARRAY:
      return this->length * this->fields.array->component_slots();

   default:
      return 0;
   }
}
