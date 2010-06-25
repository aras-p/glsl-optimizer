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

#include "glsl_symbol_table.h"
#include "ast.h"
#include "glsl_types.h"
#include "ir.h"

inline unsigned min(unsigned a, unsigned b)
{
   return (a < b) ? a : b;
}

static unsigned
process_parameters(exec_list *instructions, exec_list *actual_parameters,
		   exec_list *parameters,
		   struct _mesa_glsl_parse_state *state)
{
   unsigned count = 0;

   foreach_list (n, parameters) {
      ast_node *const ast = exec_node_data(ast_node, n, link);
      ir_rvalue *result = ast->hir(instructions, state);

      ir_constant *const constant = result->constant_expression_value();
      if (constant != NULL)
	 result = constant;

      actual_parameters->push_tail(result);
      count++;
   }

   return count;
}


static ir_rvalue *
process_call(exec_list *instructions, ir_function *f,
	     YYLTYPE *loc, exec_list *actual_parameters,
	     struct _mesa_glsl_parse_state *state)
{
   void *ctx = talloc_parent(state);

   const ir_function_signature *sig =
      f->matching_signature(actual_parameters);

   /* The instructions param will be used when the FINISHMEs below are done */
   (void) instructions;

   if (sig != NULL) {
      /* Verify that 'out' and 'inout' actual parameters are lvalues.  This
       * isn't done in ir_function::matching_signature because that function
       * cannot generate the necessary diagnostics.
       */
      exec_list_iterator actual_iter = actual_parameters->iterator();
      exec_list_iterator formal_iter = sig->parameters.iterator();

      while (actual_iter.has_next()) {
	 ir_rvalue *actual = (ir_rvalue *) actual_iter.get();
	 ir_variable *formal = (ir_variable *) formal_iter.get();

	 assert(actual != NULL);
	 assert(formal != NULL);

	 if ((formal->mode == ir_var_out)
	     || (formal->mode == ir_var_inout)) {
	    if (! actual->is_lvalue()) {
	       /* FINISHME: Log a better diagnostic here.  There is no way
		* FINISHME: to tell the user which parameter is invalid.
		*/
	       _mesa_glsl_error(loc, state, "`%s' parameter is not lvalue",
				(formal->mode == ir_var_out) ? "out" : "inout");
	    }
	 }

	 actual_iter.next();
	 formal_iter.next();
      }

      /* FINISHME: The list of actual parameters needs to be modified to
       * FINISHME: include any necessary conversions.
       */
      return new(ctx) ir_call(sig, actual_parameters);
   } else {
      /* FINISHME: Log a better error message here.  G++ will show the types
       * FINISHME: of the actual parameters and the set of candidate
       * FINISHME: functions.  A different error should also be logged when
       * FINISHME: multiple functions match.
       */
      _mesa_glsl_error(loc, state, "no matching function for call to `%s'",
		       f->name);
      return ir_call::get_error_instruction(ctx);
   }
}


static ir_rvalue *
match_function_by_name(exec_list *instructions, const char *name,
		       YYLTYPE *loc, exec_list *actual_parameters,
		       struct _mesa_glsl_parse_state *state)
{
   void *ctx = talloc_parent(state);
   ir_function *f = state->symbols->get_function(name);

   if (f == NULL) {
      _mesa_glsl_error(loc, state, "function `%s' undeclared", name);
      return ir_call::get_error_instruction(ctx);
   }

   /* Once we've determined that the function being called might exist, try
    * to find an overload of the function that matches the parameters.
    */
   return process_call(instructions, f, loc, actual_parameters, state);
}


/**
 * Perform automatic type conversion of constructor parameters
 */
static ir_rvalue *
convert_component(ir_rvalue *src, const glsl_type *desired_type)
{
   void *ctx = talloc_parent(src);
   const unsigned a = desired_type->base_type;
   const unsigned b = src->type->base_type;
   ir_expression *result = NULL;

   if (src->type->is_error())
      return src;

   assert(a <= GLSL_TYPE_BOOL);
   assert(b <= GLSL_TYPE_BOOL);

   if ((a == b) || (src->type->is_integer() && desired_type->is_integer()))
      return src;

   switch (a) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
      if (b == GLSL_TYPE_FLOAT)
	 result = new(ctx) ir_expression(ir_unop_f2i, desired_type, src, NULL);
      else {
	 assert(b == GLSL_TYPE_BOOL);
	 result = new(ctx) ir_expression(ir_unop_b2i, desired_type, src, NULL);
      }
      break;
   case GLSL_TYPE_FLOAT:
      switch (b) {
      case GLSL_TYPE_UINT:
	 result = new(ctx) ir_expression(ir_unop_u2f, desired_type, src, NULL);
	 break;
      case GLSL_TYPE_INT:
	 result = new(ctx) ir_expression(ir_unop_i2f, desired_type, src, NULL);
	 break;
      case GLSL_TYPE_BOOL:
	 result = new(ctx) ir_expression(ir_unop_b2f, desired_type, src, NULL);
	 break;
      }
      break;
   case GLSL_TYPE_BOOL:
      switch (b) {
      case GLSL_TYPE_UINT:
      case GLSL_TYPE_INT:
	 result = new(ctx) ir_expression(ir_unop_i2b, desired_type, src, NULL);
	 break;
      case GLSL_TYPE_FLOAT:
	 result = new(ctx) ir_expression(ir_unop_f2b, desired_type, src, NULL);
	 break;
      }
      break;
   }

   assert(result != NULL);

   ir_constant *const constant = result->constant_expression_value();
   return (constant != NULL) ? (ir_rvalue *) constant : (ir_rvalue *) result;
}


/**
 * Dereference a specific component from a scalar, vector, or matrix
 */
static ir_rvalue *
dereference_component(ir_rvalue *src, unsigned component)
{
   void *ctx = talloc_parent(src);
   assert(component < src->type->components());

   /* If the source is a constant, just create a new constant instead of a
    * dereference of the existing constant.
    */
   ir_constant *constant = src->as_constant();
   if (constant)
      return new(ctx) ir_constant(constant, component);

   if (src->type->is_scalar()) {
      return src;
   } else if (src->type->is_vector()) {
      return new(ctx) ir_swizzle(src, component, 0, 0, 0, 1);
   } else {
      assert(src->type->is_matrix());

      /* Dereference a row of the matrix, then call this function again to get
       * a specific element from that row.
       */
      const int c = component / src->type->column_type()->vector_elements;
      const int r = component % src->type->column_type()->vector_elements;
      ir_constant *const col_index = new(ctx) ir_constant(c);
      ir_dereference *const col = new(ctx) ir_dereference_array(src, col_index);

      col->type = src->type->column_type();

      return dereference_component(col, r);
   }

   assert(!"Should not get here.");
   return NULL;
}


static ir_rvalue *
process_array_constructor(exec_list *instructions,
			  const glsl_type *constructor_type,
			  YYLTYPE *loc, exec_list *parameters,
			  struct _mesa_glsl_parse_state *state)
{
   void *ctx = talloc_parent(state);
   /* Array constructors come in two forms: sized and unsized.  Sized array
    * constructors look like 'vec4[2](a, b)', where 'a' and 'b' are vec4
    * variables.  In this case the number of parameters must exactly match the
    * specified size of the array.
    *
    * Unsized array constructors look like 'vec4[](a, b)', where 'a' and 'b'
    * are vec4 variables.  In this case the size of the array being constructed
    * is determined by the number of parameters.
    *
    * From page 52 (page 58 of the PDF) of the GLSL 1.50 spec:
    *
    *    "There must be exactly the same number of arguments as the size of
    *    the array being constructed. If no size is present in the
    *    constructor, then the array is explicitly sized to the number of
    *    arguments provided. The arguments are assigned in order, starting at
    *    element 0, to the elements of the constructed array. Each argument
    *    must be the same type as the element type of the array, or be a type
    *    that can be converted to the element type of the array according to
    *    Section 4.1.10 "Implicit Conversions.""
    */
   exec_list actual_parameters;
   const unsigned parameter_count =
      process_parameters(instructions, &actual_parameters, parameters, state);

   if ((parameter_count == 0)
       || ((constructor_type->length != 0)
	   && (constructor_type->length != parameter_count))) {
      const unsigned min_param = (constructor_type->length == 0)
	 ? 1 : constructor_type->length;

      _mesa_glsl_error(loc, state, "array constructor must have %s %u "
		       "parameter%s",
		       (constructor_type->length != 0) ? "at least" : "exactly",
		       min_param, (min_param <= 1) ? "" : "s");
      return ir_call::get_error_instruction(ctx);
   }

   if (constructor_type->length == 0) {
      constructor_type =
	 glsl_type::get_array_instance(state,
				       constructor_type->element_type(),
				       parameter_count);
      assert(constructor_type != NULL);
      assert(constructor_type->length == parameter_count);
   }

   ir_function *f = state->symbols->get_function(constructor_type->name);

   /* If the constructor for this type of array does not exist, generate the
    * prototype and add it to the symbol table.
    */
   if (f == NULL) {
      f = constructor_type->generate_constructor(state->symbols);
   }

   ir_rvalue *const r =
      process_call(instructions, f, loc, &actual_parameters, state);

   assert(r != NULL);
   assert(r->type->is_error() || (r->type == constructor_type));

   return r;
}


/**
 * Try to convert a record constructor to a constant expression
 */
static ir_constant *
constant_record_constructor(const glsl_type *constructor_type,
			    YYLTYPE *loc, exec_list *parameters,
			    struct _mesa_glsl_parse_state *state)
{
   void *ctx = talloc_parent(state);
   bool all_parameters_are_constant = true;

   exec_node *node = parameters->head;
   for (unsigned i = 0; i < constructor_type->length; i++) {
      ir_instruction *ir = (ir_instruction *) node;

      if (node->is_tail_sentinal()) {
	 _mesa_glsl_error(loc, state,
			  "insufficient parameters to constructor for `%s'",
			  constructor_type->name);
	 return NULL;
      }

      if (ir->type != constructor_type->fields.structure[i].type) {
	 _mesa_glsl_error(loc, state,
			  "parameter type mismatch in constructor for `%s' "
			  " (%s vs %s)",
			  constructor_type->name,
			  ir->type->name,
			  constructor_type->fields.structure[i].type->name);
	 return NULL;
      }

      if (ir->as_constant() == NULL)
	 all_parameters_are_constant = false;

      node = node->next;
   }

   if (!all_parameters_are_constant)
      return NULL;

   return new(ctx) ir_constant(constructor_type, parameters);
}


/**
 * Generate data for a constant matrix constructor w/a single scalar parameter
 *
 * Matrix constructors in GLSL can be passed a single scalar of the
 * approriate type.  In these cases, the resulting matrix is the identity
 * matrix multipled by the specified scalar.  This function generates data for
 * that matrix.
 *
 * \param type         Type of the desired matrix.
 * \param initializer  Scalar value used to initialize the matrix diagonal.
 * \param data         Location to store the resulting matrix.
 */
void
generate_constructor_matrix(const glsl_type *type, ir_constant *initializer,
			    ir_constant_data *data)
{
   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
      for (unsigned i = 0; i < type->components(); i++)
	 data->u[i] = 0;

      for (unsigned i = 0; i < type->matrix_columns; i++) {
	 /* The array offset of the ith row and column of the matrix.
	  */
	 const unsigned idx = (i * type->vector_elements) + i;

	 data->u[idx] = initializer->value.u[0];
      }
      break;

   case GLSL_TYPE_FLOAT:
      for (unsigned i = 0; i < type->components(); i++)
	 data->f[i] = 0;

      for (unsigned i = 0; i < type->matrix_columns; i++) {
	 /* The array offset of the ith row and column of the matrix.
	  */
	 const unsigned idx = (i * type->vector_elements) + i;

	 data->f[idx] = initializer->value.f[0];
      }

      break;

   default:
      assert(!"Should not get here.");
      break;
   }
}


/**
 * Generate data for a constant vector constructor w/a single scalar parameter
 *
 * Vector constructors in GLSL can be passed a single scalar of the
 * approriate type.  In these cases, the resulting vector contains the specified
 * value in all components.  This function generates data for that vector.
 *
 * \param type         Type of the desired vector.
 * \param initializer  Scalar value used to initialize the vector.
 * \param data         Location to store the resulting vector data.
 */
void
generate_constructor_vector(const glsl_type *type, ir_constant *initializer,
			    ir_constant_data *data)
{
   switch (type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
      for (unsigned i = 0; i < type->components(); i++)
	 data->u[i] = initializer->value.u[0];

      break;

   case GLSL_TYPE_FLOAT:
      for (unsigned i = 0; i < type->components(); i++)
	 data->f[i] = initializer->value.f[0];

      break;

   case GLSL_TYPE_BOOL:
      for (unsigned i = 0; i < type->components(); i++)
	 data->b[i] = initializer->value.b[0];

      break;

   default:
      assert(!"Should not get here.");
      break;
   }
}


/**
 * Determine if a list consists of a single scalar r-value
 */
bool
single_scalar_parameter(exec_list *parameters)
{
   const ir_rvalue *const p = (ir_rvalue *) parameters->head;
   assert(((ir_rvalue *)p)->as_rvalue() != NULL);

   return (p->type->is_scalar() && p->next->is_tail_sentinal());
}


/**
 * Generate inline code for a vector constructor
 *
 * The generated constructor code will consist of a temporary variable
 * declaration of the same type as the constructor.  A sequence of assignments
 * from constructor parameters to the temporary will follow.
 *
 * \return
 * An \c ir_dereference_variable of the temprorary generated in the constructor
 * body.
 */
ir_rvalue *
emit_inline_vector_constructor(const glsl_type *type,
			       exec_list *instructions,
			       exec_list *parameters,
			       void *ctx)
{
   assert(!parameters->is_empty());

   ir_variable *var = new(ctx) ir_variable(type, strdup("vec_ctor"));
   instructions->push_tail(var);

   /* There are two kinds of vector constructors.
    *
    *  - Construct a vector from a single scalar by replicating that scalar to
    *    all components of the vector.
    *
    *  - Construct a vector from an arbirary combination of vectors and
    *    scalars.  The components of the constructor parameters are assigned
    *    to the vector in order until the vector is full.
    */
   const unsigned lhs_components = type->components();
   if (single_scalar_parameter(parameters)) {
      ir_rvalue *first_param = (ir_rvalue *)parameters->head;
      ir_rvalue *rhs = new(ctx) ir_swizzle(first_param, 0, 0, 0, 0,
					   lhs_components);
      ir_dereference_variable *lhs = new(ctx) ir_dereference_variable(var);

      assert(rhs->type == lhs->type);

      ir_instruction *inst = new(ctx) ir_assignment(lhs, rhs, NULL);
      instructions->push_tail(inst);
   } else {
      unsigned base_component = 0;
      foreach_list(node, parameters) {
	 ir_rvalue *rhs = (ir_rvalue *) node;
	 unsigned rhs_components = rhs->type->components();

	 /* Do not try to assign more components to the vector than it has!
	  */
	 if ((rhs_components + base_component) > lhs_components) {
	    rhs_components = lhs_components - base_component;
	 }

	 /* Emit an assignment of the constructor parameter to the next set of
	  * components in the temporary variable.
	  */
	 unsigned mask[4] = { 0, 0, 0, 0 };
	 for (unsigned i = 0; i < rhs_components; i++) {
	    mask[i] = i + base_component;
	 }


	 ir_rvalue *lhs_ref = new(ctx) ir_dereference_variable(var);
	 ir_swizzle *lhs = new(ctx) ir_swizzle(lhs_ref, mask, rhs_components);

	 ir_instruction *inst = new(ctx) ir_assignment(lhs, rhs, NULL);
	 instructions->push_tail(inst);

	 /* Advance the component index by the number of components that were
	  * just assigned.
	  */
	 base_component += rhs_components;
      }
   }
   return new(ctx) ir_dereference_variable(var);
}


/**
 * Generate assignment of a portion of a vector to a portion of a matrix column
 *
 * \param src_base  First component of the source to be used in assignment
 * \param column    Column of destination to be assiged
 * \param row_base  First component of the destination column to be assigned
 * \param count     Number of components to be assigned
 *
 * \note
 * \c src_base + \c count must be less than or equal to the number of components
 * in the source vector.
 */
ir_instruction *
assign_to_matrix_column(ir_variable *var, unsigned column, unsigned row_base,
			ir_rvalue *src, unsigned src_base, unsigned count,
			TALLOC_CTX *ctx)
{
   const unsigned mask[8] = { 0, 1, 2, 3, 0, 0, 0, 0 };

   ir_constant *col_idx = new(ctx) ir_constant(column);
   ir_rvalue *column_ref = new(ctx) ir_dereference_array(var, col_idx);

   assert(column_ref->type->components() >= (row_base + count));
   ir_rvalue *lhs = new(ctx) ir_swizzle(column_ref, &mask[row_base], count);

   assert(src->type->components() >= (src_base + count));
   ir_rvalue *rhs = new(ctx) ir_swizzle(src, &mask[src_base], count);

   return new(ctx) ir_assignment(lhs, rhs, NULL);
}


/**
 * Generate inline code for a matrix constructor
 *
 * The generated constructor code will consist of a temporary variable
 * declaration of the same type as the constructor.  A sequence of assignments
 * from constructor parameters to the temporary will follow.
 *
 * \return
 * An \c ir_dereference_variable of the temprorary generated in the constructor
 * body.
 */
ir_rvalue *
emit_inline_matrix_constructor(const glsl_type *type,
			       exec_list *instructions,
			       exec_list *parameters,
			       void *ctx)
{
   assert(!parameters->is_empty());

   ir_variable *var = new(ctx) ir_variable(type, strdup("mat_ctor"));
   instructions->push_tail(var);

   /* There are three kinds of matrix constructors.
    *
    *  - Construct a matrix from a single scalar by replicating that scalar to
    *    along the diagonal of the matrix and setting all other components to
    *    zero.
    *
    *  - Construct a matrix from an arbirary combination of vectors and
    *    scalars.  The components of the constructor parameters are assigned
    *    to the matrix in colum-major order until the matrix is full.
    *
    *  - Construct a matrix from a single matrix.  The source matrix is copied
    *    to the upper left portion of the constructed matrix, and the remaining
    *    elements take values from the identity matrix.
    */
   ir_rvalue *const first_param = (ir_rvalue *) parameters->head;
   if (single_scalar_parameter(parameters)) {
      /* Assign the scalar to the X component of a vec4, and fill the remaining
       * components with zero.
       */
      ir_variable *rhs_var = new(ctx) ir_variable(glsl_type::vec4_type,
						  strdup("mat_ctor_vec"));
      instructions->push_tail(rhs_var);

      ir_constant_data zero;
      zero.f[0] = 0.0;
      zero.f[1] = 0.0;
      zero.f[2] = 0.0;
      zero.f[3] = 0.0;

      ir_instruction *inst =
	 new(ctx) ir_assignment(new(ctx) ir_dereference_variable(rhs_var),
				new(ctx) ir_constant(rhs_var->type, &zero),
				NULL);
      instructions->push_tail(inst);

      ir_rvalue *const rhs_ref = new(ctx) ir_dereference_variable(rhs_var);
      ir_rvalue *const x_of_rhs = new(ctx) ir_swizzle(rhs_ref, 0, 0, 0, 0, 1);

      inst = new(ctx) ir_assignment(x_of_rhs, first_param, NULL);
      instructions->push_tail(inst);

      /* Assign the temporary vector to each column of the destination matrix
       * with a swizzle that puts the X component on the diagonal of the
       * matrix.  In some cases this may mean that the X component does not
       * get assigned into the column at all (i.e., when the matrix has more
       * columns than rows).
       */
      static const unsigned rhs_swiz[4][4] = {
	 { 0, 1, 1, 1 },
	 { 1, 0, 1, 1 },
	 { 1, 1, 0, 1 },
	 { 1, 1, 1, 0 }
      };

      const unsigned cols_to_init = min(type->matrix_columns,
					type->vector_elements);
      for (unsigned i = 0; i < cols_to_init; i++) {
	 ir_constant *const col_idx = new(ctx) ir_constant(i);
	 ir_rvalue *const col_ref = new(ctx) ir_dereference_array(var, col_idx);

	 ir_rvalue *const rhs_ref = new(ctx) ir_dereference_variable(rhs_var);
	 ir_rvalue *const rhs = new(ctx) ir_swizzle(rhs_ref, rhs_swiz[i],
						    type->vector_elements);

	 inst = new(ctx) ir_assignment(col_ref, rhs, NULL);
	 instructions->push_tail(inst);
      }

      for (unsigned i = cols_to_init; i < type->matrix_columns; i++) {
	 ir_constant *const col_idx = new(ctx) ir_constant(i);
	 ir_rvalue *const col_ref = new(ctx) ir_dereference_array(var, col_idx);

	 ir_rvalue *const rhs_ref = new(ctx) ir_dereference_variable(rhs_var);
	 ir_rvalue *const rhs = new(ctx) ir_swizzle(rhs_ref, 1, 1, 1, 1,
						    type->vector_elements);

	 inst = new(ctx) ir_assignment(col_ref, rhs, NULL);
	 instructions->push_tail(inst);
      }
   } else if (first_param->type->is_matrix()) {
      /* From page 50 (56 of the PDF) of the GLSL 1.50 spec:
       *
       *     "If a matrix is constructed from a matrix, then each component
       *     (column i, row j) in the result that has a corresponding
       *     component (column i, row j) in the argument will be initialized
       *     from there. All other components will be initialized to the
       *     identity matrix. If a matrix argument is given to a matrix
       *     constructor, it is an error to have any other arguments."
       */
      assert(first_param->next->is_tail_sentinal());
      ir_rvalue *const src_matrix = first_param;

      /* If the source matrix is smaller, pre-initialize the relavent parts of
       * the destination matrix to the identity matrix.
       */
      if ((src_matrix->type->matrix_columns < var->type->matrix_columns)
	  || (src_matrix->type->vector_elements < var->type->vector_elements)) {

	 /* If the source matrix has fewer rows, every column of the destination
	  * must be initialized.  Otherwise only the columns in the destination
	  * that do not exist in the source must be initialized.
	  */
	 unsigned col =
	    (src_matrix->type->vector_elements < var->type->vector_elements)
	    ? 0 : src_matrix->type->matrix_columns;

	 const glsl_type *const col_type = var->type->column_type();
	 for (/* empty */; col < var->type->matrix_columns; col++) {
	    ir_constant_data ident;

	    ident.f[0] = 0.0;
	    ident.f[1] = 0.0;
	    ident.f[2] = 0.0;
	    ident.f[3] = 0.0;

	    ident.f[col] = 1.0;

	    ir_rvalue *const rhs = new(ctx) ir_constant(col_type, &ident);

	    ir_rvalue *const lhs =
	       new(ctx) ir_dereference_array(var, new(ctx) ir_constant(col));

	    ir_instruction *inst = new(ctx) ir_assignment(lhs, rhs, NULL);
	    instructions->push_tail(inst);
	 }
      }

      /* Assign columns from the source matrix to the destination matrix.
       *
       * Since the parameter will be used in the RHS of multiple assignments,
       * generate a temporary and copy the paramter there.
       */
      ir_variable *const rhs_var = new(ctx) ir_variable(first_param->type,
							strdup("mat_ctor_mat"));
      instructions->push_tail(rhs_var);

      ir_dereference *const rhs_var_ref =
	 new(ctx) ir_dereference_variable(rhs_var);
      ir_instruction *const inst =
	 new(ctx) ir_assignment(rhs_var_ref, first_param, NULL);
      instructions->push_tail(inst);


      const unsigned swiz[4] = { 0, 1, 2, 3 };
      const unsigned last_col = min(src_matrix->type->matrix_columns,
				    var->type->matrix_columns);
      for (unsigned i = 0; i < last_col; i++) {
	 ir_rvalue *const lhs_col =
	    new(ctx) ir_dereference_array(var, new(ctx) ir_constant(i));
	 ir_rvalue *const rhs_col =
	    new(ctx) ir_dereference_array(rhs_var, new(ctx) ir_constant(i));

	 /* If one matrix has columns that are smaller than the columns of the
	  * other matrix, wrap the column access of the larger with a swizzle
	  * so that the LHS and RHS of the assignment have the same size (and
	  * therefore have the same type).
	  *
	  * It would be perfectly valid to unconditionally generate the
	  * swizzles, this this will typically result in a more compact IR tree.
	  */
	 ir_rvalue *lhs;
	 ir_rvalue *rhs;
	 if (lhs_col->type->vector_elements < rhs_col->type->vector_elements) {
	    lhs = lhs_col;

	    rhs = new(ctx) ir_swizzle(rhs_col, swiz,
				      lhs_col->type->vector_elements);
	 } else if (lhs_col->type->vector_elements
		    > rhs_col->type->vector_elements) {
	    lhs = new(ctx) ir_swizzle(lhs_col, swiz,
				      rhs_col->type->vector_elements);
	    rhs = rhs_col;
	 } else {
	    lhs = lhs_col;
	    rhs = rhs_col;
	 }

	 assert(lhs->type == rhs->type);

	 ir_instruction *inst = new(ctx) ir_assignment(lhs, rhs, NULL);
	 instructions->push_tail(inst);
      }
   } else {
      const unsigned rows = type->matrix_columns;
      const unsigned cols = type->vector_elements;
      unsigned col_idx = 0;
      unsigned row_idx = 0;

      foreach_list (node, parameters) {
	 ir_rvalue *const rhs = (ir_rvalue *) node;
	 const unsigned components_remaining_this_column = rows - row_idx;
	 unsigned rhs_components = rhs->type->components();
	 unsigned rhs_base = 0;

	 /* Since the parameter might be used in the RHS of two assignments,
	  * generate a temporary and copy the paramter there.
	  */
	 ir_variable *rhs_var = new(ctx) ir_variable(rhs->type,
						     strdup("mat_ctor_vec"));
	 instructions->push_tail(rhs_var);

	 ir_dereference *rhs_var_ref =
	    new(ctx) ir_dereference_variable(rhs_var);
	 ir_instruction *inst = new(ctx) ir_assignment(rhs_var_ref, rhs, NULL);
	 instructions->push_tail(inst);

	 /* Assign the current parameter to as many components of the matrix
	  * as it will fill.
	  *
	  * NOTE: A single vector parameter can span two matrix columns.  A
	  * single vec4, for example, can completely fill a mat2.
	  */
	 if (rhs_components >= components_remaining_this_column) {
	    const unsigned count = min(rhs_components,
				       components_remaining_this_column);

	    rhs_var_ref = new(ctx) ir_dereference_variable(rhs_var);

	    ir_instruction *inst = assign_to_matrix_column(var, col_idx,
							   row_idx,
							   rhs_var_ref, 0,
							   count, ctx);
	    instructions->push_tail(inst);

	    rhs_base = count;

	    col_idx++;
	    row_idx = 0;
	 }

	 /* If there is data left in the parameter and components left to be
	  * set in the destination, emit another assignment.  It is possible
	  * that the assignment could be of a vec4 to the last element of the
	  * matrix.  In this case col_idx==cols, but there is still data
	  * left in the source parameter.  Obviously, don't emit an assignment
	  * to data outside the destination matrix.
	  */
	 if ((col_idx < cols) && (rhs_base < rhs_components)) {
	    const unsigned count = rhs_components - rhs_base;

	    rhs_var_ref = new(ctx) ir_dereference_variable(rhs_var);

	    ir_instruction *inst = assign_to_matrix_column(var, col_idx,
							   row_idx,
							   rhs_var_ref,
							   rhs_base,
							   count, ctx);
	    instructions->push_tail(inst);

	    row_idx += count;
	 }
      }
   }

   return new(ctx) ir_dereference_variable(var);
}


ir_rvalue *
ast_function_expression::hir(exec_list *instructions,
			     struct _mesa_glsl_parse_state *state)
{
   void *ctx = talloc_parent(state);
   /* There are three sorts of function calls.
    *
    * 1. contstructors - The first subexpression is an ast_type_specifier.
    * 2. methods - Only the .length() method of array types.
    * 3. functions - Calls to regular old functions.
    *
    * Method calls are actually detected when the ast_field_selection
    * expression is handled.
    */
   if (is_constructor()) {
      const ast_type_specifier *type = (ast_type_specifier *) subexpressions[0];
      YYLTYPE loc = type->get_location();
      const char *name;

      const glsl_type *const constructor_type = type->glsl_type(& name, state);


      /* Constructors for samplers are illegal.
       */
      if (constructor_type->is_sampler()) {
	 _mesa_glsl_error(& loc, state, "cannot construct sampler type `%s'",
			  constructor_type->name);
	 return ir_call::get_error_instruction(ctx);
      }

      if (constructor_type->is_array()) {
	 if (state->language_version <= 110) {
	    _mesa_glsl_error(& loc, state,
			     "array constructors forbidden in GLSL 1.10");
	    return ir_call::get_error_instruction(ctx);
	 }

	 return process_array_constructor(instructions, constructor_type,
					  & loc, &this->expressions, state);
      }

      /* There are two kinds of constructor call.  Constructors for built-in
       * language types, such as mat4 and vec2, are free form.  The only
       * requirement is that the parameters must provide enough values of the
       * correct scalar type.  Constructors for arrays and structures must
       * have the exact number of parameters with matching types in the
       * correct order.  These constructors follow essentially the same type
       * matching rules as functions.
       */
      if (constructor_type->is_numeric() || constructor_type->is_boolean()) {
	 /* Constructing a numeric type has a couple steps.  First all values
	  * passed to the constructor are broken into individual parameters
	  * and type converted to the base type of the thing being constructed.
	  *
	  * At that point we have some number of values that match the base
	  * type of the thing being constructed.  Now the constructor can be
	  * treated like a function call.  Each numeric type has a small set
	  * of constructor functions.  The set of new parameters will either
	  * match one of those functions or the original constructor is
	  * invalid.
	  */
	 const glsl_type *const base_type = constructor_type->get_base_type();

	 /* Total number of components of the type being constructed.
	  */
	 const unsigned type_components = constructor_type->components();

	 /* Number of components from parameters that have actually been
	  * consumed.  This is used to perform several kinds of error checking.
	  */
	 unsigned components_used = 0;

	 unsigned matrix_parameters = 0;
	 unsigned nonmatrix_parameters = 0;
	 exec_list actual_parameters;

	 bool all_parameters_are_constant = true;

	 /* This handles invalid constructor calls such as 'vec4 v = vec4();'
	  */
	 if (this->expressions.is_empty()) {
	    _mesa_glsl_error(& loc, state, "too few components to construct "
			     "`%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction(ctx);
	 }

	 foreach_list (n, &this->expressions) {
	    ast_node *ast = exec_node_data(ast_node, n, link);
	    ir_rvalue *result =
	       ast->hir(instructions, state)->as_rvalue();
	    ir_variable *result_var = NULL;

	    /* Attempt to convert the parameter to a constant valued expression.
	     * After doing so, track whether or not all the parameters to the
	     * constructor are trivially constant valued expressions.
	     */
	    ir_rvalue *const constant =
	       result->constant_expression_value();

	    if (constant != NULL)
	       result = constant;
	    else
	       all_parameters_are_constant = false;

	    /* From page 50 (page 56 of the PDF) of the GLSL 1.50 spec:
	     *
	     *    "It is an error to provide extra arguments beyond this
	     *    last used argument."
	     */
	    if (components_used >= type_components) {
	       _mesa_glsl_error(& loc, state, "too many parameters to `%s' "
				"constructor",
				constructor_type->name);
	       return ir_call::get_error_instruction(ctx);
	    }

	    if (!result->type->is_numeric() && !result->type->is_boolean()) {
	       _mesa_glsl_error(& loc, state, "cannot construct `%s' from a "
				"non-numeric data type",
				constructor_type->name);
	       return ir_call::get_error_instruction(ctx);
	    }

	    /* Count the number of matrix and nonmatrix parameters.  This
	     * is used below to enforce some of the constructor rules.
	     */
	    if (result->type->is_matrix())
	       matrix_parameters++;
	    else
	       nonmatrix_parameters++;

	    /* We can't use the same instruction node in the multiple
	     * swizzle dereferences that happen, so assign it to a
	     * variable and deref that.  Plus it saves computation for
	     * complicated expressions and handles
	     * glsl-vs-constructor-call.shader_test.
	     */
	    if (result->type->components() >= 1 && !result->as_constant()) {
	       result_var = new(ctx) ir_variable(result->type,
						 "constructor_tmp");
	       ir_dereference_variable *lhs;

	       lhs = new(ctx) ir_dereference_variable(result_var);
	       instructions->push_tail(new(ctx) ir_assignment(lhs,
							      result, NULL));
	    }

	    /* Process each of the components of the parameter.  Dereference
	     * each component individually, perform any type conversions, and
	     * add it to the parameter list for the constructor.
	     */
	    for (unsigned i = 0; i < result->type->components(); i++) {
	       if (components_used >= type_components)
		  break;

	       ir_rvalue *component;

	       if (result_var) {
		  ir_dereference *d = new(ctx) ir_dereference_variable(result_var);
		  component = dereference_component(d, i);
	       } else {
		  component = dereference_component(result, i);
	       }
	       component = convert_component(component, base_type);

	       /* All cases that could result in component->type being the
		* error type should have already been caught above.
		*/
	       assert(component->type == base_type);

	       if (component->as_constant() == NULL)
		  all_parameters_are_constant = false;

	       /* Don't actually generate constructor calls for scalars.
		* Instead, do the usual component selection and conversion,
		* and return the single component.
		*/
	       if (constructor_type->is_scalar())
		  return component;

	       actual_parameters.push_tail(component);
	       components_used++;
	    }
	 }

	 /* From page 28 (page 34 of the PDF) of the GLSL 1.10 spec:
	  *
	  *    "It is an error to construct matrices from other matrices. This
	  *    is reserved for future use."
	  */
	 if ((state->language_version <= 110) && (matrix_parameters > 0)
	     && constructor_type->is_matrix()) {
	    _mesa_glsl_error(& loc, state, "cannot construct `%s' from a "
			     "matrix in GLSL 1.10",
			     constructor_type->name);
	    return ir_call::get_error_instruction(ctx);
	 }

	 /* From page 50 (page 56 of the PDF) of the GLSL 1.50 spec:
	  *
	  *    "If a matrix argument is given to a matrix constructor, it is
	  *    an error to have any other arguments."
	  */
	 if ((matrix_parameters > 0)
	     && ((matrix_parameters + nonmatrix_parameters) > 1)
	     && constructor_type->is_matrix()) {
	    _mesa_glsl_error(& loc, state, "for matrix `%s' constructor, "
			     "matrix must be only parameter",
			     constructor_type->name);
	    return ir_call::get_error_instruction(ctx);
	 }

	 /* From page 28 (page 34 of the PDF) of the GLSL 1.10 spec:
	  *
	  *    "In these cases, there must be enough components provided in the
	  *    arguments to provide an initializer for every component in the
	  *    constructed value."
	  */
	 if ((components_used < type_components) && (components_used != 1)) {
	    _mesa_glsl_error(& loc, state, "too few components to construct "
			     "`%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction(ctx);
	 }

	 ir_function *f = state->symbols->get_function(constructor_type->name);
	 if (f == NULL) {
	    _mesa_glsl_error(& loc, state, "no constructor for type `%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction(ctx);
	 }

	 const ir_function_signature *sig =
	    f->matching_signature(& actual_parameters);
	 if (sig != NULL) {
	    /* If all of the parameters are trivially constant, create a
	     * constant representing the complete collection of parameters.
	     */
	    if (all_parameters_are_constant) {
	       if (components_used >= type_components)
		  return new(ctx) ir_constant(sig->return_type,
					      & actual_parameters);

	       assert(sig->return_type->is_vector()
		      || sig->return_type->is_matrix());

	       /* Constructors with exactly one component are special for
		* vectors and matrices.  For vectors it causes all elements of
		* the vector to be filled with the value.  For matrices it
		* causes the matrix to be filled with 0 and the diagonal to be
		* filled with the value.
		*/
	       ir_constant_data data;
	       ir_constant *const initializer =
		  (ir_constant *) actual_parameters.head;
	       if (sig->return_type->is_matrix())
		  generate_constructor_matrix(sig->return_type, initializer,
					      &data);
	       else
		  generate_constructor_vector(sig->return_type, initializer,
					      &data);

	       return new(ctx) ir_constant(sig->return_type, &data);
	    } else if (constructor_type->is_vector()) {
	       return emit_inline_vector_constructor(constructor_type,
						     instructions,
						     &actual_parameters,
						     ctx);
	    } else {
	       assert(constructor_type->is_matrix());
	       return emit_inline_matrix_constructor(constructor_type,
						     instructions,
						     &actual_parameters,
						     ctx);
	    }
	 } else {
	    /* FINISHME: Log a better error message here.  G++ will show the
	     * FINSIHME: types of the actual parameters and the set of
	     * FINSIHME: candidate functions.  A different error should also be
	     * FINSIHME: logged when multiple functions match.
	     */
	    _mesa_glsl_error(& loc, state, "no matching constructor for `%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction(ctx);
	 }
      }

      return ir_call::get_error_instruction(ctx);
   } else {
      const ast_expression *id = subexpressions[0];
      YYLTYPE loc = id->get_location();
      exec_list actual_parameters;

      process_parameters(instructions, &actual_parameters, &this->expressions,
			 state);

      const glsl_type *const type =
	 state->symbols->get_type(id->primary_expression.identifier);

      if ((type != NULL) && type->is_record()) {
	 ir_constant *constant =
	    constant_record_constructor(type, &loc, &actual_parameters, state);

	 if (constant != NULL)
	    return constant;
      }

      return match_function_by_name(instructions, 
				    id->primary_expression.identifier, & loc,
				    &actual_parameters, state);
   }

   return ir_call::get_error_instruction(ctx);
}
