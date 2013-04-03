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

#include "pipe/p_config.h" 

#define DEBUG_MEMORY_IMPLEMENTATION

#include "os/os_memory.h"
#include "os/os_memory_debug.h"
#include "os/os_thread.h"

#include "util/u_debug.h" 
#include "util/u_debug_stack.h" 
#include "util/u_double_list.h" 


#define DEBUG_MEMORY_MAGIC 0x6e34090aU 
#define DEBUG_MEMORY_STACK 0 /* XXX: disabled until we have symbol lookup */

/**
 * Set to 1 to enable checking of freed blocks of memory.
 * Basically, don't really deallocate freed memory; keep it in the list
 * but mark it as freed and do extra checking in debug_memory_check().
 * This can detect some cases of use-after-free.  But note that since we
 * never really free anything this will use a lot of memory.
 */
#define DEBUG_FREED_MEMORY 0
#define DEBUG_FREED_BYTE 0x33


struct debug_memory_header 
{
   struct list_head head;
   
   unsigned long no;
   const char *file;
   unsigned line;
   const char *function;
#if DEBUG_MEMORY_STACK
   struct debug_stack_frame backtrace[DEBUG_MEMORY_STACK];
#endif
   size_t size;
#if DEBUG_FREED_MEMORY
   boolean freed;  /**< Is this a freed block? */
#endif

   unsigned magic;
   unsigned tag;
};

struct debug_memory_footer
{
   unsigned magic;
};


static struct list_head list = { &list, &list };

pipe_static_mutex(list_mutex);

static unsigned long last_no = 0;


static INLINE struct debug_memory_header *
header_from_data(void *data)
{
   if(data)
      return (struct debug_memory_header *)((char *)data - sizeof(struct debug_memory_header));
   else
      return NULL;
}

static INLINE void *
data_from_header(struct debug_memory_header *hdr)
{
   if(hdr)
      return (void *)((char *)hdr + sizeof(struct debug_memory_header));
   else
      return NULL;
}

static INLINE struct debug_memory_footer *
footer_from_header(struct debug_memory_header *hdr)
{
   if(hdr)
      return (struct debug_memory_footer *)((char *)hdr + sizeof(struct debug_memory_header) + hdr->size);
   else
      return NULL;
}


void *
debug_malloc(const char *file, unsigned line, const char *function,
             size_t size) 
{
   struct debug_memory_header *hdr;
   struct debug_memory_footer *ftr;
   
   hdr = os_malloc(sizeof(*hdr) + size + sizeof(*ftr));
   if(!hdr) {
      debug_printf("%s:%u:%s: out of memory when trying to allocate %lu bytes\n",
                   file, line, function,
                   (long unsigned)size);
      return NULL;
   }
 
   hdr->no = last_no++;
   hdr->file = file;
   hdr->line = line;
   hdr->function = function;
   hdr->size = size;
   hdr->magic = DEBUG_MEMORY_MAGIC;
   hdr->tag = 0;
#if DEBUG_FREED_MEMORY
   hdr->freed = FALSE;
#endif

#if DEBUG_MEMORY_STACK
   debug_backtrace_capture(hdr->backtrace, 0, DEBUG_MEMORY_STACK);
#endif

   ftr = footer_from_header(hdr);
   ftr->magic = DEBUG_MEMORY_MAGIC;
   
   pipe_mutex_lock(list_mutex);
   LIST_ADDTAIL(&hdr->head, &list);
   pipe_mutex_unlock(list_mutex);
   
   return data_from_header(hdr);
}

void
debug_free(const char *file, unsigned line, const char *function,
           void *ptr) 
{
   struct debug_memory_header *hdr;
   struct debug_memory_footer *ftr;
   
   if(!ptr)
      return;
   
   hdr = header_from_data(ptr);
   if(hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: freeing bad or corrupted memory %p\n",
                   file, line, function,
                   ptr);
      debug_assert(0);
      return;
   }

   ftr = footer_from_header(hdr);
   if(ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
                   hdr->file, hdr->line, hdr->function,
                   ptr);
      debug_assert(0);
   }

#if DEBUG_FREED_MEMORY
   /* Check for double-free */
   assert(!hdr->freed);
   /* Mark the block as freed but don't really free it */
   hdr->freed = TRUE;
   /* Save file/line where freed */
   hdr->file = file;
   hdr->line = line;
   /* set freed memory to special value */
   memset(ptr, DEBUG_FREED_BYTE, hdr->size);
#else
   pipe_mutex_lock(list_mutex);
   LIST_DEL(&hdr->head);
   pipe_mutex_unlock(list_mutex);
   hdr->magic = 0;
   ftr->magic = 0;
   
   os_free(hdr);
#endif
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
   struct debug_memory_header *old_hdr, *new_hdr;
   struct debug_memory_footer *old_ftr, *new_ftr;
   void *new_ptr;
   
   if(!old_ptr)
      return debug_malloc( file, line, function, new_size );
   
   if(!new_size) {
      debug_free( file, line, function, old_ptr );
      return NULL;
   }
   
   old_hdr = header_from_data(old_ptr);
   if(old_hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: reallocating bad or corrupted memory %p\n",
                   file, line, function,
                   old_ptr);
      debug_assert(0);
      return NULL;
   }
   
   old_ftr = footer_from_header(old_hdr);
   if(old_ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
                   old_hdr->file, old_hdr->line, old_hdr->function,
                   old_ptr);
      debug_assert(0);
   }

   /* alloc new */
   new_hdr = os_malloc(sizeof(*new_hdr) + new_size + sizeof(*new_ftr));
   if(!new_hdr) {
      debug_printf("%s:%u:%s: out of memory when trying to allocate %lu bytes\n",
                   file, line, function,
                   (long unsigned)new_size);
      return NULL;
   }
   new_hdr->no = old_hdr->no;
   new_hdr->file = old_hdr->file;
   new_hdr->line = old_hdr->line;
   new_hdr->function = old_hdr->function;
   new_hdr->size = new_size;
   new_hdr->magic = DEBUG_MEMORY_MAGIC;
   new_hdr->tag = 0;
#if DEBUG_FREED_MEMORY
   new_hdr->freed = FALSE;
#endif
   
   new_ftr = footer_from_header(new_hdr);
   new_ftr->magic = DEBUG_MEMORY_MAGIC;
   
   pipe_mutex_lock(list_mutex);
   LIST_REPLACE(&old_hdr->head, &new_hdr->head);
   pipe_mutex_unlock(list_mutex);

   /* copy data */
   new_ptr = data_from_header(new_hdr);
   memcpy( new_ptr, old_ptr, old_size < new_size ? old_size : new_size );

   /* free old */
   old_hdr->magic = 0;
   old_ftr->magic = 0;
   os_free(old_hdr);

   return new_ptr;
}

unsigned long
debug_memory_begin(void)
{
   return last_no;
}

void 
debug_memory_end(unsigned long start_no)
{
   size_t total_size = 0;
   struct list_head *entry;

   if(start_no == last_no)
      return;

   entry = list.prev;
   for (; entry != &list; entry = entry->prev) {
      struct debug_memory_header *hdr;
      void *ptr;
      struct debug_memory_footer *ftr;

      hdr = LIST_ENTRY(struct debug_memory_header, entry, head);
      ptr = data_from_header(hdr);
      ftr = footer_from_header(hdr);

      if(hdr->magic != DEBUG_MEMORY_MAGIC) {
         debug_printf("%s:%u:%s: bad or corrupted memory %p\n",
                      hdr->file, hdr->line, hdr->function,
                      ptr);
         debug_assert(0);
      }

      if((start_no <= hdr->no && hdr->no < last_no) ||
	 (last_no < start_no && (hdr->no < last_no || start_no <= hdr->no))) {
	 debug_printf("%s:%u:%s: %lu bytes at %p not freed\n",
		      hdr->file, hdr->line, hdr->function,
		      (unsigned long) hdr->size, ptr);
#if DEBUG_MEMORY_STACK
	 debug_backtrace_dump(hdr->backtrace, DEBUG_MEMORY_STACK);
#endif
	 total_size += hdr->size;
      }

      if(ftr->magic != DEBUG_MEMORY_MAGIC) {
         debug_printf("%s:%u:%s: buffer overflow %p\n",
                      hdr->file, hdr->line, hdr->function,
                      ptr);
         debug_assert(0);
      }
   }

   if(total_size) {
      debug_printf("Total of %lu KB of system memory apparently leaked\n",
		   (unsigned long) (total_size + 1023)/1024);
   }
   else {
      debug_printf("No memory leaks detected.\n");
   }
}


/**
 * Put a tag (arbitrary integer) on a memory block.
 * Can be useful for debugging.
 */
void
debug_memory_tag(void *ptr, unsigned tag)
{
   struct debug_memory_header *hdr;
   
   if (!ptr)
      return;
   
   hdr = header_from_data(ptr);
   if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s corrupted memory at %p\n", __FUNCTION__, ptr);
      debug_assert(0);
   }

   hdr->tag = tag;
}


/**
 * Check the given block of memory for validity/corruption.
 */
void
debug_memory_check_block(void *ptr)
{
   struct debug_memory_header *hdr;
   struct debug_memory_footer *ftr;
   
   if (!ptr)
      return;
   
   hdr = header_from_data(ptr);
   ftr = footer_from_header(hdr);

   if (hdr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: bad or corrupted memory %p\n",
                   hdr->file, hdr->line, hdr->function, ptr);
      debug_assert(0);
   }

   if (ftr->magic != DEBUG_MEMORY_MAGIC) {
      debug_printf("%s:%u:%s: buffer overflow %p\n",
                   hdr->file, hdr->line, hdr->function, ptr);
      debug_assert(0);
   }
}



/**
 * We can periodically call this from elsewhere to do a basic sanity
 * check of the heap memory we've allocated.
 */
void 
debug_memory_check(void)
{
   struct list_head *entry;

   entry = list.prev;
   for (; entry != &list; entry = entry->prev) {
      struct debug_memory_header *hdr;
      struct debug_memory_footer *ftr;
      const char *ptr;

      hdr = LIST_ENTRY(struct debug_memory_header, entry, head);
      ftr = footer_from_header(hdr);
      ptr = (const char *) data_from_header(hdr);

      if (hdr->magic != DEBUG_MEMORY_MAGIC) {
         debug_printf("%s:%u:%s: bad or corrupted memory %p\n",
                      hdr->file, hdr->line, hdr->function, ptr);
         debug_assert(0);
      }

      if (ftr->magic != DEBUG_MEMORY_MAGIC) {
         debug_printf("%s:%u:%s: buffer overflow %p\n",
                      hdr->file, hdr->line, hdr->function, ptr);
         debug_assert(0);
      }

#if DEBUG_FREED_MEMORY
      /* If this block is marked as freed, check that it hasn't been touched */
      if (hdr->freed) {
         int i;
         for (i = 0; i < hdr->size; i++) {
            if (ptr[i] != DEBUG_FREED_BYTE) {
               debug_printf("Memory error: byte %d of block at %p of size %d is 0x%x\n",
                            i, ptr, hdr->size, ptr[i]);
               debug_printf("Block was freed at %s:%d\n", hdr->file, hdr->line);
            }
            assert(ptr[i] == DEBUG_FREED_BYTE);
         }
      }
#endif
   }
}
