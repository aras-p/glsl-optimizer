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


#define PROFILE_TABLE_SIZE (1024*1024)
#define FILE_NAME_SIZE 256

struct debug_profile_entry
{
   uintptr_t caller;
   uintptr_t callee;
   uint64_t samples;
};

static unsigned long enabled = 0;

static WCHAR wFileName[FILE_NAME_SIZE] = L"\\??\\c:\\00000000.prof";
static ULONG_PTR iFile = 0;

static struct debug_profile_entry *table = NULL;
static unsigned long free_table_entries = 0;
static unsigned long max_table_entries = 0;

uint64_t start_stamp = 0;
uint64_t end_stamp = 0;


static void
debug_profile_entry(uintptr_t caller, uintptr_t callee, uint64_t samples)
{
   unsigned hash = ( caller + callee ) & PROFILE_TABLE_SIZE - 1;
   
   while(1) {
      if(table[hash].caller == 0 && table[hash].callee == 0) {
         table[hash].caller = caller;
         table[hash].callee = callee;
         table[hash].samples = samples;
         --free_table_entries;
         break;
      }
      else if(table[hash].caller == caller && table[hash].callee == callee) {
         table[hash].samples += samples;
         break;
      }
      else {
         ++hash;
      }
   }
}


static uintptr_t caller_stack[1024];
static unsigned last_caller = 0;


static int64_t delta(void) {
   int64_t result = end_stamp - start_stamp;
   if(result > UINT64_C(0xffffffff))
      result = 0;
   return result;
}


static void __cdecl 
debug_profile_enter(uintptr_t callee)
{
   uintptr_t caller = last_caller ? caller_stack[last_caller - 1] : 0;
                
   if (caller)
      debug_profile_entry(caller, 0, delta());
   debug_profile_entry(caller, callee, 1);
   caller_stack[last_caller++] = callee;
}


static void __cdecl
debug_profile_exit(uintptr_t callee)
{
   debug_profile_entry(callee, 0, delta());
   if(last_caller)
      --last_caller;
}
   
   
/**
 * Called at the start of every method or function.
 * 
 * @sa http://msdn.microsoft.com/en-us/library/c63a9b7h.aspx
 */
void __declspec(naked) __cdecl 
_penter(void) {
   _asm {
      push eax
      mov eax, [enabled]
      test eax, eax
      jz skip

      push edx
      
      rdtsc
      mov dword ptr [end_stamp], eax
      mov dword ptr [end_stamp+4], edx

      xor eax, eax
      mov [enabled], eax

      mov eax, [esp+8]

      push ebx
      push ecx
      push ebp
      push edi
      push esi

      push eax
      call debug_profile_enter
      add esp, 4

      pop esi
      pop edi
      pop ebp
      pop ecx
      pop ebx

      mov eax, 1
      mov [enabled], eax 

      rdtsc
      mov dword ptr [start_stamp], eax
      mov dword ptr [start_stamp+4], edx
      
      pop edx
skip:
      pop eax
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
      push eax
      mov eax, [enabled]
      test eax, eax
      jz skip

      push edx
      
      rdtsc
      mov dword ptr [end_stamp], eax
      mov dword ptr [end_stamp+4], edx

      xor eax, eax
      mov [enabled], eax

      mov eax, [esp+8]

      push ebx
      push ecx
      push ebp
      push edi
      push esi

      push eax
      call debug_profile_exit
      add esp, 4

      pop esi
      pop edi
      pop ebp
      pop ecx
      pop ebx

      mov eax, 1
      mov [enabled], eax 

      rdtsc
      mov dword ptr [start_stamp], eax
      mov dword ptr [start_stamp+4], edx
      
      pop edx
skip:
      pop eax
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
   WCHAR *p;

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

   table = EngMapFile(wFileName, 
                      PROFILE_TABLE_SIZE*sizeof(struct debug_profile_entry), 
                      &iFile);
   if(table) {
      unsigned i;
      
      free_table_entries = max_table_entries = PROFILE_TABLE_SIZE;
      memset(table, 0, PROFILE_TABLE_SIZE*sizeof(struct debug_profile_entry));
      
      table[0].caller = (uintptr_t)&__debug_profile_reference;
      table[0].callee = 0;
      table[0].samples = 0;
      --free_table_entries;

      _asm {
         push edx
         push eax
      
         rdtsc
         mov dword ptr [start_stamp], eax
         mov dword ptr [start_stamp+4], edx
         
         pop edx
         pop eax
      }

      last_caller = 0;
      
      enabled = 1;

      for(i = 0; i < 8; ++i) {
         _asm {
            call __debug_profile_reference
         }
      }
   }
}


void 
debug_profile_stop(void)
{
   enabled = 0;

   if(iFile)
      EngUnmapFile(iFile);
   iFile = 0;
   table = NULL;
   free_table_entries = max_table_entries = 0;
}

#endif /* PROFILE */
