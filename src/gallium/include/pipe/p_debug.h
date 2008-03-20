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


/**
 * Print debug messages.
 *
 * A debug message will be printed regardless of the DEBUG/NDEBUG macros.
 *
 * The actual channel used to output debug message is platform specific. To 
 * avoid misformating or truncation, follow these rules of thumb:   
 * - output whole lines
 * - avoid outputing large strings (512 bytes is the current maximum length 
 * that is guaranteed to be printed in all platforms)
 */
void debug_printf(const char *format, ...);

/**
 * @sa debug_printf 
 */
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


/**
 * Set a channel's debug mask.
 * 
 * uuid is just a random 32 bit integer that uniquely identifies the debugging 
 * channel. 
 * 
 * @note Due to current implementation issues, make sure the lower 8 bits of 
 * UUID are unique.
 */
void debug_mask_set(uint32_t uuid, uint32_t mask);


uint32_t debug_mask_get(uint32_t uuid);


/**
 * Conditional debug output. 
 * 
 * This is just a generalization of the debug filtering mechanism used 
 * throughout Gallium.
 * 
 * You use this function as:
 * 
 * @code
 * #define MYDRIVER_UUID 0x12345678 // random 32 bit identifier
 * 
 * static void inline 
 * mydriver_debug(uint32_t what, const char *format, ...)
 * {
 * #ifdef DEBUG
 *    va_list ap;
 *    va_start(ap, format);
 *    debug_mask_vprintf(MYDRIVER_UUID, what, format, ap);
 *    va_end(ap);
 * #endif
 * }
 * 
 * ...
 * 
 *    debug_mask_set(MYDRIVER_UUID, 
 *                   MYDRIVER_DEBUG_THIS | 
 *                   MYDRIVER_DEBUG_THAT |
 *                   ... );
 * 
 * ...
 * 
 *    mydriver_debug(MYDRIVER_DEBUG_THIS,
 *                   "this and this happened\n");
 *
 *    mydriver_debug(MYDRIVER_DEBUG_THAT,
 *                   "that = %f\n", that);
 * ...
 * @endcode     
 * 
 * You can also define several variants of mydriver_debug, with hardcoded what. 
 * Note that although macros with variable number of arguments would accomplish 
 * more in less code, they are not portable. 
 */
void debug_mask_vprintf(uint32_t uuid, 
                        uint32_t what, 
                        const char *format, 
                        va_list ap);


#ifdef DEBUG
#define debug_warning(__msg) \
   debug_printf("%s:%i:warning: %s\n", __FILE__, __LINE__, (__msg)) 
#else
#define debug_warning(__msg) \
   ((void)0) 
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


#ifdef	__cplusplus
}
#endif

#endif /* P_DEBUG_H_ */
