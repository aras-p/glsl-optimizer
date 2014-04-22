/*
 * Copyright © 2014 Intel Corporation
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

#ifndef BRW_META_UTIL_H
#define BRW_META_UTIL_H

#include <stdbool.h>
#include "main/mtypes.h"

#ifdef __cplusplus
extern "C" {
#endif

bool
brw_meta_mirror_clip_and_scissor(const struct gl_context *ctx,
                                 GLfloat *srcX0, GLfloat *srcY0,
                                 GLfloat *srcX1, GLfloat *srcY1,
                                 GLfloat *dstX0, GLfloat *dstY0,
                                 GLfloat *dstX1, GLfloat *dstY1,
                                 bool *mirror_x, bool *mirror_y);

#ifdef __cplusplus
}
#endif

#endif /* BRW_META_UTIL_H */
