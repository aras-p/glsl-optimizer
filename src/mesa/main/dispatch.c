/* $Id: dispatch.c,v 1.23 2001/06/05 23:54:00 davem69 Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


/*
 * This file generates all the gl* function entyrpoints.
 * But if we're using X86-optimized dispatch (X86/glapi_x86.S) then
 * we don't use this file's code.
 *
 * Eventually this file may be replaced by automatically generated
 * code from an API spec file.
 *
 * NOTE: This file should _not_ be used when compiling Mesa for a DRI-
 * based device driver.
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "glapi.h"
#include "glapitable.h"
#include "glthread.h"
#endif

#if !(defined(USE_X86_ASM) || defined(USE_SPARC_ASM))

#define KEYWORD1
#define KEYWORD2 GLAPIENTRY
#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   (_glapi_Dispatch->FUNC) ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return (_glapi_Dispatch->FUNC) ARGS


#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#include "glapitemp.h"


#endif /* USE_X86_ASM */
