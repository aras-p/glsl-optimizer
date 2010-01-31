#ifndef EGLIMAGE_INCLUDED
#define EGLIMAGE_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver images.
 */
struct _egl_image
{
   /* An image is a display resource */
   _EGLResource Resource;

   EGLBoolean Preserved;
   EGLint GLTextureLevel;
   EGLint GLTextureZOffset;
};


PUBLIC EGLBoolean
_eglInitImage(_EGLImage *img, _EGLDisplay *dpy, const EGLint *attrib_list);


extern _EGLImage *
_eglCreateImageKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx,
                   EGLenum target, EGLClientBuffer buffer, const EGLint *attr_list);


extern EGLBoolean
_eglDestroyImageKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *image);


/**
 * Link an image to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLImageKHR
_eglLinkImage(_EGLImage *img, _EGLDisplay *dpy)
{
   _eglLinkResource(&img->Resource, _EGL_RESOURCE_IMAGE, dpy);
   return (EGLImageKHR) img;
}


/**
 * Unlink a linked image from its display.
 * Accessing an unlinked image should generate EGL_BAD_PARAMETER error.
 */
static INLINE void
_eglUnlinkImage(_EGLImage *img)
{
   _eglUnlinkResource(&img->Resource, _EGL_RESOURCE_IMAGE);
}


/**
 * Lookup a handle to find the linked image.
 * Return NULL if the handle has no corresponding linked image.
 */
static INLINE _EGLImage *
_eglLookupImage(EGLImageKHR image, _EGLDisplay *dpy)
{
   _EGLImage *img = (_EGLImage *) image;
   if (!dpy || !_eglCheckResource((void *) img, _EGL_RESOURCE_IMAGE, dpy))
      img = NULL;
   return img;
}


/**
 * Return the handle of a linked image, or EGL_NO_IMAGE_KHR.
 */
static INLINE EGLImageKHR
_eglGetImageHandle(_EGLImage *img)
{
   _EGLResource *res = (_EGLResource *) img;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLImageKHR) img : EGL_NO_IMAGE_KHR;
}


/**
 * Return true if the image is linked to a display.
 */
static INLINE EGLBoolean
_eglIsImageLinked(_EGLImage *img)
{
   _EGLResource *res = (_EGLResource *) img;
   return (res && _eglIsResourceLinked(res));
}


#endif /* EGLIMAGE_INCLUDED */
