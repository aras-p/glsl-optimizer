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


#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#include <winddi.h>
#endif

#include "util/u_time.h"


#ifdef WIN32
static LONGLONG frequency = 0;
#endif


void 
util_time_get(struct util_time *t)
{
#ifndef WIN32
   gettimeofday(&t->tv, NULL);
#else
   EngQueryPerformanceCounter(&t->counter);
#endif
}


void 
util_time_add(const struct util_time *t1,
              int64_t usecs,
              struct util_time *t2)
{
#ifndef WIN32
   t2->tv.tv_sec = t1->tv.tv_sec + usecs / 1000000;
   t2->tv.tv_usec = t1->tv.tv_usec + usecs % 1000000;
#else
   if(!frequency)
      EngQueryPerformanceFrequency(&frequency);
   t2->counter = t1->counter + (usecs * frequency + 999999LL)/1000000LL;
#endif
}


int64_t
util_time_diff(const struct util_time *t1, 
               const struct util_time *t2)
{
#ifndef WIN32
   return (t2->tv.tv_usec - t1->tv.tv_usec) + 
          (t2->tv.tv_sec - t1->tv.tv_sec)*1000000;
#else
   return (t2->counter - t1->counter)*1000000LL/frequency;
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
#ifndef WIN32
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
#else
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


#ifdef WIN32
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
