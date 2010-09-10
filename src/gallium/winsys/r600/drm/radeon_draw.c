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
	draw->radeon = radeon;
	draw->state = calloc(radeon->max_states, sizeof(void*));
	if (draw->state == NULL)
		return -ENOMEM;
	return 0;
}

void radeon_draw_bind(struct radeon_draw *draw, struct radeon_state *state)
{
	if (state == NULL)
		return;
	draw->state[state->state_id] = state;
}

void radeon_draw_unbind(struct radeon_draw *draw, struct radeon_state *state)
{
	if (state == NULL)
		return;
	if (draw->state[state->state_id] == state) {
		draw->state[state->state_id] = NULL;
	}
}
