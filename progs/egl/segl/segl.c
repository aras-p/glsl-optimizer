#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <EGL/egl.h>

#include "segl.h"

static void
segl_log(struct segl *segl, const char *format, ...)
{
   va_list ap;

   va_start(ap, format);

   if (segl->winsys->vlog)
      segl->winsys->vlog(segl->winsys, format, ap);
   else
      vfprintf(stdout, format, ap);

   va_end(ap);
}

static EGLBoolean
segl_init_egl(struct segl *segl, const EGLint *attribs)
{
   EGLint num_conf;

   segl->dpy = eglGetDisplay(segl->winsys->dpy);
   if (!segl->dpy)
      return EGL_FALSE;

   if (!eglInitialize(segl->dpy, &segl->major, &segl->minor))
      return EGL_FALSE;

   if (segl->verbose) {
      const char *ver = eglQueryString(segl->dpy, EGL_VERSION);
      segl_log(segl, "EGL_VERSION = %s\n", ver);
   }

   if (!eglChooseConfig(segl->dpy, attribs, &segl->conf, 1, &num_conf) ||
       !num_conf) {
      segl_log(segl, "failed to choose a config\n");
      eglTerminate(segl->dpy);
      return EGL_FALSE;
   }

   return EGL_TRUE;
}

struct segl *
segl_new(struct segl_winsys *winsys, const EGLint *attribs)
{
   struct segl *segl;

   segl = calloc(1, sizeof(*segl));
   if (segl) {
      segl->verbose = EGL_TRUE;
      segl->winsys = winsys;

      if (!segl_init_egl(segl, attribs)) {
         free(segl);
         return NULL;
      }
   }

   return segl;
}

void
segl_destroy(struct segl *segl)
{
   free(segl);
}

EGLBoolean
segl_create_window(struct segl *segl, const char *name,
                   EGLint width, EGLint height, const EGLint *attribs,
                   EGLNativeWindowType *win_ret, EGLSurface *surf_ret)
{
   EGLNativeWindowType win;
   EGLSurface surf;
   EGLint visual;

   if (!win_ret) {
      if (surf_ret)
         *surf_ret = EGL_NO_SURFACE;
      return EGL_TRUE;
   }

   if (!eglGetConfigAttrib(segl->dpy, segl->conf, EGL_NATIVE_VISUAL_ID, &visual))
      return EGL_FALSE;

   win = segl->winsys->create_window(segl->winsys,
         name, width, height, visual);
   if (surf_ret) {
      surf = eglCreateWindowSurface(segl->dpy, segl->conf, win, attribs);
      if (!surf) {
         segl_log(segl, "failed to create a window surface\n");
         segl->winsys->destroy_window(segl->winsys, win);
         return EGL_FALSE;
      }

      *surf_ret = surf;
   }

   *win_ret = win;

   return EGL_TRUE;
}

EGLBoolean
segl_create_pixmap(struct segl *segl,
                   EGLint width, EGLint height, const EGLint *attribs,
                   EGLNativePixmapType *pix_ret, EGLSurface *surf_ret)
{
   EGLNativePixmapType pix;
   EGLSurface surf;
   EGLint depth;

   if (!pix_ret) {
      if (surf_ret)
         *surf_ret = EGL_NO_SURFACE;
      return EGL_TRUE;
   }

   if (!eglGetConfigAttrib(segl->dpy, segl->conf, EGL_BUFFER_SIZE, &depth))
      return EGL_FALSE;

   pix = segl->winsys->create_pixmap(segl->winsys, width, height, depth);
   if (surf_ret) {
      surf = eglCreatePixmapSurface(segl->dpy, segl->conf, pix, attribs);
      if (!surf) {
         segl_log(segl, "failed to create a pixmap surface\n");
         segl->winsys->destroy_pixmap(segl->winsys, pix);
         return EGL_FALSE;
      }

      *surf_ret = surf;
   }

   *pix_ret = pix;

   return EGL_TRUE;
}

void
segl_benchmark(struct segl *segl, double seconds,
               void (*draw_frame)(void *), void *draw_data)
{
   double begin, end, last_frame, duration;
   EGLint num_frames = 0;

   begin = segl->winsys->now(segl->winsys);
   end = begin + seconds;

   last_frame = begin;
   while (last_frame < end) {
      draw_frame(draw_data);
      last_frame = segl->winsys->now(segl->winsys);
      num_frames++;
   }

   duration = last_frame - begin;
   segl_log(segl, "%d frames in %3.1f seconds = %6.3f FPS\n",
         num_frames, duration, (double) num_frames / duration);
}
