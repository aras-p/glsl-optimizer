/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 LunarG Inc.
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
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include <assert.h>
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
#include "eglcurrent.h"
#include "egllog.h"

#include "native.h"
#include "egl_g3d.h"
#include "egl_g3d_image.h"

/**
 * Reference and return the front left buffer of the native pixmap.
 */
static struct pipe_texture *
egl_g3d_reference_native_pixmap(_EGLDisplay *dpy, EGLNativePixmapType pix)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_config *gconf;
   struct native_surface *nsurf;
   struct pipe_texture *textures[NUM_NATIVE_ATTACHMENTS];
   enum native_attachment natt;

   gconf = egl_g3d_config(egl_g3d_find_pixmap_config(dpy, pix));
   if (!gconf)
      return NULL;

   nsurf = gdpy->native->create_pixmap_surface(gdpy->native,
         pix, gconf->native);
   if (!nsurf)
      return NULL;

   natt = NATIVE_ATTACHMENT_FRONT_LEFT;
   if (!nsurf->validate(nsurf, 1 << natt, NULL, textures, NULL, NULL))
      textures[natt] = NULL;

   nsurf->destroy(nsurf);

   return textures[natt];
}

_EGLImage *
egl_g3d_create_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx,
                     EGLenum target, EGLClientBuffer buffer,
                     const EGLint *attribs)
{
   struct pipe_texture *ptex;
   struct egl_g3d_image *gimg;
   unsigned face = 0, level = 0, zslice = 0;

   gimg = CALLOC_STRUCT(egl_g3d_image);
   if (!gimg) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePbufferSurface");
      return NULL;
   }

   if (!_eglInitImage(&gimg->base, dpy, attribs)) {
      free(gimg);
      return NULL;
   }

   switch (target) {
   case EGL_NATIVE_PIXMAP_KHR:
      ptex = egl_g3d_reference_native_pixmap(dpy,
            (EGLNativePixmapType) buffer);
      break;
   default:
      ptex = NULL;
      break;
   }

   if (!ptex) {
      free(gimg);
      return NULL;
   }

   if (level > ptex->last_level) {
      _eglError(EGL_BAD_MATCH, "eglCreateEGLImageKHR");
      pipe_texture_reference(&gimg->texture, NULL);
      free(gimg);
      return NULL;
   }
   if (zslice > ptex->depth0) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
      pipe_texture_reference(&gimg->texture, NULL);
      free(gimg);
      return NULL;
   }

   /* transfer the ownership to the image */
   gimg->texture = ptex;
   gimg->face = face;
   gimg->level = level;
   gimg->zslice = zslice;

   return &gimg->base;
}

EGLBoolean
egl_g3d_destroy_image(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *img)
{
   struct egl_g3d_image *gimg = egl_g3d_image(img);

   pipe_texture_reference(&gimg->texture, NULL);
   free(gimg);

   return EGL_TRUE;
}
