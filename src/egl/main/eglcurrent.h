#ifndef EGLCURRENT_INCLUDED
#define EGLCURRENT_INCLUDED


#include "egltypedefs.h"


#define _EGL_API_ALL_BITS \
   (EGL_OPENGL_ES_BIT   | \
    EGL_OPENVG_BIT      | \
    EGL_OPENGL_ES2_BIT  | \
    EGL_OPENGL_BIT)


#define _EGL_API_FIRST_API EGL_OPENGL_ES_API
#define _EGL_API_LAST_API EGL_OPENGL_API
#define _EGL_API_NUM_APIS (_EGL_API_LAST_API - _EGL_API_FIRST_API + 1)


/**
 * Per-thread info
 */
struct _egl_thread_info
{
   EGLint LastError;
   _EGLContext *CurrentContexts[_EGL_API_NUM_APIS];
   /* use index for fast access to current context */
   EGLint CurrentAPIIndex;
};


/**
 * Return true if a client API enum is recognized.
 */
static INLINE EGLBoolean
_eglIsApiValid(EGLenum api)
{
   return (api >= _EGL_API_FIRST_API && api <= _EGL_API_LAST_API);
}


/**
 * Convert a client API enum to an index, for use by thread info.
 * The client API enum is assumed to be valid.
 */
static INLINE EGLint
_eglConvertApiToIndex(EGLenum api)
{
   return api - _EGL_API_FIRST_API;
}


/**
 * Convert an index, used by thread info, to a client API enum.
 * The index is assumed to be valid.
 */
static INLINE EGLenum
_eglConvertApiFromIndex(EGLint idx)
{
   return _EGL_API_FIRST_API + idx;
}


PUBLIC _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglDestroyCurrentThread(void);


extern EGLBoolean
_eglIsCurrentThreadDummy(void);


PUBLIC _EGLContext *
_eglGetAPIContext(EGLenum api);


PUBLIC _EGLContext *
_eglGetCurrentContext(void);


PUBLIC EGLBoolean
_eglError(EGLint errCode, const char *msg);


#endif /* EGLCURRENT_INCLUDED */
