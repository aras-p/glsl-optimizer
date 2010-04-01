#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#define EGL_EGLEXT_PROTOTYPES
#include "EGL/egl.h"
#include "EGL/eglext.h"

#include "eglutint.h"

#define MAX_MODES 100

static EGLScreenMESA kms_screen;
static EGLModeMESA kms_mode;
static EGLint kms_width, kms_height;

void
_eglutNativeInitDisplay(void)
{
   _eglut->native_dpy = EGL_DEFAULT_DISPLAY;
   _eglut->surface_type = EGL_SCREEN_BIT_MESA;
}

void
_eglutNativeFiniDisplay(void)
{
   kms_screen = 0;
   kms_mode = 0;
   kms_width = 0;
   kms_height = 0;
}

static void
init_kms(void)
{
   EGLModeMESA modes[MAX_MODES];
   EGLint num_screens, num_modes;
   EGLint width, height, best_mode;
   EGLint i;

   if (!eglGetScreensMESA(_eglut->dpy, &kms_screen, 1, &num_screens) ||
         !num_screens)
      _eglutFatal("eglGetScreensMESA failed\n");

   if (!eglGetModesMESA(_eglut->dpy, kms_screen,
            modes, MAX_MODES, &num_modes) || !num_modes)
      _eglutFatal("eglGetModesMESA failed!\n");

   printf("Found %d modes:\n", num_modes);

   best_mode = 0;
   width = 0;
   height = 0;
   for (i = 0; i < num_modes; i++) {
      EGLint w, h;
      eglGetModeAttribMESA(_eglut->dpy, modes[i], EGL_WIDTH, &w);
      eglGetModeAttribMESA(_eglut->dpy, modes[i], EGL_HEIGHT, &h);
      printf("%3d: %d x %d\n", i, w, h);
      if (w > width && h > height) {
         width = w;
         height = h;
         best_mode = i;
      }
   }

   printf("Will use screen size: %d x %d\n", width, height);

   kms_mode = modes[best_mode];
   kms_width = width;
   kms_height = height;
}

void
_eglutNativeInitWindow(struct eglut_window *win, const char *title,
                       int x, int y, int w, int h)
{
   EGLint surf_attribs[16];
   EGLint i;
   const char *exts;

   exts = eglQueryString(_eglut->dpy, EGL_EXTENSIONS);
   if (!exts || !strstr(exts, "EGL_MESA_screen_surface"))
      _eglutFatal("EGL_MESA_screen_surface is not supported\n");

   init_kms();

   i = 0;
   surf_attribs[i++] = EGL_WIDTH;
   surf_attribs[i++] = kms_width;
   surf_attribs[i++] = EGL_HEIGHT;
   surf_attribs[i++] = kms_height;
   surf_attribs[i++] = EGL_NONE;

   /* create surface */
   win->native.u.surface = eglCreateScreenSurfaceMESA(_eglut->dpy,
         win->config, surf_attribs);
   if (win->native.u.surface == EGL_NO_SURFACE)
      _eglutFatal("eglCreateScreenSurfaceMESA failed\n");

   if (!eglShowScreenSurfaceMESA(_eglut->dpy, kms_screen,
            win->native.u.surface, kms_mode))
      _eglutFatal("eglShowScreenSurfaceMESA failed\n");

   win->native.width = kms_width;
   win->native.height = kms_height;
}

void
_eglutNativeFiniWindow(struct eglut_window *win)
{
   eglShowScreenSurfaceMESA(_eglut->dpy,
         kms_screen, EGL_NO_SURFACE, 0);
   eglDestroySurface(_eglut->dpy, win->native.u.surface);
}

void
_eglutNativeEventLoop(void)
{
   int start = _eglutNow();
   int frames = 0;

   _eglut->redisplay = 1;

   while (1) {
      struct eglut_window *win = _eglut->current;
      int now = _eglutNow();

      if (now - start > 5000) {
         double elapsed = (double) (now - start) / 1000.0;

         printf("%d frames in %3.1f seconds = %6.3f FPS\n",
               frames, elapsed, frames / elapsed);

         start = now;
         frames = 0;

         /* send escape */
         if (win->keyboard_cb)
            win->keyboard_cb(27);
      }

      if (_eglut->idle_cb)
         _eglut->idle_cb();

      if (_eglut->redisplay) {
         _eglut->redisplay = 0;

         if (win->display_cb)
            win->display_cb();
         eglSwapBuffers(_eglut->dpy, win->surface);
         frames++;
      }
   }
}
