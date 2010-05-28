/*
 * Copyright 2010 Tom Stellard <tstellar@gmail.com>
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

/**
 * \file
 */

#include "radeon_emulate_loops.h"

#include "radeon_compiler.h"

struct emulate_loop_state {
	struct radeon_compiler * C;
	struct loop_info * Loops;
	unsigned int LoopCount;
	unsigned int LoopReserved;
};

struct loop_info {
	struct rc_instruction * BeginLoop;
	struct rc_instruction * EndLoop;
};

static unsigned int loop_count_instructions(struct loop_info * loop)
{
	unsigned int count = 0;
	struct rc_instruction * inst = loop->BeginLoop->Next;
	while(inst != loop->EndLoop){
		count++;
		inst = inst->Next;
	}
	return count;
}

static unsigned int loop_calc_iterations(struct loop_info * loop,
		unsigned int loop_count, unsigned int max_instructions)
{
	unsigned int icount = loop_count_instructions(loop);
	return max_instructions / (loop_count * icount);
}

static void loop_unroll(struct emulate_loop_state * s,
			struct loop_info *loop, unsigned int iterations)
{
	unsigned int i;
	struct rc_instruction * ptr;
	struct rc_instruction * first = loop->BeginLoop->Next;
	struct rc_instruction * last = loop->EndLoop->Prev;
	struct rc_instruction * append_to = last;
	rc_remove_instruction(loop->BeginLoop);
	rc_remove_instruction(loop->EndLoop);
	for( i = 1; i < iterations; i++){
		for(ptr = first; ptr != last->Next; ptr = ptr->Next){
			struct rc_instruction *new = rc_alloc_instruction(s->C);
			memcpy(new, ptr, sizeof(struct rc_instruction));
			rc_insert_instruction(append_to, new);
			append_to = new;
		}
	}
}

/** 
 * This function prepares a loop to be unrolled by converting it into an if
 * statement.  Here is an outline of the conversion process:
 * BGNLOOP;                         -> BGNLOOP;
 * SGE temp[0], temp[1], temp[2];   -> SLT temp[0], temp[1], temp[2];
 * IF temp[0];                      -> IF temp[0];
 * BRK;                             ->
 * ENDIF;                           -> <Loop Body>
 * <Loop Body>                      -> ENDIF;
 * ENDLOOP;                         -> ENDLOOP
 *
 * @param inst Pointer to a BGNLOOP instruction.
 */
static struct rc_instruction * transform_loop(struct emulate_loop_state * s,
						struct rc_instruction * inst)
{
	struct loop_info *loop;
	struct rc_instruction * ptr;

	memory_pool_array_reserve(&s->C->Pool, struct loop_info,
			s->Loops, s->LoopCount, s->LoopReserved, 1);

	loop = &s->Loops[s->LoopCount++];
	memset(loop, 0, sizeof(struct loop_info));

	loop->BeginLoop = inst;
	/* Reverse the SGE instruction */
	ptr = inst->Next;
	ptr->U.I.Opcode = RC_OPCODE_SLT;
	while(!loop->EndLoop){
		struct rc_instruction * endif;
		if(ptr->Type == RC_INSTRUCTION_NORMAL){
		}
		switch(ptr->U.I.Opcode){
		case RC_OPCODE_BGNLOOP:
			/* Nested loop */
			ptr = transform_loop(s, ptr);
			break;
		case RC_OPCODE_BRK:
			/* The BRK instruction should always be followed by
			 * an ENDIF.  This ENDIF will eventually replace the
			 * ENDLOOP insruction. */
			endif = ptr->Next;
			rc_remove_instruction(ptr);
			rc_remove_instruction(endif);
			break;
		case RC_OPCODE_ENDLOOP:
			/* Insert the ENDIF before ENDLOOP. */
			rc_insert_instruction(ptr->Prev, endif);
			loop->EndLoop = ptr;
			break;
		}
		ptr = ptr->Next;
	}
	return ptr;
}

static void rc_transform_loops(struct emulate_loop_state * s)
{
	struct rc_instruction * ptr = s->C->Program.Instructions.Next;
	while(ptr != &s->C->Program.Instructions) {
		if(ptr->Type == RC_INSTRUCTION_NORMAL &&
					ptr->U.I.Opcode == RC_OPCODE_BGNLOOP){
			ptr = transform_loop(s, ptr);
		}
		ptr = ptr->Next;
	}
}

static void rc_unroll_loops(struct emulate_loop_state *s,
						unsigned int max_instructions)
{
	int i;
	/* Iterate backwards of the list of loops so that loops that nested
	 * loops are unrolled first.
	 */
	for( i = s->LoopCount - 1; i >= 0; i-- ){
		unsigned int iterations = loop_calc_iterations(&s->Loops[i],
						s->LoopCount, max_instructions);
		loop_unroll(s, &s->Loops[i], iterations);
	}
}

void rc_emulate_loops(struct radeon_compiler *c, unsigned int max_instructions)
{
	struct emulate_loop_state s;

	memset(&s, 0, sizeof(struct emulate_loop_state));
	s.C = c;

	/* We may need to move these two operations to r3xx_(vert|frag)prog.c
	 * and run the optimization passes between them in order to increase
	 * the number of unrolls we can do for each loop.
	 */
	rc_transform_loops(&s);
	
	rc_unroll_loops(&s, max_instructions);
}
