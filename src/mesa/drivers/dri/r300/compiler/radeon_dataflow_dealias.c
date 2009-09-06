/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#include "radeon_dataflow.h"

#include "radeon_compiler.h"


#define DEALIAS_LIST_SIZE 128

struct dealias_state {
	struct radeon_compiler * C;

	unsigned int OldIndex:RC_REGISTER_INDEX_BITS;
	unsigned int NewIndex:RC_REGISTER_INDEX_BITS;
	unsigned int DealiasFail:1;

	struct rc_dataflow_vector * List[DEALIAS_LIST_SIZE];
	unsigned int Length;
};

static void push_dealias_vector(struct dealias_state * s, struct rc_dataflow_vector * vec)
{
	if (s->Length >= DEALIAS_LIST_SIZE) {
		rc_debug(s->C, "%s: list size exceeded\n", __FUNCTION__);
		s->DealiasFail = 1;
		return;
	}

	if (rc_assert(s->C, vec->File == RC_FILE_TEMPORARY && vec->Index == s->OldIndex))
		return;

	s->List[s->Length++] = vec;
}

static void run_dealias(struct dealias_state * s)
{
	unsigned int i;

	for(i = 0; i < s->Length && !s->DealiasFail; ++i) {
		struct rc_dataflow_vector * vec = s->List[i];
		struct rc_dataflow_ref * ref;

		for(ref = vec->Refs.Next; ref != &vec->Refs; ref = ref->Next) {
			if (ref->ReadInstruction->Dataflow.DstRegPrev == ref)
				push_dealias_vector(s, ref->ReadInstruction->Dataflow.DstReg);
		}
	}

	if (s->DealiasFail)
		return;

	for(i = 0; i < s->Length; ++i) {
		struct rc_dataflow_vector * vec = s->List[i];
		struct rc_dataflow_ref * ref;

		vec->Index = s->NewIndex;
		vec->WriteInstruction->I.DstReg.Index = s->NewIndex;

		for(ref = vec->Refs.Next; ref != &vec->Refs; ref = ref->Next) {
			struct rc_instruction * inst = ref->ReadInstruction;
			unsigned int i;

			for(i = 0; i < 3; ++i) {
				if (inst->Dataflow.SrcReg[i] == ref) {
					if (rc_assert(s->C, inst->I.SrcReg[i].File == RC_FILE_TEMPORARY &&
					                    inst->I.SrcReg[i].Index == s->OldIndex))
						return;

					inst->I.SrcReg[i].Index = s->NewIndex;
				}
			}
		}
	}
}

/**
 * Breaks register aliasing to reduce multiple assignments to a single register.
 *
 * This affects sequences like:
 *  MUL r0, ...;
 *  MAD r0, r1, r2, r0;
 * In this example, a new register will be used for the destination of the
 * second MAD.
 *
 * The purpose of this dealiasing is to make the resulting code more SSA-like
 * and therefore make it easier to move instructions around.
 * This is of crucial importance for R300 fragment programs, where de-aliasing
 * can help to reduce texture indirections, but other targets can benefit from
 * it as well.
 *
 * \note When compiling GLSL, there may be some benefit gained from breaking
 * up vectors whose components are unrelated. This is not done yet and should
 * be investigated at some point (of course, a matching pass to re-merge
 * components would be required).
 */
void rc_dataflow_dealias(struct radeon_compiler * c)
{
	struct dealias_state s;

	memset(&s, 0, sizeof(s));
	s.C = c;

	struct rc_instruction * inst;
	for(inst = c->Program.Instructions.Prev; inst != &c->Program.Instructions; inst = inst->Prev) {
		if (!inst->Dataflow.DstRegAliased || inst->Dataflow.DstReg->File != RC_FILE_TEMPORARY)
			continue;

		if (inst->Dataflow.DstReg->UseMask & ~inst->I.DstReg.WriteMask)
			continue;

		s.OldIndex = inst->I.DstReg.Index;
		s.NewIndex = rc_find_free_temporary(c);
		s.DealiasFail = 0;
		s.Length = 0;

		inst->Dataflow.DstRegAliased = 0;
		if (inst->Dataflow.DstRegPrev) {
			rc_dataflow_remove_ref(inst->Dataflow.DstRegPrev);
			inst->Dataflow.DstRegPrev = 0;
		}

		push_dealias_vector(&s, inst->Dataflow.DstReg);
		run_dealias(&s);
	}
}
