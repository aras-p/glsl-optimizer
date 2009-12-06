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
 * Wrapper around the spantmp.h macrofest to generate spans code for
 * all the tiling styles.
 */

#define SPANTMP_PIXEL_FMT INTEL_PIXEL_FMT
#define SPANTMP_PIXEL_TYPE INTEL_PIXEL_TYPE
#define TAG(x) INTEL_TAG(intel_gttmap_##x)
#define TAG2(x, y) INTEL_TAG(intel_gttmap_##x##y)
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT INTEL_PIXEL_FMT
#define SPANTMP_PIXEL_TYPE INTEL_PIXEL_TYPE
#define PUT_VALUE(_x, _y, v) INTEL_WRITE_VALUE(NO_TILE(_x, _y), v)
#define GET_VALUE(_x, _y) INTEL_READ_VALUE(NO_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel##x)
#define TAG2(x, y) INTEL_TAG(intel##x)##y
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT INTEL_PIXEL_FMT
#define SPANTMP_PIXEL_TYPE INTEL_PIXEL_TYPE
#define PUT_VALUE(_x, _y, v) INTEL_WRITE_VALUE(X_TILE(_x, _y), v)
#define GET_VALUE(_x, _y) INTEL_READ_VALUE(X_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel_XTile_##x)
#define TAG2(x, y) INTEL_TAG(intel_XTile_##x)##y
#include "spantmp2.h"

#define SPANTMP_PIXEL_FMT INTEL_PIXEL_FMT
#define SPANTMP_PIXEL_TYPE INTEL_PIXEL_TYPE
#define PUT_VALUE(_x, _y, v) INTEL_WRITE_VALUE(Y_TILE(_x, _y), v)
#define GET_VALUE(_x, _y) INTEL_READ_VALUE(Y_TILE(_x, _y))
#define TAG(x) INTEL_TAG(intel_YTile_##x)
#define TAG2(x, y) INTEL_TAG(intel_YTile_##x)##y
#include "spantmp2.h"

#undef INTEL_PIXEL_FMT
#undef INTEL_PIXEL_TYPE
#undef INTEL_WRITE_VALUE
#undef INTEL_READ_VALUE
#undef INTEL_TAG
