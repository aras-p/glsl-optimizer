#include "radeon_program_constants.h"

#ifndef RADEON_PROGRAM_UTIL_H
#define RADEON_PROGRAM_UTIL_H

unsigned int rc_swizzle_to_writemask(unsigned int swz);

unsigned int rc_src_reads_dst_mask(
		rc_register_file src_file,
		unsigned int src_idx,
		unsigned int src_swz,
		rc_register_file dst_file,
		unsigned int dst_idx,
		unsigned int dst_mask);

#endif /* RADEON_PROGRAM_UTIL_H */
