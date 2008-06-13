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
