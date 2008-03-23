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
 * Memory debugging.
 * 
 * @author José Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifdef WIN32
#include <windows.h>
#include <winddi.h>
#else
#include <stdio.h>
#include <stdlib.h>
#endif

#include "pipe/p_debug.h" 
#include "util/u_double_list.h" 


#define DEBUG_MEMORY_MAGIC 0x6e34090aU 


#if defined(WIN32) && !defined(WINCE)
#define real_malloc(_size) EngAllocMem(0, _size, 'D3AG')
#define real_free(_ptr) EngFreeMem(_ptr)
#else
#define real_malloc(_size) malloc(_size)
#define real_free(_ptr) free(_ptr)
#endif


struct debug_memory_header 
{
   struct list_head head;
   
   unsigned long no;
   const char *file;
   unsigned line;
   const char *function;
   size_t size;
   unsigned magic;
};

static struct list_head list = { &list, &list };

static unsigned long start_no = 0;
static unsigned long end_no = 0;


void *
debug_malloc(const char *file, unsigned line, const char *function,
             size_t size) 
{
   struct debug_memory_header *hdr;
   
   hdr = real_malloc(sizeof(*hdr) + size);
   if(!hdr)
      return NULL;
 
   hdr->no = end_no++;
   hdr->file = file;
   hdr->line = line;
   hdr->function = function;
   hdr->size = size;
   hdr->magic = DEBUG_MEMORY_MAGIC;

   LIST_ADDTAIL(&hdr->head, &list);
   
   return (void *)((char *)hdr + sizeof(*hdr));
}

void
debug_free(const char *file, unsigned line, const char *function,
           void *ptr) 
{
   struct debug_memory_header *hdr;
   
   if(!ptr)
      return;
   
   hdr = (struct debug_memory_header *)((char *)ptr - sizeof(*hdr));
   if(hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: freeing bad or corrupted memory %p\n",
                   file, line, function,
                   ptr);
      debug_assert(0);
      return;
   }

   LIST_DEL(&hdr->head);
   hdr->magic = 0;
   
   real_free(hdr);
}

void *
debug_calloc(const char *file, unsigned line, const char *function,
             size_t count, size_t size )
{
   void *ptr = debug_malloc( file, line, function, count * size );
   if( ptr )
      memset( ptr, 0, count * size );
   return ptr;
}

void *
debug_realloc(const char *file, unsigned line, const char *function,
              void *old_ptr, size_t old_size, size_t new_size )
{
   void *new_ptr = NULL;

   if (new_size != 0) {
      new_ptr = debug_malloc( file, line, function, new_size );
      
      if( new_ptr && old_ptr )
         memcpy( new_ptr, old_ptr, old_size );
   }

   debug_free( file, line, function, old_ptr );
   return new_ptr;
}

void
debug_memory_reset(void)
{
   start_no = end_no;
}

void 
debug_memory_report(void)
{
   struct list_head *entry;

   entry = list.prev;
   for (; entry != &list; entry = entry->prev) {
      struct debug_memory_header *hdr;
      void *ptr;
      hdr = LIST_ENTRY(struct debug_memory_header, entry, head);
      ptr = (void *)((char *)hdr + sizeof(*hdr));
      if(hdr->no >= start_no)
	 debug_printf("%s:%u:%s: %u bytes at %p not freed\n",
		      hdr->file, hdr->line, hdr->function,
		      hdr->size, ptr);
   }
}
