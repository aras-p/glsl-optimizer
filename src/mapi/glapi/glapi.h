/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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


/**
 * \mainpage Mesa GL API Module
 *
 * \section GLAPIIntroduction Introduction
 *
 * The Mesa GL API module is responsible for dispatching all the
 * gl*() functions.  All GL functions are dispatched by jumping through
 * the current dispatch table (basically a struct full of function
 * pointers.)
 *
 * A per-thread current dispatch table and per-thread current context
 * pointer are managed by this module too.
 *
 * This module is intended to be non-Mesa-specific so it can be used
 * with the X/DRI libGL also.
 */


#ifndef _GLAPI_H
#define _GLAPI_H

#ifndef MAPI_GLAPI_CURRENT
#define MAPI_GLAPI_CURRENT
#endif

#ifdef USE_MGL_NAMESPACE
#define _glapi_set_dispatch _mglapi_set_dispatch
#define _glapi_get_dispatch _mglapi_get_dispatch
#define _glapi_set_context _mglapi_set_context
#define _glapi_get_context _mglapi_get_context
#define _glapi_Dispatch _mglapi_Dispatch
#define _glapi_Context _mglapi_Context
#endif

#include "mapi/u_current.h"
#include "glapi/glthread.h"

typedef void (*_glapi_proc)(void);

#define GET_DISPATCH() ((struct _glapi_table *) u_current_get())
#define GET_CURRENT_CONTEXT(C) GLcontext *C = (GLcontext *) u_current_get_user()

PUBLIC unsigned int
_glapi_get_dispatch_table_size(void);


PUBLIC int
_glapi_add_dispatch( const char * const * function_names,
		     const char * parameter_signature );

PUBLIC int
_glapi_get_proc_offset(const char *funcName);


PUBLIC _glapi_proc
_glapi_get_proc_address(const char *funcName);


PUBLIC const char *
_glapi_get_proc_name(unsigned int offset);


#endif
