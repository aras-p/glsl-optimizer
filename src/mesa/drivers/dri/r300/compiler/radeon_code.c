/*
 * Copyright (C) 2009 Nicolai Haehnle.
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

#include "main/mtypes.h"
#include "shader/prog_instruction.h"

#include "radeon_code.h"

void rc_constants_init(struct rc_constant_list * c)
{
	memset(c, 0, sizeof(*c));
}

/**
 * Copy a constants structure, assuming that the destination structure
 * is not initialized.
 */
void rc_constants_copy(struct rc_constant_list * dst, struct rc_constant_list * src)
{
	dst->Constants = malloc(sizeof(struct rc_constant) * src->Count);
	memcpy(dst->Constants, src->Constants, sizeof(struct rc_constant) * src->Count);
	dst->Count = src->Count;
	dst->_Reserved = src->Count;
}

void rc_constants_destroy(struct rc_constant_list * c)
{
	free(c->Constants);
	memset(c, 0, sizeof(*c));
}

unsigned rc_constants_add(struct rc_constant_list * c, struct rc_constant * constant)
{
	unsigned index = c->Count;

	if (c->Count >= c->_Reserved) {
		struct rc_constant * newlist;

		c->_Reserved = c->_Reserved * 2;
		if (!c->_Reserved)
			c->_Reserved = 16;

		newlist = malloc(sizeof(struct rc_constant) * c->_Reserved);
		memcpy(newlist, c->Constants, sizeof(struct rc_constant) * c->Count);

		free(c->Constants);
		c->Constants = newlist;
	}

	c->Constants[index] = *constant;
	c->Count++;

	return index;
}
