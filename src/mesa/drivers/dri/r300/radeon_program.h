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


enum {
	CLAUSE_MIXED = 0,
	CLAUSE_ALU,
	CLAUSE_TEX
};

enum {
	PROGRAM_BUILTIN = PROGRAM_FILE_MAX /**< not a real register, but a special swizzle constant */
};

enum {
	OPCODE_REPL_ALPHA = MAX_OPCODE /**< used in paired instructions */
};

#define SWIZZLE_0000 MAKE_SWIZZLE4(SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO)
#define SWIZZLE_1111 MAKE_SWIZZLE4(SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE)

/**
 * Transformation context that is passed to local transformations.
 *
 * Care must be taken with some operations during transformation,
 * e.g. finding new temporary registers must use @ref radeonFindFreeTemporary
 */
struct radeon_transform_context {
	GLcontext *Ctx;
	struct gl_program *Program;
	struct prog_instruction *OldInstructions;
	GLuint OldNumInstructions;
};

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
		struct radeon_transform_context*,
		struct prog_instruction*,
		void*);
	void *userData;
};

void radeonLocalTransform(
	GLcontext* ctx,
	struct gl_program *program,
	int num_transformations,
	struct radeon_program_transformation* transformations);

/**
 * Find a usable free temporary register during program transformation
 */
GLint radeonFindFreeTemporary(struct radeon_transform_context *ctx);

struct prog_instruction *radeonAppendInstructions(struct gl_program *program, int count);

#endif
