#ifndef __egl_types_h_
#define __egl_types_h_

/*
** egltypes.h is platform dependent. It defines:
**
**     - EGL types and resources
**     - Native types
**     - EGL and native handle values
**
** EGL types and resources are to be typedef'ed with appropriate platform
** dependent resource handle types. EGLint must be an integer of at least
** 32-bit.
**
** NativeDisplayType, NativeWindowType and NativePixmapType are to be
** replaced with corresponding types of the native window system in egl.h.
**
** EGL and native handle values must match their types.
*/

#if (defined(WIN32) || defined(_WIN32_WCE))

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Windows Header Files:
#include <windows.h>

typedef HDC		NativeDisplayType;
typedef HWND	NativeWindowType;
typedef HBITMAP NativePixmapType;

#define EGL_DEFAULT_DISPLAY GetDC(0)

#elif defined(__SYMBIAN32__)

#include <e32def.h>

class RWindow;
class CWindowGc;
class CFbsBitmap;

typedef CWindowGc *		NativeDisplayType;
typedef RWindow *		NativeWindowType;
typedef CFbsBitmap * 	NativePixmapType;

#define EGL_DEFAULT_DISPLAY ((NativeDisplayType) 0)

#elif defined(__gnu_linux__)

typedef void *		NativeDisplayType;
typedef void *		NativeWindowType;
typedef void * 		NativePixmapType;

#define EGL_DEFAULT_DISPLAY ((NativeDisplayType) 0)

#else

#	error "Unsupported Operating System"

#endif

#ifdef __cplusplus

namespace EGL {
	class Context;
	class Config;
	class Surface;
}

typedef const EGL::Config *		EGLConfig;
typedef EGL::Surface *			EGLSurface;
typedef EGL::Context  *			EGLContext;

#else

typedef void *			EGLConfig;
typedef void *			EGLSurface;
typedef void *			EGLContext;

#endif


/*
** Types and resources
*/
typedef int				EGLBoolean;
typedef int				EGLint;
typedef void *			EGLDisplay;

/*
** EGL and native handle values
*/
#define EGL_NO_CONTEXT ((EGLContext)0)
#define EGL_NO_DISPLAY ((EGLDisplay)0)
#define EGL_NO_SURFACE ((EGLSurface)0)


#endif //ndef __egl_types_h_
