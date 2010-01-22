#include <stdlib.h>
#include <sys/time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "segl.h"

static Window
x11_create_window(struct segl_winsys *winsys, const char *name,
                  EGLint width, EGLint height, EGLint visual)
{
   XVisualInfo vinfo_template, *vinfo = NULL;
   EGLint val, num_vinfo;
   Window root, win;
   XSetWindowAttributes attrs;
   unsigned long mask;
   EGLint x = 0, y = 0;

   vinfo_template.visualid = (VisualID) val;
   vinfo = XGetVisualInfo(winsys->dpy, VisualIDMask, &vinfo_template, &num_vinfo);
   if (!num_vinfo) {
      if (vinfo)
         XFree(vinfo);
      return None;
   }

   root = DefaultRootWindow(winsys->dpy);

   /* window attributes */
   attrs.background_pixel = 0;
   attrs.border_pixel = 0;
   attrs.colormap = XCreateColormap(winsys->dpy, root, vinfo->visual, AllocNone);
   attrs.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   attrs.override_redirect = False;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect;

   win = XCreateWindow(winsys->dpy, root, x, y, width, height, 0,
         vinfo->depth, InputOutput, vinfo->visual, mask, &attrs);
   XFree(vinfo);

   if (!win)
      return None;

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = width;
      sizehints.height = height;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(winsys->dpy, win, &sizehints);
      XSetStandardProperties(winsys->dpy, win, name, name,
                             None, (char **)NULL, 0, &sizehints);
   }

   XMapWindow(winsys->dpy, win);

   return win;
}

static void
x11_destroy_window(struct segl_winsys *winsys, Window win)
{
   if (win)
      XDestroyWindow(winsys->dpy, win);
}


static Pixmap 
x11_create_pixmap(struct segl_winsys *winsys, EGLint width, EGLint height,
                  EGLint depth)
{
   Window root = DefaultRootWindow(winsys->dpy);
   Pixmap pix;

   pix = XCreatePixmap(winsys->dpy, (Drawable) root, width, height, depth);

   return pix;
}

static void
x11_destroy_pixmap(struct segl_winsys *winsys, Pixmap pix)
{
   if (pix)
      XFreePixmap(winsys->dpy, pix);
}

static double
x11_now(struct segl_winsys *winsys)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);

   return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

struct segl_winsys *
segl_get_winsys(EGLNativeDisplayType dpy)
{
   struct segl_winsys *winsys;

   winsys = calloc(1, sizeof(*winsys));
   if (winsys) {
      winsys->dpy = dpy;

      winsys->create_window = x11_create_window;
      winsys->destroy_window = x11_destroy_window;
      winsys->create_pixmap = x11_create_pixmap;
      winsys->destroy_pixmap = x11_destroy_pixmap;

      winsys->now = x11_now;
   }

   return winsys;
}
