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

#include "radeon_compiler_util.h"

#include "radeon_compiler.h"
#include "radeon_dataflow.h"
/**
 */
unsigned int rc_swizzle_to_writemask(unsigned int swz)
{
	unsigned int mask = 0;
	unsigned int i;

	for(i = 0; i < 4; i++) {
		mask |= 1 << GET_SWZ(swz, i);
	}
	mask &= RC_MASK_XYZW;

	return mask;
}

rc_swizzle get_swz(unsigned int swz, rc_swizzle idx)
{
	if (idx & 0x4)
		return idx;
	return GET_SWZ(swz, idx);
}

/**
 * The purpose of this function is to standardize the number channels used by
 * swizzles.  All swizzles regardless of what instruction they are a part of
 * should have 4 channels initialized with values.
 * @param channels The number of channels in initial_value that have a
 * meaningful value.
 * @return An initialized swizzle that has all of the unused channels set to
 * RC_SWIZZLE_UNUSED.
 */
unsigned int rc_init_swizzle(unsigned int initial_value, unsigned int channels)
{
	unsigned int i;
	for (i = channels; i < 4; i++) {
		SET_SWZ(initial_value, i, RC_SWIZZLE_UNUSED);
	}
	return initial_value;
}

unsigned int combine_swizzles4(unsigned int src,
		rc_swizzle swz_x, rc_swizzle swz_y, rc_swizzle swz_z, rc_swizzle swz_w)
{
	unsigned int ret = 0;

	ret |= get_swz(src, swz_x);
	ret |= get_swz(src, swz_y) << 3;
	ret |= get_swz(src, swz_z) << 6;
	ret |= get_swz(src, swz_w) << 9;

	return ret;
}

unsigned int combine_swizzles(unsigned int src, unsigned int swz)
{
	unsigned int ret = 0;

	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_X));
	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_Y)) << 3;
	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_Z)) << 6;
	ret |= get_swz(src, GET_SWZ(swz, RC_SWIZZLE_W)) << 9;

	return ret;
}

/**
 * @param mask Must be either RC_MASK_X, RC_MASK_Y, RC_MASK_Z, or RC_MASK_W
 */
rc_swizzle rc_mask_to_swizzle(unsigned int mask)
{
	switch (mask) {
	case RC_MASK_X: return RC_SWIZZLE_X;
	case RC_MASK_Y: return RC_SWIZZLE_Y;
	case RC_MASK_Z: return RC_SWIZZLE_Z;
	case RC_MASK_W: return RC_SWIZZLE_W;
	}
	return RC_SWIZZLE_UNUSED;
}

/* Reorder mask bits according to swizzle. */
unsigned swizzle_mask(unsigned swizzle, unsigned mask)
{
	unsigned ret = 0;
	for (unsigned chan = 0; chan < 4; ++chan) {
		unsigned swz = GET_SWZ(swizzle, chan);
		if (swz < 4)
			ret |= GET_BIT(mask, swz) << chan;
	}
	return ret;
}

/**
 * Left multiplication of a register with a swizzle
 */
struct rc_src_register lmul_swizzle(unsigned int swizzle, struct rc_src_register srcreg)
{
	struct rc_src_register tmp = srcreg;
	int i;
	tmp.Swizzle = 0;
	tmp.Negate = 0;
	for(i = 0; i < 4; ++i) {
		rc_swizzle swz = GET_SWZ(swizzle, i);
		if (swz < 4) {
			tmp.Swizzle |= GET_SWZ(srcreg.Swizzle, swz) << (i*3);
			tmp.Negate |= GET_BIT(srcreg.Negate, swz) << i;
		} else {
			tmp.Swizzle |= swz << (i*3);
		}
	}
	return tmp;
}

void reset_srcreg(struct rc_src_register* reg)
{
	memset(reg, 0, sizeof(struct rc_src_register));
	reg->Swizzle = RC_SWIZZLE_XYZW;
}

unsigned int rc_src_reads_dst_mask(
		rc_register_file src_file,
		unsigned int src_idx,
		unsigned int src_swz,
		rc_register_file dst_file,
		unsigned int dst_idx,
		unsigned int dst_mask)
{
	if (src_file != dst_file || src_idx != dst_idx) {
		return RC_MASK_NONE;
	}
	return dst_mask & rc_swizzle_to_writemask(src_swz);
}

/**
 * @return A bit mask specifying whether this swizzle will select from an RGB
 * source, an Alpha source, or both.
 */
unsigned int rc_source_type_swz(unsigned int swizzle)
{
	unsigned int chan;
	unsigned int swz = RC_SWIZZLE_UNUSED;
	unsigned int ret = RC_SOURCE_NONE;

	for(chan = 0; chan < 4; chan++) {
		swz = GET_SWZ(swizzle, chan);
		if (swz == RC_SWIZZLE_W) {
			ret |= RC_SOURCE_ALPHA;
		} else if (swz == RC_SWIZZLE_X || swz == RC_SWIZZLE_Y
						|| swz == RC_SWIZZLE_Z) {
			ret |= RC_SOURCE_RGB;
		}
	}
	return ret;
}

unsigned int rc_source_type_mask(unsigned int mask)
{
	unsigned int ret = RC_SOURCE_NONE;

	if (mask & RC_MASK_XYZ)
		ret |= RC_SOURCE_RGB;

	if (mask & RC_MASK_W)
		ret |= RC_SOURCE_ALPHA;

	return ret;
}

struct can_use_presub_data {
	struct rc_src_register RemoveSrcs[3];
	unsigned int RGBCount;
	unsigned int AlphaCount;
};

static void can_use_presub_read_cb(
	void * userdata,
	struct rc_instruction * inst,
	rc_register_file file,
	unsigned int index,
	unsigned int mask)
{
	struct can_use_presub_data * d = userdata;
	unsigned int src_type = rc_source_type_mask(mask);
	unsigned int i;

	if (file == RC_FILE_NONE)
		return;

	for(i = 0; i < 3; i++) {
		if (d->RemoveSrcs[i].File == file
		    && d->RemoveSrcs[i].Index == index) {
			src_type &=
				~rc_source_type_swz(d->RemoveSrcs[i].Swizzle);
		}
	}

	if (src_type & RC_SOURCE_RGB)
		d->RGBCount++;

	if (src_type & RC_SOURCE_ALPHA)
		d->AlphaCount++;
}

unsigned int rc_inst_can_use_presub(
	struct rc_instruction * inst,
	rc_presubtract_op presub_op,
	unsigned int presub_writemask,
	struct rc_src_register replace_reg,
	struct rc_src_register presub_src0,
	struct rc_src_register presub_src1)
{
	struct can_use_presub_data d;
	unsigned int num_presub_srcs;
	const struct rc_opcode_info * info =
					rc_get_opcode_info(inst->U.I.Opcode);

	if (presub_op == RC_PRESUB_NONE) {
		return 1;
	}

	if (info->HasTexture) {
		return 0;
	}

	/* We can't use more than one presubtract value in an
	 * instruction, unless the two prsubtract operations
	 * are the same and read from the same registers.
	 * XXX For now we will limit instructions to only one presubtract
	 * value.*/
	if (inst->U.I.PreSub.Opcode != RC_PRESUB_NONE) {
		return 0;
	}

	memset(&d, 0, sizeof(d));
	d.RemoveSrcs[0] = replace_reg;
	d.RemoveSrcs[1] = presub_src0;
	d.RemoveSrcs[2] = presub_src1;

	rc_for_all_reads_mask(inst, can_use_presub_read_cb, &d);

	num_presub_srcs = rc_presubtract_src_reg_count(presub_op);

	if (d.RGBCount + num_presub_srcs > 3 || d.AlphaCount + num_presub_srcs > 3) {
		return 0;
	}

	return 1;
}

