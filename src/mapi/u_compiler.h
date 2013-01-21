#ifndef _U_COMPILER_H_
#define _U_COMPILER_H_

#include "c99_compat.h" /* inline, __func__, etc. */


/* XXX: Use standard `inline` keyword instead */
#ifndef INLINE
#  define INLINE inline
#endif

/* Function visibility */
#ifndef PUBLIC
#  if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#    define PUBLIC __attribute__((visibility("default")))
#  elif defined(_MSC_VER)
#    define PUBLIC __declspec(dllexport)
#  else
#    define PUBLIC
#  endif
#endif

#ifndef likely
#  if defined(__GNUC__)
#    define likely(x)   __builtin_expect(!!(x), 1)
#    define unlikely(x) __builtin_expect(!!(x), 0)
#  else
#    define likely(x)   (x)
#    define unlikely(x) (x)
#  endif
#endif

#endif /* _U_COMPILER_H_ */
