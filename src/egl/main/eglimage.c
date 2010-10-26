#include <assert.h>
#include <string.h>

#include "eglimage.h"
#include "eglcurrent.h"
#include "egllog.h"


#ifdef EGL_KHR_image_base


/**
 * Parse the list of image attributes and return the proper error code.
 */
EGLint
_eglParseImageAttribList(_EGLImageAttribs *attrs, _EGLDisplay *dpy,
                         const EGLint *attrib_list)
{
   EGLint i, err = EGL_SUCCESS;

   (void) dpy;

   memset(attrs, 0, sizeof(attrs));
   attrs->ImagePreserved = EGL_FALSE;
   attrs->GLTextureLevel = 0;
   attrs->GLTextureZOffset = 0;

   if (!attrib_list)
      return err;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      /* EGL_KHR_image_base */
      case EGL_IMAGE_PRESERVED_KHR:
         attrs->ImagePreserved = val;
         break;

      /* EGL_KHR_gl_image */
      case EGL_GL_TEXTURE_LEVEL_KHR:
         attrs->GLTextureLevel = val;
         break;
      case EGL_GL_TEXTURE_ZOFFSET_KHR:
         attrs->GLTextureZOffset = val;
         break;

      /* EGL_MESA_drm_image */
      case EGL_WIDTH:
         attrs->Width = val;
         break;
      case EGL_HEIGHT:
         attrs->Height = val;
         break;
      case EGL_DRM_BUFFER_FORMAT_MESA:
         attrs->DRMBufferFormatMESA = val;
         break;
      case EGL_DRM_BUFFER_USE_MESA:
         attrs->DRMBufferUseMESA = val;
         break;
      case EGL_DRM_BUFFER_STRIDE_MESA:
         attrs->DRMBufferStrideMESA = val;
         break;

      default:
         /* unknown attrs are ignored */
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
_eglInitImage(_EGLImage *img, _EGLDisplay *dpy)
{
   _eglInitResource(&img->Resource, sizeof(*img), dpy);

   return EGL_TRUE;
}


#endif /* EGL_KHR_image_base */
