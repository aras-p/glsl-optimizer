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
int radeon_state_init(struct radeon_state *state, struct radeon *radeon, u32 stype, u32 id, u32 shader_type)
{
	struct radeon_stype_info *found = NULL;
	int i, j, shader_index = -1;

	/* traverse the stype array */
	for (i = 0; i < radeon->nstype; i++) {
		/* if the type doesn't match, if the shader doesn't match */
		if (stype != radeon->stype[i].stype)
			continue;
		if (shader_type) {
			for (j = 0; j < 4; j++) {
				if (radeon->stype[i].reginfo[j].shader_type == shader_type) {
					shader_index = j;
					break;
				}
			}
			if (shader_index == -1)
				continue;
		} else {
			if (radeon->stype[i].reginfo[0].shader_type)
				continue;
			else
				shader_index = 0;
		}
		if (id > radeon->stype[i].num)
			continue;
		
		found = &radeon->stype[i];
		break;
	}

	if (!found) {
		fprintf(stderr, "%s invalid type %d/id %d/shader class %d\n", __func__, stype, id, shader_type);
		return -EINVAL;
	}

	memset(state, 0, sizeof(struct radeon_state));
	state->stype = found;
	state->state_id = state->stype->num * shader_index + state->stype->base_id + id;
	state->radeon = radeon;
	state->id = id;
	state->shader_index = shader_index;
	state->refcount = 1;
	state->npm4 = found->npm4;
	state->nstates = found->reginfo[shader_index].nstates;
	return 0;
}

int radeon_state_convert(struct radeon_state *state, u32 stype, u32 id, u32 shader_type)
{
	struct radeon_stype_info *found = NULL;
	int i, j, shader_index = -1;

	if (state == NULL)
		return 0;
	/* traverse the stype array */
	for (i = 0; i < state->radeon->nstype; i++) {
		/* if the type doesn't match, if the shader doesn't match */
		if (stype != state->radeon->stype[i].stype)
			continue;
		if (shader_type) {
			for (j = 0; j < 4; j++) {
				if (state->radeon->stype[i].reginfo[j].shader_type == shader_type) {
					shader_index = j;
					break;
				}
			}
			if (shader_index == -1)
				continue;
		} else {
			if (state->radeon->stype[i].reginfo[0].shader_type)
				continue;
			else
				shader_index = 0;
		}
		if (id > state->radeon->stype[i].num)
			continue;
		
		found = &state->radeon->stype[i];
		break;
	}

	if (!found) {
		fprintf(stderr, "%s invalid type %d/id %d/shader class %d\n", __func__, stype, id, shader_type);
		return -EINVAL;
	}

	if (found->reginfo[shader_index].nstates != state->nstates) {
		fprintf(stderr, "invalid type change from (%d %d %d) to (%d %d %d)\n",
			state->stype->stype, state->id, state->shader_index, stype, id, shader_index);
	}

	state->stype = found;
	state->id = id;
	state->shader_index = shader_index;
	state->state_id = state->stype->num * shader_index + state->stype->base_id + id;
	return radeon_state_pm4(state);
}

void radeon_state_fini(struct radeon_state *state)
{
	unsigned i;

	if (state == NULL)
		return NULL;
	for (i = 0; i < state->nbo; i++) {
		state->bo[i] = radeon_bo_decref(state->radeon, state->bo[i]);
	}
	memset(state, 0, sizeof(struct radeon_state));
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

	if (state == NULL)
		return 0;
	state->cpm4 = 0;
	r = state->stype->pm4(state);
	if (r) {
		fprintf(stderr, "%s failed to build PM4 for state(%d %d)\n",
			__func__, state->stype->stype, state->id);
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
