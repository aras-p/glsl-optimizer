struct ir_instruction *
_mesa_ast_constructor_to_hir(const struct ast_node *n,
			     const struct ast_node *parameters,
			     struct _mesa_glsl_parse_state *state)
{
   const struct ast_type_specifier *type = (struct ast_type_specifier *) n;


   /* There are effectively three kinds of constructors.  Each has its own set
    * of rules.
    *
    * * Built-in scalar, vector, and matrix types:  For each of these the only
    *   matching requirement is that the number of values supplied is
    *   sufficient to initialize all of the fields of the type.
    * * Array types: The number of initializers must match the size of the
    *   array, if a size is specified.  Each of the initializers must
    *   exactly match the base type of the array.
    * * Structure types: These initializers must exactly match the fields of
    *   the structure in order.  This is the most restrictive type.
    *
    * In all cases the built-in promotions from integer to floating-point types
    * are applied.
    */

   if (type->is_array) {
      /* FINISHME */
   } else if ((type->type_specifier == ast_struct)
	      || (type->type_specifier == ast_type_name)) {
      /* FINISHME */
   } else {
      const struct glsl_type *ctor_type;

      /* Look-up the type, by name, in the symbol table.
       */


      /* Generate a series of assignments of constructor parameters to fields
       * of the object being initialized.
       */
   }
}
