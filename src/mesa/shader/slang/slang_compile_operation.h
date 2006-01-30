/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#if !defined SLANG_COMPILE_OPERATION_H
#define SLANG_COMPILE_OPERATION_H

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_operation_type_
{
	slang_oper_none,
	slang_oper_block_no_new_scope,
	slang_oper_block_new_scope,
	slang_oper_variable_decl,
	slang_oper_asm,
	slang_oper_break,
	slang_oper_continue,
	slang_oper_discard,
	slang_oper_return,
	slang_oper_expression,
	slang_oper_if,
	slang_oper_while,
	slang_oper_do,
	slang_oper_for,
	slang_oper_void,
	slang_oper_literal_bool,
	slang_oper_literal_int,
	slang_oper_literal_float,
	slang_oper_identifier,
	slang_oper_sequence,
	slang_oper_assign,
	slang_oper_addassign,
	slang_oper_subassign,
	slang_oper_mulassign,
	slang_oper_divassign,
	/*slang_oper_modassign,*/
	/*slang_oper_lshassign,*/
	/*slang_oper_rshassign,*/
	/*slang_oper_orassign,*/
	/*slang_oper_xorassign,*/
	/*slang_oper_andassign,*/
	slang_oper_select,
	slang_oper_logicalor,
	slang_oper_logicalxor,
	slang_oper_logicaland,
	/*slang_oper_bitor,*/
	/*slang_oper_bitxor,*/
	/*slang_oper_bitand,*/
	slang_oper_equal,
	slang_oper_notequal,
	slang_oper_less,
	slang_oper_greater,
	slang_oper_lessequal,
	slang_oper_greaterequal,
	/*slang_oper_lshift,*/
	/*slang_oper_rshift,*/
	slang_oper_add,
	slang_oper_subtract,
	slang_oper_multiply,
	slang_oper_divide,
	/*slang_oper_modulus,*/
	slang_oper_preincrement,
	slang_oper_predecrement,
	slang_oper_plus,
	slang_oper_minus,
	/*slang_oper_complement,*/
	slang_oper_not,
	slang_oper_subscript,
	slang_oper_call,
	slang_oper_field,
	slang_oper_postincrement,
	slang_oper_postdecrement
} slang_operation_type;

typedef struct slang_operation_
{
	slang_operation_type type;
	struct slang_operation_ *children;
	unsigned int num_children;
	float literal;		/* type: bool, literal_int, literal_float */
	char *identifier;	/* type: asm, identifier, call, field */
	slang_variable_scope *locals;
} slang_operation;

int slang_operation_construct (slang_operation *);
void slang_operation_destruct (slang_operation *);
int slang_operation_copy (slang_operation *, const slang_operation *);

#ifdef __cplusplus
}
#endif

#endif

