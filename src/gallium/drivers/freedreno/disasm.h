/*
 * Copyright © 2012 Rob Clark <robclark@freedesktop.org>
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
 */

#ifndef DISASM_H_
#define DISASM_H_

enum shader_t {
	SHADER_VERTEX,
	SHADER_FRAGMENT,
	SHADER_COMPUTE,
};

/* bitmask of debug flags */
enum debug_t {
	PRINT_RAW      = 0x1,    /* dump raw hexdump */
	PRINT_VERBOSE  = 0x2,
};

int disasm_a2xx(uint32_t *dwords, int sizedwords, int level, enum shader_t type);
int disasm_a3xx(uint32_t *dwords, int sizedwords, int level, enum shader_t type);
void disasm_set_debug(enum debug_t debug);

#endif /* DISASM_H_ */
