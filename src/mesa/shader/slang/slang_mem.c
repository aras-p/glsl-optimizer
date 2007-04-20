/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_mem.c
 *
 * Memory manager for GLSL compiler.  The general idea is to do all
 * allocations out of a large pool then just free the pool when done
 * compiling to avoid intricate malloc/free tracking and memory leaks.
 *
 * \author Brian Paul
 */

#include "context.h"
#include "slang_mem.h"

#define GRANULARITY 8
#define ROUND_UP(B)  ( ((B) + (GRANULARITY - 1)) & ~(GRANULARITY - 1) )


slang_mempool *
_slang_new_mempool(GLuint initialSize)
{
   slang_mempool *pool = (slang_mempool *) _mesa_calloc(sizeof(slang_mempool));
   if (pool) {
      pool->Data = (char *) _mesa_calloc(initialSize);
      if (!pool->Data) {
         _mesa_free(pool);
         return NULL;
      }
      pool->Size = initialSize;
      pool->Used = 0;
   }
   return pool;
}


void
_slang_delete_mempool(slang_mempool *pool)
{
   GLuint total = 0;
   while (pool) {
      slang_mempool *next = pool->Next;
      total += pool->Used;
      _mesa_free(pool->Data);
      _mesa_free(pool);
      pool = next;
   }
   printf("TOTAL USED %u\n", total);
}


/**
 * Alloc 'bytes' from shader mempool.
 */
void *
_slang_alloc(GLuint bytes)
{
   slang_mempool *pool;
   GET_CURRENT_CONTEXT(ctx);

   pool = (slang_mempool *) ctx->Shader.MemPool;

   while (pool) {
      if (pool->Used + bytes <= pool->Size) {
         /* found room */
         void *addr = (void *) (pool->Data + pool->Used);
         pool->Used += ROUND_UP(bytes);
         /*printf("alloc %u  Used %u\n", bytes, pool->Used);*/
         return addr;
      }
      else if (pool->Next) {
         /* try next block */
         pool = pool->Next;
      }
      else {
         /* alloc new pool */
         assert(bytes <= pool->Size); /* XXX or max2() */
         pool->Next = _slang_new_mempool(pool->Size);
         if (!pool->Next) {
            /* we're _really_ out of memory */
            return NULL;
         }
         else {
            pool->Used = ROUND_UP(bytes);
            return (void *) pool->Data;
         }
      }
   }
   return NULL;
}


void *
_slang_realloc(void *oldBuffer, GLuint oldSize, GLuint newSize)
{
   const GLuint copySize = (oldSize < newSize) ? oldSize : newSize;
   void *newBuffer = _slang_alloc(newSize);
   if (newBuffer && oldBuffer && copySize > 0)
      _mesa_memcpy(newBuffer, oldBuffer, copySize);
   return newBuffer;
}


/**
 * Clone string, storing in current mempool.
 */
char *
_slang_strdup(const char *s)
{
   if (s) {
      size_t l = _mesa_strlen(s);
      char *s2 = (char *) _slang_alloc(l + 1);
      if (s2)
         _mesa_strcpy(s2, s);
      return s2;
   }
   else {
      return NULL;
   }
}
