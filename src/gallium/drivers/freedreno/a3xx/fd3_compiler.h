/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FD3_COMPILER_H_
#define FD3_COMPILER_H_

#include "fd3_program.h"
#include "fd3_util.h"


/* ************************************************************************* */
/* split this out or find some helper to use.. like main/bitset.h.. */

#define MAX_REG 256

typedef uint8_t regmask_t[2 * MAX_REG / 8];

static inline unsigned regmask_idx(struct ir3_register *reg)
{
	unsigned num = reg->num;
	assert(num < MAX_REG);
	if (reg->flags & IR3_REG_HALF)
		num += MAX_REG;
	return num;
}

static inline void regmask_init(regmask_t *regmask)
{
	memset(regmask, 0, sizeof(*regmask));
}

static inline void regmask_set(regmask_t *regmask, struct ir3_register *reg)
{
	unsigned idx = regmask_idx(reg);
	unsigned i;
	for (i = 0; i < 4; i++, idx++)
		if (reg->wrmask & (1 << i))
			(*regmask)[idx / 8] |= 1 << (idx % 8);
}

static inline unsigned regmask_get(regmask_t *regmask,
		struct ir3_register *reg)
{
	unsigned idx = regmask_idx(reg);
	unsigned i;
	for (i = 0; i < 4; i++, idx++)
		if (reg->wrmask & (1 << i))
			if ((*regmask)[idx / 8] & (1 << (idx % 8)))
				return true;
	return false;
}

/* comp:
 *   0 - x
 *   1 - y
 *   2 - z
 *   3 - w
 */
static inline uint32_t regid(int num, int comp)
{
	return (num << 2) | (comp & 0x3);
}

/* ************************************************************************* */

int fd3_compile_shader(struct fd3_shader_stateobj *so,
		const struct tgsi_token *tokens);

#endif /* FD3_COMPILER_H_ */
