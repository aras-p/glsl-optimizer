/* $Id: extensions.h,v 1.14 2001/06/15 14:18:46 brianp Exp $ */

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


#ifndef _EXTENSIONS_H_
#define _EXTENSIONS_H_

#include "mtypes.h"


extern void _mesa_enable_sw_extensions(GLcontext *ctx);

extern void _mesa_enable_imaging_extensions(GLcontext *ctx);

extern void _mesa_enable_1_3_extensions(GLcontext *ctx);

extern void _mesa_add_extension( GLcontext *ctx, GLboolean enabled,
                                 const char *name, GLboolean *flag_ptr );

extern void _mesa_enable_extension( GLcontext *ctx, const char *name );

extern void _mesa_disable_extension( GLcontext *ctx, const char *name );

extern GLboolean _mesa_extension_is_enabled( GLcontext *ctx, const char *name);

extern void _mesa_extensions_dtr( GLcontext *ctx );

extern void _mesa_extensions_ctr( GLcontext *ctx );

extern const char *_mesa_extensions_get_string( GLcontext *ctx );

#endif
