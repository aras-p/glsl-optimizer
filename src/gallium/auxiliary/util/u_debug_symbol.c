/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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
 * Symbol lookup.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_compiler.h"
#include "os/os_thread.h"
#include "u_string.h"

#include "u_debug.h"
#include "u_debug_symbol.h"
#include "u_hash_table.h"

#if defined(PIPE_SUBSYSTEM_WINDOWS_USER) && defined(PIPE_ARCH_X86)
   
#include <windows.h>
#include <stddef.h>
#include <imagehlp.h>

/*
 * TODO: Cleanup code.
 * TODO: Support x86_64 
 */

static BOOL bSymInitialized = FALSE;

static HMODULE hModule_Imagehlp = NULL;

typedef BOOL (WINAPI *PFNSYMINITIALIZE)(HANDLE, LPSTR, BOOL);
static PFNSYMINITIALIZE pfnSymInitialize = NULL;

static
BOOL WINAPI j_SymInitialize(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymInitialize || (pfnSymInitialize = (PFNSYMINITIALIZE) GetProcAddress(hModule_Imagehlp, "SymInitialize")))
   )
      return pfnSymInitialize(hProcess, UserSearchPath, fInvadeProcess);
   else
      return FALSE;
}

typedef DWORD (WINAPI *PFNSYMSETOPTIONS)(DWORD);
static PFNSYMSETOPTIONS pfnSymSetOptions = NULL;

static
DWORD WINAPI j_SymSetOptions(DWORD SymOptions)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymSetOptions || (pfnSymSetOptions = (PFNSYMSETOPTIONS) GetProcAddress(hModule_Imagehlp, "SymSetOptions")))
   )
      return pfnSymSetOptions(SymOptions);
   else
      return FALSE;
}

typedef PGET_MODULE_BASE_ROUTINE PFNSYMGETMODULEBASE;
static PFNSYMGETMODULEBASE pfnSymGetModuleBase = NULL;

static
DWORD WINAPI j_SymGetModuleBase(HANDLE hProcess, DWORD dwAddr)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymGetModuleBase || (pfnSymGetModuleBase = (PFNSYMGETMODULEBASE) GetProcAddress(hModule_Imagehlp, "SymGetModuleBase")))
   )
      return pfnSymGetModuleBase(hProcess, dwAddr);
   else
      return 0;
}

typedef BOOL (WINAPI *PFNSYMGETSYMFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_SYMBOL);
static PFNSYMGETSYMFROMADDR pfnSymGetSymFromAddr = NULL;

static
BOOL WINAPI j_SymGetSymFromAddr(HANDLE hProcess, DWORD Address, PDWORD Displacement, PIMAGEHLP_SYMBOL Symbol)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymGetSymFromAddr || (pfnSymGetSymFromAddr = (PFNSYMGETSYMFROMADDR) GetProcAddress(hModule_Imagehlp, "SymGetSymFromAddr")))
   )
      return pfnSymGetSymFromAddr(hProcess, Address, Displacement, Symbol);
   else
      return FALSE;
}


static INLINE void
debug_symbol_name_imagehlp(const void *addr, char* buf, unsigned size)
{
   HANDLE hProcess;
   BYTE symbolBuffer[1024];
   PIMAGEHLP_SYMBOL pSymbol = (PIMAGEHLP_SYMBOL) symbolBuffer;
   DWORD dwDisplacement = 0;  /* Displacement of the input address, relative to the start of the symbol */

   hProcess = GetCurrentProcess();

   pSymbol->SizeOfStruct = sizeof(symbolBuffer);
   pSymbol->MaxNameLength = sizeof(symbolBuffer) - offsetof(IMAGEHLP_SYMBOL, Name);

   if(!bSymInitialized) {
      j_SymSetOptions(/* SYMOPT_UNDNAME | */ SYMOPT_LOAD_LINES);
      if(j_SymInitialize(hProcess, NULL, TRUE))
         bSymInitialized = TRUE;
   }

   if(!j_SymGetSymFromAddr(hProcess, (DWORD)addr, &dwDisplacement, pSymbol))
      buf[0] = 0;
   else
   {
      strncpy(buf, pSymbol->Name, size);
      buf[size - 1] = 0;
   }
}
#endif

#ifdef __GLIBC__
#include <execinfo.h>

/* This can only provide dynamic symbols, or binary offsets into a file.
 *
 * To fix this, post-process the output with tools/addr2line.sh
 */
static INLINE void
debug_symbol_name_glibc(const void *addr, char* buf, unsigned size)
{
   char** syms = backtrace_symbols((void**)&addr, 1);
   strncpy(buf, syms[0], size);
   buf[size - 1] = 0;
   free(syms);
}
#endif

void
debug_symbol_name(const void *addr, char* buf, unsigned size)
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_USER) && defined(PIPE_ARCH_X86)
   debug_symbol_name_imagehlp(addr, buf, size);
   if(buf[0])
      return;
#endif

#ifdef __GLIBC__
   debug_symbol_name_glibc(addr, buf, size);
   if(buf[0])
      return;
#endif

   util_snprintf(buf, size, "%p", addr);
   buf[size - 1] = 0;
}

void
debug_symbol_print(const void *addr)
{
   char buf[1024];
   debug_symbol_name(addr, buf, sizeof(buf));
   debug_printf("\t%s\n", buf);
}

struct util_hash_table* symbols_hash;
pipe_mutex symbols_mutex;

static unsigned hash_ptr(void* p)
{
   return (unsigned)(uintptr_t)p;
}

static int compare_ptr(void* a, void* b)
{
   if(a == b)
      return 0;
   else if(a < b)
      return -1;
   else
      return 1;
}

const char*
debug_symbol_name_cached(const void *addr)
{
   const char* name;
   pipe_mutex_lock(symbols_mutex);
   if(!symbols_hash)
      symbols_hash = util_hash_table_create(hash_ptr, compare_ptr);
   name = util_hash_table_get(symbols_hash, (void*)addr);
   if(!name)
   {
      char buf[1024];
      debug_symbol_name(addr, buf, sizeof(buf));
      name = strdup(buf);

      util_hash_table_set(symbols_hash, (void*)addr, (void*)name);
   }
   pipe_mutex_unlock(symbols_mutex);
   return name;
}
