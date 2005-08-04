/*
 * Generic EGL driver for DRI
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
#include "eglsurface.h"
#include "eglscreen.h"
#include "eglglobals.h"
#include "eglmode.h"

#include "egldri.h"

const char *sysfs = "/sys/class";
#define None 0
static const int empty_attribute_list[1] = { None };

/**
 * The bootstrap function.  Return a new driDriver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(_EGLDisplay *dpy)
{
   int length;
   char path[NAME_MAX];
   struct dirent *dirent;
   FILE *file;
   DIR *dir;
   _EGLDriver *driver = NULL;;

   snprintf(path, sizeof(path), "%s/drm", sysfs);
   if (!(dir = opendir(path))) {
      printf("EGL - %s DRM devices not found.", path);
      return EGL_FALSE;
   }
   while ((dirent = readdir(dir))) {

      if (strncmp(&dirent->d_name[0], "card", 4) != 0)
         continue;
      if (strcmp(&dirent->d_name[4], &dpy->Name[1]) != 0)
         continue;

      snprintf(path, sizeof(path), "%s/drm/card%s/dri_library_name", sysfs, &dpy->Name[1]);
      file = fopen(path, "r");
      fgets(path, sizeof(path), file);
      fclose(file);

      if ((length = strlen(path)) > 0)
         path[length - 1] = '\0';  /* remove the trailing newline from sysfs */
      strncat(path, "_dri", sizeof(path));

      driver = _eglOpenDriver(dpy, path);

      break;
   }
   closedir(dir);

   return driver;
}


static EGLContext
_eglDRICreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   driContext *c;
   driDisplay *disp = Lookup_driDisplay(dpy);
   driContext *share = Lookup_driContext(share_list);
   void *sharePriv;
   __GLcontextModes mode;
   int i;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreateContext");
      return EGL_NO_CONTEXT;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs defined for now */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
         return EGL_NO_CONTEXT;
      }
   }

   c = (driContext *) calloc(1, sizeof(*c));
   if (!c)
      return EGL_NO_CONTEXT;

   _eglInitContext(&c->Base);
   c->Base.Display = &disp->Base;
   c->Base.Config = conf;
   c->Base.DrawSurface = EGL_NO_SURFACE;
   c->Base.ReadSurface = EGL_NO_SURFACE;

   _eglConfigToContextModesRec(conf, &mode);

   if (share)
      sharePriv = share->driContext.private;
   else
      sharePriv = NULL;

   c->driContext.private = disp->driScreen.createNewContext(disp, &mode,
           GLX_WINDOW_BIT, sharePriv, &c->driContext);

   if (!c->driContext.private) {
      free(c);
      return EGL_FALSE;
   }

   /* generate handle and insert into hash table */
   _eglSaveContext(&c->Base);
   assert(c->Base.Handle);

   return c->Base.Handle;
}


static EGLBoolean
_eglDRIMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext context)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driContext *ctx = Lookup_driContext(context);
   EGLBoolean b;

   b = _eglMakeCurrent(drv, dpy, draw, read, context);
   if (!b)
      return EGL_FALSE;

   if (ctx) {
      ctx->driContext.bindContext(disp, 0, read, draw, &ctx->driContext);
   } else {
//      _mesa_make_current( NULL, NULL, NULL );
   }
   return EGL_TRUE;
}


static EGLSurface
_eglDRICreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
{
   int i;
   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs at this time */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateWindowSurface");
         return EGL_NO_SURFACE;
      }
   }
   printf("eglCreateWindowSurface()\n");
   /* XXX unfinished */

   return EGL_NO_SURFACE;
}


static EGLSurface
_eglDRICreatePixmapSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   EGLint i;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreatePixmapSurface");
      return EGL_NO_SURFACE;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs at this time */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreatePixmapSurface");
         return EGL_NO_SURFACE;
      }
   }

   if (conf->Attrib[EGL_SURFACE_TYPE - FIRST_ATTRIB] == 0) {
      _eglError(EGL_BAD_MATCH, "eglCreatePixmapSurface");
      return EGL_NO_SURFACE;
   }

   printf("eglCreatePixmapSurface()\n");
   return EGL_NO_SURFACE;
}


static EGLSurface
_eglDRICreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
   driSurface *surf;

   surf = (driSurface *) calloc(1, sizeof(*surf));
   if (!surf) {
      return EGL_NO_SURFACE;
   }

   if (_eglInitPbufferSurface(&surf->Base, drv, dpy, config, attrib_list) == EGL_NO_SURFACE) {
      free(surf);
      return EGL_NO_SURFACE;
   }

   /* create software-based pbuffer */
   {
//      GLcontext *ctx = NULL; /* this _should_ be OK */
      GLvisual vis;
      _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);
      assert(conf); /* bad config should be caught earlier */
      _eglConfigToContextModesRec(conf, &vis);

#if 0
      surf->mesa_framebuffer = _mesa_create_framebuffer(&vis);
      _mesa_add_soft_renderbuffers(surf->mesa_framebuffer,
                                   GL_TRUE, /* color bufs */
                                   vis.haveDepthBuffer,
                                   vis.haveStencilBuffer,
                                   vis.haveAccumBuffer,
                                   GL_FALSE, /* alpha */
                                   GL_FALSE /* aux */ );

      /* set pbuffer/framebuffer size */
      _mesa_resize_framebuffer(ctx, surf->mesa_framebuffer,
                               surf->Base.Width, surf->Base.Height);
#endif
   }

   return surf->Base.Handle;
}


static EGLBoolean
_eglDRIDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driSurface *fs = Lookup_driSurface(surface);
   _eglRemoveSurface(&fs->Base);

   fs->drawable.destroyDrawable(disp, fs->drawable.private);

   if (fs->Base.IsBound) {
      fs->Base.DeletePending = EGL_TRUE;
   }
   else {
      free(fs);
   }
   return EGL_TRUE;
}


static EGLBoolean
_eglDRIDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext context)
{
   driDisplay *disp = Lookup_driDisplay(dpy);
   driContext *fc = Lookup_driContext(context);

   _eglRemoveContext(&fc->Base);

   fc->driContext.destroyContext(disp, 0, fc->driContext.private);

   if (fc->Base.IsBound) {
      fc->Base.DeletePending = EGL_TRUE;
   } else {
      free(fc);
   }
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
   EGLSurface surf;
   GLvisual vis;

   surface = (driSurface *) malloc(sizeof(*surface));
   if (!surface) {
      return EGL_NO_SURFACE;
   }

   /* init base class, error check, etc. */
   surf = _eglInitScreenSurface(&surface->Base, drv, dpy, cfg, attrib_list);
   if (surf == EGL_NO_SURFACE) {
      free(surface);
      return EGL_NO_SURFACE;
   }

   /* convert EGLConfig to GLvisual */
   _eglConfigToContextModesRec(config, &vis);

   /* Create a new drawable */
   if (!disp->driScreen.createNewDrawable(disp, &vis, surf, &surface->drawable,
                           GLX_WINDOW_BIT, empty_attribute_list)) {
      free(surface);
      _eglRemoveSurface(&surface->Base);
      return EGL_NO_SURFACE;
   }
   return surf;
}


/**
 * Show the given surface on the named screen.
 * If surface is EGL_NO_SURFACE, disable the screen's output.
 */
EGLBoolean
_eglDRIShowSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen,
                    EGLSurface surface, EGLModeMESA m)
{
   driDisplay *display = Lookup_driDisplay(dpy);
   driScreen *scrn = Lookup_driScreen(dpy, screen);
   driSurface *surf = Lookup_driSurface(surface);
   FILE *file;
   char buffer[NAME_MAX];
   _EGLMode *mode = _eglLookupMode(dpy, m);
   int width, height, temp;

   _eglQuerySurface(drv, dpy, surface, EGL_WIDTH, &width);
   _eglQuerySurface(drv, dpy, surface, EGL_HEIGHT, &height);

   if (!_eglShowSurfaceMESA(drv, dpy, screen, surface, m))
      return EGL_FALSE;

   snprintf(buffer, sizeof(buffer), "%s/graphics/%s/blank", sysfs, scrn->fb);

   file = fopen(buffer, "r+");
   if (!file) {
err:
      printf("kernel patch?? chown all fb sysfs attrib to allow write - %s\n", buffer);
      _eglError(EGL_BAD_SURFACE, "eglShowSurfaceMESA");
      return EGL_FALSE;
   }
   snprintf(buffer, sizeof(buffer), "%d", (m == EGL_NO_MODE_MESA ? VESA_POWERDOWN : VESA_VSYNC_SUSPEND));
   fputs(buffer, file);
   fclose(file);

   if (m == EGL_NO_MODE_MESA)
      return EGL_TRUE;

   snprintf(buffer, sizeof(buffer), "%s/graphics/%s/mode", sysfs, scrn->fb);

   file = fopen(buffer, "r+");
   if (!file)
      goto err;
   fputs(mode->Name, file);
   fclose(file);

   snprintf(buffer, sizeof(buffer), "%s/graphics/%s/bits_per_pixel", sysfs, scrn->fb);

   file = fopen(buffer, "r+");
   if (!file)
      goto err;
   display->bpp = GET_CONFIG_ATTRIB(surf->Base.Config, EGL_BUFFER_SIZE);
   display->cpp = display->bpp / 8;
   snprintf(buffer, sizeof(buffer), "%d", display->bpp);
   fputs(buffer, file);
   fclose(file);

   snprintf(buffer, sizeof(buffer), "%s/graphics/%s/blank", sysfs, scrn->fb);

   file = fopen(buffer, "r+");
   if (!file)
      goto err;

   snprintf(buffer, sizeof(buffer), "%d", VESA_NO_BLANKING);
   fputs(buffer, file);
   fclose(file);

   snprintf(buffer, sizeof(buffer), "%s/graphics/%s/virtual_size", sysfs, scrn->fb);
   file = fopen(buffer, "r+");
   snprintf(buffer, sizeof(buffer), "%d,%d", width, height);
   fputs(buffer, file);
   rewind(file);
   fgets(buffer, sizeof(buffer), file);
   sscanf(buffer, "%d,%d", &display->virtualWidth, &display->virtualHeight);
   fclose(file);

   temp = display->virtualWidth;
   switch (display->bpp / 8) {
   case 1: temp = (display->virtualWidth + 127) & ~127; break;
   case 2: temp = (display->virtualWidth +  31) &  ~31; break;
   case 3:
   case 4: temp = (display->virtualWidth +  15) &  ~15; break;
   }
   display->virtualWidth = temp;

   if ((width != display->virtualWidth) || (height != display->virtualHeight))
      goto err;

   return EGL_TRUE;
}


/* If the backbuffer is on a videocard, this is extraordinarily slow!
 */
static EGLBoolean
_eglDRISwapBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw)
{
   driSurface *drawable = Lookup_driSurface(draw);

   if (!_eglSwapBuffers(drv, dpy, draw))
      return EGL_FALSE;

   drawable->drawable.swapBuffers(NULL, drawable->drawable.private);

   return EGL_TRUE;
}


EGLBoolean
_eglDRIGetDisplayInfo( driDisplay *dpy) {
   char path[ NAME_MAX ];
   FILE *file;
   int rc, mtrr;
   unsigned int i;
   drmMapType type;
   drmMapFlags flags;
   drm_handle_t handle, offset;
   drmSize size;
   drmSetVersion sv;
   drm_magic_t magic;

   snprintf( path, sizeof( path ), "%s/graphics/fb%d/device/device", sysfs, dpy->minor );
   file = fopen( path, "r" );
   fgets( path, sizeof( path ), file );
   sscanf( path, "%x", &dpy->chipset );
   fclose( file );

   sprintf(path, DRM_DEV_NAME, DRM_DIR_NAME, dpy->minor);
   if ( ( dpy->drmFD = open(path, O_RDWR, 0) ) < 0 ) {
      fprintf( stderr, "[drm] drmOpen failed\n" );
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

   for ( i = 0;; i++ ) {
      if ( ( rc = drmGetMap( dpy->drmFD, i, &offset, &size, &type, &flags, &handle, &mtrr ) ) != 0 )
         break;
      if ( type == DRM_FRAME_BUFFER ) {
         if ( ( rc = drmMap( dpy->drmFD, offset, size, ( drmAddressPtr ) & dpy->pFB ) ) < 0 )
            return EGL_FALSE;
         dpy->fbSize = size;
         break;
      }
   }
   if ( !dpy->pFB )
      return EGL_FALSE;

   dpy->SAREASize = SAREA_MAX;

   for ( i = 0;; i++ ) {
      if ( drmGetMap( dpy->drmFD, i, &offset, &size, &type, &flags, &handle, &mtrr ) != 0 )
         break;
      if ( type == DRM_SHM ) {
         if ( drmMap( dpy->drmFD, offset, size, ( drmAddressPtr ) ( &dpy->pSAREA ) ) < 0 ) {
            fprintf( stderr, "[drm] drmMap failed\n" );
            return 0;
         }
         break;
      }
   }
   if ( !dpy->pSAREA )
      return 0;

   memset( dpy->pSAREA, 0, dpy->SAREASize );
   fprintf( stderr, "[drm] mapped SAREA 0x%08lx to %p, size %d\n",
            offset, dpy->pSAREA, dpy->SAREASize );
   return EGL_TRUE;
}


 /* Return the DRI per screen structure */
static __DRIscreen *__eglFindDRIScreen(__DRInativeDisplay *ndpy, int scrn)
{
   driDisplay *disp = (driDisplay *)ndpy;
   return &disp->driScreen;
}

static GLboolean __eglCreateContextWithConfig(__DRInativeDisplay* ndpy, int screen, int configID, void* context, drm_context_t * hHWContext)
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreenPrivate *) pDRIScreen->private;
    if (psp->fd) {
        if (drmCreateContext(psp->fd, hHWContext)) {
            fprintf(stderr, ">>> drmCreateContext failed\n");
            return GL_FALSE;
        }
        *(void**)context = (void*) *hHWContext;
    }
#if 0
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;

    pDRIScreen = __glXFindDRIScreen(dpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }

    psp = (__DRIscreenPrivate *) pDRIScreen->private;

    if (psp->fd) {
        if (drmCreateContext(psp->fd, hHWContext)) {
            fprintf(stderr, ">>> drmCreateContext failed\n");
            return GL_FALSE;
        }
        *(void**)contextID = (void*) *hHWContext;
    }
#endif
    return GL_TRUE;
}

static GLboolean __eglDestroyContext( __DRInativeDisplay * ndpy, int screen,  __DRIid context )
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreenPrivate *) pDRIScreen->private;
    if (psp->fd)
      drmDestroyContext(psp->fd, context);

   return GL_TRUE;
}

static GLboolean __eglCreateDrawable( __DRInativeDisplay * ndpy, int screen, __DRIid drawable, drm_drawable_t * hHWDrawable )
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreenPrivate *) pDRIScreen->private;
    if (psp->fd) {
        if (drmCreateDrawable(psp->fd, hHWDrawable)) {
            fprintf(stderr, ">>> drmCreateDrawable failed\n");
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

static GLboolean __eglDestroyDrawable( __DRInativeDisplay * ndpy, int screen, __DRIid drawable )
{
    __DRIscreen *pDRIScreen;
    __DRIscreenPrivate *psp;

    pDRIScreen = __eglFindDRIScreen(ndpy, screen);
    if ( (pDRIScreen == NULL) || (pDRIScreen->private == NULL) ) {
        return GL_FALSE;
    }
    psp = (__DRIscreenPrivate *) pDRIScreen->private;
    if (psp->fd)
      drmDestroyDrawable(psp->fd, drawable);

   return GL_TRUE;
}

static GLboolean __eglGetDrawableInfo(__DRInativeDisplay * ndpy, int screen, __DRIid drawable,
    unsigned int* index, unsigned int* stamp,
    int* X, int* Y, int* W, int* H,
    int* numClipRects, drm_clip_rect_t ** pClipRects,
    int* backX, int* backY,
    int* numBackClipRects, drm_clip_rect_t ** pBackClipRects )
{
   driSurface *surf = Lookup_driSurface(drawable);

   *X = 0;
   *Y = 0;
   *W = surf->Base.Width;
   *H = surf->Base.Height;

   *numClipRects = 1;
   *pClipRects = malloc(sizeof(**pClipRects));
   **pClipRects = (drm_clip_rect_t){0, 0, surf->Base.Width, surf->Base.Height};

#if 0
    GLXDrawable drawable = (GLXDrawable) draw;
    drm_clip_rect_t * cliprect;
    Display* display = (Display*)dpy;
    __DRIcontextPrivate *pcp = (__DRIcontextPrivate *)CurrentContext->driContext.private;
    if (drawable == 0) {
        return GL_FALSE;
    }

    cliprect = (drm_clip_rect_t*) _mesa_malloc(sizeof(drm_clip_rect_t));
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
static __DRIfuncPtr get_proc_address( const char * proc_name )
{
#if 0
   if (strcmp( proc_name, "glxEnableExtension" ) == 0) {
      return (__DRIfuncPtr) __glXScrEnableExtension;
   }
#endif
   return NULL;
}


/**
 * Destroy a linked list of \c __GLcontextModes structures created by
 * \c _gl_context_modes_create.
 * 
 * \param modes  Linked list of structures to be destroyed.  All structres
 *               in the list will be freed.
 */
void
__egl_context_modes_destroy( __GLcontextModes * modes )
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
__GLcontextModes *
__egl_context_modes_create( unsigned count, size_t minimum_size )
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


GLboolean __eglWindowExists(__DRInativeDisplay *dpy, __DRIid draw)
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
int __eglGetUST( int64_t * ust )
{
    struct timeval  tv;
    
    if ( ust == NULL ) {
	return -EFAULT;
    }

    if ( gettimeofday( & tv, NULL ) == 0 ) {
	ust[0] = (tv.tv_sec * 1000000) + tv.tv_usec;
	return 0;
    } else {
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
GLboolean __eglGetMSCRate(__DRInativeDisplay * dpy, __DRIid drawable, int32_t * numerator, int32_t * denominator)
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


int __glXGetInternalVersion(void)
{
    return 20050725;
}

static const char createNewScreenName[] = "__driCreateNewScreen_20050727";

EGLBoolean
_eglDRICreateDisplay( driDisplay *dpy, __DRIframebuffer *framebuffer) {
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
   } else {
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
      fprintf( stderr, "Couldn't find %s in CallCreateNewScreen\n", createNewScreenName );
      return EGL_FALSE;
   }

   dpy->driScreen.private = createNewScreen( dpy, 0, &dpy->driScreen, NULL,
                            &ddx_version, &dri_version,
                            &drm_version, framebuffer,
                            dpy->pSAREA, dpy->drmFD,
                            api_ver,
                            & interface_methods,
                            ( __GLcontextModes ** ) & dpy->driver_modes );
   if (!dpy->driScreen.private)
      return EGL_FALSE;

   DRM_UNLOCK( dpy->drmFD, dpy->pSAREA, dpy->serverContext );

   return EGL_TRUE;
}


EGLBoolean
_eglDRICreateScreen( driDisplay *dpy) {
   char c, *buffer, path[ NAME_MAX ];
   unsigned int i, x, y, r;
   int fd;
   FILE *file;
   driScreen *s;
   _EGLScreen *scrn;

   /* Create a screen */
   if ( !( s = ( driScreen * ) calloc( 1, sizeof( *s ) ) ) )
      return EGL_FALSE;

   snprintf( s->fb, NAME_MAX, "fb%d", dpy->minor );
   scrn = &s->Base;
   _eglInitScreen( scrn );
   _eglAddScreen( &dpy->Base, scrn );

   snprintf( path, sizeof( path ), "%s/graphics/%s/modes", sysfs, s->fb );
   file = fopen( path, "r" );
   while ( fgets( path, sizeof( path ), file ) ) {
      path[ strlen( path ) - 1 ] = '\0';  /* strip off \n from sysfs */
      sscanf( path, "%c:%ux%u-%u", &c, &x, &y, &r );
      _eglAddMode( scrn, x, y, r * 1000, path );
   }
   fclose( file );

   /* cmap attribute uses 256 lines of 16 bytes */
   if ( !( buffer = malloc( 256 * 16 ) ) )
      return EGL_FALSE;

   /* cmap attribute uses 256 lines of 16 bytes */
   for ( i = 0; i < 256; i++ )
      sprintf( &buffer[ i * 16 ], "%02x%c%4x%4x%4x\n",
               i, ' ', 256 * i, 256 * i, 256 * i );

   snprintf( path, sizeof( path ), "%s/graphics/%s/color_map", sysfs, s->fb );
   if ( !( fd = open( path, O_RDWR ) ) )
      return EGL_FALSE;
   write( fd, buffer, 256 * 16 );
   close( fd );

   free( buffer );

   return EGL_TRUE;
}

EGLBoolean
_eglDRIInitialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   driDisplay *display;

   /* Switch display structure to one with our private fields */
   display = calloc(1, sizeof(*display));
   display->Base = *disp;
   _eglHashInsert(_eglGlobal.Displays, disp->Handle, display);
   free(disp);

   *major = 1;
   *minor = 0;

   sscanf(&disp->Name[1], "%d", &display->minor);

   drv->Initialized = EGL_TRUE;
   return EGL_TRUE;
}


static EGLBoolean
_eglDRITerminate(_EGLDriver *drv, EGLDisplay dpy)
{
   driDisplay *display = Lookup_driDisplay(dpy);
   _eglCleanupDisplay(&display->Base);
   free(display);
   free(drv);
   return EGL_TRUE;
}


void
_eglDRIInitDriverFallbacks(_EGLDriver *drv)
{
   _eglInitDriverFallbacks(drv);

   drv->Initialize = _eglDRIInitialize;
   drv->Terminate = _eglDRITerminate;
   drv->CreateContext = _eglDRICreateContext;
   drv->MakeCurrent = _eglDRIMakeCurrent;
   drv->CreateWindowSurface = _eglDRICreateWindowSurface;
   drv->CreatePixmapSurface = _eglDRICreatePixmapSurface;
   drv->CreatePbufferSurface = _eglDRICreatePbufferSurface;
   drv->DestroySurface = _eglDRIDestroySurface;
   drv->DestroyContext = _eglDRIDestroyContext;
   drv->CreateScreenSurfaceMESA = _eglDRICreateScreenSurfaceMESA;
   drv->ShowSurfaceMESA = _eglDRIShowSurfaceMESA;
   drv->SwapBuffers = _eglDRISwapBuffers;

   /* enable supported extensions */
   drv->MESA_screen_surface = EGL_TRUE;
   drv->MESA_copy_context = EGL_TRUE;

}
