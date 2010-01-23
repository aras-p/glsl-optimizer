/**
 * Surface-related functions.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "egldisplay.h"
#include "eglcontext.h"
#include "eglconfig.h"
#include "egldriver.h"
#include "eglglobals.h"
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
 * Do error check on parameters and initialize the given _EGLSurface object.
 * \return EGL_TRUE if no errors, EGL_FALSE otherwise.
 */
EGLBoolean
_eglInitSurface(_EGLDriver *drv, _EGLSurface *surf, EGLint type,
                _EGLConfig *conf, const EGLint *attrib_list)
{
   const char *func;
   EGLint width = 0, height = 0, largest = 0;
   EGLint texFormat = EGL_NO_TEXTURE, texTarget = EGL_NO_TEXTURE;
   EGLint mipmapTex = EGL_FALSE;
   EGLint renderBuffer = EGL_BACK_BUFFER;
#ifdef EGL_VERSION_1_2
   EGLint colorspace = EGL_COLORSPACE_sRGB;
   EGLint alphaFormat = EGL_ALPHA_FORMAT_NONPRE;
#endif
   EGLint i;

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

   if (!conf) {
      _eglError(EGL_BAD_CONFIG, func);
      return EGL_FALSE;
   }

   if ((GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE) & type) == 0) {
      /* The config can't be used to create a surface of this type */
      _eglError(EGL_BAD_CONFIG, func);
      return EGL_FALSE;
   }

   /*
    * Parse attribute list.  Different kinds of surfaces support different
    * attributes.
    */
   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_WIDTH:
         if (type == EGL_PBUFFER_BIT || type == EGL_SCREEN_BIT_MESA) {
            width = attrib_list[++i];
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_HEIGHT:
         if (type == EGL_PBUFFER_BIT || type == EGL_SCREEN_BIT_MESA) {
            height = attrib_list[++i];
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_LARGEST_PBUFFER:
         if (type == EGL_PBUFFER_BIT) {
            largest = attrib_list[++i];
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_TEXTURE_FORMAT:
         if (type == EGL_PBUFFER_BIT) {
            texFormat = attrib_list[++i];
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_TEXTURE_TARGET:
         if (type == EGL_PBUFFER_BIT) {
            texTarget = attrib_list[++i];
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_MIPMAP_TEXTURE:
         if (type == EGL_PBUFFER_BIT) {
            mipmapTex = attrib_list[++i];
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
#ifdef EGL_VERSION_1_2
      case EGL_RENDER_BUFFER:
         if (type == EGL_WINDOW_BIT) {
            renderBuffer = attrib_list[++i];
            if (renderBuffer != EGL_BACK_BUFFER &&
                renderBuffer != EGL_SINGLE_BUFFER) {
               _eglError(EGL_BAD_ATTRIBUTE, func);
               return EGL_FALSE;
            }
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_COLORSPACE:
         if (type == EGL_WINDOW_BIT ||
             type == EGL_PBUFFER_BIT ||
             type == EGL_PIXMAP_BIT) {
            colorspace = attrib_list[++i];
            if (colorspace != EGL_COLORSPACE_sRGB &&
                colorspace != EGL_COLORSPACE_LINEAR) {
               _eglError(EGL_BAD_ATTRIBUTE, func);
               return EGL_FALSE;
            }
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;
      case EGL_ALPHA_FORMAT:
         if (type == EGL_WINDOW_BIT ||
             type == EGL_PBUFFER_BIT ||
             type == EGL_PIXMAP_BIT) {
            alphaFormat = attrib_list[++i];
            if (alphaFormat != EGL_ALPHA_FORMAT_NONPRE &&
                alphaFormat != EGL_ALPHA_FORMAT_PRE) {
               _eglError(EGL_BAD_ATTRIBUTE, func);
               return EGL_FALSE;
            }
         }
         else {
            _eglError(EGL_BAD_ATTRIBUTE, func);
            return EGL_FALSE;
         }
         break;

#endif /* EGL_VERSION_1_2 */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, func);
         return EGL_FALSE;
      }
   }

   if (width < 0 || height < 0) {
      _eglError(EGL_BAD_ATTRIBUTE, func);
      return EGL_FALSE;
   }

   memset(surf, 0, sizeof(_EGLSurface));
   surf->Config = conf;
   surf->Type = type;
   surf->Width = width;
   surf->Height = height;
   surf->TextureFormat = texFormat;
   surf->TextureTarget = texTarget;
   surf->MipmapTexture = mipmapTex;
   surf->MipmapLevel = 0;
   /* the default swap interval is 1 */
   _eglClampSwapInterval(surf, 1);

#ifdef EGL_VERSION_1_2
   surf->SwapBehavior = EGL_BUFFER_DESTROYED; /* XXX ok? */
   surf->HorizontalResolution = EGL_UNKNOWN; /* set by caller */
   surf->VerticalResolution = EGL_UNKNOWN; /* set by caller */
   surf->AspectRatio = EGL_UNKNOWN; /* set by caller */
   surf->RenderBuffer = renderBuffer;
   surf->AlphaFormat = alphaFormat;
   surf->Colorspace = colorspace;
#endif

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
                NativePixmapType target)
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
      return EGL_TRUE;
   case EGL_HEIGHT:
      *value = surface->Height;
      return EGL_TRUE;
   case EGL_CONFIG_ID:
      *value = GET_CONFIG_ATTRIB(surface->Config, EGL_CONFIG_ID);
      return EGL_TRUE;
   case EGL_LARGEST_PBUFFER:
      *value = dpy->LargestPbuffer;
      return EGL_TRUE;
   case EGL_SURFACE_TYPE:
      *value = surface->Type;
      return EGL_TRUE;
#ifdef EGL_VERSION_1_1
   case EGL_TEXTURE_FORMAT:
      /* texture attributes: only for pbuffers, no error otherwise */
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->TextureFormat;
      return EGL_TRUE;
   case EGL_TEXTURE_TARGET:
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->TextureTarget;
      return EGL_TRUE;
   case EGL_MIPMAP_TEXTURE:
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->MipmapTexture;
      return EGL_TRUE;
   case EGL_MIPMAP_LEVEL:
      if (surface->Type == EGL_PBUFFER_BIT)
         *value = surface->MipmapLevel;
      return EGL_TRUE;
#endif /* EGL_VERSION_1_1 */
#ifdef EGL_VERSION_1_2
   case EGL_SWAP_BEHAVIOR:
      *value = surface->SwapBehavior;
      return EGL_TRUE;
   case EGL_RENDER_BUFFER:
      *value = surface->RenderBuffer;
      return EGL_TRUE;
   case EGL_PIXEL_ASPECT_RATIO:
      *value = surface->AspectRatio;
      return EGL_TRUE;
   case EGL_HORIZONTAL_RESOLUTION:
      *value = surface->HorizontalResolution;
      return EGL_TRUE;
   case EGL_VERTICAL_RESOLUTION:
      *value = surface->VerticalResolution;
      return EGL_TRUE;
   case EGL_ALPHA_FORMAT:
      *value = surface->AlphaFormat;
      return EGL_TRUE;
   case EGL_COLORSPACE:
      *value = surface->Colorspace;
      return EGL_TRUE;
#endif /* EGL_VERSION_1_2 */
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQuerySurface");
      return EGL_FALSE;
   }
}


/**
 * Example function - drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                        NativeWindowType window, const EGLint *attrib_list)
{
#if 0 /* THIS IS JUST EXAMPLE CODE */
   _EGLSurface *surf;

   surf = (_EGLSurface *) calloc(1, sizeof(_EGLSurface));
   if (!surf)
      return NULL;

   if (!_eglInitSurface(drv, surf, EGL_WINDOW_BIT, conf, attrib_list)) {
      free(surf);
      return NULL;
   }

   return surf;
#endif
   return NULL;
}


/**
 * Example function - drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                        NativePixmapType pixmap, const EGLint *attrib_list)
{
#if 0 /* THIS IS JUST EXAMPLE CODE */
   _EGLSurface *surf;

   surf = (_EGLSurface *) calloc(1, sizeof(_EGLSurface));
   if (!surf)
      return NULL;

   if (!_eglInitSurface(drv, surf, EGL_PIXMAP_BIT, conf, attrib_list)) {
      free(surf);
      return NULL;
   }

   return surf;
#endif
   return NULL;
}


/**
 * Example function - drivers should do a proper implementation.
 */
_EGLSurface *
_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                         const EGLint *attrib_list)
{
#if 0 /* THIS IS JUST EXAMPLE CODE */
   _EGLSurface *surf;

   surf = (_EGLSurface *) calloc(1, sizeof(_EGLSurface));
   if (!surf)
      return NULL;

   if (!_eglInitSurface(drv, surf, EGL_PBUFFER_BIT, conf, attrib_list)) {
      free(surf);
      return NULL;
   }

   return NULL;
#endif
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
   switch (attribute) {
   case EGL_MIPMAP_LEVEL:
      surface->MipmapLevel = value;
      break;
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglSurfaceAttrib");
      return EGL_FALSE;
   }
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


/**
 * Link a surface to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
EGLSurface
_eglLinkSurface(_EGLSurface *surf, _EGLDisplay *dpy)
{
   surf->Display = dpy;
   surf->Next = dpy->SurfaceList;
   dpy->SurfaceList = surf;
   return (EGLSurface) surf;
}


/**
 * Unlink a linked surface from its display.
 * Accessing an unlinked surface should generate EGL_BAD_SURFACE error.
 */
void
_eglUnlinkSurface(_EGLSurface *surf)
{
   _EGLSurface *prev;

   prev = surf->Display->SurfaceList;
   if (prev != surf) {
      while (prev) {
         if (prev->Next == surf)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = surf->Next;
   }
   else {
      prev = NULL;
      surf->Display->SurfaceList = surf->Next;
   }

   surf->Next = NULL;
   surf->Display = NULL;
}


#ifndef _EGL_SKIP_HANDLE_CHECK


/**
 * Return EGL_TRUE if the given handle is a valid handle to a surface.
 */
EGLBoolean
_eglCheckSurfaceHandle(EGLSurface surf, _EGLDisplay *dpy)
{
   _EGLSurface *cur = NULL;

   if (dpy)
      cur = dpy->SurfaceList;
   while (cur) {
      if (cur == (_EGLSurface *) surf) {
         assert(cur->Display == dpy);
         break;
      }
      cur = cur->Next;
   }
   return (cur != NULL);
}


#endif /* !_EGL_SKIP_HANDLE_CHECK */
