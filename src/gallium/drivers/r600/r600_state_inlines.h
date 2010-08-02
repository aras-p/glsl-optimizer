/*
 * Copyright 2010 Red Hat Inc.
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef R600_STATE_INLINES_H
#define R600_STATE_INLINES_H

#include "r600_reg.h"

static INLINE uint32_t r600_translate_blend_function(int blend_func)
{
	switch (blend_func) {
        case PIPE_BLEND_ADD:
		return R600_BLEND_FCN_ADD;
        case PIPE_BLEND_SUBTRACT:
		return R600_BLEND_FCN_SUBTRACT;
        case PIPE_BLEND_REVERSE_SUBTRACT:
		return R600_BLEND_FCN_RSUB;
        case PIPE_BLEND_MIN:
		return R600_BLEND_FCN_MIN;
        case PIPE_BLEND_MAX:
		return R600_BLEND_FCN_MAX;
        default:
            fprintf(stderr, "r600: Unknown blend function %d\n", blend_func);
            assert(0);
            break;
    }
    return 0;
}

static INLINE uint32_t r600_translate_blend_factor(int blend_fact)
{
	switch (blend_fact) {
        case PIPE_BLENDFACTOR_ONE:
		return R600_BLEND_ZERO;
        case PIPE_BLENDFACTOR_SRC_COLOR:
		return R600_BLEND_SRC_COLOR;
        case PIPE_BLENDFACTOR_SRC_ALPHA:
		return R600_BLEND_SRC_ALPHA;
        case PIPE_BLENDFACTOR_DST_ALPHA:
		return R600_BLEND_DST_ALPHA;
        case PIPE_BLENDFACTOR_DST_COLOR:
		return R600_BLEND_DST_COLOR;
        case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return R600_BLEND_SRC_ALPHA_SATURATE;
        case PIPE_BLENDFACTOR_CONST_COLOR:
		return R600_BLEND_CONST_COLOR;
        case PIPE_BLENDFACTOR_CONST_ALPHA:
		return R600_BLEND_CONST_ALPHA;
        case PIPE_BLENDFACTOR_ZERO:
		return R600_BLEND_ZERO;
        case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return R600_BLEND_ONE_MINUS_SRC_COLOR;
        case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return R600_BLEND_ONE_MINUS_SRC_ALPHA;
        case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return R600_BLEND_ONE_MINUS_DST_ALPHA;
        case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return R600_BLEND_ONE_MINUS_DST_COLOR;
        case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return R600_BLEND_ONE_MINUS_CONST_COLOR;
        case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return R600_BLEND_ONE_MINUS_CONST_ALPHA;

        case PIPE_BLENDFACTOR_SRC1_COLOR:
		return R600_BLEND_SRC1_COLOR;
        case PIPE_BLENDFACTOR_SRC1_ALPHA:
		return R600_BLEND_SRC1_ALPHA;
        case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
		return R600_BLEND_INV_SRC1_COLOR;
        case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
		return R600_BLEND_INV_SRC1_ALPHA;
	default:
		fprintf(stderr, "r600: Implementation error: "
			"Bad blend factor %d not supported!\n", blend_fact);
		assert(0);
		break;
	}
	return 0;
}

static INLINE uint32_t r600_translate_stencil_op(int s_op)
{
	switch (s_op) {
	case PIPE_STENCIL_OP_KEEP:
		return R600_ZS_KEEP;
        case PIPE_STENCIL_OP_ZERO:
		return R600_ZS_ZERO;
        case PIPE_STENCIL_OP_REPLACE:
		return R600_ZS_REPLACE;
        case PIPE_STENCIL_OP_INCR:
		return R600_ZS_INCR;
        case PIPE_STENCIL_OP_DECR:
		return R600_ZS_DECR;
        case PIPE_STENCIL_OP_INCR_WRAP:
		return R600_ZS_INCR_WRAP;
        case PIPE_STENCIL_OP_DECR_WRAP:
		return R600_ZS_DECR_WRAP;
        case PIPE_STENCIL_OP_INVERT:
		return R600_ZS_INVERT;
        default:
		fprintf(stderr, "r600: Unknown stencil op %d", s_op);
		assert(0);
		break;
	}
	return 0;
}

/* translates straight */
static INLINE uint32_t r600_translate_ds_func(int func)
{
	return func;
}

#endif
