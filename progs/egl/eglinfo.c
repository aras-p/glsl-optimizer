/*
 * eglinfo - like glxinfo but for EGL
 *
 * Brian Paul
 * 11 March 2005
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
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


#include <EGL/egl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CONFIGS 1000
#define MAX_MODES 1000
#define MAX_SCREENS 10


/**
 * Print table of all available configurations.
 */
static void
PrintConfigs(EGLDisplay d)
{
   EGLConfig configs[MAX_CONFIGS];
   EGLint numConfigs, i;

   eglGetConfigs(d, configs, MAX_CONFIGS, &numConfigs);

   printf("Configurations:\n");
   printf("     bf lv d st colorbuffer dp st  ms    vis   supported\n");
   printf("  id sz  l b ro  r  g  b  a th cl ns b    id   surfaces \n");
   printf("--------------------------------------------------------\n");
   for (i = 0; i < numConfigs; i++) {
      EGLint id, size, level;
      EGLint red, green, blue, alpha;
      EGLint depth, stencil;
      EGLint surfaces;
      EGLint doubleBuf = 1, stereo = 0;
      EGLint vid;
      EGLint samples, sampleBuffers;
      char surfString[100] = "";

      eglGetConfigAttrib(d, configs[i], EGL_CONFIG_ID, &id);
      eglGetConfigAttrib(d, configs[i], EGL_BUFFER_SIZE, &size);
      eglGetConfigAttrib(d, configs[i], EGL_LEVEL, &level);

      eglGetConfigAttrib(d, configs[i], EGL_RED_SIZE, &red);
      eglGetConfigAttrib(d, configs[i], EGL_GREEN_SIZE, &green);
      eglGetConfigAttrib(d, configs[i], EGL_BLUE_SIZE, &blue);
      eglGetConfigAttrib(d, configs[i], EGL_ALPHA_SIZE, &alpha);
      eglGetConfigAttrib(d, configs[i], EGL_DEPTH_SIZE, &depth);
      eglGetConfigAttrib(d, configs[i], EGL_STENCIL_SIZE, &stencil);
      eglGetConfigAttrib(d, configs[i], EGL_NATIVE_VISUAL_ID, &vid);
      eglGetConfigAttrib(d, configs[i], EGL_SURFACE_TYPE, &surfaces);

      eglGetConfigAttrib(d, configs[i], EGL_SAMPLES, &samples);
      eglGetConfigAttrib(d, configs[i], EGL_SAMPLE_BUFFERS, &sampleBuffers);

      if (surfaces & EGL_WINDOW_BIT)
         strcat(surfString, "win,");
      if (surfaces & EGL_PBUFFER_BIT)
         strcat(surfString, "pb,");
      if (surfaces & EGL_PIXMAP_BIT)
         strcat(surfString, "pix,");
#ifdef EGL_MESA_screen_surface
      if (surfaces & EGL_SCREEN_BIT_MESA)
         strcat(surfString, "scrn,");
#endif
      if (strlen(surfString) > 0)
         surfString[strlen(surfString) - 1] = 0;

      printf("0x%02x %2d %2d %c  %c %2d %2d %2d %2d %2d %2d %2d%2d  0x%02x   %-12s\n",
             id, size, level,
             doubleBuf ? 'y' : '.',
             stereo ? 'y' : '.',
             red, green, blue, alpha,
             depth, stencil,
             samples, sampleBuffers, vid, surfString);
   }
}


/**
 * Print table of all available configurations.
 */
static void
PrintModes(EGLDisplay d)
{
#ifdef EGL_MESA_screen_surface
   const char *extensions = eglQueryString(d, EGL_EXTENSIONS);
   if (strstr(extensions, "EGL_MESA_screen_surface")) {
      EGLScreenMESA screens[MAX_SCREENS];
      EGLint numScreens = 1, scrn;
      EGLModeMESA modes[MAX_MODES];

      eglGetScreensMESA(d, screens, MAX_SCREENS, &numScreens);
      printf("Number of Screens: %d\n\n", numScreens);

      for (scrn = 0; scrn < numScreens; scrn++) {
         EGLint numModes, i;

         eglGetModesMESA(d, screens[scrn], modes, MAX_MODES, &numModes);

         printf("Screen %d Modes:\n", scrn);
         printf("  id  width height refresh  name\n");
         printf("-----------------------------------------\n");
         for (i = 0; i < numModes; i++) {
            EGLint id, w, h, r;
            const char *str;
            eglGetModeAttribMESA(d, modes[i], EGL_MODE_ID_MESA, &id);
            eglGetModeAttribMESA(d, modes[i], EGL_WIDTH, &w);
            eglGetModeAttribMESA(d, modes[i], EGL_HEIGHT, &h);
            eglGetModeAttribMESA(d, modes[i], EGL_REFRESH_RATE_MESA, &r);
            str = eglQueryModeStringMESA(d, modes[i]);
            printf("0x%02x %5d  %5d   %.3f  %s\n", id, w, h, r / 1000.0, str);
         }
      }
   }
#endif
}



int
main(int argc, char *argv[])
{
   int maj, min;
   EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);

   if (!eglInitialize(d, &maj, &min)) {
      printf("eglinfo: eglInitialize failed\n");
      exit(1);
   }

   printf("EGL API version: %d.%d\n", maj, min);
   printf("EGL vendor string: %s\n", eglQueryString(d, EGL_VENDOR));
   printf("EGL version string: %s\n", eglQueryString(d, EGL_VERSION));
#ifdef EGL_VERSION_1_2
   printf("EGL client APIs: %s\n", eglQueryString(d, EGL_CLIENT_APIS));
#endif
   printf("EGL extensions string:\n");
   printf("    %s\n", eglQueryString(d, EGL_EXTENSIONS));

   PrintConfigs(d);

   PrintModes(d);

   eglTerminate(d);

   return 0;
}
