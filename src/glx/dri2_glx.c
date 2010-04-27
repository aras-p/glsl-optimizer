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
#include "../../mesa/drivers/dri/common/dri_util.h"

#undef DRI2_MINOR
#define DRI2_MINOR 1

typedef struct __GLXDRIdisplayPrivateRec __GLXDRIdisplayPrivate;
typedef struct __GLXDRIcontextPrivateRec __GLXDRIcontextPrivate;
typedef struct __GLXDRIdrawablePrivateRec __GLXDRIdrawablePrivate;

struct __GLXDRIdisplayPrivateRec
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
};

struct __GLXDRIcontextPrivateRec
{
   __GLXDRIcontext base;
   __DRIcontext *driContext;
   __GLXscreenConfigs *psc;
};

struct __GLXDRIdrawablePrivateRec
{
   __GLXDRIdrawable base;
   __DRIbuffer buffers[5];
   int bufferCount;
   int width, height;
   int have_back;
   int have_fake_front;
   int swap_interval;
};

static void dri2WaitX(__GLXDRIdrawable * pdraw);

static void
dri2DestroyContext(__GLXDRIcontext * context,
                   __GLXscreenConfigs * psc, Display * dpy)
{
   __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
   const __DRIcoreExtension *core = pcp->psc->core;

   (*core->destroyContext) (pcp->driContext);

   Xfree(pcp);
}

static Bool
dri2BindContext(__GLXDRIcontext * context,
                __GLXDRIdrawable * draw, __GLXDRIdrawable * read)
{
   __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
   const __DRIcoreExtension *core = pcp->psc->core;

   return (*core->bindContext) (pcp->driContext,
                                draw->driDrawable, read->driDrawable);
}

static void
dri2UnbindContext(__GLXDRIcontext * context)
{
   __GLXDRIcontextPrivate *pcp = (__GLXDRIcontextPrivate *) context;
   const __DRIcoreExtension *core = pcp->psc->core;

   (*core->unbindContext) (pcp->driContext);
}

static __GLXDRIcontext *
dri2CreateContext(__GLXscreenConfigs * psc,
                  const __GLcontextModes * mode,
                  GLXContext gc, GLXContext shareList, int renderType)
{
   __GLXDRIcontextPrivate *pcp, *pcp_shared;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) mode;
   __DRIcontext *shared = NULL;

   if (shareList) {
      pcp_shared = (__GLXDRIcontextPrivate *) shareList->driContext;
      shared = pcp_shared->driContext;
   }

   pcp = Xmalloc(sizeof *pcp);
   if (pcp == NULL)
      return NULL;

   pcp->psc = psc;
   pcp->driContext =
      (*psc->dri2->createNewContext) (psc->__driScreen,
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
dri2DestroyDrawable(__GLXDRIdrawable * pdraw)
{
   const __DRIcoreExtension *core = pdraw->psc->core;

   (*core->destroyDrawable) (pdraw->driDrawable);
   DRI2DestroyDrawable(pdraw->psc->dpy, pdraw->xDrawable);
   Xfree(pdraw);
}

static __GLXDRIdrawable *
dri2CreateDrawable(__GLXscreenConfigs * psc,
                   XID xDrawable,
                   GLXDrawable drawable, const __GLcontextModes * modes)
{
   __GLXDRIdrawablePrivate *pdraw;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;
   __GLXdisplayPrivate *dpyPriv;
   __GLXDRIdisplayPrivate *pdp;

   pdraw = Xmalloc(sizeof(*pdraw));
   if (!pdraw)
      return NULL;

   pdraw->base.destroyDrawable = dri2DestroyDrawable;
   pdraw->base.xDrawable = xDrawable;
   pdraw->base.drawable = drawable;
   pdraw->base.psc = psc;
   pdraw->bufferCount = 0;
   pdraw->swap_interval = 1;
   pdraw->have_back = 0;

   DRI2CreateDrawable(psc->dpy, xDrawable);

   dpyPriv = __glXInitialize(psc->dpy);
   pdp = (__GLXDRIdisplayPrivate *)dpyPriv->dri2Display;;
   /* Create a new drawable */
   pdraw->base.driDrawable =
      (*psc->dri2->createNewDrawable) (psc->__driScreen,
                                       config->driConfig, pdraw);

   if (!pdraw->base.driDrawable) {
      DRI2DestroyDrawable(psc->dpy, xDrawable);
      Xfree(pdraw);
      return NULL;
   }

#ifdef X_DRI2SwapInterval
   /*
    * Make sure server has the same swap interval we do for the new
    * drawable.
    */
   if (pdp->swapAvailable)
      DRI2SwapInterval(psc->dpy, xDrawable, pdraw->swap_interval);
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
   __GLXDRIdrawablePrivate *priv = (__GLXDRIdrawablePrivate *) pdraw;
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
   if (pdraw->psc->f)
      (*pdraw->psc->f->flush) (pdraw->driDrawable);
#endif

   region = XFixesCreateRegion(pdraw->psc->dpy, &xrect, 1);
   /* should get a fence ID back from here at some point */
   DRI2CopyRegion(pdraw->psc->dpy, pdraw->xDrawable, region,
                  DRI2BufferFrontLeft, DRI2BufferBackLeft);
   XFixesDestroyRegion(pdraw->psc->dpy, region);

   /* Refresh the fake front (if present) after we just damaged the real
    * front.
    */
   dri2WaitX(pdraw);
}

static void
dri2WaitX(__GLXDRIdrawable *pdraw)
{
   __GLXDRIdrawablePrivate *priv = (__GLXDRIdrawablePrivate *) pdraw;
   XRectangle xrect;
   XserverRegion region;

   /* Check we have the right attachments */
   if (!priv->have_fake_front)
      return;

   xrect.x = 0;
   xrect.y = 0;
   xrect.width = priv->width;
   xrect.height = priv->height;

#ifdef __DRI2_FLUSH
   if (pdraw->psc->f)
      (*pdraw->psc->f->flush) (pdraw->driDrawable);
#endif

   region = XFixesCreateRegion(pdraw->psc->dpy, &xrect, 1);
   DRI2CopyRegion(pdraw->psc->dpy, pdraw->xDrawable, region,
                  DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);
   XFixesDestroyRegion(pdraw->psc->dpy, region);
}

static void
dri2WaitGL(__GLXDRIdrawable * pdraw)
{
   __GLXDRIdrawablePrivate *priv = (__GLXDRIdrawablePrivate *) pdraw;
   XRectangle xrect;
   XserverRegion region;

   if (!priv->have_fake_front)
      return;

   xrect.x = 0;
   xrect.y = 0;
   xrect.width = priv->width;
   xrect.height = priv->height;

#ifdef __DRI2_FLUSH
   if (pdraw->psc->f)
      (*pdraw->psc->f->flush) (pdraw->driDrawable);
#endif

   region = XFixesCreateRegion(pdraw->psc->dpy, &xrect, 1);
   DRI2CopyRegion(pdraw->psc->dpy, pdraw->xDrawable, region,
                  DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
   XFixesDestroyRegion(pdraw->psc->dpy, region);
}

static void
dri2FlushFrontBuffer(__DRIdrawable *driDrawable, void *loaderPrivate)
{
   __GLXDRIdrawablePrivate *pdraw = loaderPrivate;
   __GLXdisplayPrivate *priv = __glXInitialize(pdraw->base.psc->dpy);
   __GLXDRIdisplayPrivate *pdp = (__GLXDRIdisplayPrivate *)priv->dri2Display;

   /* Old servers don't send invalidate events */
   if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(priv->dpy, pdraw->base.drawable);

   dri2WaitGL(loaderPrivate);
}


static void
dri2DestroyScreen(__GLXscreenConfigs * psc)
{
   /* Free the direct rendering per screen data */
   (*psc->core->destroyScreen) (psc->__driScreen);
   close(psc->fd);
   psc->__driScreen = NULL;
}

/**
 * Process list of buffer received from the server
 *
 * Processes the list of buffers received in a reply from the server to either
 * \c DRI2GetBuffers or \c DRI2GetBuffersWithFormat.
 */
static void
process_buffers(__GLXDRIdrawablePrivate * pdraw, DRI2Buffer * buffers,
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
    __GLXDRIdrawablePrivate *priv = (__GLXDRIdrawablePrivate *) pdraw;
    __GLXdisplayPrivate *dpyPriv = __glXInitialize(priv->base.psc->dpy);
    __GLXDRIdisplayPrivate *pdp =
	(__GLXDRIdisplayPrivate *)dpyPriv->dri2Display;
    int64_t ret;

#ifdef __DRI2_FLUSH
    if (pdraw->psc->f)
    	(*pdraw->psc->f->flush)(pdraw->driDrawable);
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
    DRI2SwapBuffers(pdraw->psc->dpy, pdraw->xDrawable, target_msc, divisor,
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
   __GLXDRIdrawablePrivate *pdraw = loaderPrivate;
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
   __GLXDRIdrawablePrivate *pdraw = loaderPrivate;
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

static void
dri2SetSwapInterval(__GLXDRIdrawable *pdraw, int interval)
{
   __GLXDRIdrawablePrivate *priv =  (__GLXDRIdrawablePrivate *) pdraw;

   DRI2SwapInterval(priv->base.psc->dpy, pdraw->xDrawable, interval);
   priv->swap_interval = interval;
}

static unsigned int
dri2GetSwapInterval(__GLXDRIdrawable *pdraw)
{
   __GLXDRIdrawablePrivate *priv =  (__GLXDRIdrawablePrivate *) pdraw;

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

static const __DRIextension *loader_extensions[] = {
   &dri2LoaderExtension.base,
   &systemTimeExtension.base,
   NULL
};

static const __DRIextension *loader_extensions_old[] = {
   &dri2LoaderExtension_old.base,
   &systemTimeExtension.base,
   NULL
};

_X_HIDDEN void
dri2InvalidateBuffers(Display *dpy, XID drawable)
{
   __GLXDRIdrawable *pdraw = GetGLXDRIDrawable(dpy, drawable, NULL);

#if __DRI2_FLUSH_VERSION >= 3
   if (pdraw && pdraw->psc->f)
       pdraw->psc->f->invalidate(pdraw->driDrawable);
#endif
}

static __GLXDRIscreen *
dri2CreateScreen(__GLXscreenConfigs * psc, int screen,
                 __GLXdisplayPrivate * priv)
{
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   const __GLXDRIdisplayPrivate *const pdp = (__GLXDRIdisplayPrivate *)
      priv->dri2Display;
   __GLXDRIscreen *psp;
   char *driverName, *deviceName;
   drm_magic_t magic;
   int i;

   psp = Xmalloc(sizeof *psp);
   if (psp == NULL)
      return NULL;

   if (!DRI2Connect(psc->dpy, RootWindow(psc->dpy, screen),
		    &driverName, &deviceName)) {
      XFree(psp);
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

   if (!DRI2Authenticate(psc->dpy, RootWindow(psc->dpy, screen), magic)) {
      ErrorMessageF("failed to authenticate magic %d\n", magic);
      goto handle_error;
   }

   /* If the server does not support the protocol for
    * DRI2GetBuffersWithFormat, don't supply that interface to the driver.
    */
   psc->__driScreen =
      psc->dri2->createNewScreen(screen, psc->fd, ((pdp->driMinor < 1)
						   ? loader_extensions_old
						   : loader_extensions),
				 &driver_configs, psc);

   if (psc->__driScreen == NULL) {
      ErrorMessageF("failed to create dri screen\n");
      goto handle_error;
   }

   driBindCommonExtensions(psc);
   dri2BindExtensions(psc);

   psc->configs = driConvertConfigs(psc->core, psc->configs, driver_configs);
   psc->visuals = driConvertConfigs(psc->core, psc->visuals, driver_configs);

   psc->driver_configs = driver_configs;

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
      __glXEnableDirectExtension(psc, "GLX_OML_sync_control");
#endif
   }

   /* DRI2 suports SubBuffer through DRI2CopyRegion, so it's always
    * available.*/
   psp->copySubBuffer = dri2CopySubBuffer;
   __glXEnableDirectExtension(psc, "GLX_MESA_copy_sub_buffer");

   Xfree(driverName);
   Xfree(deviceName);

   return psp;

handle_error:
   Xfree(driverName);
   Xfree(deviceName);
   XFree(psp);

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

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *
dri2CreateDisplay(Display * dpy)
{
   __GLXDRIdisplayPrivate *pdp;
   int eventBase, errorBase;

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

   return &pdp->base;
}

#endif /* GLX_DIRECT_RENDERING */
