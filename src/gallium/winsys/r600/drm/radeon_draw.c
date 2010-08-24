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
struct radeon_draw *radeon_draw(struct radeon *radeon)
{
	struct radeon_draw *draw;

	draw = calloc(1, sizeof(*draw));
	if (draw == NULL)
		return NULL;
	draw->nstate = radeon->nstate;
	draw->radeon = radeon;
	draw->refcount = 1;
	draw->state = calloc(1, sizeof(void*) * draw->nstate);
	if (draw->state == NULL) {
		free(draw);
		return NULL;
	}
	return draw;
}

struct radeon_draw *radeon_draw_incref(struct radeon_draw *draw)
{
	draw->refcount++;
	return draw;
}

struct radeon_draw *radeon_draw_decref(struct radeon_draw *draw)
{
	unsigned i;

	if (draw == NULL)
		return NULL;
	if (--draw->refcount > 0)
		return NULL;
	for (i = 0; i < draw->nstate; i++) {
		draw->state[i] = radeon_state_decref(draw->state[i]);
	}
	free(draw->state);
	memset(draw, 0, sizeof(*draw));
	free(draw);
	return NULL;
}

int radeon_draw_set_new(struct radeon_draw *draw, struct radeon_state *state)
{
	if (state == NULL)
		return 0;
	draw->state[state->id] = radeon_state_decref(draw->state[state->id]);
	draw->state[state->id] = state;
	return 0;
}

int radeon_draw_set(struct radeon_draw *draw, struct radeon_state *state)
{
	if (state == NULL)
		return 0;
	radeon_state_incref(state);
	return radeon_draw_set_new(draw, state);
}

int radeon_draw_check(struct radeon_draw *draw)
{
	unsigned i;
	int r;

	r = radeon_draw_pm4(draw);
	if (r)
		return r;
	for (i = 0, draw->cpm4 = 0; i < draw->nstate; i++) {
		if (draw->state[i]) {
			draw->cpm4 += draw->state[i]->cpm4;
			draw->cpm4 += draw->radeon->type[draw->state[i]->id].header_cpm4;
		}
	}
	return 0;
}

struct radeon_draw *radeon_draw_duplicate(struct radeon_draw *draw)
{
	struct radeon_draw *ndraw;
	unsigned i;

	if (draw == NULL)
		return NULL;
	ndraw = radeon_draw(draw->radeon);
	if (ndraw == NULL) {
		return NULL;
	}
	for (i = 0; i < draw->nstate; i++) {
		if (radeon_draw_set(ndraw, draw->state[i])) {
			radeon_draw_decref(ndraw);
			return NULL;
		}
	}
	return ndraw;
}

int radeon_draw_pm4(struct radeon_draw *draw)
{
	unsigned i;
	int r;

	for (i = 0; i < draw->nstate; i++) {
		r = radeon_state_pm4(draw->state[i]);
		if (r)
			return r;
	}
	return 0;
}
