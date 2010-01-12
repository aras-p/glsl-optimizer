/*
 * Exercise EGL API functions
 */

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/**
 * Test EGL_MESA_screen_surface functions
 */
static void
TestScreens(EGLDisplay dpy)
{
#define MAX 8
   EGLScreenMESA screens[MAX];
   EGLint numScreens;
   EGLint i;

   eglGetScreensMESA(dpy, screens, MAX, &numScreens);
   printf("Found %d screens\n", numScreens);
   for (i = 0; i < numScreens; i++) {
      printf(" Screen %d handle: %d\n", i, (int) screens[i]);
   }
}

/**
 * Print table of all available configurations.
 */
static void
PrintConfigs(EGLDisplay d, EGLConfig *configs, EGLint numConfigs)
{
   EGLint i;

   printf("Configurations:\n");
   printf("     bf lv d st colorbuffer dp st   supported \n");
   printf("  id sz  l b ro  r  g  b  a th cl   surfaces  \n");
   printf("----------------------------------------------\n");
   for (i = 0; i < numConfigs; i++) {
      EGLint id, size, level;
      EGLint red, green, blue, alpha;
      EGLint depth, stencil;
      EGLint surfaces;
      EGLint doubleBuf = 1, stereo = 0;
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
      eglGetConfigAttrib(d, configs[i], EGL_SURFACE_TYPE, &surfaces);

      if (surfaces & EGL_WINDOW_BIT)
         strcat(surfString, "win,");
      if (surfaces & EGL_PBUFFER_BIT)
         strcat(surfString, "pb,");
      if (surfaces & EGL_PIXMAP_BIT)
         strcat(surfString, "pix,");
      if (strlen(surfString) > 0)
         surfString[strlen(surfString) - 1] = 0;

      printf("0x%02x %2d %2d %c  %c %2d %2d %2d %2d %2d %2d   %-12s\n",
             id, size, level,
             doubleBuf ? 'y' : '.',
             stereo ? 'y' : '.',
             red, green, blue, alpha,
             depth, stencil, surfString);
   }
}



int
main(int argc, char *argv[])
{
   int maj, min;
   EGLContext ctx;
   EGLSurface pbuffer;
   EGLConfig *configs;
   EGLint numConfigs;
   EGLBoolean b;
   const EGLint pbufAttribs[] = {
      EGL_WIDTH, 500,
      EGL_HEIGHT, 500,
      EGL_NONE
   };

   EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   assert(d);

   if (!eglInitialize(d, &maj, &min)) {
      printf("demo: eglInitialize failed\n");
      exit(1);
   }

   printf("EGL version = %d.%d\n", maj, min);
   printf("EGL_VENDOR = %s\n", eglQueryString(d, EGL_VENDOR));

   eglGetConfigs(d, NULL, 0, &numConfigs);
   configs = malloc(sizeof(*configs) *numConfigs);
   eglGetConfigs(d, configs, numConfigs, &numConfigs);

   PrintConfigs(d, configs, numConfigs);

   eglBindAPI(EGL_OPENGL_API);
   ctx = eglCreateContext(d, configs[0], EGL_NO_CONTEXT, NULL);
   if (ctx == EGL_NO_CONTEXT) {
      printf("failed to create context\n");
      return 0;
   }

   pbuffer = eglCreatePbufferSurface(d, configs[0], pbufAttribs);
   if (pbuffer == EGL_NO_SURFACE) {
      printf("failed to create pbuffer\n");
      return 0;
   }

   free(configs);

   b = eglMakeCurrent(d, pbuffer, pbuffer, ctx);
   if (!b) {
      printf("make current failed\n");
      return 0;
   }

   b = eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

   TestScreens(d);

   eglDestroySurface(d, pbuffer);
   eglDestroyContext(d, ctx);
   eglTerminate(d);

   return 0;
}
