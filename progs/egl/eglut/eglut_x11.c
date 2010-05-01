#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "eglutint.h"

void
_eglutNativeInitDisplay(void)
{
   _eglut->native_dpy = XOpenDisplay(_eglut->display_name);
   if (!_eglut->native_dpy)
      _eglutFatal("failed to initialize native display");

   _eglut->surface_type = EGL_WINDOW_BIT;
}

void
_eglutNativeFiniDisplay(void)
{
   XCloseDisplay(_eglut->native_dpy);
}

void
_eglutNativeInitWindow(struct eglut_window *win, const char *title,
                       int x, int y, int w, int h)
{
   XVisualInfo *visInfo, visTemplate;
   int num_visuals;
   Window root, xwin;
   XSetWindowAttributes attr;
   unsigned long mask;
   EGLint vid;

   if (!eglGetConfigAttrib(_eglut->dpy,
            win->config, EGL_NATIVE_VISUAL_ID, &vid))
      _eglutFatal("failed to get visual id");

   /* The X window visual must match the EGL config */
   visTemplate.visualid = vid;
   visInfo = XGetVisualInfo(_eglut->native_dpy,
         VisualIDMask, &visTemplate, &num_visuals);
   if (!visInfo)
      _eglutFatal("failed to get an visual of id 0x%x", vid);

   root = RootWindow(_eglut->native_dpy, DefaultScreen(_eglut->native_dpy));

   /* window attributes */
   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(_eglut->native_dpy,
         root, visInfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

   xwin = XCreateWindow(_eglut->native_dpy, root, x, y, w, h,
         0, visInfo->depth, InputOutput, visInfo->visual, mask, &attr);
   if (!xwin)
      _eglutFatal("failed to create a window");

   XFree(visInfo);

   /* set hints and properties */
   {
      XSizeHints sizehints;
      sizehints.x = x;
      sizehints.y = y;
      sizehints.width  = w;
      sizehints.height = h;
      sizehints.flags = USSize | USPosition;
      XSetNormalHints(_eglut->native_dpy, xwin, &sizehints);
      XSetStandardProperties(_eglut->native_dpy, xwin,
            title, title, None, (char **) NULL, 0, &sizehints);
   }

   XMapWindow(_eglut->native_dpy, xwin);

   win->native.u.window = xwin;
   win->native.width = w;
   win->native.height = h;
}

void
_eglutNativeFiniWindow(struct eglut_window *win)
{
   XDestroyWindow(_eglut->native_dpy, win->native.u.window);
}

static int
lookup_keysym(KeySym sym)
{
   int special;

   switch (sym) {
   case XK_F1:
      special = EGLUT_KEY_F1;
      break;
   case XK_F2:
      special = EGLUT_KEY_F2;
      break;
   case XK_F3:
      special = EGLUT_KEY_F3;
      break;
   case XK_F4:
      special = EGLUT_KEY_F4;
      break;
   case XK_F5:
      special = EGLUT_KEY_F5;
      break;
   case XK_F6:
      special = EGLUT_KEY_F6;
      break;
   case XK_F7:
      special = EGLUT_KEY_F7;
      break;
   case XK_F8:
      special = EGLUT_KEY_F8;
      break;
   case XK_F9:
      special = EGLUT_KEY_F9;
      break;
   case XK_F10:
      special = EGLUT_KEY_F10;
      break;
   case XK_F11:
      special = EGLUT_KEY_F11;
      break;
   case XK_F12:
      special = EGLUT_KEY_F12;
      break;
   case XK_KP_Left:
   case XK_Left:
      special = EGLUT_KEY_LEFT;
      break;
   case XK_KP_Up:
   case XK_Up:
      special = EGLUT_KEY_UP;
      break;
   case XK_KP_Right:
   case XK_Right:
      special = EGLUT_KEY_RIGHT;
      break;
   case XK_KP_Down:
   case XK_Down:
      special = EGLUT_KEY_DOWN;
      break;
   default:
      special = -1;
      break;
   }

   return special;
}

static void
next_event(struct eglut_window *win)
{
   int redraw = 0;
   XEvent event;

   if (!XPending(_eglut->native_dpy)) {
      if (_eglut->idle_cb)
         _eglut->idle_cb();
      return;
   }

   XNextEvent(_eglut->native_dpy, &event);

   switch (event.type) {
   case Expose:
      redraw = 1;
      break;
   case ConfigureNotify:
      win->native.width = event.xconfigure.width;
      win->native.height = event.xconfigure.height;
      if (win->reshape_cb)
         win->reshape_cb(win->native.width, win->native.height);
      break;
   case KeyPress:
      {
         char buffer[1];
         KeySym sym;
         int r;

         r = XLookupString(&event.xkey,
               buffer, sizeof(buffer), &sym, NULL);
         if (r && win->keyboard_cb) {
            win->keyboard_cb(buffer[0]);
         }
         else if (!r && win->special_cb) {
            r = lookup_keysym(sym);
            if (r >= 0)
               win->special_cb(r);
         }
      }
      redraw = 1;
      break;
   default:
      ; /*no-op*/
   }

   _eglut->redisplay = redraw;
}

void
_eglutNativeEventLoop(void)
{
   while (1) {
      struct eglut_window *win = _eglut->current;

      next_event(win);

      if (_eglut->redisplay) {
         _eglut->redisplay = 0;

         if (win->display_cb)
            win->display_cb();
         eglSwapBuffers(_eglut->dpy, win->surface);
      }
   }
}
