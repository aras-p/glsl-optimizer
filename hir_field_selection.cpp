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
#include <stdio.h>
#include "main/imports.h"
#include "symbol_table.h"
#include "glsl_parser_extras.h"
#include "ast.h"
#include "glsl_types.h"
#include "ir.h"

#define X 1
#define R 5
#define S 9
#define I 13

static bool
generate_swizzle(const char *str, struct ir_swizzle_mask *swiz,
		 unsigned vector_length)
{
   /* For each possible swizzle character, this table encodes the value in
    * \c idx_map that represents the 0th element of the vector.  For invalid
    * swizzle characters (e.g., 'k'), a special value is used that will allow
    * detection of errors.
    */
   unsigned char base_idx[26] = {
   /* a  b  c  d  e  f  g  h  i  j  k  l  m */
      R, R, I, I, I, I, R, I, I, I, I, I, I,
   /* n  o  p  q  r  s  t  u  v  w  x  y  z */
      I, I, S, S, R, S, S, I, I, X, X, X, X
   };

   /* Each valid swizzle character has an entry in the previous table.  This
    * table encodes the base index encoded in the previous table plus the actual
    * index of the swizzle character.  When processing swizzles, the first
    * character in the string is indexed in the previous table.  Each character
    * in the string is indexed in this table, and the value found there has the
    * value form the first table subtracted.  The result must be on the range
    * [0,3].
    *
    * For example, the string "wzyx" will get X from the first table.  Each of
    * the charcaters will get X+3, X+2, X+1, and X+0 from this table.  After
    * subtraction, the swizzle values are { 3, 2, 1, 0 }.
    *
    * The string "wzrg" will get X from the first table.  Each of the characters
    * will get X+3, X+2, R+0, and R+1 from this table.  After subtraction, the
    * swizzle values are { 3, 2, 4, 5 }.  Since 4 and 5 are outside the range
    * [0,3], the error is detected.
    */
   unsigned char idx_map[26] = {
   /* a    b    c    d    e    f    g    h    i    j    k    l    m */
      R+3, R+2, 0,   0,   0,   0,   R+1, 0,   0,   0,   0,   0,   0,
   /* n    o    p    q    r    s    t    u    v    w    x    y    z */
      0,   0,   S+2, S+3, R+0, S+0, S+1, 0,   0,   X+3, X+0, X+1, X+2
   };

   int swiz_idx[4] = { 0, 0, 0, 0 };
   unsigned base;
   unsigned dup_mask = 0;
   unsigned seen_mask = 0;
   unsigned i;


   /* Validate the first character in the swizzle string and look up the base
    * index value as described above.
    */
   if ((str[0] < 'a') || (str[0] > 'z'))
      return false;

   base = base_idx[str[0] - 'a'];


   for (i = 0; (i < 4) && (str[i] != '\0'); i++) {
      unsigned bit;

      /* Validate the next character, and, as described above, convert it to a
       * swizzle index.
       */
      if ((str[i] < 'a') || (str[i] > 'z'))
	 return false;

      swiz_idx[i] = idx_map[str[0] - 'a'] - base;
      if ((swiz_idx[i] < 0) || (swiz_idx[i] >= (int) vector_length))
	 return false;


      /* Track a bit-mask of the swizzle index values that have been seen.  If
       * a value is seen more than once, set the "duplicate" flag.
       */
      bit = (1U << swiz_idx[i]);
      dup_mask |= seen_mask & bit;
      seen_mask |= bit;
   }

   if (str[i] != '\0')
	 return false;

   swiz->x = swiz_idx[0];
   swiz->y = swiz_idx[1];
   swiz->z = swiz_idx[2];
   swiz->w = swiz_idx[3];
   swiz->num_components = i;
   swiz->has_duplicates = (dup_mask != 0);

   return true;
}


struct ir_instruction *
_mesa_ast_field_selection_to_hir(const ast_expression *expr,
				 exec_list *instructions,
				 struct _mesa_glsl_parse_state *state)
{
   ir_instruction *op;
   ir_dereference *deref;
   YYLTYPE loc;


   op = expr->subexpressions[0]->hir(instructions, state);
   deref = new ir_dereference(op);

   /* Initially assume that the resulting type of the field selection is an
    * error.  This make the error paths below a bit easier to follow.
    */
   deref->type = glsl_error_type;

   /* If processing the thing being dereferenced generated an error, bail out
    * now.  Doing so prevents spurious error messages from being logged below.
    */
   if (is_error_type(op->type))
      return (struct ir_instruction *) deref;

   /* There are two kinds of field selection.  There is the selection of a
    * specific field from a structure, and there is the selection of a
    * swizzle / mask from a vector.  Which is which is determined entirely
    * by the base type of the thing to which the field selection operator is
    * being applied.
    */
   loc = expr->get_location();
   if (op->type->is_vector()) {
      if (generate_swizzle(expr->primary_expression.identifier, 
			   & deref->selector.swizzle,
			   op->type->vector_elements)) {
	 /* Based on the number of elements in the swizzle and the base type
	  * (i.e., float, int, unsigned, or bool) of the vector being swizzled,
	  * generate the type of the resulting value.
	  */
	 deref->type =
	    _mesa_glsl_get_vector_type(op->type->base_type,
				       deref->selector.swizzle.num_components);
      } else {
	 /* FINISHME: Logging of error messages should be moved into
	  * FINISHME: generate_swizzle.  This allows the generation of more
	  * FINISHME: specific error messages.
	  */
	 _mesa_glsl_error(& loc, state, "Invalid swizzle / mask `%s'",
			  expr->primary_expression.identifier);
      }
   } else if (op->type->base_type == GLSL_TYPE_STRUCT) {
      /* FINISHME: Handle field selection from structures. */
   } else {
      _mesa_glsl_error(& loc, state, "Cannot access field `%s' of "
		       "non-structure / non-vector.",
		       expr->primary_expression.identifier);
   }

   return (struct ir_instruction *) deref;
}
