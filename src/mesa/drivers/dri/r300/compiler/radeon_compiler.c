/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
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

#include "radeon_compiler.h"

#include <stdarg.h>


void rc_init(struct radeon_compiler * c)
{
	memset(c, 0, sizeof(*c));

	memory_pool_init(&c->Pool);
	c->Program.Instructions.Prev = &c->Program.Instructions;
	c->Program.Instructions.Next = &c->Program.Instructions;
	c->Program.Instructions.I.Opcode = OPCODE_END;
}

void rc_destroy(struct radeon_compiler * c)
{
	memory_pool_destroy(&c->Pool);
	free(c->ErrorMsg);
}

void rc_debug(struct radeon_compiler * c, const char * fmt, ...)
{
	va_list ap;

	if (!c->Debug)
		return;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

void rc_error(struct radeon_compiler * c, const char * fmt, ...)
{
	va_list ap;

	c->Error = GL_TRUE;

	if (!c->ErrorMsg) {
		/* Only remember the first error */
		char buf[1024];
		int written;

		va_start(ap, fmt);
		written = vsnprintf(buf, sizeof(buf), fmt, ap);
		va_end(ap);

		if (written < sizeof(buf)) {
			c->ErrorMsg = strdup(buf);
		} else {
			c->ErrorMsg = malloc(written + 1);

			va_start(ap, fmt);
			vsnprintf(c->ErrorMsg, written + 1, fmt, ap);
			va_end(ap);
		}
	}

	if (c->Debug) {
		fprintf(stderr, "r300compiler error: ");

		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}
