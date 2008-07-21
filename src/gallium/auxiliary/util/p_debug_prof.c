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
 * Poor-man profiling.
 * 
 * @author José Fonseca <jrfonseca@tungstengraphics.com>
 * 
 * @sa http://blogs.msdn.com/joshpoley/archive/2008/03/12/poor-man-s-profiler.aspx
 * @sa http://www.johnpanzer.com/aci_cuj/index.html
 */

#include "pipe/p_config.h" 

#if defined(PROFILE) && defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

#include <windows.h>
#include <winddi.h>

#include "pipe/p_debug.h" 
#include "util/u_string.h" 


#define PROFILE_FILE_SIZE 4*1024*1024
#define FILE_NAME_SIZE 256

static WCHAR wFileName[FILE_NAME_SIZE] = L"\\??\\c:\\00000000.trace";
static ULONG_PTR iFile = 0;
static BYTE *pMap = NULL;
static BYTE *pMapBegin = NULL;
static BYTE *pMapEnd = NULL;


void __declspec(naked) __cdecl 
debug_profile_close(void)
{
   _asm {
      push eax
      push ebx
      push ecx
      push edx
      push ebp
      push edi
      push esi
    }

   if(iFile) {
      EngUnmapFile(iFile);
      /* Truncate the file */
      pMap = EngMapFile(wFileName, pMap - pMapBegin, &iFile);
      if(pMap)
         EngUnmapFile(iFile);
   }
   iFile = 0;
   pMapBegin = pMapEnd = pMap = NULL;
   
   _asm {
      pop esi
      pop edi
      pop ebp
      pop edx
      pop ecx
      pop ebx
      pop eax
      ret
    }
}


void __declspec(naked) __cdecl 
debug_profile_open(void)
{
   WCHAR *p;
   
   _asm {
      push eax
      push ebx
      push ecx
      push edx
      push ebp
      push edi
      push esi
    }

   debug_profile_close();
   
   // increment starting from the less significant digit
   p = &wFileName[14];
   while(1) {
      if(*p == '9') {
         *p-- = '0';
      }
      else {
         *p += 1;
         break;
      }
   }

   pMap = EngMapFile(wFileName, PROFILE_FILE_SIZE, &iFile);
   if(pMap) {
      pMapBegin = pMap;
      pMapEnd = pMap + PROFILE_FILE_SIZE;
   }
   
   _asm {
      pop esi
      pop edi
      pop ebp
      pop edx
      pop ecx
      pop ebx
      pop eax
      ret
    }
}
   
   
/**
 * Called at the start of every method or function.
 * 
 * @sa http://msdn.microsoft.com/en-us/library/c63a9b7h.aspx
 */
void __declspec(naked) __cdecl 
_penter(void) {
   _asm {
      push ebx
retry:
      mov ebx, [pMap]
      test ebx, ebx
      jz done
      cmp ebx, [pMapEnd]
      jne ready
      call debug_profile_open
      jmp retry
ready:
      push eax
      push edx
      mov eax, [esp+12]
      and eax, 0xfffffffe
      mov [ebx], eax
      add ebx, 4
      rdtsc
      mov [ebx], eax
      add ebx, 4
      mov [pMap], ebx
      pop edx
      pop eax
done:
      pop ebx
      ret  
   }
}


/**
 * Called at the end of Calls the end of every method or function.
 * 
 * @sa http://msdn.microsoft.com/en-us/library/xc11y76y.aspx
 */
void __declspec(naked) __cdecl 
_pexit(void) {
   _asm {
      push ebx
retry:
      mov ebx, [pMap]
      test ebx, ebx
      jz done
      cmp ebx, [pMapEnd]
      jne ready
      call debug_profile_open
      jmp retry
ready:
      push eax
      push edx
      mov eax, [esp+12]
      or eax, 0x00000001
      mov [ebx], eax
      add ebx, 4
      rdtsc
      mov [ebx], eax
      add ebx, 4
      mov [pMap], ebx
      pop edx
      pop eax
done:
      pop ebx
      ret
   }
}


/**
 * Reference function for calibration. 
 */
void __declspec(naked) 
__debug_profile_reference(void) {
   _asm {
      call _penter
      call _pexit
      ret
   }
}


void
debug_profile_start(void)
{
   debug_profile_open();
   
   if(pMap) {
      unsigned i;
      for(i = 0; i < 8; ++i) {
         _asm {
            call _penter
            call __debug_profile_reference
            call _pexit
         }
      }
   }
}


void 
debug_profile_stop(void)
{
   debug_profile_close();
}

#endif /* PROFILE */
