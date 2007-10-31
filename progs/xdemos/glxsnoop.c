/**
 * Display/snoop the z/stencil/back/front buffers of another app's window.
 * Also, an example of the need for shared ancillary renderbuffers.
 *
 * Hint: use 'xwininfo' to get a window's ID.
 *
 * Brian Paul
 * 11 Oct 2007
 */

#define GL_GLEXT_PROTOTYPES

#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/keysym.h>


#define Z_BUFFER 1
#define STENCIL_BUFFER 2
#define BACK_BUFFER 3
#define FRONT_BUFFER 4


static int Buffer = BACK_BUFFER;
static int WindowID = 0;
static const char *DisplayName = NULL;
static GLXContext Context = 0;
static int Width, Height;


/**
 * Grab the z/stencil/back/front image from the srcWin and display it
 * (possibly converted to grayscale) in the dstWin.
 */
static void
redraw(Display *dpy, Window srcWin, Window dstWin )
{
   GLubyte *image = malloc(Width * Height * 4);

   glXMakeCurrent(dpy, srcWin, Context);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   if (Buffer == BACK_BUFFER) {
      glReadBuffer(GL_BACK);
      glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, image);
   }
   else if (Buffer == FRONT_BUFFER) {
      glReadBuffer(GL_FRONT);
      glReadPixels(0, 0, Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, image);
   }
   else if (Buffer == Z_BUFFER) {
      GLfloat *z = malloc(Width * Height * sizeof(GLfloat));
      int i;
      glReadPixels(0, 0, Width, Height, GL_DEPTH_COMPONENT, GL_FLOAT, z);
      for (i = 0; i < Width * Height; i++) {
         image[i*4+0] =
         image[i*4+1] =
         image[i*4+2] = (GLint) (255.0 * z[i]);
         image[i*4+3] = 255;
      }
      free(z);
   }
   else if (Buffer == STENCIL_BUFFER) {
      GLubyte *sten = malloc(Width * Height * sizeof(GLubyte));
      int i, min = 100, max = -1;
      float step;
      int sz;
      glGetIntegerv(GL_STENCIL_BITS, &sz);
      glReadPixels(0, 0, Width, Height,
                   GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, sten);
      /* find min/max for converting stencil to grayscale */
      for (i = 0; i < Width * Height; i++) {
         if (sten[i] < min)
            min = sten[i];
         if (sten[i] > max)
            max = sten[i];
      }
      if (min == max)
         step = 0;
      else
         step = 255.0 / (float) (max - min);
      for (i = 0; i < Width * Height; i++) {
         image[i*4+0] =
         image[i*4+1] =
         image[i*4+2] = (GLint) ((sten[i] - min) * step);
         image[i*4+3] = 255;
      }
      free(sten);
   }

   glXMakeCurrent(dpy, dstWin, Context);
   glWindowPos2iARB(0, 0);
   glDrawBuffer(GL_FRONT);
   glDrawPixels(Width, Height, GL_RGBA, GL_UNSIGNED_BYTE, image);
   glFlush();

   free(image);
}


static void
set_window_title(Display *dpy, Window win, const char *title)
{
   XSizeHints sizehints;
   sizehints.flags = 0;
   XSetStandardProperties(dpy, win, title, title,
                          None, (char **)NULL, 0, &sizehints);
}


static Window
make_gl_window(Display *dpy, XVisualInfo *visinfo, int width, int height)
{
   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   int x = 0, y = 0;
   char *name = NULL;

   scrnum = DefaultScreen( dpy );
   root = RootWindow( dpy, scrnum );

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( dpy, root, x, y, width, height,
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

   return win;
}


static void
update_window_title(Display *dpy, Window win)
{
   char title[1000], *buf;

   switch (Buffer) {
   case Z_BUFFER:
      buf = "Z";
      break;
   case STENCIL_BUFFER:
      buf = "Stencil";
      break;
   case BACK_BUFFER:
      buf = "Back";
      break;
   case FRONT_BUFFER:
      buf = "Front";
      break;
   default:
      buf = "";
   }

   sprintf(title, "glxsnoop window 0x%x (%s buffer)", (int) WindowID, buf);

   set_window_title(dpy, win, title);
}


static void
keypress(Display *dpy, Window win, char key)
{
   switch (key) {
   case 27:
      /* escape */
      exit(0);
      break;
   case 's':
      Buffer = STENCIL_BUFFER;
      break;
   case 'z':
      Buffer = Z_BUFFER;
      break;
   case 'f':
      Buffer = FRONT_BUFFER;
      break;
   case 'b':
      Buffer = BACK_BUFFER;
      break;
   default:
      return;
   }

   update_window_title(dpy, win);
   redraw(dpy, WindowID, win);
}


static void
event_loop(Display *dpy, Window win)
{
   XEvent event;

   while (1) {
      XNextEvent( dpy, &event );

      switch (event.type) {
      case Expose:
         redraw(dpy, WindowID, win);
         break;
      case ConfigureNotify:
         /*resize( event.xconfigure.width, event.xconfigure.height );*/
         break;
      case KeyPress:
         {
            char buffer[10];
            int r, code;
            code = XLookupKeysym(&event.xkey, 0);
            if (code == XK_Left) {
            }
            else {
               r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                                 NULL, NULL);
               keypress(dpy, win, buffer[0]);
            }
         }
      default:
         /* nothing */
         ;
      }
   }
}


static VisualID
get_window_visualid(Display *dpy, Window win)
{
   XWindowAttributes attr;

   if (XGetWindowAttributes(dpy, win, &attr)) {
      return attr.visual->visualid;
   }
   else {
      return 0;
   }
}


static void
get_window_size(Display *dpy, Window win, int *w, int *h)
{
   XWindowAttributes attr;

   if (XGetWindowAttributes(dpy, win, &attr)) {
      *w = attr.width;
      *h = attr.height;
   }
   else {
      *w = *h = 0;
   }
}


static XVisualInfo *
visualid_to_visualinfo(Display *dpy, VisualID vid)
{
   XVisualInfo *vinfo, templ;
   long mask;
   int n;

   templ.visualid = vid;
   mask = VisualIDMask;

   vinfo = XGetVisualInfo(dpy, mask, &templ, &n);
   return vinfo;
}


static void
key_usage(void)
{
   printf("Keyboard:\n");
   printf("  z - display Z buffer\n");
   printf("  s - display stencil buffer\n");
   printf("  f - display front color buffer\n");
   printf("  b - display back buffer\n");
}


static void
usage(void)
{
   printf("Usage: glxsnoop [-display dpy] windowID\n");
   key_usage();
}


static void
parse_opts(int argc, char *argv[])
{
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-h") == 0) {
         usage();
         exit(0);
      }
      else if (strcmp(argv[i], "-display") == 0) {
         DisplayName = argv[i + 1];
         i++;
      }
      else {
         if (argv[i][0] == '0' && argv[i][1] == 'x') {
            /* hex */
            WindowID = strtol(argv[i], NULL, 16);
         }
         else {
            WindowID = atoi(argv[i]);
         }
         break;
      }
   }

   if (!WindowID) {
      usage();
      exit(0);
   }
}


int
main( int argc, char *argv[] )
{
   Display *dpy;
   VisualID vid;
   XVisualInfo *visinfo;
   Window win;

   parse_opts(argc, argv);

   key_usage();

   dpy = XOpenDisplay(DisplayName);

   /* find the VisualID for the named window */
   vid = get_window_visualid(dpy, WindowID);
   get_window_size(dpy, WindowID, &Width, &Height);

   visinfo = visualid_to_visualinfo(dpy, vid);

   Context = glXCreateContext( dpy, visinfo, NULL, True );
   if (!Context) {
      printf("Error: glXCreateContext failed\n");
      exit(1);
   }

   win = make_gl_window(dpy, visinfo, Width, Height);
   XMapWindow(dpy, win);
   update_window_title(dpy, win);

   event_loop( dpy, win );

   return 0;
}
