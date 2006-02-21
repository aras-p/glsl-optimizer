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

/**
 * \file slang_execute.c
 * intermediate code interpreter
 * \author Michal Krol
 */

#include "imports.h"
#include "context.h"
#include "swrast/s_context.h"
#include "colormac.h"
#include "slang_utility.h"
#include "slang_assemble.h"
#include "slang_storage.h"
#include "slang_execute.h"
#include "slang_library_noise.h"

#define DEBUG_SLANG 0

void slang_machine_init (slang_machine *mach)
{
	mach->ip = 0;
	mach->sp = SLANG_MACHINE_STACK_SIZE;
	mach->bp = 0;
	mach->kill = 0;
	mach->exit = 0;
}

int _slang_execute (const slang_assembly_file *file)
{
	slang_machine mach;

	slang_machine_init (&mach);
	return _slang_execute2 (file, &mach);
}

#if DEBUG_SLANG

static void dump_instruction (FILE *f, slang_assembly *a, unsigned int i)
{
	fprintf (f, "%.5u:\t", i);
	
	switch (a->type)
	{
	/* core */
	case slang_asm_none:
		fprintf (f, "none");
		break;
	case slang_asm_float_copy:
		fprintf (f, "float_copy\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_float_move:
		fprintf (f, "float_move\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_float_push:
		fprintf (f, "float_push\t%f", a->literal);
		break;
	case slang_asm_float_deref:
		fprintf (f, "float_deref");
		break;
	case slang_asm_float_add:
		fprintf (f, "float_add");
		break;
	case slang_asm_float_multiply:
		fprintf (f, "float_multiply");
		break;
	case slang_asm_float_divide:
		fprintf (f, "float_divide");
		break;
	case slang_asm_float_negate:
		fprintf (f, "float_negate");
		break;
	case slang_asm_float_less:
		fprintf (f, "float_less");
		break;
	case slang_asm_float_equal_exp:
		fprintf (f, "float_equal");
		break;
	case slang_asm_float_equal_int:
		fprintf (f, "float_equal\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_float_to_int:
		fprintf (f, "float_to_int");
		break;
	case slang_asm_float_sine:
		fprintf (f, "float_sine");
		break;
	case slang_asm_float_arcsine:
		fprintf (f, "float_arcsine");
		break;
	case slang_asm_float_arctan:
		fprintf (f, "float_arctan");
		break;
	case slang_asm_float_power:
		fprintf (f, "float_power");
		break;
	case slang_asm_float_log2:
		fprintf (f, "float_log2");
		break;
	case slang_asm_float_floor:
		fprintf (f, "float_floor");
		break;
	case slang_asm_float_ceil:
		fprintf (f, "float_ceil");
		break;
	case slang_asm_float_noise1:
		fprintf (f, "float_noise1");
		break;
	case slang_asm_float_noise2:
		fprintf (f, "float_noise2");
		break;
	case slang_asm_float_noise3:
		fprintf (f, "float_noise3");
		break;
	case slang_asm_float_noise4:
		fprintf (f, "float_noise4");
		break;
	case slang_asm_int_copy:
		fprintf (f, "int_copy\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_int_move:
		fprintf (f, "int_move\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_int_push:
		fprintf (f, "int_push\t%d", (GLint) a->literal);
		break;
	case slang_asm_int_deref:
		fprintf (f, "int_deref");
		break;
	case slang_asm_int_to_float:
		fprintf (f, "int_to_float");
		break;
	case slang_asm_int_to_addr:
		fprintf (f, "int_to_addr");
		break;
	case slang_asm_bool_copy:
		fprintf (f, "bool_copy\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_bool_move:
		fprintf (f, "bool_move\t%d, %d", a->param[0], a->param[1]);
		break;
	case slang_asm_bool_push:
		fprintf (f, "bool_push\t%d", a->literal != 0.0f);
		break;
	case slang_asm_bool_deref:
		fprintf (f, "bool_deref");
		break;
	case slang_asm_addr_copy:
		fprintf (f, "addr_copy");
		break;
	case slang_asm_addr_push:
		fprintf (f, "addr_push\t%u", a->param[0]);
		break;
	case slang_asm_addr_deref:
		fprintf (f, "addr_deref");
		break;
	case slang_asm_addr_add:
		fprintf (f, "addr_add");
		break;
	case slang_asm_addr_multiply:
		fprintf (f, "addr_multiply");
		break;
	case slang_vec4_tex2d:
		fprintf (f, "vec4_tex2d");
		break;
	case slang_asm_jump:
		fprintf (f, "jump\t%u", a->param[0]);
		break;
	case slang_asm_jump_if_zero:
		fprintf (f, "jump_if_zero\t%u", a->param[0]);
		break;
	case slang_asm_enter:
		fprintf (f, "enter\t%u", a->param[0]);
		break;
	case slang_asm_leave:
		fprintf (f, "leave");
		break;
	case slang_asm_local_alloc:
		fprintf (f, "local_alloc\t%u", a->param[0]);
		break;
	case slang_asm_local_free:
		fprintf (f, "local_free\t%u", a->param[0]);
		break;
	case slang_asm_local_addr:
		fprintf (f, "local_addr\t%u, %u", a->param[0], a->param[1]);
		break;
	case slang_asm_call:
		fprintf (f, "call\t%u", a->param[0]);
		break;
	case slang_asm_return:
		fprintf (f, "return");
		break;
	case slang_asm_discard:
		fprintf (f, "discard");
		break;
	case slang_asm_exit:
		fprintf (f, "exit");
		break;
	/* mesa-specific extensions */
	case slang_asm_float_print:
		fprintf (f, "float_print");
		break;
	case slang_asm_int_print:
		fprintf (f, "int_print");
		break;
	case slang_asm_bool_print:
		fprintf (f, "bool_print");
		break;
	default:
		break;
	}

	fprintf (f, "\n");
}

static void dump (const slang_assembly_file *file)
{
	unsigned int i;
	static unsigned int counter = 0;
	FILE *f;
	char filename[256];

	counter++;
	_mesa_sprintf (filename, "~mesa-slang-assembly-dump-(%u).txt", counter);
	f = fopen (filename, "w");
	if (f == NULL)
		return;

	for (i = 0; i < file->count; i++)
		dump_instruction (f, file->code + i, i);

	fclose (f);
}

#endif

static void fetch_texel (GLuint sampler, const GLfloat texcoord[4], GLfloat lambda, GLfloat color[4])
{
	GET_CURRENT_CONTEXT(ctx);
	SWcontext *swrast = SWRAST_CONTEXT(ctx);
	GLchan rgba[4];

	/* XXX: the function pointer is NULL! */
	swrast->TextureSample[sampler] (ctx, ctx->Texture.Unit[sampler]._Current, 1,
		(const GLfloat (*)[4]) texcoord, &lambda, &rgba);
	color[0] = CHAN_TO_FLOAT(rgba[0]);
	color[1] = CHAN_TO_FLOAT(rgba[1]);
	color[2] = CHAN_TO_FLOAT(rgba[2]);
	color[3] = CHAN_TO_FLOAT(rgba[3]);
}

int _slang_execute2 (const slang_assembly_file *file, slang_machine *mach)
{
	slang_machine_slot *stack;

#if DEBUG_SLANG
	static unsigned int counter = 0;
	char filename[256];
	FILE *f;
#endif

	/* assume 32-bit floats and uints; should work fine also on 64-bit platforms */
	static_assert(sizeof (GLfloat) == 4);
	static_assert(sizeof (GLuint) == 4);

#if DEBUG_SLANG
	dump (file);
	counter++;
	_mesa_sprintf (filename, "~mesa-slang-assembly-exec-(%u).txt", counter);
	f = fopen (filename, "w");
#endif

	stack = mach->mem + SLANG_MACHINE_GLOBAL_SIZE;

	while (!mach->exit)
	{
		slang_assembly *a = &file->code[mach->ip];

#if DEBUG_SLANG
		if (f != NULL && a->type != slang_asm_none)
		{
			unsigned int i;

			dump_instruction (f, file->code + mach->ip, mach->ip);
			fprintf (f, "\t\tsp=%u bp=%u\n", mach->sp, mach->bp);
			for (i = mach->sp; i < SLANG_MACHINE_STACK_SIZE; i++)
				fprintf (f, "\t%.5u\t%6f\t%u\n", i, stack[i]._float, stack[i]._addr);
			fflush (f);
		}
#endif

		mach->ip++;

		switch (a->type)
		{
		/* core */
		case slang_asm_none:
			break;
		case slang_asm_float_copy:
		case slang_asm_int_copy:
		case slang_asm_bool_copy:
			mach->mem[(stack[mach->sp + a->param[0] / 4]._addr + a->param[1]) / 4]._float =
				stack[mach->sp]._float;
			mach->sp++;
			break;
		case slang_asm_float_move:
		case slang_asm_int_move:
		case slang_asm_bool_move:
			stack[mach->sp + a->param[0] / 4]._float =
				stack[mach->sp + (stack[mach->sp]._addr + a->param[1]) / 4]._float;
			break;
		case slang_asm_float_push:
		case slang_asm_int_push:
		case slang_asm_bool_push:
			mach->sp--;
			stack[mach->sp]._float = a->literal;
			break;
		case slang_asm_float_deref:
		case slang_asm_int_deref:
		case slang_asm_bool_deref:
			stack[mach->sp]._float = mach->mem[stack[mach->sp]._addr / 4]._float;
			break;
		case slang_asm_float_add:
			stack[mach->sp + 1]._float += stack[mach->sp]._float;
			mach->sp++;
			break;
		case slang_asm_float_multiply:
			stack[mach->sp + 1]._float *= stack[mach->sp]._float;
			mach->sp++;
			break;
		case slang_asm_float_divide:
			stack[mach->sp + 1]._float /= stack[mach->sp]._float;
			mach->sp++;
			break;
		case slang_asm_float_negate:
			stack[mach->sp]._float = -stack[mach->sp]._float;
			break;
		case slang_asm_float_less:
			stack[mach->sp + 1]._float =
				stack[mach->sp + 1]._float < stack[mach->sp]._float ? (GLfloat) 1 : (GLfloat) 0;
			mach->sp++;
			break;
		case slang_asm_float_equal_exp:
			stack[mach->sp + 1]._float =
				stack[mach->sp + 1]._float == stack[mach->sp]._float ? (GLfloat) 1 : (GLfloat) 0;
			mach->sp++;
			break;
		case slang_asm_float_equal_int:
			mach->sp--;
			stack[mach->sp]._float = stack[mach->sp + 1 + a->param[0] / 4]._float ==
				stack[mach->sp + 1 + a->param[1] / 4]._float ? (GLfloat) 1 : (GLfloat) 0;
			break;
		case slang_asm_float_to_int:
			stack[mach->sp]._float = (GLfloat) (GLint) stack[mach->sp]._float;
			break;
		case slang_asm_float_sine:
			stack[mach->sp]._float = (GLfloat) _mesa_sin (stack[mach->sp]._float);
			break;
		case slang_asm_float_arcsine:
			stack[mach->sp]._float = _mesa_asinf (stack[mach->sp]._float);
			break;
		case slang_asm_float_arctan:
			stack[mach->sp]._float = _mesa_atanf (stack[mach->sp]._float);
			break;
		case slang_asm_float_power:
			stack[mach->sp + 1]._float =
				(GLfloat) _mesa_pow (stack[mach->sp + 1]._float, stack[mach->sp]._float);
			mach->sp++;
			break;
		case slang_asm_float_log2:
			stack[mach->sp]._float = LOG2 (stack[mach->sp]._float);
			break;
		case slang_asm_float_floor:
			stack[mach->sp]._float = FLOORF (stack[mach->sp]._float);
			break;
		case slang_asm_float_ceil:
			stack[mach->sp]._float = CEILF (stack[mach->sp]._float);
			break;
		case slang_asm_float_noise1:
			stack[mach->sp]._float = _slang_library_noise1 (stack[mach->sp]._float);
			break;
		case slang_asm_float_noise2:
			stack[mach->sp + 1]._float = _slang_library_noise2 (stack[mach->sp]._float,
				stack[mach->sp + 1]._float);
			mach->sp++;
			break;
		case slang_asm_float_noise3:
			stack[mach->sp + 2]._float = _slang_library_noise3 (stack[mach->sp]._float,
				stack[mach->sp + 1]._float, stack[mach->sp + 2]._float);
			mach->sp += 2;
			break;
		case slang_asm_float_noise4:
			stack[mach->sp + 3]._float = _slang_library_noise4 (stack[mach->sp]._float,
				stack[mach->sp + 1]._float, stack[mach->sp + 2]._float, stack[mach->sp + 3]._float);
			mach->sp += 3;
			break;
		case slang_asm_int_to_float:
			break;
		case slang_asm_int_to_addr:
			stack[mach->sp]._addr = (GLuint) (GLint) stack[mach->sp]._float;
			break;
		case slang_asm_addr_copy:
			mach->mem[stack[mach->sp + 1]._addr / 4]._addr = stack[mach->sp]._addr;
			mach->sp++;
			break;
		case slang_asm_addr_push:
			mach->sp--;
			stack[mach->sp]._addr = a->param[0];
			break;
		case slang_asm_addr_deref:
			stack[mach->sp]._addr = mach->mem[stack[mach->sp]._addr / 4]._addr;
			break;
		case slang_asm_addr_add:
			stack[mach->sp + 1]._addr += stack[mach->sp]._addr;
			mach->sp++;
			break;
		case slang_asm_addr_multiply:
			stack[mach->sp + 1]._addr *= stack[mach->sp]._addr;
			mach->sp++;
			break;
		case slang_asm_vec4_tex2d:
			{
				GLfloat st[4] = { stack[mach->sp]._float, stack[mach->sp + 1]._float, 0.0f, 1.0f };
				GLuint sampler = (GLuint) stack[mach->sp + 2]._float;
				GLfloat *rgba = &mach->mem[stack[mach->sp + 3]._addr / 4]._float;

				fetch_texel (sampler, st, 0.0f, rgba);
			}
			mach->sp += 3;
			break;
		case slang_asm_jump:
			mach->ip = a->param[0];
			break;
		case slang_asm_jump_if_zero:
			if (stack[mach->sp]._float == 0.0f)
				mach->ip = a->param[0];
			mach->sp++;
			break;
		case slang_asm_enter:
			mach->sp--;
			stack[mach->sp]._addr = mach->bp;
			mach->bp = mach->sp + a->param[0] / 4;
			break;
		case slang_asm_leave:
			mach->bp = stack[mach->sp]._addr;
			mach->sp++;
			break;
		case slang_asm_local_alloc:
			mach->sp -= a->param[0] / 4;
			break;
		case slang_asm_local_free:
			mach->sp += a->param[0] / 4;
			break;
		case slang_asm_local_addr:
			mach->sp--;
			stack[mach->sp]._addr = SLANG_MACHINE_GLOBAL_SIZE * 4 + mach->bp * 4 - 
				(a->param[0] + a->param[1]) + 4;
			break;
		case slang_asm_call:
			mach->sp--;
			stack[mach->sp]._addr = mach->ip;
			mach->ip = a->param[0];
			break;
		case slang_asm_return:
			mach->ip = stack[mach->sp]._addr;
			mach->sp++;
			break;
		case slang_asm_discard:
			mach->kill = 1;
			break;
		case slang_asm_exit:
			mach->exit = 1;
			break;
		/* mesa-specific extensions */
		case slang_asm_float_print:
			_mesa_printf ("slang print: %f\n", stack[mach->sp]._float);
			break;
		case slang_asm_int_print:
			_mesa_printf ("slang print: %d\n", (GLint) stack[mach->sp]._float);
			break;
		case slang_asm_bool_print:
			_mesa_printf ("slang print: %s\n", (GLint) stack[mach->sp]._float ? "true" : "false");
			break;
		default:
			assert (0);
		}
	}

#if DEBUG_SLANG
	if (f != NULL)
		fclose (f);
#endif

	return 1;
}

