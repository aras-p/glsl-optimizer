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
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/dri2.h>
#include "glxclient.h"
#include <X11/extensions/dri2proto.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include "dri2.h"
#include "dri_common.h"
#include "dri2_priv.h"
#include "loader.h"

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

struct dri2_context
{
   struct glx_context base;
   __DRIcontext *driContext;
};

struct dri2_drawable
{
   __GLXDRIdrawable base;
   __DRIdrawable *driDrawable;
   __DRIbuffer buffers[5];
   int bufferCount;
   int width, height;
   int have_back;
   int have_fake_front;
   int swap_interval;

   uint64_t previous_time;
   unsigned frames;
};

static const struct glx_context_vtable dri2_context_vtable;

/* For XCB's handling of ust/msc/sbc counters, we have to hand it the high and
 * low halves separately.  This helps you split them.
 */
static void
split_counter(uint64_t counter, uint32_t *hi, uint32_t *lo)
{
   *hi = (counter >> 32);
   *lo = counter & 0xffffffff;
}

static uint64_t
merge_counter(uint32_t hi, uint32_t lo)
{
   return ((uint64_t)hi << 32) | lo;
}

static void
dri2_destroy_context(struct glx_context *context)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) context->psc;

   driReleaseDrawables(&pcp->base);

   free((char *) context->extensions);

   (*psc->core->destroyContext) (pcp->driContext);

   free(pcp);
}

static Bool
dri2_bind_context(struct glx_context *context, struct glx_context *old,
		  GLXDrawable draw, GLXDrawable read)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) pcp->base.psc;
   struct dri2_drawable *pdraw, *pread;
   __DRIdrawable *dri_draw = NULL, *dri_read = NULL;
   struct glx_display *dpyPriv = psc->base.display;
   struct dri2_display *pdp;

   pdraw = (struct dri2_drawable *) driFetchDrawable(context, draw);
   pread = (struct dri2_drawable *) driFetchDrawable(context, read);

   driReleaseDrawables(&pcp->base);

   if (pdraw)
      dri_draw = pdraw->driDrawable;
   else if (draw != None)
      return GLXBadDrawable;

   if (pread)
      dri_read = pread->driDrawable;
   else if (read != None)
      return GLXBadDrawable;

   if (!(*psc->core->bindContext) (pcp->driContext, dri_draw, dri_read))
      return GLXBadContext;

   /* If the server doesn't send invalidate events, we may miss a
    * resize before the rendering starts.  Invalidate the buffers now
    * so the driver will recheck before rendering starts. */
   pdp = (struct dri2_display *) dpyPriv->dri2Display;
   if (!pdp->invalidateAvailable && pdraw) {
      dri2InvalidateBuffers(psc->base.dpy, pdraw->base.xDrawable);
      if (pread != pdraw && pread)
	 dri2InvalidateBuffers(psc->base.dpy, pread->base.xDrawable);
   }

   return Success;
}

static void
dri2_unbind_context(struct glx_context *context, struct glx_context *new)
{
   struct dri2_context *pcp = (struct dri2_context *) context;
   struct dri2_screen *psc = (struct dri2_screen *) pcp->base.psc;

   (*psc->core->unbindContext) (pcp->driContext);
}

static struct glx_context *
dri2_create_context(struct glx_screen *base,
		    struct glx_config *config_base,
		    struct glx_context *shareList, int renderType)
{
   struct dri2_context *pcp, *pcp_shared;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   __DRIcontext *shared = NULL;

   /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, renderType))
       return NULL;

   if (shareList) {
      /* If the shareList context is not a DRI2 context, we cannot possibly
       * create a DRI2 context that shares it.
       */
      if (shareList->vtable->destroy != dri2_destroy_context) {
	 return NULL;
      }

      pcp_shared = (struct dri2_context *) shareList;
      shared = pcp_shared->driContext;
   }

   pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL)
      return NULL;

   if (!glx_context_init(&pcp->base, &psc->base, &config->base)) {
      free(pcp);
      return NULL;
   }

   pcp->base.renderType = renderType;

   pcp->driContext =
      (*psc->dri2->createNewContext) (psc->driScreen,
                                      config->driConfig, shared, pcp);

   if (pcp->driContext == NULL) {
      free(pcp);
      return NULL;
   }

   pcp->base.vtable = &dri2_context_vtable;

   return &pcp->base;
}

static struct glx_context *
dri2_create_context_attribs(struct glx_screen *base,
			    struct glx_config *config_base,
			    struct glx_context *shareList,
			    unsigned num_attribs,
			    const uint32_t *attribs,
			    unsigned *error)
{
   struct dri2_context *pcp = NULL;
   struct dri2_context *pcp_shared = NULL;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   __DRIcontext *shared = NULL;

   uint32_t minor_ver;
   uint32_t major_ver;
   uint32_t renderType;
   uint32_t flags;
   unsigned api;
   int reset;
   uint32_t ctx_attribs[2 * 5];
   unsigned num_ctx_attribs = 0;

   if (psc->dri2->base.version < 3) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      goto error_exit;
   }

   /* Remap the GLX tokens to DRI2 tokens.
    */
   if (!dri2_convert_glx_attribs(num_attribs, attribs,
                                 &major_ver, &minor_ver, &renderType, &flags,
                                 &api, &reset, error))
      goto error_exit;

   /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, renderType))
       goto error_exit;

   if (shareList) {
      pcp_shared = (struct dri2_context *) shareList;
      shared = pcp_shared->driContext;
   }

   pcp = calloc(1, sizeof *pcp);
   if (pcp == NULL) {
      *error = __DRI_CTX_ERROR_NO_MEMORY;
      goto error_exit;
   }

   if (!glx_context_init(&pcp->base, &psc->base, &config->base))
      goto error_exit;

   ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = major_ver;
   ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
   ctx_attribs[num_ctx_attribs++] = minor_ver;

   /* Only send a value when the non-default value is requested.  By doing
    * this we don't have to check the driver's DRI2 version before sending the
    * default value.
    */
   if (reset != __DRI_CTX_RESET_NO_NOTIFICATION) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_RESET_STRATEGY;
      ctx_attribs[num_ctx_attribs++] = reset;
   }

   if (flags != 0) {
      ctx_attribs[num_ctx_attribs++] = __DRI_CTX_ATTRIB_FLAGS;

      /* The current __DRI_CTX_FLAG_* values are identical to the
       * GLX_CONTEXT_*_BIT values.
       */
      ctx_attribs[num_ctx_attribs++] = flags;
   }

   /* The renderType is retrieved from attribs, or set to default
    *  of GLX_RGBA_TYPE.
    */
   pcp->base.renderType = renderType;

   pcp->driContext =
      (*psc->dri2->createContextAttribs) (psc->driScreen,
					  api,
					  config->driConfig,
					  shared,
					  num_ctx_attribs / 2,
					  ctx_attribs,
					  error,
					  pcp);

   if (pcp->driContext == NULL)
      goto error_exit;

   pcp->base.vtable = &dri2_context_vtable;

   return &pcp->base;

error_exit:
   free(pcp);

   return NULL;
}

static void
dri2DestroyDrawable(__GLXDRIdrawable *base)
{
   struct dri2_screen *psc = (struct dri2_screen *) base->psc;
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct glx_display *dpyPriv = psc->base.display;
   struct dri2_display *pdp = (struct dri2_display *)dpyPriv->dri2Display;

   __glxHashDelete(pdp->dri2Hash, pdraw->base.xDrawable);
   (*psc->core->destroyDrawable) (pdraw->driDrawable);

   /* If it's a GLX 1.3 drawables, we can destroy the DRI2 drawable
    * now, as the application explicitly asked to destroy the GLX
    * drawable.  Otherwise, for legacy drawables, we let the DRI2
    * drawable linger on the server, since there's no good way of
    * knowing when the application is done with it.  The server will
    * destroy the DRI2 drawable when it destroys the X drawable or the
    * client exits anyway. */
   if (pdraw->base.xDrawable != pdraw->base.drawable)
      DRI2DestroyDrawable(psc->base.dpy, pdraw->base.xDrawable);

   free(pdraw);
}

static __GLXDRIdrawable *
dri2CreateDrawable(struct glx_screen *base, XID xDrawable,
		   GLXDrawable drawable, struct glx_config *config_base)
{
   struct dri2_drawable *pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   struct glx_display *dpyPriv;
   struct dri2_display *pdp;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;

   dpyPriv = __glXInitialize(psc->base.dpy);
   if (dpyPriv == NULL)
      return NULL;

   pdraw = calloc(1, sizeof(*pdraw));
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
   pdp = (struct dri2_display *)dpyPriv->dri2Display;;
   /* Create a new drawable */
   pdraw->driDrawable =
      (*psc->dri2->createNewDrawable) (psc->driScreen,
                                       config->driConfig, pdraw);

   if (!pdraw->driDrawable) {
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
      free(pdraw);
      return NULL;
   }

   if (__glxHashInsert(pdp->dri2Hash, xDrawable, pdraw)) {
      (*psc->core->destroyDrawable) (pdraw->driDrawable);
      DRI2DestroyDrawable(psc->base.dpy, xDrawable);
      free(pdraw);
      return None;
   }

   /*
    * Make sure server has the same swap interval we do for the new
    * drawable.
    */
   if (psc->vtable.setSwapInterval)
      psc->vtable.setSwapInterval(&pdraw->base, pdraw->swap_interval);

   return &pdraw->base;
}

static int
dri2DrawableGetMSC(struct glx_screen *psc, __GLXDRIdrawable *pdraw,
		   int64_t *ust, int64_t *msc, int64_t *sbc)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   xcb_dri2_get_msc_cookie_t get_msc_cookie;
   xcb_dri2_get_msc_reply_t *get_msc_reply;

   get_msc_cookie = xcb_dri2_get_msc_unchecked(c, pdraw->xDrawable);
   get_msc_reply = xcb_dri2_get_msc_reply(c, get_msc_cookie, NULL);

   if (!get_msc_reply)
      return 0;

   *ust = merge_counter(get_msc_reply->ust_hi, get_msc_reply->ust_lo);
   *msc = merge_counter(get_msc_reply->msc_hi, get_msc_reply->msc_lo);
   *sbc = merge_counter(get_msc_reply->sbc_hi, get_msc_reply->sbc_lo);
   free(get_msc_reply);

   return 1;
}

static int
dri2WaitForMSC(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
	       int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   xcb_dri2_wait_msc_cookie_t wait_msc_cookie;
   xcb_dri2_wait_msc_reply_t *wait_msc_reply;
   uint32_t target_msc_hi, target_msc_lo;
   uint32_t divisor_hi, divisor_lo;
   uint32_t remainder_hi, remainder_lo;

   split_counter(target_msc, &target_msc_hi, &target_msc_lo);
   split_counter(divisor, &divisor_hi, &divisor_lo);
   split_counter(remainder, &remainder_hi, &remainder_lo);

   wait_msc_cookie = xcb_dri2_wait_msc_unchecked(c, pdraw->xDrawable,
                                                 target_msc_hi, target_msc_lo,
                                                 divisor_hi, divisor_lo,
                                                 remainder_hi, remainder_lo);
   wait_msc_reply = xcb_dri2_wait_msc_reply(c, wait_msc_cookie, NULL);

   if (!wait_msc_reply)
      return 0;

   *ust = merge_counter(wait_msc_reply->ust_hi, wait_msc_reply->ust_lo);
   *msc = merge_counter(wait_msc_reply->msc_hi, wait_msc_reply->msc_lo);
   *sbc = merge_counter(wait_msc_reply->sbc_hi, wait_msc_reply->sbc_lo);
   free(wait_msc_reply);

   return 1;
}

static int
dri2WaitForSBC(__GLXDRIdrawable *pdraw, int64_t target_sbc, int64_t *ust,
	       int64_t *msc, int64_t *sbc)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   xcb_dri2_wait_sbc_cookie_t wait_sbc_cookie;
   xcb_dri2_wait_sbc_reply_t *wait_sbc_reply;
   uint32_t target_sbc_hi, target_sbc_lo;

   split_counter(target_sbc, &target_sbc_hi, &target_sbc_lo);

   wait_sbc_cookie = xcb_dri2_wait_sbc_unchecked(c, pdraw->xDrawable,
                                                 target_sbc_hi, target_sbc_lo);
   wait_sbc_reply = xcb_dri2_wait_sbc_reply(c, wait_sbc_cookie, NULL);

   if (!wait_sbc_reply)
      return 0;

   *ust = merge_counter(wait_sbc_reply->ust_hi, wait_sbc_reply->ust_lo);
   *msc = merge_counter(wait_sbc_reply->msc_hi, wait_sbc_reply->msc_lo);
   *sbc = merge_counter(wait_sbc_reply->sbc_hi, wait_sbc_reply->sbc_lo);
   free(wait_sbc_reply);

   return 1;
}

static __DRIcontext *
dri2GetCurrentContext()
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_context *dri2Ctx = (struct dri2_context *)gc;

   return dri2Ctx ? dri2Ctx->driContext : NULL;
}

/**
 * dri2Throttle - Request driver throttling
 *
 * This function uses the DRI2 throttle extension to give the
 * driver the opportunity to throttle on flush front, copysubbuffer
 * and swapbuffers.
 */
static void
dri2Throttle(struct dri2_screen *psc,
	     struct dri2_drawable *draw,
	     enum __DRI2throttleReason reason)
{
   if (psc->throttle) {
      __DRIcontext *ctx = dri2GetCurrentContext();

      psc->throttle->throttle(ctx, draw->driDrawable, reason);
   }
}

/**
 * Asks the driver to flush any queued work necessary for serializing with the
 * X command stream, and optionally the slightly more strict requirement of
 * glFlush() equivalence (which would require flushing even if nothing had
 * been drawn to a window system framebuffer, for example).
 */
static void
dri2Flush(struct dri2_screen *psc,
          __DRIcontext *ctx,
          struct dri2_drawable *draw,
          unsigned flags,
          enum __DRI2throttleReason throttle_reason)
{
   if (ctx && psc->f && psc->f->base.version >= 4) {
      psc->f->flush_with_flags(ctx, draw->driDrawable, flags, throttle_reason);
   } else {
      if (flags & __DRI2_FLUSH_CONTEXT)
         glFlush();

      if (psc->f)
         psc->f->flush(draw->driDrawable);

      dri2Throttle(psc, draw, throttle_reason);
   }
}

static void
__dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y,
		    int width, int height,
		    enum __DRI2throttleReason reason, Bool flush)
{
   struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
   struct dri2_screen *psc = (struct dri2_screen *) pdraw->psc;
   XRectangle xrect;
   XserverRegion region;
   __DRIcontext *ctx = dri2GetCurrentContext();
   unsigned flags;

   /* Check we have the right attachments */
   if (!priv->have_back)
      return;

   xrect.x = x;
   xrect.y = priv->height - y - height;
   xrect.width = width;
   xrect.height = height;

   flags = __DRI2_FLUSH_DRAWABLE;
   if (flush)
      flags |= __DRI2_FLUSH_CONTEXT;
   dri2Flush(psc, ctx, priv, flags, __DRI2_THROTTLE_SWAPBUFFER);

   region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
                  DRI2BufferFrontLeft, DRI2BufferBackLeft);

   /* Refresh the fake front (if present) after we just damaged the real
    * front.
    */
   if (priv->have_fake_front)
      DRI2CopyRegion(psc->base.dpy, pdraw->xDrawable, region,
		     DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);

   XFixesDestroyRegion(psc->base.dpy, region);
}

static void
dri2CopySubBuffer(__GLXDRIdrawable *pdraw, int x, int y,
		  int width, int height, Bool flush)
{
   __dri2CopySubBuffer(pdraw, x, y, width, height,
		       __DRI2_THROTTLE_COPYSUBBUFFER, flush);
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

   if (psc->f)
      (*psc->f->flush) (priv->driDrawable);

   region = XFixesCreateRegion(psc->base.dpy, &xrect, 1);
   DRI2CopyRegion(psc->base.dpy, priv->base.xDrawable, region, dest, src);
   XFixesDestroyRegion(psc->base.dpy, region);

}

static void
dri2_wait_x(struct glx_context *gc)
{
   struct dri2_drawable *priv = (struct dri2_drawable *)
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);

   if (priv == NULL || !priv->have_fake_front)
      return;

   dri2_copy_drawable(priv, DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);
}

static void
dri2_wait_gl(struct glx_context *gc)
{
   struct dri2_drawable *priv = (struct dri2_drawable *)
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);

   if (priv == NULL || !priv->have_fake_front)
      return;

   dri2_copy_drawable(priv, DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
}

/**
 * Called by the driver when it needs to update the real front buffer with the
 * contents of its fake front buffer.
 */
static void
dri2FlushFrontBuffer(__DRIdrawable *driDrawable, void *loaderPrivate)
{
   struct glx_display *priv;
   struct dri2_display *pdp;
   struct glx_context *gc;
   struct dri2_drawable *pdraw = loaderPrivate;
   struct dri2_screen *psc;

   if (!pdraw)
      return;

   if (!pdraw->base.psc)
      return;

   psc = (struct dri2_screen *) pdraw->base.psc;

   priv = __glXInitialize(psc->base.dpy);

   if (priv == NULL)
       return;

   pdp = (struct dri2_display *) priv->dri2Display;
   gc = __glXGetCurrentContext();

   dri2Throttle(psc, pdraw, __DRI2_THROTTLE_FLUSHFRONT);

   /* Old servers don't send invalidate events */
   if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(priv->dpy, pdraw->base.xDrawable);

   dri2_wait_gl(gc);
}


static void
dri2DestroyScreen(struct glx_screen *base)
{
   struct dri2_screen *psc = (struct dri2_screen *) base;

   /* Free the direct rendering per screen data */
   (*psc->core->destroyScreen) (psc->driScreen);
   driDestroyConfigs(psc->driver_configs);
   close(psc->fd);
   free(psc);
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

unsigned dri2GetSwapEventType(Display* dpy, XID drawable)
{
      struct glx_display *glx_dpy = __glXInitialize(dpy);
      __GLXDRIdrawable *pdraw;
      pdraw = dri2GetGlxDrawableFromXDrawableId(dpy, drawable);
      if (!pdraw || !(pdraw->eventMask & GLX_BUFFER_SWAP_COMPLETE_INTEL_MASK))
         return 0;
      return glx_dpy->codes->first_event + GLX_BufferSwapComplete;
}

static void show_fps(struct dri2_drawable *draw)
{
   const int interval =
      ((struct dri2_screen *) draw->base.psc)->show_fps_interval;
   struct timeval tv;
   uint64_t current_time;

   gettimeofday(&tv, 0);
   current_time = (uint64_t)tv.tv_sec*1000000 + (uint64_t)tv.tv_usec;

   draw->frames++;

   if (draw->previous_time + interval * 1000000 <= current_time) {
      if (draw->previous_time) {
         fprintf(stderr, "libGL: FPS = %.1f\n",
                 ((uint64_t)draw->frames * 1000000) /
                 (double)(current_time - draw->previous_time));
      }
      draw->frames = 0;
      draw->previous_time = current_time;
   }
}

static int64_t
dri2XcbSwapBuffers(Display *dpy,
                  __GLXDRIdrawable *pdraw,
                  int64_t target_msc,
                  int64_t divisor,
                  int64_t remainder)
{
   xcb_dri2_swap_buffers_cookie_t swap_buffers_cookie;
   xcb_dri2_swap_buffers_reply_t *swap_buffers_reply;
   uint32_t target_msc_hi, target_msc_lo;
   uint32_t divisor_hi, divisor_lo;
   uint32_t remainder_hi, remainder_lo;
   int64_t ret = 0;
   xcb_connection_t *c = XGetXCBConnection(dpy);

   split_counter(target_msc, &target_msc_hi, &target_msc_lo);
   split_counter(divisor, &divisor_hi, &divisor_lo);
   split_counter(remainder, &remainder_hi, &remainder_lo);

   swap_buffers_cookie =
      xcb_dri2_swap_buffers_unchecked(c, pdraw->xDrawable,
                                      target_msc_hi, target_msc_lo,
                                      divisor_hi, divisor_lo,
                                      remainder_hi, remainder_lo);

   /* Immediately wait on the swapbuffers reply.  If we didn't, we'd have
    * to do so some time before reusing a (non-pageflipped) backbuffer.
    * Otherwise, the new rendering could get ahead of the X Server's
    * dispatch of the swapbuffer and you'd display garbage.
    *
    * We use XSync() first to reap the invalidate events through the event
    * filter, to ensure that the next drawing doesn't use an invalidated
    * buffer.
    */
   XSync(dpy, False);

   swap_buffers_reply =
      xcb_dri2_swap_buffers_reply(c, swap_buffers_cookie, NULL);
   if (swap_buffers_reply) {
      ret = merge_counter(swap_buffers_reply->swap_hi,
                          swap_buffers_reply->swap_lo);
      free(swap_buffers_reply);
   }
   return ret;
}

static int64_t
dri2SwapBuffers(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
		int64_t remainder, Bool flush)
{
    struct dri2_drawable *priv = (struct dri2_drawable *) pdraw;
    struct glx_display *dpyPriv = __glXInitialize(priv->base.psc->dpy);
    struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;
    struct dri2_display *pdp =
	(struct dri2_display *)dpyPriv->dri2Display;
    int64_t ret = 0;

    /* Check we have the right attachments */
    if (!priv->have_back)
	return ret;

    /* Old servers can't handle swapbuffers */
    if (!pdp->swapAvailable) {
       __dri2CopySubBuffer(pdraw, 0, 0, priv->width, priv->height,
			   __DRI2_THROTTLE_SWAPBUFFER, flush);
    } else {
       __DRIcontext *ctx = dri2GetCurrentContext();
       unsigned flags = __DRI2_FLUSH_DRAWABLE;
       if (flush)
          flags |= __DRI2_FLUSH_CONTEXT;
       dri2Flush(psc, ctx, priv, flags, __DRI2_THROTTLE_SWAPBUFFER);

       ret = dri2XcbSwapBuffers(pdraw->psc->dpy, pdraw,
                                target_msc, divisor, remainder);
    }

    if (psc->show_fps_interval) {
       show_fps(priv);
    }

    /* Old servers don't send invalidate events */
    if (!pdp->invalidateAvailable)
       dri2InvalidateBuffers(dpyPriv->dpy, pdraw->xDrawable);

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

   free(buffers);

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

   free(buffers);

   return pdraw->buffers;
}

static int
dri2SetSwapInterval(__GLXDRIdrawable *pdraw, int interval)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;
   struct dri2_screen *psc = (struct dri2_screen *) priv->base.psc;

   if (psc->config)
      psc->config->configQueryi(psc->driScreen,
				"vblank_mode", &vblank_mode);

   switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
      if (interval != 0)
         return GLX_BAD_VALUE;
      break;
   case DRI_CONF_VBLANK_ALWAYS_SYNC:
      if (interval <= 0)
	 return GLX_BAD_VALUE;
      break;
   default:
      break;
   }

   xcb_dri2_swap_interval(c, priv->base.xDrawable, interval);
   priv->swap_interval = interval;

   return 0;
}

static int
dri2GetSwapInterval(__GLXDRIdrawable *pdraw)
{
   struct dri2_drawable *priv =  (struct dri2_drawable *) pdraw;

  return priv->swap_interval;
}

static const __DRIdri2LoaderExtension dri2LoaderExtension = {
   .base = { __DRI_DRI2_LOADER, 3 },

   .getBuffers              = dri2GetBuffers,
   .flushFrontBuffer        = dri2FlushFrontBuffer,
   .getBuffersWithFormat    = dri2GetBuffersWithFormat,
};

static const __DRIdri2LoaderExtension dri2LoaderExtension_old = {
   .base = { __DRI_DRI2_LOADER, 3 },

   .getBuffers              = dri2GetBuffers,
   .flushFrontBuffer        = dri2FlushFrontBuffer,
   .getBuffersWithFormat    = NULL,
};

static const __DRIuseInvalidateExtension dri2UseInvalidate = {
   .base = { __DRI_USE_INVALIDATE, 1 }
};

_X_HIDDEN void
dri2InvalidateBuffers(Display *dpy, XID drawable)
{
   __GLXDRIdrawable *pdraw =
      dri2GetGlxDrawableFromXDrawableId(dpy, drawable);
   struct dri2_screen *psc;
   struct dri2_drawable *pdp = (struct dri2_drawable *) pdraw;

   if (!pdraw)
      return;

   psc = (struct dri2_screen *) pdraw->psc;

   if (pdraw && psc->f && psc->f->base.version >= 3 && psc->f->invalidate)
       psc->f->invalidate(pdp->driDrawable);
}

static void
dri2_bind_tex_image(Display * dpy,
		    GLXDrawable drawable,
		    int buffer, const int *attrib_list)
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_context *pcp = (struct dri2_context *) gc;
   __GLXDRIdrawable *base = GetGLXDRIDrawable(dpy, drawable);
   struct glx_display *dpyPriv = __glXInitialize(dpy);
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct dri2_display *pdp;
   struct dri2_screen *psc;

   if (dpyPriv == NULL)
       return;

   pdp = (struct dri2_display *) dpyPriv->dri2Display;

   if (pdraw != NULL) {
      psc = (struct dri2_screen *) base->psc;

      if (!pdp->invalidateAvailable && psc->f &&
           psc->f->base.version >= 3 && psc->f->invalidate)
	 psc->f->invalidate(pdraw->driDrawable);

      if (psc->texBuffer->base.version >= 2 &&
	  psc->texBuffer->setTexBuffer2 != NULL) {
	 (*psc->texBuffer->setTexBuffer2) (pcp->driContext,
					   pdraw->base.textureTarget,
					   pdraw->base.textureFormat,
					   pdraw->driDrawable);
      }
      else {
	 (*psc->texBuffer->setTexBuffer) (pcp->driContext,
					  pdraw->base.textureTarget,
					  pdraw->driDrawable);
      }
   }
}

static void
dri2_release_tex_image(Display * dpy, GLXDrawable drawable, int buffer)
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri2_context *pcp = (struct dri2_context *) gc;
   __GLXDRIdrawable *base = GetGLXDRIDrawable(dpy, drawable);
   struct glx_display *dpyPriv = __glXInitialize(dpy);
   struct dri2_drawable *pdraw = (struct dri2_drawable *) base;
   struct dri2_screen *psc;

   if (dpyPriv != NULL && pdraw != NULL) {
      psc = (struct dri2_screen *) base->psc;

      if (psc->texBuffer->base.version >= 3 &&
          psc->texBuffer->releaseTexBuffer != NULL) {
         (*psc->texBuffer->releaseTexBuffer) (pcp->driContext,
                                           pdraw->base.textureTarget,
                                           pdraw->driDrawable);
      }
   }
}

static const struct glx_context_vtable dri2_context_vtable = {
   .destroy             = dri2_destroy_context,
   .bind                = dri2_bind_context,
   .unbind              = dri2_unbind_context,
   .wait_gl             = dri2_wait_gl,
   .wait_x              = dri2_wait_x,
   .use_x_font          = DRI_glXUseXFont,
   .bind_tex_image      = dri2_bind_tex_image,
   .release_tex_image   = dri2_release_tex_image,
   .get_proc_address    = NULL,
};

static void
dri2BindExtensions(struct dri2_screen *psc, struct glx_display * priv,
                   const char *driverName)
{
   const struct dri2_display *const pdp = (struct dri2_display *)
      priv->dri2Display;
   const __DRIextension **extensions;
   int i;

   extensions = psc->core->getExtensions(psc->driScreen);

   __glXEnableDirectExtension(&psc->base, "GLX_SGI_video_sync");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_make_current_read");

   /*
    * GLX_INTEL_swap_event is broken on the server side, where it's
    * currently unconditionally enabled. This completely breaks
    * systems running on drivers which don't support that extension.
    * There's no way to test for its presence on this side, so instead
    * of disabling it unconditionally, just disable it for drivers
    * which are known to not support it, or for DDX drivers supporting
    * only an older (pre-ScheduleSwap) version of DRI2.
    *
    * This is a hack which is required until:
    * http://lists.x.org/archives/xorg-devel/2013-February/035449.html
    * is merged and updated xserver makes it's way into distros:
    */
   if (pdp->swapAvailable && strcmp(driverName, "vmwgfx") != 0) {
      __glXEnableDirectExtension(&psc->base, "GLX_INTEL_swap_event");
   }

   if (psc->dri2->base.version >= 3) {
      const unsigned mask = psc->dri2->getAPIMask(psc->driScreen);

      __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context");
      __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_profile");

      if ((mask & (1 << __DRI_API_GLES2)) != 0)
	 __glXEnableDirectExtension(&psc->base,
				    "GLX_EXT_create_context_es2_profile");
   }

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

      if (((strcmp(extensions[i]->name, __DRI2_THROTTLE) == 0)))
	 psc->throttle = (__DRI2throttleExtension *) extensions[i];

      /* DRI2 version 3 is also required because
       * GLX_ARB_create_context_robustness requires GLX_ARB_create_context.
       */
      if (psc->dri2->base.version >= 3
          && strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0)
         __glXEnableDirectExtension(&psc->base,
                                    "GLX_ARB_create_context_robustness");

      /* DRI2 version 3 is also required because GLX_MESA_query_renderer
       * requires GLX_ARB_create_context_profile.
       */
      if (psc->dri2->base.version >= 3
          && strcmp(extensions[i]->name, __DRI2_RENDERER_QUERY) == 0) {
         psc->rendererQuery = (__DRI2rendererQueryExtension *) extensions[i];
         __glXEnableDirectExtension(&psc->base, "GLX_MESA_query_renderer");
      }
   }
}

static const struct glx_screen_vtable dri2_screen_vtable = {
   .create_context         = dri2_create_context,
   .create_context_attribs = dri2_create_context_attribs,
   .query_renderer_integer = dri2_query_renderer_integer,
   .query_renderer_string  = dri2_query_renderer_string,
};

static struct glx_screen *
dri2CreateScreen(int screen, struct glx_display * priv)
{
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   const struct dri2_display *const pdp = (struct dri2_display *)
      priv->dri2Display;
   struct dri2_screen *psc;
   __GLXDRIscreen *psp;
   struct glx_config *configs = NULL, *visuals = NULL;
   char *driverName = NULL, *loader_driverName, *deviceName, *tmp;
   drm_magic_t magic;
   int i;

   psc = calloc(1, sizeof *psc);
   if (psc == NULL)
      return NULL;

   psc->fd = -1;

   if (!glx_screen_init(&psc->base, screen, priv)) {
      free(psc);
      return NULL;
   }

   if (!DRI2Connect(priv->dpy, RootWindow(priv->dpy, screen),
		    &driverName, &deviceName)) {
      glx_screen_cleanup(&psc->base);
      free(psc);
      InfoMessageF("screen %d does not appear to be DRI2 capable\n", screen);
      return NULL;
   }

#ifdef O_CLOEXEC
   psc->fd = open(deviceName, O_RDWR | O_CLOEXEC);
   if (psc->fd == -1 && errno == EINVAL)
#endif
   {
      psc->fd = open(deviceName, O_RDWR);
      if (psc->fd != -1)
         fcntl(psc->fd, F_SETFD, fcntl(psc->fd, F_GETFD) | FD_CLOEXEC);
   }
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

   /* If Mesa knows about the appropriate driver for this fd, then trust it.
    * Otherwise, default to the server's value.
    */
   loader_driverName = loader_get_driver_for_fd(psc->fd, 0);
   if (loader_driverName) {
      free(driverName);
      driverName = loader_driverName;
   }

   psc->driver = driOpenDriver(driverName);
   if (psc->driver == NULL) {
      ErrorMessageF("driver pointer missing\n");
      goto handle_error;
   }

   extensions = driGetDriverExtensions(psc->driver, driverName);
   if (extensions == NULL)
      goto handle_error;

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

   if (psc->dri2->base.version >= 4) {
      psc->driScreen =
         psc->dri2->createNewScreen2(screen, psc->fd,
                                     (const __DRIextension **)
                                     &pdp->loader_extensions[0],
                                     extensions,
                                     &driver_configs, psc);
   } else {
      psc->driScreen =
         psc->dri2->createNewScreen(screen, psc->fd,
                                    (const __DRIextension **)
                                    &pdp->loader_extensions[0],
                                    &driver_configs, psc);
   }

   if (psc->driScreen == NULL) {
      ErrorMessageF("failed to create dri screen\n");
      goto handle_error;
   }

   dri2BindExtensions(psc, priv, driverName);

   configs = driConvertConfigs(psc->core, psc->base.configs, driver_configs);
   visuals = driConvertConfigs(psc->core, psc->base.visuals, driver_configs);

   if (!configs || !visuals)
       goto handle_error;

   glx_config_destroy_list(psc->base.configs);
   psc->base.configs = configs;
   glx_config_destroy_list(psc->base.visuals);
   psc->base.visuals = visuals;

   psc->driver_configs = driver_configs;

   psc->base.vtable = &dri2_screen_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = dri2DestroyScreen;
   psp->createDrawable = dri2CreateDrawable;
   psp->swapBuffers = dri2SwapBuffers;
   psp->getDrawableMSC = NULL;
   psp->waitForMSC = NULL;
   psp->waitForSBC = NULL;
   psp->setSwapInterval = NULL;
   psp->getSwapInterval = NULL;
   psp->getBufferAge = NULL;

   if (pdp->driMinor >= 2) {
      psp->getDrawableMSC = dri2DrawableGetMSC;
      psp->waitForMSC = dri2WaitForMSC;
      psp->waitForSBC = dri2WaitForSBC;
      psp->setSwapInterval = dri2SetSwapInterval;
      psp->getSwapInterval = dri2GetSwapInterval;
      __glXEnableDirectExtension(&psc->base, "GLX_OML_sync_control");
   }

   /* DRI2 suports SubBuffer through DRI2CopyRegion, so it's always
    * available.*/
   psp->copySubBuffer = dri2CopySubBuffer;
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_copy_sub_buffer");

   free(driverName);
   free(deviceName);

   tmp = getenv("LIBGL_SHOW_FPS");
   psc->show_fps_interval = (tmp) ? atoi(tmp) : 0;
   if (psc->show_fps_interval < 0)
      psc->show_fps_interval = 0;

   return &psc->base;

handle_error:
   CriticalErrorMessageF("failed to load driver: %s\n", driverName);

   if (configs)
       glx_config_destroy_list(configs);
   if (visuals)
       glx_config_destroy_list(visuals);
   if (psc->driScreen)
       psc->core->destroyScreen(psc->driScreen);
   psc->driScreen = NULL;
   if (psc->fd >= 0)
      close(psc->fd);
   if (psc->driver)
      dlclose(psc->driver);

   free(driverName);
   free(deviceName);
   glx_screen_cleanup(&psc->base);
   free(psc);

   return NULL;
}

/* Called from __glXFreeDisplayPrivate.
 */
static void
dri2DestroyDisplay(__GLXDRIdisplay * dpy)
{
   struct dri2_display *pdp = (struct dri2_display *) dpy;

   __glxHashDestroy(pdp->dri2Hash);
   free(dpy);
}

_X_HIDDEN __GLXDRIdrawable *
dri2GetGlxDrawableFromXDrawableId(Display *dpy, XID id)
{
   struct glx_display *d = __glXInitialize(dpy);
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

   pdp = malloc(sizeof *pdp);
   if (pdp == NULL)
      return NULL;

   if (!DRI2QueryVersion(dpy, &pdp->driMajor, &pdp->driMinor)) {
      free(pdp);
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

   pdp->loader_extensions[i++] = &dri2UseInvalidate.base;

   pdp->loader_extensions[i++] = NULL;

   pdp->dri2Hash = __glxHashCreate();
   if (pdp->dri2Hash == NULL) {
      free(pdp);
      return NULL;
   }

   return &pdp->base;
}

#endif /* GLX_DIRECT_RENDERING */
