/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

/**
 * \file glxext.c
 * GLX protocol interface boot-strap code.
 *
 * Direct rendering support added by Precision Insight, Inc.
 *
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 */

#include <assert.h>
#include "glxclient.h"
#include <X11/extensions/Xext.h>
#include <X11/extensions/extutil.h>
#include "glapi.h"
#include "glxextensions.h"
#include "glcontextmodes.h"

#ifdef USE_XCB
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/glx.h>
#endif


#ifdef DEBUG
void __glXDumpDrawBuffer(__GLXcontext * ctx);
#endif

#ifdef USE_SPARC_ASM
static void _glx_mesa_init_sparc_glapi_relocs(void);
static int _mesa_sparc_needs_init = 1;
#define INIT_MESA_SPARC do {               \
   if (_mesa_sparc_needs_init) {           \
      _glx_mesa_init_sparc_glapi_relocs(); \
      _mesa_sparc_needs_init = 0;          \
   }                                       \
  } while(0)
#else
#define INIT_MESA_SPARC do { } while(0)
#endif

/*
** You can set this cell to 1 to force the gl drawing stuff to be
** one command per packet
*/
_X_HIDDEN int __glXDebug = 0;

/* Extension required boiler plate */

static char *__glXExtensionName = GLX_EXTENSION_NAME;
XExtensionInfo *__glXExtensionInfo = NULL;

static /* const */ char *error_list[] = {
   "GLXBadContext",
   "GLXBadContextState",
   "GLXBadDrawable",
   "GLXBadPixmap",
   "GLXBadContextTag",
   "GLXBadCurrentWindow",
   "GLXBadRenderRequest",
   "GLXBadLargeRequest",
   "GLXUnsupportedPrivateRequest",
   "GLXBadFBConfig",
   "GLXBadPbuffer",
   "GLXBadCurrentDrawable",
   "GLXBadWindow",
};

static int
__glXCloseDisplay(Display * dpy, XExtCodes * codes)
{
   GLXContext gc;

   gc = __glXGetCurrentContext();
   if (dpy == gc->currentDpy) {
      __glXSetCurrentContextNull();
      __glXFreeContext(gc);
   }

   return XextRemoveDisplay(__glXExtensionInfo, dpy);
}


static
XEXT_GENERATE_ERROR_STRING(__glXErrorString, __glXExtensionName,
                           __GLX_NUMBER_ERRORS, error_list)

     static /* const */ XExtensionHooks __glXExtensionHooks = {
        NULL,                   /* create_gc */
        NULL,                   /* copy_gc */
        NULL,                   /* flush_gc */
        NULL,                   /* free_gc */
        NULL,                   /* create_font */
        NULL,                   /* free_font */
        __glXCloseDisplay,      /* close_display */
        NULL,                   /* wire_to_event */
        NULL,                   /* event_to_wire */
        NULL,                   /* error */
        __glXErrorString,       /* error_string */
     };

static
XEXT_GENERATE_FIND_DISPLAY(__glXFindDisplay, __glXExtensionInfo,
                           __glXExtensionName, &__glXExtensionHooks,
                           __GLX_NUMBER_EVENTS, NULL)

/************************************************************************/
/*
** Free the per screen configs data as well as the array of
** __glXScreenConfigs.
*/
static void
FreeScreenConfigs(__GLXdisplayPrivate * priv)
{
   __GLXscreenConfigs *psc;
   GLint i, screens;

   /* Free screen configuration information */
   psc = priv->screenConfigs;
   screens = ScreenCount(priv->dpy);
   for (i = 0; i < screens; i++, psc++) {
      if (psc->configs) {
         _gl_context_modes_destroy(psc->configs);
         if (psc->effectiveGLXexts)
            Xfree(psc->effectiveGLXexts);
         psc->configs = NULL;   /* NOTE: just for paranoia */
      }
      if (psc->visuals) {
         _gl_context_modes_destroy(psc->visuals);
         psc->visuals = NULL;   /* NOTE: just for paranoia */
      }
      Xfree((char *) psc->serverGLXexts);

#ifdef GLX_DIRECT_RENDERING
      if (psc->driScreen) {
         psc->driScreen->destroyScreen(psc);
         __glxHashDestroy(psc->drawHash);
         XFree(psc->driScreen);
         psc->driScreen = NULL;
      }
#endif
   }
   XFree((char *) priv->screenConfigs);
   priv->screenConfigs = NULL;
}

/*
** Release the private memory referred to in a display private
** structure.  The caller will free the extension structure.
*/
static int
__glXFreeDisplayPrivate(XExtData * extension)
{
   __GLXdisplayPrivate *priv;

   priv = (__GLXdisplayPrivate *) extension->private_data;
   FreeScreenConfigs(priv);
   if (priv->serverGLXvendor) {
      Xfree((char *) priv->serverGLXvendor);
      priv->serverGLXvendor = 0x0;      /* to protect against double free's */
   }
   if (priv->serverGLXversion) {
      Xfree((char *) priv->serverGLXversion);
      priv->serverGLXversion = 0x0;     /* to protect against double free's */
   }

#ifdef GLX_DIRECT_RENDERING
   /* Free the direct rendering per display data */
   if (priv->driswDisplay)
      (*priv->driswDisplay->destroyDisplay) (priv->driswDisplay);
   priv->driswDisplay = NULL;

   if (priv->driDisplay)
      (*priv->driDisplay->destroyDisplay) (priv->driDisplay);
   priv->driDisplay = NULL;

   if (priv->dri2Display)
      (*priv->dri2Display->destroyDisplay) (priv->dri2Display);
   priv->dri2Display = NULL;
#endif

   Xfree((char *) priv);
   return 0;
}

/************************************************************************/

/*
** Query the version of the GLX extension.  This procedure works even if
** the client extension is not completely set up.
*/
static Bool
QueryVersion(Display * dpy, int opcode, int *major, int *minor)
{
#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_query_version_reply_t* reply =
      xcb_glx_query_version_reply(c,
                                  xcb_glx_query_version(c,
                                                        GLX_MAJOR_VERSION,
                                                        GLX_MINOR_VERSION),
                                  NULL);

   if(reply->major_version != GLX_MAJOR_VERSION)
   {
      free(reply);
      return GL_FALSE;
   }
   *major = reply->major_version;
   *minor = min(reply->minor_version, GLX_MINOR_VERSION);
   free(reply);
   return GL_TRUE;
#else
   xGLXQueryVersionReq *req;
   xGLXQueryVersionReply reply;

   /* Send the glXQueryVersion request */
   LockDisplay(dpy);
   GetReq(GLXQueryVersion, req);
   req->reqType = opcode;
   req->glxCode = X_GLXQueryVersion;
   req->majorVersion = GLX_MAJOR_VERSION;
   req->minorVersion = GLX_MINOR_VERSION;
   _XReply(dpy, (xReply *) & reply, 0, False);
   UnlockDisplay(dpy);
   SyncHandle();

   if (reply.majorVersion != GLX_MAJOR_VERSION) {
      /*
       ** The server does not support the same major release as this
       ** client.
       */
      return GL_FALSE;
   }
   *major = reply.majorVersion;
   *minor = min(reply.minorVersion, GLX_MINOR_VERSION);
   return GL_TRUE;
#endif /* USE_XCB */
}


_X_HIDDEN void
__glXInitializeVisualConfigFromTags(__GLcontextModes * config, int count,
                                    const INT32 * bp, Bool tagged_only,
                                    Bool fbconfig_style_tags)
{
   int i;

   if (!tagged_only) {
      /* Copy in the first set of properties */
      config->visualID = *bp++;

      config->visualType = _gl_convert_from_x_visual_type(*bp++);

      config->rgbMode = *bp++;

      config->redBits = *bp++;
      config->greenBits = *bp++;
      config->blueBits = *bp++;
      config->alphaBits = *bp++;
      config->accumRedBits = *bp++;
      config->accumGreenBits = *bp++;
      config->accumBlueBits = *bp++;
      config->accumAlphaBits = *bp++;

      config->doubleBufferMode = *bp++;
      config->stereoMode = *bp++;

      config->rgbBits = *bp++;
      config->depthBits = *bp++;
      config->stencilBits = *bp++;
      config->numAuxBuffers = *bp++;
      config->level = *bp++;

      count -= __GLX_MIN_CONFIG_PROPS;
   }

   /*
    ** Additional properties may be in a list at the end
    ** of the reply.  They are in pairs of property type
    ** and property value.
    */

#define FETCH_OR_SET(tag) \
    config-> tag = ( fbconfig_style_tags ) ? *bp++ : 1

   for (i = 0; i < count; i += 2) {
      switch (*bp++) {
      case GLX_RGBA:
         FETCH_OR_SET(rgbMode);
         break;
      case GLX_BUFFER_SIZE:
         config->rgbBits = *bp++;
         break;
      case GLX_LEVEL:
         config->level = *bp++;
         break;
      case GLX_DOUBLEBUFFER:
         FETCH_OR_SET(doubleBufferMode);
         break;
      case GLX_STEREO:
         FETCH_OR_SET(stereoMode);
         break;
      case GLX_AUX_BUFFERS:
         config->numAuxBuffers = *bp++;
         break;
      case GLX_RED_SIZE:
         config->redBits = *bp++;
         break;
      case GLX_GREEN_SIZE:
         config->greenBits = *bp++;
         break;
      case GLX_BLUE_SIZE:
         config->blueBits = *bp++;
         break;
      case GLX_ALPHA_SIZE:
         config->alphaBits = *bp++;
         break;
      case GLX_DEPTH_SIZE:
         config->depthBits = *bp++;
         break;
      case GLX_STENCIL_SIZE:
         config->stencilBits = *bp++;
         break;
      case GLX_ACCUM_RED_SIZE:
         config->accumRedBits = *bp++;
         break;
      case GLX_ACCUM_GREEN_SIZE:
         config->accumGreenBits = *bp++;
         break;
      case GLX_ACCUM_BLUE_SIZE:
         config->accumBlueBits = *bp++;
         break;
      case GLX_ACCUM_ALPHA_SIZE:
         config->accumAlphaBits = *bp++;
         break;
      case GLX_VISUAL_CAVEAT_EXT:
         config->visualRating = *bp++;
         break;
      case GLX_X_VISUAL_TYPE:
         config->visualType = *bp++;
         break;
      case GLX_TRANSPARENT_TYPE:
         config->transparentPixel = *bp++;
         break;
      case GLX_TRANSPARENT_INDEX_VALUE:
         config->transparentIndex = *bp++;
         break;
      case GLX_TRANSPARENT_RED_VALUE:
         config->transparentRed = *bp++;
         break;
      case GLX_TRANSPARENT_GREEN_VALUE:
         config->transparentGreen = *bp++;
         break;
      case GLX_TRANSPARENT_BLUE_VALUE:
         config->transparentBlue = *bp++;
         break;
      case GLX_TRANSPARENT_ALPHA_VALUE:
         config->transparentAlpha = *bp++;
         break;
      case GLX_VISUAL_ID:
         config->visualID = *bp++;
         break;
      case GLX_DRAWABLE_TYPE:
         config->drawableType = *bp++;
         break;
      case GLX_RENDER_TYPE:
         config->renderType = *bp++;
         break;
      case GLX_X_RENDERABLE:
         config->xRenderable = *bp++;
         break;
      case GLX_FBCONFIG_ID:
         config->fbconfigID = *bp++;
         break;
      case GLX_MAX_PBUFFER_WIDTH:
         config->maxPbufferWidth = *bp++;
         break;
      case GLX_MAX_PBUFFER_HEIGHT:
         config->maxPbufferHeight = *bp++;
         break;
      case GLX_MAX_PBUFFER_PIXELS:
         config->maxPbufferPixels = *bp++;
         break;
      case GLX_OPTIMAL_PBUFFER_WIDTH_SGIX:
         config->optimalPbufferWidth = *bp++;
         break;
      case GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX:
         config->optimalPbufferHeight = *bp++;
         break;
      case GLX_VISUAL_SELECT_GROUP_SGIX:
         config->visualSelectGroup = *bp++;
         break;
      case GLX_SWAP_METHOD_OML:
         config->swapMethod = *bp++;
         break;
      case GLX_SAMPLE_BUFFERS_SGIS:
         config->sampleBuffers = *bp++;
         break;
      case GLX_SAMPLES_SGIS:
         config->samples = *bp++;
         break;
      case GLX_BIND_TO_TEXTURE_RGB_EXT:
         config->bindToTextureRgb = *bp++;
         break;
      case GLX_BIND_TO_TEXTURE_RGBA_EXT:
         config->bindToTextureRgba = *bp++;
         break;
      case GLX_BIND_TO_MIPMAP_TEXTURE_EXT:
         config->bindToMipmapTexture = *bp++;
         break;
      case GLX_BIND_TO_TEXTURE_TARGETS_EXT:
         config->bindToTextureTargets = *bp++;
         break;
      case GLX_Y_INVERTED_EXT:
         config->yInverted = *bp++;
         break;
      case None:
         i = count;
         break;
      default:
         break;
      }
   }

   config->renderType =
      (config->rgbMode) ? GLX_RGBA_BIT : GLX_COLOR_INDEX_BIT;

   config->haveAccumBuffer = ((config->accumRedBits +
                               config->accumGreenBits +
                               config->accumBlueBits +
                               config->accumAlphaBits) > 0);
   config->haveDepthBuffer = (config->depthBits > 0);
   config->haveStencilBuffer = (config->stencilBits > 0);
}

static __GLcontextModes *
createConfigsFromProperties(Display * dpy, int nvisuals, int nprops,
                            int screen, GLboolean tagged_only)
{
   INT32 buf[__GLX_TOTAL_CONFIG], *props;
   unsigned prop_size;
   __GLcontextModes *modes, *m;
   int i;

   if (nprops == 0)
      return NULL;

   /* FIXME: Is the __GLX_MIN_CONFIG_PROPS test correct for FBconfigs? */

   /* Check number of properties */
   if (nprops < __GLX_MIN_CONFIG_PROPS || nprops > __GLX_MAX_CONFIG_PROPS)
      return NULL;

   /* Allocate memory for our config structure */
   modes = _gl_context_modes_create(nvisuals, sizeof(__GLcontextModes));
   if (!modes)
      return NULL;

   prop_size = nprops * __GLX_SIZE_INT32;
   if (prop_size <= sizeof(buf))
      props = buf;
   else
      props = Xmalloc(prop_size);

   /* Read each config structure and convert it into our format */
   m = modes;
   for (i = 0; i < nvisuals; i++) {
      _XRead(dpy, (char *) props, prop_size);
      /* Older X servers don't send this so we default it here. */
      m->drawableType = GLX_WINDOW_BIT;
      __glXInitializeVisualConfigFromTags(m, nprops, props,
                                          tagged_only, GL_TRUE);
      m->screen = screen;
      m = m->next;
   }

   if (props != buf)
      Xfree(props);

   return modes;
}

static GLboolean
getVisualConfigs(Display * dpy, __GLXdisplayPrivate * priv, int screen)
{
   xGLXGetVisualConfigsReq *req;
   __GLXscreenConfigs *psc;
   xGLXGetVisualConfigsReply reply;

   LockDisplay(dpy);

   psc = priv->screenConfigs + screen;
   psc->visuals = NULL;
   GetReq(GLXGetVisualConfigs, req);
   req->reqType = priv->majorOpcode;
   req->glxCode = X_GLXGetVisualConfigs;
   req->screen = screen;

   if (!_XReply(dpy, (xReply *) & reply, 0, False))
      goto out;

   psc->visuals = createConfigsFromProperties(dpy,
                                              reply.numVisuals,
                                              reply.numProps,
                                              screen, GL_FALSE);

 out:
   UnlockDisplay(dpy);
   return psc->visuals != NULL;
}

static GLboolean
getFBConfigs(Display * dpy, __GLXdisplayPrivate * priv, int screen)
{
   xGLXGetFBConfigsReq *fb_req;
   xGLXGetFBConfigsSGIXReq *sgi_req;
   xGLXVendorPrivateWithReplyReq *vpreq;
   xGLXGetFBConfigsReply reply;
   __GLXscreenConfigs *psc;

   psc = priv->screenConfigs + screen;
   psc->serverGLXexts = __glXQueryServerString(dpy, priv->majorOpcode, screen, GLX_EXTENSIONS);

   LockDisplay(dpy);

   psc->configs = NULL;
   if (atof(priv->serverGLXversion) >= 1.3) {
      GetReq(GLXGetFBConfigs, fb_req);
      fb_req->reqType = priv->majorOpcode;
      fb_req->glxCode = X_GLXGetFBConfigs;
      fb_req->screen = screen;
   }
   else if (strstr(psc->serverGLXexts, "GLX_SGIX_fbconfig") != NULL) {
      GetReqExtra(GLXVendorPrivateWithReply,
                  sz_xGLXGetFBConfigsSGIXReq +
                  sz_xGLXVendorPrivateWithReplyReq, vpreq);
      sgi_req = (xGLXGetFBConfigsSGIXReq *) vpreq;
      sgi_req->reqType = priv->majorOpcode;
      sgi_req->glxCode = X_GLXVendorPrivateWithReply;
      sgi_req->vendorCode = X_GLXvop_GetFBConfigsSGIX;
      sgi_req->screen = screen;
   }
   else
      goto out;

   if (!_XReply(dpy, (xReply *) & reply, 0, False))
      goto out;

   psc->configs = createConfigsFromProperties(dpy,
                                              reply.numFBConfigs,
                                              reply.numAttribs * 2,
                                              screen, GL_TRUE);

 out:
   UnlockDisplay(dpy);
   return psc->configs != NULL;
}

/*
** Allocate the memory for the per screen configs for each screen.
** If that works then fetch the per screen configs data.
*/
static Bool
AllocAndFetchScreenConfigs(Display * dpy, __GLXdisplayPrivate * priv)
{
   __GLXscreenConfigs *psc;
   GLint i, screens;

   /*
    ** First allocate memory for the array of per screen configs.
    */
   screens = ScreenCount(dpy);
   psc = (__GLXscreenConfigs *) Xmalloc(screens * sizeof(__GLXscreenConfigs));
   if (!psc) {
      return GL_FALSE;
   }
   memset(psc, 0, screens * sizeof(__GLXscreenConfigs));
   priv->screenConfigs = psc;

   priv->serverGLXversion = __glXQueryServerString(dpy, priv->majorOpcode, 0, GLX_VERSION);
   if (priv->serverGLXversion == NULL) {
      FreeScreenConfigs(priv);
      return GL_FALSE;
   }

   for (i = 0; i < screens; i++, psc++) {
      getVisualConfigs(dpy, priv, i);
      getFBConfigs(dpy, priv, i);

#ifdef GLX_DIRECT_RENDERING
      psc->scr = i;
      psc->dpy = dpy;
      psc->drawHash = __glxHashCreate();
      if (psc->drawHash == NULL)
         continue;

      if (priv->dri2Display)
         psc->driScreen = (*priv->dri2Display->createScreen) (psc, i, priv);

      if (psc->driScreen == NULL && priv->driDisplay)
         psc->driScreen = (*priv->driDisplay->createScreen) (psc, i, priv);

      if (psc->driScreen == NULL && priv->driswDisplay)
         psc->driScreen = (*priv->driswDisplay->createScreen) (psc, i, priv);

      if (psc->driScreen == NULL) {
         __glxHashDestroy(psc->drawHash);
         psc->drawHash = NULL;
      }
#endif
   }
   SyncHandle();
   return GL_TRUE;
}

/*
** Initialize the client side extension code.
*/
_X_HIDDEN __GLXdisplayPrivate *
__glXInitialize(Display * dpy)
{
   XExtDisplayInfo *info = __glXFindDisplay(dpy);
   XExtData **privList, *private, *found;
   __GLXdisplayPrivate *dpyPriv;
   XEDataObject dataObj;
   int major, minor;
#ifdef GLX_DIRECT_RENDERING
   Bool glx_direct, glx_accel;
#endif

#if defined(USE_XTHREADS)
   {
      static int firstCall = 1;
      if (firstCall) {
         /* initialize the GLX mutexes */
         xmutex_init(&__glXmutex);
         firstCall = 0;
      }
   }
#endif

   INIT_MESA_SPARC;
   /* The one and only long long lock */
   __glXLock();

   if (!XextHasExtension(info)) {
      /* No GLX extension supported by this server. Oh well. */
      __glXUnlock();
      XMissingExtension(dpy, __glXExtensionName);
      return 0;
   }

   /* See if a display private already exists.  If so, return it */
   dataObj.display = dpy;
   privList = XEHeadOfExtensionList(dataObj);
   found = XFindOnExtensionList(privList, info->codes->extension);
   if (found) {
      __glXUnlock();
      return (__GLXdisplayPrivate *) found->private_data;
   }

   /* See if the versions are compatible */
   if (!QueryVersion(dpy, info->codes->major_opcode, &major, &minor)) {
      /* The client and server do not agree on versions.  Punt. */
      __glXUnlock();
      return 0;
   }

   /*
    ** Allocate memory for all the pieces needed for this buffer.
    */
   private = (XExtData *) Xmalloc(sizeof(XExtData));
   if (!private) {
      __glXUnlock();
      return 0;
   }
   dpyPriv = (__GLXdisplayPrivate *) Xcalloc(1, sizeof(__GLXdisplayPrivate));
   if (!dpyPriv) {
      __glXUnlock();
      Xfree((char *) private);
      return 0;
   }

   /*
    ** Init the display private and then read in the screen config
    ** structures from the server.
    */
   dpyPriv->majorOpcode = info->codes->major_opcode;
   dpyPriv->majorVersion = major;
   dpyPriv->minorVersion = minor;
   dpyPriv->dpy = dpy;

   dpyPriv->serverGLXvendor = 0x0;
   dpyPriv->serverGLXversion = 0x0;

#ifdef GLX_DIRECT_RENDERING
   glx_direct = (getenv("LIBGL_ALWAYS_INDIRECT") == NULL);
   glx_accel = (getenv("LIBGL_ALWAYS_SOFTWARE") == NULL);

   /*
    ** Initialize the direct rendering per display data and functions.
    ** Note: This _must_ be done before calling any other DRI routines
    ** (e.g., those called in AllocAndFetchScreenConfigs).
    */
   if (glx_direct && glx_accel) {
      dpyPriv->dri2Display = dri2CreateDisplay(dpy);
      dpyPriv->driDisplay = driCreateDisplay(dpy);
   }
   if (glx_direct)
      dpyPriv->driswDisplay = driswCreateDisplay(dpy);
#endif

   if (!AllocAndFetchScreenConfigs(dpy, dpyPriv)) {
      __glXUnlock();
      Xfree((char *) dpyPriv);
      Xfree((char *) private);
      return 0;
   }

   /*
    ** Fill in the private structure.  This is the actual structure that
    ** hangs off of the Display structure.  Our private structure is
    ** referred to by this structure.  Got that?
    */
   private->number = info->codes->extension;
   private->next = 0;
   private->free_private = __glXFreeDisplayPrivate;
   private->private_data = (char *) dpyPriv;
   XAddToExtensionList(privList, private);

   if (dpyPriv->majorVersion == 1 && dpyPriv->minorVersion >= 1) {
      __glXClientInfo(dpy, dpyPriv->majorOpcode);
   }
   __glXUnlock();

   return dpyPriv;
}

/*
** Setup for sending a GLX command on dpy.  Make sure the extension is
** initialized.  Try to avoid calling __glXInitialize as its kinda slow.
*/
_X_HIDDEN CARD8
__glXSetupForCommand(Display * dpy)
{
   GLXContext gc;
   __GLXdisplayPrivate *priv;

   /* If this thread has a current context, flush its rendering commands */
   gc = __glXGetCurrentContext();
   if (gc->currentDpy) {
      /* Flush rendering buffer of the current context, if any */
      (void) __glXFlushRenderBuffer(gc, gc->pc);

      if (gc->currentDpy == dpy) {
         /* Use opcode from gc because its right */
         INIT_MESA_SPARC;
         return gc->majorOpcode;
      }
      else {
         /*
          ** Have to get info about argument dpy because it might be to
          ** a different server
          */
      }
   }

   /* Forced to lookup extension via the slow initialize route */
   priv = __glXInitialize(dpy);
   if (!priv) {
      return 0;
   }
   return priv->majorOpcode;
}

/**
 * Flush the drawing command transport buffer.
 * 
 * \param ctx  Context whose transport buffer is to be flushed.
 * \param pc   Pointer to first unused buffer location.
 * 
 * \todo
 * Modify this function to use \c ctx->pc instead of the explicit
 * \c pc parameter.
 */
_X_HIDDEN GLubyte *
__glXFlushRenderBuffer(__GLXcontext * ctx, GLubyte * pc)
{
   Display *const dpy = ctx->currentDpy;
#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
#else
   xGLXRenderReq *req;
#endif /* USE_XCB */
   const GLint size = pc - ctx->buf;

   if ((dpy != NULL) && (size > 0)) {
#ifdef USE_XCB
      xcb_glx_render(c, ctx->currentContextTag, size,
                     (const uint8_t *) ctx->buf);
#else
      /* Send the entire buffer as an X request */
      LockDisplay(dpy);
      GetReq(GLXRender, req);
      req->reqType = ctx->majorOpcode;
      req->glxCode = X_GLXRender;
      req->contextTag = ctx->currentContextTag;
      req->length += (size + 3) >> 2;
      _XSend(dpy, (char *) ctx->buf, size);
      UnlockDisplay(dpy);
      SyncHandle();
#endif
   }

   /* Reset pointer and return it */
   ctx->pc = ctx->buf;
   return ctx->pc;
}


/**
 * Send a portion of a GLXRenderLarge command to the server.  The advantage of
 * this function over \c __glXSendLargeCommand is that callers can use the
 * data buffer in the GLX context and may be able to avoid allocating an
 * extra buffer.  The disadvantage is the clients will have to do more
 * GLX protocol work (i.e., calculating \c totalRequests, etc.).
 *
 * \sa __glXSendLargeCommand
 *
 * \param gc             GLX context
 * \param requestNumber  Which part of the whole command is this?  The first
 *                       request is 1.
 * \param totalRequests  How many requests will there be?
 * \param data           Command data.
 * \param dataLen        Size, in bytes, of the command data.
 */
_X_HIDDEN void
__glXSendLargeChunk(__GLXcontext * gc, GLint requestNumber,
                    GLint totalRequests, const GLvoid * data, GLint dataLen)
{
   Display *dpy = gc->currentDpy;
#ifdef USE_XCB
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_glx_render_large(c, gc->currentContextTag, requestNumber,
                        totalRequests, dataLen, data);
#else
   xGLXRenderLargeReq *req;

   if (requestNumber == 1) {
      LockDisplay(dpy);
   }

   GetReq(GLXRenderLarge, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = X_GLXRenderLarge;
   req->contextTag = gc->currentContextTag;
   req->length += (dataLen + 3) >> 2;
   req->requestNumber = requestNumber;
   req->requestTotal = totalRequests;
   req->dataBytes = dataLen;
   Data(dpy, data, dataLen);

   if (requestNumber == totalRequests) {
      UnlockDisplay(dpy);
      SyncHandle();
   }
#endif /* USE_XCB */
}


/**
 * Send a command that is too large for the GLXRender protocol request.
 * 
 * Send a large command, one that is too large for some reason to
 * send using the GLXRender protocol request.  One reason to send
 * a large command is to avoid copying the data.
 * 
 * \param ctx        GLX context
 * \param header     Header data.
 * \param headerLen  Size, in bytes, of the header data.  It is assumed that
 *                   the header data will always be small enough to fit in
 *                   a single X protocol packet.
 * \param data       Command data.
 * \param dataLen    Size, in bytes, of the command data.
 */
_X_HIDDEN void
__glXSendLargeCommand(__GLXcontext * ctx,
                      const GLvoid * header, GLint headerLen,
                      const GLvoid * data, GLint dataLen)
{
   GLint maxSize;
   GLint totalRequests, requestNumber;

   /*
    ** Calculate the maximum amount of data can be stuffed into a single
    ** packet.  sz_xGLXRenderReq is added because bufSize is the maximum
    ** packet size minus sz_xGLXRenderReq.
    */
   maxSize = (ctx->bufSize + sz_xGLXRenderReq) - sz_xGLXRenderLargeReq;
   totalRequests = 1 + (dataLen / maxSize);
   if (dataLen % maxSize)
      totalRequests++;

   /*
    ** Send all of the command, except the large array, as one request.
    */
   assert(headerLen <= maxSize);
   __glXSendLargeChunk(ctx, 1, totalRequests, header, headerLen);

   /*
    ** Send enough requests until the whole array is sent.
    */
   for (requestNumber = 2; requestNumber <= (totalRequests - 1);
        requestNumber++) {
      __glXSendLargeChunk(ctx, requestNumber, totalRequests, data, maxSize);
      data = (const GLvoid *) (((const GLubyte *) data) + maxSize);
      dataLen -= maxSize;
      assert(dataLen > 0);
   }

   assert(dataLen <= maxSize);
   __glXSendLargeChunk(ctx, requestNumber, totalRequests, data, dataLen);
}

/************************************************************************/

#ifdef DEBUG
_X_HIDDEN void
__glXDumpDrawBuffer(__GLXcontext * ctx)
{
   GLubyte *p = ctx->buf;
   GLubyte *end = ctx->pc;
   GLushort opcode, length;

   while (p < end) {
      /* Fetch opcode */
      opcode = *((GLushort *) p);
      length = *((GLushort *) (p + 2));
      printf("%2x: %5d: ", opcode, length);
      length -= 4;
      p += 4;
      while (length > 0) {
         printf("%08x ", *((unsigned *) p));
         p += 4;
         length -= 4;
      }
      printf("\n");
   }
}
#endif

#ifdef  USE_SPARC_ASM
/*
 * This is where our dispatch table's bounds are.
 * And the static mesa_init is taken directly from
 * Mesa's 'sparc.c' initializer.
 *
 * We need something like this here, because this version
 * of openGL/glx never initializes a Mesa context, and so
 * the address of the dispatch table pointer never gets stuffed
 * into the dispatch jump table otherwise.
 *
 * It matters only on SPARC, and only if you are using assembler
 * code instead of C-code indirect dispatch.
 *
 * -- FEM, 04.xii.03
 */
extern unsigned int _mesa_sparc_glapi_begin;
extern unsigned int _mesa_sparc_glapi_end;
extern void __glapi_sparc_icache_flush(unsigned int *);

static void
_glx_mesa_init_sparc_glapi_relocs(void)
{
   unsigned int *insn_ptr, *end_ptr;
   unsigned long disp_addr;

   insn_ptr = &_mesa_sparc_glapi_begin;
   end_ptr = &_mesa_sparc_glapi_end;
   disp_addr = (unsigned long) &_glapi_Dispatch;

   /*
    * Verbatim from Mesa sparc.c.  It's needed because there doesn't
    * seem to be a better way to do this:
    *
    * UNCONDITIONAL_JUMP ( (*_glapi_Dispatch) + entry_offset )
    *
    * This code is patching in the ADDRESS of the pointer to the
    * dispatch table.  Hence, it must be called exactly once, because
    * that address is not going to change.
    *
    * What it points to can change, but Mesa (and hence, we) assume
    * that there is only one pointer.
    *
    */
   while (insn_ptr < end_ptr) {
#if ( defined(__sparc_v9__) && ( !defined(__linux__) || defined(__linux_64__) ) )
/*
	This code patches for 64-bit addresses.  This had better
	not happen for Sparc/Linux, no matter what architecture we
	are building for.  So, don't do this.

        The 'defined(__linux_64__)' is used here as a placeholder for
        when we do do 64-bit usermode on sparc linux.
	*/
      insn_ptr[0] |= (disp_addr >> (32 + 10));
      insn_ptr[1] |= ((disp_addr & 0xffffffff) >> 10);
      __glapi_sparc_icache_flush(&insn_ptr[0]);
      insn_ptr[2] |= ((disp_addr >> 32) & ((1 << 10) - 1));
      insn_ptr[3] |= (disp_addr & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&insn_ptr[2]);
      insn_ptr += 11;
#else
      insn_ptr[0] |= (disp_addr >> 10);
      insn_ptr[1] |= (disp_addr & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&insn_ptr[0]);
      insn_ptr += 5;
#endif
   }
}
#endif /* sparc ASM in use */
