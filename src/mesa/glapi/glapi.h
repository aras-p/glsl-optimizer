/* $Id: glapi.h,v 1.7 1999/12/15 15:03:16 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


#ifndef _GLAPI_H
#define _GLAPI_H


#include "GL/gl.h"
#include "glapitable.h"


extern void
_glapi_set_dispatch(struct _glapi_table *dispatch);


extern struct _glapi_table *
_glapi_get_dispatch(void);


extern void
_glapi_enable_thread_safety(void);


extern GLuint
_glapi_get_dispatch_table_size(void);


extern const char *
_glapi_get_version(void);


extern void
_glapi_check_table(const struct _glapi_table *table);


extern GLboolean
_glapi_add_entrypoint(const char *funcName, GLuint offset);


extern GLint
_glapi_get_proc_offset(const char *funcName);


extern const GLvoid *
_glapi_get_proc_address(const char *funcName);


extern const char *
_glapi_get_proc_name(GLuint offset);


#endif
