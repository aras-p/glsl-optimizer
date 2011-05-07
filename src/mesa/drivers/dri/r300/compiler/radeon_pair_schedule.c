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

#include "radeon_program_pair.h"

#include <stdio.h>

#include "radeon_compiler.h"
#include "radeon_compiler_util.h"
#include "radeon_dataflow.h"


#define VERBOSE 0

#define DBG(...) do { if (VERBOSE) fprintf(stderr, __VA_ARGS__); } while(0)

struct schedule_instruction {
	struct rc_instruction * Instruction;

	/** Next instruction in the linked list of ready instructions. */
	struct schedule_instruction *NextReady;

	/** Values that this instruction reads and writes */
	struct reg_value * WriteValues[4];
	struct reg_value * ReadValues[12];
	unsigned int NumWriteValues:3;
	unsigned int NumReadValues:4;

	/**
	 * Number of (read and write) dependencies that must be resolved before
	 * this instruction can be scheduled.
	 */
	unsigned int NumDependencies:5;

	/** List of all readers (see rc_get_readers() for the definition of
	 * "all readers"), even those outside the basic block this instruction
	 * lives in. */
	struct rc_reader_data GlobalReaders;
};


/**
 * Used to keep track of which instructions read a value.
 */
struct reg_value_reader {
	struct schedule_instruction *Reader;
	struct reg_value_reader *Next;
};

/**
 * Used to keep track which values are stored in each component of a
 * RC_FILE_TEMPORARY.
 */
struct reg_value {
	struct schedule_instruction * Writer;

	/**
	 * Unordered linked list of instructions that read from this value.
	 * When this value becomes available, we increase all readers'
	 * dependency count.
	 */
	struct reg_value_reader *Readers;

	/**
	 * Number of readers of this value. This is decremented each time
	 * a reader of the value is committed.
	 * When the reader cound reaches zero, the dependency count
	 * of the instruction writing \ref Next is decremented.
	 */
	unsigned int NumReaders;

	struct reg_value *Next; /**< Pointer to the next value to be written to the same register */
};

struct register_state {
	struct reg_value * Values[4];
};

struct remap_reg {
	struct rc_instruciont * Inst;
	unsigned int OldIndex:(RC_REGISTER_INDEX_BITS+1);
	unsigned int OldSwizzle:3;
	unsigned int NewIndex:(RC_REGISTER_INDEX_BITS+1);
	unsigned int NewSwizzle:3;
	unsigned int OnlyTexReads:1;
	struct remap_reg * Next;
};

struct schedule_state {
	struct radeon_compiler * C;
	struct schedule_instruction * Current;

	struct register_state Temporary[RC_REGISTER_MAX_INDEX];

	/**
	 * Linked lists of instructions that can be scheduled right now,
	 * based on which ALU/TEX resources they require.
	 */
	/*@{*/
	struct schedule_instruction *ReadyFullALU;
	struct schedule_instruction *ReadyRGB;
	struct schedule_instruction *ReadyAlpha;
	struct schedule_instruction *ReadyTEX;
	/*@}*/
};

static struct reg_value ** get_reg_valuep(struct schedule_state * s,
		rc_register_file file, unsigned int index, unsigned int chan)
{
	if (file != RC_FILE_TEMPORARY)
		return 0;

	if (index >= RC_REGISTER_MAX_INDEX) {
		rc_error(s->C, "%s: index %i out of bounds\n", __FUNCTION__, index);
		return 0;
	}

	return &s->Temporary[index].Values[chan];
}

static void add_inst_to_list(struct schedule_instruction ** list, struct schedule_instruction * inst)
{
	inst->NextReady = *list;
	*list = inst;
}

static void add_inst_to_list_end(struct schedule_instruction ** list,
					struct schedule_instruction * inst)
{
	if(!*list){
		*list = inst;
	}else{
		struct schedule_instruction * temp = *list;
		while(temp->NextReady){
			temp = temp->NextReady;
		}
		temp->NextReady = inst;
	}
}

static void instruction_ready(struct schedule_state * s, struct schedule_instruction * sinst)
{
	DBG("%i is now ready\n", sinst->Instruction->IP);

	/* Adding Ready TEX instructions to the end of the "Ready List" helps
	 * us emit TEX instructions in blocks without losing our place. */
	if (sinst->Instruction->Type == RC_INSTRUCTION_NORMAL)
		add_inst_to_list_end(&s->ReadyTEX, sinst);
	else if (sinst->Instruction->U.P.Alpha.Opcode == RC_OPCODE_NOP)
		add_inst_to_list(&s->ReadyRGB, sinst);
	else if (sinst->Instruction->U.P.RGB.Opcode == RC_OPCODE_NOP)
		add_inst_to_list(&s->ReadyAlpha, sinst);
	else
		add_inst_to_list(&s->ReadyFullALU, sinst);
}

static void decrease_dependencies(struct schedule_state * s, struct schedule_instruction * sinst)
{
	assert(sinst->NumDependencies > 0);
	sinst->NumDependencies--;
	if (!sinst->NumDependencies)
		instruction_ready(s, sinst);
}

/**
 * This function decreases the dependencies of the next instruction that
 * wants to write to each of sinst's read values.
 */
static void commit_update_reads(struct schedule_state * s,
					struct schedule_instruction * sinst){
	unsigned int i;
	for(i = 0; i < sinst->NumReadValues; ++i) {
		struct reg_value * v = sinst->ReadValues[i];
		assert(v->NumReaders > 0);
		v->NumReaders--;
		if (!v->NumReaders) {
			if (v->Next)
				decrease_dependencies(s, v->Next->Writer);
		}
	}
}

static void commit_update_writes(struct schedule_state * s,
					struct schedule_instruction * sinst){
	unsigned int i;
	for(i = 0; i < sinst->NumWriteValues; ++i) {
		struct reg_value * v = sinst->WriteValues[i];
		if (v->NumReaders) {
			for(struct reg_value_reader * r = v->Readers; r; r = r->Next) {
				decrease_dependencies(s, r->Reader);
			}
		} else {
			/* This happens in instruction sequences of the type
			 *  OP r.x, ...;
			 *  OP r.x, r.x, ...;
			 * See also the subtlety in how instructions that both
			 * read and write the same register are scanned.
			 */
			if (v->Next)
				decrease_dependencies(s, v->Next->Writer);
		}
	}
}

static void commit_alu_instruction(struct schedule_state * s, struct schedule_instruction * sinst)
{
	DBG("%i: commit\n", sinst->Instruction->IP);

	commit_update_reads(s, sinst);

	commit_update_writes(s, sinst);
}

/**
 * Emit all ready texture instructions in a single block.
 *
 * Emit as a single block to (hopefully) sample many textures in parallel,
 * and to avoid hardware indirections on R300.
 */
static void emit_all_tex(struct schedule_state * s, struct rc_instruction * before)
{
	struct schedule_instruction *readytex;
	struct rc_instruction * inst_begin;

	assert(s->ReadyTEX);

	/* Node marker for R300 */
	inst_begin = rc_insert_new_instruction(s->C, before->Prev);
	inst_begin->U.I.Opcode = RC_OPCODE_BEGIN_TEX;

	/* Link texture instructions back in */
	readytex = s->ReadyTEX;
	while(readytex) {
		rc_insert_instruction(before->Prev, readytex->Instruction);
		DBG("%i: commit TEX reads\n", readytex->Instruction->IP);

		/* All of the TEX instructions in the same TEX block have
		 * their source registers read from before any of the
		 * instructions in that block write to their destination
		 * registers.  This means that when we commit a TEX
		 * instruction, any other TEX instruction that wants to write
		 * to one of the committed instruction's source register can be
		 * marked as ready and should be emitted in the same TEX
		 * block. This prevents the following sequence from being
		 * emitted in two different TEX blocks:
		 * 0: TEX temp[0].xyz, temp[1].xy__, 2D[0];
		 * 1: TEX temp[1].xyz, temp[2].xy__, 2D[0];
		 */
		commit_update_reads(s, readytex);
		readytex = readytex->NextReady;
	}
	readytex = s->ReadyTEX;
	s->ReadyTEX = 0;
	while(readytex){
		DBG("%i: commit TEX writes\n", readytex->Instruction->IP);
		commit_update_writes(s, readytex);
		readytex = readytex->NextReady;
	}
}

/* This is a helper function for destructive_merge_instructions().  It helps
 * merge presubtract sources from two instructions and makes sure the
 * presubtract sources end up in the correct spot.  This function assumes that
 * dst_full is an rgb instruction, meaning that it has a vector instruction(rgb)
 * but no scalar instruction (alpha).
 * @return 0 if merging the presubtract sources fails.
 * @retrun 1 if merging the presubtract sources succeeds.
 */
static int merge_presub_sources(
	struct rc_pair_instruction * dst_full,
	struct rc_pair_sub_instruction src,
	unsigned int type)
{
	unsigned int srcp_src, srcp_regs, is_rgb, is_alpha;
	struct rc_pair_sub_instruction * dst_sub;
	const struct rc_opcode_info * info;

	assert(dst_full->Alpha.Opcode == RC_OPCODE_NOP);

	switch(type) {
	case RC_SOURCE_RGB:
		is_rgb = 1;
		is_alpha = 0;
		dst_sub = &dst_full->RGB;
		break;
	case RC_SOURCE_ALPHA:
		is_rgb = 0;
		is_alpha = 1;
		dst_sub = &dst_full->Alpha;
		break;
	default:
		assert(0);
		return 0;
	}

	info = rc_get_opcode_info(dst_full->RGB.Opcode);

	if (dst_sub->Src[RC_PAIR_PRESUB_SRC].Used)
		return 0;

	srcp_regs = rc_presubtract_src_reg_count(
					src.Src[RC_PAIR_PRESUB_SRC].Index);
	for(srcp_src = 0; srcp_src < srcp_regs; srcp_src++) {
		unsigned int arg;
		int free_source;
		unsigned int one_way = 0;
		struct rc_pair_instruction_source srcp = src.Src[srcp_src];
		struct rc_pair_instruction_source temp;

		free_source = rc_pair_alloc_source(dst_full, is_rgb, is_alpha,
							srcp.File, srcp.Index);

		/* If free_source < 0 then there are no free source
		 * slots. */
		if (free_source < 0)
			return 0;

		temp = dst_sub->Src[srcp_src];
		dst_sub->Src[srcp_src] = dst_sub->Src[free_source];

		/* srcp needs src0 and src1 to be the same */
		if (free_source < srcp_src) {
			if (!temp.Used)
				continue;
			free_source = rc_pair_alloc_source(dst_full, is_rgb,
					is_alpha, temp.File, temp.Index);
			if (free_source < 0)
				return 0;
			one_way = 1;
		} else {
			dst_sub->Src[free_source] = temp;
		}

		/* If free_source == srcp_src, then the presubtract
		 * source is already in the correct place. */
		if (free_source == srcp_src)
			continue;

		/* Shuffle the sources, so we can put the
		 * presubtract source in the correct place. */
		for(arg = 0; arg < info->NumSrcRegs; arg++) {
			/*If this arg does not read from an rgb source,
			 * do nothing. */
			if (!(rc_source_type_swz(dst_full->RGB.Arg[arg].Swizzle)
								& type)) {
				continue;
			}

			if (dst_full->RGB.Arg[arg].Source == srcp_src)
				dst_full->RGB.Arg[arg].Source = free_source;
			/* We need to do this just in case register
			 * is one of the sources already, but in the
			 * wrong spot. */
			else if(dst_full->RGB.Arg[arg].Source == free_source
							&& !one_way) {
				dst_full->RGB.Arg[arg].Source = srcp_src;
			}
		}
	}
	return 1;
}


/* This function assumes that rgb.Alpha and alpha.RGB are unused */
static int destructive_merge_instructions(
		struct rc_pair_instruction * rgb,
		struct rc_pair_instruction * alpha)
{
	const struct rc_opcode_info * opcode;

	assert(rgb->Alpha.Opcode == RC_OPCODE_NOP);
	assert(alpha->RGB.Opcode == RC_OPCODE_NOP);

	/* Presubtract registers need to be merged first so that registers
	 * needed by the presubtract operation can be placed in src0 and/or
	 * src1. */

	/* Merge the rgb presubtract registers. */
	if (alpha->RGB.Src[RC_PAIR_PRESUB_SRC].Used) {
		if (!merge_presub_sources(rgb, alpha->RGB, RC_SOURCE_RGB)) {
			return 0;
		}
	}
	/* Merge the alpha presubtract registers */
	if (alpha->Alpha.Src[RC_PAIR_PRESUB_SRC].Used) {
		if(!merge_presub_sources(rgb,  alpha->Alpha, RC_SOURCE_ALPHA)){
			return 0;
		}
	}

	/* Copy alpha args into rgb */
	opcode = rc_get_opcode_info(alpha->Alpha.Opcode);

	for(unsigned int arg = 0; arg < opcode->NumSrcRegs; ++arg) {
		unsigned int srcrgb = 0;
		unsigned int srcalpha = 0;
		unsigned int oldsrc = alpha->Alpha.Arg[arg].Source;
		rc_register_file file = 0;
		unsigned int index = 0;
		int source;

		if (GET_SWZ(alpha->Alpha.Arg[arg].Swizzle, 0) < 3) {
			srcrgb = 1;
			file = alpha->RGB.Src[oldsrc].File;
			index = alpha->RGB.Src[oldsrc].Index;
		} else if (GET_SWZ(alpha->Alpha.Arg[arg].Swizzle, 0) < 4) {
			srcalpha = 1;
			file = alpha->Alpha.Src[oldsrc].File;
			index = alpha->Alpha.Src[oldsrc].Index;
		}

		source = rc_pair_alloc_source(rgb, srcrgb, srcalpha, file, index);
		if (source < 0)
			return 0;

		rgb->Alpha.Arg[arg].Source = source;
		rgb->Alpha.Arg[arg].Swizzle = alpha->Alpha.Arg[arg].Swizzle;
		rgb->Alpha.Arg[arg].Abs = alpha->Alpha.Arg[arg].Abs;
		rgb->Alpha.Arg[arg].Negate = alpha->Alpha.Arg[arg].Negate;
	}

	/* Copy alpha opcode into rgb */
	rgb->Alpha.Opcode = alpha->Alpha.Opcode;
	rgb->Alpha.DestIndex = alpha->Alpha.DestIndex;
	rgb->Alpha.WriteMask = alpha->Alpha.WriteMask;
	rgb->Alpha.OutputWriteMask = alpha->Alpha.OutputWriteMask;
	rgb->Alpha.DepthWriteMask = alpha->Alpha.DepthWriteMask;
	rgb->Alpha.Saturate = alpha->Alpha.Saturate;

	/* Merge ALU result writing */
	if (alpha->WriteALUResult) {
		if (rgb->WriteALUResult)
			return 0;

		rgb->WriteALUResult = alpha->WriteALUResult;
		rgb->ALUResultCompare = alpha->ALUResultCompare;
	}

	return 1;
}

/**
 * Try to merge the given instructions into the rgb instructions.
 *
 * Return true on success; on failure, return false, and keep
 * the instructions untouched.
 */
static int merge_instructions(struct rc_pair_instruction * rgb, struct rc_pair_instruction * alpha)
{
	struct rc_pair_instruction backup;

	/*Instructions can't write output registers and ALU result at the
	 * same time. */
	if ((rgb->WriteALUResult && alpha->Alpha.OutputWriteMask)
		|| (rgb->RGB.OutputWriteMask && alpha->WriteALUResult)) {
		return 0;
	}
	memcpy(&backup, rgb, sizeof(struct rc_pair_instruction));

	if (destructive_merge_instructions(rgb, alpha))
		return 1;

	memcpy(rgb, &backup, sizeof(struct rc_pair_instruction));
	return 0;
}

static void presub_nop(struct rc_instruction * emitted) {
	int prev_rgb_index, prev_alpha_index, i, num_src;

	/* We don't need a nop if the previous instruction is a TEX. */
	if (emitted->Prev->Type != RC_INSTRUCTION_PAIR) {
		return;
	}
	if (emitted->Prev->U.P.RGB.WriteMask)
		prev_rgb_index = emitted->Prev->U.P.RGB.DestIndex;
	else
		prev_rgb_index = -1;
	if (emitted->Prev->U.P.Alpha.WriteMask)
		prev_alpha_index = emitted->Prev->U.P.Alpha.DestIndex;
	else
		prev_alpha_index = 1;

	/* Check the previous rgb instruction */
	if (emitted->U.P.RGB.Src[RC_PAIR_PRESUB_SRC].Used) {
		num_src = rc_presubtract_src_reg_count(
				emitted->U.P.RGB.Src[RC_PAIR_PRESUB_SRC].Index);
		for (i = 0; i < num_src; i++) {
			unsigned int index = emitted->U.P.RGB.Src[i].Index;
			if (emitted->U.P.RGB.Src[i].File == RC_FILE_TEMPORARY
			    && (index  == prev_rgb_index
				|| index == prev_alpha_index)) {
				emitted->Prev->U.P.Nop = 1;
				return;
			}
		}
	}

	/* Check the previous alpha instruction. */
	if (!emitted->U.P.Alpha.Src[RC_PAIR_PRESUB_SRC].Used)
		return;

	num_src = rc_presubtract_src_reg_count(
				emitted->U.P.Alpha.Src[RC_PAIR_PRESUB_SRC].Index);
	for (i = 0; i < num_src; i++) {
		unsigned int index = emitted->U.P.Alpha.Src[i].Index;
		if(emitted->U.P.Alpha.Src[i].File == RC_FILE_TEMPORARY
		   && (index == prev_rgb_index || index == prev_alpha_index)) {
			emitted->Prev->U.P.Nop = 1;
			return;
		}
	}
}

static void rgb_to_alpha_remap (
	struct rc_instruction * inst,
	struct rc_pair_instruction_arg * arg,
	rc_register_file old_file,
	rc_swizzle old_swz,
	unsigned int new_index)
{
	int new_src_index;
	unsigned int i;

	for (i = 0; i < 3; i++) {
		if (get_swz(arg->Swizzle, i) == old_swz) {
			SET_SWZ(arg->Swizzle, i, RC_SWIZZLE_W);
		}
	}
	new_src_index = rc_pair_alloc_source(&inst->U.P, 0, 1,
							old_file, new_index);
	/* This conversion is not possible, we must have made a mistake in
	 * is_rgb_to_alpha_possible. */
	if (new_src_index < 0) {
		assert(0);
		return;
	}

	arg->Source = new_src_index;
}

static int can_remap(unsigned int opcode)
{
	switch(opcode) {
	case RC_OPCODE_DDX:
	case RC_OPCODE_DDY:
		return 0;
	default:
		return 1;
	}
}

static int can_convert_opcode_to_alpha(unsigned int opcode)
{
	switch(opcode) {
	case RC_OPCODE_DDX:
	case RC_OPCODE_DDY:
	case RC_OPCODE_DP2:
	case RC_OPCODE_DP3:
	case RC_OPCODE_DP4:
	case RC_OPCODE_DPH:
		return 0;
	default:
		return 1;
	}
}

static void is_rgb_to_alpha_possible(
	void * userdata,
	struct rc_instruction * inst,
	struct rc_pair_instruction_arg * arg,
	struct rc_pair_instruction_source * src)
{
	unsigned int chan_count = 0;
	unsigned int alpha_sources = 0;
	unsigned int i;
	struct rc_reader_data * reader_data = userdata;

	if (!can_remap(inst->U.P.RGB.Opcode)
	    || !can_remap(inst->U.P.Alpha.Opcode)) {
		reader_data->Abort = 1;
		return;
	}

	if (!src)
		return;

	/* XXX There are some cases where we can still do the conversion if
	 * a reader reads from a presubtract source, but for now we'll prevent
	 * it. */
	if (arg->Source == RC_PAIR_PRESUB_SRC) {
		reader_data->Abort = 1;
		return;
	}

	/* Make sure the source only reads from one component.
	 * XXX We should allow the source to read from the same component twice.
	 * XXX If the index we will be converting to is the same as the
	 * current index, then it is OK to read from more than one component.
	 */
	for (i = 0; i < 3; i++) {
		rc_swizzle swz = get_swz(arg->Swizzle, i);
		switch(swz) {
		case RC_SWIZZLE_X:
		case RC_SWIZZLE_Y:
		case RC_SWIZZLE_Z:
		case RC_SWIZZLE_W:
			chan_count++;
			break;
		default:
			break;
		}
	}
	if (chan_count > 1) {
		reader_data->Abort = 1;
		return;
	}

	/* Make sure there are enough alpha sources.
	 * XXX If we know what register all the readers are going
	 * to be remapped to, then in some situations we can still do
	 * the subsitution, even if all 3 alpha sources are being used.*/
	for (i = 0; i < 3; i++) {
		if (inst->U.P.Alpha.Src[i].Used) {
			alpha_sources++;
		}
	}
	if (alpha_sources > 2) {
		reader_data->Abort = 1;
		return;
	}
}

static int convert_rgb_to_alpha(
	struct schedule_state * s,
	struct schedule_instruction * sched_inst)
{
	struct rc_pair_instruction * pair_inst = &sched_inst->Instruction->U.P;
	unsigned int old_mask = pair_inst->RGB.WriteMask;
	unsigned int old_swz = rc_mask_to_swizzle(old_mask);
	const struct rc_opcode_info * info =
				rc_get_opcode_info(pair_inst->RGB.Opcode);
	int new_index = -1;
	unsigned int i;

	if (sched_inst->GlobalReaders.Abort)
		return 0;

	if (!pair_inst->RGB.WriteMask)
		return 0;

	if (!can_convert_opcode_to_alpha(pair_inst->RGB.Opcode)
	    || !can_convert_opcode_to_alpha(pair_inst->Alpha.Opcode)) {
		return 0;
	}

	assert(sched_inst->NumWriteValues == 1);

	if (!sched_inst->WriteValues[0]) {
		assert(0);
		return 0;
	}

	/* We start at the old index, because if we can reuse the same
	 * register and just change the swizzle then it is more likely we
	 * will be able to convert all the readers. */
	for (i = pair_inst->RGB.DestIndex; i < RC_REGISTER_MAX_INDEX; i++) {
		struct reg_value ** new_regvalp = get_reg_valuep(
						s, RC_FILE_TEMPORARY, i, 3);
		if (!*new_regvalp) {
			struct reg_value ** old_regvalp =
				get_reg_valuep(s,
					RC_FILE_TEMPORARY,
					pair_inst->RGB.DestIndex,
					rc_mask_to_swizzle(old_mask));
			new_index = i;
			*new_regvalp = *old_regvalp;
			*old_regvalp = NULL;
			new_regvalp = get_reg_valuep(s, RC_FILE_TEMPORARY, i, 3);
			break;
		}
	}
	if (new_index < 0) {
		return 0;
	}

	pair_inst->Alpha.Opcode = pair_inst->RGB.Opcode;
	pair_inst->Alpha.DestIndex = new_index;
	pair_inst->Alpha.WriteMask = RC_MASK_W;
	pair_inst->Alpha.Target = pair_inst->RGB.Target;
	pair_inst->Alpha.OutputWriteMask = pair_inst->RGB.OutputWriteMask;
	pair_inst->Alpha.DepthWriteMask = pair_inst->RGB.DepthWriteMask;
	pair_inst->Alpha.Saturate = pair_inst->RGB.Saturate;
	memcpy(pair_inst->Alpha.Arg, pair_inst->RGB.Arg,
						sizeof(pair_inst->Alpha.Arg));
	/* Move the swizzles into the first chan */
	for (i = 0; i < info->NumSrcRegs; i++) {
		unsigned int j;
		for (j = 0; j < 3; j++) {
			unsigned int swz = get_swz(pair_inst->Alpha.Arg[i].Swizzle, j);
			if (swz != RC_SWIZZLE_UNUSED) {
				pair_inst->Alpha.Arg[i].Swizzle =
							rc_init_swizzle(swz, 1);
				break;
			}
		}
	}
	pair_inst->RGB.Opcode = RC_OPCODE_NOP;
	pair_inst->RGB.DestIndex = 0;
	pair_inst->RGB.WriteMask = 0;
	pair_inst->RGB.Target = 0;
	pair_inst->RGB.OutputWriteMask = 0;
	pair_inst->RGB.DepthWriteMask = 0;
	pair_inst->RGB.Saturate = 0;
	memset(pair_inst->RGB.Arg, 0, sizeof(pair_inst->RGB.Arg));

	for(i = 0; i < sched_inst->GlobalReaders.ReaderCount; i++) {
		struct rc_reader reader = sched_inst->GlobalReaders.Readers[i];
		rgb_to_alpha_remap(reader.Inst, reader.U.P.Arg,
					RC_FILE_TEMPORARY, old_swz, new_index);
	}
	return 1;
}

/**
 * Find a good ALU instruction or pair of ALU instruction and emit it.
 *
 * Prefer emitting full ALU instructions, so that when we reach a point
 * where no full ALU instruction can be emitted, we have more candidates
 * for RGB/Alpha pairing.
 */
static void emit_one_alu(struct schedule_state *s, struct rc_instruction * before)
{
	struct schedule_instruction * sinst;

	if (s->ReadyFullALU) {
		sinst = s->ReadyFullALU;
		s->ReadyFullALU = s->ReadyFullALU->NextReady;
		rc_insert_instruction(before->Prev, sinst->Instruction);
		commit_alu_instruction(s, sinst);
	} else {
		struct schedule_instruction **prgb;
		struct schedule_instruction **palpha;
		struct schedule_instruction *prev;
pair:
		/* Some pairings might fail because they require too
		 * many source slots; try all possible pairings if necessary */
		for(prgb = &s->ReadyRGB; *prgb; prgb = &(*prgb)->NextReady) {
			for(palpha = &s->ReadyAlpha; *palpha; palpha = &(*palpha)->NextReady) {
				struct schedule_instruction * psirgb = *prgb;
				struct schedule_instruction * psialpha = *palpha;

				if (!merge_instructions(&psirgb->Instruction->U.P, &psialpha->Instruction->U.P))
					continue;

				*prgb = (*prgb)->NextReady;
				*palpha = (*palpha)->NextReady;
				rc_insert_instruction(before->Prev, psirgb->Instruction);
				commit_alu_instruction(s, psirgb);
				commit_alu_instruction(s, psialpha);
				goto success;
			}
		}
		prev = NULL;
		/* No success in pairing, now try to convert one of the RGB
		 * instructions to an Alpha so we can pair it with another RGB.
		 */
		if (s->ReadyRGB && s->ReadyRGB->NextReady) {
		for(prgb = &s->ReadyRGB; *prgb; prgb = &(*prgb)->NextReady) {
			if ((*prgb)->NumWriteValues == 1) {
				struct schedule_instruction * prgb_next;
				if (!convert_rgb_to_alpha(s, *prgb))
					goto cont_loop;
				prgb_next = (*prgb)->NextReady;
				/* Add instruction to the Alpha ready list. */
				(*prgb)->NextReady = s->ReadyAlpha;
				s->ReadyAlpha = *prgb;
				/* Remove instruction from the RGB ready list.*/
				if (prev)
					prev->NextReady = prgb_next;
				else
					s->ReadyRGB = prgb_next;
				goto pair;
			}
cont_loop:
			prev = *prgb;
		}
		}
		/* Still no success in pairing, just take the first RGB
		 * or alpha instruction. */
		if (s->ReadyRGB) {
			sinst = s->ReadyRGB;
			s->ReadyRGB = s->ReadyRGB->NextReady;
		} else if (s->ReadyAlpha) {
			sinst = s->ReadyAlpha;
			s->ReadyAlpha = s->ReadyAlpha->NextReady;
		} else {
			/*XXX Something real bad has happened. */
			assert(0);
		}

		rc_insert_instruction(before->Prev, sinst->Instruction);
		commit_alu_instruction(s, sinst);
	success: ;
	}
	/* If the instruction we just emitted uses a presubtract value, and
	 * the presubtract sources were written by the previous intstruction,
	 * the previous instruction needs a nop. */
	presub_nop(before->Prev);
}

static void scan_read(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int chan)
{
	struct schedule_state * s = data;
	struct reg_value ** v = get_reg_valuep(s, file, index, chan);
	struct reg_value_reader * reader;

	if (!v)
		return;

	if (*v && (*v)->Writer == s->Current) {
		/* The instruction reads and writes to a register component.
		 * In this case, we only want to increment dependencies by one.
		 */
		return;
	}

	DBG("%i: read %i[%i] chan %i\n", s->Current->Instruction->IP, file, index, chan);

	reader = memory_pool_malloc(&s->C->Pool, sizeof(*reader));
	reader->Reader = s->Current;
	if (!*v) {
		/* In this situation, the instruction reads from a register
		 * that hasn't been written to or read from in the current
		 * block. */
		*v = memory_pool_malloc(&s->C->Pool, sizeof(struct reg_value));
		memset(*v, 0, sizeof(struct reg_value));
		(*v)->Readers = reader;
	} else {
		reader->Next = (*v)->Readers;
		(*v)->Readers = reader;
		/* Only update the current instruction's dependencies if the
		 * register it reads from has been written to in this block. */
		if ((*v)->Writer) {
			s->Current->NumDependencies++;
		}
	}
	(*v)->NumReaders++;

	if (s->Current->NumReadValues >= 12) {
		rc_error(s->C, "%s: NumReadValues overflow\n", __FUNCTION__);
	} else {
		s->Current->ReadValues[s->Current->NumReadValues++] = *v;
	}
}

static void scan_write(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int chan)
{
	struct schedule_state * s = data;
	struct reg_value ** pv = get_reg_valuep(s, file, index, chan);
	struct reg_value * newv;

	if (!pv)
		return;

	DBG("%i: write %i[%i] chan %i\n", s->Current->Instruction->IP, file, index, chan);

	newv = memory_pool_malloc(&s->C->Pool, sizeof(*newv));
	memset(newv, 0, sizeof(*newv));

	newv->Writer = s->Current;

	if (*pv) {
		(*pv)->Next = newv;
		s->Current->NumDependencies++;
	}

	*pv = newv;

	if (s->Current->NumWriteValues >= 4) {
		rc_error(s->C, "%s: NumWriteValues overflow\n", __FUNCTION__);
	} else {
		s->Current->WriteValues[s->Current->NumWriteValues++] = newv;
	}
}

static void is_rgb_to_alpha_possible_normal(
	void * userdata,
	struct rc_instruction * inst,
	struct rc_src_register * src)
{
	struct rc_reader_data * reader_data = userdata;
	reader_data->Abort = 1;

}

static void schedule_block(struct r300_fragment_program_compiler * c,
		struct rc_instruction * begin, struct rc_instruction * end)
{
	struct schedule_state s;
	unsigned int ip;

	memset(&s, 0, sizeof(s));
	s.C = &c->Base;

	/* Scan instructions for data dependencies */
	ip = 0;
	for(struct rc_instruction * inst = begin; inst != end; inst = inst->Next) {
		s.Current = memory_pool_malloc(&c->Base.Pool, sizeof(*s.Current));
		memset(s.Current, 0, sizeof(struct schedule_instruction));

		s.Current->Instruction = inst;
		inst->IP = ip++;

		DBG("%i: Scanning\n", inst->IP);

		/* The order of things here is subtle and maybe slightly
		 * counter-intuitive, to account for the case where an
		 * instruction writes to the same register as it reads
		 * from. */
		rc_for_all_writes_chan(inst, &scan_write, &s);
		rc_for_all_reads_chan(inst, &scan_read, &s);

		DBG("%i: Has %i dependencies\n", inst->IP, s.Current->NumDependencies);

		if (!s.Current->NumDependencies)
			instruction_ready(&s, s.Current);

		/* Get global readers for possible RGB->Alpha conversion. */
		s.Current->GlobalReaders.ExitOnAbort = 1;
		rc_get_readers(s.C, inst, &s.Current->GlobalReaders,
				is_rgb_to_alpha_possible_normal,
				is_rgb_to_alpha_possible, NULL);
	}

	/* Temporarily unlink all instructions */
	begin->Prev->Next = end;
	end->Prev = begin->Prev;

	/* Schedule instructions back */
	while(!s.C->Error &&
	      (s.ReadyTEX || s.ReadyRGB || s.ReadyAlpha || s.ReadyFullALU)) {
		if (s.ReadyTEX)
			emit_all_tex(&s, end);

		while(!s.C->Error && (s.ReadyFullALU || s.ReadyRGB || s.ReadyAlpha))
			emit_one_alu(&s, end);
	}
}

static int is_controlflow(struct rc_instruction * inst)
{
	if (inst->Type == RC_INSTRUCTION_NORMAL) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->U.I.Opcode);
		return opcode->IsFlowControl;
	}
	return 0;
}

void rc_pair_schedule(struct radeon_compiler *cc, void *user)
{
	struct schedule_state s;

	struct r300_fragment_program_compiler *c = (struct r300_fragment_program_compiler*)cc;
	struct rc_instruction * inst = c->Base.Program.Instructions.Next;

	memset(&s, 0, sizeof(s));
	s.C = &c->Base;
	while(inst != &c->Base.Program.Instructions) {
		struct rc_instruction * first;

		if (is_controlflow(inst)) {
			inst = inst->Next;
			continue;
		}

		first = inst;

		while(inst != &c->Base.Program.Instructions && !is_controlflow(inst))
			inst = inst->Next;

		DBG("Schedule one block\n");
		schedule_block(c, first, inst);
	}
}
