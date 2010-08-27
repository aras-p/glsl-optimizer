/*
 * Copyright 2008 George Sapountzis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)

#include <X11/Xlib.h>
#include "glxclient.h"
#include <dlfcn.h>
#include "dri_common.h"

struct drisw_display
{
   __GLXDRIdisplay base;
};

struct drisw_context
{
   struct glx_context base;
   __DRIcontext *driContext;

};

struct drisw_screen
{
   struct glx_screen base;

   __DRIscreen *driScreen;
   __GLXDRIscreen vtable;
   const __DRIcoreExtension *core;
   const __DRIswrastExtension *swrast;
   const __DRIconfig **driver_configs;

   void *driver;
};

struct drisw_drawable
{
   __GLXDRIdrawable base;

   GC gc;
   GC swapgc;

   __DRIdrawable *driDrawable;
   XVisualInfo *visinfo;
   XImage *ximage;
};

static Bool
XCreateDrawable(struct drisw_drawable * pdp,
                Display * dpy, XID drawable, int visualid)
{
   XGCValues gcvalues;
   long visMask;
   XVisualInfo visTemp;
   int num_visuals;

   /* create GC's */
   pdp->gc = XCreateGC(dpy, drawable, 0, NULL);
   pdp->swapgc = XCreateGC(dpy, drawable, 0, NULL);

   gcvalues.function = GXcopy;
   gcvalues.graphics_exposures = False;
   XChangeGC(dpy, pdp->gc, GCFunction, &gcvalues);
   XChangeGC(dpy, pdp->swapgc, GCFunction, &gcvalues);
   XChangeGC(dpy, pdp->swapgc, GCGraphicsExposures, &gcvalues);

   /* visual */
   visTemp.screen = DefaultScreen(dpy);
   visTemp.visualid = visualid;
   visMask = (VisualScreenMask | VisualIDMask);
   pdp->visinfo = XGetVisualInfo(dpy, visMask, &visTemp, &num_visuals);

   /* create XImage */
   pdp->ximage = XCreateImage(dpy,
                              pdp->visinfo->visual,
                              pdp->visinfo->depth,
                              ZPixmap, 0,             /* format, offset */
                              NULL,                   /* data */
                              0, 0,                   /* width, height */
                              32,                     /* bitmap_pad */
                              0);                     /* bytes_per_line */

   return True;
}

static void
XDestroyDrawable(struct drisw_drawable * pdp, Display * dpy, XID drawable)
{
   XDestroyImage(pdp->ximage);
   XFree(pdp->visinfo);

   XFreeGC(dpy, pdp->gc);
   XFreeGC(dpy, pdp->swapgc);
}

/**
 * swrast loader functions
 */

static void
swrastGetDrawableInfo(__DRIdrawable * draw,
                      int *x, int *y, int *w, int *h,
                      void *loaderPrivate)
{
   struct drisw_drawable *pdp = loaderPrivate;
   __GLXDRIdrawable *pdraw = &(pdp->base);
   Display *dpy = pdraw->psc->dpy;
   Drawable drawable;

   Window root;
   Status stat;
   unsigned uw, uh, bw, depth;

   drawable = pdraw->xDrawable;

   stat = XGetGeometry(dpy, drawable, &root,
                       x, y, &uw, &uh, &bw, &depth);
   *w = uw;
   *h = uh;
}

/**
 * Align renderbuffer pitch.
 *
 * This should be chosen by the driver and the loader (libGL, xserver/glx)
 * should use the driver provided pitch.
 *
 * It seems that the xorg loader (that is the xserver loading swrast_dri for
 * indirect rendering, not client-side libGL) requires that the pitch is
 * exactly the image width padded to 32 bits. XXX
 *
 * The above restriction can probably be overcome by using ScratchPixmap and
 * CopyArea in the xserver, similar to ShmPutImage, and setting the width of
 * the scratch pixmap to 'pitch / cpp'.
 */
static inline int
bytes_per_line(unsigned pitch_bits, unsigned mul)
{
   unsigned mask = mul - 1;

   return ((pitch_bits + mask) & ~mask) / 8;
}

static void
swrastPutImage(__DRIdrawable * draw, int op,
               int x, int y, int w, int h,
               char *data, void *loaderPrivate)
{
   struct drisw_drawable *pdp = loaderPrivate;
   __GLXDRIdrawable *pdraw = &(pdp->base);
   Display *dpy = pdraw->psc->dpy;
   Drawable drawable;
   XImage *ximage;
   GC gc;

   switch (op) {
   case __DRI_SWRAST_IMAGE_OP_DRAW:
      gc = pdp->gc;
      break;
   case __DRI_SWRAST_IMAGE_OP_SWAP:
      gc = pdp->swapgc;
      break;
   default:
      return;
   }

   drawable = pdraw->xDrawable;

   ximage = pdp->ximage;
   ximage->data = data;
   ximage->width = w;
   ximage->height = h;
   ximage->bytes_per_line = bytes_per_line(w * ximage->bits_per_pixel, 32);

   XPutImage(dpy, drawable, gc, ximage, 0, 0, x, y, w, h);

   ximage->data = NULL;
}

static void
swrastGetImage(__DRIdrawable * read,
               int x, int y, int w, int h,
               char *data, void *loaderPrivate)
{
   struct drisw_drawable *prp = loaderPrivate;
   __GLXDRIdrawable *pread = &(prp->base);
   Display *dpy = pread->psc->dpy;
   Drawable readable;
   XImage *ximage;

   readable = pread->xDrawable;

   ximage = prp->ximage;
   ximage->data = data;
   ximage->width = w;
   ximage->height = h;
   ximage->bytes_per_line = bytes_per_line(w * ximage->bits_per_pixel, 32);

   XGetSubImage(dpy, readable, x, y, w, h, ~0L, ZPixmap, ximage, 0, 0);

   ximage->data = NULL;
}

static const __DRIswrastLoaderExtension swrastLoaderExtension = {
   {__DRI_SWRAST_LOADER, __DRI_SWRAST_LOADER_VERSION},
   swrastGetDrawableInfo,
   swrastPutImage,
   swrastGetImage
};

static const __DRIextension *loader_extensions[] = {
   &systemTimeExtension.base,
   &swrastLoaderExtension.base,
   NULL
};

/**
 * GLXDRI functions
 */

static void
drisw_destroy_context(struct glx_context *context)
{
   struct drisw_context *pcp = (struct drisw_context *) context;
   struct drisw_screen *psc = (struct drisw_screen *) context->psc;

   if (context->xid)
      glx_send_destroy_context(psc->base.dpy, context->xid);

   if (context->extensions)
      XFree((char *) context->extensions);

   (*psc->core->destroyContext) (pcp->driContext);

   Xfree(pcp);
}

static int
drisw_bind_context(struct glx_context *context, struct glx_context *old,
		   GLXDrawable draw, GLXDrawable read)
{
   struct drisw_context *pcp = (struct drisw_context *) context;
   struct drisw_screen *psc = (struct drisw_screen *) pcp->base.psc;
   struct drisw_drawable *pdraw, *pread;

   pdraw = (struct drisw_drawable *) driFetchDrawable(context, draw);
   pread = (struct drisw_drawable *) driFetchDrawable(context, read);

   if (pdraw == NULL || pread == NULL)
      return GLXBadDrawable;

   if ((*psc->core->bindContext) (pcp->driContext,
				  pdraw->driDrawable, pread->driDrawable))
      return Success;

   return GLXBadContext;
}

static void
drisw_unbind_context(struct glx_context *context, struct glx_context *new)
{
   struct drisw_context *pcp = (struct drisw_context *) context;
   struct drisw_screen *psc = (struct drisw_screen *) pcp->base.psc;

   (*psc->core->unbindContext) (pcp->driContext);

   driReleaseDrawables(&pcp->base);
}

static const struct glx_context_vtable drisw_context_vtable = {
   drisw_destroy_context,
   drisw_bind_context,
   drisw_unbind_context,
   NULL,
   NULL,
   DRI_glXUseXFont,
   NULL,
   NULL,
};

static struct glx_context *
drisw_create_context(struct glx_screen *base,
		     struct glx_config *config_base,
		     struct glx_context *shareList, int renderType)
{
   struct drisw_context *pcp, *pcp_shared;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   struct drisw_screen *psc = (struct drisw_screen *) base;
   __DRIcontext *shared = NULL;

   if (!psc->base.driScreen)
      return NULL;

   if (shareList) {
      pcp_shared = (struct drisw_context *) shareList;
      shared = pcp_shared->driContext;
   }

   pcp = Xmalloc(sizeof *pcp);
   if (pcp == NULL)
      return NULL;

   memset(pcp, 0, sizeof *pcp);
   if (!glx_context_init(&pcp->base, &psc->base, &config->base)) {
      Xfree(pcp);
      return NULL;
   }

   pcp->driContext =
      (*psc->core->createNewContext) (psc->driScreen,
				      config->driConfig, shared, pcp);
   if (pcp->driContext == NULL) {
      Xfree(pcp);
      return NULL;
   }

   pcp->base.vtable = &drisw_context_vtable;

   return &pcp->base;
}

static void
driDestroyDrawable(__GLXDRIdrawable * pdraw)
{
   struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
   struct drisw_screen *psc = (struct drisw_screen *) pdp->base.psc;

   (*psc->core->destroyDrawable) (pdp->driDrawable);

   XDestroyDrawable(pdp, pdraw->psc->dpy, pdraw->drawable);
   Xfree(pdp);
}

static __GLXDRIdrawable *
driCreateDrawable(struct glx_screen *base, XID xDrawable,
		  GLXDrawable drawable, struct glx_config *modes)
{
   struct drisw_drawable *pdp;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) modes;
   struct drisw_screen *psc = (struct drisw_screen *) base;

   const __DRIswrastExtension *swrast = psc->swrast;

   /* Old dri can't handle GLX 1.3+ drawable constructors. */
   if (xDrawable != drawable)
      return NULL;

   pdp = Xmalloc(sizeof(*pdp));
   if (!pdp)
      return NULL;

   memset(pdp, 0, sizeof *pdp);
   pdp->base.xDrawable = xDrawable;
   pdp->base.drawable = drawable;
   pdp->base.psc = &psc->base;

   XCreateDrawable(pdp, psc->base.dpy, xDrawable, modes->visualID);

   /* Create a new drawable */
   pdp->driDrawable =
      (*swrast->createNewDrawable) (psc->driScreen, config->driConfig, pdp);

   if (!pdp->driDrawable) {
      XDestroyDrawable(pdp, psc->base.dpy, xDrawable);
      Xfree(pdp);
      return NULL;
   }

   pdp->base.destroyDrawable = driDestroyDrawable;

   return &pdp->base;
}

static int64_t
driSwapBuffers(__GLXDRIdrawable * pdraw,
               int64_t target_msc, int64_t divisor, int64_t remainder)
{
   struct drisw_drawable *pdp = (struct drisw_drawable *) pdraw;
   struct drisw_screen *psc = (struct drisw_screen *) pdp->base.psc;

   (void) target_msc;
   (void) divisor;
   (void) remainder;

   (*psc->core->swapBuffers) (pdp->driDrawable);

   return 0;
}

static void
driDestroyScreen(struct glx_screen *base)
{
   struct drisw_screen *psc = (struct drisw_screen *) base;

   /* Free the direct rendering per screen data */
   (*psc->core->destroyScreen) (psc->driScreen);
   driDestroyConfigs(psc->driver_configs);
   psc->driScreen = NULL;
   if (psc->driver)
      dlclose(psc->driver);
}

static void *
driOpenSwrast(void)
{
   void *driver = NULL;

   if (driver == NULL)
      driver = driOpenDriver("swrast");

   if (driver == NULL)
      driver = driOpenDriver("swrastg");

   return driver;
}

static const struct glx_screen_vtable drisw_screen_vtable = {
   drisw_create_context
};

static struct glx_screen *
driCreateScreen(int screen, struct glx_display *priv)
{
   __GLXDRIscreen *psp;
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   struct drisw_screen *psc;
   int i;

   psc = Xcalloc(1, sizeof *psc);
   if (psc == NULL)
      return NULL;

   memset(psc, 0, sizeof *psc);
   if (!glx_screen_init(&psc->base, screen, priv))
       return NULL;

   psc->driver = driOpenSwrast();
   if (psc->driver == NULL)
      goto handle_error;

   extensions = dlsym(psc->driver, __DRI_DRIVER_EXTENSIONS);
   if (extensions == NULL) {
      ErrorMessageF("driver exports no extensions (%s)\n", dlerror());
      goto handle_error;
   }

   for (i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, __DRI_CORE) == 0)
	 psc->core = (__DRIcoreExtension *) extensions[i];
      if (strcmp(extensions[i]->name, __DRI_SWRAST) == 0)
	 psc->swrast = (__DRIswrastExtension *) extensions[i];
   }

   if (psc->core == NULL || psc->swrast == NULL) {
      ErrorMessageF("core dri extension not found\n");
      goto handle_error;
   }

   psc->driScreen =
      psc->swrast->createNewScreen(screen, loader_extensions,
				   &driver_configs, psc);
   if (psc->driScreen == NULL) {
      ErrorMessageF("failed to create dri screen\n");
      goto handle_error;
   }

   psc->base.configs =
      driConvertConfigs(psc->core, psc->base.configs, driver_configs);
   psc->base.visuals =
      driConvertConfigs(psc->core, psc->base.visuals, driver_configs);

   psc->driver_configs = driver_configs;

   psc->base.vtable = &drisw_screen_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = driDestroyScreen;
   psp->createDrawable = driCreateDrawable;
   psp->swapBuffers = driSwapBuffers;

   return &psc->base;

 handle_error:
   if (psc->driver)
      dlclose(psc->driver);
   Xfree(psc);

   ErrorMessageF("reverting to indirect rendering\n");

   return NULL;
}

/* Called from __glXFreeDisplayPrivate.
 */
static void
driDestroyDisplay(__GLXDRIdisplay * dpy)
{
   Xfree(dpy);
}

/*
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer.
 */
_X_HIDDEN __GLXDRIdisplay *
driswCreateDisplay(Display * dpy)
{
   struct drisw_display *pdpyp;

   pdpyp = Xmalloc(sizeof *pdpyp);
   if (pdpyp == NULL)
      return NULL;

   pdpyp->base.destroyDisplay = driDestroyDisplay;
   pdpyp->base.createScreen = driCreateScreen;

   return &pdpyp->base;
}

#endif /* GLX_DIRECT_RENDERING */
