/* $Id: glxpixmap.c,v 1.1 1999/08/19 00:55:43 jtg Exp $ */


/*
 * A demonstration of using the GLXPixmap functions.  This program is in
 * the public domain.
 *
 * Brian Paul
 */


/*
 * $Id: glxpixmap.c,v 1.1 1999/08/19 00:55:43 jtg Exp $
 */


#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>



static GLXContext ctx;
static XVisualInfo *visinfo;
static GC gc;



static Window make_rgb_window( Display *dpy,
				  unsigned int width, unsigned int height )
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;

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
   /* TODO: share root colormap if possible */
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   /* make an X GC so we can do XCopyArea later */
   gc = XCreateGC( dpy, win, 0, NULL );

   ctx = glXCreateContext( dpy, visinfo, NULL, True );

   return win;
}


static GLXPixmap make_pixmap( Display *dpy, Window win,
			       unsigned int width, unsigned int height )
{
   Pixmap pm;
   GLXPixmap glxpm;
   XWindowAttributes attr;

   pm = XCreatePixmap( dpy, win, width, height, visinfo->depth );
   XGetWindowAttributes( dpy, win, &attr );

   /*
    * IMPORTANT:
    *   Use the glXCreateGLXPixmapMESA funtion when using Mesa because
    *   Mesa needs to know the colormap associated with a pixmap in order
    *   to render correctly.  This is because Mesa allows RGB rendering
    *   into any kind of visual, not just TrueColor or DirectColor.
    */
#ifdef GLX_MESA_pixmap_colormap
   glxpm = glXCreateGLXPixmapMESA( dpy, visinfo, pm, attr.colormap );
#else
   /* This will work with Mesa too if the visual is TrueColor or DirectColor */
   glxpm = glXCreateGLXPixmap( dpy, visinfo, pm );
#endif

   return glxpm;
}



static void event_loop( Display *dpy, GLXPixmap pm )
{
   XEvent event;

   while (1) {
      XNextEvent( dpy, &event );

      switch (event.type) {
	 case Expose:
	    printf("Redraw\n");
	    /* copy the image from GLXPixmap to window */
	    XCopyArea( dpy, pm, event.xany.window,  /* src, dest */
		       gc, 0, 0, 300, 300,          /* gc, src pos, size */
		       0, 0 );                      /* dest pos */
	    break;
	 case ConfigureNotify:
	    /* nothing */
	    break;
      }
   }
}



int main( int argc, char *argv[] )
{
   Display *dpy;
   Window win;
   GLXPixmap pm;

   dpy = XOpenDisplay(NULL);

   win = make_rgb_window( dpy, 300, 300 );
   pm = make_pixmap( dpy, win, 300, 300 );

#ifdef JUNK
   glXMakeCurrent( dpy, win, ctx );  /*to make sure ctx is properly initialized*/
#endif

   glXMakeCurrent( dpy, pm, ctx );

   /* Render an image into the pixmap */
   glShadeModel( GL_FLAT );
   glClearColor( 0.5, 0.5, 0.5, 1.0 );
   glClear( GL_COLOR_BUFFER_BIT );
   glViewport( 0, 0, 300, 300 );
   glOrtho( -1.0, 1.0, -1.0, 1.0, -1.0, 1.0 );
   glColor3f( 0.0, 1.0, 1.0 );
   glRectf( -0.75, -0.75, 0.75, 0.75 );
   glFlush();

   /* when a redraw is needed we'll just copy the pixmap image to the window */

   XMapWindow( dpy, win );

   event_loop( dpy, pm );
   return 0;
}
