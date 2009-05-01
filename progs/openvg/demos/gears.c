#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <VG/openvg.h>
#include <GLES/egl.h>

static VGint width, height;
static VGPath gear1;
static VGPath gear2;
static VGPath gear3;

static VGPaint fill;
const VGfloat color[4] = {0.5, 0.5, 0.5, 1.0};

static VGfloat gear1_angle = 35;
static VGfloat gear2_angle = 24;
static VGfloat gear3_angle = 33.5;

static  void moveTo(VGPath path, VGfloat x, VGfloat y)
{
   static VGubyte moveTo = VG_MOVE_TO | VG_ABSOLUTE;
   VGfloat pathData[2];
   pathData[0] = x; pathData[1] = y;
   vgAppendPathData(path, 1, &moveTo, pathData);
}

static  void lineTo(VGPath path, VGfloat x, VGfloat y)
{
   static VGubyte lineTo = VG_LINE_TO | VG_ABSOLUTE;
   VGfloat pathData[2];
   pathData[0] = x; pathData[1] = y;
   vgAppendPathData(path, 1, &lineTo, pathData);
}

static  void closeSubpath(VGPath path)
{
   static VGubyte close = VG_CLOSE_PATH | VG_ABSOLUTE;
   VGfloat pathData[2];
   vgAppendPathData(path, 1, &close, pathData);
}

static  void cubicTo(VGPath path, VGfloat x1, VGfloat y1, VGfloat x2, VGfloat y2,
                     VGfloat midx, VGfloat midy)
{
   static VGubyte cubic = VG_CUBIC_TO | VG_ABSOLUTE;
   VGfloat pathData[6];
   pathData[0] = x1;
   pathData[1] = y1;
   pathData[2] = x2;
   pathData[3] = y2;
   pathData[4] = midx;
   pathData[5] = midy;
   vgAppendPathData(path, 1, &cubic, pathData);
}

static VGPath gearsPath(double inner_radius, double outer_radius,
                        int teeth, double tooth_depth)
{
   VGPath path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f,
                              0, 0, (unsigned int)VG_PATH_CAPABILITY_ALL);

   int i;
   double r0, r1, r2;
   double angle, da;

   r0 = inner_radius;
   r1 = outer_radius - tooth_depth / 2.0;
   r2 = outer_radius + tooth_depth / 2.0;

   da = 2.0 * M_PI / (VGfloat) teeth / 4.0;

   angle = 0.0;
   moveTo(path, r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da));

   for (i = 1; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / (VGfloat)teeth;

      lineTo(path, r1 * cos(angle), r1 * sin(angle));
      lineTo(path, r2 * cos(angle + da), r2 * sin(angle + da));
      lineTo(path, r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da));

      if (i < teeth)
         lineTo(path, r1 * cos(angle + 3 * da),
                r1 * sin(angle + 3 * da));
   }

   closeSubpath(path);

   moveTo(path, r0 * cos(angle + 3 * da), r0 * sin(angle + 3 * da));

   for (i = 1; i <= teeth; i++) {
      angle = i * 2.0 * M_PI / (VGfloat) teeth;

      lineTo(path, r0 * cos(angle), r0 * sin(angle));
   }

   closeSubpath(path);
   return path;
}

static void
draw(void)
{
   vgClear(0, 0, width, height);

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);

   vgLoadIdentity();
   vgLoadIdentity();
   vgTranslate(170, 330);
   vgRotate(gear1_angle);
   vgDrawPath(gear1, VG_FILL_PATH);

   vgLoadIdentity();
   vgTranslate(369, 330);
   vgRotate(gear2_angle);
   vgDrawPath(gear2, VG_FILL_PATH);

   vgLoadIdentity();
   vgTranslate(170, 116);
   vgRotate(gear3_angle);
   vgDrawPath(gear3, VG_FILL_PATH);

   gear1_angle += 1;
   gear2_angle -= (20.0 / 12.0);
   gear3_angle -= (20.0 / 14.0);
}


/* new window size or exposure */
static void
reshape(int w, int h)
{
   width  = w;
   height = h;
}


static void
init(void)
{
   float clear_color[4] = {1.0, 1.0, 1.0, 1.0};
   vgSetfv(VG_CLEAR_COLOR, 4, clear_color);

   gear1 = gearsPath(30.0, 120.0, 20, 20.0);
   gear2 = gearsPath(15.0, 75.0, 12, 20.0);
   gear3 = gearsPath(20.0, 90.0, 14, 20.0);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, color);
   vgSetPaint(fill, VG_FILL_PATH);
}


/*
 * Create an RGB, double-buffered X window.
 * Return the window and context handles.
 */
static void
make_x_window(Display *x_dpy, EGLDisplay egl_dpy,
              const char *name,
              int x, int y, int width, int height,
              Window *winRet,
              EGLContext *ctxRet,
              EGLSurface *surfRet)
{
   static const EGLint attribs[] = {
      EGL_RED_SIZE, 1,
      EGL_GREEN_SIZE, 1,
      EGL_BLUE_SIZE, 1,
      EGL_NONE
   };

   int scrnum;
   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   Window win;
   XVisualInfo *visInfo, visTemplate;
   int num_visuals;
   EGLContext ctx;
   EGLConfig config;
   EGLint num_configs;
   EGLint vid;

   scrnum = DefaultScreen( x_dpy );
   root = RootWindow( x_dpy, scrnum );

   if (!eglChooseConfig( egl_dpy, attribs, &config, 1, &num_configs)) {
      printf("Error: couldn't get an EGL visual config\n");
      exit(1);
   }

   assert(config);
   assert(num_configs > 0);

   if (!eglGetConfigAttrib(egl_dpy, config, EGL_NATIVE_VISUAL_ID, &vid)) {
      printf("Error: eglGetConfigAttrib() failed\n");
      exit(1);
   }

   /* The X window visual must match the EGL config */
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo(x_dpy, VisualIDMask, &visTemplate, &num_visuals);
   if (!visInfo) {
      printf("Error: couldn't get X visual\n");
      exit(1);
   }

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap( x_dpy, root, visInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   win = XCreateWindow( x_dpy, root, 0, 0, width, height,
		        0, visInfo->depth, InputOutput,
		        visInfo->visual, mask, &attr );

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(x_dpy, win, &sizehints);
      XSetStandardProperties(x_dpy, win, name, name,
                              None, (char **)NULL, 0, &sizehints);
   }

   eglBindAPI(EGL_OPENVG_API);

   ctx = eglCreateContext(egl_dpy, config, EGL_NO_CONTEXT, NULL );
   if (!ctx) {
      printf("Error: eglCreateContext failed\n");
      exit(1);
   }

   *surfRet = eglCreateWindowSurface(egl_dpy, config, win, NULL);

   if (!*surfRet) {
      printf("Error: eglCreateWindowSurface failed\n");
      exit(1);
   }

   XFree(visInfo);

   *winRet = win;
   *ctxRet = ctx;
}


static void
event_loop(Display *dpy, Window win,
           EGLDisplay egl_dpy, EGLSurface egl_surf)
{
   while (1) {
      XEvent event;

      while (XPending(dpy) > 0) {
         XNextEvent(dpy, &event);

         switch (event.type) {
         case Expose:
            break;
         case ConfigureNotify:
            reshape(event.xconfigure.width, event.xconfigure.height);
            break;
         case KeyPress:
         {
            char buffer[10];
            int r, code;
            code = XLookupKeysym(&event.xkey, 0);
            r = XLookupString(&event.xkey, buffer, sizeof(buffer),
                              NULL, NULL);
            if (buffer[0] == 27) {
               /* escape */
               return;
            }
         }
         break;
         default:
            ; /*no-op*/
         }
      }

      draw();
      eglSwapBuffers(egl_dpy, egl_surf);
   }
}


static void
usage(void)
{
   printf("Usage:\n");
   printf("  -display <displayname>  set the display to run on\n");
   printf("  -info                   display OpenGL renderer info\n");
}

int
main(int argc, char *argv[])
{
   const int winWidth = 500, winHeight = 500;
   Display *x_dpy;
   Window win;
   EGLSurface egl_surf;
   EGLContext egl_ctx;
   EGLDisplay egl_dpy;
   char *dpyName = NULL;
   GLboolean printInfo = GL_FALSE;
   EGLint egl_major, egl_minor;
   int i;
   const char *s;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0) {
         dpyName = argv[i+1];
         i++;
      }
      else if (strcmp(argv[i], "-info") == 0) {
         printInfo = GL_TRUE;
      }
      else {
         usage();
         return -1;
      }
   }

   x_dpy = XOpenDisplay(dpyName);
   if (!x_dpy) {
      printf("Error: couldn't open display %s\n",
	     dpyName ? dpyName : getenv("DISPLAY"));
      return -1;
   }

   egl_dpy = eglGetDisplay(x_dpy);
   if (!egl_dpy) {
      printf("Error: eglGetDisplay() failed\n");
      return -1;
   }

   if (!eglInitialize(egl_dpy, &egl_major, &egl_minor)) {
      printf("Error: eglInitialize() failed\n");
      return -1;
   }

   s = eglQueryString(egl_dpy, EGL_VERSION);
   printf("EGL_VERSION = %s\n", s);

   make_x_window(x_dpy, egl_dpy,
                 "xegl_tri", 0, 0, winWidth, winHeight,
                 &win, &egl_ctx, &egl_surf);

   XMapWindow(x_dpy, win);
   if (!eglMakeCurrent(egl_dpy, egl_surf, egl_surf, egl_ctx)) {
      printf("Error: eglMakeCurrent() failed\n");
      return -1;
   }

   if (printInfo) {
      printf("VG_RENDERER   = %s\n", (char *) vgGetString(VG_RENDERER));
      printf("VG_VERSION    = %s\n", (char *) vgGetString(VG_VERSION));
      printf("VG_VENDOR     = %s\n", (char *) vgGetString(VG_VENDOR));
   }

   init();

   /* Set initial projection/viewing transformation.
    * We can't be sure we'll get a ConfigureNotify event when the window
    * first appears.
    */
   reshape(winWidth, winHeight);

   event_loop(x_dpy, win, egl_dpy, egl_surf);

   eglDestroyContext(egl_dpy, egl_ctx);
   eglDestroySurface(egl_dpy, egl_surf);
   eglTerminate(egl_dpy);


   XDestroyWindow(x_dpy, win);
   XCloseDisplay(x_dpy);

   return 0;
}
