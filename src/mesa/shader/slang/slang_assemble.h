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

#if !defined SLANG_ASSEMBLE_H
#define SLANG_ASSEMBLE_H

#include "slang_compile.h"

#if defined __cplusplus
extern "C" {
#endif

typedef enum slang_assembly_type_
{
	/* core */
	slang_asm_none,
	slang_asm_float_copy,
	slang_asm_float_move,
	slang_asm_float_push,
	slang_asm_float_deref,
	slang_asm_float_add,
	slang_asm_float_multiply,
	slang_asm_float_divide,
	slang_asm_float_negate,
	slang_asm_float_less,
	slang_asm_float_equal_exp,
	slang_asm_float_equal_int,
	slang_asm_float_to_int,
	slang_asm_float_sine,
	slang_asm_float_arcsine,
	slang_asm_float_arctan,
	slang_asm_float_power,
	slang_asm_float_log2,
	slang_asm_float_floor,
	slang_asm_float_ceil,
	slang_asm_int_copy,
	slang_asm_int_move,
	slang_asm_int_push,
	slang_asm_int_deref,
	slang_asm_int_to_float,
	slang_asm_int_to_addr,
	slang_asm_bool_copy,
	slang_asm_bool_move,
	slang_asm_bool_push,
	slang_asm_bool_deref,
	slang_asm_addr_copy,
	slang_asm_addr_push,
	slang_asm_addr_deref,
	slang_asm_addr_add,
	slang_asm_addr_multiply,
	slang_asm_jump,
	slang_asm_jump_if_zero,
	slang_asm_enter,
	slang_asm_leave,
	slang_asm_local_alloc,
	slang_asm_local_free,
	slang_asm_local_addr,
	slang_asm_call,
	slang_asm_return,
	slang_asm_discard,
	slang_asm_exit,
	/* mesa-specific extensions */
	slang_asm_float_print,
	slang_asm_int_print,
	slang_asm_bool_print,
	slang_asm__last
} slang_assembly_type;

typedef struct slang_assembly_
{
	slang_assembly_type type;
	GLfloat literal;
	GLuint param[2];
} slang_assembly;

typedef struct slang_assembly_file_
{
	slang_assembly *code;
	unsigned int count;
	unsigned int capacity;
} slang_assembly_file;

int slang_assembly_file_construct (slang_assembly_file *);
void slang_assembly_file_destruct (slang_assembly_file *);
int slang_assembly_file_push (slang_assembly_file *, slang_assembly_type);
int slang_assembly_file_push_label (slang_assembly_file *, slang_assembly_type, GLuint);
int slang_assembly_file_push_label2 (slang_assembly_file *, slang_assembly_type, GLuint, GLuint);
int slang_assembly_file_push_literal (slang_assembly_file *, slang_assembly_type, GLfloat);

typedef struct slang_assembly_file_restore_point_
{
	unsigned int count;
} slang_assembly_file_restore_point;

int slang_assembly_file_restore_point_save (slang_assembly_file *,
	slang_assembly_file_restore_point *);
int slang_assembly_file_restore_point_load (slang_assembly_file *,
	slang_assembly_file_restore_point *);

typedef struct slang_assembly_flow_control_
{
	unsigned int loop_start;			/* for "continue" statement */
	unsigned int loop_end;				/* for "break" statement */
	unsigned int function_end;			/* for "return" statement */
} slang_assembly_flow_control;

typedef struct slang_assembly_local_info_
{
	unsigned int ret_size;
	unsigned int addr_tmp;
	unsigned int swizzle_tmp;
} slang_assembly_local_info;

typedef enum
{
	slang_ref_force,
	slang_ref_forbid,
	slang_ref_freelance
} slang_ref_type;

/*
	holds a complete information about vector swizzle - the <swizzle> array contains
	vector component sources indices, where 0 is "x", 1 is "y", ...
	example: "xwz" --> { 3, { 0, 3, 2, n/u } }
*/
typedef struct slang_swizzle_
{
	unsigned int num_components;
	unsigned int swizzle[4];
} slang_swizzle;

typedef struct slang_assembly_stack_info_
{
	slang_swizzle swizzle;
} slang_assembly_stack_info;

typedef struct slang_assembly_name_space_
{
	struct slang_function_scope_ *funcs;
	struct slang_struct_scope_ *structs;
	struct slang_variable_scope_ *vars;
} slang_assembly_name_space;

typedef struct slang_assemble_ctx_
{
	slang_assembly_file *file;
	struct slang_machine_ *mach;
	slang_atom_pool *atoms;
	slang_assembly_name_space space;
	slang_assembly_flow_control flow;
	slang_assembly_local_info local;
	slang_ref_type ref;
	slang_assembly_stack_info swz;
} slang_assemble_ctx;

slang_function *_slang_locate_function (slang_function_scope *funcs, slang_atom a_name,
	slang_operation *params, unsigned int num_params, slang_assembly_name_space *space,
	slang_atom_pool *);

int _slang_assemble_function (slang_assemble_ctx *, struct slang_function_ *);

int _slang_cleanup_stack (slang_assembly_file *, slang_operation *, int ref,
	slang_assembly_name_space *, struct slang_machine_ *, slang_atom_pool *);
int _slang_cleanup_stack_ (slang_assemble_ctx *, slang_operation *);

int _slang_dereference (slang_assembly_file *, slang_operation *, slang_assembly_name_space *,
	slang_assembly_local_info *, struct slang_machine_ *, slang_atom_pool *);

int _slang_assemble_function_call (slang_assemble_ctx *, slang_function *,
	slang_operation *, GLuint, GLboolean);

int _slang_assemble_function_call_name (slang_assemble_ctx *, const char *,
	slang_operation *, GLuint, GLboolean);

int _slang_assemble_operation (slang_assembly_file *, struct slang_operation_ *, int reference,
	slang_assembly_flow_control *, slang_assembly_name_space *, slang_assembly_local_info *,
	slang_assembly_stack_info *, struct slang_machine_ *, slang_atom_pool *);
int _slang_assemble_operation_ (slang_assemble_ctx *, struct slang_operation_ *, slang_ref_type);

#ifdef __cplusplus
}
#endif

#endif

