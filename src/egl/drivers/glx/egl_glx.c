/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * This is an EGL driver that wraps GLX. This gives the benefit of being
 * completely agnostic of the direct rendering implementation.
 *
 * Authors: Alan Hourihane <alanh@tungstengraphics.com>
 */

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

#include "eglconfigutil.h"
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglcurrent.h"
#include "egllog.h"
#include "eglsurface.h"

#define CALLOC_STRUCT(T)   (struct T *) calloc(1, sizeof(struct T))
#define ARRAY_SIZE(a)      (sizeof(a) / sizeof(a[0]))

#ifndef GLX_VERSION_1_4
#error "GL/glx.h must be equal to or greater than GLX 1.4"
#endif

/** subclass of _EGLDriver */
struct GLX_egl_driver
{
   _EGLDriver Base;   /**< base class */
};


/** driver data of _EGLDisplay */
struct GLX_egl_display
{
   Display *dpy;
   XVisualInfo *visuals;
   GLXFBConfig *fbconfigs;

   int glx_maj, glx_min;

   const char *extensions;
   EGLBoolean have_1_3;
   EGLBoolean have_make_current_read;
   EGLBoolean have_fbconfig;
   EGLBoolean have_pbuffer;

   /* GLX_SGIX_pbuffer */
   PFNGLXCREATEGLXPBUFFERSGIXPROC glXCreateGLXPbufferSGIX;
   PFNGLXDESTROYGLXPBUFFERSGIXPROC glXDestroyGLXPbufferSGIX;

   /* workaround quirks of different GLX implementations */
   EGLBoolean single_buffered_quirk;
   EGLBoolean glx_window_quirk;

};


/** subclass of _EGLContext */
struct GLX_egl_context
{
   _EGLContext Base;   /**< base class */

   GLXContext context;
};


/** subclass of _EGLSurface */
struct GLX_egl_surface
{
   _EGLSurface Base;   /**< base class */

   Drawable drawable;
   GLXDrawable glx_drawable;

   void (*destroy)(Display *, GLXDrawable);
};


/** subclass of _EGLConfig */
struct GLX_egl_config
{
   _EGLConfig Base;   /**< base class */
   EGLBoolean double_buffered;
   int index;
};

/* standard typecasts */
_EGL_DRIVER_STANDARD_TYPECASTS(GLX_egl)

static int
GLX_egl_config_index(_EGLConfig *conf)
{
   struct GLX_egl_config *GLX_conf = GLX_egl_config(conf);
   return GLX_conf->index;
}


#define MAP_ATTRIB(attr, memb) \
   { attr, offsetof(__GLcontextModes, memb) }


static const struct {
   int attr;
   int offset;
} fbconfig_attributes[] = {
   /* table 3.1 of GLX 1.4 */
   MAP_ATTRIB(GLX_FBCONFIG_ID,                  fbconfigID),
   MAP_ATTRIB(GLX_BUFFER_SIZE,                  rgbBits),
   MAP_ATTRIB(GLX_LEVEL,                        level),
   MAP_ATTRIB(GLX_DOUBLEBUFFER,                 doubleBufferMode),
   MAP_ATTRIB(GLX_STEREO,                       stereoMode),
   MAP_ATTRIB(GLX_AUX_BUFFERS,                  numAuxBuffers),
   MAP_ATTRIB(GLX_RED_SIZE,                     redBits),
   MAP_ATTRIB(GLX_GREEN_SIZE,                   greenBits),
   MAP_ATTRIB(GLX_BLUE_SIZE,                    blueBits),
   MAP_ATTRIB(GLX_ALPHA_SIZE,                   alphaBits),
   MAP_ATTRIB(GLX_DEPTH_SIZE,                   depthBits),
   MAP_ATTRIB(GLX_STENCIL_SIZE,                 stencilBits),
   MAP_ATTRIB(GLX_ACCUM_RED_SIZE,               accumRedBits),
   MAP_ATTRIB(GLX_ACCUM_GREEN_SIZE,             accumGreenBits),
   MAP_ATTRIB(GLX_ACCUM_BLUE_SIZE,              accumBlueBits),
   MAP_ATTRIB(GLX_ACCUM_ALPHA_SIZE,             accumAlphaBits),
   MAP_ATTRIB(GLX_SAMPLE_BUFFERS,               sampleBuffers),
   MAP_ATTRIB(GLX_SAMPLES,                      samples),
   MAP_ATTRIB(GLX_RENDER_TYPE,                  renderType),
   MAP_ATTRIB(GLX_DRAWABLE_TYPE,                drawableType),
   MAP_ATTRIB(GLX_X_RENDERABLE,                 xRenderable),
   MAP_ATTRIB(GLX_X_VISUAL_TYPE,                visualType),
   MAP_ATTRIB(GLX_CONFIG_CAVEAT,                visualRating),
   MAP_ATTRIB(GLX_TRANSPARENT_TYPE,             transparentPixel),
   MAP_ATTRIB(GLX_TRANSPARENT_INDEX_VALUE,      transparentIndex),
   MAP_ATTRIB(GLX_TRANSPARENT_RED_VALUE,        transparentRed),
   MAP_ATTRIB(GLX_TRANSPARENT_GREEN_VALUE,      transparentGreen),
   MAP_ATTRIB(GLX_TRANSPARENT_BLUE_VALUE,       transparentBlue),
   MAP_ATTRIB(GLX_TRANSPARENT_ALPHA_VALUE,      transparentAlpha),
   MAP_ATTRIB(GLX_MAX_PBUFFER_WIDTH,            maxPbufferWidth),
   MAP_ATTRIB(GLX_MAX_PBUFFER_HEIGHT,           maxPbufferHeight),
   MAP_ATTRIB(GLX_MAX_PBUFFER_PIXELS,           maxPbufferPixels),
   MAP_ATTRIB(GLX_VISUAL_ID,                    visualID),
};


static EGLBoolean
convert_fbconfig(Display *dpy, GLXFBConfig fbconfig,
                 struct GLX_egl_config *GLX_conf)
{
   __GLcontextModes mode;
   int err = 0, attr, val, i;

   memset(&mode, 0, sizeof(mode));

   for (i = 0; i < ARRAY_SIZE(fbconfig_attributes); i++) {
      int offset = fbconfig_attributes[i].offset;
      attr = fbconfig_attributes[i].attr;
      err = glXGetFBConfigAttrib(dpy, fbconfig, attr, &val);
      if (err) {
         if (err == GLX_BAD_ATTRIBUTE) {
            err = 0;
            continue;
         }
         break;
      }
      *((int *) ((char *) &mode + offset)) = val;
   }
   if (err)
      return EGL_FALSE;

   /* must have rgba bit */
   if (!(mode.renderType & GLX_RGBA_BIT))
      return EGL_FALSE;

   /* pixmap and pbuffer surfaces must be single-buffered in EGL */
   if (mode.doubleBufferMode) {
      mode.drawableType &= ~(GLX_PIXMAP_BIT | GLX_PBUFFER_BIT);
      if (!mode.drawableType)
         return EGL_FALSE;
   }

   mode.rgbMode = GL_TRUE;
   mode.haveAccumBuffer = (mode.accumRedBits +
                           mode.accumGreenBits +
                           mode.accumBlueBits +
                           mode.accumAlphaBits > 0);
   mode.haveDepthBuffer = (mode.depthBits > 0);
   mode.haveStencilBuffer = (mode.stencilBits > 0);

   GLX_conf->double_buffered = (mode.doubleBufferMode != 0);
   return _eglConfigFromContextModesRec(&GLX_conf->Base, &mode,
                                        EGL_OPENGL_BIT, EGL_OPENGL_BIT);
}


static const struct {
   int attr;
   int offset;
} visual_attributes[] = {
   /* table 3.7 of GLX 1.4 */
   /* no GLX_USE_GL */
   MAP_ATTRIB(GLX_BUFFER_SIZE,         rgbBits),
   MAP_ATTRIB(GLX_LEVEL,               level),
   MAP_ATTRIB(GLX_RGBA,                rgbMode),
   MAP_ATTRIB(GLX_DOUBLEBUFFER,        doubleBufferMode),
   MAP_ATTRIB(GLX_STEREO,              stereoMode),
   MAP_ATTRIB(GLX_AUX_BUFFERS,         numAuxBuffers),
   MAP_ATTRIB(GLX_RED_SIZE,            redBits),
   MAP_ATTRIB(GLX_GREEN_SIZE,          greenBits),
   MAP_ATTRIB(GLX_BLUE_SIZE,           blueBits),
   MAP_ATTRIB(GLX_ALPHA_SIZE,          alphaBits),
   MAP_ATTRIB(GLX_DEPTH_SIZE,          depthBits),
   MAP_ATTRIB(GLX_STENCIL_SIZE,        stencilBits),
   MAP_ATTRIB(GLX_ACCUM_RED_SIZE,      accumRedBits),
   MAP_ATTRIB(GLX_ACCUM_GREEN_SIZE,    accumGreenBits),
   MAP_ATTRIB(GLX_ACCUM_BLUE_SIZE,     accumBlueBits),
   MAP_ATTRIB(GLX_ACCUM_ALPHA_SIZE,    accumAlphaBits),
   MAP_ATTRIB(GLX_SAMPLE_BUFFERS,      sampleBuffers),
   MAP_ATTRIB(GLX_SAMPLES,             samples),
   MAP_ATTRIB(GLX_FBCONFIG_ID,         fbconfigID),
   /* GLX_EXT_visual_rating */
   MAP_ATTRIB(GLX_VISUAL_CAVEAT_EXT,   visualRating),
};


static int
get_visual_type(const XVisualInfo *vis)
{
   int klass;

#if defined(__cplusplus) || defined(c_plusplus)
   klass = vis->c_class;
#else
   klass = vis->class;
#endif

   switch (klass) {
   case TrueColor:
      return GLX_TRUE_COLOR;
   case DirectColor:
      return GLX_DIRECT_COLOR;
   case PseudoColor:
      return GLX_PSEUDO_COLOR;
   case StaticColor:
      return GLX_STATIC_COLOR;
   case GrayScale:
      return GLX_GRAY_SCALE;
   case StaticGray:
      return GLX_STATIC_GRAY;
   default:
      return GLX_NONE;
   }
}


static EGLBoolean
convert_visual(Display *dpy, XVisualInfo *vinfo,
               struct GLX_egl_config *GLX_conf)
{
   __GLcontextModes mode;
   int err, attr, val, i;

   /* the visual must support OpenGL */
   err = glXGetConfig(dpy, vinfo, GLX_USE_GL, &val);
   if (err || !val)
      return EGL_FALSE;

   memset(&mode, 0, sizeof(mode));

   for (i = 0; i < ARRAY_SIZE(visual_attributes); i++) {
      int offset = visual_attributes[i].offset;
      attr = visual_attributes[i].attr;
      err = glXGetConfig(dpy, vinfo, attr, &val);
      if (err) {
         if (err == GLX_BAD_ATTRIBUTE) {
            err = 0;
            continue;
         }
         break;
      }
      *((int *) ((char *) &mode + offset)) = val;
   }
   if (err)
      return EGL_FALSE;

   /* must be RGB mode */
   if (!mode.rgbMode)
      return EGL_FALSE;

   mode.visualID = vinfo->visualid;
   mode.visualType = get_visual_type(vinfo);
   mode.redMask = vinfo->red_mask;
   mode.greenMask = vinfo->green_mask;
   mode.blueMask = vinfo->blue_mask;

   mode.drawableType = GLX_WINDOW_BIT;
   /* pixmap surfaces must be single-buffered in EGL */
   if (!mode.doubleBufferMode)
      mode.drawableType |= GLX_PIXMAP_BIT;

   mode.renderType = GLX_RGBA_BIT;
   mode.xRenderable = GL_TRUE;
   mode.haveAccumBuffer = (mode.accumRedBits +
                           mode.accumGreenBits +
                           mode.accumBlueBits +
                           mode.accumAlphaBits > 0);
   mode.haveDepthBuffer = (mode.depthBits > 0);
   mode.haveStencilBuffer = (mode.stencilBits > 0);

   GLX_conf->double_buffered = (mode.doubleBufferMode != 0);
   return _eglConfigFromContextModesRec(&GLX_conf->Base, &mode,
                                        EGL_OPENGL_BIT, EGL_OPENGL_BIT);
}


static void
fix_config(struct GLX_egl_display *GLX_dpy, struct GLX_egl_config *GLX_conf)
{
   _EGLConfig *conf = &GLX_conf->Base;
   EGLint surface_type, r, g, b, a;

   surface_type = GET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE);
   if (!GLX_conf->double_buffered && GLX_dpy->single_buffered_quirk) {
      /* some GLX impls do not like single-buffered window surface */
      surface_type &= ~EGL_WINDOW_BIT;
      /* pbuffer bit is usually not set */
      if (GLX_dpy->have_pbuffer)
         surface_type |= EGL_PBUFFER_BIT;
      SET_CONFIG_ATTRIB(conf, EGL_SURFACE_TYPE, surface_type);
   }

   /* no visual attribs unless window bit is set */
   if (!(surface_type & EGL_WINDOW_BIT)) {
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_ID, 0);
      SET_CONFIG_ATTRIB(conf, EGL_NATIVE_VISUAL_TYPE, EGL_NONE);
   }

   /* make sure buffer size is set correctly */
   r = GET_CONFIG_ATTRIB(conf, EGL_RED_SIZE);
   g = GET_CONFIG_ATTRIB(conf, EGL_GREEN_SIZE);
   b = GET_CONFIG_ATTRIB(conf, EGL_BLUE_SIZE);
   a = GET_CONFIG_ATTRIB(conf, EGL_ALPHA_SIZE);
   SET_CONFIG_ATTRIB(conf, EGL_BUFFER_SIZE, r + g + b + a);
}


static EGLBoolean
create_configs(_EGLDisplay *dpy, struct GLX_egl_display *GLX_dpy,
               EGLint screen)
{
   EGLint num_configs = 0, i;
   EGLint id = 1;

   if (GLX_dpy->have_fbconfig) {
      GLX_dpy->fbconfigs = glXGetFBConfigs(GLX_dpy->dpy, screen, &num_configs);
   }
   else {
      XVisualInfo vinfo_template;
      long mask;

      vinfo_template.screen = screen;
      mask = VisualScreenMask;
      GLX_dpy->visuals = XGetVisualInfo(GLX_dpy->dpy, mask, &vinfo_template,
                                        &num_configs);
   }

   if (!num_configs)
      return EGL_FALSE;

   for (i = 0; i < num_configs; i++) {
      struct GLX_egl_config *GLX_conf, template;
      EGLBoolean ok;

      memset(&template, 0, sizeof(template));
      _eglInitConfig(&template.Base, dpy, id);
      if (GLX_dpy->have_fbconfig)
         ok = convert_fbconfig(GLX_dpy->dpy, GLX_dpy->fbconfigs[i], &template);
      else
         ok = convert_visual(GLX_dpy->dpy, &GLX_dpy->visuals[i], &template);
      if (!ok)
        continue;

      fix_config(GLX_dpy, &template);
      if (!_eglValidateConfig(&template.Base, EGL_FALSE)) {
         _eglLog(_EGL_DEBUG, "GLX: failed to validate config %d", i);
         continue;
      }

      GLX_conf = CALLOC_STRUCT(GLX_egl_config);
      if (GLX_conf) {
         memcpy(GLX_conf, &template, sizeof(template));
         GLX_conf->index = i;

         _eglAddConfig(dpy, &GLX_conf->Base);
         id++;
      }
   }

   return EGL_TRUE;
}


static void
check_extensions(struct GLX_egl_display *GLX_dpy, EGLint screen)
{
   GLX_dpy->extensions =
      glXQueryExtensionsString(GLX_dpy->dpy, screen);
   if (GLX_dpy->extensions) {
      /* glXGetProcAddress is assumed */

      if (strstr(GLX_dpy->extensions, "GLX_SGI_make_current_read")) {
         /* GLX 1.3 entry points are used */
         GLX_dpy->have_make_current_read = EGL_TRUE;
      }

      if (strstr(GLX_dpy->extensions, "GLX_SGIX_fbconfig")) {
         /* GLX 1.3 entry points are used */
         GLX_dpy->have_fbconfig = EGL_TRUE;
      }

      if (strstr(GLX_dpy->extensions, "GLX_SGIX_pbuffer")) {
         GLX_dpy->glXCreateGLXPbufferSGIX = (PFNGLXCREATEGLXPBUFFERSGIXPROC)
            glXGetProcAddress((const GLubyte *) "glXCreateGLXPbufferSGIX");
         GLX_dpy->glXDestroyGLXPbufferSGIX = (PFNGLXDESTROYGLXPBUFFERSGIXPROC)
            glXGetProcAddress((const GLubyte *) "glXDestroyGLXPbufferSGIX");

         if (GLX_dpy->glXCreateGLXPbufferSGIX &&
             GLX_dpy->glXDestroyGLXPbufferSGIX &&
             GLX_dpy->have_fbconfig)
            GLX_dpy->have_pbuffer = EGL_TRUE;
      }
   }

   if (GLX_dpy->glx_maj == 1 && GLX_dpy->glx_min >= 3) {
      GLX_dpy->have_1_3 = EGL_TRUE;
      GLX_dpy->have_make_current_read = EGL_TRUE;
      GLX_dpy->have_fbconfig = EGL_TRUE;
      GLX_dpy->have_pbuffer = EGL_TRUE;
   }
}


static void
check_quirks(struct GLX_egl_display *GLX_dpy, EGLint screen)
{
   const char *vendor;

   GLX_dpy->single_buffered_quirk = EGL_TRUE;
   GLX_dpy->glx_window_quirk = EGL_TRUE;

   vendor = glXGetClientString(GLX_dpy->dpy, GLX_VENDOR);
   if (vendor && strstr(vendor, "NVIDIA")) {
      vendor = glXQueryServerString(GLX_dpy->dpy, screen, GLX_VENDOR);
      if (vendor && strstr(vendor, "NVIDIA")) {
         _eglLog(_EGL_DEBUG, "disable quirks");
         GLX_dpy->single_buffered_quirk = EGL_FALSE;
         GLX_dpy->glx_window_quirk = EGL_FALSE;
      }
   }
}


/**
 * Called via eglInitialize(), GLX_drv->API.Initialize().
 */
static EGLBoolean
GLX_eglInitialize(_EGLDriver *drv, _EGLDisplay *disp,
                   EGLint *major, EGLint *minor)
{
   struct GLX_egl_display *GLX_dpy;

   GLX_dpy = CALLOC_STRUCT(GLX_egl_display);
   if (!GLX_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");

   GLX_dpy->dpy = (Display *) disp->NativeDisplay;
   if (!GLX_dpy->dpy) {
      GLX_dpy->dpy = XOpenDisplay(NULL);
      if (!GLX_dpy->dpy) {
         _eglLog(_EGL_WARNING, "GLX: XOpenDisplay failed");
         free(GLX_dpy);
         return EGL_FALSE;
      }
   }

   if (!glXQueryVersion(GLX_dpy->dpy, &GLX_dpy->glx_maj, &GLX_dpy->glx_min)) {
      _eglLog(_EGL_WARNING, "GLX: glXQueryVersion failed");
      if (!disp->NativeDisplay)
         XCloseDisplay(GLX_dpy->dpy);
      free(GLX_dpy);
      return EGL_FALSE;
   }

   check_extensions(GLX_dpy, DefaultScreen(GLX_dpy->dpy));
   check_quirks(GLX_dpy, DefaultScreen(GLX_dpy->dpy));

   create_configs(disp, GLX_dpy, DefaultScreen(GLX_dpy->dpy));
   if (!disp->NumConfigs) {
      _eglLog(_EGL_WARNING, "GLX: failed to create any config");
      if (!disp->NativeDisplay)
         XCloseDisplay(GLX_dpy->dpy);
      free(GLX_dpy);
      return EGL_FALSE;
   }

   disp->DriverData = (void *) GLX_dpy;
   disp->ClientAPIsMask = EGL_OPENGL_BIT;

   /* we're supporting EGL 1.4 */
   *major = 1;
   *minor = 4;

   return EGL_TRUE;
}

/**
 * Called via eglTerminate(), drv->API.Terminate().
 */
static EGLBoolean
GLX_eglTerminate(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);

   _eglReleaseDisplayResources(drv, disp);
   _eglCleanupDisplay(disp);

   if (GLX_dpy->visuals)
      XFree(GLX_dpy->visuals);
   if (GLX_dpy->fbconfigs)
      XFree(GLX_dpy->fbconfigs);

   if (!disp->NativeDisplay)
      XCloseDisplay(GLX_dpy->dpy);
   free(GLX_dpy);

   disp->DriverData = NULL;

   return EGL_TRUE;
}


/**
 * Called via eglCreateContext(), drv->API.CreateContext().
 */
static _EGLContext *
GLX_eglCreateContext(_EGLDriver *drv, _EGLDisplay *disp, _EGLConfig *conf,
                      _EGLContext *share_list, const EGLint *attrib_list)
{
   struct GLX_egl_context *GLX_ctx = CALLOC_STRUCT(GLX_egl_context);
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_context *GLX_ctx_shared = GLX_egl_context(share_list);

   if (!GLX_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
      return NULL;
   }

   if (!_eglInitContext(&GLX_ctx->Base, disp, conf, attrib_list)) {
      free(GLX_ctx);
      return NULL;
   }

   if (GLX_dpy->have_fbconfig)
      GLX_ctx->context =
         glXCreateNewContext(GLX_dpy->dpy,
                             GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
                             GLX_RGBA_TYPE,
                             GLX_ctx_shared ? GLX_ctx_shared->context : NULL,
                             GL_TRUE);
   else
      GLX_ctx->context =
         glXCreateContext(GLX_dpy->dpy,
                          &GLX_dpy->visuals[GLX_egl_config_index(conf)],
                          GLX_ctx_shared ? GLX_ctx_shared->context : NULL,
                          GL_TRUE);
   if (!GLX_ctx->context) {
      free(GLX_ctx);
      return NULL;
   }

   return &GLX_ctx->Base;
}


/**
 * Destroy a surface.  The display is allowed to be uninitialized.
 */
static void
destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
{
   struct GLX_egl_surface *GLX_surf = GLX_egl_surface(surf);

   if (GLX_surf->destroy)
      GLX_surf->destroy(disp->NativeDisplay, GLX_surf->glx_drawable);

   free(GLX_surf);
}


/**
 * Called via eglMakeCurrent(), drv->API.MakeCurrent().
 */
static EGLBoolean
GLX_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *dsurf,
                   _EGLSurface *rsurf, _EGLContext *ctx)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_dsurf = GLX_egl_surface(dsurf);
   struct GLX_egl_surface *GLX_rsurf = GLX_egl_surface(rsurf);
   struct GLX_egl_context *GLX_ctx = GLX_egl_context(ctx);
   GLXDrawable ddraw, rdraw;
   GLXContext cctx;
   EGLBoolean ret = EGL_FALSE;

   /* bind the new context and return the "orphaned" one */
   if (!_eglBindContext(&ctx, &dsurf, &rsurf))
      return EGL_FALSE;

   ddraw = (GLX_dsurf) ? GLX_dsurf->glx_drawable : None;
   rdraw = (GLX_rsurf) ? GLX_rsurf->glx_drawable : None;
   cctx = (GLX_ctx) ? GLX_ctx->context : NULL;

   if (GLX_dpy->have_make_current_read)
      ret = glXMakeContextCurrent(GLX_dpy->dpy, ddraw, rdraw, cctx);
   else if (ddraw == rdraw)
      ret = glXMakeCurrent(GLX_dpy->dpy, ddraw, cctx);

   if (ret) {
      if (dsurf && !_eglIsSurfaceLinked(dsurf))
         destroy_surface(disp, dsurf);
      if (rsurf && rsurf != dsurf && !_eglIsSurfaceLinked(rsurf))
         destroy_surface(disp, rsurf);
   }
   else {
      _eglBindContext(&ctx, &dsurf, &rsurf);
   }

   return ret;
}

/** Get size of given window */
static Status
get_drawable_size(Display *dpy, Drawable d, uint *width, uint *height)
{
   Window root;
   Status stat;
   int xpos, ypos;
   unsigned int w, h, bw, depth;
   stat = XGetGeometry(dpy, d, &root, &xpos, &ypos, &w, &h, &bw, &depth);
   *width = w;
   *height = h;
   return stat;
}

/**
 * Called via eglCreateWindowSurface(), drv->API.CreateWindowSurface().
 */
static _EGLSurface *
GLX_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *disp,
                           _EGLConfig *conf, EGLNativeWindowType window,
                           const EGLint *attrib_list)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf;
   uint width, height;

   GLX_surf = CALLOC_STRUCT(GLX_egl_surface);
   if (!GLX_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreateWindowSurface");
      return NULL;
   }

   if (!_eglInitSurface(&GLX_surf->Base, disp, EGL_WINDOW_BIT,
                        conf, attrib_list)) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->drawable = window;

   if (GLX_dpy->have_1_3 && !GLX_dpy->glx_window_quirk)
      GLX_surf->glx_drawable =
         glXCreateWindow(GLX_dpy->dpy,
                         GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
                         GLX_surf->drawable, NULL);
   else
      GLX_surf->glx_drawable = GLX_surf->drawable;

   if (!GLX_surf->glx_drawable) {
      free(GLX_surf);
      return NULL;
   }

   if (GLX_dpy->have_1_3 && !GLX_dpy->glx_window_quirk)
      GLX_surf->destroy = glXDestroyWindow;

   get_drawable_size(GLX_dpy->dpy, window, &width, &height);
   GLX_surf->Base.Width = width;
   GLX_surf->Base.Height = height;

   return &GLX_surf->Base;
}

static _EGLSurface *
GLX_eglCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *disp,
                           _EGLConfig *conf, EGLNativePixmapType pixmap,
                           const EGLint *attrib_list)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf;
   uint width, height;

   GLX_surf = CALLOC_STRUCT(GLX_egl_surface);
   if (!GLX_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePixmapSurface");
      return NULL;
   }

   if (!_eglInitSurface(&GLX_surf->Base, disp, EGL_PIXMAP_BIT,
                        conf, attrib_list)) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->drawable = pixmap;

   if (GLX_dpy->have_1_3) {
      GLX_surf->glx_drawable =
         glXCreatePixmap(GLX_dpy->dpy,
                         GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
                         GLX_surf->drawable, NULL);
   }
   else if (GLX_dpy->have_fbconfig) {
      GLXFBConfig fbconfig = GLX_dpy->fbconfigs[GLX_egl_config_index(conf)];
      XVisualInfo *vinfo = glXGetVisualFromFBConfig(GLX_dpy->dpy, fbconfig);
      if (vinfo) {
         GLX_surf->glx_drawable =
            glXCreateGLXPixmap(GLX_dpy->dpy, vinfo, GLX_surf->drawable);
         XFree(vinfo);
      }
   }
   else {
      GLX_surf->glx_drawable =
         glXCreateGLXPixmap(GLX_dpy->dpy,
                            &GLX_dpy->visuals[GLX_egl_config_index(conf)],
                            GLX_surf->drawable);
   }

   if (!GLX_surf->glx_drawable) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->destroy = (GLX_dpy->have_1_3) ?
      glXDestroyPixmap : glXDestroyGLXPixmap;

   get_drawable_size(GLX_dpy->dpy, pixmap, &width, &height);
   GLX_surf->Base.Width = width;
   GLX_surf->Base.Height = height;

   return &GLX_surf->Base;
}

static _EGLSurface *
GLX_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *disp,
                            _EGLConfig *conf, const EGLint *attrib_list)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf;
   int attribs[5];
   int i;

   GLX_surf = CALLOC_STRUCT(GLX_egl_surface);
   if (!GLX_surf) {
      _eglError(EGL_BAD_ALLOC, "eglCreatePbufferSurface");
      return NULL;
   }

   if (!_eglInitSurface(&GLX_surf->Base, disp, EGL_PBUFFER_BIT,
                        conf, attrib_list)) {
      free(GLX_surf);
      return NULL;
   }

   i = 0;
   attribs[i] = None;

   GLX_surf->drawable = None;

   if (GLX_dpy->have_1_3) {
      /* put geometry in attribs */
      if (GLX_surf->Base.Width) {
         attribs[i++] = GLX_PBUFFER_WIDTH;
         attribs[i++] = GLX_surf->Base.Width;
      }
      if (GLX_surf->Base.Height) {
         attribs[i++] = GLX_PBUFFER_HEIGHT;
         attribs[i++] = GLX_surf->Base.Height;
      }
      attribs[i] = None;

      GLX_surf->glx_drawable =
         glXCreatePbuffer(GLX_dpy->dpy,
                          GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
                          attribs);
   }
   else if (GLX_dpy->have_pbuffer) {
      GLX_surf->glx_drawable = GLX_dpy->glXCreateGLXPbufferSGIX(
            GLX_dpy->dpy,
            GLX_dpy->fbconfigs[GLX_egl_config_index(conf)],
            GLX_surf->Base.Width,
            GLX_surf->Base.Height,
            attribs);
   }

   if (!GLX_surf->glx_drawable) {
      free(GLX_surf);
      return NULL;
   }

   GLX_surf->destroy = (GLX_dpy->have_1_3) ?
      glXDestroyPbuffer : GLX_dpy->glXDestroyGLXPbufferSGIX;

   return &GLX_surf->Base;
}


static EGLBoolean
GLX_eglDestroySurface(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *surf)
{
   if (!_eglIsSurfaceBound(surf))
      destroy_surface(disp, surf);

   return EGL_TRUE;
}


static EGLBoolean
GLX_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *disp, _EGLSurface *draw)
{
   struct GLX_egl_display *GLX_dpy = GLX_egl_display(disp);
   struct GLX_egl_surface *GLX_surf = GLX_egl_surface(draw);

   glXSwapBuffers(GLX_dpy->dpy, GLX_surf->glx_drawable);

   return EGL_TRUE;
}

/*
 * Called from eglGetProcAddress() via drv->API.GetProcAddress().
 */
static _EGLProc
GLX_eglGetProcAddress(_EGLDriver *drv, const char *procname)
{
   return (_EGLProc) glXGetProcAddress((const GLubyte *) procname);
}

static EGLBoolean
GLX_eglWaitClient(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   glXWaitGL();
   return EGL_TRUE;
}

static EGLBoolean
GLX_eglWaitNative(_EGLDriver *drv, _EGLDisplay *dpy, EGLint engine)
{
   if (engine != EGL_CORE_NATIVE_ENGINE)
      return _eglError(EGL_BAD_PARAMETER, "eglWaitNative");
   glXWaitX();
   return EGL_TRUE;
}

static void
GLX_Unload(_EGLDriver *drv)
{
   struct GLX_egl_driver *GLX_drv = GLX_egl_driver(drv);
   free(GLX_drv);
}


/**
 * This is the main entrypoint into the driver, called by libEGL.
 * Create a new _EGLDriver object and init its dispatch table.
 */
_EGLDriver *
_eglMain(const char *args)
{
   struct GLX_egl_driver *GLX_drv = CALLOC_STRUCT(GLX_egl_driver);

   if (!GLX_drv)
      return NULL;

   _eglInitDriverFallbacks(&GLX_drv->Base);
   GLX_drv->Base.API.Initialize = GLX_eglInitialize;
   GLX_drv->Base.API.Terminate = GLX_eglTerminate;
   GLX_drv->Base.API.CreateContext = GLX_eglCreateContext;
   GLX_drv->Base.API.MakeCurrent = GLX_eglMakeCurrent;
   GLX_drv->Base.API.CreateWindowSurface = GLX_eglCreateWindowSurface;
   GLX_drv->Base.API.CreatePixmapSurface = GLX_eglCreatePixmapSurface;
   GLX_drv->Base.API.CreatePbufferSurface = GLX_eglCreatePbufferSurface;
   GLX_drv->Base.API.DestroySurface = GLX_eglDestroySurface;
   GLX_drv->Base.API.SwapBuffers = GLX_eglSwapBuffers;
   GLX_drv->Base.API.GetProcAddress = GLX_eglGetProcAddress;
   GLX_drv->Base.API.WaitClient = GLX_eglWaitClient;
   GLX_drv->Base.API.WaitNative = GLX_eglWaitNative;

   GLX_drv->Base.Name = "GLX";
   GLX_drv->Base.Unload = GLX_Unload;

   return &GLX_drv->Base;
}
