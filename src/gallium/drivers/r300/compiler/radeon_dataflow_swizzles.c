/*
 * Copyright (C) 2009 Nicolai Haehnle.
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 * Authors:
 * Nicolai Haehnle
 * Tom Stellard <thomas.stellard@amd.com>
 */

#include "radeon_dataflow.h"

#include "radeon_code.h"
#include "radeon_compiler.h"
#include "radeon_compiler_util.h"
#include "radeon_swizzle.h"


static void rewrite_source(struct radeon_compiler * c,
		struct rc_instruction * inst, unsigned src)
{
	struct rc_swizzle_split split;
	unsigned int tempreg = rc_find_free_temporary(c);
	unsigned int usemask;

	usemask = 0;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		if (GET_SWZ(inst->U.I.SrcReg[src].Swizzle, chan) != RC_SWIZZLE_UNUSED)
			usemask |= 1 << chan;
	}

	c->SwizzleCaps->Split(inst->U.I.SrcReg[src], usemask, &split);

	for(unsigned int phase = 0; phase < split.NumPhases; ++phase) {
		struct rc_instruction * mov = rc_insert_new_instruction(c, inst->Prev);
		unsigned int phase_refmask;
		unsigned int masked_negate;

		mov->U.I.Opcode = RC_OPCODE_MOV;
		mov->U.I.DstReg.File = RC_FILE_TEMPORARY;
		mov->U.I.DstReg.Index = tempreg;
		mov->U.I.DstReg.WriteMask = split.Phase[phase];
		mov->U.I.SrcReg[0] = inst->U.I.SrcReg[src];
		mov->U.I.PreSub = inst->U.I.PreSub;

		phase_refmask = 0;
		for(unsigned int chan = 0; chan < 4; ++chan) {
			if (!GET_BIT(split.Phase[phase], chan))
				SET_SWZ(mov->U.I.SrcReg[0].Swizzle, chan, RC_SWIZZLE_UNUSED);
			else
				phase_refmask |= 1 << GET_SWZ(mov->U.I.SrcReg[0].Swizzle, chan);
		}

		phase_refmask &= RC_MASK_XYZW;

		masked_negate = split.Phase[phase] & mov->U.I.SrcReg[0].Negate;
		if (masked_negate == 0)
			mov->U.I.SrcReg[0].Negate = 0;
		else if (masked_negate == split.Phase[phase])
			mov->U.I.SrcReg[0].Negate = RC_MASK_XYZW;

	}

	inst->U.I.SrcReg[src].File = RC_FILE_TEMPORARY;
	inst->U.I.SrcReg[src].Index = tempreg;
	inst->U.I.SrcReg[src].Swizzle = 0;
	inst->U.I.SrcReg[src].Negate = RC_MASK_NONE;
	inst->U.I.SrcReg[src].Abs = 0;
	for(unsigned int chan = 0; chan < 4; ++chan) {
		SET_SWZ(inst->U.I.SrcReg[src].Swizzle, chan,
				GET_BIT(usemask, chan) ? chan : RC_SWIZZLE_UNUSED);
	}
}

/**
 * This function will attempt to rewrite non-native swizzles that read from
 * immediate registers by rearranging the immediates to allow the
 * instruction to use native swizzles.
 */
static unsigned try_rewrite_constant(struct radeon_compiler *c,
					struct rc_src_register *reg)
{
	unsigned new_swizzle, chan, swz0, swz1, swz2, swz3, found_swizzle, swz;
	unsigned all_inline = 0;
	float imms[4] = {0.0f, 0.0f, 0.0f, 0.0f};

	if (!rc_src_reg_is_immediate(c, reg->File, reg->Index)) {
		/* The register does not contain immediates, but if all
		 * the swizzles are inline constants, we can still rewrite
		 * it. */

		new_swizzle = RC_SWIZZLE_XYZW;
		for (chan = 0 ; chan < 4; chan++) {
			unsigned swz = GET_SWZ(reg->Swizzle, chan);
			if (swz <= RC_SWIZZLE_W) {
				return 0;
			}
			if (swz == RC_SWIZZLE_UNUSED) {
				SET_SWZ(new_swizzle, chan, RC_SWIZZLE_UNUSED);
			}
		}
		all_inline = 1;
	} else {
		new_swizzle = reg->Swizzle;
	}

	swz = RC_SWIZZLE_UNUSED;
	found_swizzle = 1;
	/* Check if all channels have the same swizzle.  If they do we can skip
	 * the search for a native swizzle.  We only need to check the first
	 * three channels, because any swizzle is legal in the fourth channel.
	 */
	for (chan = 0; chan < 3; chan++) {
		unsigned chan_swz = GET_SWZ(reg->Swizzle, chan);
		if (chan_swz == RC_SWIZZLE_UNUSED) {
			continue;
		}
		if (swz == RC_SWIZZLE_UNUSED) {
			swz = chan_swz;
		} else if (swz != chan_swz) {
			found_swizzle = 0;
			break;
		}
	}

	/* Find a legal swizzle */

	/* This loop attempts to find a native swizzle where all the
	 * channels are different. */
	while (!found_swizzle && !all_inline) {
		swz0 = GET_SWZ(new_swizzle, 0);
		swz1 = GET_SWZ(new_swizzle, 1);
		swz2 = GET_SWZ(new_swizzle, 2);

		/* Swizzle .W. is never legal. */
		if (swz1 == RC_SWIZZLE_W ||
			swz1 == RC_SWIZZLE_UNUSED ||
			swz1 == RC_SWIZZLE_ZERO ||
			swz1 == RC_SWIZZLE_HALF ||
			swz1 == RC_SWIZZLE_ONE) {
			/* We chose Z, because there are two non-repeating
			 * swizzle combinations of the form .Z. There are
			 * only one combination each for .X. and .Y. */
			SET_SWZ(new_swizzle, 1, RC_SWIZZLE_Z);
			continue;
		}

		if (swz2 == RC_SWIZZLE_UNUSED) {
			/* We choose Y, because there are two non-repeating
			 * swizzle combinations of the form ..Y */
			SET_SWZ(new_swizzle, 2, RC_SWIZZLE_Y);
			continue;
		}

		switch (swz0) {
		/* X.. */
		case RC_SWIZZLE_X:
			/* Legal swizzles that start with X: XYZ, XXX */
			switch (swz1) {
			/* XX. */
			case RC_SWIZZLE_X:
				/*  The new swizzle will be:
				 *  ZXY (XX. => ZX. => ZXY) */
				SET_SWZ(new_swizzle, 0, RC_SWIZZLE_Z);
				break;
			/* XY. */
			case RC_SWIZZLE_Y:
				/* The new swizzle is XYZ */
				SET_SWZ(new_swizzle, 2, RC_SWIZZLE_Z);
				found_swizzle = 1;
				break;
			/* XZ. */
			case RC_SWIZZLE_Z:
				/* XZZ */
				if (swz2 == RC_SWIZZLE_Z) {
					/* The new swizzle is XYZ */
					SET_SWZ(new_swizzle, 1, RC_SWIZZLE_Y);
					found_swizzle = 1;
				} else { /* XZ[^Z] */
					/* The new swizzle will be:
					 * YZX (XZ. => YZ. => YZX) */
					SET_SWZ(new_swizzle, 0, RC_SWIZZLE_Y);
				}
				break;
			/* XW. Should have already been handled. */
			case RC_SWIZZLE_W:
				assert(0);
				break;
			}
			break;
		/* Y.. */
		case RC_SWIZZLE_Y:
			/* Legal swizzles that start with Y: YYY, YZX */
			switch (swz1) {
			/* YY. */
			case RC_SWIZZLE_Y:
				/* The new swizzle will be:
				 * XYZ (YY. => XY. => XYZ) */
				SET_SWZ(new_swizzle, 0, RC_SWIZZLE_X);
				break;
			/* YZ. */
			case RC_SWIZZLE_Z:
				/* The new swizzle is YZX */
				SET_SWZ(new_swizzle, 2, RC_SWIZZLE_X);
				found_swizzle = 1;
				break;
			/* YX. */
			case RC_SWIZZLE_X:
				/* YXX */
				if (swz2 == RC_SWIZZLE_X) {
					/*The new swizzle is YZX */
					SET_SWZ(new_swizzle, 1, RC_SWIZZLE_Z);
					found_swizzle = 1;
				} else { /* YX[^X] */
					/* The new swizzle will be:
					 * ZXY (YX. => ZX. -> ZXY) */
					SET_SWZ(new_swizzle, 0, RC_SWIZZLE_Z);
				}
				break;
			/* YW. Should have already been handled. */
			case RC_SWIZZLE_W:
				assert(0);
				break;
			}
			break;
		/* Z.. */
		case RC_SWIZZLE_Z:
			/* Legal swizzles that start with Z: ZZZ, ZXY */
			switch (swz1) {
			/* ZZ. */
			case RC_SWIZZLE_Z:
				/* The new swizzle will be:
				 * WZY (ZZ. => WZ. => WZY) */
				SET_SWZ(new_swizzle, 0, RC_SWIZZLE_W);
				break;
			/* ZX. */
			case RC_SWIZZLE_X:
				/* The new swizzle is ZXY */
				SET_SWZ(new_swizzle, 2, RC_SWIZZLE_Y);
				found_swizzle = 1;
				break;
			/* ZY. */
			case RC_SWIZZLE_Y:
				/* ZYY */
				if (swz2 == RC_SWIZZLE_Y) {
					/* The new swizzle is ZXY */
					SET_SWZ(new_swizzle, 1, RC_SWIZZLE_X);
					found_swizzle = 1;
				} else { /* ZY[^Y] */
					/* The new swizzle will be:
					 * XYZ (ZY. => XY. => XYZ) */
					SET_SWZ(new_swizzle, 0, RC_SWIZZLE_X);
				}
				break;
			/* ZW. Should have already been handled. */
			case RC_SWIZZLE_W:
				assert(0);
				break;
			}
			break;

		/* W.. */
		case RC_SWIZZLE_W:
			/* Legal swizzles that start with X: WWW, WZY */
			switch (swz1) {
			/* WW. Should have already been handled. */
			case RC_SWIZZLE_W:
				assert(0);
				break;
			/* WZ. */
			case RC_SWIZZLE_Z:
				/* The new swizzle will be WZY */
				SET_SWZ(new_swizzle, 2, RC_SWIZZLE_Y);
				found_swizzle = 1;
				break;
			/* WX. */
			case RC_SWIZZLE_X:
			/* WY. */
			case RC_SWIZZLE_Y:
				/* W[XY]Y */
				if (swz2 == RC_SWIZZLE_Y) {
					/* The new swizzle will be WZY */
					SET_SWZ(new_swizzle, 1, RC_SWIZZLE_Z);
					found_swizzle = 1;
				} else { /* W[XY][^Y] */
					/* The new swizzle will be:
					 * ZXY (WX. => XX. => ZX. => ZXY) or
					 * XYZ (WY. => XY. => XYZ)
					 */
					SET_SWZ(new_swizzle, 0, RC_SWIZZLE_X);
				}
				break;
			}
			break;
		/* U.. 0.. 1.. H..*/
		case RC_SWIZZLE_UNUSED:
		case RC_SWIZZLE_ZERO:
		case RC_SWIZZLE_ONE:
		case RC_SWIZZLE_HALF:
			SET_SWZ(new_swizzle, 0, RC_SWIZZLE_X);
			break;
		}
	}

	/* Handle the swizzle in the w channel. */
	swz3 = GET_SWZ(reg->Swizzle, 3);

	/* We can skip this if the swizzle in channel w is an inline constant. */
	if (swz3 <= RC_SWIZZLE_W) {
		for (chan = 0; chan < 3; chan++) {
			unsigned old_swz = GET_SWZ(reg->Swizzle, chan);
			unsigned new_swz = GET_SWZ(new_swizzle, chan);
			/* If the swizzle in the w channel is the same as the
			 * swizzle in any other channels, we need to rewrite it.
			 * For example:
			 * reg->Swizzle == XWZW
			 * new_swizzle  == XYZX
			 * Since the swizzle in the y channel is being
			 * rewritten from W -> Y we need to change the swizzle
			 * in the w channel from W -> Y as well.
			 */
			if (old_swz == swz3) {
				SET_SWZ(new_swizzle, 3,
						GET_SWZ(new_swizzle, chan));
				break;
			}

			/* The swizzle in channel w will be overwritten by one
			 * of the new swizzles. */
			if (new_swz == swz3) {
				/* Find an unused swizzle */
				unsigned i;
				unsigned used = 0;
				for (i = 0; i < 3; i++) {
					used |= 1 << GET_SWZ(new_swizzle, i);
				}
				for (i = 0; i < 4; i++) {
					if (used & (1 << i)) {
						continue;
					}
					SET_SWZ(new_swizzle, 3, i);
				}
			}
		}
	}

	for (chan = 0; chan < 4; chan++) {
		unsigned old_swz = GET_SWZ(reg->Swizzle, chan);
		unsigned new_swz = GET_SWZ(new_swizzle, chan);

		if (old_swz == RC_SWIZZLE_UNUSED) {
			continue;
		}

		/* We don't need to change the swizzle in channel w if it is
		 * an inline constant.  These are always legal in the w channel.
		 *
		 * Swizzles with a value > RC_SWIZZLE_W are inline constants.
		 */
		if (chan == 3 && old_swz > RC_SWIZZLE_W) {
			continue;
		}

		assert(new_swz <= RC_SWIZZLE_W);

		switch (old_swz) {
		case RC_SWIZZLE_ZERO:
			imms[new_swz] = 0.0f;
			break;
		case RC_SWIZZLE_HALF:
			if (reg->Negate & (1 << chan)) {
				imms[new_swz] = -0.5f;
			} else {
				imms[new_swz] = 0.5f;
			}
			break;
		case RC_SWIZZLE_ONE:
			if (reg->Negate & (1 << chan)) {
				imms[new_swz] = -1.0f;
			} else {
				imms[new_swz] = 1.0f;
			}
			break;
		default:
			imms[new_swz] = rc_get_constant_value(c, reg->Index,
					reg->Swizzle, reg->Negate, chan);
		}
		SET_SWZ(reg->Swizzle, chan, new_swz);
	}
	reg->Index = rc_constants_add_immediate_vec4(&c->Program.Constants,
							imms);
	/* We need to set the register file to CONSTANT in case we are
	 * converting a non-constant register with constant swizzles (e.g.
	 * ONE, ZERO, HALF).
	 */
	reg->File = RC_FILE_CONSTANT;
	reg->Negate = 0;
	return 1;
}

void rc_dataflow_swizzles(struct radeon_compiler * c, void *user)
{
	struct rc_instruction * inst;

	for(inst = c->Program.Instructions.Next;
					inst != &c->Program.Instructions;
					inst = inst->Next) {
		const struct rc_opcode_info * opcode =
					rc_get_opcode_info(inst->U.I.Opcode);
		unsigned int src;

		for(src = 0; src < opcode->NumSrcRegs; ++src) {
			struct rc_src_register *reg = &inst->U.I.SrcReg[src];
			if (c->SwizzleCaps->IsNative(inst->U.I.Opcode, *reg)) {
				continue;
			}
			if (!c->is_r500 &&
			    c->Program.Constants.Count < R300_PFS_NUM_CONST_REGS &&
			    try_rewrite_constant(c, reg)) {
				continue;
			}
			rewrite_source(c, inst, src);
		}
	}
	if (c->Debug & RC_DBG_LOG)
		rc_constants_print(&c->Program.Constants);
}
