#include "radeon_program_constants.h"

#ifndef RADEON_PROGRAM_UTIL_H
#define RADEON_PROGRAM_UTIL_H

struct rc_instruction;
struct rc_src_register;

unsigned int rc_swizzle_to_writemask(unsigned int swz);

rc_swizzle get_swz(unsigned int swz, rc_swizzle idx);

unsigned int combine_swizzles4(unsigned int src,
			       rc_swizzle swz_x, rc_swizzle swz_y,
			       rc_swizzle swz_z, rc_swizzle swz_w);

unsigned int combine_swizzles(unsigned int src, unsigned int swz);

rc_swizzle rc_mask_to_swizzle(unsigned int mask);

unsigned swizzle_mask(unsigned swizzle, unsigned mask);

struct rc_src_register lmul_swizzle(unsigned int swizzle, struct rc_src_register srcreg);

void reset_srcreg(struct rc_src_register* reg);

unsigned int rc_src_reads_dst_mask(
		rc_register_file src_file,
		unsigned int src_idx,
		unsigned int src_swz,
		rc_register_file dst_file,
		unsigned int dst_idx,
		unsigned int dst_mask);

unsigned int rc_source_type_swz(unsigned int swizzle, unsigned int channels);

unsigned int rc_source_type_mask(unsigned int mask);

unsigned int rc_inst_can_use_presub(
	struct rc_instruction * inst,
	rc_presubtract_op presub_op,
	unsigned int presub_writemask,
	struct rc_src_register replace_reg,
	struct rc_src_register presub_src0,
	struct rc_src_register presub_src1);

#endif /* RADEON_PROGRAM_UTIL_H */
