#include <stdlib.h>
#include <sys/time.h>

#include "segl.h"

static EGLNativeWindowType
kms_create_window(struct segl_winsys *winsys, const char *name,
                  EGLint width, EGLint height, EGLint visual)
{
   return 0;
}

static void
kms_destroy_window(struct segl_winsys *winsys, EGLNativeWindowType win)
{
}


static EGLNativePixmapType 
kms_create_pixmap(struct segl_winsys *winsys, EGLint width, EGLint height,
                  EGLint depth)
{
   return 0;
}

static void
kms_destroy_pixmap(struct segl_winsys *winsys, EGLNativePixmapType pix)
{
}

static double
kms_now(struct segl_winsys *winsys)
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

      winsys->create_window = kms_create_window;
      winsys->destroy_window = kms_destroy_window;
      winsys->create_pixmap = kms_create_pixmap;
      winsys->destroy_pixmap = kms_destroy_pixmap;

      winsys->now = kms_now;
   }

   return winsys;
}
