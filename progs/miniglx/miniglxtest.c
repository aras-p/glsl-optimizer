/* $Id: miniglxtest.c,v 1.3 2004/03/25 14:58:39 brianp Exp $ */

/*
 * Test the mini GLX interface.
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <GL/gl.h>
#define USE_MINI_GLX 1
#if USE_MINI_GLX
#include <GL/miniglx.h>
#else
#include <GL/glx.h>
#endif

#define FRONTBUFFER 1
#define NR          6
#define DO_SLEEPS   1
#define NR_DISPLAYS 2

GLXContext ctx;


static void _subset_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   glBegin( GL_QUADS );
   glVertex2f( x1, y1 );
   glVertex2f( x2, y1 );
   glVertex2f( x2, y2 );
   glVertex2f( x1, y2 );
   glEnd();
}



static void redraw( Display *dpy, Window w, int rot )
{
   printf("Redraw event\n");

#if FRONTBUFFER
    glDrawBuffer( GL_FRONT ); 
#else
/*     glDrawBuffer( GL_BACK );    */
#endif

   glClearColor( rand()/(float)RAND_MAX, 
		 rand()/(float)RAND_MAX, 
		 rand()/(float)RAND_MAX,
		 1);

   glClear( GL_COLOR_BUFFER_BIT ); 

#if 1
   glColor3f( rand()/(float)RAND_MAX, 
	      rand()/(float)RAND_MAX, 
	      rand()/(float)RAND_MAX );
   glPushMatrix();
   glRotatef(rot, 0, 0, 1);
   glScalef(.5, .5, .5);
   _subset_Rectf( -1, -1, 1, 1 );
   glPopMatrix();
#endif

#if FRONTBUFFER
   glFlush();
#else
   glXSwapBuffers( dpy, w ); 
#endif
   glFinish();
}


static Window make_rgb_db_window( Display *dpy,
				  unsigned int width, unsigned int height )
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
#if !FRONTBUFFER
 		    GLX_DOUBLEBUFFER, 
#endif
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   XVisualInfo *visinfo;

   scrnum = 0;
   root = RootWindow( dpy, scrnum );

   if (!(visinfo = glXChooseVisual( dpy, scrnum, attrib ))) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   if(!(ctx = glXCreateContext( dpy, visinfo, NULL, True ))) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );
   if (!win) {
      printf("Error: XCreateWindow failed\n");
      exit(1);
   }

   glXMakeCurrent( dpy, win, ctx );

   glViewport(0, 0, width, height);

   return win;
}


static void event_loop( Display *dpy, Window win )
{
   int i;

   printf("Hang on... drawing %d frames\n", NR);
   for (i = 0; i < NR; i++) {
      redraw( dpy, win, i*10 );
      if (DO_SLEEPS) {
	 printf("sleep(1)\n");   
	 sleep(1);  
      }
   }
}


static int foo( void )
{
   Display *dpy;
   Window win;

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      printf("Error: XOpenDisplay failed\n");
      return 1;
   }

   win = make_rgb_db_window( dpy, 800, 600);

   srand(getpid());

   glShadeModel( GL_FLAT );
   glClearColor( 0.5, 0.5, 0.5, 1.0 );

   XMapWindow( dpy, win );

   {
      XEvent e;
      while (1) {
	 XNextEvent( dpy, &e );
	 if (e.type == MapNotify && e.xmap.window == win) {
	    break;
	 }
      }
   }

   event_loop( dpy, win );

   glXDestroyContext( dpy, ctx );
   XDestroyWindow( dpy, win );

   XCloseDisplay( dpy );

   return 0;
}


int main()
{
   int i;
   for (i = 0 ; i < NR_DISPLAYS ; i++) {
      if (foo() != 0)
	 break;
   }

   return 0;
}
