/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef _TABLE_H_
#define _TABLE_H_

#include "u_compiler.h"
#include "stub.h"

#define MAPI_TMP_DEFINES
#include "mapi_tmp.h"

struct mapi_table {
#define MAPI_TMP_TABLE
#include "mapi_tmp.h"
   mapi_func last;
};

extern const mapi_func table_noop_array[];

/**
 * Get the no-op dispatch table.
 */
static INLINE const struct mapi_table *
table_get_noop(void)
{
   return (const struct mapi_table *) table_noop_array;
}

/**
 * Update the dispatch table to dispatch a stub to the given function.
 */
static INLINE void
table_set_func(struct mapi_table *tbl,
               const struct mapi_stub *stub, mapi_func func)
{
   mapi_func *funcs = (mapi_func *) tbl;
   funcs[stub->slot] = func;
}

/**
 * Return the dispatched function of a stub.
 */
static INLINE mapi_func
table_get_func(const struct mapi_table *tbl, const struct mapi_stub *stub)
{
   const mapi_func *funcs = (const mapi_func *) tbl;
   return funcs[stub->slot];
}

#endif /* _TABLE_H_ */
