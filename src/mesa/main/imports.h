/* $Id: imports.h,v 1.4 2002/06/29 19:48:16 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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


#ifndef IMPORTS_H
#define IMPORTS_H


#include "glheader.h"


extern int CAPI
_mesa_sprintf(__GLcontext *gc, char *str, const char *fmt, ...);

extern void
_mesa_warning(__GLcontext *gc, const char *fmtString, ...);

extern void
_mesa_fatal(__GLcontext *gc, char *str);

extern void
_mesa_problem( const __GLcontext *ctx, const char *s );

extern void
_mesa_error( __GLcontext *ctx, GLenum error, const char *fmtString, ... );

extern void
_mesa_debug( const __GLcontext *ctx, const char *fmtString, ... );

extern void
_mesa_printf( const __GLcontext *ctx, const char *fmtString, ... );


extern void
_mesa_init_default_imports(__GLimports *imports, void *driverCtx);


#endif
