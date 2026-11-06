/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


/*
 * Miscellaneous OS services.
 */


#ifndef _OS_MISC_H_
#define _OS_MISC_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "util/detect.h"
#include "util/u_math.h"


#if DETECT_OS_POSIX
#  include <signal.h> /* for kill() */
#  include <unistd.h> /* getpid() */
#endif


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Trap into the debugger.
 */
#if (DETECT_ARCH_X86 || DETECT_ARCH_X86_64) && DETECT_CC_GCC
#  define os_break() __asm("int3")
#elif DETECT_CC_MSVC
#  define os_break()  __debugbreak()
#elif DETECT_OS_POSIX
#  define os_break() kill(getpid(), SIGTRAP)
#else
#  define os_break() abort()
#endif


/*
 * Abort the program.
 */
#if MESA_DEBUG
#  define os_abort() do { os_break(); abort(); } while(0)
#else
#  define os_abort() abort()
#endif


/*
 * Output a message. Message should preferably end in a newline.
 */
void
os_log_message(const char *message);


/*
 * Get an option(environment variable or os property) in a platform independent way.
 * Should return NULL if specified option for @name is not set.
 * It has the same disadvantage as getenv, see
 * https://wiki.sei.cmu.edu/confluence/display/c/ENV34-C.+Do+not+store+pointers+returned+by+certain+functions
 */
const char *
os_get_option(const char *name);

/*
 * Equivalent to os_get_option except the return value need to be `free()`
 * os_get_option is not safe when the returned value is not immediately used.
 * E.g.
 *  1. when multiple consecutive calls to os_get_option are performed before using the returned values
 *  2. when the value returned by os_get_option is assigned to a struct member
 */
char *
os_get_option_dup(const char *name);

/*
 * Get an option(environment variable or os property) in a platform independent way.
 * Should return NULL if specified option for @name is not set.
 * Same as `os_get_option()` but uses `secure_getenv()` instead of `getenv()`
 */
const char *
os_get_option_secure(const char *name);

/*
 * Equivalent to os_get_option_secure except the return value need to be `free()`
 * os_get_option_secure is not safe when the returned value is not immediately used.
 * E.g.
 *  1. when multiple consecutive calls to os_get_option_secure are performed before using the returned values
 *  2. when the value returned by os_get_option_secure is assigned to a struct member
 */
char *
os_get_option_secure_dup(const char *name);

/*
 * Get an option(environment variable or os property) in a platform independent way.
 * Should return NULL if specified option for @name is not set.
 * It's will save the option into hash table for the first time, and
 * for latter calling, it's will return the value comes from hash table
 * directly, and the returned value will always be valid before program exit
 * The disadvantage is that os_set_option won't take effect after this
 * function is called
 */
const char *
os_get_option_cached(const char *name);

/*
 * Set an option(environment variable or os property) in a platform independent way.
 * If @overwrite is true:
 *   On windows, equivalent to SetEnvironmentVariableA.
 *   On non-windows, when @value is NULL, equivalent to unsetenv, otherwise setenv(override=1).
 * If @overwrite is false:
 *   When name already exist, do nothing, otherwise, equivalent to @overwrite is true
 */
void
os_set_option(const char *name, const char *value, bool override);

/**
 * Unset an option through os_set_option
 */
static inline void
os_unset_option(const char *name)
{
   os_set_option(name, NULL, true);
}

/*
 * Get the total amount of physical memory available on the system.
 */
bool
os_get_total_physical_memory(uint64_t *size);

#define OS_GPU_HEAP_SIZE_HEURISTIC (0.0f)

/*
 * Calculate the gpu heap size based on a percentage of @memory.
 * If @percent is OS_GPU_HEAP_SIZE_HEURISTIC:
 *    Use a heuristic.
 */
static inline uint64_t
os_gpu_heap_size_calculate(uint64_t memory, float percent, float *percent_out)
{
   if (percent == OS_GPU_HEAP_SIZE_HEURISTIC) {
      /* We don't want to burn too much ram with the GPU on devices with a small
       * amount of memory.
       */
      if (memory <= 1ull * 1024ull * 1024ull * 1024ull)
         percent = 0.25f;
      else if (memory <= 4ull * 1024ull * 1024ull * 1024ull)
         percent = 0.5f;
      else
         percent = 0.75f;
   }

   if (percent_out)
      *percent_out = percent;

   return ROUND_DOWN_TO((uint64_t)(memory * percent), 1 << 20);
}

/*
 * Calculate the gpu heap size based on a percentage of the system memory.
 * If @percent is OS_GPU_HEAP_SIZE_HEURISTIC:
 *    Use a heuristic.
 *
 * @percent_out is preserved on failure.
 */
static inline uint64_t
os_get_gpu_heap_size(float percent, float *percent_out)
{
   uint64_t memory;

   const bool success = os_get_total_physical_memory(&memory);
   if (!success)
      return 0;

   return os_gpu_heap_size_calculate(memory, percent, percent_out);
}

/*
 * Amount of physical memory available to a process
 */
bool
os_get_available_system_memory(uint64_t *size);

/*
 * Size of a page
 */
bool
os_get_page_size(uint64_t *size);

/*
 * Whether this process is permitted to create JIT (writable and executable)
 * mappings.  Always true except on macOS, where a hardened process without the
 * com.apple.security.cs.allow-jit entitlement is denied them.
 */
bool
os_jit_allowed(void);


#ifdef __cplusplus
}
#endif


#endif /* _OS_MISC_H_ */
