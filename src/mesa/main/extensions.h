/* $Id: extensions.h,v 1.8 2000/03/07 18:24:14 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include "types.h"


#define DEFAULT_OFF    0x0
#define DEFAULT_ON     0x1
#define ALWAYS_ENABLED 0x2

/* Return 0 on success.
 */
extern int gl_extensions_add( GLcontext *ctx, int state, 
			      const char *name, void (*notify)( void ) );

extern int gl_extensions_enable( GLcontext *ctx, const char *name );
extern int gl_extensions_disable( GLcontext *ctx, const char *name );
extern GLboolean gl_extension_is_enabled( GLcontext *ctx, const char *name);
extern void gl_extensions_dtr( GLcontext *ctx );
extern void gl_extensions_ctr( GLcontext *ctx );
extern const char *gl_extensions_get_string( GLcontext *ctx );

#endif


