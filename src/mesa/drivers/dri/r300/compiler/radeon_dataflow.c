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


static void reads_normal(struct rc_instruction * fullinst, rc_read_write_chan_fn cb, void * userdata)
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

		if (refmask)
			cb(userdata, fullinst, inst->SrcReg[src].File, inst->SrcReg[src].Index, refmask);

		if (refmask && inst->SrcReg[src].RelAddr)
			cb(userdata, fullinst, RC_FILE_ADDRESS, 0, RC_MASK_X);
	}
}

static void reads_pair(struct rc_instruction * fullinst,  rc_read_write_mask_fn cb, void * userdata)
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
		if (inst->RGB.Src[src].Used && (refmasks[src] & RC_MASK_XYZ))
			cb(userdata, fullinst, inst->RGB.Src[src].File, inst->RGB.Src[src].Index,
			   refmasks[src] & RC_MASK_XYZ);

		if (inst->Alpha.Src[src].Used && (refmasks[src] & RC_MASK_W))
			cb(userdata, fullinst, inst->Alpha.Src[src].File, inst->Alpha.Src[src].Index, RC_MASK_W);
	}
}

/**
 * Calls a callback function for all register reads.
 *
 * This is conservative, i.e. if the same register is referenced multiple times,
 * the callback may also be called multiple times.
 * Also, the writemask of the instruction is not taken into account.
 */
void rc_for_all_reads_mask(struct rc_instruction * inst, rc_read_write_mask_fn cb, void * userdata)
{
	if (inst->Type == RC_INSTRUCTION_NORMAL) {
		reads_normal(inst, cb, userdata);
	} else {
		reads_pair(inst, cb, userdata);
	}
}



static void writes_normal(struct rc_instruction * fullinst, rc_read_write_mask_fn cb, void * userdata)
{
	struct rc_sub_instruction * inst = &fullinst->U.I;
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->Opcode);

	if (opcode->HasDstReg && inst->DstReg.WriteMask)
		cb(userdata, fullinst, inst->DstReg.File, inst->DstReg.Index, inst->DstReg.WriteMask);

	if (inst->WriteALUResult)
		cb(userdata, fullinst, RC_FILE_SPECIAL, RC_SPECIAL_ALU_RESULT, RC_MASK_X);
}

static void writes_pair(struct rc_instruction * fullinst, rc_read_write_mask_fn cb, void * userdata)
{
	struct rc_pair_instruction * inst = &fullinst->U.P;

	if (inst->RGB.WriteMask)
		cb(userdata, fullinst, RC_FILE_TEMPORARY, inst->RGB.DestIndex, inst->RGB.WriteMask);

	if (inst->Alpha.WriteMask)
		cb(userdata, fullinst, RC_FILE_TEMPORARY, inst->Alpha.DestIndex, RC_MASK_W);

	if (inst->WriteALUResult)
		cb(userdata, fullinst, RC_FILE_SPECIAL, RC_SPECIAL_ALU_RESULT, RC_MASK_X);
}

/**
 * Calls a callback function for all register writes in the instruction,
 * reporting writemasks to the callback function.
 *
 * \warning Does not report output registers for paired instructions!
 */
void rc_for_all_writes_mask(struct rc_instruction * inst, rc_read_write_mask_fn cb, void * userdata)
{
	if (inst->Type == RC_INSTRUCTION_NORMAL) {
		writes_normal(inst, cb, userdata);
	} else {
		writes_pair(inst, cb, userdata);
	}
}


struct mask_to_chan_data {
	void * UserData;
	rc_read_write_chan_fn Fn;
};

static void mask_to_chan_cb(void * data, struct rc_instruction * inst,
		rc_register_file file, unsigned int index, unsigned int mask)
{
	struct mask_to_chan_data * d = data;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		if (GET_BIT(mask, chan))
			d->Fn(d->UserData, inst, file, index, chan);
	}
}

/**
 * Calls a callback function for all sourced register channels.
 *
 * This is conservative, i.e. channels may be called multiple times,
 * and the writemask of the instruction is not taken into account.
 */
void rc_for_all_reads_chan(struct rc_instruction * inst, rc_read_write_chan_fn cb, void * userdata)
{
	struct mask_to_chan_data d;
	d.UserData = userdata;
	d.Fn = cb;
	rc_for_all_reads_mask(inst, &mask_to_chan_cb, &d);
}

/**
 * Calls a callback function for all written register channels.
 *
 * \warning Does not report output registers for paired instructions!
 */
void rc_for_all_writes_chan(struct rc_instruction * inst, rc_read_write_chan_fn cb, void * userdata)
{
	struct mask_to_chan_data d;
	d.UserData = userdata;
	d.Fn = cb;
	rc_for_all_writes_mask(inst, &mask_to_chan_cb, &d);
}

static void remap_normal_instruction(struct rc_instruction * fullinst,
		rc_remap_register_fn cb, void * userdata)
{
	struct rc_sub_instruction * inst = &fullinst->U.I;
	const struct rc_opcode_info * opcode = rc_get_opcode_info(inst->Opcode);

	if (opcode->HasDstReg) {
		rc_register_file file = inst->DstReg.File;
		unsigned int index = inst->DstReg.Index;

		cb(userdata, fullinst, &file, &index);

		inst->DstReg.File = file;
		inst->DstReg.Index = index;
	}

	for(unsigned int src = 0; src < opcode->NumSrcRegs; ++src) {
		rc_register_file file = inst->SrcReg[src].File;
		unsigned int index = inst->SrcReg[src].Index;

		cb(userdata, fullinst, &file, &index);

		inst->SrcReg[src].File = file;
		inst->SrcReg[src].Index = index;
	}
}

static void remap_pair_instruction(struct rc_instruction * fullinst,
		rc_remap_register_fn cb, void * userdata)
{
	struct rc_pair_instruction * inst = &fullinst->U.P;

	if (inst->RGB.WriteMask) {
		rc_register_file file = RC_FILE_TEMPORARY;
		unsigned int index = inst->RGB.DestIndex;

		cb(userdata, fullinst, &file, &index);

		inst->RGB.DestIndex = index;
	}

	if (inst->Alpha.WriteMask) {
		rc_register_file file = RC_FILE_TEMPORARY;
		unsigned int index = inst->Alpha.DestIndex;

		cb(userdata, fullinst, &file, &index);

		inst->Alpha.DestIndex = index;
	}

	for(unsigned int src = 0; src < 3; ++src) {
		if (inst->RGB.Src[src].Used) {
			rc_register_file file = inst->RGB.Src[src].File;
			unsigned int index = inst->RGB.Src[src].Index;

			cb(userdata, fullinst, &file, &index);

			inst->RGB.Src[src].File = file;
			inst->RGB.Src[src].Index = index;
		}

		if (inst->Alpha.Src[src].Used) {
			rc_register_file file = inst->Alpha.Src[src].File;
			unsigned int index = inst->Alpha.Src[src].Index;

			cb(userdata, fullinst, &file, &index);

			inst->Alpha.Src[src].File = file;
			inst->Alpha.Src[src].Index = index;
		}
	}
}


/**
 * Remap all register accesses according to the given function.
 * That is, call the function \p cb for each referenced register (both read and written)
 * and update the given instruction \p inst accordingly
 * if it modifies its \ref pfile and \ref pindex contents.
 */
void rc_remap_registers(struct rc_instruction * inst, rc_remap_register_fn cb, void * userdata)
{
	if (inst->Type == RC_INSTRUCTION_NORMAL)
		remap_normal_instruction(inst, cb, userdata);
	else
		remap_pair_instruction(inst, cb, userdata);
}
