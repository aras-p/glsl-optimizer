/*
 * egltypes.h - EGL API compatibility
 *
 * The intention here is to support multiple EGL implementations for the
 * various backends - GLX, AGL, WGL, Solo - so we define the EGL types as
 * opaque handles.  We also define the Native types as opaque handles for
 * now, which should be fine for GLX and Solo but the others who knows.
 * They can extend this later.
 *
 * We require that 'int' be 32 bits.  Other than that this should be pretty
 * portable.
 *
 * Derived from the OpenGL|ES 1.1 egl.h on the Khronos website:
 * http://www.khronos.org/opengles/spec_headers/opengles1_1/egl.h
 *
 */

#ifndef _EGLTYPES_H
#define _EGLTYPES_H

#include <sys/types.h>

/*
 * Native types
 */
typedef void    *NativeDisplayType;
typedef void    *NativePixmapType;
typedef void    *NativeWindowType;

/*
 * Types and resources
 */
typedef GLboolean   EGLBoolean;
typedef GLint       EGLint;
typedef void        *EGLDisplay;
typedef void        *EGLConfig;
typedef void        *EGLSurface;
typedef void        *EGLContext;

/*
 * EGL and native handle values
 */
#define EGL_DEFAULT_DISPLAY ((NativeDisplayType)0)
#define EGL_NO_CONTEXT      ((EGLContext)0)
#define EGL_NO_DISPLAY      ((EGLDisplay)0)
#define EGL_NO_SURFACE      ((EGLSurface)0)

#endif /* _EGLTYPES_H */
