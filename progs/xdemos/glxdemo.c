

/*
 * A demonstration of using the GLX functions.  This program is in the
 * public domain.
 *
 * Brian Paul
 */

#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>



static void redraw( Display *dpy, Window w )
{
   printf("Redraw event\n");

   glClear( GL_COLOR_BUFFER_BIT );

   glColor3f( 1.0, 1.0, 0.0 );
   glRectf( -0.8, -0.8, 0.8, 0.8 );

   glXSwapBuffers( dpy, w );
}



static void resize( unsigned int width, unsigned int height )
{
   printf("Resize event\n");
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );
}



static Window make_rgb_db_window( Display *dpy,
				  unsigned int width, unsigned int height )
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   GLXContext ctx;
   XVisualInfo *visinfo;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   visinfo = glXChooseVisual( dpy, scrnum, attrib );
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
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

   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   glXMakeCurrent( dpy, win, ctx );

   return win;
}


static void event_loop( Display *dpy )
{
   XEvent event;

   while (1) {
      XNextEvent( dpy, &event );

      switch (event.type) {
	 case Expose:
	    redraw( dpy, event.xany.window );
	    break;
	 case ConfigureNotify:
	    resize( event.xconfigure.width, event.xconfigure.height );
	    break;
      }
   }
}



int main( int argc, char *argv[] )
{
   Display *dpy;
   Window win;

   dpy = XOpenDisplay(NULL);

   win = make_rgb_db_window( dpy, 300, 300 );

   glShadeModel( GL_FLAT );
   glClearColor( 0.5, 0.5, 0.5, 1.0 );

   XMapWindow( dpy, win );

   event_loop( dpy );
   return 0;
}
