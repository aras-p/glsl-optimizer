/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include <stdarg.h>

#ifdef WIN32
#include <windows.h>
#include <winddi.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include "pipe/p_debug.h" 
#include "pipe/p_compiler.h" 


void debug_printf(const char *format, ...)
{
   va_list ap;
   va_start( ap, format );  
#ifdef WIN32
   EngDebugPrint("Gallium3D: ", (PCHAR)format, ap);
#else
   vfprintf(stderr, format, ap);
#endif
   va_end( ap );
}


static INLINE debug_abort(void) 
{
#ifdef WIN32
   EngDebugBreak();
#else
   abort();
#endif
}


void debug_assert_fail(const char *expr, const char *file, unsigned line) 
{
   debug_printf("%s:%i: Assertion `%s' failed.");
   debug_abort();
}
