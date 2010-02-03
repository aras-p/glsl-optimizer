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


/*
 * OS memory management abstractions for Windows kernel.
 */


#ifndef _OS_MEMORY_H_
#error "Must not be included directly. Include os_memory.h instead"
#endif


#include "pipe/p_compiler.h"


#ifdef __cplusplus
extern "C" {
#endif


#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

void * __stdcall
EngAllocMem(unsigned long Flags,
            unsigned long MemSize,
            unsigned long Tag);

void __stdcall
EngFreeMem(void *Mem);

#define os_malloc(_size) EngAllocMem(0, _size, 'D3AG')
#define os_calloc(_count, _size) EngAllocMem(1, (_count)*(_size), 'D3AG')
#define _os_free(_ptr) EngFreeMem(_ptr)

#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)

void *
ExAllocatePool(unsigned long PoolType,
               size_t NumberOfBytes);

void 
ExFreePool(void *P);

#define os_malloc(_size) ExAllocatePool(0, _size)
#define _os_free(_ptr) ExFreePool(_ptr)

static INLINE void *
os_calloc(unsigned count, unsigned size)
{
   void *ptr = os_malloc(count * size);
   if (ptr) {
      memset(ptr, 0, count * size);
   }
   return ptr;
}

#else

#error "Unsupported subsystem"

#endif


static INLINE void
os_free( void *ptr )
{
   if (ptr) {
      _os_free(ptr);
   }
}


static INLINE void *
os_realloc(void *old_ptr, unsigned old_size, unsigned new_size)
{
   void *new_ptr = NULL;

   if (new_size != 0) {
      unsigned copy_size = old_size < new_size ? old_size : new_size;
      new_ptr = os_malloc( new_size );
      if (new_ptr && old_ptr && copy_size) {
         memcpy(new_ptr, old_ptr, copy_size);
      }
   }

   os_free(old_ptr);

   return new_ptr;
}


#ifdef __cplusplus
}
#endif


#include "os_memory_aligned.h"
