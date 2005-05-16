/*
 * Test egl driver for fb_dri.so
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h> 
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

#include "utils.h"
#include "buffers.h"
#include "extensions.h"
#include "framebuffer.h"
#include "renderbuffer.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"
#include "drirenderbuffer.h"

#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglmode.h"
#include "eglscreen.h"
#include "eglsurface.h"

extern void
fbSetSpanFunctions(driRenderbuffer *drb, const GLvisual *vis);

/**
 * fb driver-specific driver class derived from _EGLDriver
 */
typedef struct fb_driver
{
   _EGLDriver Base;  /* base class/object */
   GLuint fbStuff;
} fbDriver;

/**
 * fb display-specific driver class derived from _EGLDisplay
 */
typedef struct fb_display
{
   _EGLDisplay Base;  /* base class/object */
   void *pFB;
} fbDisplay;

/**
 * fb driver-specific screen class derived from _EGLScreen
 */
typedef struct fb_screen
{
   _EGLScreen Base;
   char fb[NAME_MAX];
} fbScreen;


/**
 * fb driver-specific surface class derived from _EGLSurface
 */
typedef struct fb_surface
{
   _EGLSurface Base;  /* base class/object */
   struct gl_framebuffer *mesa_framebuffer;
} fbSurface;


/**
 * fb driver-specific context class derived from _EGLContext
 */
typedef struct fb_context
{
   _EGLContext Base;  /* base class/object */
   GLcontext *glCtx;
} fbContext;


static EGLBoolean
fbFillInConfigs(_EGLDisplay *disp, unsigned pixel_bits, unsigned depth_bits,
               unsigned stencil_bits, GLboolean have_back_buffer) {
   _EGLConfig *configs;
   _EGLConfig *c;
   unsigned int i, num_configs;
   unsigned int depth_buffer_factor;
   unsigned int back_buffer_factor;
   GLenum fb_format;
   GLenum fb_type;

   /* Right now GLX_SWAP_COPY_OML isn't supported, but it would be easy
   * enough to add support.  Basically, if a context is created with an
   * fbconfig where the swap method is GLX_SWAP_COPY_OML, pageflipping
   * will never be used.
   */
   static const GLenum back_buffer_modes[] = {
            GLX_NONE, GLX_SWAP_UNDEFINED_OML /*, GLX_SWAP_COPY_OML */
         };

   u_int8_t depth_bits_array[2];
   u_int8_t stencil_bits_array[2];

   depth_bits_array[0] = depth_bits;
   depth_bits_array[1] = depth_bits;

   /* Just like with the accumulation buffer, always provide some modes
   * with a stencil buffer.  It will be a sw fallback, but some apps won't
   * care about that.
   */
   stencil_bits_array[0] = 0;
   stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;

   depth_buffer_factor = ((depth_bits != 0) || (stencil_bits != 0)) ? 2 : 1;
   back_buffer_factor = (have_back_buffer) ? 2 : 1;

   num_configs = depth_buffer_factor * back_buffer_factor * 4;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   } else {
      fb_format = GL_RGBA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   configs = calloc(sizeof(*configs), num_configs);
   c = configs;
   if (!_eglFillInConfigs(c, fb_format, fb_type,
                          depth_bits_array, stencil_bits_array, depth_buffer_factor,
                          back_buffer_modes, back_buffer_factor,
                          GLX_TRUE_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n",
               __func__, __LINE__);
      return EGL_FALSE;
   }

   c = &configs[depth_buffer_factor * back_buffer_factor * 2];
   if (!_eglFillInConfigs(c, fb_format, fb_type,
                          depth_bits_array, stencil_bits_array, depth_buffer_factor,
                          back_buffer_modes, back_buffer_factor,
                          GLX_DIRECT_COLOR)) {
      fprintf(stderr, "[%s:%u] Error creating FBConfig!\n",
               __func__, __LINE__);
      return EGL_FALSE;
   }

   /* Mark the visual as slow if there are "fake" stencil bits.
   */
   for (i = 0, c = configs; i < num_configs; i++, c++) {
      if ((c->glmode.stencilBits != 0)  && (c->glmode.stencilBits != stencil_bits)) {
         c->glmode.visualRating = GLX_SLOW_CONFIG;
      }
   }

   for (i = 0, c = configs; i < num_configs; i++, c++)
      _eglAddConfig(disp, c);
      
   free(configs);
   
   return EGL_TRUE;
}

static EGLBoolean
fbSetupFramebuffer(fbDisplay *disp, char *fbdev) 
{
   int fd;
   char dev[20];
   struct fb_var_screeninfo varInfo;
   struct fb_fix_screeninfo fixedInfo;
   
   snprintf(dev, sizeof(dev), "/dev/%s", fbdev);

   /* open the framebuffer device */
   fd = open(dev, O_RDWR);
   if (fd < 0) {
      fprintf(stderr, "Error opening %s: %s\n", fbdev, strerror(errno));
      return EGL_FALSE;
   }

   /* get the original variable screen info */
   if (ioctl(fd, FBIOGET_VSCREENINFO, &varInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_VSCREENINFO) failed: %s\n",
               strerror(errno));
      return EGL_FALSE;
   }

   /* Turn off hw accels (otherwise mmap of mmio region will be
    * refused)
    */
   if (varInfo.accel_flags) {
      varInfo.accel_flags = 0;
      if (ioctl(fd, FBIOPUT_VSCREENINFO, &varInfo)) {
         fprintf(stderr, "error: ioctl(FBIOPUT_VSCREENINFO) failed: %s\n",
                  strerror(errno));
         return EGL_FALSE;
      }
   }

   /* Get the fixed screen info */
   if (ioctl(fd, FBIOGET_FSCREENINFO, &fixedInfo)) {
      fprintf(stderr, "error: ioctl(FBIOGET_FSCREENINFO) failed: %s\n",
               strerror(errno));
      return EGL_FALSE;
   }

   /* mmap the framebuffer into our address space */
   disp->pFB = (caddr_t)mmap(0,  /* start */
                      fixedInfo.smem_len,  /* bytes */
                      PROT_READ | PROT_WRITE,  /* prot */
                      MAP_SHARED,  /* flags */
                      fd,  /* fd */
                      0); /* offset */ 
   if (disp->pFB == (caddr_t)-1) {
      fprintf(stderr, "error: unable to mmap framebuffer: %s\n",
               strerror(errno));
      return EGL_FALSE;
   }
   
   return EGL_TRUE;
}
   
const char *sysfs = "/sys/class/graphics";

static EGLBoolean
fbInitialize(_EGLDriver *drv, EGLDisplay dpy, EGLint *major, EGLint *minor)
{
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   fbDisplay *display;
   fbScreen *s;
   _EGLScreen *scrn;
   char c;
   unsigned int i, x, y, r;
   DIR *dir;
   FILE *file;
   struct dirent *dirent;
   char path[NAME_MAX];
   
   /* Switch display structure to one with our private fields */
   display = calloc(1, sizeof(*display));
   display->Base = *disp;
   _eglHashInsert(_eglGlobal.Displays, disp->Handle, display);
   free(disp);
   
   *major = 1;
   *minor = 0;
   
   dir = opendir(sysfs);
   if (!dir) {
      printf("EGL - %s framebuffer device not found.", sysfs);
      return EGL_FALSE;
   }
   
   while (dirent = readdir(dir)) {
      
      if (dirent->d_name[0] != 'f')
         continue;
      if (dirent->d_name[1] != 'b')
         continue;
   
      if (fbSetupFramebuffer(display, dirent->d_name) == EGL_FALSE)
         continue;
         
      /* Create a screen */
      s = (fbScreen *) calloc(1, sizeof(fbScreen));
      if (!s)
         return EGL_FALSE;

      strncpy(s->fb, dirent->d_name, NAME_MAX);
      scrn = &s->Base;
      _eglInitScreen(scrn);
      _eglAddScreen(&display->Base, scrn);
      
      snprintf(path, sizeof(path), "%s/%s/modes", sysfs, s->fb);
      file = fopen(path, "r");
      while (fgets(path, sizeof(path), file)) {
         sscanf(path, "%c:%ux%u-%u", &c, &x, &y, &r);
         _eglAddMode(scrn, x, y, r * 1000, path);
      }
      fclose(file);

      fbFillInConfigs(&display->Base, 32, 24, 8, 1);
      
   }
   closedir(dir);

   drv->Initialized = EGL_TRUE;
   return EGL_TRUE;
}


static fbDisplay *
Lookup_fbDisplay(EGLDisplay dpy)
{
   _EGLDisplay *d = _eglLookupDisplay(dpy);
   return (fbDisplay *) d;
}


static fbScreen *
Lookup_fbScreen(EGLDisplay dpy, EGLScreenMESA screen)
{
   _EGLScreen *s = _eglLookupScreen(dpy, screen);
   return (fbScreen *) s;
}


static fbContext *
Lookup_fbContext(EGLContext ctx)
{
   _EGLContext *c = _eglLookupContext(ctx);
   return (fbContext *) c;
}


static fbSurface *
Lookup_fbSurface(EGLSurface surf)
{
   _EGLSurface *s = _eglLookupSurface(surf);
   return (fbSurface *) s;
}


static EGLBoolean
fbTerminate(_EGLDriver *drv, EGLDisplay dpy)
{
   fbDisplay *display = Lookup_fbDisplay(dpy);
   _eglCleanupDisplay(&display->Base);
   free(display);
   free(drv);
   return EGL_TRUE;
}


static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
   (void) ctx;
   switch (pname) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa dumb framebuffer";
      default:
         return NULL;
   }
}


static void
update_state( GLcontext *ctx, GLuint new_state )
{
   /* not much to do here - pass it on */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


/**
 * Called by ctx->Driver.GetBufferSize from in core Mesa to query the
 * current framebuffer size.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   *width  = buffer->Width;
   *height = buffer->Height;
}


static void
viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   _mesa_ResizeBuffersMESA();
}


static void
init_core_functions( struct dd_function_table *functions )
{
   functions->GetString = get_string;
   functions->UpdateState = update_state;
   functions->ResizeBuffers = _mesa_resize_framebuffer;
   functions->GetBufferSize = get_buffer_size;
   functions->Viewport = viewport;

   functions->Clear = _swrast_Clear;  /* could accelerate with blits */
}


static EGLContext
fbCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
   GLcontext *ctx;
   _EGLConfig *conf;
   fbContext *c;
   _EGLDisplay *disp = _eglLookupDisplay(dpy);
   struct dd_function_table functions;
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

   c = (fbContext *) calloc(1, sizeof(fbContext));
   if (!c)
      return EGL_NO_CONTEXT;

   _eglInitContext(&c->Base);
   c->Base.Display = disp;
   c->Base.Config = conf;
   c->Base.DrawSurface = EGL_NO_SURFACE;
   c->Base.ReadSurface = EGL_NO_SURFACE;
   printf("fbCreateContext\n");

   /* generate handle and insert into hash table */
   _eglSaveContext(&c->Base);
   assert(c->Base.Handle);

   /* Init default driver functions then plug in our FBdev-specific functions
    */
   _mesa_init_driver_functions(&functions);
   init_core_functions(&functions);

   ctx = c->glCtx = _mesa_create_context(&conf->glmode, NULL, &functions, (void *)c);
   if (!c->glCtx) {
      _mesa_free(c);
      return GL_FALSE;
   }

   /* Create module contexts */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _swsetup_Wakeup( ctx );


   /* swrast init -- need to verify these tests - I just plucked the
    * numbers out of the air.  (KW)
    */
   {
      struct swrast_device_driver *swdd;
      swdd = _swrast_GetDeviceDriverReference( ctx );
   }

   /* use default TCL pipeline */
   {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.RunPipeline = _tnl_run_pipeline;
   }

   _mesa_enable_sw_extensions(ctx);

   return c->Base.Handle;
}


static EGLSurface
fbCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list)
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
fbCreatePixmapSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list)
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
fbCreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   EGLint i, width = 0, height = 0, largest, texFormat, texTarget, mipmapTex;
   fbSurface *surf;

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

   surf = (fbSurface *) calloc(1, sizeof(fbSurface));
   if (!surf)
      return EGL_NO_SURFACE;

   surf->Base.Config = conf;
   surf->Base.Type = EGL_PBUFFER_BIT;
   surf->Base.Width = width;
   surf->Base.Height = height;
   surf->Base.TextureFormat = texFormat;
   surf->Base.TextureTarget = texTarget;
   surf->Base.MipmapTexture = mipmapTex;
   surf->Base.MipmapLevel = 0;
   surf->Base.SwapInterval = 0;

   printf("eglCreatePbufferSurface()\n");

   /* insert into hash table */
   _eglSaveSurface(&surf->Base);
   assert(surf->Base.Handle);

   return surf->Base.Handle;
}


static EGLBoolean
fbDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface)
{
   fbSurface *fs = Lookup_fbSurface(surface);
   _eglRemoveSurface(&fs->Base);
   if (fs->Base.IsBound) {
      fs->Base.DeletePending = EGL_TRUE;
   }
   else {
      free(fs);
   }
   return EGL_TRUE;
}


static EGLBoolean
fbDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext context)
{
   fbContext *fc = Lookup_fbContext(context);
   _eglRemoveContext(&fc->Base);
   if (fc->Base.IsBound) {
      fc->Base.DeletePending = EGL_TRUE;
   }
   else {
      free(fc);
   }
   return EGL_TRUE;
}


static EGLBoolean
fbMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext context)
{
   fbSurface *readSurf = Lookup_fbSurface(read);
   fbSurface *drawSurf = Lookup_fbSurface(draw);
   fbContext *ctx = Lookup_fbContext(context);
   EGLBoolean b;

   b = _eglMakeCurrent(drv, dpy, draw, read, context);
   if (!b)
      return EGL_FALSE;

   if (ctx) {
      _mesa_make_current( ctx->glCtx, 
                           drawSurf->mesa_framebuffer,
                           readSurf->mesa_framebuffer);
   } else
      _mesa_make_current( NULL, NULL, NULL );

   printf("eglMakeCurrent()\n");
   return EGL_TRUE;
}


static const char *
fbQueryString(_EGLDriver *drv, EGLDisplay dpy, EGLint name)
{
   if (name == EGL_EXTENSIONS) {
      return "EGL_MESA_screen_surface";
   }
   else {
      return _eglQueryString(drv, dpy, name);
   }
}


/**
 * Create a drawing surface which can be directly displayed on a screen.
 */
static EGLSurface
fbCreateScreenSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLConfig cfg,
                            const EGLint *attrib_list)
{
   _EGLConfig *config = _eglLookupConfig(drv, dpy, cfg);
   fbDisplay *display = Lookup_fbDisplay(dpy);
   fbSurface *surface;
   EGLSurface surf;
   
   const GLboolean swDepth = config->glmode.depthBits > 0;
   const GLboolean swAlpha = config->glmode.alphaBits > 0;
   const GLboolean swAccum = config->glmode.accumRedBits > 0;
   const GLboolean swStencil = config->glmode.stencilBits > 0;
   
   int bytesPerPixel = config->glmode.rgbBits / 8;
   int origin = 0;
   int width, height, stride;
   
   surface = (fbSurface *)malloc(sizeof(*surface));
   surf = _eglInitScreenSurface(&surface->Base, drv, dpy, cfg, attrib_list);
   if (surf == EGL_NO_SURFACE) {
      free(surface);
      return EGL_NO_SURFACE;
   }
   width = surface->Base.Width;
   stride = width * bytesPerPixel;
   height = surface->Base.Height;

   surface->mesa_framebuffer = _mesa_create_framebuffer(&config->glmode);
   if (!surface->mesa_framebuffer) {
      free(surface);
      return EGL_NO_SURFACE;
   }
   surface->mesa_framebuffer->Width = width;
   surface->mesa_framebuffer->Height = height;
   {
      driRenderbuffer *drb = driNewRenderbuffer(GL_RGBA, bytesPerPixel, origin, stride);
      fbSetSpanFunctions(drb, &config->glmode);
      drb->Base.Data = display->pFB;
      _mesa_add_renderbuffer(surface->mesa_framebuffer,
                             BUFFER_FRONT_LEFT, &drb->Base);
   }
   if (config->glmode.doubleBufferMode) {
      driRenderbuffer *drb = driNewRenderbuffer(GL_RGBA, bytesPerPixel, origin, stride);
      fbSetSpanFunctions(drb, &config->glmode);
      drb->Base.Data =  _mesa_malloc(stride * height);
      _mesa_add_renderbuffer(surface->mesa_framebuffer,
                             BUFFER_BACK_LEFT, &drb->Base);
   }

   _mesa_add_soft_renderbuffers(surface->mesa_framebuffer,
                                GL_FALSE, /* color */
                                swDepth,
                                swStencil,
                                swAccum,
                                swAlpha,
                                GL_FALSE /* aux */);
   
   return surf;
}


/**
 * Show the given surface on the named screen.
 * If surface is EGL_NO_SURFACE, disable the screen's output.
 */
static EGLBoolean
fbShowSurfaceMESA(_EGLDriver *drv, EGLDisplay dpy, EGLScreenMESA screen,
                    EGLSurface surface, EGLModeMESA m)
{
   FILE *file;
   char buffer[NAME_MAX];
   fbScreen *scrn = Lookup_fbScreen(dpy, screen);
   fbSurface *surf = Lookup_fbSurface(surface);
   _EGLMode *mode = _eglLookupMode(dpy, m);
   
   if (!_eglShowSurfaceMESA(drv, dpy, screen, surface, m))
      return EGL_FALSE;
      
   snprintf(buffer, sizeof(buffer), "%s/%s/mode", sysfs, scrn->fb);
   
   file = fopen(buffer, "r+");
   if (!file) {
err:
      printf("chown all fb sysfs attrib to allow write - %s\n", buffer);
      return EGL_FALSE;
   }
   fputs(mode->Name, file);
   fclose(file);
   
   snprintf(buffer, sizeof(buffer), "%s/%s/bits_per_pixel", sysfs, scrn->fb);
   
   file = fopen(buffer, "r+");
   if (!file)
      goto err;
   snprintf(buffer, sizeof(buffer), "%d", surf->Base.Config->glmode.rgbBits);
   fputs(buffer, file);
   fclose(file);

   return EGL_TRUE;
}


/*
 * Just to silence warning
 */
extern _EGLDriver *
_eglMain(NativeDisplayType dpy);


/**
 * The bootstrap function.  Return a new fbDriver object and
 * plug in API functions.
 */
_EGLDriver *
_eglMain(NativeDisplayType dpy)
{
   fbDriver *fb;

   fb = (fbDriver *) calloc(1, sizeof(fbDriver));
   if (!fb) {
      return NULL;
   }

   /* First fill in the dispatch table with defaults */
   _eglInitDriverFallbacks(&fb->Base);
   
   /* then plug in our fb-specific functions */
   fb->Base.Initialize = fbInitialize;
   fb->Base.Terminate = fbTerminate;
   fb->Base.CreateContext = fbCreateContext;
   fb->Base.MakeCurrent = fbMakeCurrent;
   fb->Base.CreateWindowSurface = fbCreateWindowSurface;
   fb->Base.CreatePixmapSurface = fbCreatePixmapSurface;
   fb->Base.CreatePbufferSurface = fbCreatePbufferSurface;
   fb->Base.DestroySurface = fbDestroySurface;
   fb->Base.DestroyContext = fbDestroyContext;
   fb->Base.QueryString = fbQueryString;
   fb->Base.CreateScreenSurfaceMESA = fbCreateScreenSurfaceMESA;
   fb->Base.ShowSurfaceMESA = fbShowSurfaceMESA;
   
   return &fb->Base;
}
