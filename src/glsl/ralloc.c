/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <talloc.h>

#include "ralloc.h"

#ifdef __GNUC__
#define likely(x)       __builtin_expect(!!(x),1)
#define unlikely(x)     __builtin_expect(!!(x),0)
#else
#define likely(x)       !!(x)
#define unlikely(x)     !!(x)
#endif

void *
ralloc_context(const void *ctx)
{
   return talloc_new(ctx);
}

void *
ralloc_size(const void *ctx, size_t size)
{
   return talloc_size(ctx, size);
}

void *
rzalloc_size(const void *ctx, size_t size)
{
   return talloc_zero_size(ctx, size);
}

void *
reralloc_size(const void *ctx, void *ptr, size_t size)
{
   return talloc_realloc_size(ctx, ptr, size);
}

void *
ralloc_array_size(const void *ctx, size_t size, unsigned count)
{
   if (count > SIZE_MAX/size)
      return NULL;

   return talloc_array_size(ctx, size, count);
}

void *
rzalloc_array_size(const void *ctx, size_t size, unsigned count)
{
   if (count > SIZE_MAX/size)
      return NULL;

   return rzalloc_size(ctx, size * count);
}

void *
reralloc_array_size(const void *ctx, void *ptr, size_t size, unsigned count)
{
   if (count > SIZE_MAX/size)
      return false;

   return reralloc_size(ctx, ptr, size * count);
}

void
ralloc_free(void *ptr)
{
   talloc_free(ptr);
}

void
ralloc_steal(const void *new_ctx, void *ptr)
{
   talloc_steal(new_ctx, ptr);
}

void *
ralloc_parent(const void *ptr)
{
   return talloc_parent(ptr);
}

void *
ralloc_autofree_context(void)
{
   return talloc_autofree_context();
}

void
ralloc_set_destructor(const void *ptr, void(*destructor)(void *))
{
   talloc_set_destructor(ptr, (int(*)(void *)) destructor);
}

char *
ralloc_strdup(const void *ctx, const char *str)
{
   return talloc_strdup(ctx, str);
}

char *
ralloc_strndup(const void *ctx, const char *str, size_t max)
{
   return talloc_strndup(ctx, str, max);
}

bool
ralloc_strcat(char **dest, const char *str)
{
   void *ptr;
   assert(dest != NULL);
   ptr = talloc_strdup_append(*dest, str);

   if (unlikely(ptr == NULL))
      return false;

   *dest = ptr;
   return true;
}

bool
ralloc_strncat(char **dest, const char *str, size_t n)
{
   void *ptr;
   assert(dest != NULL);
   ptr = talloc_strndup_append(*dest, str, n);

   if (unlikely(ptr == NULL))
      return false;

   *dest = ptr;
   return true;
}

char *
ralloc_asprintf(const void *ctx, const char *fmt, ...)
{
   char *ptr;
   va_list args;
   va_start(args, fmt);
   ptr = ralloc_vasprintf(ctx, fmt, args);
   va_end(args);
   return ptr;
}

char *
ralloc_vasprintf(const void *ctx, const char *fmt, va_list args)
{
   return talloc_vasprintf(ctx, fmt, args);
}

bool
ralloc_asprintf_append(char **str, const char *fmt, ...)
{
   va_list args;

   va_start(args, fmt);
   return ralloc_vasprintf_append(str, fmt, args);
   va_end(args);
}

bool
ralloc_vasprintf_append(char **str, const char *fmt, va_list args)
{
   void *ptr;
   assert(str != NULL);
   ptr = talloc_vasprintf_append(*str, fmt, args);

   if (unlikely(ptr == NULL))
      return false;

   *str = ptr;
   return true;
}
