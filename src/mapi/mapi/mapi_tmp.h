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

#include "u_macros.h"

#ifndef MAPI_ABI_HEADER
#error "MAPI_ABI_HEADER must be defined"
#endif


/**
 * Get API defines.
 */
#ifdef MAPI_TMP_DEFINES
#   define MAPI_ABI_DEFINES
#   include MAPI_ABI_HEADER

#ifndef MAPI_ABI_PREFIX
#error "MAPI_ABI_PREFIX must be defined"
#endif
#ifndef MAPI_ABI_PUBLIC
#error "MAPI_ABI_PUBLIC must be defined"
#endif
#ifndef MAPI_ABI_ATTR
#error "MAPI_ABI_ATTR must be defined"
#endif

#undef MAPI_TMP_DEFINES
#endif /* MAPI_TMP_DEFINES */


/**
 * Generate fields of struct mapi_table.
 */
#ifdef MAPI_TMP_TABLE
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      ret (MAPI_ABI_ATTR *name) params;
#   define MAPI_DYNAMIC_ENTRY(ret, name, params)               \
      ret (MAPI_ABI_ATTR *name) params;
#   include MAPI_ABI_HEADER
#undef MAPI_TMP_TABLE
#endif /* MAPI_TMP_TABLE */


/**
 * Declare public entries.
 */
#ifdef MAPI_TMP_PUBLIC_DECLARES
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      MAPI_ABI_PUBLIC ret MAPI_ABI_ATTR U_CONCAT(MAPI_ABI_PREFIX, name) params;
#   define MAPI_ALIAS_ENTRY(alias, ret, name, params)          \
      MAPI_ABI_ENTRY(ret, name, params);
#   define MAPI_ABI_ENTRY_HIDDEN(ret, name, params)            \
      HIDDEN ret MAPI_ABI_ATTR U_CONCAT(MAPI_ABI_PREFIX, name) params;
#   define MAPI_ALIAS_ENTRY_HIDDEN(alias, ret, name, params)   \
      MAPI_ABI_ENTRY_HIDDEN(ret, name, params)
#   include MAPI_ABI_HEADER
#undef MAPI_TMP_PUBLIC_DECLARES
#endif /* MAPI_TMP_PUBLIC_DECLARES */


/**
 * Generate string pool and public stubs.
 */
#ifdef MAPI_TMP_PUBLIC_STUBS
/* define the string pool */
static const char public_string_pool[] =
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      U_STRINGIFY(name) "\0"
#   define MAPI_ALIAS_ENTRY(alias, ret, name, params)          \
      MAPI_ABI_ENTRY(ret, name, params)
#   include MAPI_ABI_HEADER
   ;
/* define public_sorted_indices */
#   define MAPI_ABI_SORTED_INDICES public_sorted_indices
#   include MAPI_ABI_HEADER

/* define public_stubs */
static const struct mapi_stub public_stubs[] = {
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      { (mapi_func) U_CONCAT(MAPI_ABI_PREFIX, name),           \
         MAPI_SLOT_ ## name, (void *) MAPI_POOL_ ## name },
#   define MAPI_ALIAS_ENTRY(alias, ret, name, params)          \
      MAPI_ABI_ENTRY(ret, name, params)
#   include MAPI_ABI_HEADER
   { NULL, -1, (void *) -1 }
};

#undef MAPI_TMP_PUBLIC_STUBS
#endif /* MAPI_TMP_PUBLIC_STUBS */


/**
 * Generate public entries.
 */
#ifdef MAPI_TMP_PUBLIC_ENTRIES
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      ret MAPI_ABI_ATTR U_CONCAT(MAPI_ABI_PREFIX, name) params
#   define MAPI_ABI_CODE(ret, name, args)                      \
      {                                                        \
         const struct mapi_table *tbl = u_current_get();       \
         tbl->name args;                                       \
      }
#   define MAPI_ABI_CODE_RETURN(ret, name, args)               \
      {                                                        \
         const struct mapi_table *tbl = u_current_get();       \
         return tbl->name args;                                \
      }
#   define MAPI_ALIAS_ENTRY(alias, ret, name, params)          \
      MAPI_ABI_ENTRY(ret, name, params)
#   define MAPI_ALIAS_CODE(alias, ret, name, args)             \
      MAPI_ABI_CODE(ret, alias, args)
#   define MAPI_ALIAS_CODE_RETURN(alias, ret, name, args)      \
      MAPI_ABI_CODE_RETURN(ret, alias, args)
#   include MAPI_ABI_HEADER
#undef MAPI_TMP_PUBLIC_ENTRIES
#endif /* MAPI_TMP_PUBLIC_ENTRIES */


/**
 * Generate noop entries.
 */
#ifdef MAPI_TMP_NOOP_ARRAY
#ifdef DEBUG
#   define MAPI_ABI_ENTRY(ret, name, params)                         \
      static ret MAPI_ABI_ATTR U_CONCAT(noop_, name) params
#   define MAPI_ABI_CODE(ret, name, args)                            \
      {                                                              \
         noop_warn(U_CONCAT_STR(MAPI_ABI_PREFIX, name));             \
      }
#   define MAPI_ABI_CODE_RETURN(ret, name, args)                     \
      {                                                              \
         noop_warn(U_CONCAT_STR(MAPI_ABI_PREFIX, name));             \
         return (ret) 0;                                             \
      }
#   include MAPI_ABI_HEADER

/* define the noop function array that may be casted to mapi_table */
const mapi_func table_noop_array[] = {
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      (mapi_func) U_CONCAT(noop_, name),
#   define MAPI_DYNAMIC_ENTRY(ret, name, params)               \
      (mapi_func) noop_generic,
#   include MAPI_ABI_HEADER
      (mapi_func) noop_generic
};

#else /* DEBUG */

const mapi_func table_noop_array[] = {
#   define MAPI_ABI_ENTRY(ret, name, params)                   \
      (mapi_func) noop_generic,
#   define MAPI_DYNAMIC_ENTRY(ret, name, params)               \
      (mapi_func) noop_generic,
#   include MAPI_ABI_HEADER
      (mapi_func) noop_generic
};

#endif /* DEBUG */
#undef MAPI_TMP_NOOP_ARRAY
#endif /* MAPI_TMP_NOOP_ARRAY */


#ifdef MAPI_TMP_STUB_ASM_GCC
#   define STUB_ASM_ALIAS(func, to)    \
      ".globl " func "\n"              \
      ".set " func ", " to
#   define STUB_ASM_HIDE(func)         \
      ".hidden " func

#   define MAPI_ABI_ENTRY(ret, name, params)                         \
      __asm__(STUB_ASM_ENTRY(U_CONCAT_STR(MAPI_ABI_PREFIX, name)));
#   define MAPI_ABI_CODE(ret, name, args)                            \
      __asm__(STUB_ASM_CODE(U_STRINGIFY(MAPI_SLOT_ ## name)));
#   define MAPI_ALIAS_ENTRY(alias, ret, name, params)                \
      __asm__(STUB_ASM_ALIAS(U_CONCAT_STR(MAPI_ABI_PREFIX, name),    \
            U_CONCAT_STR(MAPI_ABI_PREFIX, alias)));
#   define MAPI_ABI_ENTRY_HIDDEN(ret, name, params)                  \
      __asm__(STUB_ASM_HIDE(U_CONCAT_STR(MAPI_ABI_PREFIX, name)));   \
      MAPI_ABI_ENTRY(ret, name, params);
#   define MAPI_ALIAS_ENTRY_HIDDEN(alias, ret, name, params)         \
      __asm__(STUB_ASM_HIDE(U_CONCAT_STR(MAPI_ABI_PREFIX, name)));   \
      MAPI_ALIAS_ENTRY(alias, ret, name, params);
#   include MAPI_ABI_HEADER
#undef MAPI_TMP_STUB_ASM_GCC
#endif /* MAPI_TMP_STUB_ASM_GCC */
