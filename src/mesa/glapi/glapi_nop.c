/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2010  VMWare, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * No-op dispatch table.
 *
 * This file defines a special dispatch table which is loaded with no-op
 * functions.
 *
 * When there's no current rendering context, calling a GL function like
 * glBegin() is a no-op.  Apps should never normally do this.  So as a
 * debugging aid, each of the no-op functions will emit a warning to
 * stderr if the MESA_DEBUG or LIBGL_DEBUG env var is set.
 */



#include "main/compiler.h"
#include "glapi/glapi.h"


static GLboolean WarnFlag = GL_FALSE;
static _glapi_warning_func warning_func;

/*
 * Enable/disable printing of warning messages.
 */
PUBLIC void
_glapi_noop_enable_warnings(GLboolean enable)
{
  WarnFlag = enable;
}

/*
 * Register a callback function for reporting errors.
 */
PUBLIC void
_glapi_set_warning_func( _glapi_warning_func func )
{
  warning_func = func;
}


static int
warn(const char *func)
{
#if !defined(_WIN32_WCE)
   if ((WarnFlag || getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
       && warning_func) {
     warning_func(NULL, "GL User Error: called without context: %s", func);
   }
#endif
  return 0;
}

#ifdef DEBUG

#define KEYWORD1 static
#define KEYWORD1_ALT static
#define KEYWORD2 GLAPIENTRY
#define NAME(func)  NoOp##func
#define F NULL

#define DISPATCH(func, args, msg)					      \
  warn(#func);

#define RETURN_DISPATCH(func, args, msg)				      \
  warn(#func); return 0

#define TABLE_ENTRY(name) (_glapi_proc) NoOp##name

#else

static void
NoOpGeneric(void)
{
   if ((WarnFlag || getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
      && warning_func) {
      warning_func(NULL, "GL User Error: calling GL function");
   }
}


#define TABLE_ENTRY(name) (_glapi_proc) NoOpGeneric

#endif

#define DISPATCH_TABLE_NAME __glapi_noop_table
#define UNUSED_TABLE_NAME __unused_noop_functions

static int NoOpUnused(void)
{
   return warn("extension function");
}

#include "glapi/glapitemp.h"
