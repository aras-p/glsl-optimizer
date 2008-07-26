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

#ifndef __RADEON_PROGRAM_PAIR_H_
#define __RADEON_PROGRAM_PAIR_H_

#include "radeon_program.h"


/**
 * Represents a paired instruction, as found in R300 and R500
 * fragment programs.
 */
struct radeon_pair_instruction_source {
	GLuint Index:8;
	GLuint Constant:1;
	GLuint Used:1;
};

struct radeon_pair_instruction_rgb {
	GLuint Opcode:8;
	GLuint DestIndex:8;
	GLuint WriteMask:3;
	GLuint OutputWriteMask:3;
	GLuint Saturate:1;

	struct radeon_pair_instruction_source Src[3];

	struct {
		GLuint Source:2;
		GLuint Swizzle:9;
		GLuint Abs:1;
		GLuint Negate:1;
	} Arg[3];
};

struct radeon_pair_instruction_alpha {
	GLuint Opcode:8;
	GLuint DestIndex:8;
	GLuint WriteMask:1;
	GLuint OutputWriteMask:1;
	GLuint DepthWriteMask:1;
	GLuint Saturate:1;

	struct radeon_pair_instruction_source Src[3];

	struct {
		GLuint Source:2;
		GLuint Swizzle:3;
		GLuint Abs:1;
		GLuint Negate:1;
	} Arg[3];
};

struct radeon_pair_instruction {
	struct radeon_pair_instruction_rgb RGB;
	struct radeon_pair_instruction_alpha Alpha;
};


/**
 *
 */
struct radeon_pair_handler {
	/**
	 * Fill in the proper hardware index for the given constant register.
	 *
	 * @return GL_FALSE on error.
	 */
	GLboolean (*EmitConst)(void*, GLuint file, GLuint index, GLuint *hwindex);

	/**
	 * Write a paired instruction to the hardware.
	 *
	 * @return GL_FALSE on error.
	 */
	GLboolean (*EmitPaired)(void*, struct radeon_pair_instruction*);

	/**
	 * Write a texture instruction to the hardware.
	 * Register indices have already been rewritten to the allocated
	 * hardware register numbers.
	 *
	 * @return GL_FALSE on error.
	 */
	GLboolean (*EmitTex)(void*, struct prog_instruction*);

	/**
	 * Called before a block of contiguous, independent texture
	 * instructions is emitted.
	 */
	GLboolean (*BeginTexBlock)(void*);

	GLuint MaxHwTemps;
};

GLboolean radeonPairProgram(GLcontext *ctx, struct gl_program *program,
	const struct radeon_pair_handler*, void *userdata);

void radeonPrintPairInstruction(struct radeon_pair_instruction *inst);

#endif /* __RADEON_PROGRAM_PAIR_H_ */
