/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2006  Brian Paul   All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file bitset.h
 * \brief Bitset of arbitrary size definitions.
 * \author Michal Krol
 */

#ifndef BITSET_H
#define BITSET_H

#include "imports.h"

/****************************************************************************
 * generic bitset implementation
 */

#define BITSET_WORD GLuint
#define BITSET_WORDBITS (sizeof (BITSET_WORD) * 8)

/* bitset declarations
 */
#define BITSET_WORDS(bits) (ALIGN(bits, BITSET_WORDBITS) / BITSET_WORDBITS)
#define BITSET_DECLARE(name, bits) BITSET_WORD name[BITSET_WORDS(bits)]

/* bitset operations
 */
#define BITSET_COPY(x, y) memcpy( (x), (y), sizeof (x) )
#define BITSET_EQUAL(x, y) (memcmp( (x), (y), sizeof (x) ) == 0)
#define BITSET_ZERO(x) memset( (x), 0, sizeof (x) )
#define BITSET_ONES(x) memset( (x), 0xff, sizeof (x) )

#define BITSET_BITWORD(b) ((b) / BITSET_WORDBITS)
#define BITSET_BIT(b) (1 << ((b) % BITSET_WORDBITS))

/* single bit operations
 */
#define BITSET_TEST(x, b) ((x)[BITSET_BITWORD(b)] & BITSET_BIT(b))
#define BITSET_SET(x, b) ((x)[BITSET_BITWORD(b)] |= BITSET_BIT(b))
#define BITSET_CLEAR(x, b) ((x)[BITSET_BITWORD(b)] &= ~BITSET_BIT(b))

#define BITSET_MASK(b) ((b) == BITSET_WORDBITS ? ~0 : BITSET_BIT(b) - 1)
#define BITSET_RANGE(b, e) (BITSET_MASK((e) + 1) & ~BITSET_MASK(b))

/* bit range operations
 */
#define BITSET_TEST_RANGE(x, b, e) \
   (BITSET_BITWORD(b) == BITSET_BITWORD(e) ? \
   ((x)[BITSET_BITWORD(b)] & BITSET_RANGE(b, e)) : \
   (assert (!"BITSET_TEST_RANGE: bit range crosses word boundary"), 0))
#define BITSET_SET_RANGE(x, b, e) \
   (BITSET_BITWORD(b) == BITSET_BITWORD(e) ? \
   ((x)[BITSET_BITWORD(b)] |= BITSET_RANGE(b, e)) : \
   (assert (!"BITSET_SET_RANGE: bit range crosses word boundary"), 0))
#define BITSET_CLEAR_RANGE(x, b, e) \
   (BITSET_BITWORD(b) == BITSET_BITWORD(e) ? \
   ((x)[BITSET_BITWORD(b)] &= ~BITSET_RANGE(b, e)) : \
   (assert (!"BITSET_CLEAR_RANGE: bit range crosses word boundary"), 0))

/* Get first bit set in a bitset.
 */
static inline int
__bitset_ffs(const BITSET_WORD *x, int n)
{
   int i;

   for (i = 0; i < n; i++) {
      if (x[i])
	 return ffs(x[i]) + BITSET_WORDBITS * i;
   }

   return 0;
}

#define BITSET_FFS(x) __bitset_ffs(x, Elements(x))

#endif
