/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * \file glapi_dispatch.c
 *
 * This file generates all the gl* function entrypoints.  This code is not
 * used if optimized assembly stubs are available (e.g., using
 * glapi/glapi_x86.S on IA32 or glapi/glapi_sparc.S on SPARC).
 *
 * \note
 * This file is also used to build the client-side libGL that loads DRI-based
 * device drivers.  At build-time it is symlinked to src/glx.
 *
 * \author Brian Paul <brian@precisioninsight.com>
 */

#include "glapi/glapi_priv.h"
#include "glapi/glapitable.h"
#include "glapi/glapidispatch.h"

#if !(defined(USE_X86_ASM) || defined(USE_X86_64_ASM) || defined(USE_SPARC_ASM))

#if defined(WIN32)
#define KEYWORD1 GLAPI
#else
#define KEYWORD1 PUBLIC
#endif

#define KEYWORD2 GLAPIENTRY

#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   CALL_ ## FUNC(GET_DISPATCH(), ARGS);

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return CALL_ ## FUNC(GET_DISPATCH(), ARGS);


#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#ifdef GLX_INDIRECT_RENDERING
/* those link to libglapi.a should provide the entry points */
#define _GLAPI_SKIP_PROTO_ENTRY_POINTS
#endif
#include "glapi/glapitemp.h"

#endif /* USE_X86_ASM */


#ifdef DEBUG

static void *logger_data;
static void (*logger_func)(void *data, const char *fmt, ...);
static struct _glapi_table *real_dispatch; /* FIXME: This need to be TLS etc */

#define KEYWORD1 static
#define KEYWORD1_ALT static
#define KEYWORD2
#define NAME(func)  log_##func
#define F logger_data

static void
log_Unused(void)
{
}

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   logger_func MESSAGE;				\
   CALL_ ## FUNC(real_dispatch, ARGS);

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   logger_func MESSAGE;			\
   return CALL_ ## FUNC(real_dispatch, ARGS);

#define DISPATCH_TABLE_NAME __glapi_logging_table

#define TABLE_ENTRY(func) (_glapi_proc) log_##func

#include "glapi/glapitemp.h"

int
_glapi_logging_available(void)
{
   return 1;
}

void
_glapi_enable_logging(void (*func)(void *data, const char *fmt, ...),
		      void *data)
{
   real_dispatch = GET_DISPATCH();
   logger_func = func;
   logger_data = data;
   _glapi_set_dispatch(&__glapi_logging_table);
}

#else

int
_glapi_logging_available(void)
{
   return 0
}

void
_glapi_enable_logging(void (*func)(void *data, const char *fmt, ...),
		      void *data)
{
}

#endif
