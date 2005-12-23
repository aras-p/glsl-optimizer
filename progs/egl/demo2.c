/*
 * Exercise EGL API functions
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <GLES/egl.h>

/*#define FRONTBUFFER*/

static void _subset_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   glBegin( GL_QUADS );
   glVertex2f( x1, y1 );
   glVertex2f( x2, y1 );
   glVertex2f( x2, y2 );
   glVertex2f( x1, y2 );
   glEnd();
}


static void redraw(EGLDisplay dpy, EGLSurface surf, int rot)
{
   printf("Redraw event\n");

#ifdef FRONTBUFFER
    glDrawBuffer( GL_FRONT ); 
#else
    glDrawBuffer( GL_BACK );
#endif

   glClearColor( rand()/(float)RAND_MAX, 
		 rand()/(float)RAND_MAX, 
		 rand()/(float)RAND_MAX,
		 1);

   glClear( GL_COLOR_BUFFER_BIT ); 

   glColor3f( rand()/(float)RAND_MAX, 
	      rand()/(float)RAND_MAX, 
	      rand()/(float)RAND_MAX );
   glPushMatrix();
   glRotatef(rot, 0, 0, 1);
   glScalef(.5, .5, .5);
   _subset_Rectf( -1, -1, 1, 1 );
   glPopMatrix();

#ifdef FRONTBUFFER
   glFlush();
#else
   eglSwapBuffers( dpy, surf ); 
#endif
   glFinish();
}


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
   EGLSurface pbuffer, screen_surf;
   EGLConfig configs[10];
   EGLint numConfigs, i;
   EGLBoolean b;
   const EGLint pbufAttribs[] = {
      EGL_WIDTH, 500,
      EGL_HEIGHT, 500,
      EGL_NONE
   };
   const EGLint screenAttribs[] = {
      EGL_WIDTH, 1024,
      EGL_HEIGHT, 768,
      EGL_NONE
   };
   EGLModeMESA mode;
   EGLScreenMESA screen;
   EGLint count;

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
   
   eglGetScreensMESA(d, &screen, 1, &count);
   eglGetModesMESA(d, screen, &mode, 1, &count);

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

   screen_surf = eglCreateScreenSurfaceMESA(d, configs[0], screenAttribs);
   if (screen_surf == EGL_NO_SURFACE) {
      printf("failed to create screen surface\n");
      return 0;
   }
   
   eglShowScreenSurfaceMESA(d, screen, screen_surf, mode);

   b = eglMakeCurrent(d, screen_surf, screen_surf, ctx);
   if (!b) {
      printf("make current failed\n");
      return 0;
   }

   glViewport(0, 0, 1024, 768);
   glDrawBuffer( GL_FRONT ); 

   glClearColor( 0, 
		 1.0, 
		 0,
		 1);

   glClear( GL_COLOR_BUFFER_BIT ); 
   
      
   TestScreens(d);

   glShadeModel( GL_FLAT );
   
   for (i = 0; i < 6; i++) {
      redraw(d, screen_surf, i*10 );

      printf("sleep(1)\n");   
      sleep(1);  
   }

   eglDestroySurface(d, pbuffer);
   eglDestroyContext(d, ctx);
   eglTerminate(d);

   return 0;
}
