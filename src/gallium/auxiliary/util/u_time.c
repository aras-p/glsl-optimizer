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


#include "pipe/p_config.h"

#if defined(PIPE_OS_LINUX)
#include <sys/time.h>
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
#include <windows.h>
#include <winddi.h>
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
#include <windows.h>
extern VOID KeQuerySystemTime(PLARGE_INTEGER);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
#include <windows.h>
#else
#error Unsupported OS
#endif

#include "util/u_time.h"


#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) || defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)

static int64_t frequency = 0;

static INLINE void 
util_time_get_frequency(void)
{
   if(!frequency) {
#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
      LONGLONG temp;
      EngQueryPerformanceFrequency(&temp);
      frequency = temp;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
      LARGE_INTEGER temp;
      QueryPerformanceFrequency(&temp);
      frequency = temp.QuadPart;
#endif
   }
}
#endif


void 
util_time_get(struct util_time *t)
{
#if defined(PIPE_OS_LINUX)
   gettimeofday(&t->tv, NULL);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   LONGLONG temp;
   EngQueryPerformanceCounter(&temp);
   t->counter = temp;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
   /* Updated every 10 miliseconds, measured in units of 100 nanoseconds.
    * http://msdn.microsoft.com/en-us/library/ms801642.aspx */
   LARGE_INTEGER temp;
   KeQuerySystemTime(&temp);
   t->counter = temp.QuadPart;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   LARGE_INTEGER temp;
   QueryPerformanceCounter(&temp);
   t->counter = temp.QuadPart;
#endif
}


void 
util_time_add(const struct util_time *t1,
              int64_t usecs,
              struct util_time *t2)
{
#if defined(PIPE_OS_LINUX)
   t2->tv.tv_sec = t1->tv.tv_sec + usecs / 1000000;
   t2->tv.tv_usec = t1->tv.tv_usec + usecs % 1000000;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) || defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   util_time_get_frequency();
   t2->counter = t1->counter + (usecs * frequency + INT64_C(999999))/INT64_C(1000000);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
   /* 1 tick = 100 nano seconds. */
   t2->counter = t1->counter + usecs * 10;
#elif 
   LARGE_INTEGER temp;
   LONGLONG freq;
   freq = temp.QuadPart;
   t2->counter = t1->counter + (usecs * freq)/1000000L;
#endif
}


int64_t
util_time_diff(const struct util_time *t1, 
               const struct util_time *t2)
{
#if defined(PIPE_OS_LINUX)
   return (t2->tv.tv_usec - t1->tv.tv_usec) + 
          (t2->tv.tv_sec - t1->tv.tv_sec)*1000000;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) || defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   util_time_get_frequency();
   return (t2->counter - t1->counter)*INT64_C(1000000)/frequency;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
   return (t2->counter - t1->counter)/10;
#endif
}



uint64_t
util_time_micros( void )
{
   struct util_time t1;
   
   util_time_get(&t1);
   
#if defined(PIPE_OS_LINUX)
   return t1.tv.tv_usec + t1.tv.tv_sec*1000000LL;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) || defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   util_time_get_frequency();
   return t1.counter*INT64_C(1000000)/frequency;
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
   return t1.counter/10;
#endif
}



/**
 * Compare two time values.
 * 
 * Not publicly available because it does not take in account wrap-arounds. 
 * Use util_time_timeout instead.
 */
static INLINE int
util_time_compare(const struct util_time *t1, 
                  const struct util_time *t2)
{
#if defined(PIPE_OS_LINUX)
   if (t1->tv.tv_sec < t2->tv.tv_sec)
      return -1;
   else if(t1->tv.tv_sec > t2->tv.tv_sec)
      return 1;
   else if (t1->tv.tv_usec < t2->tv.tv_usec)
      return -1;
   else if(t1->tv.tv_usec > t2->tv.tv_usec)
      return 1;
   else 
      return 0;
#elif defined(PIPE_OS_WINDOWS)
   if (t1->counter < t2->counter)
      return -1;
   else if(t1->counter > t2->counter)
      return 1;
   else 
      return 0;
#endif
}


boolean 
util_time_timeout(const struct util_time *start, 
                  const struct util_time *end,
                  const struct util_time *curr) 
{
   if(util_time_compare(start, end) <= 0)
      return !(util_time_compare(start, curr) <= 0 && util_time_compare(curr, end) < 0);
   else
      return !(util_time_compare(start, curr) <= 0 || util_time_compare(curr, end) < 0);
}


#if defined(PIPE_SUBSYSYEM_WINDOWS_DISPLAY)
void util_time_sleep(unsigned usecs)
{
   LONGLONG start, curr, end;
   
   EngQueryPerformanceCounter(&start);
   
   if(!frequency)
      EngQueryPerformanceFrequency(&frequency);
   
   end = start + (usecs * frequency + 999999LL)/1000000LL;
   
   do {
      EngQueryPerformanceCounter(&curr);
   } while(start <= curr && curr < end || 
	   end < start && (curr < end || start <= curr));
}
#endif
