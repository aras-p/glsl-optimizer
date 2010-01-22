#ifndef _SEGL_H_
#define _SEGL_H_

#include <stdarg.h>
#include <EGL/egl.h>

struct segl_winsys {
   EGLNativeDisplayType dpy;

   EGLNativeWindowType (*create_window)(struct segl_winsys *winsys,
                                        const char *name,
                                        EGLint width, EGLint height,
                                        EGLint visual);
   void (*destroy_window)(struct segl_winsys *winsys, EGLNativeWindowType win);

   EGLNativePixmapType (*create_pixmap)(struct segl_winsys *winsys,
                                        EGLint width, EGLint height,
                                        EGLint depth);
   void (*destroy_pixmap)(struct segl_winsys *winsys, EGLNativePixmapType pix);

   /* get current time in seconds */
   double (*now)(struct segl_winsys *winsys);
   /* log a message.  OPTIONAL */
   void (*vlog)(struct segl_winsys *winsys, const char *format, va_list ap);
};

struct segl {
   EGLBoolean verbose;

   struct segl_winsys *winsys;

   EGLint major, minor;
   EGLDisplay dpy;
   EGLConfig conf;
};

struct segl_winsys *
segl_get_winsys(EGLNativeDisplayType dpy);

struct segl *
segl_new(struct segl_winsys *winsys, const EGLint *attribs);

void
segl_destroy(struct segl *segl);

EGLBoolean
segl_create_window(struct segl *segl, const char *name,
                   EGLint width, EGLint height, const EGLint *attribs,
                   EGLNativeWindowType *win_ret, EGLSurface *surf_ret);

EGLBoolean
segl_create_pixmap(struct segl *segl,
                   EGLint width, EGLint height, const EGLint *attribs,
                   EGLNativePixmapType *pix_ret, EGLSurface *surf_ret);

void
segl_benchmark(struct segl *segl, double seconds,
               void (*draw_frame)(void *), void *draw_data);

#endif /* _SEGL_H_ */
