/*
 * Copyright © 2025 Collabora, Ltd.
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdlib.h>

#ifndef UTIL_STACK_ARRAY_H
#define UTIL_STACK_ARRAY_H

#define STACK_ARRAY_SIZE 8

/* Sometimes gcc may claim -Wmaybe-uninitialized for the stack array in some
 * places it can't verify that when size is 0 nobody down the call chain reads
 * the array. Please don't try to fix it by zero-initializing the array here
 * since it's used in a lot of different places. An "if (size == 0) return;"
 * may work for you.
 */
#define STACK_ARRAY(type, name, size) \
   type _stack_##name[STACK_ARRAY_SIZE]; \
   type *const name = \
     ((size) <= STACK_ARRAY_SIZE ? _stack_##name : (type *)malloc((size) * sizeof(type)))

#define STACK_ARRAY_FINISH(name) \
   if (name != _stack_##name) free(name)

#endif /* UTIL_STACK_ARRAY_H */
