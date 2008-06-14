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

#include "glheader.h"
#include "macros.h"
#include "enums.h"
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

#define SWIZZLE_0000 MAKE_SWIZZLE4(SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO)
#define SWIZZLE_1111 MAKE_SWIZZLE4(SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE, SWIZZLE_ONE)

/**
 * A clause is simply a sequence of instructions that are executed
 * in order.
 */
struct radeon_clause {
	/**
	 * Type of this clause, one of CLAUSE_XXX.
	 */
	int Type : 2;

	/**
	 * Pointer to an array of instructions.
	 * The array is terminated by an OPCODE_END instruction.
	 */
	struct prog_instruction *Instructions;

	/**
	 * Number of instructions in this clause.
	 */
	int NumInstructions;

	/**
	 * Space reserved for instructions in this clause.
	 */
	int ReservedInstructions;
};

/**
 * A compile object, holding the current intermediate state during compilation.
 */
struct radeon_compiler {
	struct gl_program *Source;
	GLcontext* Ctx;

	/**
	 * Number of clauses in this program.
	 */
	int NumClauses;

	/**
	 * Pointer to an array of NumClauses clauses.
	 */
	struct radeon_clause *Clauses;

	/**
	 * Number of registers in the PROGRAM_TEMPORARIES file.
	 */
	int NumTemporaries;
};

void radeonCompilerInit(
	struct radeon_compiler *compiler,
	GLcontext *ctx,
	struct gl_program *source);
void radeonCompilerCleanup(struct radeon_compiler *compiler);
int radeonCompilerAllocateTemporary(struct radeon_compiler *compiler);
void radeonCompilerDump(struct radeon_compiler *compiler);

struct radeon_clause *radeonCompilerInsertClause(
	struct radeon_compiler *compiler,
	int position,
	int type);
void radeonCompilerEraseClauses(
	struct radeon_compiler *compiler,
	int start,
	int end);

struct prog_instruction* radeonClauseInsertInstructions(
	struct radeon_compiler *compiler,
	struct radeon_clause *clause,
	int position, int count);

/**
 *
 */
struct radeon_program_transform_context {
	struct radeon_compiler *compiler;

	/**
	 * Destination clause where new instructions must be written.
	 */
	struct radeon_clause *dest;

	/**
	 * Original clause that is currently being transformed.
	 */
	struct radeon_clause *src;
};

/**
 * A transformation that can be passed to \ref radeonClauseLinearTransform.
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
		struct radeon_program_transform_context*,
		struct prog_instruction*,
		void*);
	void *userData;
};

void radeonClauseLocalTransform(
	struct radeon_compiler *compiler,
	struct radeon_clause *clause,
	int num_transformations,
	struct radeon_program_transformation* transformations);

#endif
