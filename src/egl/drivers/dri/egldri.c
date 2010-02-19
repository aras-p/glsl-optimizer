/**
 * Generic EGL driver for DRI.  This is basically an "adaptor" driver
 * that allows libEGL to load/use regular DRI drivers.
 *
 * This file contains all the code needed to interface DRI-based drivers
 * with libEGL.
 *
 * There's a lot of dependencies on fbdev and the /sys/ filesystem.
 */


#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <linux/fb.h>
#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include "egldriver.h"
#include "egldisplay.h"
#include "eglcontext.h"
#include "eglconfig.h"
#include "eglconfigutil.h"
#include "eglsurface.h"
#include "eglscreen.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglmode.h"

#include "egldri.h"

const char *sysfs = "/sys/class";

static const int empty_attribute_list[1] = { None };



/**
 * Given a card number, return the name of the DRI driver to use.
 * This generally means reading the contents of
 * /sys/class/drm/cardX/dri_library_name, where X is the card number
 */
static EGLBoolean
driver_name_from_card_number(int card, char *driverName, int maxDriverName)
{
   char path[2000];
   FILE *f;
   int length;

   snprintf(path, sizeof(path), "%s/drm/card%d/dri_library_name", sysfs, card);

   f = fopen(path, "r");
   if (!f)
      return EGL_FALSE;

   fgets(driverName, maxDriverName, f);
   fclose(f);

   if ((length = strlen(driverName)) > 1) {
      /* remove the trailing newline from sysfs */
      driverName[length - 1] = '\0';
      strncat(driverName, "_dri", maxDriverName);
      return EGL_TRUE;
   }
   else {
      return EGL_FALSE;
   }   
}



/**
 * The bootstrap function.
 * Return a new driDriver object and plug in API functions.
 * This function, in turn, loads a specific DRI driver (ex: r200_dri.so).
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy, const char *args)
{
#if 1
   const int card = args ? atoi(args) : 0;
   _EGLDriver *driver = NULL;
   char driverName[1000];

   if (!driver_name_from_card_number(card, driverName, sizeof(driverName))) {
      _eglLog(_EGL_WARNING,
              "Unable to determine driver name for card %d\n", card);
      return NULL;
   }

   _eglLog(_EGL_DEBUG, "Driver name: %s\n", driverName);

   driver = _eglOpenDriver(dpy, driverName, args);

   return driver;

#else

   int length;
   char path[NAME_MAX];
   struct dirent *dirent;
#if 1
   FILE *file;
#endif
   DIR *dir;
   _EGLDriver *driver = NULL;;

   snprintf(path, sizeof(path), "%s/drm", sysfs);
   if (!(dir = opendir(path))) {
      _eglLog(_EGL_WARNING, "%s DRM devices not found.", path);
      return EGL_FALSE;
   }

   /* loop over dir entries looking for cardX where "X" is in the
    * dpy->DriverName ":X" string.
    */
   while ((dirent = readdir(dir))) {

      if (strncmp(&dirent->d_name[0], "card", 4) != 0)
         continue;
      if (strcmp(&dirent->d_name[4], &driverName[1]) != 0)
         continue;

      snprintf(path, sizeof(path), "%s/drm/card%s/dri_library_name",
               sysfs, &driverName[1]);
      _eglLog(_EGL_INFO, "Opening %s", path);
#if 1
      file = fopen(path, "r");
      if (!file) {
         _eglLog(_EGL_WARNING, "Failed to open %s", path);
         return NULL;
      }
      fgets(path, sizeof(path), file);
      fclose(file);
#else
      strcpy(path, "r200\n");
#endif
      if ((length = strlen(path)) > 0)
         path[length - 1] = '\0';  /* remove the trailing newline from sysfs */
      strncat(path, "_dri", sizeof(path));

      driver = _eglOpenDriver(dpy, path);

      break;
   }
   closedir(dir);

   return driver;
#endif
}


/**
 * Called by eglCreateContext via drv->API.CreateContext().
 */
static EGLContext
_eglDRICreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                     EGLContext share_list, const EGLint *attrib_list)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driContext *c, *share;
   void *sharePriv;
   _EGLConfig *conf;
   __GLcontextModes visMode;

   c = (driContext *) calloc(1, sizeof(*c));
   if (!c)
      return EGL_NO_CONTEXT;

   conf = _eglLookupConfig(drv, dpy, config);
   assert(conf);

   if (!_eglInitContext(drv, &c->Base, conf, attrib_list)) {
      free(c);
      return EGL_NO_CONTEXT;
   }

   if (share_list != EGL_NO_CONTEXT) {
      _EGLContext *shareCtx = _eglLookupContext(share_list);
      if (!shareCtx) {
         _eglError(EGL_BAD_CONTEXT, "eglCreateContext(share_list)");
         return EGL_FALSE;
      }
   }
   share = Lookup_driContext(share_list);
   if (share)
      sharePriv = share->driContext.private;
   else
      sharePriv = NULL;

   _eglConfigToContextModesRec(conf, &visMode);

   c->driContext.private = disp->driScreen.createNewContext(disp, &visMode,
           GLX_WINDOW_BIT, sharePriv, &c->driContext);
   if (!c->driContext.private) {
      free(c);
      return EGL_FALSE;
   }

   /* link to display */
   _eglLinkContext(&c->Base, &disp->Base);

   return _eglGetContextHandle(&c->Base);
}


static EGLBoolean
_eglDRIMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw,
                   EGLSurface read, EGLContext context)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driContext *ctx = Lookup_driContext(context);
   EGLBoolean b;
   __DRIid drawBuf = (__DRIid) draw;
   __DRIid readBuf = (__DRIid) read;

   b = _eglMakeCurrent(drv, dpy, draw, read, context);
   if (!b)
      return EGL_FALSE;

   if (ctx) {
      ctx->driContext.bindContext(disp, 0, drawBuf, readBuf, &ctx->driContext);
   }
   else {
      /* what's this??? */
      /*      _mesa_make_current( NULL, NULL, NULL );*/
   }
   return EGL_TRUE;
}


static EGLSurface
_eglDRICreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                            const EGLint *attrib_list)
{
   driSurface *surf;
   _EGLConfig *conf;

   conf = _eglLookupConfig(drv, dpy, config);
   assert(conf);

   surf = (driSurface *) calloc(1, sizeof(*surf));
   if (!surf) {
      return EGL_NO_SURFACE;
   }

   if (!_eglInitSurface(drv, &surf->Base, EGL_PBUFFER_BIT,
                        conf, attrib_list)) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   /* create software-based pbuffer */
   {
#if 0
      GLcontext *ctx = NULL; /* this _should_ be OK */
#endif
      __GLcontextModes visMode;
      _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);
      assert(conf); /* bad config should be caught earlier */
      _eglConfigToContextModesRec(conf, &visMode);

#if 0
      surf->mesa_framebuffer = _mesa_create_framebuffer(&visMode);
      _mesa_add_soft_renderbuffers(surf->mesa_framebuffer,
                                   GL_TRUE, /* color bufs */
                                   visMode.haveDepthBuffer,
                                   visMode.haveStencilBuffer,
                                   visMode.haveAccumBuffer,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */ );

      /* set pbuffer/framebuffer size */
      _mesa_resize_framebuffer(ctx, surf->mesa_framebuffer,
                               surf->Base.Width, surf->Base.Height);
#endif
   }

   _eglLinkSurface(&surf->Base, _eglLookupDisplay(dpy));

   return surf->Base.Handle;
}


static EGLBoolean
_eglDRIDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driSurface *fs = Lookup_driSurface(surface);

   _eglUnlinkSurface(&fs->Base);

   fs->drawable.destroyDrawable(disp, fs->drawable.private);

   if (!_eglIsSurfaceBound(&fs->Base))
      free(fs);
   return EGL_TRUE;
}


static EGLBoolean
_eglDRIDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext context)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driContext *fc = Lookup_driContext(context);

   _eglUnlinkContext(&fc->Base);

   fc->driContext.destroyContext(disp, 0, fc->driContext.private);

   if (!_eglIsContextBound(&fc->Base))
      free(fc);
   return EGL_TRUE;
}


/**
 * Create a drawing surface which can be directly displayed on a screen.
 */
static EGLSurface
_eglDRICreateScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLConfig cfg,
                               const EGLint *attrib_list)
{
   _EGLConfig *config = _eglLookupConfig(drv, dpy, cfg);
   driDisplay *disp = Lookup_driDisplay(dpy);
   driSurface *surface;
   __GLcontextModes visMode;
   __DRIid drawBuf;

   surface = (driSurface *) calloc(1, sizeof(*surface));
   if (!surface) {
      return EGL_NO_SURFACE;
   }

   /* init base class, do error checking, etc. */
   if (!_eglInitSurface(drv, &surface->Base, EGL_SCREEN_BIT_MESA,
                        config, attrib_list)) {
      free(surface);
      return EGL_NO_SURFACE;
   }

   _eglLinkSurface(&surface->Base &disp->Base);


   /*
    * XXX this is where we should allocate video memory for the surface!
    */


   /* convert EGLConfig to GLvisual */
   _eglConfigToContextModesRec(config, &visMode);

   drawBuf = (__DRIid) _eglGetSurfaceHandle(&surface->Base);

   /* Create a new DRI drawable */
   if (!disp->driScreen.createNewDrawable(disp, &visMode, drawBuf,
                                          &surface->drawable, GLX_WINDOW_BIT,
                                          empty_attribute_list)) {
      _eglUnlinkSurface(&surface->Base);
      free(surface);
      return EGL_NO_SURFACE;
   }

   return surface->Base.Handle;
}


/**
 * Set the fbdev colormap to a simple linear ramp.
 */
static void
_eglDRILoadColormap(driScreen *scrn)
{
   char path[ NAME_MAX ];
   char *buffer;
   int i, fd;

   /* cmap attribute uses 256 lines of 16 bytes.
    * Allocate one extra char for the \0 added by sprintf()
    */
   if ( !( buffer = malloc( 256 * 16 + 1 ) ) ) {
      _eglLog(_EGL_WARNING, "Out of memory in _eglDRILoadColormap");
      return;
   }

   /* cmap attribute uses 256 lines of 16 bytes */
   for ( i = 0; i < 256; i++ ) {
      int c = (i << 8) | i; /* expand to 16-bit value */
      sprintf(&buffer[i * 16], "%02x%c%04x%04x%04x\n", i, ' ', c, c, c);
   }

   snprintf(path, sizeof(path), "%s/graphics/%s/color_map", sysfs, scrn->fb);
   if ( !( fd = open( path, O_RDWR ) ) ) {
      _eglLog(_EGL_WARNING, "Unable to open %s to set colormap", path);
      return;
   }
   write( fd, buffer, 256 * 16 );
   close( fd );

   free( buffer );
}


/**
 * Show the given surface on the named screen.
 * If surface is EGL_NO_SURFACE, disable the screen's output.
 * Called via eglShowSurfaceMESA().
 */
EGLBoolean
_eglDRIShowScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy,
                             EGLScreenMESA screen,
                             EGLSurface surface, EGLModeMESA m)
{
   driDisplay *display = Lookup_driDisplay(dpy);
   driScreen *scrn = Lookup_driScreen(dpy, screen);
   driSurface *surf = Lookup_driSurface(surface);
   _EGLMode *mode = _eglLookupMode(dpy, m);
   FILE *file;
   char fname[NAME_MAX], buffer[1000];
   int temp;

   _eglLog(_EGL_DEBUG, "Enter _eglDRIShowScreenSurface");

   /* This will check that surface, screen, and mode are valid.
    * Also, it checks that the surface is large enough for the mode, etc.
    */
   if (!_eglShowScreenSurfaceMESA(drv, dpy, screen, surface, m))
      return EGL_FALSE;

   assert(surface == EGL_NO_SURFACE || surf);
   assert(m == EGL_NO_MODE_MESA || mode);
   assert(scrn);

   /*
    * Blank/unblank screen depending on if m == EGL_NO_MODE_MESA
    */
   snprintf(fname, sizeof(fname), "%s/graphics/%s/blank", sysfs, scrn->fb);
   file = fopen(fname, "r+");
   if (!file) {
      _eglLog(_EGL_WARNING, "kernel patch?? chown all fb sysfs attrib to allow"
              " write - %s\n", fname);
      return EGL_FALSE;
   }
   snprintf(buffer, sizeof(buffer), "%d",
            (m == EGL_NO_MODE_MESA ? VESA_POWERDOWN : VESA_VSYNC_SUSPEND));
   fputs(buffer, file);
   fclose(file);

   if (m == EGL_NO_MODE_MESA) {
      /* all done! */
      return EGL_TRUE;
   }

   _eglLog(_EGL_INFO, "Setting display mode to %d x %d, %d bpp",
           mode->Width, mode->Height, display->bpp);

   /*
    * Set the display mode
    */
   snprintf(fname, sizeof(fname), "%s/graphics/%s/mode", sysfs, scrn->fb);
   file = fopen(fname, "r+");
   if (!file) {
      _eglLog(_EGL_WARNING, "Failed to open %s to set mode", fname);
      return EGL_FALSE;
   }
   /* note: nothing happens without the \n! */
   snprintf(buffer, sizeof(buffer), "%s\n", mode->Name);
   fputs(buffer, file);
   fclose(file);
   _eglLog(_EGL_INFO, "Set mode to %s in %s", mode->Name, fname);

   /*
    * Set display bpp
    */
   snprintf(fname, sizeof(fname), "%s/graphics/%s/bits_per_pixel",
            sysfs, scrn->fb);
   file = fopen(fname, "r+");
   if (!file) {
      _eglLog(_EGL_WARNING, "Failed to open %s to set bpp", fname);
      return EGL_FALSE;
   }
   display->bpp = GET_CONFIG_ATTRIB(surf->Base.Config, EGL_BUFFER_SIZE);
   display->cpp = display->bpp / 8;
   snprintf(buffer, sizeof(buffer), "%d", display->bpp);
   fputs(buffer, file);
   fclose(file);

   /*
    * Unblank display
    */
   snprintf(fname, sizeof(fname), "%s/graphics/%s/blank", sysfs, scrn->fb);
   file = fopen(fname, "r+");
   if (!file) {
      _eglLog(_EGL_WARNING, "Failed to open %s", fname);
      return EGL_FALSE;
   }
   snprintf(buffer, sizeof(buffer), "%d", VESA_NO_BLANKING);
   fputs(buffer, file);
   fclose(file);

   /*
    * Set fbdev buffer virtual size to surface's size.
    */
   snprintf(fname, sizeof(fname), "%s/graphics/%s/virtual_size", sysfs, scrn->fb);
   file = fopen(fname, "r+");
   snprintf(buffer, sizeof(buffer), "%d,%d", surf->Base.Width, surf->Base.Height);
   fputs(buffer, file);
   rewind(file);
   fgets(buffer, sizeof(buffer), file);
   sscanf(buffer, "%d,%d", &display->virtualWidth, &display->virtualHeight);
   fclose(file);

   /*
    * round up pitch as needed
    */
   temp = display->virtualWidth;
   switch (display->bpp / 8) {
   case 1: temp = (display->virtualWidth + 127) & ~127; break;
   case 2: temp = (display->virtualWidth +  31) &  ~31; break;
   case 3:
   case 4: temp = (display->virtualWidth +  15) &  ~15; break;
   default:
      _eglLog(_EGL_WARNING, "Bad display->bpp = %d in _eglDRIShowScreenSurface");
   }
   display->virtualWidth = temp;

   /*
    * sanity check
    */
   if (surf->Base.Width < display->virtualWidth ||
       surf->Base.Height < display->virtualHeight) {
      /* this case _should_ have been caught at the top of this function */
      _eglLog(_EGL_WARNING, "too small of surface in _eglDRIShowScreenSurfaceMESA "
              "%d x %d < %d x %d", 
              surf->Base.Width,
              surf->Base.Height,
              display->virtualWidth,
              display->virtualHeight);
      /*
      return EGL_FALSE;
      */
   }

   /* This used to be done in the _eglDRICreateScreens routine. */
   _eglDRILoadColormap(scrn);

   return EGL_TRUE;
}


/**
 * Called by eglSwapBuffers via the drv->API.SwapBuffers() pointer.
 *
 * If the backbuffer is on a videocard, this is extraordinarily slow!
 */
static EGLBoolean
_eglDRISwapBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
{
   driSurface *drawable = Lookup_driSurface(draw);

   /* this does error checking */
   if (!_eglSwapBuffers(drv, dpy, draw))
      return EGL_FALSE;

   drawable->drawable.swapBuffers(NULL, drawable->drawable.private);

   return EGL_TRUE;
}


EGLBoolean
_eglDRIGetDisplayInfo(driDisplay *dpy)
{
   char path[ NAME_MAX ];
   FILE *file;
   int i, rc;
   drmSetVersion sv;
   drm_magic_t magic;

   snprintf( path, sizeof( path ), "%s/graphics/fb%d/device/device", sysfs, dpy->minor );
   file = fopen( path, "r" );
   if (!file) {
      _eglLog(_EGL_WARNING, "Unable to open %s", path);
      return EGL_FALSE;
   }
   fgets( path, sizeof( path ), file );
   sscanf( path, "%x", &dpy->chipset );
   fclose( file );

   sprintf(path, DRM_DEV_NAME, DRM_DIR_NAME, dpy->minor);
   if ( ( dpy->drmFD = open(path, O_RDWR, 0) ) < 0 ) {
      _eglLog(_EGL_WARNING, "drmOpen failed.");
      return EGL_FALSE;
   }

   /* Set the interface version, asking for 1.2 */
   sv.drm_di_major = 1;
   sv.drm_di_minor = 2;
   sv.drm_dd_major = -1;
   if ((rc = drmSetInterfaceVersion(dpy->drmFD, &sv)))
      return EGL_FALSE;

   /* self authorize */
   if (drmGetMagic(dpy->drmFD, &magic))
      return EGL_FALSE;
   if (drmAuthMagic(dpy->drmFD, magic))
      return EGL_FALSE;

   /* Map framebuffer and SAREA */
   for (i = 0; ; i++) {
      drm_handle_t handle, offset;
      drmSize size;
      drmMapType type;
      drmMapFlags flags;
      int mtrr;

      if (drmGetMap(dpy->drmFD, i, &offset, &size, &type, &flags,
                    &handle, &mtrr))
         break;

      if (type == DRM_FRAME_BUFFER) {
         rc = drmMap( dpy->drmFD, offset, size, (drmAddressPtr) &dpy->pFB);
         if (rc < 0) {
            _eglLog(_EGL_WARNING, "drmMap DRM_FAME_BUFFER failed");
            return EGL_FALSE;
         }
         dpy->fbSize = size;
         _eglLog(_EGL_INFO, "Found framebuffer size: %d", dpy->fbSize);
      }
      else if (type == DRM_SHM) {
         rc = drmMap(dpy->drmFD, offset, size, (drmAddressPtr) &dpy->pSAREA);
         if (rc < 0 ) {
            _eglLog(_EGL_WARNING, "drmMap DRM_SHM failed.");
            return EGL_FALSE;
         }
         dpy->SAREASize = SAREA_MAX;
         _eglLog(_EGL_DEBUG, "mapped SAREA 0x%08lx to %p, size %d",
                 (unsigned long) offset, dpy->pSAREA, dpy->SAREASize );
      }
   }

   if (!dpy->pFB) {
      _eglLog(_EGL_WARNING, "failed to map framebuffer");
      return EGL_FALSE;
   }

   if (!dpy->pSAREA) {
      /* if this happens, make sure you're using the most recent DRM modules */
      _eglLog(_EGL_WARNING, "failed to map SAREA");
      return EGL_FALSE;
   }

   memset( dpy->pSAREA, 0, dpy->SAREASize );

   return EGL_TRUE;
}


 /* Return the DRI per screen structure */
static __DRIscreen *
__eglFindDRIScreen(__DRInativeDisplay *ndpy, int scrn)
{
   driDisplay *disp = (driDisplay *)ndpy;
   return &disp->driScreen;
}


static GLboolean
__eglCreateContextWithConfig(__DRInativeDisplay* ndpy, int screen,
                             int configID, void* context,
                             drm_context_t * hHWContext)
{
    __DRIscreen *pDRIScreen;
    __DRIscreen *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreen *) pDRIScreen->private;
    if (psp->fd) {
        if (drmCreateContext(psp->fd, hHWContext)) {
            _eglLog(_EGL_WARNING, "drmCreateContext failed.");
            return GL_FALSE;
        }
        *(void**)context = (void*) *hHWContext;
    }
#if 0
    __DRIscreen *pDRIScreen;
    __DRIscreen *psp;

    pDRIScreen = __glXFindDRIScreen(dpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }

    psp = (__DRIscreen *) pDRIScreen->private;

    if (psp->fd) {
        if (drmCreateContext(psp->fd, hHWContext)) {
            _eglLog(_EGL_WARNING, "drmCreateContext failed.");
            return GL_FALSE;
        }
        *(void**)contextID = (void*) *hHWContext;
    }
#endif
    return GL_TRUE;
}


static GLboolean
__eglDestroyContext( __DRInativeDisplay * ndpy, int screen,  __DRIid context )
{
    __DRIscreen *pDRIScreen;
    __DRIscreen *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreen *) pDRIScreen->private;
    if (psp->fd)
      drmDestroyContext(psp->fd, context);

   return GL_TRUE;
}


static GLboolean
__eglCreateDrawable(__DRInativeDisplay * ndpy, int screen,
                    __DRIid drawable, drm_drawable_t * hHWDrawable)
{
    __DRIscreen *pDRIScreen;
    __DRIscreen *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreen *) pDRIScreen->private;
    if (psp->fd) {
        if (drmCreateDrawable(psp->fd, hHWDrawable)) {
            _eglLog(_EGL_WARNING, "drmCreateDrawable failed.");
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}


static GLboolean
__eglDestroyDrawable( __DRInativeDisplay * ndpy, int screen, __DRIid drawable )
{
    __DRIscreen *pDRIScreen;
    __DRIscreen *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreen *) pDRIScreen->private;
    if (psp->fd)
      drmDestroyDrawable(psp->fd, drawable);

   return GL_TRUE;
}

static GLboolean
__eglGetDrawableInfo(__DRInativeDisplay * ndpy, int screen, __DRIid drawable,
                     unsigned int* index, unsigned int* stamp,
                     int* X, int* Y, int* W, int* H,
                     int* numClipRects, drm_clip_rect_t ** pClipRects,
                     int* backX, int* backY,
                     int* numBackClipRects, drm_clip_rect_t ** pBackClipRects )
{
    __DRIscreen *pDRIScreen;
    __DRIscreen *psp;
    driSurface *surf = Lookup_driSurface((EGLSurface) drawable);

   pDRIScreen = __eglFindDRIScreen(ndpy, screen);

   if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
       return GL_FALSE;
   }
   psp = (__DRIscreen *) pDRIScreen->private;
   *X = 0;
   *Y = 0;
   *W = surf->Base.Width;
   *H = surf->Base.Height;

   *backX = 0;
   *backY = 0;
   *numBackClipRects = 0;
   *pBackClipRects = NULL;

   *numClipRects = 1;
   *pClipRects = malloc(sizeof(**pClipRects));
   **pClipRects = (drm_clip_rect_t){0, 0, surf->Base.Width, surf->Base.Height};

   psp->pSAREA->drawableTable[0].stamp = 1;
   *stamp = 1;
#if 0
    GLXDrawable drawable = (GLXDrawable) draw;
    drm_clip_rect_t * cliprect;
    Display* display = (Display*)dpy;
    __DRIcontext *pcp = (__DRIcontext *)CurrentContext->driContext.private;
    if (drawable == 0) {
        return GL_FALSE;
    }

    cliprect = (drm_clip_rect_t*) malloc(sizeof(drm_clip_rect_t));
    cliprect->x1 = drawable->x;
    cliprect->y1 = drawable->y;
    cliprect->x2 = drawable->x + drawable->w;
    cliprect->y2 = drawable->y + drawable->h;
    
    /* the drawable index is by client id */
    *index = display->clientID;

    *stamp = pcp->driScreenPriv->pSAREA->drawableTable[display->clientID].stamp;
    *x = drawable->x;
    *y = drawable->y;
    *width = drawable->w;
    *height = drawable->h;
    *numClipRects = 1;
    *pClipRects = cliprect;
    
    *backX = drawable->x;
    *backY = drawable->y;
    *numBackClipRects = 0;
    *pBackClipRects = 0;
#endif
   return GL_TRUE;
}


/**
 * Implement \c __DRIinterfaceMethods::getProcAddress.
 */
static __DRIfuncPtr
get_proc_address(const char * proc_name)
{
   return NULL;
}


/**
 * Destroy a linked list of \c __GLcontextModes structures created by
 * \c _gl_context_modes_create.
 * 
 * \param modes  Linked list of structures to be destroyed.  All structres
 *               in the list will be freed.
 */
static void
__egl_context_modes_destroy(__GLcontextModes *modes)
{
   while ( modes != NULL ) {
      __GLcontextModes * const next = modes->next;

      free( modes );
      modes = next;
   }
}


/**
 * Allocate a linked list of \c __GLcontextModes structures.  The fields of
 * each structure will be initialized to "reasonable" default values.  In
 * most cases this is the default value defined by table 3.4 of the GLX
 * 1.3 specification.  This means that most values are either initialized to
 * zero or \c GLX_DONT_CARE (which is -1).  As support for additional
 * extensions is added, the new values will be initialized to appropriate
 * values from the extension specification.
 * 
 * \param count         Number of structures to allocate.
 * \param minimum_size  Minimum size of a structure to allocate.  This allows
 *                      for differences in the version of the
 *                      \c __GLcontextModes stucture used in libGL and in a
 *                      DRI-based driver.
 * \returns A pointer to the first element in a linked list of \c count
 *          stuctures on success, or \c NULL on failure.
 * 
 * \warning Use of \c minimum_size does \b not guarantee binary compatibility.
 *          The fundamental assumption is that if the \c minimum_size
 *          specified by the driver and the size of the \c __GLcontextModes
 *          structure in libGL is the same, then the meaning of each byte in
 *          the structure is the same in both places.  \b Be \b careful!
 *          Basically this means that fields have to be added in libGL and
 *          then propagated to drivers.  Drivers should \b never arbitrarilly
 *          extend the \c __GLcontextModes data-structure.
 */
static __GLcontextModes *
__egl_context_modes_create(unsigned count, size_t minimum_size)
{
   const size_t size = (minimum_size > sizeof( __GLcontextModes ))
       ? minimum_size : sizeof( __GLcontextModes );
   __GLcontextModes * base = NULL;
   __GLcontextModes ** next;
   unsigned   i;

   next = & base;
   for ( i = 0 ; i < count ; i++ ) {
      *next = (__GLcontextModes *) malloc( size );
      if ( *next == NULL ) {
	 __egl_context_modes_destroy( base );
	 base = NULL;
	 break;
      }

      (void) memset( *next, 0, size );
      (*next)->visualID = GLX_DONT_CARE;
      (*next)->visualType = GLX_DONT_CARE;
      (*next)->visualRating = GLX_NONE;
      (*next)->transparentPixel = GLX_NONE;
      (*next)->transparentRed = GLX_DONT_CARE;
      (*next)->transparentGreen = GLX_DONT_CARE;
      (*next)->transparentBlue = GLX_DONT_CARE;
      (*next)->transparentAlpha = GLX_DONT_CARE;
      (*next)->transparentIndex = GLX_DONT_CARE;
      (*next)->xRenderable = GLX_DONT_CARE;
      (*next)->fbconfigID = GLX_DONT_CARE;
      (*next)->swapMethod = GLX_SWAP_UNDEFINED_OML;

      next = & ((*next)->next);
   }

   return base;
}


static GLboolean
__eglWindowExists(__DRInativeDisplay *dpy, __DRIid draw)
{
    return EGL_TRUE;
}


/**
 * Get the unadjusted system time (UST).  Currently, the UST is measured in
 * microseconds since Epoc.  The actual resolution of the UST may vary from
 * system to system, and the units may vary from release to release.
 * Drivers should not call this function directly.  They should instead use
 * \c glXGetProcAddress to obtain a pointer to the function.
 *
 * \param ust Location to store the 64-bit UST
 * \returns Zero on success or a negative errno value on failure.
 * 
 * \sa glXGetProcAddress, PFNGLXGETUSTPROC
 *
 * \since Internal API version 20030317.
 */
static int
__eglGetUST(int64_t *ust)
{
    struct timeval  tv;
    
    if ( ust == NULL ) {
	return -EFAULT;
    }

    if ( gettimeofday( & tv, NULL ) == 0 ) {
	ust[0] = (tv.tv_sec * 1000000) + tv.tv_usec;
	return 0;
    }
    else {
	return -errno;
    }
}

/**
 * Determine the refresh rate of the specified drawable and display.
 * 
 * \param dpy          Display whose refresh rate is to be determined.
 * \param drawable     Drawable whose refresh rate is to be determined.
 * \param numerator    Numerator of the refresh rate.
 * \param demoninator  Denominator of the refresh rate.
 * \return  If the refresh rate for the specified display and drawable could
 *          be calculated, True is returned.  Otherwise False is returned.
 * 
 * \note This function is implemented entirely client-side.  A lot of other
 *       functionality is required to export GLX_OML_sync_control, so on
 *       XFree86 this function can be called for direct-rendering contexts
 *       when GLX_OML_sync_control appears in the client extension string.
 */
static GLboolean
__eglGetMSCRate(__DRInativeDisplay * dpy, __DRIid drawable,
                int32_t * numerator, int32_t * denominator)
{
   return EGL_TRUE;
}


/**
 * Table of functions exported by the loader to the driver.
 */
static const __DRIinterfaceMethods interface_methods = {
    get_proc_address,

    __egl_context_modes_create,
    __egl_context_modes_destroy,

    __eglFindDRIScreen,
    __eglWindowExists,

    __eglCreateContextWithConfig,
    __eglDestroyContext,

    __eglCreateDrawable,
    __eglDestroyDrawable,
    __eglGetDrawableInfo,

    __eglGetUST,
    __eglGetMSCRate,
};


static int
__glXGetInternalVersion(void)
{
    return 20050725;
}

static const char createNewScreenName[] = "__driCreateNewScreen_20050727";


/**
 * Do per-display initialization.
 */
EGLBoolean
_eglDRICreateDisplay(driDisplay *dpy, __DRIframebuffer *framebuffer)
{
   PFNCREATENEWSCREENFUNC createNewScreen;
    int api_ver = __glXGetInternalVersion();
   __DRIversion ddx_version;
   __DRIversion dri_version;
   __DRIversion drm_version;
   drmVersionPtr version;

   version = drmGetVersion( dpy->drmFD );
   if ( version ) {
      drm_version.major = version->version_major;
      drm_version.minor = version->version_minor;
      drm_version.patch = version->version_patchlevel;
      drmFreeVersion( version );
   }
   else {
      drm_version.major = -1;
      drm_version.minor = -1;
      drm_version.patch = -1;
   }

   /*
    * Get device name (like "tdfx") and the ddx version numbers.
    * We'll check the version in each DRI driver's "createScreen"
    * function.
    */
   ddx_version.major = 4;
   ddx_version.minor = 0;
   ddx_version.patch = 0;

   /*
    * Get the DRI X extension version.
    */
   dri_version.major = 4;
   dri_version.minor = 0;
   dri_version.patch = 0;

   createNewScreen = ( PFNCREATENEWSCREENFUNC ) dlsym( dpy->Base.Driver->LibHandle, createNewScreenName );
   if ( !createNewScreen ) {
      _eglLog(_EGL_WARNING, "Couldn't find %s function in the driver.",
              createNewScreenName );
      return EGL_FALSE;
   }

   dpy->driScreen.private = createNewScreen( dpy, 0, &dpy->driScreen, NULL,
                            &ddx_version, &dri_version,
                            &drm_version, framebuffer,
                            dpy->pSAREA, dpy->drmFD,
                            api_ver,
                            & interface_methods,
                            NULL);
   if (!dpy->driScreen.private) {
      _eglLog(_EGL_WARNING, "egldri.c: DRI create new screen failed");
      return EGL_FALSE;
   }

   DRM_UNLOCK( dpy->drmFD, dpy->pSAREA, dpy->serverContext );

   return EGL_TRUE;
}


/**
 * Create all the EGL screens for the given display.
 */
EGLBoolean
_eglDRICreateScreens(driDisplay *dpy)
{
   const int numScreens = 1;  /* XXX fix this someday */
   int i;

   for (i = 0; i < numScreens; i++) {
      char path[ NAME_MAX ];
      FILE *file;
      driScreen *s;

      /* Create a screen */
      if ( !( s = ( driScreen * ) calloc( 1, sizeof( *s ) ) ) )
         return EGL_FALSE;

      snprintf( s->fb, NAME_MAX, "fb%d", dpy->minor );
      _eglInitScreen( &s->Base );

      _eglAddScreen( &dpy->Base, &s->Base );

      /* Create the screen's mode list */
      snprintf( path, sizeof( path ), "%s/graphics/%s/modes", sysfs, s->fb );
      file = fopen( path, "r" );
      while ( fgets( path, sizeof( path ), file ) ) {
         unsigned int x, y, r;
         char c;
         path[ strlen( path ) - 1 ] = '\0';  /* strip off \n from sysfs */
         sscanf( path, "%c:%ux%u-%u", &c, &x, &y, &r );
         _eglAddNewMode( &s->Base, x, y, r * 1000, path );
      }
      fclose( file );

      /*
       * NOTE: we used to set the colormap here, but that didn't work reliably.
       * Some entries near the start of the table would get corrupted by later
       * mode changes.
       */
   }

   return EGL_TRUE;
}


EGLBoolean
_eglDRIInitialize(_EGLDriver *drv, EGLDisplay dpy,
                  EGLint *major, EGLint *minor)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   driDisplay *display;
   const char *driverName = (const char *) disp->NativeDisplay;

   assert(disp);

   /* Create new driDisplay object to replace the _EGLDisplay that was
    * previously created.
    */
   display = calloc(1, sizeof(*display));
   display->Base = *disp;
   _eglSaveDisplay(&display->Base);
   free(disp);

   *major = 1;
   *minor = 0;

   sscanf(driverName + 1, "%d", &display->minor);

   drv->Initialized = EGL_TRUE;
   return EGL_TRUE;
}


static EGLBoolean
_eglDRITerminate(_EGLDriver *drv, EGLDisplay dpy)
{
   driDisplay *display = Lookup_driDisplay(dpy);
   _eglCleanupDisplay(&display->Base);/*rename that function*/
   free(display);
   free(drv);
   return EGL_TRUE;
}


/**
 * Plug in the DRI-specific functions into the driver's dispatch table.
 * Also, enable some EGL extensions.
 */
void
_eglDRIInitDriverFallbacks(_EGLDriver *drv)
{
   _eglInitDriverFallbacks(drv);

   drv->API.Initialize = _eglDRIInitialize;
   drv->API.Terminate = _eglDRITerminate;
   drv->API.CreateContext = _eglDRICreateContext;
   drv->API.MakeCurrent = _eglDRIMakeCurrent;
   drv->API.CreatePbufferSurface = _eglDRICreatePbufferSurface;
   drv->API.DestroySurface = _eglDRIDestroySurface;
   drv->API.DestroyContext = _eglDRIDestroyContext;
   drv->API.CreateScreenSurfaceMESA = _eglDRICreateScreenSurfaceMESA;
   drv->API.ShowScreenSurfaceMESA = _eglDRIShowScreenSurfaceMESA;
   drv->API.SwapBuffers = _eglDRISwapBuffers;

   /* enable supported extensions */
   drv->Extensions.MESA_screen_surface = EGL_TRUE;
   drv->Extensions.MESA_copy_context = EGL_TRUE;
}
