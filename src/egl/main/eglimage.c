#include <assert.h>

#include "eglimage.h"
#include "egldisplay.h"


#ifdef EGL_KHR_image_base


EGLBoolean
_eglInitImage(_EGLDriver *drv, _EGLImage *img, const EGLint *attrib_list)
{
   EGLint i;

   img->Preserved = EGL_FALSE;

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_IMAGE_PRESERVED_KHR:
         i++;
         img->Preserved = attrib_list[i];
         break;
      default:
         /* not an error */
         break;
      }
   }

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
