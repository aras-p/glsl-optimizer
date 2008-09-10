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

#include "p_compiler.h"


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

   
/* MSVC bebore VC7 does not have the __FUNCTION__ macro */
#if defined(_MSC_VER) && _MSC_VER < 1300
#define __FUNCTION__ "???"
#endif


void _debug_vprintf(const char *format, va_list ap);
   

static INLINE void
_debug_printf(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
}


/**
 * Print debug messages.
 *
 * The actual channel used to output debug message is platform specific. To 
 * avoid misformating or truncation, follow these rules of thumb:   
 * - output whole lines
 * - avoid outputing large strings (512 bytes is the current maximum length 
 * that is guaranteed to be printed in all platforms)
 */
static INLINE void
debug_printf(const char *format, ...)
{
#ifdef DEBUG
   va_list ap;
   va_start(ap, format);
   _debug_vprintf(format, ap);
   va_end(ap);
#else
   (void) format; /* silence warning */
#endif
}


#ifdef DEBUG
#define debug_vprintf(_format, _ap) _debug_vprintf(_format, _ap)
#else
#define debug_vprintf(_format, _ap) ((void)0)
#endif


#ifdef DEBUG
/**
 * Dump a blob in hex to the same place that debug_printf sends its
 * messages.
 */
void debug_print_blob( const char *name, const void *blob, unsigned size );

/* Print a message along with a prettified format string
 */
void debug_print_format(const char *msg, unsigned fmt );
#else
#define debug_print_blob(_name, _blob, _size) ((void)0)
#define debug_print_format(_msg, _fmt) ((void)0)
#endif


void _debug_break(void);


/**
 * Hard-coded breakpoint.
 */
#ifdef DEBUG
#if defined(PIPE_ARCH_X86) && defined(PIPE_CC_GCC)
#define debug_break() __asm("int3")
#elif defined(PIPE_ARCH_X86) && defined(PIPE_CC_MSVC)
#define debug_break()  do { _asm {int 3} } while(0)
#else
#define debug_break() _debug_break()
#endif
#else /* !DEBUG */
#define debug_break() ((void)0)
#endif /* !DEBUG */


long
debug_get_num_option(const char *name, long dfault);

void _debug_assert_fail(const char *expr, 
                        const char *file, 
                        unsigned line, 
                        const char *function);


/** 
 * Assert macro
 * 
 * Do not expect that the assert call terminates -- errors must be handled 
 * regardless of assert behavior.
 */
#ifdef DEBUG
#define debug_assert(expr) ((expr) ? (void)0 : _debug_assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__))
#else
#define debug_assert(expr) ((void)0)
#endif


/** Override standard assert macro */
#ifdef assert
#undef assert
#endif
#define assert(expr) debug_assert(expr)


/**
 * Output the current function name.
 */
#ifdef DEBUG
#define debug_checkpoint() \
   _debug_printf("%s\n", __FUNCTION__)
#else
#define debug_checkpoint() \
   ((void)0) 
#endif


/**
 * Output the full source code position.
 */
#ifdef DEBUG
#define debug_checkpoint_full() \
   _debug_printf("%s:%u:%s", __FILE__, __LINE__, __FUNCTION__) 
#else
#define debug_checkpoint_full() \
   ((void)0) 
#endif


/**
 * Output a warning message. Muted on release version.
 */
#ifdef DEBUG
#define debug_warning(__msg) \
   _debug_printf("%s:%u:%s: warning: %s\n", __FILE__, __LINE__, __FUNCTION__, __msg)
#else
#define debug_warning(__msg) \
   ((void)0) 
#endif


/**
 * Output an error message. Not muted on release version.
 */
#ifdef DEBUG
#define debug_error(__msg) \
   _debug_printf("%s:%u:%s: error: %s\n", __FILE__, __LINE__, __FUNCTION__, __msg) 
#else
#define debug_error(__msg) \
   _debug_printf("error: %s\n", __msg)
#endif


/**
 * Used by debug_dump_enum and debug_dump_flags to describe symbols.
 */
struct debug_named_value
{
   const char *name;
   unsigned long value;
};


/**
 * Some C pre-processor magic to simplify creating named values.
 * 
 * Example:
 * @code
 * static const debug_named_value my_names[] = {
 *    DEBUG_NAMED_VALUE(MY_ENUM_VALUE_X),
 *    DEBUG_NAMED_VALUE(MY_ENUM_VALUE_Y),
 *    DEBUG_NAMED_VALUE(MY_ENUM_VALUE_Z),
 *    DEBUG_NAMED_VALUE_END
 * };
 * 
 *    ...
 *    debug_printf("%s = %s\n", 
 *                 name,
 *                 debug_dump_enum(my_names, my_value));
 *    ...
 * @endcode
 */
#define DEBUG_NAMED_VALUE(__symbol) {#__symbol, (unsigned long)__symbol} 
#define DEBUG_NAMED_VALUE_END {NULL, 0} 


/**
 * Convert a enum value to a string.
 */
const char *
debug_dump_enum(const struct debug_named_value *names, 
                unsigned long value);


/**
 * Convert binary flags value to a string.
 */
const char *
debug_dump_flags(const struct debug_named_value *names, 
                 unsigned long value);


/**
 * Get option.
 * 
 * It is an alias for getenv on Linux. 
 * 
 * On Windows it reads C:\gallium.cfg, which is a text file with CR+LF line 
 * endings with one option per line as
 *  
 *   NAME=value
 * 
 * This file must be terminated with an extra empty line.
 */
const char *
debug_get_option(const char *name, const char *dfault);

boolean
debug_get_bool_option(const char *name, boolean dfault);

long
debug_get_num_option(const char *name, long dfault);

unsigned long
debug_get_flags_option(const char *name, 
                       const struct debug_named_value *flags,
                       unsigned long dfault);


void *
debug_malloc(const char *file, unsigned line, const char *function,
             size_t size);

void
debug_free(const char *file, unsigned line, const char *function,
           void *ptr);

void *
debug_calloc(const char *file, unsigned line, const char *function,
             size_t count, size_t size );

void *
debug_realloc(const char *file, unsigned line, const char *function,
              void *old_ptr, size_t old_size, size_t new_size );

unsigned long
debug_memory_begin(void);

void 
debug_memory_end(unsigned long beginning);


#if defined(PROFILE) && defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

void
debug_profile_start(void);

void 
debug_profile_stop(void);

#endif


#ifdef DEBUG
struct pipe_surface;
void debug_dump_image(const char *prefix,
                      unsigned format, unsigned cpp,
                      unsigned width, unsigned height,
                      unsigned stride,
                      const void *data);
void debug_dump_surface(const char *prefix,
                        struct pipe_surface *surface);   
void debug_dump_surface_bmp(const char *filename,
                            struct pipe_surface *surface);
#else
#define debug_dump_image(prefix, format, cpp, width, height, stride, data) ((void)0)
#define debug_dump_surface(prefix, surface) ((void)0)
#define debug_dump_surface_bmp(filename, surface) ((void)0)
#endif


#ifdef	__cplusplus
}
#endif

#endif /* P_DEBUG_H_ */
