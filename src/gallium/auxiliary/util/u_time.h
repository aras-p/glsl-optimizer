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
 * OS independent time-manipulation functions.
 * 
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef U_TIME_H_
#define U_TIME_H_


#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE)
#include <time.h> /* timeval */
#include <unistd.h> /* usleep */
#endif

#if defined(PIPE_OS_HAIKU)
#include <sys/time.h> /* timeval */
#include <unistd.h>
#endif

#include "pipe/p_compiler.h"


#ifdef	__cplusplus
extern "C" {
#endif


/**
 * Time abstraction.
 * 
 * Do not access this structure directly. Use the provided function instead.
 */
struct util_time 
{
#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
   struct timeval tv;
#else
   int64_t counter;
#endif
};
   

void 
util_time_get(struct util_time *t);

/**
 * Return t2 = t1 + usecs
 */
void 
util_time_add(const struct util_time *t1,
              int64_t usecs,
              struct util_time *t2);

/**
 * Return current time in microseconds
 */
uint64_t
util_time_micros( void );

/**
 * Return difference between times, in microseconds
 */
int64_t
util_time_diff(const struct util_time *t1, 
               const struct util_time *t2);

/**
 * Returns non-zero when the timeout expires.
 */
boolean 
util_time_timeout(const struct util_time *start, 
                  const struct util_time *end,
                  const struct util_time *curr);

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
#define util_time_sleep usleep
#else
void
util_time_sleep(unsigned usecs);
#endif


#ifdef	__cplusplus
}
#endif

#endif /* U_TIME_H_ */
