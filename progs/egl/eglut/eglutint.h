#ifndef _EGLUTINT_H_
#define _EGLUTINT_H_

#include "EGL/egl.h"
#include "eglut.h"

struct eglut_window {
   EGLConfig config;
   EGLContext context;

   /* initialized by native display */
   struct {
      union {
         EGLNativeWindowType window;
         EGLNativePixmapType pixmap;
         EGLSurface surface; /* pbuffer or screen surface */
      } u;
      int width, height;
   } native;

   EGLSurface surface;

   int index;

   EGLUTreshapeCB reshape_cb;
   EGLUTdisplayCB display_cb;
   EGLUTkeyboardCB keyboard_cb;
   EGLUTspecialCB special_cb;
};

struct eglut_state {
   int api_mask;
   int window_width, window_height;
   const char *display_name;
   int verbose;
   int init_time;

   EGLUTidleCB idle_cb;

   int num_windows;

   /* initialized by native display */
   EGLNativeDisplayType native_dpy;
   EGLint surface_type;

   EGLDisplay dpy;
   EGLint major, minor;

   struct eglut_window *current;

   int redisplay;
};

extern struct eglut_state *_eglut;

void
_eglutFatal(char *format, ...);

int
_eglutNow(void);

void
_eglutNativeInitDisplay(void);

void
_eglutNativeFiniDisplay(void);

void
_eglutNativeInitWindow(struct eglut_window *win, const char *title,
                       int x, int y, int w, int h);

void
_eglutNativeFiniWindow(struct eglut_window *win);

void
_eglutNativeEventLoop(void);

#endif /* _EGLUTINT_H_ */
