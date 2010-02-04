/**************************************************************************
 *
 * Copyright 2008-2010 VMware, Inc.
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

/**
 * @file
 * OS independent time-manipulation functions.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "pipe/p_config.h"

#if !defined(PIPE_OS_EMBEDDED)

#if defined(PIPE_OS_LINUX) || defined(PIPE_OS_BSD) || defined(PIPE_OS_SOLARIS) || defined(PIPE_OS_APPLE) || defined(PIPE_OS_HAIKU)
#  include <sys/time.h> /* timeval */
#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
#  include <windows.h>
#  include <winddi.h>
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
#  include <windows.h>
extern VOID KeQuerySystemTime(PLARGE_INTEGER);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)
#  include <windows.h>
#else
#  error Unsupported OS
#endif

#include "os_time.h"


int64_t
os_time_get(void)
{
#if defined(PIPE_OS_UNIX)

   struct timeval tv;
   gettimeofday(&tv, NULL);
   return tv.tv_usec + tv.tv_sec*1000000LL;

#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

   static LONGLONG frequency;
   LONGLONG counter;
   if(!frequency)
      EngQueryPerformanceFrequency(&frequency);
   EngQueryPerformanceCounter(&counter);
   return counter*INT64_C(1000000)/frequency;

#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER) || defined(PIPE_SUBSYSTEM_WINDOWS_CE)

   static LARGE_INTEGER frequency;
   LARGE_INTEGER counter;
   if(!frequency.QuadPart)
      QueryPerformanceFrequency(&frequency);
   QueryPerformanceCounter(&counter);
   return counter.QuadPart*INT64_C(1000000)/frequency.QuadPart;

#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)

   /* Updated every 10 miliseconds, measured in units of 100 nanoseconds.
    * http://msdn.microsoft.com/en-us/library/ms801642.aspx */
   LARGE_INTEGER counter;
   KeQuerySystemTime(&counter);
   return counter.QuadPart/10;

#endif
}


#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

void
os_time_sleep(int64_t usecs)
{
   static LONGLONG frequency;
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

#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)

void
os_time_sleep(int64_t usecs)
{
   Sleep((usecs + 999) / 1000);
}

#endif


#endif /* !PIPE_OS_EMBEDDED */
