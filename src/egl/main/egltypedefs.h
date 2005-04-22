#ifndef EGLTYPEDEFS_INCLUDED
#define EGLTYPEDEFS_INCLUDED


#include <GL/egl.h>


typedef struct _egl_config _EGLConfig;

typedef struct _egl_context _EGLContext;

typedef struct _egl_display _EGLDisplay;

typedef struct _egl_driver _EGLDriver;

typedef struct _egl_mode _EGLMode;

typedef struct _egl_screen _EGLScreen;

typedef struct _egl_surface _EGLSurface;


typedef void (*_EGLProc)();

typedef _EGLDriver *(*_EGLMain_t)(_EGLDisplay *dpy);


#endif /* EGLTYPEDEFS_INCLUDED */
