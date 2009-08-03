/*
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

/*
 * This is a port of the infamous "gears" demo to straight GLX (i.e. no GLUT)
 * Port by Brian Paul  23 March 2001
 *
 * Modified by Ian Romanick <idr@us.ibm.com> 09 April 2003 to support
 * GLX_{MESA,SGI}_swap_control and GLX_OML_sync_control.
 *
 * Command line options:
 *    -display       Name of the display to use.
 *    -info          print GL implementation information
 *    -swap N        Attempt to set the swap interval to 1/N second
 *    -forcegetrate  Get the display refresh rate even if the required GLX
 *                   extension is not supported.
 */


#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#ifndef __VMS
/*# include <stdint.h>*/
#endif
# define GLX_GLXEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glx.h>

#ifndef GLX_MESA_swap_control
typedef GLint ( * PFNGLXSWAPINTERVALMESAPROC) (unsigned interval);
typedef GLint ( * PFNGLXGETSWAPINTERVALMESAPROC) ( void );
#endif

#if !defined( GLX_OML_sync_control ) && defined( _STDINT_H )
#define GLX_OML_sync_control 1
typedef Bool ( * PFNGLXGETMSCRATEOMLPROC) (Display *dpy, GLXDrawable drawable, int32_t *numerator, int32_t *denominator);
#endif

#ifndef GLX_MESA_swap_frame_usage
#define GLX_MESA_swap_frame_usage 1
typedef int ( * PFNGLXGETFRAMEUSAGEMESAPROC) (Display *dpy, GLXDrawable drawable, float * usage );
#endif

#define BENCHMARK

PFNGLXGETFRAMEUSAGEMESAPROC get_frame_usage = NULL;

#ifdef BENCHMARK

/* XXX this probably isn't very portable */

#include <sys/time.h>
#include <unistd.h>

#define NUL '\0'

/* return current time (in seconds) */
static int
current_time(void)
{
   struct timeval tv;
#ifdef __VMS
   (void) gettimeofday(&tv, NULL );
#else
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
#endif
   return (int) tv.tv_sec;
}

#else /*BENCHMARK*/

/* dummy */
static int
current_time(void)
{
   return 0;
}

#endif /*BENCHMARK*/



#ifndef M_PI
#define M_PI 3.14159265
#endif


static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;

static GLboolean has_OML_sync_control = GL_FALSE;
static GLboolean has_SGI_swap_control = GL_FALSE;
static GLboolean has_MESA_swap_control = GL_FALSE;
static GLboolean has_MESA_swap_frame_usage = GL_FALSE;

static char ** extension_table = NULL;
static unsigned num_extensions;

static GLboolean use_ztrick = GL_FALSE;
static GLfloat aspectX = 1.0f, aspectY = 1.0f;

/*
 *
 *  Draw a gear wheel.  You'll probably want to call this function when
 *  building a display list since we do a lot of trig here.
 * 
 *  Input:  inner_radius - radius of hole at center
 *          outer_radius - radius at center of teeth
 *          width - width of gear
 *          teeth - number of teeth
 *          tooth_depth - depth of tooth
 */
static void
gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width,
     GLint teeth, GLfloat tooth_depth)
{
   GLint i;
   GLfloat r0, r1, r2;
   GLfloat angle, da;
   GLfloat u, v, len;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / teeth / 4.0;

   glShadeModel(GL_FLAT);

   glNormal3f(0.0, 0.0, 1.0);

   /* draw front face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      if (i < teeth) {
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    width * 0.5);
      }
   }
   glEnd();

   /* draw front sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
   }
   glEnd();

   glNormal3f(0.0, 0.0, -1.0);

   /* draw back face */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      if (i < teeth) {
	 glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		    -width * 0.5);
	 glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      }
   }
   glEnd();

   /* draw back sides of teeth */
   glBegin(GL_QUADS);
   da = 2.0 * M_PI / teeth / 4.0;
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
   }
   glEnd();

   /* draw outward faces of teeth */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i < teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;

      glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
      glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
      u = r2 * cos(angle + da) - r1 * cos(angle);
      v = r2 * sin(angle + da) - r1 * sin(angle);
      len = sqrt(u * u + v * v);
      u /= len;
      v /= len;
      glNormal3f(v, -u, 0.0);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
      glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 width * 0.5);
      glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		 -width * 0.5);
      u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
      v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
      glNormal3f(v, -u, 0.0);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 width * 0.5);
      glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		 -width * 0.5);
      glNormal3f(cos(angle), sin(angle), 0.0);
   }

   glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
   glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

   glEnd();

   glShadeModel(GL_SMOOTH);

   /* draw inside radius cylinder */
   glBegin(GL_QUAD_STRIP);
   for (i = 0; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / teeth;
      glNormal3f(-cos(angle), -sin(angle), 0.0);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
      glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
   }
   glEnd();
}


static void
draw(void)
{
   if ( use_ztrick ) {
      static GLboolean flip = GL_FALSE;
      static const GLfloat vert[4][3] = {
	 { -1, -1, -0.999 },
	 {  1, -1, -0.999 },
	 {  1,  1, -0.999 },
	 { -1,  1, -0.999 }
      };
      static const GLfloat col[4][3] = {
	 { 1.0, 0.6, 0.0 },
	 { 1.0, 0.6, 0.0 },
	 { 0.0, 0.0, 0.0 },
	 { 0.0, 0.0, 0.0 },
      };
      
      if ( flip ) {
	 glDepthRange(0, 0.5);
	 glDepthFunc(GL_LEQUAL);
      }
      else {
	 glDepthRange(1.0, 0.4999);
	 glDepthFunc(GL_GEQUAL);
      }

      flip = !flip;

      /* The famous Quake "Z trick" only works when the whole screen is
       * re-drawn each frame.
       */

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(-1, 1, -1, 1, -1, 1);
      glDisable(GL_LIGHTING);
      glShadeModel(GL_SMOOTH);

      glEnable( GL_VERTEX_ARRAY );
      glEnable( GL_COLOR_ARRAY );
      glVertexPointer( 3, GL_FLOAT, 0, vert );
      glColorPointer( 3, GL_FLOAT, 0, col );
      glDrawArrays( GL_POLYGON, 0, 4 );
      glDisable( GL_COLOR_ARRAY );
      glDisable( GL_VERTEX_ARRAY );

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glFrustum(-aspectX, aspectX, -aspectY, aspectY, 5.0, 60.0);

      glEnable(GL_LIGHTING);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0.0, 0.0, -45.0);
   }
   else {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   }

   glPushMatrix();
   glRotatef(view_rotx, 1.0, 0.0, 0.0);
   glRotatef(view_roty, 0.0, 1.0, 0.0);
   glRotatef(view_rotz, 0.0, 0.0, 1.0);

   glPushMatrix();
   glTranslatef(-3.0, -2.0, 0.0);
   glRotatef(angle, 0.0, 0.0, 1.0);
   glCallList(gear1);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(3.1, -2.0, 0.0);
   glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
   glCallList(gear2);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-3.1, 4.2, 0.0);
   glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
   glCallList(gear3);
   glPopMatrix();

   glPopMatrix();
}


/* new window size or exposure */
static void
reshape(int width, int height)
{
   if (width > height) {
      aspectX = (GLfloat) width / (GLfloat) height;
      aspectY = 1.0;
   }
   else {
      aspectX = 1.0;
      aspectY = (GLfloat) height / (GLfloat) width;
   }

   glViewport(0, 0, (GLint) width, (GLint) height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   glFrustum(-aspectX, aspectX, -aspectY, aspectY, 5.0, 60.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -45.0);
}


static void
init(void)
{
   static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
   static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
   static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
   static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };

   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_CULL_FACE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);

   /* make the gears */
   gear1 = glGenLists(1);
   glNewList(gear1, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
   gear(1.0, 4.0, 1.0, 20, 0.7);
   glEndList();

   gear2 = glGenLists(1);
   glNewList(gear2, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
   gear(0.5, 2.0, 2.0, 10, 0.7);
   glEndList();

   gear3 = glGenLists(1);
   glNewList(gear3, GL_COMPILE);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
   gear(1.3, 2.0, 0.5, 10, 0.7);
   glEndList();

   glEnable(GL_NORMALIZE);
}


/**
 * Remove window border/decorations.
 */
static void
no_border( Display *dpy, Window w)
{
   static const unsigned MWM_HINTS_DECORATIONS = (1 << 1);
   static const int PROP_MOTIF_WM_HINTS_ELEMENTS = 5;

   typedef struct
   {
      unsigned long       flags;
      unsigned long       functions;
      unsigned long       decorations;
      long                inputMode;
      unsigned long       status;
   } PropMotifWmHints;

   PropMotifWmHints motif_hints;
   Atom prop, proptype;
   unsigned long flags = 0;

   /* setup the property */
   motif_hints.flags = MWM_HINTS_DECORATIONS;
   motif_hints.decorations = flags;

   /* get the atom for the property */
   prop = XInternAtom( dpy, "_MOTIF_WM_HINTS", True );
   if (!prop) {
      /* something went wrong! */
      return;
   }

   /* not sure this is correct, seems to work, XA_WM_HINTS didn't work */
   proptype = prop;

   XChangeProperty( dpy, w,                         /* display, window */
                    prop, proptype,                 /* property, type */
                    32,                             /* format: 32-bit datums */
                    PropModeReplace,                /* mode */
                    (unsigned char *) &motif_hints, /* data */
                    PROP_MOTIF_WM_HINTS_ELEMENTS    /* nelements */
                  );
}


/*
 * Create an RGB, double-buffered window.
 * Return the window and context handles.
 */
static void
make_window( Display *dpy, const char *name,
             int x, int y, int width, int height, GLboolean fullscreen,
             Window *winRet, GLXContext *ctxRet)
{
   int attrib[] = { GLX_RGBA,
		    GLX_RED_SIZE, 1,
		    GLX_GREEN_SIZE, 1,
		    GLX_BLUE_SIZE, 1,
		    GLX_DOUBLEBUFFER,
		    GLX_DEPTH_SIZE, 1,
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

   if (fullscreen) {
      x = y = 0;
      width = DisplayWidth( dpy, scrnum );
      height = DisplayHeight( dpy, scrnum );
   }

   visinfo = glXChooseVisual( dpy, scrnum, attrib );
   if (!visinfo) {
      printf("Error: couldn't get an RGB, Double-buffered visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, 0, 0, width, height,
		        0, visinfo->depth, InputOutput,
		        visinfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(dpy, win, &sizehints);
      XSetStandardProperties(dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   if (fullscreen)
      no_border(dpy, win);

   ctx = glXCreateContext( dpy, visinfo, NULL, True );
   if (!ctx) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   XFree(visinfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, Window win)
{
   float  frame_usage = 0.0;

   while (1) {
      while (XPending(dpy) > 0) {
         XEvent event;
         XNextEvent(dpy, &event);
         switch (event.type) {
	 case Expose:
            /* we'll redraw below */
	    break;
	 case ConfigureNotify:
	    reshape(event.xconfigure.width, event.xconfigure.height);
	    break;
         case KeyPress:
            {
               char buffer[10];
               int r, code;
               code = XLookupKeysym(&event.xkey, 0);
               if (code == XK_Left) {
                  view_roty += 5.0;
               }
               else if (code == XK_Right) {
                  view_roty -= 5.0;
               }
               else if (code == XK_Up) {
                  view_rotx += 5.0;
               }
               else if (code == XK_Down) {
                  view_rotx -= 5.0;
               }
               else {
                  r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                    NULL, NULL);
                  if (buffer[0] == 27) {
                     /* escape */
                     return;
                  }
               }
            }
         }
      }

      /* next frame */
      angle += 2.0;

      draw();

      glXSwapBuffers(dpy, win);

      if ( get_frame_usage != NULL ) {
	 GLfloat   temp;
	 
	 (*get_frame_usage)( dpy, win, & temp );
	 frame_usage += temp;
      }

      /* calc framerate */
      {
         static int t0 = -1;
         static int frames = 0;
         int t = current_time();

         if (t0 < 0)
            t0 = t;

         frames++;

         if (t - t0 >= 5.0) {
            GLfloat seconds = t - t0;
            GLfloat fps = frames / seconds;
	    if ( get_frame_usage != NULL ) {
	       printf("%d frames in %3.1f seconds = %6.3f FPS (%3.1f%% usage)\n",
		      frames, seconds, fps,
		      (frame_usage * 100.0) / (float) frames );
	    }
	    else {
	       printf("%d frames in %3.1f seconds = %6.3f FPS\n",
		      frames, seconds, fps);
	    }

            t0 = t;
            frames = 0;
	    frame_usage = 0.0;
         }
      }
   }
}


/**
 * Display the refresh rate of the display using the GLX_OML_sync_control
 * extension.
 */
static void
show_refresh_rate( Display * dpy )
{
#if defined(GLX_OML_sync_control) && defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
   PFNGLXGETMSCRATEOMLPROC  get_msc_rate;
   int32_t  n;
   int32_t  d;

   get_msc_rate = (PFNGLXGETMSCRATEOMLPROC) glXGetProcAddressARB( (const GLubyte *) "glXGetMscRateOML" );
   if ( get_msc_rate != NULL ) {
      (*get_msc_rate)( dpy, glXGetCurrentDrawable(), &n, &d );
      printf( "refresh rate: %.1fHz\n", (float) n / d );
      return;
   }
#endif
   printf( "glXGetMscRateOML not supported.\n" );
}


/**
 * Fill in the table of extension strings from a supplied extensions string
 * (as returned by glXQueryExtensionsString).
 *
 * \param string   String of GLX extensions.
 * \sa is_extension_supported
 */
static void
make_extension_table( const char * string )
{
   char ** string_tab;
   unsigned  num_strings;
   unsigned  base;
   unsigned  idx;
   unsigned  i;
      
   /* Count the number of spaces in the string.  That gives a base-line
    * figure for the number of extension in the string.
    */
   
   num_strings = 1;
   for ( i = 0 ; string[i] != NUL ; i++ ) {
      if ( string[i] == ' ' ) {
	 num_strings++;
      }
   }
   
   string_tab = (char **) malloc( sizeof( char * ) * num_strings );
   if ( string_tab == NULL ) {
      return;
   }

   base = 0;
   idx = 0;

   while ( string[ base ] != NUL ) {
      /* Determine the length of the next extension string.
       */

      for ( i = 0 
	    ; (string[ base + i ] != NUL) && (string[ base + i ] != ' ')
	    ; i++ ) {
	 /* empty */ ;
      }

      if ( i > 0 ) {
	 /* If the string was non-zero length, add it to the table.  We
	  * can get zero length strings if there is a space at the end of
	  * the string or if there are two (or more) spaces next to each
	  * other in the string.
	  */

	 string_tab[ idx ] = malloc( sizeof( char ) * (i + 1) );
	 if ( string_tab[ idx ] == NULL ) {
	    return;
	 }

	 (void) memcpy( string_tab[ idx ], & string[ base ], i );
	 string_tab[ idx ][i] = NUL;
	 idx++;
      }


      /* Skip to the start of the next extension string.
       */

      for ( base += i
	    ; (string[ base ] == ' ') && (string[ base ] != NUL) 
	    ; base++ ) {
	 /* empty */ ;
      }
   }

   extension_table = string_tab;
   num_extensions = idx;
}

    
/**
 * Determine of an extension is supported.  The extension string table
 * must have already be initialized by calling \c make_extension_table.
 * 
 * \praram ext  Extension to be tested.
 * \return GL_TRUE of the extension is supported, GL_FALSE otherwise.
 * \sa make_extension_table
 */
static GLboolean
is_extension_supported( const char * ext )
{
   unsigned   i;
   
   for ( i = 0 ; i < num_extensions ; i++ ) {
      if ( strcmp( ext, extension_table[i] ) == 0 ) {
	 return GL_TRUE;
      }
   }
   
   return GL_FALSE;
}


int
main(int argc, char *argv[])
{
   Display *dpy;
   Window win;
   GLXContext ctx;
   char *dpyName = NULL;
   int swap_interval = 1;
   GLboolean do_swap_interval = GL_FALSE;
   GLboolean force_get_rate = GL_FALSE;
   GLboolean fullscreen = GL_FALSE;
   GLboolean printInfo = GL_FALSE;
   int i;
   PFNGLXSWAPINTERVALMESAPROC set_swap_interval = NULL;
   PFNGLXGETSWAPINTERVALMESAPROC get_swap_interval = NULL;
   int width = 300, height = 300;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0 && i + 1 < argc) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else if (strcmp(argv[i], "-swap") == 0 && i + 1 < argc) {
	 swap_interval = atoi( argv[i+1] );
	 do_swap_interval = GL_TRUE;
	 i++;
      }
      else if (strcmp(argv[i], "-forcegetrate") == 0) {
	 /* This option was put in because some DRI drivers don't support the
	  * full GLX_OML_sync_control extension, but they do support
	  * glXGetMscRateOML.
	  */
	 force_get_rate = GL_TRUE;
      }
      else if (strcmp(argv[i], "-fullscreen") == 0) {
         fullscreen = GL_TRUE;
      }
      else if (strcmp(argv[i], "-ztrick") == 0) {
	 use_ztrick = GL_TRUE;
      }
      else if (strcmp(argv[i], "-help") == 0) {
         printf("Usage:\n");
         printf("  gears [options]\n");
         printf("Options:\n");
         printf("  -help                   Print this information\n");
         printf("  -display displayName    Specify X display\n");
         printf("  -info                   Display GL information\n");
         printf("  -swap N                 Swap no more than once per N vertical refreshes\n");
         printf("  -forcegetrate           Try to use glXGetMscRateOML function\n");
         printf("  -fullscreen             Full-screen window\n");
         return 0;
      }
   }

   dpy = XOpenDisplay(dpyName);
   if (!dpy) {
      printf("Error: couldn't open display %s\n", XDisplayName(dpyName));
      return -1;
   }

   make_window(dpy, "glxgears", 0, 0, width, height, fullscreen, &win, &ctx);
   XMapWindow(dpy, win);
   glXMakeCurrent(dpy, win, ctx);

   make_extension_table( (char *) glXQueryExtensionsString(dpy,DefaultScreen(dpy)) );
   has_OML_sync_control = is_extension_supported( "GLX_OML_sync_control" );
   has_SGI_swap_control = is_extension_supported( "GLX_SGI_swap_control" );
   has_MESA_swap_control = is_extension_supported( "GLX_MESA_swap_control" );
   has_MESA_swap_frame_usage = is_extension_supported( "GLX_MESA_swap_frame_usage" );

   if ( has_MESA_swap_control ) {
      set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddressARB( (const GLubyte *) "glXSwapIntervalMESA" );
      get_swap_interval = (PFNGLXGETSWAPINTERVALMESAPROC) glXGetProcAddressARB( (const GLubyte *) "glXGetSwapIntervalMESA" );
   }
   else if ( has_SGI_swap_control ) {
      set_swap_interval = (PFNGLXSWAPINTERVALMESAPROC) glXGetProcAddressARB( (const GLubyte *) "glXSwapIntervalSGI" );
   }


   if ( has_MESA_swap_frame_usage ) {
      get_frame_usage = (PFNGLXGETFRAMEUSAGEMESAPROC)  glXGetProcAddressARB( (const GLubyte *) "glXGetFrameUsageMESA" );
   }
      

   if (printInfo) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
      if ( has_OML_sync_control || force_get_rate ) {
	 show_refresh_rate( dpy );
      }

      if ( get_swap_interval != NULL ) {
	 printf("Default swap interval = %d\n", (*get_swap_interval)() );
      }
   }

   if ( do_swap_interval ) {
      if ( set_swap_interval != NULL ) {
	 if ( ((swap_interval == 0) && !has_MESA_swap_control)
	      || (swap_interval < 0) ) {
	    printf( "Swap interval must be non-negative or greater than zero "
		    "if GLX_MESA_swap_control is not supported.\n" );
	 }
	 else {
	    (*set_swap_interval)( swap_interval );
	 }

	 if ( printInfo && (get_swap_interval != NULL) ) {
	    printf("Current swap interval = %d\n", (*get_swap_interval)() );
	 }
      }
      else {
	 printf("Unable to set swap-interval.  Neither GLX_SGI_swap_control "
		"nor GLX_MESA_swap_control are supported.\n" );
      }
   }

   init();

   /* Set initial projection/viewing transformation.
    * same as glxgears.c
    */
   reshape(width, height);

   event_loop(dpy, win);

   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);
   XCloseDisplay(dpy);

   return 0;
}
