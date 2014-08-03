/**
 * \file extensions.h
 * Extension handling.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include "glheader.h"

struct gl_context;
struct gl_extensions;

extern void _mesa_enable_sw_extensions(struct gl_context *ctx);

extern void _mesa_one_time_init_extension_overrides(void);

extern void _mesa_init_extensions(struct gl_extensions *extentions);

extern GLubyte *_mesa_make_extension_string(struct gl_context *ctx);

extern GLuint
_mesa_get_extension_count(struct gl_context *ctx);

extern const GLubyte *
_mesa_get_enabled_extension(struct gl_context *ctx, GLuint index);

extern struct gl_extensions _mesa_extension_override_enables;
extern struct gl_extensions _mesa_extension_override_disables;

#endif
