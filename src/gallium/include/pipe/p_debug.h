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

/**
 * @file
 * Cross-platform debugging helpers.
 * 
 * For now it just has assert and printf replacements, but it might be extended 
 * with stack trace reports and more advanced logging in the near future. 
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef P_DEBUG_H_
#define P_DEBUG_H_


#include <stdarg.h>


#ifdef	__cplusplus
extern "C" {
#endif


#ifdef DBG
#ifndef DEBUG
#define DEBUG 1
#endif
#else
#ifndef NDEBUG
#define NDEBUG 1
#endif
#endif


void debug_printf(const char *format, ...);

void debug_vprintf(const char *format, va_list ap);

void debug_assert_fail(const char *expr, const char *file, unsigned line);


/** Assert macro */
#ifdef DEBUG
#define debug_assert(expr) ((expr) ? (void)0 : debug_assert_fail(#expr, __FILE__, __LINE__))
#else
#define debug_assert(expr) ((void)0)
#endif


#ifdef assert
#undef assert
#endif
#define assert(expr) debug_assert(expr)


#ifdef	__cplusplus
}
#endif

#endif /* P_DEBUG_H_ */
