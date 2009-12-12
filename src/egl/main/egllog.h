#ifndef EGLLOG_INCLUDED
#define EGLLOG_INCLUDED

#include "egltypedefs.h"

#define _EGL_FATAL   0   /* unrecoverable error */
#define _EGL_WARNING 1   /* recoverable error/problem */
#define _EGL_INFO    2   /* just useful info */
#define _EGL_DEBUG   3   /* useful info for debugging */


typedef void (*_EGLLogProc)(EGLint level, const char *msg);


extern void
_eglSetLogProc(_EGLLogProc logger);


extern void
_eglSetLogLevel(EGLint level);


extern void
_eglLog(EGLint level, const char *fmtStr, ...);


#endif /* EGLLOG_INCLUDED */
