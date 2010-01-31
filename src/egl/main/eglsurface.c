/**
 * Surface-related functions.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "egldisplay.h"
#include "eglcontext.h"
#include "eglconfig.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"


static void
_eglClampSwapInterval(_EGLSurface *surf, EGLint interval)
{
   EGLint bound = GET_CONFIG_ATTRIB(surf->Config, EGL_MAX_SWAP_INTERVAL);
   if (interval >= bound) {
      interval = bound;
   }
   else {
      bound = GET_CONFIG_ATTRIB(surf->Config, EGL_MIN_SWAP_INTERVAL);
      if (interval < bound)
         interval = bound;
   }
   surf->SwapInterval = interval;
}


/**
 * Parse the list of surface attributes and return the proper error code.
 */
static EGLint
_eglParseSurfaceAttribList(_EGLSurface *surf, const EGLint *attrib_list)
{
   EGLint type = surf->Type;
   EGLint i, err = EGL_SUCCESS;

   if (!attrib_list)
      return EGL_SUCCESS;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      /* common (except for screen surfaces) attributes */
      case EGL_VG_COLORSPACE:
         if (type == EGL_SCREEN_BIT_MESA) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         switch (val) {
         case EGL_VG_COLORSPACE_sRGB:
         case EGL_VG_COLORSPACE_LINEAR:
            break;
         default:
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (err != EGL_SUCCESS)
            break;
         surf->VGColorspace = val;
         break;
      case EGL_VG_ALPHA_FORMAT:
         if (type == EGL_SCREEN_BIT_MESA) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         switch (val) {
         case EGL_VG_ALPHA_FORMAT_NONPRE:
         case EGL_VG_ALPHA_FORMAT_PRE:
            break;
         default:
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (err != EGL_SUCCESS)
            break;
         surf->VGAlphaFormat = val;
         break;
      /* window surface attributes */
      case EGL_RENDER_BUFFER:
         if (type != EGL_WINDOW_BIT) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (val != EGL_BACK_BUFFER && val != EGL_SINGLE_BUFFER) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         surf->RenderBuffer = val;
         break;
      /* pbuffer surface attributes */
      case EGL_WIDTH:
         if (type != EGL_PBUFFER_BIT && type != EGL_SCREEN_BIT_MESA) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (val < 0) {
            err = EGL_BAD_PARAMETER;
            break;
         }
         surf->Width = val;
         break;
      case EGL_HEIGHT:
         if (type != EGL_PBUFFER_BIT && type != EGL_SCREEN_BIT_MESA) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (val < 0) {
            err = EGL_BAD_PARAMETER;
            break;
         }
         surf->Height = val;
         break;
      case EGL_LARGEST_PBUFFER:
         if (type != EGL_PBUFFER_BIT) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         surf->LargestPbuffer = !!val;
         break;
      case EGL_TEXTURE_FORMAT:
         if (type != EGL_PBUFFER_BIT) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         switch (val) {
         case EGL_TEXTURE_RGB:
         case EGL_TEXTURE_RGBA:
         case EGL_NO_TEXTURE:
            break;
         default:
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (err != EGL_SUCCESS)
            break;
         surf->TextureFormat = val;
         break;
      case EGL_TEXTURE_TARGET:
         if (type != EGL_PBUFFER_BIT) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         switch (val) {
         case EGL_TEXTURE_2D:
         case EGL_NO_TEXTURE:
            break;
         default:
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (err != EGL_SUCCESS)
            break;
         surf->TextureTarget = val;
         break;
      case EGL_MIPMAP_TEXTURE:
         if (type != EGL_PBUFFER_BIT) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         surf->MipmapTexture = !!val;
         break;
      /* no pixmap surface specific attributes */
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_WARNING, "bad surface attribute 0x%04x", attr);
         break;
      }
   }

   return err;
}


/**
 * Do error check on parameters and initialize the given _EGLSurface object.
 * \return EGL_TRUE if no errors, EGL_FALSE otherwise.
 */
EGLBoolean
_eglInitSurface(_EGLSurface *surf, _EGLDisplay *dpy, EGLint type,
                _EGLConfig *conf, const EGLint *attrib_list)
{
   const char *func;
   EGLint renderBuffer = EGL_BACK_BUFFER;
   EGLint err;

   switch (type) {
   case EGL_WINDOW_BIT:
      func = "eglCreateWindowSurface";
      break;
   case EGL_PIXMAP_BIT:
      func = "eglCreatePixmapSurface";
      renderBuffer = EGL_SINGLE_BUFFER;
      break;
   case EGL_PBUFFER_BIT:
      func = "eglCreatePBufferSurface";
      break;
   case EGL_SCREEN_BIT_MESA:
      func = "eglCreateScreenSurface";
      renderBuffer = EGL_SINGLE_BUFFER; /* XXX correct? */
      break;
   default:
      _eglLog(_EGL_WARNING, "Bad type in _eglInitSurface");
      return EGL_FALSE;
   }

   if ((GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE) & type) == 0) {
      /* The config can't be used to create a surface of this type */
      _eglError(EGL_BAD_CONFIG, func);
      return EGL_FALSE;
   }

   memset(surf, 0, sizeof(_EGLSurface));
   surf->Resource.Display = dpy;
   surf->Type = type;
   surf->Config = conf;

   surf->Width = 0;
   surf->Height = 0;
   surf->TextureFormat = EGL_NO_TEXTURE;
   surf->TextureTarget = EGL_NO_TEXTURE;
   surf->MipmapTexture = EGL_FALSE;
   surf->LargestPbuffer = EGL_FALSE;
   surf->RenderBuffer = renderBuffer;
   surf->VGAlphaFormat = EGL_VG_ALPHA_FORMAT_NONPRE;
   surf->VGColorspace = EGL_VG_COLORSPACE_sRGB;

   surf->MipmapLevel = 0;
   surf->MultisampleResolve = EGL_MULTISAMPLE_RESOLVE_DEFAULT;
   surf->SwapBehavior = EGL_BUFFER_DESTROYED;

   surf->HorizontalResolution = EGL_UNKNOWN;
   surf->VerticalResolution = EGL_UNKNOWN;
   surf->AspectRatio = EGL_UNKNOWN;

   /* the default swap interval is 1 */
   _eglClampSwapInterval(surf, 1);

   err = _eglParseSurfaceAttribList(surf, attrib_list);
   if (err != EGL_SUCCESS)
      return _eglError(err, func);

   return EGL_TRUE;
}


EGLBoolean
_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   /* Drivers have to do the actual buffer swap.  */
   return EGL_TRUE;
}


EGLBoolean
_eglCopyBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                EGLNativePixmapType target)
{
   /* copy surface to native pixmap */
   /* All implementation burdon for this is in the device driver */
   return EGL_FALSE;
}


EGLBoolean
_eglQuerySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface,
                 EGLint attribute, EGLint *value)
{
   switch (attribute) {
   case EGL_WIDTH:
      *value = surface->Width;
      break;
   case EGL_HEIGHT:
      *value = surface->Height;
      break;
   case EGL_CONFIG_ID:
      *value = GET_CONFIG_ATTRIB(surface->Config, EGL_CONFIG_ID);
      break;
   case EGL_LARGEST_PBUFFER:
      *value = surface->LargestPbuffer;
      break;
   case EGL_TEXTURE_FORMAT:
      /* texture attributes: only for pbuffers, no error otherwise */
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->TextureFormat;
      break;
   case EGL_TEXTURE_TARGET:
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->TextureTarget;
      break;
   case EGL_MIPMAP_TEXTURE:
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->MipmapTexture;
      break;
   case EGL_MIPMAP_LEVEL:
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->MipmapLevel;
      break;
   case EGL_SWAP_BEHAVIOR:
      *value = surface->SwapBehavior;
      break;
   case EGL_RENDER_BUFFER:
      *value = surface->RenderBuffer;
      break;
   case EGL_PIXEL_ASPECT_RATIO:
      *value = surface->AspectRatio;
      break;
   case EGL_HORIZONTAL_RESOLUTION:
      *value = surface->HorizontalResolution;
      break;
   case EGL_VERTICAL_RESOLUTION:
      *value = surface->VerticalResolution;
      break;
   case EGL_MULTISAMPLE_RESOLVE:
      *value = surface->MultisampleResolve;
      break;
   case EGL_VG_ALPHA_FORMAT:
      *value = surface->VGAlphaFormat;
      break;
   case EGL_VG_COLORSPACE:
      *value = surface->VGColorspace;
      break;
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQuerySurface");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}


/**
 * Drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                        EGLNativeWindowType window, const EGLint *attrib_list)
{
   return NULL;
}


/**
 * Drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                        EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
   return NULL;
}


/**
 * Drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                         const EGLint *attrib_list)
{
   return NULL;
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLBoolean
_eglDestroySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf)
{
   if (!_eglIsSurfaceBound(surf))
      free(surf);
   return EGL_TRUE;
}


/**
 * Default fallback routine - drivers might override this.
 */
EGLBoolean
_eglSurfaceAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface,
                  EGLint attribute, EGLint value)
{
   EGLint confval;
   EGLint err = EGL_SUCCESS;

   switch (attribute) {
   case EGL_MIPMAP_LEVEL:
      confval = GET_CONFIG_ATTRIB(surface->Config, EGL_RENDERABLE_TYPE);
      if (!(confval & (EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT))) {
         err = EGL_BAD_PARAMETER;
         break;
      }
      surface->MipmapLevel = value;
      break;
   case EGL_MULTISAMPLE_RESOLVE:
      switch (value) {
      case EGL_MULTISAMPLE_RESOLVE_DEFAULT:
         break;
      case EGL_MULTISAMPLE_RESOLVE_BOX:
         confval = GET_CONFIG_ATTRIB(surface->Config, EGL_SURFACE_TYPE);
         if (!(confval & EGL_MULTISAMPLE_RESOLVE_BOX_BIT))
            err = EGL_BAD_MATCH;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }
      if (err != EGL_SUCCESS)
         break;
      surface->MultisampleResolve = value;
      break;
   case EGL_SWAP_BEHAVIOR:
      switch (value) {
      case EGL_BUFFER_DESTROYED:
         break;
      case EGL_BUFFER_PRESERVED:
         confval = GET_CONFIG_ATTRIB(surface->Config, EGL_SURFACE_TYPE);
         if (!(confval & EGL_SWAP_BEHAVIOR_PRESERVED_BIT))
            err = EGL_BAD_MATCH;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }
      if (err != EGL_SUCCESS)
         break;
      surface->SwapBehavior = value;
      break;
   default:
      err = EGL_BAD_ATTRIBUTE;
      break;
   }

   if (err != EGL_SUCCESS)
      return _eglError(err, "eglSurfaceAttrib");
   return EGL_TRUE;
}


EGLBoolean
_eglBindTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface,
                 EGLint buffer)
{
   /* Just do basic error checking and return success/fail.
    * Drivers must implement the real stuff.
    */

   if (surface->Type != EGL_PBUFFER_BIT) {
      _eglError(EGL_BAD_SURFACE, "eglBindTexImage");
      return EGL_FALSE;
   }

   if (surface->TextureFormat == EGL_NO_TEXTURE) {
      _eglError(EGL_BAD_MATCH, "eglBindTexImage");
      return EGL_FALSE;
   }

   if (buffer != EGL_BACK_BUFFER) {
      _eglError(EGL_BAD_PARAMETER, "eglBindTexImage");
      return EGL_FALSE;
   }

   surface->BoundToTexture = EGL_TRUE;

   return EGL_TRUE;
}


EGLBoolean
_eglReleaseTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surface,
                    EGLint buffer)
{
   /* Just do basic error checking and return success/fail.
    * Drivers must implement the real stuff.
    */

   if (surface->Type != EGL_PBUFFER_BIT) {
      _eglError(EGL_BAD_SURFACE, "eglBindTexImage");
      return EGL_FALSE;
   }

   if (surface->TextureFormat == EGL_NO_TEXTURE) {
      _eglError(EGL_BAD_MATCH, "eglBindTexImage");
      return EGL_FALSE;
   }

   if (buffer != EGL_BACK_BUFFER) {
      _eglError(EGL_BAD_PARAMETER, "eglReleaseTexImage");
      return EGL_FALSE;
   }

   if (!surface->BoundToTexture) {
      _eglError(EGL_BAD_SURFACE, "eglReleaseTexImage");
      return EGL_FALSE;
   }

   surface->BoundToTexture = EGL_FALSE;

   return EGL_TRUE;
}


EGLBoolean
_eglSwapInterval(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf,
                 EGLint interval)
{
   _eglClampSwapInterval(surf, interval);
   return EGL_TRUE;
}


#ifdef EGL_VERSION_1_2

/**
 * Example function - drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreatePbufferFromClientBuffer(_EGLDriver *drv, _EGLDisplay *dpy,
                                  EGLenum buftype, EGLClientBuffer buffer,
                                  _EGLConfig *conf, const EGLint *attrib_list)
{
   if (buftype != EGL_OPENVG_IMAGE) {
      _eglError(EGL_BAD_PARAMETER, "eglCreatePbufferFromClientBuffer");
      return NULL;
   }

   return NULL;
}

#endif /* EGL_VERSION_1_2 */
