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

static struct reg_value * get_reg_value(struct schedule_state * s,
		rc_register_file file, unsigned int index, unsigned int chan)
{
	struct reg_value ** pv = get_reg_valuep(s, file, index, chan);
	if (!pv)
		return 0;
	return *pv;
}

static void add_inst_to_list(struct schedule_instruction ** list, struct schedule_instruction * inst)
{
	inst->NextReady = *list;
	*list = inst;
}

static void instruction_ready(struct schedule_state * s, struct schedule_instruction * sinst)
{
	DBG("%i is now ready\n", sinst->Instruction->IP);

	if (sinst->Instruction->Type == RC_INSTRUCTION_NORMAL)
		add_inst_to_list(&s->ReadyTEX, sinst);
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

static void commit_instruction(struct schedule_state * s, struct schedule_instruction * sinst)
{
	DBG("%i: commit\n", sinst->Instruction->IP);

	for(unsigned int i = 0; i < sinst->NumReadValues; ++i) {
		struct reg_value * v = sinst->ReadValues[i];
		assert(v->NumReaders > 0);
		v->NumReaders--;
		if (!v->NumReaders) {
			if (v->Next)
				decrease_dependencies(s, v->Next->Writer);
		}
	}

	for(unsigned int i = 0; i < sinst->NumWriteValues; ++i) {
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

/**
 * Emit all ready texture instructions in a single block.
 *
 * Emit as a single block to (hopefully) sample many textures in parallel,
 * and to avoid hardware indirections on R300.
 */
static void emit_all_tex(struct schedule_state * s, struct rc_instruction * before)
{
	struct schedule_instruction *readytex;

	assert(s->ReadyTEX);

	/* Don't let the ready list change under us! */
	readytex = s->ReadyTEX;
	s->ReadyTEX = 0;

	/* Node marker for R300 */
	struct rc_instruction * inst_begin = rc_insert_new_instruction(s->C, before->Prev);
	inst_begin->U.I.Opcode = RC_OPCODE_BEGIN_TEX;

	/* Link texture instructions back in */
	while(readytex) {
		struct schedule_instruction * tex = readytex;
		readytex = readytex->NextReady;

		rc_insert_instruction(before->Prev, tex->Instruction);
		commit_instruction(s, tex);
	}
}


static int destructive_merge_instructions(
		struct rc_pair_instruction * rgb,
		struct rc_pair_instruction * alpha)
{
	assert(rgb->Alpha.Opcode == RC_OPCODE_NOP);
	assert(alpha->RGB.Opcode == RC_OPCODE_NOP);

	/* Copy alpha args into rgb */
	const struct rc_opcode_info * opcode = rc_get_opcode_info(alpha->Alpha.Opcode);

	for(unsigned int arg = 0; arg < opcode->NumSrcRegs; ++arg) {
		unsigned int srcrgb = 0;
		unsigned int srcalpha = 0;
		unsigned int oldsrc = alpha->Alpha.Arg[arg].Source;
		rc_register_file file = 0;
		unsigned int index = 0;

		if (alpha->Alpha.Arg[arg].Swizzle < 3) {
			srcrgb = 1;
			file = alpha->RGB.Src[oldsrc].File;
			index = alpha->RGB.Src[oldsrc].Index;
		} else if (alpha->Alpha.Arg[arg].Swizzle < 4) {
			srcalpha = 1;
			file = alpha->Alpha.Src[oldsrc].File;
			index = alpha->Alpha.Src[oldsrc].Index;
		}

		int source = rc_pair_alloc_source(rgb, srcrgb, srcalpha, file, index);
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

	memcpy(&backup, rgb, sizeof(struct rc_pair_instruction));

	if (destructive_merge_instructions(rgb, alpha))
		return 1;

	memcpy(rgb, &backup, sizeof(struct rc_pair_instruction));
	return 0;
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

	if (s->ReadyFullALU || !(s->ReadyRGB && s->ReadyAlpha)) {
		if (s->ReadyFullALU) {
			sinst = s->ReadyFullALU;
			s->ReadyFullALU = s->ReadyFullALU->NextReady;
		} else if (s->ReadyRGB) {
			sinst = s->ReadyRGB;
			s->ReadyRGB = s->ReadyRGB->NextReady;
		} else {
			sinst = s->ReadyAlpha;
			s->ReadyAlpha = s->ReadyAlpha->NextReady;
		}

		rc_insert_instruction(before->Prev, sinst->Instruction);
		commit_instruction(s, sinst);
	} else {
		struct schedule_instruction **prgb;
		struct schedule_instruction **palpha;

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
				commit_instruction(s, psirgb);
				commit_instruction(s, psialpha);
				goto success;
			}
		}

		/* No success in pairing; just take the first RGB instruction */
		sinst = s->ReadyRGB;
		s->ReadyRGB = s->ReadyRGB->NextReady;

		rc_insert_instruction(before->Prev, sinst->Instruction);
		commit_instruction(s, sinst);
	success: ;
	}
}

static void scan_read(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int chan)
{
	struct schedule_state * s = data;
	struct reg_value * v = get_reg_value(s, file, index, chan);

	if (!v)
		return;

	if (v->Writer == s->Current) {
		/* The instruction reads and writes to a register component.
		 * In this case, we only want to increment dependencies by one.
		 */
		return;
	}

	DBG("%i: read %i[%i] chan %i\n", s->Current->Instruction->IP, file, index, chan);

	struct reg_value_reader * reader = memory_pool_malloc(&s->C->Pool, sizeof(*reader));
	reader->Reader = s->Current;
	reader->Next = v->Readers;
	v->Readers = reader;
	v->NumReaders++;

	s->Current->NumDependencies++;

	if (s->Current->NumReadValues >= 12) {
		rc_error(s->C, "%s: NumReadValues overflow\n", __FUNCTION__);
	} else {
		s->Current->ReadValues[s->Current->NumReadValues++] = v;
	}
}

static void scan_write(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int chan)
{
	struct schedule_state * s = data;
	struct reg_value ** pv = get_reg_valuep(s, file, index, chan);

	if (!pv)
		return;

	DBG("%i: write %i[%i] chan %i\n", s->Current->Instruction->IP, file, index, chan);

	struct reg_value * newv = memory_pool_malloc(&s->C->Pool, sizeof(*newv));
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

static void schedule_block(struct r300_fragment_program_compiler * c,
		struct rc_instruction * begin, struct rc_instruction * end)
{
	struct schedule_state s;

	memset(&s, 0, sizeof(s));
	s.C = &c->Base;

	/* Scan instructions for data dependencies */
	unsigned int ip = 0;
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
		rc_for_all_writes(inst, &scan_write, &s);
		rc_for_all_reads(inst, &scan_read, &s);

		DBG("%i: Has %i dependencies\n", inst->IP, s.Current->NumDependencies);

		if (!s.Current->NumDependencies)
			instruction_ready(&s, s.Current);
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

void rc_pair_schedule(struct r300_fragment_program_compiler *c)
{
	struct rc_instruction * inst = c->Base.Program.Instructions.Next;
	while(inst != &c->Base.Program.Instructions) {
		if (is_controlflow(inst)) {
			inst = inst->Next;
			continue;
		}

		struct rc_instruction * first = inst;

		while(inst != &c->Base.Program.Instructions && !is_controlflow(inst))
			inst = inst->Next;

		DBG("Schedule one block\n");
		schedule_block(c, first, inst);
	}
}
