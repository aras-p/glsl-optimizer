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

#include "u_debug.h"
#include "u_debug_symbol.h"

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

typedef BOOL (WINAPI *PFNSYMCLEANUP)(HANDLE);
static PFNSYMCLEANUP pfnSymCleanup = NULL;

static
BOOL WINAPI j_SymCleanup(HANDLE hProcess)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymCleanup || (pfnSymCleanup = (PFNSYMCLEANUP) GetProcAddress(hModule_Imagehlp, "SymCleanup")))
   )
      return pfnSymCleanup(hProcess);
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

typedef BOOL (WINAPI *PFNSYMUNDNAME)(PIMAGEHLP_SYMBOL, PSTR, DWORD);
static PFNSYMUNDNAME pfnSymUnDName = NULL;

static
BOOL WINAPI j_SymUnDName(PIMAGEHLP_SYMBOL Symbol, PSTR UnDecName, DWORD UnDecNameLength)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymUnDName || (pfnSymUnDName = (PFNSYMUNDNAME) GetProcAddress(hModule_Imagehlp, "SymUnDName")))
   )
      return pfnSymUnDName(Symbol, UnDecName, UnDecNameLength);
   else
      return FALSE;
}

typedef PFUNCTION_TABLE_ACCESS_ROUTINE PFNSYMFUNCTIONTABLEACCESS;
static PFNSYMFUNCTIONTABLEACCESS pfnSymFunctionTableAccess = NULL;

static
PVOID WINAPI j_SymFunctionTableAccess(HANDLE hProcess, DWORD AddrBase)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymFunctionTableAccess || (pfnSymFunctionTableAccess = (PFNSYMFUNCTIONTABLEACCESS) GetProcAddress(hModule_Imagehlp, "SymFunctionTableAccess")))
   )
      return pfnSymFunctionTableAccess(hProcess, AddrBase);
   else
      return NULL;
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

typedef BOOL (WINAPI *PFNSTACKWALK)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, LPVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE);
static PFNSTACKWALK pfnStackWalk = NULL;

static
BOOL WINAPI j_StackWalk(
   DWORD MachineType, 
   HANDLE hProcess, 
   HANDLE hThread, 
   LPSTACKFRAME StackFrame, 
   PVOID ContextRecord, 
   PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine,  
   PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine,
   PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, 
   PTRANSLATE_ADDRESS_ROUTINE TranslateAddress 
)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnStackWalk || (pfnStackWalk = (PFNSTACKWALK) GetProcAddress(hModule_Imagehlp, "StackWalk")))
   )
      return pfnStackWalk(
         MachineType, 
         hProcess, 
         hThread, 
         StackFrame, 
         ContextRecord, 
         ReadMemoryRoutine,  
         FunctionTableAccessRoutine,
         GetModuleBaseRoutine, 
         TranslateAddress 
      );
   else
      return FALSE;
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

typedef BOOL (WINAPI *PFNSYMGETLINEFROMADDR)(HANDLE, DWORD, LPDWORD, PIMAGEHLP_LINE);
static PFNSYMGETLINEFROMADDR pfnSymGetLineFromAddr = NULL;

static
BOOL WINAPI j_SymGetLineFromAddr(HANDLE hProcess, DWORD dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE Line)
{
   if(
      (hModule_Imagehlp || (hModule_Imagehlp = LoadLibraryA("IMAGEHLP.DLL"))) &&
      (pfnSymGetLineFromAddr || (pfnSymGetLineFromAddr = (PFNSYMGETLINEFROMADDR) GetProcAddress(hModule_Imagehlp, "SymGetLineFromAddr")))
   )
      return pfnSymGetLineFromAddr(hProcess, dwAddr, pdwDisplacement, Line);
   else
      return FALSE;
}


static INLINE boolean
debug_symbol_print_imagehlp(const void *addr)
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
      return FALSE;

   debug_printf("\t%s\n", pSymbol->Name);

   return TRUE;
   
}
#endif


void
debug_symbol_print(const void *addr)
{
#if defined(PIPE_SUBSYSTEM_WINDOWS_USER) && defined(PIPE_ARCH_X86)
   if(debug_symbol_print_imagehlp(addr))
      return;
#endif
   
   debug_printf("\t%p\n", addr);
}
