/**
 * Surface-related functions.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "eglconfig.h"
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
   EGLint i;

   switch (type) {
   case EGL_WINDOW_BIT:
      func = "eglCreateWindowSurface";
      break;
   case EGL_PIXMAP_BIT:
      func = "eglCreatePixmapSurface";
      break;
   case EGL_PBUFFER_BIT:
      func = "eglCreatePBufferSurface";
      break;
   case EGL_SCREEN_BIT_MESA:
      func = "eglCreateScreenSurface";
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
      default:
         _eglError(EGL_BAD_ATTRIBUTE, func);
         return EGL_FALSE;
      }
   }

   if (width <= 0 || height <= 0) {
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

   return EGL_TRUE;
}


void
_eglSaveSurface(_EGLSurface *surf)
{
   assert(surf);
   assert(!surf->Handle);
   surf->Handle = _eglHashGenKey(_eglGlobal.Contexts);
   assert(surf->Handle);
   _eglHashInsert(_eglGlobal.Surfaces, surf->Handle, surf);
}


void
_eglRemoveSurface(_EGLSurface *surf)
{
   _eglHashRemove(_eglGlobal.Surfaces, surf->Handle);
}


_EGLSurface *
_eglLookupSurface(EGLSurface surf)
{
   _EGLSurface *c = (_EGLSurface *) _eglHashLookup(_eglGlobal.Surfaces, surf);
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
   _EGLContext *context = _eglGetCurrentContext();
   _EGLSurface *surface = _eglLookupSurface(draw);
   if (context && context->DrawSurface != surface) {
      _eglError(EGL_BAD_SURFACE, "eglSwapBuffers");
      return EGL_FALSE;
   }
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
   case EGL_SURFACE_TYPE:
      *value = surface->Type;
      return EGL_TRUE;
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
      _eglHashRemove(_eglGlobal.Surfaces, surface);
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
_eglSurfaceAttrib(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surf, EGLint attribute, EGLint value)
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
_eglBindTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   /* XXX unfinished */
   return EGL_FALSE;
}


EGLBoolean
_eglReleaseTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
   /* XXX unfinished */
   return EGL_FALSE;
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
