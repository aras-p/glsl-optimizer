/**
 * Surface-related functions.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "eglconfig.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglhash.h"
#include "egllog.h"
#include "eglsurface.h"


/**
 * Do error check on parameters and initialize the given _EGLSurface object.
 * \return EGL_TRUE if no errors, EGL_FALSE otherwise.
 */
EGLBoolean
_eglInitSurface(_EGLDriver *drv, EGLDisplay dpy,
                _EGLSurface *surf, EGLint type, EGLConfig config,
                const EGLint *attrib_list)
{
   const char *func;
   _EGLConfig *conf;
   EGLint width = 0, height = 0, largest = 0;
   EGLint texFormat = 0, texTarget = 0, mipmapTex = 0;
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

   conf = _eglLookupConfig(drv, dpy, config);
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
   surf->SwapInterval = 0;
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


void
_eglSaveSurface(_EGLSurface *surf)
{
   EGLuint key = _eglHashGenKey(_eglGlobal.Surfaces);
   assert(surf);
   assert(!surf->Handle);
   surf->Handle = (EGLSurface) key;
   assert(surf->Handle);
   _eglHashInsert(_eglGlobal.Surfaces, key, surf);
}


void
_eglRemoveSurface(_EGLSurface *surf)
{
   _eglHashRemove(_eglGlobal.Surfaces, (EGLuint) surf->Handle);
}



/**
 * Return the public handle for an internal _EGLSurface.
 * This is the inverse of _eglLookupSurface().
 */
EGLSurface
_eglGetSurfaceHandle(_EGLSurface *surface)
{
   if (surface)
      return surface->Handle;
   else
      return EGL_NO_SURFACE;
}


/**
 * Return the private _EGLSurface which corresponds to a public EGLSurface
 * handle.
 * This is the inverse of _eglGetSurfaceHandle().
 */
_EGLSurface *
_eglLookupSurface(EGLSurface surf)
{
   _EGLSurface *c = (_EGLSurface *) _eglHashLookup(_eglGlobal.Surfaces,
                                                   (EGLuint) surf);
   return c;
}


_EGLSurface *
_eglGetCurrentSurface(EGLint readdraw)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   if (ctx) {
      switch (readdraw) {
      case EGL_DRAW:
         return ctx->DrawSurface;
      case EGL_READ:
         return ctx->ReadSurface;
      default:
         return NULL;
      }
   }
   return NULL;
}


EGLBoolean
_eglSwapBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
{
   /* Basically just do error checking here.  Drivers have to do the
    * actual buffer swap.
    */
   _EGLSurface *surface = _eglLookupSurface(draw);
   if (surface == NULL) {
      _eglError(EGL_BAD_SURFACE, "eglSwapBuffers");
      return EGL_FALSE;
   }
   return EGL_TRUE;
}


EGLBoolean
_eglCopyBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface,
                NativePixmapType target)
{
   /* copy surface to native pixmap */
   /* All implementation burdon for this is in the device driver */
   return EGL_FALSE;
}


EGLBoolean
_eglQuerySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surf,
                 EGLint attribute, EGLint *value)
{
   _EGLSurface *surface = _eglLookupSurface(surf);
   if (surface == NULL) {
      _eglError(EGL_BAD_SURFACE, "eglQuerySurface");
      return EGL_FALSE;
   }
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
      *value = drv->LargestPbuffer;
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
EGLSurface
_eglCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                        NativeWindowType window, const EGLint *attrib_list)
{
#if 0 /* THIS IS JUST EXAMPLE CODE */
   _EGLSurface *surf;

   surf = (_EGLSurface *) calloc(1, sizeof(_EGLSurface));
   if (!surf)
      return EGL_NO_SURFACE;

   if (!_eglInitSurface(drv, dpy, surf, EGL_WINDOW_BIT, config, attrib_list)) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   _eglSaveSurface(surf);

   return surf->Handle;
#endif
   return EGL_NO_SURFACE;
}


/**
 * Example function - drivers should do a proper implementation.
 */
EGLSurface
_eglCreatePixmapSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                        NativePixmapType pixmap, const EGLint *attrib_list)
{
#if 0 /* THIS IS JUST EXAMPLE CODE */
   _EGLSurface *surf;

   surf = (_EGLSurface *) calloc(1, sizeof(_EGLSurface));
   if (!surf)
      return EGL_NO_SURFACE;

   if (!_eglInitSurface(drv, dpy, surf, EGL_PIXMAP_BIT, config, attrib_list)) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   _eglSaveSurface(surf);

   return surf->Handle;
#endif
   return EGL_NO_SURFACE;
}


/**
 * Example function - drivers should do a proper implementation.
 */
EGLSurface
_eglCreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                         const EGLint *attrib_list)
{
#if 0 /* THIS IS JUST EXAMPLE CODE */
   _EGLSurface *surf;

   surf = (_EGLSurface *) calloc(1, sizeof(_EGLSurface));
   if (!surf)
      return EGL_NO_SURFACE;

   if (!_eglInitSurface(drv, dpy, surf, EGL_PBUFFER_BIT, config, attrib_list)) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   _eglSaveSurface(surf);

   return surf->Handle;
#endif
   return EGL_NO_SURFACE;
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLBoolean
_eglDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   _EGLSurface *surf = _eglLookupSurface(surface);
   if (surf) {
      _eglHashRemove(_eglGlobal.Surfaces, (EGLuint) surface);
      if (surf->IsBound) {
         surf->DeletePending = EGL_TRUE;
      }
      else {
         free(surf);
      }
      return EGL_TRUE;
   }
   else {
      _eglError(EGL_BAD_SURFACE, "eglDestroySurface");
      return EGL_FALSE;
   }
}


/**
 * Default fallback routine - drivers might override this.
 */
EGLBoolean
_eglSurfaceAttrib(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surf,
                  EGLint attribute, EGLint value)
{
   _EGLSurface *surface = _eglLookupSurface(surf);

   if (surface == NULL) {
      _eglError(EGL_BAD_SURFACE, "eglSurfaceAttrib");
      return EGL_FALSE;
   }

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
_eglBindTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surf,
                 EGLint buffer)
{
   /* Just do basic error checking and return success/fail.
    * Drivers must implement the real stuff.
    */
   _EGLSurface *surface = _eglLookupSurface(surf);

   if (!surface || surface->Type != EGL_PBUFFER_BIT) {
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
_eglReleaseTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surf,
                    EGLint buffer)
{
   /* Just do basic error checking and return success/fail.
    * Drivers must implement the real stuff.
    */
   _EGLSurface *surface = _eglLookupSurface(surf);

   if (!surface || surface->Type != EGL_PBUFFER_BIT) {
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
_eglSwapInterval(_EGLDriver *drv, EGLDisplay dpy, EGLint interval)
{
   _EGLSurface *surf = _eglGetCurrentSurface(EGL_DRAW);
   if (surf == NULL) {
      _eglError(EGL_BAD_SURFACE, "eglSwapInterval");
      return EGL_FALSE;
   }
   surf->SwapInterval = interval;
   return EGL_TRUE;
}


#ifdef EGL_VERSION_1_2

/**
 * Example function - drivers should do a proper implementation.
 */
EGLSurface
_eglCreatePbufferFromClientBuffer(_EGLDriver *drv, EGLDisplay dpy,
                                  EGLenum buftype, EGLClientBuffer buffer,
                                  EGLConfig config, const EGLint *attrib_list)
{
   if (buftype != EGL_OPENVG_IMAGE) {
      _eglError(EGL_BAD_PARAMETER, "eglCreatePbufferFromClientBuffer");
      return EGL_NO_SURFACE;
   }

   return EGL_NO_SURFACE;
}

#endif /* EGL_VERSION_1_2 */
