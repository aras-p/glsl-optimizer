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
 * Memory functions
 */


#ifndef U_MEMORY_H
#define U_MEMORY_H


#include "util/u_pointer.h"
#include "pipe/p_debug.h"


#ifdef __cplusplus
extern "C" {
#endif


/* Define ENOMEM for WINCE */ 
#if (_WIN32_WCE < 600)
#ifndef ENOMEM
#define ENOMEM 12
#endif
#endif


#if defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY) && defined(DEBUG) 

/* memory debugging */

#include "pipe/p_debug.h"

#define MALLOC( _size ) \
   debug_malloc( __FILE__, __LINE__, __FUNCTION__, _size )
#define CALLOC( _count, _size ) \
   debug_calloc(__FILE__, __LINE__, __FUNCTION__, _count, _size )
#define FREE( _ptr ) \
   debug_free( __FILE__, __LINE__, __FUNCTION__,  _ptr )
#define REALLOC( _ptr, _old_size, _size ) \
   debug_realloc( __FILE__, __LINE__, __FUNCTION__,  _ptr, _old_size, _size )

#elif defined(PIPE_SUBSYSTEM_WINDOWS_DISPLAY)

void * __stdcall
EngAllocMem(
    unsigned long Flags,
    unsigned long MemSize,
    unsigned long Tag );

void __stdcall
EngFreeMem(
    void *Mem );

#define MALLOC( _size ) EngAllocMem( 0, _size, 'D3AG' )
#define _FREE( _ptr ) EngFreeMem( _ptr )

#elif defined(PIPE_SUBSYSTEM_WINDOWS_MINIPORT)

void *
ExAllocatePool(
    unsigned long PoolType, 
    size_t NumberOfBytes);

void 
ExFreePool(void *P);

#define MALLOC(_size) ExAllocatePool(0, _size)
#define _FREE(_ptr) ExFreePool(_ptr)

#else

#define MALLOC( SIZE )  malloc( SIZE )
#define CALLOC( COUNT, SIZE )   calloc( COUNT, SIZE )
#define FREE( PTR )  free( PTR )
#define REALLOC( OLDPTR, OLDSIZE, NEWSIZE )  realloc( OLDPTR, NEWSIZE )

#endif


#ifndef CALLOC
static INLINE void *
CALLOC( unsigned count, unsigned size )
{
   void *ptr = MALLOC( count * size );
   if( ptr ) {
      memset( ptr, 0, count * size );
   }
   return ptr;
}
#endif /* !CALLOC */

#ifndef FREE
static INLINE void
FREE( void *ptr )
{
   if( ptr ) {
      _FREE( ptr );
   }
}
#endif /* !FREE */

#ifndef REALLOC
static INLINE void *
REALLOC( void *old_ptr, unsigned old_size, unsigned new_size )
{
   void *new_ptr = NULL;

   if (new_size != 0) {
      unsigned copy_size = old_size < new_size ? old_size : new_size;
      new_ptr = MALLOC( new_size );
      if (new_ptr && old_ptr && copy_size) {
         memcpy( new_ptr, old_ptr, copy_size );
      }
   }

   FREE( old_ptr );
   return new_ptr;
}
#endif /* !REALLOC */


#define MALLOC_STRUCT(T)   (struct T *) MALLOC(sizeof(struct T))

#define CALLOC_STRUCT(T)   (struct T *) CALLOC(1, sizeof(struct T))


/**
 * Return memory on given byte alignment
 */
static INLINE void *
align_malloc(size_t bytes, uint alignment)
{
#if defined(HAVE_POSIX_MEMALIGN)
   void *mem;
   alignment = (alignment + (uint)sizeof(void*) - 1) & ~((uint)sizeof(void*) - 1);
   if(posix_memalign(& mem, alignment, bytes) != 0)
      return NULL;
   return mem;
#else
   char *ptr, *buf;

   assert( alignment > 0 );

   ptr = (char *) MALLOC(bytes + alignment + sizeof(void *));
   if (!ptr)
      return NULL;

   buf = (char *) align_pointer( ptr + sizeof(void *), alignment );
   *(char **)(buf - sizeof(void *)) = ptr;

   return buf;
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}

/**
 * Free memory returned by align_malloc().
 */
static INLINE void
align_free(void *ptr)
{
#if defined(HAVE_POSIX_MEMALIGN)
   FREE(ptr);
#else
   void **cubbyHole = (void **) ((char *) ptr - sizeof(void *));
   void *realAddr = *cubbyHole;
   FREE(realAddr);
#endif /* defined(HAVE_POSIX_MEMALIGN) */
}


/**
 * Duplicate a block of memory.
 */
static INLINE void *
mem_dup(const void *src, uint size)
{
   void *dup = MALLOC(size);
   if (dup)
      memcpy(dup, src, size);
   return dup;
}


/**
 * Number of elements in an array.
 */
#ifndef Elements
#define Elements(x) (sizeof(x)/sizeof((x)[0]))
#endif


/**
 * Offset of a field in a struct, in bytes.
 */
#define Offset(TYPE, MEMBER) ((unsigned)&(((TYPE *)NULL)->MEMBER))



#ifdef __cplusplus
}
#endif


#endif /* U_MEMORY_H */
