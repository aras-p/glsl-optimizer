/*
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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

#ifndef R300_DEFINES_H
#define R300_DEFINES_H

#include "pipe/p_defines.h"

#define R300_MAX_TEXTURE_LEVELS         13
#define R300_MAX_DRAW_VBO_SIZE          (1024 * 1024)

#define R300_TEXTURE_USAGE_TRANSFER     PIPE_TEXTURE_USAGE_CUSTOM

/* Non-atom dirty state flags. */
#define R300_NEW_FRAGMENT_SHADER                0x00000020
#define R300_NEW_FRAGMENT_SHADER_CONSTANTS      0x00000040
#define R300_NEW_VERTEX_SHADER_CONSTANTS        0x10000000
#define R300_NEW_QUERY                          0x40000000
#define R300_NEW_KITCHEN_SINK                   0x7fffffff

/* Tiling flags. */
enum r300_buffer_tiling {
    R300_BUFFER_LINEAR = 0,
    R300_BUFFER_TILED,
    R300_BUFFER_SQUARETILED
};

#endif
