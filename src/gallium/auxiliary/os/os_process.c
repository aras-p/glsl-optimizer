/**************************************************************************
 *
 * Copyright 2013 VMware, Inc.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "pipe/p_config.h"
#include "os/os_process.h"
#include "util/u_memory.h"

#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
#  include <windows.h>
#elif defined(__GLIBC__) || defined(__CYGWIN__)
#  include <errno.h>
#elif defined(PIPE_OS_BSD) || defined(PIPE_OS_APPLE)
#  include <stdlib.h>
#elif defined(PIPE_OS_HAIKU)
#  include <kernel/OS.h>
#  include <kernel/image.h>
#else
#warning unexpected platform in os_process.c
#endif


/**
 * Return the name of the current process.
 * \param procname  returns the process name
 * \param size  size of the procname buffer
 * \return  TRUE or FALSE for success, failure
 */
boolean
os_get_process_name(char *procname, size_t size)
{
   const char *name;
#if defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   char szProcessPath[MAX_PATH];
   char *lpProcessName;
   char *lpProcessExt;

   GetModuleFileNameA(NULL, szProcessPath, Elements(szProcessPath));

   lpProcessName = strrchr(szProcessPath, '\\');
   lpProcessName = lpProcessName ? lpProcessName + 1 : szProcessPath;

   lpProcessExt = strrchr(lpProcessName, '.');
   if (lpProcessExt) {
      *lpProcessExt = '\0';
   }

   name = lpProcessName;

#elif defined(__GLIBC__) || defined(__CYGWIN__)
   name = program_invocation_short_name;
#elif defined(PIPE_OS_BSD) || defined(PIPE_OS_APPLE)
   /* *BSD and OS X */
   name = getprogname();
#elif defined(PIPE_OS_HAIKU)
   image_info info;
   get_image_info(B_CURRENT_TEAM, &info);
   name = info.name;
#else
#warning unexpected platform in os_process.c
   return FALSE;
#endif

   assert(size > 0);
   assert(procname);

   if (name && procname && size > 0) {
      strncpy(procname, name, size);
      procname[size - 1] = '\0';
      return TRUE;
   }
   else {
      return FALSE;
   }
}
