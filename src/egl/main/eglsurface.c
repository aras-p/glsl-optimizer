/**
 * Surface-related functions.
 *
 * See the eglcontext.c file for comments that also apply here.
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "eglconfig.h"
#include "eglsurface.h"
#include "eglglobals.h"
#include "eglhash.h"


void
_eglInitSurface(_EGLSurface *surf)
{
   /* XXX fix this up */
   memset(surf, 0, sizeof(_EGLSurface));
}


void
_eglSaveSurface(_EGLSurface *surf)
{
   assert(surf);
   surf->Handle = _eglHashGenKey(_eglGlobal.Contexts);
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
   /* Basically just do error checking */
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
_eglCopyBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
   /* XXX unfinished */
   return EGL_FALSE;
}


EGLBoolean
_eglQuerySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surf, EGLint attribute, EGLint *value)
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
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQuerySurface");
      return EGL_FALSE;
   }
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLSurface
_eglCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
   /* nothing - just a placeholder */
   return EGL_NO_SURFACE;
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLSurface
_eglCreatePixmapSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
   /* nothing - just a placeholder */
   return EGL_NO_SURFACE;
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLSurface
_eglCreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
   /* nothing - just a placeholder */
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


