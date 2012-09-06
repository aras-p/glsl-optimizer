/*
 * Copyright 2012 Advanced Micro Devices, Inc.
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
 * Author: Tom Stellard <thomas.stellard@amd.com>
 */

#ifndef RADEON_REGALLOC_H
#define RADEON_REGALLOC_H

struct ra_regs;

enum rc_reg_class {
	RC_REG_CLASS_SINGLE,
	RC_REG_CLASS_DOUBLE,
	RC_REG_CLASS_TRIPLE,
	RC_REG_CLASS_ALPHA,
	RC_REG_CLASS_SINGLE_PLUS_ALPHA,
	RC_REG_CLASS_DOUBLE_PLUS_ALPHA,
	RC_REG_CLASS_TRIPLE_PLUS_ALPHA,
	RC_REG_CLASS_X,
	RC_REG_CLASS_Y,
	RC_REG_CLASS_Z,
	RC_REG_CLASS_XY,
	RC_REG_CLASS_YZ,
	RC_REG_CLASS_XZ,
	RC_REG_CLASS_XW,
	RC_REG_CLASS_YW,
	RC_REG_CLASS_ZW,
	RC_REG_CLASS_XYW,
	RC_REG_CLASS_YZW,
	RC_REG_CLASS_XZW,
	RC_REG_CLASS_COUNT
};

struct rc_regalloc_state {
	struct ra_regs *regs;
	unsigned class_ids[RC_REG_CLASS_COUNT];
};

void rc_init_regalloc_state(struct rc_regalloc_state *s);
void rc_destroy_regalloc_state(struct rc_regalloc_state *s);

#endif /* RADEON_REGALLOC_H */
