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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "radeon_priv.h"

/*
 * state core functions
 */
struct radeon_state *radeon_state(struct radeon *radeon, u32 type, u32 id)
{
	struct radeon_state *state;

	if (type > radeon->ntype) {
		fprintf(stderr, "%s invalid type %d\n", __func__, type);
		return NULL;
	}
	if (id > radeon->nstate) {
		fprintf(stderr, "%s invalid state id %d\n", __func__, id);
		return NULL;
	}
	state = calloc(1, sizeof(*state));
	if (state == NULL)
		return NULL;
	state->radeon = radeon;
	state->type = type;
	state->id = id;
	state->refcount = 1;
	state->npm4 = radeon->type[type].npm4;
	state->nstates = radeon->type[type].nstates;
	state->states = calloc(1, state->nstates * 4);
	state->pm4 = calloc(1, radeon->type[type].npm4 * 4);
	if (state->states == NULL || state->pm4 == NULL) {
		radeon_state_decref(state);
		return NULL;
	}
	return state;
}

struct radeon_state *radeon_state_duplicate(struct radeon_state *state)
{
	struct radeon_state *nstate = radeon_state(state->radeon, state->type, state->id);
	unsigned i;

	if (state == NULL)
		return NULL;
	nstate->cpm4 = state->cpm4;
	nstate->nbo = state->nbo;
	nstate->nreloc = state->nreloc;
	memcpy(nstate->states, state->states, state->nstates * 4);
	memcpy(nstate->pm4, state->pm4, state->npm4 * 4);
	memcpy(nstate->placement, state->placement, 8 * 4);
	memcpy(nstate->reloc_pm4_id, state->reloc_pm4_id, 8 * 4);
	memcpy(nstate->reloc_bo_id, state->reloc_bo_id, 8 * 4);
	memcpy(nstate->bo_dirty, state->bo_dirty, 4 * 4);
	for (i = 0; i < state->nbo; i++) {
		nstate->bo[i] = radeon_bo_incref(state->radeon, state->bo[i]);
	}
	return nstate;
}

struct radeon_state *radeon_state_incref(struct radeon_state *state)
{
	state->refcount++;
	return state;
}

struct radeon_state *radeon_state_decref(struct radeon_state *state)
{
	unsigned i;

	if (state == NULL)
		return NULL;
	if (--state->refcount > 0) {
		return NULL;
	}
	for (i = 0; i < state->nbo; i++) {
		state->bo[i] = radeon_bo_decref(state->radeon, state->bo[i]);
	}
	free(state->immd);
	free(state->states);
	free(state->pm4);
	memset(state, 0, sizeof(*state));
	free(state);
	return NULL;
}

int radeon_state_replace_always(struct radeon_state *ostate,
				struct radeon_state *nstate)
{
	return 1;
}

int radeon_state_pm4_generic(struct radeon_state *state)
{
	return -EINVAL;
}

static u32 crc32(void *d, size_t len)
{
	u16 *data = (uint16_t*)d;
	u32 sum1 = 0xffff, sum2 = 0xffff;

	len = len >> 1;
	while (len) {
		unsigned tlen = len > 360 ? 360 : len;
		len -= tlen;
		do {
			sum1 += *data++;
			sum2 += sum1;
		} while (--tlen);
		sum1 = (sum1 & 0xffff) + (sum1 >> 16);
		sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	}
	/* Second reduction step to reduce sums to 16 bits */
	sum1 = (sum1 & 0xffff) + (sum1 >> 16);
	sum2 = (sum2 & 0xffff) + (sum2 >> 16);
	return sum2 << 16 | sum1;
}

int radeon_state_pm4(struct radeon_state *state)
{
	int r;

	if (state == NULL || state->cpm4)
		return 0;
	r = state->radeon->type[state->type].pm4(state);
	if (r) {
		fprintf(stderr, "%s failed to build PM4 for state(%d %d)\n",
			__func__, state->type, state->id);
		return r;
	}
	state->pm4_crc = crc32(state->pm4, state->cpm4 * 4);
	return 0;
}

int radeon_state_reloc(struct radeon_state *state, unsigned id, unsigned bo_id)
{
	state->reloc_pm4_id[state->nreloc] = id;
	state->reloc_bo_id[state->nreloc] = bo_id;
	state->nreloc++;
	return 0;
}
