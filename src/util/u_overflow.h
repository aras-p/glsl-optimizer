/*
 * Copyright © 2025 Advanced Micro Devices, Inc.
 * SPDX-License-Identifier: MIT
 */
#include "util/macros.h"

#ifndef U_OVERFLOW_H
#define U_OVERFLOW_H

#ifdef HAVE___BUILTIN_ADD_OVERFLOW
#define util_add_overflow(ty, a, b, c) __builtin_add_overflow(a, b, c)
#else
#define DEFINE_U_ADD_OVERFLOW_UINT(ty)                        \
   static inline bool                                         \
   util_add_overflow_##ty(ty a, ty b, ty * res) {     \
      *res = a + b;                                           \
      return *res < a;                                        \
   }
#define DEFINE_U_ADD_OVERFLOW_SINT(ty)                        \
   static inline bool                                         \
   util_add_overflow_##ty(ty a, ty b, ty * res) {     \
      if ((b > 0 && a > u_intN_max(sizeof(a) * 8) - b) ||     \
          (b < 0 && a < u_intN_min(sizeof(a) * 8) - b))       \
         return true;                                         \
      *res = a + b;                                           \
      return false;                                           \
   }

DEFINE_U_ADD_OVERFLOW_UINT(size_t)
DEFINE_U_ADD_OVERFLOW_UINT(uint64_t)
DEFINE_U_ADD_OVERFLOW_SINT(int64_t)

#define util_add_overflow(ty, a, b, c) util_add_overflow_##ty(a, b, c)
#endif /* HAVE___BUILTIN_ADD_OVERFLOW */

#ifdef HAVE___BUILTIN_ADD_OVERFLOW_P
#define util_add_check_overflow(ty, a, b) __builtin_add_overflow_p(a, b, (ty)0)
#else
#define DEFINE_U_ADD_CHECK_OVERFLOW_UINT(ty)                  \
   static inline bool                                         \
   util_add_check_overflow_##ty(ty a, ty b) {                 \
      ty c = a + b;                                           \
      return c < a;                                           \
   }
#define DEFINE_U_ADD_CHECK_OVERFLOW_SINT(ty)                  \
   static inline bool                                         \
   util_add_check_overflow_##ty(ty a, ty b) {                 \
      return (b > 0 && a > u_intN_max(sizeof(a) * 8) - b) ||  \
             (b < 0 && a < u_intN_min(sizeof(a) * 8) - b);    \
   }
DEFINE_U_ADD_CHECK_OVERFLOW_UINT(uint8_t)
DEFINE_U_ADD_CHECK_OVERFLOW_UINT(uint16_t)
DEFINE_U_ADD_CHECK_OVERFLOW_UINT(uint32_t)
DEFINE_U_ADD_CHECK_OVERFLOW_UINT(uint64_t)

DEFINE_U_ADD_CHECK_OVERFLOW_SINT(int8_t)
DEFINE_U_ADD_CHECK_OVERFLOW_SINT(int16_t)
DEFINE_U_ADD_CHECK_OVERFLOW_SINT(int32_t)
DEFINE_U_ADD_CHECK_OVERFLOW_SINT(int64_t)

#define util_add_check_overflow(ty, a, b) util_add_check_overflow_##ty(a, b)
#endif /* HAVE___BUILTIN_ADD_OVERFLOW_P */

#ifdef HAVE___BUILTIN_SUB_OVERFLOW_P
#define util_sub_check_overflow(ty, a, b) __builtin_sub_overflow_p(a, b, (ty)0)
#else
#define DEFINE_U_SUB_CHECK_OVERFLOW_SINT(ty)                  \
   static inline bool                                         \
   util_sub_check_overflow_##ty(ty a, ty b) {                 \
      return (b < 0 && a > u_intN_max(sizeof(a) * 8) + b) ||  \
             (b > 0 && a < u_intN_min(sizeof(a) * 8) + b);    \
   }

DEFINE_U_SUB_CHECK_OVERFLOW_SINT(int8_t)
DEFINE_U_SUB_CHECK_OVERFLOW_SINT(int16_t)
DEFINE_U_SUB_CHECK_OVERFLOW_SINT(int32_t)
DEFINE_U_SUB_CHECK_OVERFLOW_SINT(int64_t)

#define util_sub_check_overflow(ty, a, b) util_sub_check_overflow_##ty(a, b)
#endif /* HAVE___BUILTIN_SUB_OVERFLOW_P */

#endif
