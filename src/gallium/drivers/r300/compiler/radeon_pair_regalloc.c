/*
 * Copyright (C) 2009 Nicolai Haehnle.
 * Copyright 2011 Tom Stellard <tstellar@gmail.com>
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

#include "radeon_program_pair.h"

#include <stdio.h>

#include "main/glheader.h"
#include "program/register_allocate.h"
#include "util/u_memory.h"
#include "ralloc.h"

#include "r300_fragprog_swizzle.h"
#include "radeon_compiler.h"
#include "radeon_compiler_util.h"
#include "radeon_dataflow.h"
#include "radeon_list.h"
#include "radeon_regalloc.h"
#include "radeon_variable.h"

#define VERBOSE 0

#define DBG(...) do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while(0)



struct register_info {
	struct live_intervals Live[4];

	unsigned int Used:1;
	unsigned int Allocated:1;
	unsigned int File:3;
	unsigned int Index:RC_REGISTER_INDEX_BITS;
	unsigned int Writemask;
};

struct regalloc_state {
	struct radeon_compiler * C;

	struct register_info * Input;
	unsigned int NumInputs;

	struct register_info * Temporary;
	unsigned int NumTemporaries;

	unsigned int Simple;
	int LoopEnd;
};

struct rc_class {
	enum rc_reg_class ID;

	unsigned int WritemaskCount;

	/** List of writemasks that belong to this class */
	unsigned int Writemasks[3];


};

static const struct rc_class rc_class_list [] = {
	{RC_REG_CLASS_SINGLE, 3,
		{RC_MASK_X,
		 RC_MASK_Y,
		 RC_MASK_Z}},
	{RC_REG_CLASS_DOUBLE, 3,
		{RC_MASK_X | RC_MASK_Y,
		 RC_MASK_X | RC_MASK_Z,
		 RC_MASK_Y | RC_MASK_Z}},
	{RC_REG_CLASS_TRIPLE, 1,
		{RC_MASK_X | RC_MASK_Y | RC_MASK_Z,
		 RC_MASK_NONE,
		 RC_MASK_NONE}},
	{RC_REG_CLASS_ALPHA, 1,
		{RC_MASK_W,
		 RC_MASK_NONE,
		 RC_MASK_NONE}},
	{RC_REG_CLASS_SINGLE_PLUS_ALPHA, 3,
		{RC_MASK_X | RC_MASK_W,
		 RC_MASK_Y | RC_MASK_W,
		 RC_MASK_Z | RC_MASK_W}},
	{RC_REG_CLASS_DOUBLE_PLUS_ALPHA, 3,
		{RC_MASK_X | RC_MASK_Y | RC_MASK_W,
		 RC_MASK_X | RC_MASK_Z | RC_MASK_W,
		 RC_MASK_Y | RC_MASK_Z | RC_MASK_W}},
	{RC_REG_CLASS_TRIPLE_PLUS_ALPHA, 1,
		{RC_MASK_X | RC_MASK_Y | RC_MASK_Z | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_X, 1,
		{RC_MASK_X,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_Y, 1,
		{RC_MASK_Y,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_Z, 1,
		{RC_MASK_Z,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_XY, 1,
		{RC_MASK_X | RC_MASK_Y,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_YZ, 1,
		{RC_MASK_Y | RC_MASK_Z,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_XZ, 1,
		{RC_MASK_X | RC_MASK_Z,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_XW, 1,
		{RC_MASK_X | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_YW, 1,
		{RC_MASK_Y | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_ZW, 1,
		{RC_MASK_Z | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_XYW, 1,
		{RC_MASK_X | RC_MASK_Y | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_YZW, 1,
		{RC_MASK_Y | RC_MASK_Z | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}},
	{RC_REG_CLASS_XZW, 1,
		{RC_MASK_X | RC_MASK_Z | RC_MASK_W,
		RC_MASK_NONE,
		RC_MASK_NONE}}
};

static void print_live_intervals(struct live_intervals * src)
{
	if (!src || !src->Used) {
		DBG("(null)");
		return;
	}

	DBG("(%i,%i)", src->Start, src->End);
}

static int overlap_live_intervals(struct live_intervals * a, struct live_intervals * b)
{
	if (VERBOSE) {
		DBG("overlap_live_intervals: ");
		print_live_intervals(a);
		DBG(" to ");
		print_live_intervals(b);
		DBG("\n");
	}

	if (!a->Used || !b->Used) {
		DBG("    unused interval\n");
		return 0;
	}

	if (a->Start > b->Start) {
		if (a->Start < b->End) {
			DBG("    overlap\n");
			return 1;
		}
	} else if (b->Start > a->Start) {
		if (b->Start < a->End) {
			DBG("    overlap\n");
			return 1;
		}
	} else { /* a->Start == b->Start */
		if (a->Start != a->End && b->Start != b->End) {
			DBG("    overlap\n");
			return 1;
		}
	}

	DBG("    no overlap\n");

	return 0;
}

static void scan_read_callback(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct regalloc_state * s = data;
	struct register_info * reg;
	unsigned int i;

	if (file != RC_FILE_INPUT)
		return;

	s->Input[index].Used = 1;
	reg = &s->Input[index];

	for (i = 0; i < 4; i++) {
		if (!((mask >> i) & 0x1)) {
			continue;
		}
		reg->Live[i].Used = 1;
		reg->Live[i].Start = 0;
		reg->Live[i].End =
			s->LoopEnd > inst->IP ? s->LoopEnd : inst->IP;
	}
}

static void remap_register(void * data, struct rc_instruction * inst,
		rc_register_file * file, unsigned int * index)
{
	struct regalloc_state * s = data;
	const struct register_info * reg;

	if (*file == RC_FILE_TEMPORARY && s->Simple)
		reg = &s->Temporary[*index];
	else if (*file == RC_FILE_INPUT)
		reg = &s->Input[*index];
	else
		return;

	if (reg->Allocated) {
		*index = reg->Index;
	}
}

static void alloc_input_simple(void * data, unsigned int input,
							unsigned int hwreg)
{
	struct regalloc_state * s = data;

	if (input >= s->NumInputs)
		return;

	s->Input[input].Allocated = 1;
	s->Input[input].File = RC_FILE_TEMPORARY;
	s->Input[input].Index = hwreg;
}

/* This functions offsets the temporary register indices by the number
 * of input registers, because input registers are actually temporaries and
 * should not occupy the same space.
 *
 * This pass is supposed to be used to maintain correct allocation of inputs
 * if the standard register allocation is disabled. */
static void do_regalloc_inputs_only(struct regalloc_state * s)
{
	for (unsigned i = 0; i < s->NumTemporaries; i++) {
		s->Temporary[i].Allocated = 1;
		s->Temporary[i].File = RC_FILE_TEMPORARY;
		s->Temporary[i].Index = i + s->NumInputs;
	}
}

static unsigned int is_derivative(rc_opcode op)
{
	return (op == RC_OPCODE_DDX || op == RC_OPCODE_DDY);
}

static int find_class(
	const struct rc_class * classes,
	unsigned int writemask,
	unsigned int max_writemask_count)
{
	unsigned int i;
	for (i = 0; i < RC_REG_CLASS_COUNT; i++) {
		unsigned int j;
		if (classes[i].WritemaskCount > max_writemask_count) {
			continue;
		}
		for (j = 0; j < 3; j++) {
			if (classes[i].Writemasks[j] == writemask) {
				return i;
			}
		}
	}
	return -1;
}

struct variable_get_class_cb_data {
	unsigned int * can_change_writemask;
	unsigned int conversion_swizzle;
};

static void variable_get_class_read_cb(
	void * userdata,
	struct rc_instruction * inst,
	struct rc_pair_instruction_arg * arg,
	struct rc_pair_instruction_source * src)
{
	struct variable_get_class_cb_data * d = userdata;
	unsigned int new_swizzle = rc_adjust_channels(arg->Swizzle,
							d->conversion_swizzle);
	if (!r300_swizzle_is_native_basic(new_swizzle)) {
		*d->can_change_writemask = 0;
	}
}

static enum rc_reg_class variable_get_class(
	struct rc_variable * variable,
	const struct rc_class * classes)
{
	unsigned int i;
	unsigned int can_change_writemask= 1;
	unsigned int writemask = rc_variable_writemask_sum(variable);
	struct rc_list * readers = rc_variable_readers_union(variable);
	int class_index;

	if (!variable->C->is_r500) {
		struct rc_class c;
		struct rc_variable * var_ptr;
		/* The assumption here is that if an instruction has type
		 * RC_INSTRUCTION_NORMAL then it is a TEX instruction.
		 * r300 and r400 can't swizzle the result of a TEX lookup. */
		for (var_ptr = variable; var_ptr; var_ptr = var_ptr->Friend) {
			if (var_ptr->Inst->Type == RC_INSTRUCTION_NORMAL) {
				writemask = RC_MASK_XYZW;
			}
		}

		/* Check if it is possible to do swizzle packing for r300/r400
		 * without creating non-native swizzles. */
		class_index = find_class(classes, writemask, 3);
		if (class_index < 0) {
			goto error;
		}
		c = classes[class_index];
		if (c.WritemaskCount == 1) {
			goto done;
		}
		for (i = 0; i < c.WritemaskCount; i++) {
			struct rc_variable * var_ptr;
			for (var_ptr = variable; var_ptr;
						var_ptr = var_ptr->Friend) {
				int j;
				unsigned int conversion_swizzle =
						rc_make_conversion_swizzle(
						writemask, c.Writemasks[i]);
				struct variable_get_class_cb_data d;
				d.can_change_writemask = &can_change_writemask;
				d.conversion_swizzle = conversion_swizzle;
				/* If we get this far var_ptr->Inst has to
				 * be a pair instruction.  If variable or any
				 * of its friends are normal instructions,
				 * then the writemask will be set to RC_MASK_XYZW
				 * and the function will return before it gets
				 * here. */
				rc_pair_for_all_reads_arg(var_ptr->Inst,
					variable_get_class_read_cb, &d);

				for (j = 0; j < var_ptr->ReaderCount; j++) {
					unsigned int old_swizzle;
					unsigned int new_swizzle;
					struct rc_reader r = var_ptr->Readers[j];
					if (r.Inst->Type ==
							RC_INSTRUCTION_PAIR ) {
						old_swizzle = r.U.P.Arg->Swizzle;
					} else {
						/* Source operands of TEX
						 * instructions can't be
						 * swizzle on r300/r400 GPUs.
						 */
						if (!variable->C->is_r500) {
							can_change_writemask = 0;
							break;
						}
						old_swizzle = r.U.I.Src->Swizzle;
					}
					new_swizzle = rc_adjust_channels(
						old_swizzle, conversion_swizzle);
					if (!r300_swizzle_is_native_basic(
								new_swizzle)) {
						can_change_writemask = 0;
						break;
					}
				}
				if (!can_change_writemask) {
					break;
				}
			}
			if (!can_change_writemask) {
				break;
			}
		}
	}

	if (variable->Inst->Type == RC_INSTRUCTION_PAIR) {
		/* DDX/DDY seem to always fail when their writemasks are
		 * changed.*/
		if (is_derivative(variable->Inst->U.P.RGB.Opcode)
		    || is_derivative(variable->Inst->U.P.Alpha.Opcode)) {
			can_change_writemask = 0;
		}
	}
	for ( ; readers; readers = readers->Next) {
		struct rc_reader * r = readers->Item;
		if (r->Inst->Type == RC_INSTRUCTION_PAIR) {
			if (r->U.P.Arg->Source == RC_PAIR_PRESUB_SRC) {
				can_change_writemask = 0;
				break;
			}
			/* DDX/DDY also fail when their swizzles are changed. */
			if (is_derivative(r->Inst->U.P.RGB.Opcode)
			    || is_derivative(r->Inst->U.P.Alpha.Opcode)) {
				can_change_writemask = 0;
				break;
			}
		}
	}

	class_index = find_class(classes, writemask,
						can_change_writemask ? 3 : 1);
done:
	if (class_index > -1) {
		return classes[class_index].ID;
	} else {
error:
		rc_error(variable->C,
				"Could not find class for index=%u mask=%u\n",
				variable->Dst.Index, writemask);
		return 0;
	}
}

static unsigned int overlap_live_intervals_array(
	struct live_intervals * a,
	struct live_intervals * b)
{
	unsigned int a_chan, b_chan;
	for (a_chan = 0; a_chan < 4; a_chan++) {
		for (b_chan = 0; b_chan < 4; b_chan++) {
			if (overlap_live_intervals(&a[a_chan], &b[b_chan])) {
					return 1;
			}
		}
	}
	return 0;
}

static unsigned int reg_get_index(int reg)
{
	return reg / RC_MASK_XYZW;
}

static unsigned int reg_get_writemask(int reg)
{
	return (reg % RC_MASK_XYZW) + 1;
}

static int get_reg_id(unsigned int index, unsigned int writemask)
{
	assert(writemask);
	if (writemask == 0) {
		return 0;
	}
	return (index * RC_MASK_XYZW) + (writemask - 1);
}

#if VERBOSE
static void print_reg(int reg)
{
	unsigned int index = reg_get_index(reg);
	unsigned int mask = reg_get_writemask(reg);
	fprintf(stderr, "Temp[%u].%c%c%c%c", index,
		mask & RC_MASK_X ? 'x' : '_',
		mask & RC_MASK_Y ? 'y' : '_',
		mask & RC_MASK_Z ? 'z' : '_',
		mask & RC_MASK_W ? 'w' : '_');
}
#endif

static void add_register_conflicts(
	struct ra_regs * regs,
	unsigned int max_temp_regs)
{
	unsigned int index, a_mask, b_mask;
	for (index = 0; index < max_temp_regs; index++) {
		for(a_mask = 1; a_mask <= RC_MASK_XYZW; a_mask++) {
			for (b_mask = a_mask + 1; b_mask <= RC_MASK_XYZW;
								b_mask++) {
				if (a_mask & b_mask) {
					ra_add_reg_conflict(regs,
						get_reg_id(index, a_mask),
						get_reg_id(index, b_mask));
				}
			}
		}
	}
}

static void do_advanced_regalloc(struct regalloc_state * s)
{

	unsigned int i, input_node, node_count, node_index;
	unsigned int * node_classes;
	struct rc_instruction * inst;
	struct rc_list * var_ptr;
	struct rc_list * variables;
	struct ra_graph * graph;
	const struct rc_regalloc_state *ra_state = s->C->regalloc_state;

	/* Get list of program variables */
	variables = rc_get_variables(s->C);
	node_count = rc_list_count(variables);
	node_classes = memory_pool_malloc(&s->C->Pool,
			node_count * sizeof(unsigned int));

	for (var_ptr = variables, node_index = 0; var_ptr;
					var_ptr = var_ptr->Next, node_index++) {
		unsigned int class_index;
		/* Compute the live intervals */
		rc_variable_compute_live_intervals(var_ptr->Item);

		class_index = variable_get_class(var_ptr->Item,	rc_class_list);
		node_classes[node_index] = ra_state->class_ids[class_index];
	}


	/* Calculate live intervals for input registers */
	for (inst = s->C->Program.Instructions.Next;
					inst != &s->C->Program.Instructions;
					inst = inst->Next) {
		rc_opcode op = rc_get_flow_control_inst(inst);
		if (op == RC_OPCODE_BGNLOOP) {
			struct rc_instruction * endloop =
							rc_match_bgnloop(inst);
			if (endloop->IP > s->LoopEnd) {
				s->LoopEnd = endloop->IP;
			}
		}
		rc_for_all_reads_mask(inst, scan_read_callback, s);
	}

	/* Compute the writemask for inputs. */
	for (i = 0; i < s->NumInputs; i++) {
		unsigned int chan, writemask = 0;
		for (chan = 0; chan < 4; chan++) {
			if (s->Input[i].Live[chan].Used) {
				writemask |= (1 << chan);
			}
		}
		s->Input[i].Writemask = writemask;
	}

	graph = ra_alloc_interference_graph(ra_state->regs,
						node_count + s->NumInputs);

	/* Build the interference graph */
	for (var_ptr = variables, node_index = 0; var_ptr;
					var_ptr = var_ptr->Next,node_index++) {
		struct rc_list * a, * b;
		unsigned int b_index;

		ra_set_node_class(graph, node_index, node_classes[node_index]);

		for (a = var_ptr, b = var_ptr->Next, b_index = node_index + 1;
						b; b = b->Next, b_index++) {
			struct rc_variable * var_a = a->Item;
			while (var_a) {
				struct rc_variable * var_b = b->Item;
				while (var_b) {
					if (overlap_live_intervals_array(var_a->Live, var_b->Live)) {
						ra_add_node_interference(graph,
							node_index, b_index);
					}
					var_b = var_b->Friend;
				}
				var_a = var_a->Friend;
			}
		}
	}

	/* Add input registers to the interference graph */
	for (i = 0, input_node = 0; i< s->NumInputs; i++) {
		if (!s->Input[i].Writemask) {
			continue;
		}
		for (var_ptr = variables, node_index = 0;
				var_ptr; var_ptr = var_ptr->Next, node_index++) {
			struct rc_variable * var = var_ptr->Item;
			if (overlap_live_intervals_array(s->Input[i].Live,
								var->Live)) {
				ra_add_node_interference(graph, node_index,
						node_count + input_node);
			}
		}
		/* Manually allocate a register for this input */
		ra_set_node_reg(graph, node_count + input_node, get_reg_id(
				s->Input[i].Index, s->Input[i].Writemask));
		input_node++;
	}

	if (!ra_allocate_no_spills(graph)) {
		rc_error(s->C, "Ran out of hardware temporaries\n");
		return;
	}

	/* Rewrite the registers */
	for (var_ptr = variables, node_index = 0; var_ptr;
				var_ptr = var_ptr->Next, node_index++) {
		int reg = ra_get_node_reg(graph, node_index);
		unsigned int writemask = reg_get_writemask(reg);
		unsigned int index = reg_get_index(reg);
		struct rc_variable * var = var_ptr->Item;

		if (!s->C->is_r500 && var->Inst->Type == RC_INSTRUCTION_NORMAL) {
			writemask = rc_variable_writemask_sum(var);
		}

		if (var->Dst.File == RC_FILE_INPUT) {
			continue;
		}
		rc_variable_change_dst(var, index, writemask);
	}

	ralloc_free(graph);
}

void rc_init_regalloc_state(struct rc_regalloc_state *s)
{
	unsigned i, j, index;
	unsigned **ra_q_values;

	/* Pre-computed q values.  This array describes the maximum number of
	 * a class's [row] registers that are in conflict with a single
	 * register from another class [column].
	 *
	 * For example:
	 * q_values[0][2] is 3, because a register from class 2
	 * (RC_REG_CLASS_TRIPLE) may conflict with at most 3 registers from
	 * class 0 (RC_REG_CLASS_SINGLE) e.g. T0.xyz conflicts with T0.x, T0.y,
	 * and T0.z.
	 *
	 * q_values[2][0] is 1, because a register from class 0
	 * (RC_REG_CLASS_SINGLE) may conflict with at most 1 register from
	 * class 2 (RC_REG_CLASS_TRIPLE) e.g. T0.x conflicts with T0.xyz
	 *
	 * The q values for each register class [row] will never be greater
	 * than the maximum number of writemask combinations for that class.
	 *
	 * For example:
	 *
	 * Class 2 (RC_REG_CLASS_TRIPLE) only has 1 writemask combination,
	 * so no value in q_values[2][0..RC_REG_CLASS_COUNT] will be greater
	 * than 1.
	 */
	const unsigned q_values[RC_REG_CLASS_COUNT][RC_REG_CLASS_COUNT] = {
	{1, 2, 3, 0, 1, 2, 3, 1, 1, 1, 2, 2, 2, 1, 1, 1, 2, 2, 2},
	{2, 3, 3, 0, 2, 3, 3, 2, 2, 2, 3, 3, 3, 2, 2, 2, 3, 3, 3},
	{1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1},
	{1, 2, 3, 3, 3, 3, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3},
	{2, 3, 3, 3, 3, 3, 3, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1},
	{1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0},
	{1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 0, 1, 1},
	{1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1},
	{1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1},
	{1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
	};

	/* Allocate the main ra data structure */
	s->regs = ra_alloc_reg_set(NULL, R500_PFS_NUM_TEMP_REGS * RC_MASK_XYZW);

	/* Create the register classes */
	for (i = 0; i < RC_REG_CLASS_COUNT; i++) {
		const struct rc_class *class = &rc_class_list[i];
		s->class_ids[class->ID] = ra_alloc_reg_class(s->regs);

		/* Assign registers to the classes */
		for (index = 0; index < R500_PFS_NUM_TEMP_REGS; index++) {
			for (j = 0; j < class->WritemaskCount; j++) {
				int reg_id = get_reg_id(index,
						class->Writemasks[j]);
				ra_class_add_reg(s->regs,
					s->class_ids[class->ID], reg_id);
			}
		}
	}

	/* Set the q values.  The q_values array is indexed based on
	 * the rc_reg_class ID (RC_REG_CLASS_*) which might be
	 * different than the ID assigned to that class by ra.
	 * This why we need to manually construct this list.
	 */
	ra_q_values = MALLOC(RC_REG_CLASS_COUNT * sizeof(unsigned *));

	for (i = 0; i < RC_REG_CLASS_COUNT; i++) {
		ra_q_values[i] = MALLOC(RC_REG_CLASS_COUNT * sizeof(unsigned));
		for (j = 0; j < RC_REG_CLASS_COUNT; j++) {
			ra_q_values[s->class_ids[i]][s->class_ids[j]] =
							q_values[i][j];
		}
	}

	/* Add register conflicts */
	add_register_conflicts(s->regs, R500_PFS_NUM_TEMP_REGS);

	ra_set_finalize(s->regs, ra_q_values);

	for (i = 0; i < RC_REG_CLASS_COUNT; i++) {
		FREE(ra_q_values[i]);
	}
	FREE(ra_q_values);
}

void rc_destroy_regalloc_state(struct rc_regalloc_state *s)
{
	ralloc_free(s->regs);
}

/**
 * @param user This parameter should be a pointer to an integer value.  If this
 * integer value is zero, then a simple register allocator will be used that
 * only allocates space for input registers (\sa do_regalloc_inputs_only).  If
 * user is non-zero, then the regular register allocator will be used
 * (\sa do_regalloc).
  */
void rc_pair_regalloc(struct radeon_compiler *cc, void *user)
{
	struct r300_fragment_program_compiler *c =
				(struct r300_fragment_program_compiler*)cc;
	struct regalloc_state s;
	int * do_full_regalloc = (int*)user;

	memset(&s, 0, sizeof(s));
	s.C = cc;
	s.NumInputs = rc_get_max_index(cc, RC_FILE_INPUT) + 1;
	s.Input = memory_pool_malloc(&cc->Pool,
			s.NumInputs * sizeof(struct register_info));
	memset(s.Input, 0, s.NumInputs * sizeof(struct register_info));

	s.NumTemporaries = rc_get_max_index(cc, RC_FILE_TEMPORARY) + 1;
	s.Temporary = memory_pool_malloc(&cc->Pool,
			s.NumTemporaries * sizeof(struct register_info));
	memset(s.Temporary, 0, s.NumTemporaries * sizeof(struct register_info));

	rc_recompute_ips(s.C);

	c->AllocateHwInputs(c, &alloc_input_simple, &s);
	if (*do_full_regalloc) {
		do_advanced_regalloc(&s);
	} else {
		s.Simple = 1;
		do_regalloc_inputs_only(&s);
	}

	/* Rewrite inputs and if we are doing the simple allocation, rewrite
	 * temporaries too. */
	for (struct rc_instruction *inst = s.C->Program.Instructions.Next;
					inst != &s.C->Program.Instructions;
					inst = inst->Next) {
		rc_remap_registers(inst, &remap_register, &s);
	}
}
