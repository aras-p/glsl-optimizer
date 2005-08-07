#ifndef EGLDRI_INCLUDED
#define EGLDRI_INCLUDED

#include "egldisplay.h"
#include "eglscreen.h"
#include "eglsurface.h"
#include "eglcontext.h"
#include "mtypes.h"
#include "dri_util.h"
#include "drm_sarea.h"

/**
 * dri display-specific driver class derived from _EGLDisplay
 */
typedef struct dri_display
{
   _EGLDisplay Base;  /* base class/object */
   void *pFB;
   int drmFD;  /**< \brief DRM device file descriptor */
   int minor;
   unsigned long hFrameBuffer;

   int virtualWidth;
   int virtualHeight;
   int fbSize;
   int bpp;
   int cpp;
   int isPCI;
   int SAREASize;
   drm_sarea_t *pSAREA;
   unsigned int serverContext;	/**< \brief DRM context only active on server */
   unsigned long FBStart;   /**< \brief physical address of the framebuffer */
   void *driverClientMsg;
   int driverClientMsgSize;
   int chipset;
   void *driverPrivate;
   drm_magic_t magic;

   __GLcontextModes *driver_modes;
   __DRIscreen driScreen;

} driDisplay;

/**
 * dri driver-specific screen class derived from _EGLScreen
 */
typedef struct dri_screen
{
   _EGLScreen Base;
   char fb[NAME_MAX];
} driScreen;


/**
 * dri driver-specific surface class derived from _EGLSurface
 */
typedef struct dri_surface
{
   _EGLSurface Base;  /* base class/object */
   __DRIdrawable drawable;
} driSurface;


/**
 * dri driver-specific context class derived from _EGLContext
 */
typedef struct dri_context
{
   _EGLContext Base;  /* base class/object */
   __DRIcontext driContext; /**< \brief context dependent methods */
} driContext;



static inline driDisplay *
Lookup_driDisplay(EGLDisplay dpy)
{
   _EGLDisplay *d = _eglLookupDisplay(dpy);
   return (driDisplay *) d;
}


static inline driScreen *
Lookup_driScreen(EGLDisplay dpy, EGLScreenMESA screen)
{
   _EGLScreen *s = _eglLookupScreen(dpy, screen);
   return (driScreen *) s;
}


static inline driContext *
Lookup_driContext(EGLContext ctx)
{
   _EGLContext *c = _eglLookupContext(ctx);
   return (driContext *) c;
}


static inline driSurface *
Lookup_driSurface(EGLSurface surf)
{
   _EGLSurface *s = _eglLookupSurface(surf);
   return (driSurface *) s;
}

extern void _eglDRIInitDriverFallbacks(_EGLDriver *drv);
extern EGLBoolean _eglDRIShowSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen, EGLSurface surface, EGLModeMESA m);
extern EGLBoolean _eglDRIInitialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor);
extern EGLBoolean _eglDRIGetDisplayInfo(driDisplay *dpy);
extern EGLBoolean _eglDRICreateDisplay(driDisplay *dpy, __DRIframebuffer *framebuffer);
extern EGLBoolean _eglDRICreateScreen(driDisplay *dpy);

#endif /* EGLDRI_INCLUDED */
