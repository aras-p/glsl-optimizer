/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
 *
 * Based on eglgears by
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "winsys.h"

#define MAX_MODES 100

static struct {
   EGLBoolean verbose;

   EGLDisplay dpy;
   EGLConfig conf;

   EGLScreenMESA screen;
   EGLModeMESA mode;
   EGLint width, height;

   EGLContext ctx;
   EGLSurface surf;
} screen;


static EGLBoolean
init_screen(void)
{
   EGLModeMESA modes[MAX_MODES];
   EGLint num_screens, num_modes;
   EGLint width, height, best_mode;
   EGLint i;

   if (!eglGetScreensMESA(screen.dpy, &screen.screen, 1, &num_screens) ||
       !num_screens) {
      printf("eglGetScreensMESA failed\n");
      return EGL_FALSE;
   }

   if (!eglGetModesMESA(screen.dpy, screen.screen, modes, MAX_MODES,
                        &num_modes) ||
       !num_modes) {
      printf("eglGetModesMESA failed!\n");
      return EGL_FALSE;
   }

   printf("Found %d modes:\n", num_modes);

   best_mode = 0;
   width = 0;
   height = 0;
   for (i = 0; i < num_modes; i++) {
      EGLint w, h;
      eglGetModeAttribMESA(screen.dpy, modes[i], EGL_WIDTH, &w);
      eglGetModeAttribMESA(screen.dpy, modes[i], EGL_HEIGHT, &h);
      printf("%3d: %d x %d\n", i, w, h);
      if (w > width && h > height) {
         width = w;
         height = h;
         best_mode = i;
      }
   }

   screen.mode = modes[best_mode];
   screen.width = width;
   screen.height = height;

   return EGL_TRUE;
}


static EGLBoolean
init_display(void)
{
   EGLint maj, min;
   const char *exts;
   const EGLint attribs[] = {
      EGL_SURFACE_TYPE, 0x0,    /* should be EGL_SCREEN_BIT_MESA */
      EGL_RENDERABLE_TYPE, 0x0, /* should be EGL_OPENGL_ES_BIT */
      EGL_NONE
   };
   EGLint num_configs;

   screen.dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if (!screen.dpy) {
      printf("eglGetDisplay failed\n");
      return EGL_FALSE;
   }

   if (!eglInitialize(screen.dpy, &maj, &min)) {
      printf("eglInitialize failed\n");
      return EGL_FALSE;
   }

   printf("EGL_VERSION = %s\n", eglQueryString(screen.dpy, EGL_VERSION));
   printf("EGL_VENDOR = %s\n", eglQueryString(screen.dpy, EGL_VENDOR));

   exts = eglQueryString(screen.dpy, EGL_EXTENSIONS);
   assert(exts);

   if (!strstr(exts, "EGL_MESA_screen_surface")) {
      printf("EGL_MESA_screen_surface is not supported\n");
      return EGL_FALSE;
   }

   if (!eglChooseConfig(screen.dpy, attribs, &screen.conf, 1,
                        &num_configs) ||
       !num_configs) {
      printf("eglChooseConfig failed\n");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}


EGLBoolean
winsysInitScreen(void)
{
        EGLint surf_attribs[20];
        EGLint i;
        EGLBoolean ok;

        if (!init_display())
           goto fail;
        if (!init_screen())
           goto fail;

        /* create context */
	screen.ctx = eglCreateContext(screen.dpy, screen.conf,
                                      EGL_NO_CONTEXT, NULL);
	if (screen.ctx == EGL_NO_CONTEXT) {
		printf("eglCreateContext failed\n");
                goto fail;
	}

	i = 0;
	surf_attribs[i++] = EGL_WIDTH;
	surf_attribs[i++] = screen.width;
	surf_attribs[i++] = EGL_HEIGHT;
	surf_attribs[i++] = screen.height;
	surf_attribs[i++] = EGL_NONE;

        /* create surface */
        printf("Using screen size: %d x %d\n", screen.width, screen.height);
        screen.surf = eglCreateScreenSurfaceMESA(screen.dpy, screen.conf,
                                                 surf_attribs);
	if (screen.surf == EGL_NO_SURFACE) {
		printf("eglCreateScreenSurfaceMESA failed\n");
                goto fail;
	}

	ok = eglMakeCurrent(screen.dpy, screen.surf, screen.surf, screen.ctx);
	if (!ok) {
		printf("eglMakeCurrent failed\n");
		goto fail;
	}

	ok = eglShowScreenSurfaceMESA(screen.dpy, screen.screen,
                                      screen.surf, screen.mode);
	if (!ok) {
		printf("eglShowScreenSurfaceMESA failed\n");
                goto fail;
	}

        return EGL_TRUE;

fail:
        winsysFiniScreen();
        return EGL_FALSE;
}


EGLBoolean
winsysQueryScreenSize(EGLint *width, EGLint *height)
{
   if (!screen.dpy)
      return EGL_FALSE;

   if (width)
      *width = screen.width;
   if (height)
      *height = screen.height;

   return EGL_TRUE;
}


void
winsysFiniScreen(void)
{
   if (screen.dpy) {
      eglMakeCurrent(screen.dpy, EGL_NO_SURFACE, EGL_NO_SURFACE,
                     EGL_NO_CONTEXT);
      if (screen.surf != EGL_NO_SURFACE)
         eglDestroySurface(screen.dpy, screen.surf);
      if (screen.ctx != EGL_NO_CONTEXT)
         eglDestroyContext(screen.dpy, screen.ctx);
      eglTerminate(screen.dpy);

      memset(&screen, 0, sizeof(screen));
   }
}


void
winsysSwapBuffers(void)
{
   eglSwapBuffers(screen.dpy, screen.surf);
}


/* return current time (in seconds) */
double
winsysNow(void)
{
   struct timeval tv;
   gettimeofday(&tv, NULL);
   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}


void
winsysRun(double seconds, void (*draw_frame)(void *data), void *data)
{
        double begin, end, last_frame, duration;
	EGLint num_frames = 0;

        begin = winsysNow();
        end = begin + seconds;

        last_frame = begin;
        while (last_frame < end) {
           draw_frame(data);
           winsysSwapBuffers();
           last_frame = winsysNow();
           num_frames++;
        }

        duration = last_frame - begin;
	printf("%d frames in %3.1f seconds = %6.3f FPS\n",
               num_frames, duration, (double) num_frames / duration);
}
