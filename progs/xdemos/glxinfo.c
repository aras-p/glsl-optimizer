/* $Id: glxinfo.c,v 1.1 1999/09/16 16:40:46 brianp Exp $ */


/*
 * Query GLX extensions, version, vendor, etc.
 * This program is in the public domain.
 * brian_paul@mesa3d.org
 */



#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <stdio.h>
#include <stdlib.h>



static void
query_glx( Display *dpy, int scr )
{
   printf("GL_VERSION: %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_EXTENSIONS: %s\n", (char *) glGetString(GL_EXTENSIONS));
   printf("GL_RENDERER: %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VENDOR: %s\n", (char *) glGetString(GL_VENDOR));
   printf("GLU_VERSION: %s\n", (char *) gluGetString(GLU_VERSION));
   printf("GLU_EXTENSIONS: %s\n", (char *) gluGetString(GLU_EXTENSIONS));

   printf("server GLX_VENDOR: %s\n", (char *) glXQueryServerString( dpy, scr, GLX_VENDOR));
   printf("server GLX_VERSION: %s\n", (char *) glXQueryServerString( dpy, scr, GLX_VERSION));
   printf("server GLX_EXTENSIONS: %s\n", (char *) glXQueryServerString( dpy, scr, GLX_EXTENSIONS));

   printf("client GLX_VENDOR: %s\n", (char *) glXGetClientString( dpy, GLX_VENDOR));
   printf("client GLX_VERSION: %s\n", (char *) glXGetClientString( dpy, GLX_VERSION));
   printf("client GLX_EXTENSIONS: %s\n", (char *) glXGetClientString( dpy, GLX_EXTENSIONS));

   printf("GLX extensions: %s\n", (char *) glXQueryExtensionsString(dpy, scr));
}




int
main( int argc, char *argv[] )
{
   Display *dpy;
   Window win;
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    None };
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   GLXContext ctx;
   XVisualInfo *visinfo;
   int width = 100, height = 100;

   dpy = XOpenDisplay(NULL);
   if (!dpy) {
      fprintf(stderr, "Unable to open default display!\n");
      return 1;
   }

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   visinfo = glXChooseVisual( dpy, scrnum, attrib );
   if (!visinfo) {
      fprintf(stderr, "Error: couldn't find RGB GLX visual!\n");
      return 1;
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

   glXMakeCurrent( dpy, win, ctx );

   query_glx(dpy, scrnum);

   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return 0;
}
