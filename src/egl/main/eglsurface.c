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
_eglCopyBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, NativePixmapType target)
{
   /* copy surface to native pixmap */
   /* All implementation burdon for this is in the device driver */
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
   case EGL_SURFACE_TYPE:
      *value = surface->Type;
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



/**
 ** EGL Surface Utility Functions.  This could be handy for device drivers.
 **/


/**
 * Initialize the fields of the given _EGLSurface object from the other
 * parameters.  Do error checking too.  Allocate EGLSurface handle and
 * insert into hash table.
 * \return EGLSurface handle or EGL_NO_SURFACE if any error
 */
EGLSurface
_eglInitPbufferSurface(_EGLSurface *surface, _EGLDriver *drv, EGLDisplay dpy,
                       EGLConfig config, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   EGLint width = 0, height = 0, largest = 0;
   EGLint texFormat = 0, texTarget = 0, mipmapTex = 0;
   EGLint i;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreatePbufferSurface");
      return EGL_NO_SURFACE;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_WIDTH:
         width = attrib_list[++i];
         break;
      case EGL_HEIGHT:
         height = attrib_list[++i];
         break;
      case EGL_LARGEST_PBUFFER:
         largest = attrib_list[++i];
         break;
      case EGL_TEXTURE_FORMAT:
         texFormat = attrib_list[++i];
         break;
      case EGL_TEXTURE_TARGET:
         texTarget = attrib_list[++i];
         break;
      case EGL_MIPMAP_TEXTURE:
         mipmapTex = attrib_list[++i];
         break;
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreatePbufferSurface");
         return EGL_NO_SURFACE;
      }
   }

   if (width <= 0 || height <= 0) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglCreatePbufferSurface(width or height)");
      return EGL_NO_SURFACE;
   }

   surface->Config = conf;
   surface->Type = EGL_PBUFFER_BIT;
   surface->Width = width;
   surface->Height = height;
   surface->TextureFormat = texFormat;
   surface->TextureTarget = texTarget;
   surface->MipmapTexture = mipmapTex;
   surface->MipmapLevel = 0;
   surface->SwapInterval = 0;

   /* insert into hash table */
   _eglSaveSurface(surface);
   assert(surface->Handle);

   return surface->Handle;
}
