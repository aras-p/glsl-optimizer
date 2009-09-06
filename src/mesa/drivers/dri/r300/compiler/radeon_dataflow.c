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


static void add_ref_to_vector(struct rc_dataflow_ref * ref, struct rc_dataflow_vector * vector)
{
	ref->Vector = vector;
	ref->Prev = &vector->Refs;
	ref->Next = vector->Refs.Next;
	ref->Prev->Next = ref;
	ref->Next->Prev = ref;
}

struct rc_dataflow_ref * rc_dataflow_create_ref(struct radeon_compiler * c,
		struct rc_dataflow_vector * vector, struct rc_instruction * inst)
{
	struct rc_dataflow_ref * ref = memory_pool_malloc(&c->Pool, sizeof(struct rc_dataflow_ref));
	ref->ReadInstruction = inst;
	ref->UseMask = 0;

	add_ref_to_vector(ref, vector);

	return ref;
}

struct rc_dataflow_vector * rc_dataflow_create_vector(struct radeon_compiler * c,
		rc_register_file file, unsigned int index, struct rc_instruction * inst)
{
	struct rc_dataflow_vector * vec = memory_pool_malloc(&c->Pool, sizeof(struct rc_dataflow_vector));

	memset(vec, 0, sizeof(struct rc_dataflow_vector));
	vec->File = file;
	vec->Index = index;
	vec->WriteInstruction = inst;

	vec->Refs.Next = vec->Refs.Prev = &vec->Refs;

	return vec;
}

void rc_dataflow_remove_ref(struct rc_dataflow_ref * ref)
{
	ref->Prev->Next = ref->Next;
	ref->Next->Prev = ref->Prev;
}

void rc_dataflow_remove_instruction(struct rc_instruction * inst)
{
	for(unsigned int i = 0; i < 3; ++i) {
		if (inst->Dataflow.SrcReg[i]) {
			rc_dataflow_remove_ref(inst->Dataflow.SrcReg[i]);
			inst->Dataflow.SrcReg[i] = 0;
		}
		if (inst->Dataflow.SrcRegAddress[i]) {
			rc_dataflow_remove_ref(inst->Dataflow.SrcRegAddress[i]);
			inst->Dataflow.SrcRegAddress[i] = 0;
		}
	}

	if (inst->Dataflow.DstReg) {
		while(inst->Dataflow.DstReg->Refs.Next != &inst->Dataflow.DstReg->Refs) {
			struct rc_dataflow_ref * ref = inst->Dataflow.DstReg->Refs.Next;
			rc_dataflow_remove_ref(ref);
			if (inst->Dataflow.DstRegPrev)
				add_ref_to_vector(ref, inst->Dataflow.DstRegPrev->Vector);
		}

		inst->Dataflow.DstReg->WriteInstruction = 0;
		inst->Dataflow.DstReg = 0;
	}

	if (inst->Dataflow.DstRegPrev) {
		rc_dataflow_remove_ref(inst->Dataflow.DstRegPrev);
		inst->Dataflow.DstRegPrev = 0;
	}

	inst->Dataflow.DstRegAliased = 0;
}
