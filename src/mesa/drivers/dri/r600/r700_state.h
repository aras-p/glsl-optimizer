/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#ifndef _R700_STATE_H
#define _R700_STATE_H

#include "main/mtypes.h"

#include "r600_context.h"

#include "r700_chip.h"

extern void r700UpdateStateParameters(struct gl_context * ctx, GLuint new_state);
extern void r700UpdateShaders (struct gl_context * ctx);
extern void r700UpdateShaderStates(struct gl_context * ctx);

extern void r700UpdateViewportOffset(struct gl_context * ctx);

extern void r700InitState (struct gl_context * ctx);
extern void r700InitStateFuncs (radeonContextPtr radeon, struct dd_function_table *functions);

extern void r700SetScissor(context_t *context);

#endif	/* _R600_SCREEN_H */
