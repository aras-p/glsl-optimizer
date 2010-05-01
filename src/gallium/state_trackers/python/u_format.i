/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


static INLINE const char *
util_format_name(enum pipe_format format);

static INLINE boolean
util_format_is_s3tc(enum pipe_format format);

static INLINE boolean
util_format_is_depth_or_stencil(enum pipe_format format);

static INLINE boolean
util_format_is_depth_and_stencil(enum pipe_format format);


uint
util_format_get_blocksizebits(enum pipe_format format);

uint
util_format_get_blocksize(enum pipe_format format);

uint
util_format_get_blockwidth(enum pipe_format format);

uint
util_format_get_blockheight(enum pipe_format format);

unsigned
util_format_get_nblocksx(enum pipe_format format,
                         unsigned x);

unsigned
util_format_get_nblocksy(enum pipe_format format,
                         unsigned y);

unsigned
util_format_get_nblocks(enum pipe_format format,
                        unsigned width,
                        unsigned height);

size_t
util_format_get_stride(enum pipe_format format,
                       unsigned width);

size_t
util_format_get_2d_size(enum pipe_format format,
                        size_t stride,
                        unsigned height);

uint
util_format_get_component_bits(enum pipe_format format,
                               enum util_format_colorspace colorspace,
                               uint component);

boolean
util_format_has_alpha(enum pipe_format format);


unsigned
util_format_get_nr_components(enum pipe_format format);


