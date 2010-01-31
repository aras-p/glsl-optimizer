#include <assert.h>
#include <string.h>

#include "eglimage.h"
#include "eglcurrent.h"
#include "egllog.h"


#ifdef EGL_KHR_image_base


/**
 * Parse the list of image attributes and return the proper error code.
 */
static EGLint
_eglParseImageAttribList(_EGLImage *img, const EGLint *attrib_list)
{
   EGLint i, err = EGL_SUCCESS;

   if (!attrib_list)
      return EGL_SUCCESS;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      case EGL_IMAGE_PRESERVED_KHR:
         img->Preserved = val;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad image attribute 0x%04x", attr);
         break;
      }
   }

   return err;
}


EGLBoolean
_eglInitImage(_EGLDriver *drv, _EGLImage *img, const EGLint *attrib_list)
{
   EGLint err;

   memset(img, 0, sizeof(_EGLImage));

   img->Preserved = EGL_FALSE;

   err = _eglParseImageAttribList(img, attrib_list);
   if (err != EGL_SUCCESS)
      return _eglError(err, "eglCreateImageKHR");

   return EGL_TRUE;
}


_EGLImage *
_eglCreateImageKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx,
                   EGLenum target, EGLClientBuffer buffer,
                   const EGLint *attr_list)
{
   /* driver should override this function */
   return NULL;
}


EGLBoolean
_eglDestroyImageKHR(_EGLDriver *drv, _EGLDisplay *dpy, _EGLImage *image)
{
   /* driver should override this function */
   return EGL_FALSE;
}


#endif /* EGL_KHR_image_base */
