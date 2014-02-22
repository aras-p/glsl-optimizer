/* -*- mode: C; c-file-style: "k&r"; tab-width 4; indent-tabs-mode: t; -*- */

/*
 * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Rob Clark <robclark@freedesktop.org>
 */

#ifndef FD3_UTIL_H_
#define FD3_UTIL_H_

#include "freedreno_util.h"

#include "a3xx.xml.h"

enum a3xx_vtx_fmt fd3_pipe2vtx(enum pipe_format format);
enum a3xx_tex_fmt fd3_pipe2tex(enum pipe_format format);
enum a3xx_tex_fetchsize fd3_pipe2fetchsize(enum pipe_format format);
enum a3xx_color_fmt fd3_pipe2color(enum pipe_format format);
enum pipe_format fd3_gmem_restore_format(enum pipe_format format);
enum a3xx_color_swap fd3_pipe2swap(enum pipe_format format);

uint32_t fd3_tex_swiz(enum pipe_format format, unsigned swizzle_r,
		unsigned swizzle_g, unsigned swizzle_b, unsigned swizzle_a);

/* Configuration key used to identify a shader variant.. different
 * shader variants can be used to implement features not supported
 * in hw (two sided color), binning-pass vertex shader, etc.
 *
 * NOTE: this is declared here (rather than fd3_program.h) as it is
 * passed around through a lot of the emit code in various parts
 * which would otherwise not necessarily need to incl fd3_program.h
 */
struct fd3_shader_key {
	/* vertex shader variant parameters: */
	unsigned binning_pass : 1;

	/* fragment shader variant parameters: */
	unsigned color_two_side : 1;
	unsigned half_precision : 1;
};
struct fd3_shader_variant;

#endif /* FD3_UTIL_H_ */
