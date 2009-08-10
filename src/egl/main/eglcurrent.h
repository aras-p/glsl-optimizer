#ifndef EGLCURRENT_INCLUDED
#define EGLCURRENT_INCLUDED

#include "egltypedefs.h"


#define _EGL_API_NUM_INDICES \
   (EGL_OPENGL_API - EGL_OPENGL_ES_API + 2) /* idx 0 is for EGL_NONE */


/**
 * Per-thread info
 */
struct _egl_thread_info
{
   EGLint LastError;
   _EGLContext *CurrentContexts[_EGL_API_NUM_INDICES];
   /* use index for fast access to current context */
   EGLint CurrentAPIIndex;
};


/**
 * Return true if a client API enum can be converted to an index.
 */
static INLINE EGLBoolean
_eglIsApiValid(EGLenum api)
{
   return ((api >= EGL_OPENGL_ES_API && api <= EGL_OPENGL_API) ||
           api == EGL_NONE);
}


/**
 * Convert a client API enum to an index, for use by thread info.
 * The client API enum is assumed to be valid.
 */
static INLINE EGLint
_eglConvertApiToIndex(EGLenum api)
{
   return (api != EGL_NONE) ? api - EGL_OPENGL_ES_API + 1 : 0;
}


/**
 * Convert an index, used by thread info, to a client API enum.
 * The index is assumed to be valid.
 */
static INLINE EGLenum
_eglConvertApiFromIndex(EGLint idx)
{
   return (idx) ? EGL_OPENGL_ES_API + idx - 1 : EGL_NONE;
}


extern _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglDestroyCurrentThread(void);


extern EGLBoolean
_eglIsCurrentThreadDummy(void);


extern _EGLContext *
_eglGetCurrentContext(void);


extern _EGLDisplay *
_eglGetCurrentDisplay(void);


extern _EGLSurface *
_eglGetCurrentSurface(EGLint readdraw);


extern EGLBoolean
_eglError(EGLint errCode, const char *msg);


#endif /* EGLCURRENT_INCLUDED */
