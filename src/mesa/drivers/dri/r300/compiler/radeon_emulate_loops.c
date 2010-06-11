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
#include "radeon_dataflow.h"

#define VERBOSE 0

#define DBG(...) do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while(0)

struct emulate_loop_state {
	struct radeon_compiler * C;
	struct loop_info * Loops;
	unsigned int LoopCount;
	unsigned int LoopReserved;
};

struct loop_info {
	struct rc_instruction * BeginLoop;
	struct rc_instruction * Cond;
	struct rc_instruction * If;
	struct rc_instruction * Brk;
	struct rc_instruction * EndIf;
	struct rc_instruction * EndLoop;
};

struct const_value {
	
	struct radeon_compiler * C;
	struct rc_src_register * Src;
	float Value;
	int HasValue;
};

struct count_inst {
	struct radeon_compiler * C;
	int Index;
	rc_swizzle Swz;
	float Amount;
	int Unknown;
};

static float get_constant_value(struct radeon_compiler * c,
						struct rc_src_register * src,
						int chan)
{
	float base = 1.0f;
	int swz = GET_SWZ(src->Swizzle, chan);
	if(swz >= 4 || src->Index >= c->Program.Constants.Count ){
		rc_error(c, "get_constant_value: Can't find a value.\n");
		return 0.0f;
	}
	if(GET_BIT(src->Negate, chan)){
		base = -1.0f;
	}
	return base *
		c->Program.Constants.Constants[src->Index].u.Immediate[swz];
}

static int src_reg_is_immediate(struct rc_src_register * src,
						struct radeon_compiler * c)
{
	return src->File == RC_FILE_CONSTANT &&
	c->Program.Constants.Constants[src->Index].Type==RC_CONSTANT_IMMEDIATE;
}

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


static void update_const_value(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct const_value * value = data;
	if(value->Src->File != file ||
	   value->Src->Index != index ||
	   !(1 << GET_SWZ(value->Src->Swizzle, 0) & mask)){
	   	return;
	}
	switch(inst->U.I.Opcode){
	case RC_OPCODE_MOV:
		if(!src_reg_is_immediate(&inst->U.I.SrcReg[0], value->C)){
			return;
		}
		value->HasValue = 1;
		value->Value =
			get_constant_value(value->C, &inst->U.I.SrcReg[0], 0);
		break;
	}
}

static void get_incr_amount(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct count_inst * count_inst = data;
	int amnt_src_index;
	const struct rc_opcode_info * opcode;
	float amount;

	if(file != RC_FILE_TEMPORARY ||
	   count_inst->Index != index ||
	   (1 << GET_SWZ(count_inst->Swz,0) != mask)){
	   	return;
	}
	/* Find the index of the counter register. */
	opcode = rc_get_opcode_info(inst->U.I.Opcode);
	if(opcode->NumSrcRegs != 2){
		count_inst->Unknown = 1;
		return;
	}
	if(inst->U.I.SrcReg[0].File == RC_FILE_TEMPORARY &&
	   inst->U.I.SrcReg[0].Index == count_inst->Index &&
	   inst->U.I.SrcReg[0].Swizzle == count_inst->Swz){
		amnt_src_index = 1;
	} else if( inst->U.I.SrcReg[1].File == RC_FILE_TEMPORARY &&
		   inst->U.I.SrcReg[1].Index == count_inst->Index &&
		   inst->U.I.SrcReg[1].Swizzle == count_inst->Swz){
		amnt_src_index = 0;
	}
	else{
		count_inst->Unknown = 1;
		return;
	}
	if(src_reg_is_immediate(&inst->U.I.SrcReg[amnt_src_index],
							count_inst->C)){
		amount = get_constant_value(count_inst->C,
				&inst->U.I.SrcReg[amnt_src_index], 0);
	}
	else{
		count_inst->Unknown = 1 ;
		return;
	}
	switch(inst->U.I.Opcode){
	case RC_OPCODE_ADD:
		count_inst->Amount += amount;
		break;
	case RC_OPCODE_SUB:
		if(amnt_src_index == 0){
			count_inst->Unknown = 0;
			return;
		}
		count_inst->Amount -= amount;
		break;
	default:
		count_inst->Unknown = 1;
		return;
	}
	
}

static int transform_const_loop(struct emulate_loop_state * s,
						struct loop_info * loop,
						struct rc_instruction * cond)
{
	int end_loops = 1;
	int iterations;
	struct count_inst count_inst;
	float limit_value;
	struct rc_src_register * counter;
	struct rc_src_register * limit;
	struct const_value counter_value;
	struct rc_instruction * inst;

	/* Find the counter and the upper limit */
	
	if(src_reg_is_immediate(&cond->U.I.SrcReg[0], s->C)){
		limit = &cond->U.I.SrcReg[0];
		counter = &cond->U.I.SrcReg[1];
	}
	else if(src_reg_is_immediate(&cond->U.I.SrcReg[1], s->C)){
		limit = &cond->U.I.SrcReg[1];
		counter = &cond->U.I.SrcReg[0];
	}
	else{
		DBG("No constant limit.\n");
		return 0;
	}
	
	/* Find the initial value of the counter */
	counter_value.Src = counter;
	counter_value.Value = 0.0f;
	counter_value.HasValue = 0;
	counter_value.C = s->C;
	for(inst = s->C->Program.Instructions.Next; inst != loop->BeginLoop;
							inst = inst->Next){
		rc_for_all_writes_mask(inst, update_const_value, &counter_value);
	}
	if(!counter_value.HasValue){
		DBG("Initial counter value cannot be determined.\n");
		return 0;
	}
	DBG("Initial counter value is %f\n", counter_value.Value);
	/* Determine how the counter is modified each loop */
	count_inst.C = s->C;
	count_inst.Index = counter->Index;
	count_inst.Swz = counter->Swizzle;
	count_inst.Amount = 0.0f;
	count_inst.Unknown = 0;
	for(inst = loop->BeginLoop->Next; end_loops > 0; inst = inst->Next){
		switch(inst->U.I.Opcode){
		/* XXX In the future we might want to try to unroll nested
		 * loops here.*/
		case RC_OPCODE_BGNLOOP:
			end_loops++;
			break;
		case RC_OPCODE_ENDLOOP:
			loop->EndLoop = inst;
			end_loops--;
			break;
		/* XXX Check if the counter is modified within an if statement.
		 */
		case RC_OPCODE_IF:
			break;
		default:
			rc_for_all_writes_mask(inst, get_incr_amount, &count_inst);
			if(count_inst.Unknown){
				return 0;
			}
			break;
		}
	}
	/* Infinite loop */
	if(count_inst.Amount == 0.0f){
		return 0;
	}
	DBG("Counter is increased by %f each iteration.\n", count_inst.Amount);
	/* Calculate the number of iterations of this loop.  Keeping this
	 * simple, since we only support increment and decrement loops.
	 */
	limit_value = get_constant_value(s->C, limit, 0);
	iterations = (int) ((limit_value - counter_value.Value) /
							count_inst.Amount);

	DBG("Loop will have %d iterations.\n", iterations);
	
	/* Prepare loop for unrolling */
	rc_remove_instruction(loop->Cond);
	rc_remove_instruction(loop->If);
	rc_remove_instruction(loop->Brk);
	rc_remove_instruction(loop->EndIf);
	
	loop_unroll(s, loop, iterations);
	loop->EndLoop = NULL;
	return 1;
}

/** 
 * This function prepares a loop to be unrolled by converting it into an if
 * statement.  Here is an outline of the conversion process:
 * BGNLOOP;                         	-> BGNLOOP;
 * <Additional conditional code>	-> <Additional conditional code>
 * SGE/SLT temp[0], temp[1], temp[2];	-> SLT/SGE temp[0], temp[1], temp[2];
 * IF temp[0];                      	-> IF temp[0];
 * BRK;                             	->
 * ENDIF;                           	-> <Loop Body>
 * <Loop Body>                      	-> ENDIF;
 * ENDLOOP;                         	-> ENDLOOP
 *
 * @param inst A pointer to a BGNLOOP instruction.
 * @return If the loop can be unrolled, a pointer to the first instruction of
 * 		the unrolled loop.
 * 	   Otherwise, A pointer to the ENDLOOP instruction.
 * 	   Null if there is an error.
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
	if(inst->U.I.Opcode != RC_OPCODE_BGNLOOP){
		rc_error(s->C, "expected BGNLOOP\n", __FUNCTION__);
		return NULL;
	}
	loop->BeginLoop = inst;

	for(ptr = loop->BeginLoop->Next; !loop->EndLoop; ptr = ptr->Next){
		switch(ptr->U.I.Opcode){
		case RC_OPCODE_BGNLOOP:
			/* Nested loop */
			ptr = transform_loop(s, ptr);
			if(!ptr){
				return NULL;
			}
			break;
		case RC_OPCODE_BRK:
			loop->Brk = ptr;
			if(ptr->Next->U.I.Opcode != RC_OPCODE_ENDIF){
				rc_error(s->C,
					"%s: expected ENDIF\n",__FUNCTION__);
				return NULL;
			}
			loop->EndIf = ptr->Next;
			if(ptr->Prev->U.I.Opcode != RC_OPCODE_IF){
				rc_error(s->C,
					"%s: expected IF\n", __FUNCTION__);
				return NULL;
			}
			loop->If = ptr->Prev;
			switch(loop->If->Prev->U.I.Opcode){
			case RC_OPCODE_SLT:
			case RC_OPCODE_SGE:
			case RC_OPCODE_SGT:
			case RC_OPCODE_SLE:
			case RC_OPCODE_SEQ:
			case RC_OPCODE_SNE:
				break;
			default:
				rc_error(s->C, "%s expected conditional\n",
								__FUNCTION__);
				return NULL;
			}
			loop->Cond = loop->If->Prev;
			ptr = loop->EndIf;
			break;
		case RC_OPCODE_ENDLOOP:
			loop->EndLoop = ptr;
			break;
		}
	}
	/* Reverse the conditional instruction */
	switch(loop->Cond->U.I.Opcode){
	case RC_OPCODE_SGE:
		loop->Cond->U.I.Opcode = RC_OPCODE_SLT;
		break;
	case RC_OPCODE_SLT:
		loop->Cond->U.I.Opcode = RC_OPCODE_SGE;
		break;
	case RC_OPCODE_SLE:
		loop->Cond->U.I.Opcode = RC_OPCODE_SGT;
		break;
	case RC_OPCODE_SGT:
		loop->Cond->U.I.Opcode = RC_OPCODE_SLE;
		break;
	case RC_OPCODE_SEQ:
		loop->Cond->U.I.Opcode = RC_OPCODE_SNE;
		break;
	case RC_OPCODE_SNE:
		loop->Cond->U.I.Opcode = RC_OPCODE_SEQ;
		break;
	default:
		rc_error(s->C, "loop->Cond is not a conditional.\n");
		return NULL;
	}
	
	/* Check if the number of loops is known at compile time. */
	if(transform_const_loop(s, loop, ptr)){
		return loop->BeginLoop->Next;
	}

	/* Prepare the loop to be unrolled */
	rc_remove_instruction(loop->Brk);
	rc_remove_instruction(loop->EndIf);
	rc_insert_instruction(loop->EndLoop->Prev, loop->EndIf);
	return loop->EndLoop;
}

static void rc_transform_loops(struct emulate_loop_state * s)
{
	struct rc_instruction * ptr = s->C->Program.Instructions.Next;
	while(ptr != &s->C->Program.Instructions) {
		if(ptr->Type == RC_INSTRUCTION_NORMAL &&
					ptr->U.I.Opcode == RC_OPCODE_BGNLOOP){
			ptr = transform_loop(s, ptr);
			if(!ptr){
				return;
			}
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
		if(!s->Loops[i].EndLoop){
			continue;
		}
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
