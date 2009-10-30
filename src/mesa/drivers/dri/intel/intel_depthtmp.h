/*
 * Copyright © 2009 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/**
 * Wrapper around the depthtmp.h macrofest to generate spans code for
 * all the tiling styles.
 */

#define VALUE_TYPE INTEL_VALUE_TYPE
#define WRITE_DEPTH(_x, _y, d) \
   (*(INTEL_VALUE_TYPE *)(irb->region->buffer->virtual +	\
			  NO_TILE(_x, _y)) = d)
#define READ_DEPTH(d, _x, _y) \
   d = *(INTEL_VALUE_TYPE *)(irb->region->buffer->virtual +	\
			     NO_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel_gttmap_##x)
#include "depthtmp.h"

#define VALUE_TYPE INTEL_VALUE_TYPE
#define WRITE_DEPTH(_x, _y, d) INTEL_WRITE_DEPTH(NO_TILE(_x, _y), d)
#define READ_DEPTH(d, _x, _y) d = INTEL_READ_DEPTH(NO_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel##x)
#include "depthtmp.h"

#define VALUE_TYPE INTEL_VALUE_TYPE
#define WRITE_DEPTH(_x, _y, d) INTEL_WRITE_DEPTH(X_TILE(_x, _y), d)
#define READ_DEPTH(d, _x, _y) d = INTEL_READ_DEPTH(X_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel_XTile_##x)
#include "depthtmp.h"

#define VALUE_TYPE INTEL_VALUE_TYPE
#define WRITE_DEPTH(_x, _y, d) INTEL_WRITE_DEPTH(Y_TILE(_x, _y), d)
#define READ_DEPTH(d, _x, _y) d = INTEL_READ_DEPTH(Y_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel_YTile_##x)
#include "depthtmp.h"

#undef INTEL_VALUE_TYPE
#undef INTEL_WRITE_DEPTH
#undef INTEL_READ_DEPTH
#undef INTEL_TAG
