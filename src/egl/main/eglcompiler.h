#ifndef EGLCOMPILER_INCLUDED
#define EGLCOMPILER_INCLUDED


/**
 * Function inlining
 */
#if defined(__GNUC__)
#  define INLINE __inline__
#elif defined(__MSC__)
#  define INLINE __inline
#elif defined(_MSC_VER)
#  define INLINE __inline
#elif defined(__ICL)
#  define INLINE __inline
#elif defined(__INTEL_COMPILER)
#  define INLINE inline
#elif defined(__WATCOMC__) && (__WATCOMC__ >= 1100)
#  define INLINE __inline
#elif defined(__SUNPRO_C) && defined(__C99FEATURES__)
#  define INLINE inline
#  define __inline inline
#  define __inline__ inline
#elif (__STDC_VERSION__ >= 199901L) /* C99 */
#  define INLINE inline
#else
#  define INLINE
#endif


#endif /* EGLCOMPILER_INCLUDED */
