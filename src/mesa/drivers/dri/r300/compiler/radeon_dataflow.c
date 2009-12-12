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

#include "radeon_program.h"


static void reads_normal(struct rc_instruction * fullinst, rc_read_write_fn cb, void * userdata)
{
	struct rc_sub_instruction * inst = &fullinst->U.I;
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->Opcode);

	for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
		unsigned int refmask = 0;

		if (inst->SrcReg[src].File == RC_FILE_NONE)
			return;

		for(unsigned int chan = 0; chan < 4; ++chan)
			refmask |= 1 << GET_SWZ(inst->SrcReg[src].Swizzle, chan);

		refmask &= RC_MASK_XYZW;

		for(unsigned int chan = 0; chan < 4; ++chan) {
			if (GET_BIT(refmask, chan)) {
				cb(userdata, fullinst, inst->SrcReg[src].File, inst->SrcReg[src].Index, chan);
			}
		}

		if (refmask && inst->SrcReg[src].RelAddr)
			cb(userdata, fullinst, RC_FILE_ADDRESS, 0, RC_MASK_X);
	}
}

static void reads_pair(struct rc_instruction * fullinst,  rc_read_write_fn cb, void * userdata)
{
	struct rc_pair_instruction * inst = &fullinst->U.P;
	unsigned int refmasks[3] = { 0, 0, 0 };

	if (inst->RGB.Opcode != RC_OPCODE_NOP) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->RGB.Opcode);

		for(unsigned int arg = 0; arg < opcode->NumSrcRegs; ++arg) {
			for(unsigned int chan = 0; chan < 3; ++chan) {
				unsigned int swz = GET_SWZ(inst->RGB.Arg[arg].Swizzle, chan);
				if (swz < 4)
					refmasks[inst->RGB.Arg[arg].Source] |= 1 << swz;
			}
		}
	}

	if (inst->Alpha.Opcode != RC_OPCODE_NOP) {
		const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->Alpha.Opcode);

		for(unsigned int arg = 0; arg < opcode->NumSrcRegs; ++arg) {
			if (inst->Alpha.Arg[arg].Swizzle < 4)
				refmasks[inst->Alpha.Arg[arg].Source] |= 1 << inst->Alpha.Arg[arg].Swizzle;
		}
	}

	for(unsigned int src = 0; src < 3; ++src) {
		if (inst->RGB.Src[src].Used) {
			for(unsigned int chan = 0; chan < 3; ++chan) {
				if (GET_BIT(refmasks[src], chan))
					cb(userdata, fullinst, inst->RGB.Src[src].File, inst->RGB.Src[src].Index, chan);
			}
		}

		if (inst->Alpha.Src[src].Used) {
			if (GET_BIT(refmasks[src], 3))
				cb(userdata, fullinst, inst->Alpha.Src[src].File, inst->Alpha.Src[src].Index, 3);
		}
	}
}

/**
 * Calls a callback function for all sourced register channels.
 *
 * This is conservative, i.e. channels may be called multiple times,
 * and the writemask of the instruction is not taken into account.
 */
void rc_for_all_reads(struct rc_instruction * inst, rc_read_write_fn cb, void * userdata)
{
	if (inst->Type == RC_INSTRUCTION_NORMAL) {
		reads_normal(inst, cb, userdata);
	} else {
		reads_pair(inst, cb, userdata);
	}
}



static void writes_normal(struct rc_instruction * fullinst, rc_read_write_fn cb, void * userdata)
{
	struct rc_sub_instruction * inst = &fullinst->U.I;
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->Opcode);

	if (opcode->HasDstReg) {
		for(unsigned int chan = 0; chan < 4; ++chan) {
			if (GET_BIT(inst->DstReg.WriteMask, chan))
				cb(userdata, fullinst, inst->DstReg.File, inst->DstReg.Index, chan);
		}
	}

	if (inst->WriteALUResult)
		cb(userdata, fullinst, RC_FILE_SPECIAL, RC_SPECIAL_ALU_RESULT, 0);
}

static void writes_pair(struct rc_instruction * fullinst, rc_read_write_fn cb, void * userdata)
{
	struct rc_pair_instruction * inst = &fullinst->U.P;

	for(unsigned int chan = 0; chan < 3; ++chan) {
		if (GET_BIT(inst->RGB.WriteMask, chan))
			cb(userdata, fullinst, RC_FILE_TEMPORARY, inst->RGB.DestIndex, chan);
	}

	if (inst->Alpha.WriteMask)
		cb(userdata, fullinst, RC_FILE_TEMPORARY, inst->Alpha.DestIndex, 3);

	if (inst->WriteALUResult)
		cb(userdata, fullinst, RC_FILE_SPECIAL, RC_SPECIAL_ALU_RESULT, 0);
}

/**
 * Calls a callback function for all written register channels.
 *
 * \warning Does not report output registers for paired instructions!
 */
void rc_for_all_writes(struct rc_instruction * inst, rc_read_write_fn cb, void * userdata)
{
	if (inst->Type == RC_INSTRUCTION_NORMAL) {
		writes_normal(inst, cb, userdata);
	} else {
		writes_pair(inst, cb, userdata);
	}
}
