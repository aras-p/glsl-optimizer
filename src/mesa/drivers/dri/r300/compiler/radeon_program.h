/*
 * Copyright (C) 2008 Nicolai Haehnle.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __RADEON_PROGRAM_H_
#define __RADEON_PROGRAM_H_

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"

struct radeon_compiler;
struct rc_instruction;
struct rc_program;

enum {
	PROGRAM_BUILTIN = PROGRAM_FILE_MAX /**< not a real register, but a special swizzle constant */
};

enum {
	OPCODE_REPL_ALPHA = MAX_OPCODE /**< used in paired instructions */
};

#define SWIZZLE_0000 MAKE_SWIZZLE4(SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO)
#define SWIZZLE_1111 MAKE_SWIZZLE4(SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE)

static inline GLuint get_swz(GLuint swz, GLuint idx)
{
	if (idx & 0x4)
		return idx;
	return GET_SWZ(swz, idx);
}

static inline GLuint combine_swizzles4(GLuint src, GLuint swz_x, GLuint swz_y, GLuint swz_z, GLuint swz_w)
{
	GLuint ret = 0;

	ret |= get_swz(src, swz_x);
	ret |= get_swz(src, swz_y) << 3;
	ret |= get_swz(src, swz_z) << 6;
	ret |= get_swz(src, swz_w) << 9;

	return ret;
}

static inline GLuint combine_swizzles(GLuint src, GLuint swz)
{
	GLuint ret = 0;

	ret |= get_swz(src, GET_SWZ(swz, SWIZZLE_X));
	ret |= get_swz(src, GET_SWZ(swz, SWIZZLE_Y)) << 3;
	ret |= get_swz(src, GET_SWZ(swz, SWIZZLE_Z)) << 6;
	ret |= get_swz(src, GET_SWZ(swz, SWIZZLE_W)) << 9;

	return ret;
}

static INLINE void reset_srcreg(struct prog_src_register* reg)
{
	_mesa_bzero(reg, sizeof(*reg));
	reg->Swizzle = SWIZZLE_NOOP;
}


/**
 * A transformation that can be passed to \ref radeonLocalTransform.
 *
 * The function will be called once for each instruction.
 * It has to either emit the appropriate transformed code for the instruction
 * and return GL_TRUE, or return GL_FALSE if it doesn't understand the
 * instruction.
 *
 * The function gets passed the userData as last parameter.
 */
struct radeon_program_transformation {
	GLboolean (*function)(
		struct radeon_compiler*,
		struct rc_instruction*,
		void*);
	void *userData;
};

void radeonLocalTransform(
	struct radeon_compiler *c,
	int num_transformations,
	struct radeon_program_transformation* transformations);

GLint rc_find_free_temporary(struct radeon_compiler * c);

struct rc_instruction *rc_alloc_instruction(struct radeon_compiler * c);
struct rc_instruction *rc_insert_new_instruction(struct radeon_compiler * c, struct rc_instruction * after);
void rc_remove_instruction(struct rc_instruction * inst);

void rc_print_program(const struct rc_program *prog);

#endif
