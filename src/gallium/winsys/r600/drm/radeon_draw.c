/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
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
 *      Jerome Glisse
 */
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "radeon_priv.h"

/*
 * draw functions
 */
int radeon_draw_init(struct radeon_draw *draw, struct radeon *radeon)
{
	memset(draw, 0, sizeof(struct radeon_draw));
	draw->nstate = radeon->nstate;
	draw->radeon = radeon;
	return 0;
}

int radeon_draw_set(struct radeon_draw *draw, struct radeon_state *state)
{
	if (state == NULL)
		return 0;
	if (state->type >= draw->radeon->ntype)
		return -EINVAL;
	draw->state[state->id] = *state;
	return 0;
}

int radeon_draw_check(struct radeon_draw *draw)
{
	unsigned i;
	int r;

	for (i = 0, draw->cpm4 = 0; i < draw->nstate; i++) {
		if (draw->state[i].valid) {
			draw->cpm4 += draw->state[i].cpm4;
		}
	}
	return 0;
}
