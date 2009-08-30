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

struct r300_fragment_program_compiler;


/**
 * Represents a paired instruction, as found in R300 and R500
 * fragment programs.
 */
struct radeon_pair_instruction_source {
	unsigned int Index:8;
	unsigned int Constant:1;
	unsigned int Used:1;
};

struct radeon_pair_instruction_rgb {
	unsigned int Opcode:8;
	unsigned int DestIndex:8;
	unsigned int WriteMask:3;
	unsigned int OutputWriteMask:3;
	unsigned int Saturate:1;

	struct radeon_pair_instruction_source Src[3];

	struct {
		unsigned int Source:2;
		unsigned int Swizzle:9;
		unsigned int Abs:1;
		unsigned int Negate:1;
	} Arg[3];
};

struct radeon_pair_instruction_alpha {
	unsigned int Opcode:8;
	unsigned int DestIndex:8;
	unsigned int WriteMask:1;
	unsigned int OutputWriteMask:1;
	unsigned int DepthWriteMask:1;
	unsigned int Saturate:1;

	struct radeon_pair_instruction_source Src[3];

	struct {
		unsigned int Source:2;
		unsigned int Swizzle:3;
		unsigned int Abs:1;
		unsigned int Negate:1;
	} Arg[3];
};

struct radeon_pair_instruction {
	struct radeon_pair_instruction_rgb RGB;
	struct radeon_pair_instruction_alpha Alpha;
};


enum {
	RADEON_OPCODE_TEX = 0,
	RADEON_OPCODE_TXB,
	RADEON_OPCODE_TXP,
	RADEON_OPCODE_KIL
};

struct radeon_pair_texture_instruction {
	unsigned int Opcode:2; /**< one of RADEON_OPCODE_xxx */

	unsigned int DestIndex:8;
	unsigned int WriteMask:4;

	unsigned int TexSrcUnit:5;
	unsigned int TexSrcTarget:3;

	unsigned int SrcIndex:8;
	unsigned int SrcSwizzle:12;
};


/**
 *
 */
struct radeon_pair_handler {
	/**
	 * Write a paired instruction to the hardware.
	 *
	 * @return 0 on error.
	 */
	int (*EmitPaired)(void*, struct radeon_pair_instruction*);

	/**
	 * Write a texture instruction to the hardware.
	 * Register indices have already been rewritten to the allocated
	 * hardware register numbers.
	 *
	 * @return 0 on error.
	 */
	int (*EmitTex)(void*, struct radeon_pair_texture_instruction*);

	/**
	 * Called before a block of contiguous, independent texture
	 * instructions is emitted.
	 */
	int (*BeginTexBlock)(void*);

	unsigned MaxHwTemps;
};

void radeonPairProgram(
	struct r300_fragment_program_compiler * compiler,
	const struct radeon_pair_handler*, void *userdata);

void radeonPrintPairInstruction(struct radeon_pair_instruction *inst);

#endif /* __RADEON_PROGRAM_PAIR_H_ */
