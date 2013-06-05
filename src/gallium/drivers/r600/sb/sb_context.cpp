/*
 * Copyright 2013 Vadim Girlin <vadimgirlin@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Vadim Girlin
 */

#include "sb_bc.h"

namespace r600_sb {

sb_log sblog;

unsigned sb_context::dump_pass = 0;
unsigned sb_context::dump_stat = 0;
unsigned sb_context::dry_run = 0;
unsigned sb_context::no_fallback = 0;
unsigned sb_context::safe_math = 0;

unsigned sb_context::dskip_start = 0;
unsigned sb_context::dskip_end = 0;
unsigned sb_context::dskip_mode = 0;

int sb_context::init(r600_isa *isa, sb_hw_chip chip, sb_hw_class cclass) {
	if (chip == HW_CHIP_UNKNOWN || cclass == HW_CLASS_UNKNOWN)
		return -1;

	this->isa = isa;

	hw_chip = chip;
	hw_class = cclass;

	alu_temp_gprs = 4;

	max_fetch = is_r600() ? 8 : 16;

	has_trans = !is_cayman();

	vtx_src_num = 1;

	num_slots = has_trans ? 5 : 4;

	uses_mova_gpr = is_r600() && chip != HW_CHIP_RV670;

	switch (chip) {
	case HW_CHIP_RV610:
	case HW_CHIP_RS780:
	case HW_CHIP_RV620:
	case HW_CHIP_RS880:

	case HW_CHIP_RV630:
	case HW_CHIP_RV635:
	case HW_CHIP_RV730:
	case HW_CHIP_RV710:
	case HW_CHIP_PALM:
	case HW_CHIP_CEDAR:
		stack_entry_size = 8;
		break;
	default:
		stack_entry_size = 4;
		break;
	}

	return 0;
}

}
