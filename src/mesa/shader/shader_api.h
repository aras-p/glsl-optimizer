/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2004-2006  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef SHADER_API_H
#define SHADER_API_H


#include "glheader.h"
#include "mtypes.h"


extern struct gl_linked_program *
_mesa_new_linked_program(GLcontext *ctx, GLuint name);

extern void
_mesa_free_linked_program_data(GLcontext *ctx,
                               struct gl_linked_program *linked);

extern void
_mesa_delete_linked_program(GLcontext *ctx, struct gl_linked_program *linked);

extern struct gl_linked_program *
_mesa_lookup_linked_program(GLcontext *ctx, GLuint name);


extern struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type);

extern struct gl_program *
_mesa_lookup_shader(GLcontext *ctx, GLuint name);


extern GLvoid
_mesa_init_shaderobjects(GLcontext * ctx);


#endif /* SHADER_API_H */
