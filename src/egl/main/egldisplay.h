#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED


#include "egltypedefs.h"
#include "egldefines.h"
#include "eglmutex.h"


enum _egl_resource_type {
   _EGL_RESOURCE_CONTEXT,
   _EGL_RESOURCE_SURFACE,
   _EGL_RESOURCE_IMAGE,

   _EGL_NUM_RESOURCES
};
/* this cannot and need not go into egltypedefs.h */
typedef enum _egl_resource_type _EGLResourceType;


/**
 * A resource of a display.
 */
struct _egl_resource
{
   /* which display the resource belongs to */
   _EGLDisplay *Display;
   EGLBoolean IsLinked;

   /* used to link resources of the same type */
   _EGLResource *Next;
};


/**
 * Optional EGL extensions info.
 */
struct _egl_extensions
{
   EGLBoolean MESA_screen_surface;
   EGLBoolean MESA_copy_context;
   EGLBoolean KHR_image_base;
   EGLBoolean KHR_image_pixmap;
   EGLBoolean KHR_vg_parent_image;
   EGLBoolean KHR_gl_texture_2D_image;
   EGLBoolean KHR_gl_texture_cubemap_image;
   EGLBoolean KHR_gl_texture_3D_image;
   EGLBoolean KHR_gl_renderbuffer_image;

   char String[_EGL_MAX_EXTENSIONS_LEN];
};


struct _egl_display 
{
   /* used to link displays */
   _EGLDisplay *Next;

   _EGLMutex Mutex;

   EGLNativeDisplayType NativeDisplay;

   EGLBoolean Initialized; /**< True if the display is initialized */
   _EGLDriver *Driver;
   void *DriverData; /* private to driver */

   int APImajor, APIminor; /**< as returned by eglInitialize() */
   char Version[1000];     /**< initialized from APImajor/minor, DriverName */

   /** Bitmask of supported APIs (EGL_xx_BIT) set by the driver during init */
   EGLint ClientAPIsMask;
   char ClientAPIs[1000];   /**< updated by eglQueryString */

   _EGLExtensions Extensions;

   EGLint NumScreens;
   _EGLScreen **Screens;  /* array [NumScreens] */

   EGLint MaxConfigs;
   EGLint NumConfigs;
   _EGLConfig **Configs;  /* array [NumConfigs] of ptr to _EGLConfig */

   /* lists of resources */
   _EGLResource *ResourceLists[_EGL_NUM_RESOURCES];
};


extern void
_eglFiniDisplay(void);


extern _EGLDisplay *
_eglFindDisplay(EGLNativeDisplayType displayName);


PUBLIC void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *dpy);


PUBLIC void
_eglCleanupDisplay(_EGLDisplay *disp);


extern EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy);


PUBLIC EGLBoolean
_eglCheckResource(void *res, _EGLResourceType type, _EGLDisplay *dpy);


/**
 * Lookup a handle to find the linked display.
 * Return NULL if the handle has no corresponding linked display.
 */
static INLINE _EGLDisplay *
_eglLookupDisplay(EGLDisplay display)
{
   _EGLDisplay *dpy = (_EGLDisplay *) display;
   if (!_eglCheckDisplayHandle(display))
      dpy = NULL;
   return dpy;
}


/**
 * Return the handle of a linked display, or EGL_NO_DISPLAY.
 */
static INLINE EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *dpy)
{
   return (EGLDisplay) ((dpy) ? dpy : EGL_NO_DISPLAY);
}


extern void
_eglLinkResource(_EGLResource *res, _EGLResourceType type, _EGLDisplay *dpy);


extern void
_eglUnlinkResource(_EGLResource *res, _EGLResourceType type);


/**
 * Return true if the resource is linked.
 */
static INLINE EGLBoolean
_eglIsResourceLinked(_EGLResource *res)
{
   return res->IsLinked;
}


#endif /* EGLDISPLAY_INCLUDED */
