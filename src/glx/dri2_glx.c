/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Soft-
 * ware"), to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, provided that the above copyright
 * notice(s) and this permission notice appear in all copies of the Soft-
 * ware and that both the above copyright notice(s) and this permission
 * notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABIL-
 * ITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY
 * RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS INCLUDED IN
 * THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR CONSE-
 * QUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFOR-
 * MANCE OF THIS SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization of
 * the copyright holder.
 *
 * Authors:
 *   Kristian Høgsberg (krh@redhat.com)
 */

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)

#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xdamage.h>
#include "glapi.h"
#include "glxclient.h"
#include <X11/extensions/dri2proto.h>
#include "xf86dri.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include "xf86drm.h"
#include "dri2.h"
#include "dri_common.h"

/* From xmlpool/options.h, user exposed so should be stable */
#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3

#undef DRI2_MINOR
#define DRI2_MINOR 1

struct dri2_display
{
   __GLXDRIdisplay base;

   /*
    ** XFree86-DRI version information
    */
   int driMajor;
   int driMinor;
   int driPatch;
   int swapAvailable;
   int invalidateAvailable;

   __glxHashTable *dri2Hash;

   const __DRIextension *loader_extensions[4];
};

struct dri2_screen {
   __GLXscreenConfigs base;

   __DRIscreen *driScreen;
   __GLXDRIscreen vtable;
   const __DRIdri2Extension *dri2;
   const __DRIcoreExtension *core;

   const __DRI2flushExtension *f;
   const __DRI2configQueryExtension *config;
   const __DRItexBufferExtension *texBuffer;

   void *driver;
   int fd;
};

struct dri2_context
{
   __GLXDRIcontext base;
   __DRIcontext *driContext;
   __GLXscreenConfigs *psc;
};

struct dri2_drawable
{
   __GLXDRIdrawable base;
   __DRIbuffer buffers[5];
   int bufferCount;
   int width, height;
   int have_back;
   int have_fake_front;
   int swap_interval;
};

static void
dri2DestroyContext(__GLXDRIcontext *context,
		   __GLXscreenConfigs *base, Display *dpy)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) base;

   (*psc->core->destroyContext) (pcp->driContext);

   Xfree(pcp);
}

static Bool
dri2BindContext(__GLXDRIcontext *context,
		__GLXDRIdrawable *draw, __GLXDRIdrawable *read)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) pcp->psc;

   return (*psc->core->bindContext) (pcp->driContext,
				     draw->driDrawable, read->driDrawable);
}

static void
dri2UnbindContext(__GLXDRIcontext *context)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) pcp->psc;

   (*psc->core->unbindContext) (pcp->driContext);
}

static __GLXDRIcontext *
dri2CreateContext(__GLXscreenConfigs *base,
                  const __GLcontextModes * mode,
                  GLXContext gc, GLXContext shareList, int renderType)
{
   struct dri2_context *pcp, *pcp_shared;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) mode;
   __DRIcontext *shared = NULL;

   if (shareList) {
      pcp_shared = (struct dri2_context *) shareList->driContext;
      shared = pcp_shared->driContext;
   }

   pcp = Xmalloc(sizeof *pcp);
   if (pcp == NULL)
      return NULL;

   pcp->psc = &psc->base;
   pcp->driContext =
      (*psc->dri2->createNewContext) (psc->driScreen,
                                      config->driConfig, shared, pcp);
   gc->__driContext = pcp->driContext;

   if (pcp->driContext == NULL) {
      Xfree(pcp);
      return NULL;
   }

   pcp->base.destroyContext = dri2DestroyContext;
   pcp->base.bindContext = dri2BindContext;
   pcp->base.unbindContext = dri2UnbindContext;

   return &pcp->base;
}

static void
dri2DestroyDrawable(__GLXDRIdrawable *pdraw)
{
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;
   __GLXdisplayPrivate *dpyPriv;
   struct dri2_display *pdp;

   dpyPriv = __glXInitialize(pdraw->psc->dpy);
   pdp = (struct dri2_display *)dpyPriv->dri2Display;

   __glxHashDelete(pdp->dri2Hash, pdraw->xDrawable);
   (*psc->core->destroyDrawable) (pdraw->driDrawable);
   DRI2DestroyDrawable(psc->base.dpy, pdraw->xDrawable);
   Xfree(pdraw);
}

static __GLXDRIdrawable *
dri2CreateDrawable(__GLXscreenConfigs *base, XID xDrawable,
		   GLXDrawable drawable, const __GLcontextModes * modes)
{
   struct dri2_drawable *pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;
   __GLXdisplayPrivate *dpyPriv;
   struct dri2_display *pdp;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;

   pdraw = Xmalloc(sizeof(*pdraw));
   if (!pdraw)
      return NULL;

   pdraw->base.destroyDrawable = dri2DestroyDrawable;
   pdraw->base.xDrawable = xDrawable;
   pdraw->base.drawable = drawable;
   pdraw->base.psc = &psc->base;
   pdraw->bufferCount = 0;
   pdraw->swap_interval = 1; /* default may be overridden below */
   pdraw->have_back = 0;

   if (psc->config)
      psc->config->configQueryi(psc->driScreen,
				"vblank_mode", &vblank_mode);

   switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
   case DRI_CONF_VBLANK_DEF_INTERVAL_0:
      pdraw->swap_interval = 0;
      break;
   case DRI_CONF_VBLANK_DEF_INTERVAL_1:
   case DRI_CONF_VBLANK_ALWAYS_SYNC:
   default:
      pdraw->swap_interval = 1;
      break;
   }

   DRI2CreateDrawable(psc->base.dpy, xDrawable);

   dpyPriv = __glXInitialize(psc->base.dpy);
   pdp = (struct dri2_display *)dpyPriv->dri2Display;;
   /* Create a new drawable */
   pdraw->base.driDrawable =
      (*psc->dri2->createNewDrawable) (psc->driScreen,
                                       config->driConfig, pdraw);

   if (!pdraw->base.driDrawable) {
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
      Xfree(pdraw);
      return NULL;
   }

   if (__glxHashInsert(pdp->dri2Hash, xDrawable, pdraw)) {
      (*psc->core->destroyDrawable) (pdraw->base.driDrawable);
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
      Xfree(pdraw);
      return None;
   }


#ifdef X_DRI2SwapInterval
   /*
    * Make sure server has the same swap interval we do for the new
    * drawable.
    */
   if (pdp->swapAvailable)
      DRI2SwapInterval(psc->base.dpy, xDrawable, pdraw->swap_interval);
#endif

   return &pdraw->base;
}

#ifdef X_DRI2GetMSC

static int
dri2DrawableGetMSC(__GLXscreenConfigs *psc, __GLXDRIdrawable *pdraw,
		   int64_t *ust, int64_t *msc, int64_t *sbc)
{
   return DRI2GetMSC(psc->dpy, pdraw->xDrawable, ust, msc, sbc);
}

#endif


#ifdef X_DRI2WaitMSC

static int
dri2WaitForMSC(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
	       int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc)
{
   return DRI2WaitMSC(pdraw->psc->dpy, pdraw->xDrawable, target_msc, divisor,
		      remainder, ust, msc, sbc);
}

static int
dri2WaitForSBC(__GLXDRIdrawable *pdraw, int64_t target_sbc, int64_t *ust,
	       int64_t *msc, int64_t *sbc)
{
   return DRI2WaitSBC(pdraw->psc->dpy, pdraw->xDrawable, target_sbc, ust, msc,
		      sbc);
}

#endif /* X_DRI2WaitMSC */

static void
dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y, int width, int height)
{
   struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;
   XRectangle xrect;
   XserverRegion region;

   /* Check we have the right attachments */
   if (!priv->have_back)
      return;

   xrect.x = x;
   xrect.y = priv->height - y - height;
   xrect.width = width;
   xrect.height = height;

#ifdef __DRI2_FLUSH
   if (psc->f)
      (*psc->f->flush) (pdraw->driDrawable);
#endif

   region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
                  DRI2BufferFrontLeft, DRI2BufferBackLeft);
   XFixesDestroyRegion(psc->base.dpy, region);

   /* Refresh the fake front (if present) after we just damaged the real
    * front.
    */
   DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
		  DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);
   XFixesDestroyRegion(psc->base.dpy, region);
}

static void
dri2_copy_drawable(struct dri2_drawable *priv, int dest, int src)
{
   XRectangle xrect;
   XserverRegion region;
   struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;

   xrect.x = 0;
   xrect.y = 0;
   xrect.width = priv->width;
   xrect.height = priv->height;

#ifdef __DRI2_FLUSH
   if (psc->f)
      (*psc->f->flush) (priv->base.driDrawable);
#endif

   region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, priv->base.xDrawable, region, dest, src);
   XFixesDestroyRegion(psc->base.dpy, region);

}

static void
dri2WaitX(__GLXDRIdrawable *pdraw)
{
   struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;

   if (!priv->have_fake_front)
      return;

   dri2_copy_drawable(priv, DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);
}

static void
dri2WaitGL(__GLXDRIdrawable * pdraw)
{
   struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;

   if (!priv->have_fake_front)
      return;

   dri2_copy_drawable(priv, DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
}

static void
dri2FlushFrontBuffer(__DRIdrawable *driDrawable, void *loaderPrivate)
{
   struct dri2_drawable *pdraw = loaderPrivate;
   __GLXdisplayPrivate *priv = __glXInitialize(pdraw->base.psc->dpy);
   struct dri2_display *pdp = (struct dri2_display *)priv->dri2Display;

   /* Old servers don't send invalidate events */
   if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(priv->dpy, pdraw->base.drawable);

   dri2WaitGL(loaderPrivate);
}


static void
dri2DestroyScreen(__GLXscreenConfigs *base)
{
   struct dri2_screen *psc = (struct dri2_screen *) base;

   /* Free the direct rendering per screen data */
   (*psc->core->destroyScreen) (psc->driScreen);
   close(psc->fd);
   Xfree(psc);
}

/**
 * Process list of buffer received from the server
 *
 * Processes the list of buffers received in a reply from the server to either
 * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
 */
static void
process_buffers(struct dri2_drawable * pdraw, DRI2Buffer * buffers,
                unsigned count)
{
   int i;

   pdraw->bufferCount = count;
   pdraw->have_fake_front = 0;
   pdraw->have_back = 0;

   /* This assumes the DRI2 buffer attachment tokens matches the
    * __DRIbuffer tokens. */
   for (i = 0; i < count; i++) {
      pdraw->buffers[i].attachment = buffers[i].attachment;
      pdraw->buffers[i].name = buffers[i].name;
      pdraw->buffers[i].pitch = buffers[i].pitch;
      pdraw->buffers[i].cpp = buffers[i].cpp;
      pdraw->buffers[i].flags = buffers[i].flags;
      if (pdraw->buffers[i].attachment == __DRI_BUFFER_FAKE_FRONT_LEFT)
         pdraw->have_fake_front = 1;
      if (pdraw->buffers[i].attachment == __DRI_BUFFER_BACK_LEFT)
         pdraw->have_back = 1;
   }

}

static int64_t
dri2SwapBuffers(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
		int64_t remainder)
{
    struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
    __GLXdisplayPrivate *dpyPriv = __glXInitialize(priv->base.psc->dpy);
    struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;
    struct dri2_display *pdp =
	(struct dri2_display *)dpyPriv->dri2Display;
    int64_t ret;

#ifdef __DRI2_FLUSH
    if (psc->f)
    	(*psc->f->flush)(pdraw->driDrawable);
#endif

    /* Old servers don't send invalidate events */
    if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(dpyPriv->dpy, pdraw->drawable);

    /* Old servers can't handle swapbuffers */
    if (!pdp->swapAvailable) {
       dri2CopySubBuffer(pdraw, 0, 0, priv->width, priv->height);
       return 0;
    }

#ifdef X_DRI2SwapBuffers
    DRI2SwapBuffers(psc->base.dpy, pdraw->xDrawable, target_msc, divisor,
		    remainder, &ret);
#endif

    return ret;
}

static __DRIbuffer *
dri2GetBuffers(__DRIdrawable * driDrawable,
               int *width, int *height,
               unsigned int *attachments, int count,
               int *out_count, void *loaderPrivate)
{
   struct dri2_drawable *pdraw = loaderPrivate;
   DRI2Buffer *buffers;

   buffers = DRI2GetBuffers(pdraw->base.psc->dpy, pdraw->base.xDrawable,
                            width, height, attachments, count, out_count);
   if (buffers == NULL)
      return NULL;

   pdraw->width = *width;
   pdraw->height = *height;
   process_buffers(pdraw, buffers, *out_count);

   Xfree(buffers);

   return pdraw->buffers;
}

static __DRIbuffer *
dri2GetBuffersWithFormat(__DRIdrawable * driDrawable,
                         int *width, int *height,
                         unsigned int *attachments, int count,
                         int *out_count, void *loaderPrivate)
{
   struct dri2_drawable *pdraw = loaderPrivate;
   DRI2Buffer *buffers;

   buffers = DRI2GetBuffersWithFormat(pdraw->base.psc->dpy,
                                      pdraw->base.xDrawable,
                                      width, height, attachments,
                                      count, out_count);
   if (buffers == NULL)
      return NULL;

   pdraw->width = *width;
   pdraw->height = *height;
   process_buffers(pdraw, buffers, *out_count);

   Xfree(buffers);

   return pdraw->buffers;
}

#ifdef X_DRI2SwapInterval

static int
dri2SetSwapInterval(__GLXDRIdrawable *pdraw, int interval)
{
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;
   struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;

   if (psc->config)
      psc->config->configQueryi(psc->driScreen,
				"vblank_mode", &vblank_mode);

   switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
      return GLX_BAD_VALUE;
   case DRI_CONF_VBLANK_ALWAYS_SYNC:
      if (interval <= 0)
	 return GLX_BAD_VALUE;
      break;
   default:
      break;
   }

   DRI2SwapInterval(priv->base.psc->dpy, priv->base.xDrawable, interval);
   priv->swap_interval = interval;

   return 0;
}

static unsigned int
dri2GetSwapInterval(__GLXDRIdrawable *pdraw)
{
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;

  return priv->swap_interval;
}

#endif /* X_DRI2SwapInterval */

static const __DRIdri2LoaderExtension dri2LoaderExtension = {
   {__DRI_DRI2_LOADER, __DRI_DRI2_LOADER_VERSION},
   dri2GetBuffers,
   dri2FlushFrontBuffer,
   dri2GetBuffersWithFormat,
};

static const __DRIdri2LoaderExtension dri2LoaderExtension_old = {
   {__DRI_DRI2_LOADER, __DRI_DRI2_LOADER_VERSION},
   dri2GetBuffers,
   dri2FlushFrontBuffer,
   NULL,
};

#ifdef __DRI_USE_INVALIDATE
static const __DRIuseInvalidateExtension dri2UseInvalidate = {
   { __DRI_USE_INVALIDATE, __DRI_USE_INVALIDATE_VERSION }
};
#endif

_X_HIDDEN void
dri2InvalidateBuffers(Display *dpy, XID drawable)
{
   __GLXDRIdrawable *pdraw =
      dri2GetGlxDrawableFromXDrawableId(dpy, drawable);
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;

#if __DRI2_FLUSH_VERSION >= 3
   if (pdraw && psc->f)
       psc->f->invalidate(pdraw->driDrawable);
#endif
}

static void
dri2_bind_tex_image(Display * dpy,
		    GLXDrawable drawable,
		    int buffer, const int *attrib_list)
{
   GLXContext gc = __glXGetCurrentContext();
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable, NULL);
    __GLXdisplayPrivate *dpyPriv = __glXInitialize(dpy);
    struct dri2_display *pdp =
	(struct dri2_display *) dpyPriv->dri2Display;
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;

   if (pdraw != NULL) {

#if __DRI2_FLUSH_VERSION >= 3
      if (!pdp->invalidateAvailable && psc->f)
	 psc->f->invalidate(pdraw->driDrawable);
#endif

      if (psc->texBuffer->base.version >= 2 &&
	  psc->texBuffer->setTexBuffer2 != NULL) {
	 (*psc->texBuffer->setTexBuffer2) (gc->__driContext,
					   pdraw->textureTarget,
					   pdraw->textureFormat,
					   pdraw->driDrawable);
      }
      else {
	 (*psc->texBuffer->setTexBuffer) (gc->__driContext,
					  pdraw->textureTarget,
					  pdraw->driDrawable);
      }
   }
}

static void
dri2_release_tex_image(Display * dpy, GLXDrawable drawable, int buffer)
{
}

static const struct glx_context_vtable dri2_context_vtable = {
   dri2_bind_tex_image,
   dri2_release_tex_image,
};

static void
dri2BindExtensions(struct dri2_screen *psc, const __DRIextension **extensions)
{
   int i;

   __glXEnableDirectExtension(&psc->base, "GLX_SGI_video_sync");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_swap_control");

   /* FIXME: if DRI2 version supports it... */
   __glXEnableDirectExtension(&psc->base, "INTEL_swap_event");

   for (i = 0; extensions[i]; i++) {
      if ((strcmp(extensions[i]->name, __DRI_TEX_BUFFER) == 0)) {
	 psc->texBuffer = (__DRItexBufferExtension *) extensions[i];
	 __glXEnableDirectExtension(&psc->base, "GLX_EXT_texture_from_pixmap");
      }

      if ((strcmp(extensions[i]->name, __DRI2_FLUSH) == 0)) {
	 psc->f = (__DRI2flushExtension *) extensions[i];
	 /* internal driver extension, no GL extension exposed */
      }

      if ((strcmp(extensions[i]->name, __DRI2_CONFIG_QUERY) == 0))
	 psc->config = (__DRI2configQueryExtension *) extensions[i];
   }
}


static __GLXscreenConfigs *
dri2CreateScreen(int screen, __GLXdisplayPrivate * priv)
{
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   const struct dri2_display *const pdp = (struct dri2_display *)
      priv->dri2Display;
   struct dri2_screen *psc;
   __GLXDRIscreen *psp;
   char *driverName, *deviceName;
   drm_magic_t magic;
   int i;

   psc = Xmalloc(sizeof *psc);
   if (psc == NULL)
      return NULL;

   memset(psc, 0, sizeof *psc);
   if (!glx_screen_init(&psc->base, screen, priv))
       return NULL;

   if (!DRI2Connect(priv->dpy, RootWindow(priv->dpy, screen),
		    &driverName, &deviceName)) {
      XFree(psc);
      return NULL;
   }

   psc->driver = driOpenDriver(driverName);
   if (psc->driver == NULL) {
      ErrorMessageF("driver pointer missing\n");
      goto handle_error;
   }

   extensions = dlsym(psc->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      ErrorMessageF("driver exports no extensions (%s)\n", dlerror());
      goto handle_error;
   }

   for (i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, __DRI_CORE) == 0)
	 psc->core = (__DRIcoreExtension *) extensions[i];
      if (strcmp(extensions[i]->name, __DRI_DRI2) == 0)
	 psc->dri2 = (__DRIdri2Extension *) extensions[i];
   }

   if (psc->core == NULL || psc->dri2 == NULL) {
      ErrorMessageF("core dri or dri2 extension not found\n");
      goto handle_error;
   }

   psc->fd = open(deviceName, O_RDWR);
   if (psc->fd < 0) {
      ErrorMessageF("failed to open drm device: %s\n", strerror(errno));
      goto handle_error;
   }

   if (drmGetMagic(psc->fd, &magic)) {
      ErrorMessageF("failed to get magic\n");
      goto handle_error;
   }

   if (!DRI2Authenticate(priv->dpy, RootWindow(priv->dpy, screen), magic)) {
      ErrorMessageF("failed to authenticate magic %d\n", magic);
      goto handle_error;
   }

   
   /* If the server does not support the protocol for
    * DRI2GetBuffersWithFormat, don't supply that interface to the driver.
    */
   psc->driScreen =
      psc->dri2->createNewScreen(screen, psc->fd,
				 (const __DRIextension **)
				 &pdp->loader_extensions[0],
				 &driver_configs, psc);

   if (psc->driScreen == NULL) {
      ErrorMessageF("failed to create dri screen\n");
      goto handle_error;
   }

   extensions = psc->core->getExtensions(psc->driScreen);
   driBindCommonExtensions(&psc->base, extensions);
   dri2BindExtensions(psc, extensions);

   psc->base.configs =
      driConvertConfigs(psc->core, psc->base.configs, driver_configs);
   psc->base.visuals =
      driConvertConfigs(psc->core, psc->base.visuals, driver_configs);

   psc->base.driver_configs = driver_configs;

   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = dri2DestroyScreen;
   psp->createContext = dri2CreateContext;
   psp->createDrawable = dri2CreateDrawable;
   psp->swapBuffers = dri2SwapBuffers;
   psp->waitGL = dri2WaitGL;
   psp->waitX = dri2WaitX;
   psp->getDrawableMSC = NULL;
   psp->waitForMSC = NULL;
   psp->waitForSBC = NULL;
   psp->setSwapInterval = NULL;
   psp->getSwapInterval = NULL;

   if (pdp->driMinor >= 2) {
#ifdef X_DRI2GetMSC
      psp->getDrawableMSC = dri2DrawableGetMSC;
#endif
#ifdef X_DRI2WaitMSC
      psp->waitForMSC = dri2WaitForMSC;
      psp->waitForSBC = dri2WaitForSBC;
#endif
#ifdef X_DRI2SwapInterval
      psp->setSwapInterval = dri2SetSwapInterval;
      psp->getSwapInterval = dri2GetSwapInterval;
#endif
#if defined(X_DRI2GetMSC) && defined(X_DRI2WaitMSC) && defined(X_DRI2SwapInterval)
      __glXEnableDirectExtension(&psc->base, "GLX_OML_sync_control");
#endif
   }

   /* DRI2 suports SubBuffer through DRI2CopyRegion, so it's always
    * available.*/
   psp->copySubBuffer = dri2CopySubBuffer;
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_copy_sub_buffer");

   psc->base.direct_context_vtable = &dri2_context_vtable;

   Xfree(driverName);
   Xfree(deviceName);

   return &psc->base;

handle_error:
   Xfree(driverName);
   Xfree(deviceName);
   XFree(psc);

   /* FIXME: clean up here */

   return NULL;
}

/* Called from __glXFreeDisplayPrivate.
 */
static void
dri2DestroyDisplay(__GLXDRIdisplay * dpy)
{
   Xfree(dpy);
}

_X_HIDDEN __GLXDRIdrawable *
dri2GetGlxDrawableFromXDrawableId(Display *dpy, XID id)
{
   __GLXdisplayPrivate *d = __glXInitialize(dpy);
   struct dri2_display *pdp = (struct dri2_display *) d->dri2Display;
   __GLXDRIdrawable *pdraw;

   if (__glxHashLookup(pdp->dri2Hash, id, (void *) &pdraw) == 0)
      return pdraw;

   return NULL;
}

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *
dri2CreateDisplay(Display * dpy)
{
   struct dri2_display *pdp;
   int eventBase, errorBase, i;

   if (!DRI2QueryExtension(dpy, &eventBase, &errorBase))
      return NULL;

   pdp = Xmalloc(sizeof *pdp);
   if (pdp == NULL)
      return NULL;

   if (!DRI2QueryVersion(dpy, &pdp->driMajor, &pdp->driMinor)) {
      Xfree(pdp);
      return NULL;
   }

   pdp->driPatch = 0;
   pdp->swapAvailable = (pdp->driMinor >= 2);
   pdp->invalidateAvailable = (pdp->driMinor >= 3);

   pdp->base.destroyDisplay = dri2DestroyDisplay;
   pdp->base.createScreen = dri2CreateScreen;

   i = 0;
   if (pdp->driMinor < 1)
      pdp->loader_extensions[i++] = &dri2LoaderExtension_old.base;
   else
      pdp->loader_extensions[i++] = &dri2LoaderExtension.base;
   
   pdp->loader_extensions[i++] = &systemTimeExtension.base;

#ifdef __DRI_USE_INVALIDATE
   pdp->loader_extensions[i++] = &dri2UseInvalidate.base;
#endif
   pdp->loader_extensions[i++] = NULL;

   pdp->dri2Hash = __glxHashCreate();
   if (pdp->dri2Hash == NULL) {
      Xfree(pdp);
      return NULL;
   }

   return &pdp->base;
}

#endif /* GLX_DIRECT_RENDERING */
