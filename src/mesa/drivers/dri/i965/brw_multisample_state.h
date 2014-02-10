/*
 * Copyright © 2013 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdint.h>

/**
 * 1x MSAA has a single sample at the center: (0.5, 0.5) -> (0x8, 0x8).
 *
 * 2x MSAA sample positions are (0.25, 0.25) and (0.75, 0.75):
 *   4 c
 * 4 0
 * c   1
 */
static const uint32_t
brw_multisample_positions_1x_2x = 0x0088cc44;

/**
 * Sample positions:
 *   2 6 a e
 * 2   0
 * 6       1
 * a 2
 * e     3
 */
static const uint32_t
brw_multisample_positions_4x = 0xae2ae662;

/**
 * Sample positions are based on a solution to the "8 queens" puzzle.
 * Rationale: in a solution to the 8 queens puzzle, no two queens share
 * a row, column, or diagonal.  This is a desirable property for samples
 * in a multisampling pattern, because it ensures that the samples are
 * relatively uniformly distributed through the pixel.
 *
 * There are several solutions to the 8 queens puzzle (see
 * http://en.wikipedia.org/wiki/Eight_queens_puzzle).  This solution was
 * chosen because it has a queen close to the center; this should
 * improve the accuracy of centroid interpolation, since the hardware
 * implements centroid interpolation by choosing the centermost sample
 * that overlaps with the primitive being drawn.
 *
 * Note: from the Ivy Bridge PRM, Vol2 Part1 p304 (3DSTATE_MULTISAMPLE:
 * Programming Notes):
 *
 *     "When programming the sample offsets (for NUMSAMPLES_4 or _8 and
 *     MSRASTMODE_xxx_PATTERN), the order of the samples 0 to 3 (or 7
 *     for 8X) must have monotonically increasing distance from the
 *     pixel center. This is required to get the correct centroid
 *     computation in the device."
 *
 * Sample positions:
 *   1 3 5 7 9 b d f
 * 1     5
 * 3           2
 * 5               6
 * 7 4
 * 9       0
 * b             3
 * d         1
 * f   7
 */
static const uint32_t
brw_multisample_positions_8x[] = { 0xdbb39d79, 0x3ff55117 };
