/*
 * Copyright (C) 2008 Nicolai Haehnle.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __R300_FRAGPROG_SWIZZLE_H_
#define __R300_FRAGPROG_SWIZZLE_H_

#include "main/glheader.h"
#include "shader/prog_instruction.h"

struct nqssadce_state;

GLboolean r300FPIsNativeSwizzle(GLuint opcode, struct prog_src_register reg);
void r300FPBuildSwizzle(struct nqssadce_state*, struct prog_dst_register dst, struct prog_src_register src);

GLuint r300FPTranslateRGBSwizzle(GLuint src, GLuint swizzle);
GLuint r300FPTranslateAlphaSwizzle(GLuint src, GLuint swizzle);

#endif /* __R300_FRAGPROG_SWIZZLE_H_ */
