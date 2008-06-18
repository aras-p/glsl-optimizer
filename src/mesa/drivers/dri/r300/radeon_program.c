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

#include "radeon_program.h"

#include "shader/prog_print.h"

/**
 * Initialize a compiler structure with a single mixed clause
 * containing all instructions from the source program.
 */
void radeonCompilerInit(
	struct radeon_compiler *compiler,
	GLcontext *ctx,
	struct gl_program *source)
{
	struct radeon_clause* clause;

	_mesa_memset(compiler, 0, sizeof(*compiler));
	compiler->Source = source;
	compiler->Ctx = ctx;

	compiler->NumTemporaries = source->NumTemporaries;

	clause = radeonCompilerInsertClause(compiler, 0, CLAUSE_MIXED);
	clause->NumInstructions = 0;
	while(source->Instructions[clause->NumInstructions].Opcode != OPCODE_END)
		clause->NumInstructions++;
	clause->ReservedInstructions = clause->NumInstructions;
	clause->Instructions = _mesa_alloc_instructions(clause->NumInstructions);
	_mesa_copy_instructions(clause->Instructions, source->Instructions, clause->NumInstructions);
}


/**
 * Free all data that is referenced by the compiler structure.
 * However, the compiler structure itself is not freed.
 */
void radeonCompilerCleanup(struct radeon_compiler *compiler)
{
	radeonCompilerEraseClauses(compiler, 0, compiler->NumClauses);
}


/**
 * Allocate and return a unique temporary register.
 */
int radeonCompilerAllocateTemporary(struct radeon_compiler *compiler)
{
	if (compiler->NumTemporaries >= 256) {
		_mesa_problem(compiler->Ctx, "radeonCompiler: Too many temporaries");
		return 0;
	}

	return compiler->NumTemporaries++;
}


static const char* clausename(int type)
{
	switch(type) {
	case CLAUSE_MIXED: return "CLAUSE_MIXED";
	case CLAUSE_ALU: return "CLAUSE_ALU";
	case CLAUSE_TEX: return "CLAUSE_TEX";
	default: return "CLAUSE_UNKNOWN";
	}
}


/**
 * Dump the current compiler state to the console for debugging.
 */
void radeonCompilerDump(struct radeon_compiler *compiler)
{
	int i;
	for(i = 0; i < compiler->NumClauses; ++i) {
		struct radeon_clause *clause = &compiler->Clauses[i];
		int j;

		_mesa_printf("%2i: %s\n", i+1, clausename(clause->Type));

		for(j = 0; j < clause->NumInstructions; ++j) {
			_mesa_printf("%4i: ", j+1);
			_mesa_print_instruction(&clause->Instructions[j]);
		}
	}
}


/**
 * \p position index of the new clause; later clauses are moved
 * \p type of the new clause; one of CLAUSE_XXX
 * \return a pointer to the new clause
 */
struct radeon_clause* radeonCompilerInsertClause(
	struct radeon_compiler *compiler,
	int position, int type)
{
	struct radeon_clause* oldClauses = compiler->Clauses;
	struct radeon_clause* clause;

	assert(position >= 0 && position <= compiler->NumClauses);

	compiler->Clauses = (struct radeon_clause *)
		_mesa_malloc((compiler->NumClauses+1) * sizeof(struct radeon_clause));
	if (oldClauses) {
		_mesa_memcpy(compiler->Clauses, oldClauses,
			position*sizeof(struct radeon_clause));
		_mesa_memcpy(compiler->Clauses+position+1, oldClauses+position,
			(compiler->NumClauses - position) * sizeof(struct radeon_clause));
		_mesa_free(oldClauses);
	}
	compiler->NumClauses++;

	clause = compiler->Clauses + position;
	_mesa_memset(clause, 0, sizeof(*clause));
	clause->Type = type;

	return clause;
}


/**
 * Remove clauses in the range [start, end)
 */
void radeonCompilerEraseClauses(
	struct radeon_compiler *compiler,
	int start, int end)
{
	struct radeon_clause* oldClauses = compiler->Clauses;
	int i;

	assert(0 <= start);
	assert(start <= end);
	assert(end <= compiler->NumClauses);

	if (end == start)
		return;

	for(i = start; i < end; ++i) {
		struct radeon_clause* clause = oldClauses + i;
		_mesa_free_instructions(clause->Instructions, clause->NumInstructions);
	}

	if (start > 0 || end < compiler->NumClauses) {
		compiler->Clauses = (struct radeon_clause*)
			_mesa_malloc((compiler->NumClauses+start-end) * sizeof(struct radeon_clause));
		_mesa_memcpy(compiler->Clauses, oldClauses,
			start * sizeof(struct radeon_clause));
		_mesa_memcpy(compiler->Clauses + start, oldClauses + end,
			(compiler->NumClauses - end) * sizeof(struct radeon_clause));
		compiler->NumClauses -= end - start;
	} else {
		compiler->Clauses = 0;
		compiler->NumClauses = 0;
	}

	_mesa_free(oldClauses);
}


/**
 * Insert new instructions at the given position, initialize them as NOPs
 * and return a pointer to the first new instruction.
 */
struct prog_instruction* radeonClauseInsertInstructions(
	struct radeon_compiler *compiler,
	struct radeon_clause *clause,
	int position, int count)
{
	int newNumInstructions = clause->NumInstructions + count;

	assert(position >= 0 && position <= clause->NumInstructions);

	if (newNumInstructions <= clause->ReservedInstructions) {
		memmove(clause->Instructions + position + count, clause->Instructions + position,
			(clause->NumInstructions - position) * sizeof(struct prog_instruction));
	} else {
		struct prog_instruction *oldInstructions = clause->Instructions;

		clause->ReservedInstructions *= 2;
		if (newNumInstructions > clause->ReservedInstructions)
			clause->ReservedInstructions = newNumInstructions;

		clause->Instructions = (struct prog_instruction*)
			_mesa_malloc(clause->ReservedInstructions * sizeof(struct prog_instruction));

		if (oldInstructions) {
			_mesa_memcpy(clause->Instructions, oldInstructions,
				position * sizeof(struct prog_instruction));
			_mesa_memcpy(clause->Instructions + position + count, oldInstructions + position,
				(clause->NumInstructions - position) * sizeof(struct prog_instruction));

			_mesa_free(oldInstructions);
		}
	}

	clause->NumInstructions = newNumInstructions;
	_mesa_init_instructions(clause->Instructions + position, count);
	return clause->Instructions + position;
}


/**
 * Transform the given clause in the following way:
 *  1. Replace it with an empty clause
 *  2. For every instruction in the original clause, try the given
 *     transformations in order.
 *  3. If one of the transformations returns GL_TRUE, assume that it
 *     has emitted the appropriate instruction(s) into the new clause;
 *     otherwise, copy the instruction verbatim.
 *
 * \note The transformation is currently not recursive; in other words,
 * instructions emitted by transformations are not transformed.
 *
 * \note The transform is called 'local' because it can only look at
 * one instruction at a time.
 */
void radeonClauseLocalTransform(
	struct radeon_compiler *compiler,
	struct radeon_clause *clause,
	int num_transformations,
	struct radeon_program_transformation* transformations)
{
	struct radeon_program_transform_context context;
	struct radeon_clause source;
	int ip;

	source = *clause;
	clause->Instructions = 0;
	clause->NumInstructions = 0;
	clause->ReservedInstructions = 0;

	context.compiler = compiler;
	context.dest = clause;
	context.src = &source;

	for(ip = 0; ip < source.NumInstructions; ++ip) {
		struct prog_instruction *instr = source.Instructions + ip;
		int i;

		for(i = 0; i < num_transformations; ++i) {
			struct radeon_program_transformation* t = transformations + i;

			if (t->function(&context, instr, t->userData))
				break;
		}

		if (i >= num_transformations) {
			struct prog_instruction *tgt =
				radeonClauseInsertInstructions(compiler, clause, clause->NumInstructions, 1);
			_mesa_copy_instructions(tgt, instr, 1);
		}
	}

	_mesa_free_instructions(source.Instructions, source.NumInstructions);
}
