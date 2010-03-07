/**************************************************************************
 *
 * Copyright 2008-2010 Vmware, Inc.
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


#include "os_misc.h"

#include <stdarg.h>


#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY

#include <windows.h>
#include <winddi.h>

#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE)

#include <stdio.h> 
#include <stdlib.h> 
#include <windows.h> 
#include <types.h> 

#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN      // Exclude rarely-used stuff from Windows headers
#endif
#include <windows.h>
#include <stdio.h>

#else

#include <stdio.h>
#include <stdlib.h>

#endif


#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY
static INLINE void 
_EngDebugPrint(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   EngDebugPrint("", (PCHAR)format, ap);
   va_end(ap);
}
#endif


void
os_log_message(const char *message)
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   _EngDebugPrint("%s", message);
#elif defined(PIPE_SUBSYSTEM_WINDOWS_USER)
   OutputDebugStringA(message);
   if(GetConsoleWindow() && !IsDebuggerPresent()) {
      fflush(stdout);
      fputs(message, stderr);
      fflush(stderr);
   }
#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE)
   wchar_t *wide_format;
   long wide_str_len;   
   /* Format is ascii - needs to be converted to wchar_t for printing */   
   wide_str_len = MultiByteToWideChar(CP_ACP, 0, message, -1, NULL, 0);
   wide_format = (wchar_t *) malloc((wide_str_len+1) * sizeof(wchar_t));   
   if (wide_format) {   
      MultiByteToWideChar(CP_ACP, 0, message, -1,
            wide_format, wide_str_len);   
      NKDbgPrintfW(wide_format, wide_format);   
      free(wide_format);   
   } 
#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)
   /* TODO */
#else /* !PIPE_SUBSYSTEM_WINDOWS */
   fflush(stdout);
   fputs(message, stderr);
#endif
}


#ifdef PIPE_SUBSYSTEM_WINDOWS_DISPLAY
static const char *
find(const char *start, const char *end, char c)
{
   const char *p;
   for(p = start; !end || p != end; ++p) {
      if(*p == c)
         return p;
      if(*p < 32)
         break;
   }
   return NULL;
}

static int
compare(const char *start, const char *end, const char *s)
{
   const char *p, *q;
   for(p = start, q = s; p != end && *q != '\0'; ++p, ++q) {
      if(*p != *q)
         return 0;
   }
   return p == end && *q == '\0';
}

static void
copy(char *dst, const char *start, const char *end, size_t n)
{
   const char *p;
   char *q;
   for(p = start, q = dst, n = n - 1; p != end && n; ++p, ++q, --n)
      *q = *p;
   *q = '\0';
}
#endif


const char *
os_get_option(const char *name)
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)
   /* EngMapFile creates the file if it does not exists, so it must either be
    * disabled on release versions (or put in a less conspicuous place). */
#ifdef DEBUG
   const char *result = NULL;
   ULONG_PTR iFile = 0;
   const void *pMap = NULL;
   const char *sol, *eol, *sep;
   static char output[1024];
   
   pMap = EngMapFile(L"\\??\\c:\\gallium.cfg", 0, &iFile);
   if(pMap) {
      sol = (const char *)pMap;
      while(1) {
	 /* TODO: handle LF line endings */
	 eol = find(sol, NULL, '\r');
	 if(!eol || eol == sol)
	    break;
	 sep = find(sol, eol, '=');
	 if(!sep)
	    break;
	 if(compare(sol, sep, name)) {
	    copy(output, sep + 1, eol, sizeof(output));
	    result = output;
	    break;
	 }
	 sol = eol + 2;
      }
      EngUnmapFile(iFile);
   }
   return result;
#else
   return NULL;
#endif
#elif defined(PIPE_SUBSYSTEM_WINDOWS_CE) || defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT) 
   /* TODO: implement */
   return NULL;
#else
   return getenv(name);
#endif
}

