/*
 * Exercise EGL API functions
 */

#include <GLES/egl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


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



int
main(int argc, char *argv[])
{
   int maj, min;
   EGLContext ctx;
   EGLSurface pbuffer;
   EGLConfig configs[10];
   EGLint numConfigs, i;
   EGLBoolean b;
   const EGLint pbufAttribs[] = {
      EGL_WIDTH, 500,
      EGL_HEIGHT, 500,
      EGL_NONE
   };

   /*
   EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   */
   EGLDisplay d = eglGetDisplay("!fb_dri");
   assert(d);

   if (!eglInitialize(d, &maj, &min)) {
      printf("demo: eglInitialize failed\n");
      exit(1);
   }

   printf("EGL version = %d.%d\n", maj, min);
   printf("EGL_VENDOR = %s\n", eglQueryString(d, EGL_VENDOR));

   eglGetConfigs(d, configs, 10, &numConfigs);
   printf("Got %d EGL configs:\n", numConfigs);
   for (i = 0; i < numConfigs; i++) {
      EGLint id, red, depth;
      eglGetConfigAttrib(d, configs[i], EGL_CONFIG_ID, &id);
      eglGetConfigAttrib(d, configs[i], EGL_RED_SIZE, &red);
      eglGetConfigAttrib(d, configs[i], EGL_DEPTH_SIZE, &depth);
      printf("%2d:  Red Size = %d  Depth Size = %d\n", id, red, depth);
   }

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
