#ifndef EGLIMAGE_INCLUDED
#define EGLIMAGE_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


struct _egl_image_attribs
{
   /* EGL_KHR_image_base */
   EGLBoolean ImagePreserved;

   /* EGL_KHR_gl_image */
   EGLint GLTextureLevel;
   EGLint GLTextureZOffset;

   /* EGL_MESA_drm_image */
   EGLint Width;
   EGLint Height;
   EGLint DRMBufferFormatMESA;
   EGLint DRMBufferUseMESA;
   EGLint DRMBufferStrideMESA;
};

/**
 * "Base" class for device driver images.
 */
struct _egl_image
{
   /* An image is a display resource */
   _EGLResource Resource;
};


PUBLIC EGLint
_eglParseImageAttribList(_EGLImageAttribs *attrs, _EGLDisplay *dpy,
                         const EGLint *attrib_list);


PUBLIC EGLBoolean
_eglInitImage(_EGLImage *img, _EGLDisplay *dpy);


/**
 * Increment reference count for the image.
 */
static INLINE _EGLImage *
_eglGetImage(_EGLImage *img)
{
   if (img)
      _eglGetResource(&img->Resource);
   return img;
}


/**
 * Decrement reference count for the image.
 */
static INLINE EGLBoolean
_eglPutImage(_EGLImage *img)
{
   return (img) ? _eglPutResource(&img->Resource) : EGL_FALSE;
}


/**
 * Link an image to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLImageKHR
_eglLinkImage(_EGLImage *img)
{
   _eglLinkResource(&img->Resource, _EGL_RESOURCE_IMAGE);
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
