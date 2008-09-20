/*
** egltypes.h for Mesa
**
** ONLY egl.h SHOULD INCLUDE THIS FILE!
**
** See comments about egltypes.h in the standard egl.h file.
*/


#include <sys/types.h>


/*
** These opaque EGL types are implemented as unsigned 32-bit integers:
*/
typedef uint32_t EGLDisplay;
typedef uint32_t EGLConfig;
typedef uint32_t EGLSurface;
typedef uint32_t EGLContext;

/* EGL_MESA_screen_surface */
typedef uint32_t EGLModeMESA;
typedef uint32_t EGLScreenMESA;


/*
** Other basic EGL types:
*/
typedef uint8_t EGLBoolean;
typedef int32_t EGLint;

typedef void * NativeDisplayType;
typedef int NativePixmapType;
typedef int NativeWindowType;

/*
** EGL and native handle null values:
*/
#define EGL_DEFAULT_DISPLAY ((NativeDisplayType) 0)
#define EGL_NO_CONTEXT ((EGLContext) 0)
#define EGL_NO_DISPLAY ((EGLDisplay) 0)
#define EGL_NO_SURFACE ((EGLSurface) 0)

/* EGL_MESA_screen_surface */
#define EGL_NO_MODE_MESA ((EGLModeMESA) 0)
