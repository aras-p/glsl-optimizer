/*
 * Copyright © 2013 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * Portions of this code were adapted from dri2_glx.c which carries the
 * following copyright:
 *
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
#include <X11/xshmfence.h>
#include <xcb/xcb.h>
#include <xcb/dri3.h>
#include <xcb/present.h>
#include <GL/gl.h>
#include "glxclient.h"
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "dri_common.h"
#include "dri3_priv.h"
#include "loader.h"
#include "dri2.h"

static const struct glx_context_vtable dri3_context_vtable;

static inline void
dri3_fence_reset(xcb_connection_t *c, struct dri3_buffer *buffer)
{
   xshmfence_reset(buffer->shm_fence);
}

static inline void
dri3_fence_set(struct dri3_buffer *buffer)
{
   xshmfence_trigger(buffer->shm_fence);
}

static inline void
dri3_fence_trigger(xcb_connection_t *c, struct dri3_buffer *buffer)
{
   xcb_sync_trigger_fence(c, buffer->sync_fence);
}

static inline void
dri3_fence_await(xcb_connection_t *c, struct dri3_buffer *buffer)
{
   xcb_flush(c);
   xshmfence_await(buffer->shm_fence);
}

static inline Bool
dri3_fence_triggered(struct dri3_buffer *buffer)
{
   return xshmfence_query(buffer->shm_fence);
}

static void
dri3_destroy_context(struct glx_context *context)
{
   struct dri3_context *pcp = (struct dri3_context *) context;
   struct dri3_screen *psc = (struct dri3_screen *) context->psc;

   driReleaseDrawables(&pcp->base);

   free((char *) context->extensions);

   (*psc->core->destroyContext) (pcp->driContext);

   free(pcp);
}

static Bool
dri3_bind_context(struct glx_context *context, struct glx_context *old,
                  GLXDrawable draw, GLXDrawable read)
{
   struct dri3_context *pcp = (struct dri3_context *) context;
   struct dri3_screen *psc = (struct dri3_screen *) pcp->base.psc;
   struct dri3_drawable *pdraw, *pread;

   pdraw = (struct dri3_drawable *) driFetchDrawable(context, draw);
   pread = (struct dri3_drawable *) driFetchDrawable(context, read);

   driReleaseDrawables(&pcp->base);

   if (pdraw == NULL || pread == NULL)
      return GLXBadDrawable;

   if (!(*psc->core->bindContext) (pcp->driContext,
                                   pdraw->driDrawable, pread->driDrawable))
      return GLXBadContext;

   return Success;
}

static void
dri3_unbind_context(struct glx_context *context, struct glx_context *new)
{
   struct dri3_context *pcp = (struct dri3_context *) context;
   struct dri3_screen *psc = (struct dri3_screen *) pcp->base.psc;

   (*psc->core->unbindContext) (pcp->driContext);
}

static struct glx_context *
dri3_create_context_attribs(struct glx_screen *base,
                            struct glx_config *config_base,
                            struct glx_context *shareList,
                            unsigned num_attribs,
                            const uint32_t *attribs,
                            unsigned *error)
{
   struct dri3_context *pcp = NULL;
   struct dri3_context *pcp_shared = NULL;
   struct dri3_screen *psc = (struct dri3_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   __DRIcontext *shared = NULL;

   uint32_t minor_ver = 1;
   uint32_t major_ver = 2;
   uint32_t flags = 0;
   unsigned api;
   int reset = __DRI_CTX_RESET_NO_NOTIFICATION;
   uint32_t ctx_attribs[2 * 5];
   unsigned num_ctx_attribs = 0;
   uint32_t render_type;

   /* Remap the GLX tokens to DRI2 tokens.
    */
   if (!dri2_convert_glx_attribs(num_attribs, attribs,
                                 &major_ver, &minor_ver,
                                 &render_type, &flags, &api,
                                 &reset, error))
      goto error_exit;

   /* Check the renderType value */
   if (!validate_renderType_against_config(config_base, render_type))
       goto error_exit;

   if (shareList) {
      pcp_shared = (struct dri3_context *) shareList;
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
    * this we don't have to check the driver's DRI3 version before sending the
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

   pcp->driContext =
      (*psc->image_driver->createContextAttribs) (psc->driScreen,
                                                  api,
                                                  config->driConfig,
                                                  shared,
                                                  num_ctx_attribs / 2,
                                                  ctx_attribs,
                                                  error,
                                                  pcp);

   if (pcp->driContext == NULL)
      goto error_exit;

   pcp->base.vtable = &dri3_context_vtable;

   return &pcp->base;

error_exit:
   free(pcp);

   return NULL;
}

static struct glx_context *
dri3_create_context(struct glx_screen *base,
                    struct glx_config *config_base,
                    struct glx_context *shareList, int renderType)
{
   unsigned int error;

   return dri3_create_context_attribs(base, config_base, shareList,
                                      0, NULL, &error);
}

static void
dri3_free_render_buffer(struct dri3_drawable *pdraw, struct dri3_buffer *buffer);

static void
dri3_update_num_back(struct dri3_drawable *priv)
{
   priv->num_back = 1;
   if (priv->flipping)
      priv->num_back++;
   if (priv->swap_interval == 0)
      priv->num_back++;
}

static void
dri3_destroy_drawable(__GLXDRIdrawable *base)
{
   struct dri3_screen *psc = (struct dri3_screen *) base->psc;
   struct dri3_drawable *pdraw = (struct dri3_drawable *) base;
   xcb_connection_t     *c = XGetXCBConnection(pdraw->base.psc->dpy);
   int i;

   (*psc->core->destroyDrawable) (pdraw->driDrawable);

   for (i = 0; i < DRI3_NUM_BUFFERS; i++) {
      if (pdraw->buffers[i])
         dri3_free_render_buffer(pdraw, pdraw->buffers[i]);
   }

   if (pdraw->special_event)
      xcb_unregister_for_special_event(c, pdraw->special_event);
   free(pdraw);
}

static __GLXDRIdrawable *
dri3_create_drawable(struct glx_screen *base, XID xDrawable,
                     GLXDrawable drawable, struct glx_config *config_base)
{
   struct dri3_drawable *pdraw;
   struct dri3_screen *psc = (struct dri3_screen *) base;
   __GLXDRIconfigPrivate *config = (__GLXDRIconfigPrivate *) config_base;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;

   pdraw = calloc(1, sizeof(*pdraw));
   if (!pdraw)
      return NULL;

   pdraw->base.destroyDrawable = dri3_destroy_drawable;
   pdraw->base.xDrawable = xDrawable;
   pdraw->base.drawable = drawable;
   pdraw->base.psc = &psc->base;
   pdraw->swap_interval = 1; /* default may be overridden below */
   pdraw->have_back = 0;
   pdraw->have_fake_front = 0;

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

   dri3_update_num_back(pdraw);

   (void) __glXInitialize(psc->base.dpy);

   /* Create a new drawable */
   pdraw->driDrawable =
      (*psc->image_driver->createNewDrawable) (psc->driScreen,
                                               config->driConfig, pdraw);

   if (!pdraw->driDrawable) {
      free(pdraw);
      return NULL;
   }

   /*
    * Make sure server has the same swap interval we do for the new
    * drawable.
    */
   if (psc->vtable.setSwapInterval)
      psc->vtable.setSwapInterval(&pdraw->base, pdraw->swap_interval);

   return &pdraw->base;
}

/*
 * Process one Present event
 */
static void
dri3_handle_present_event(struct dri3_drawable *priv, xcb_present_generic_event_t *ge)
{
   switch (ge->evtype) {
   case XCB_PRESENT_CONFIGURE_NOTIFY: {
      xcb_present_configure_notify_event_t *ce = (void *) ge;

      priv->width = ce->width;
      priv->height = ce->height;
      break;
   }
   case XCB_PRESENT_COMPLETE_NOTIFY: {
      xcb_present_complete_notify_event_t *ce = (void *) ge;

      /* Compute the processed SBC number from the received 32-bit serial number merged
       * with the upper 32-bits of the sent 64-bit serial number while checking for
       * wrap
       */
      if (ce->kind == XCB_PRESENT_COMPLETE_KIND_PIXMAP) {
         priv->recv_sbc = (priv->send_sbc & 0xffffffff00000000LL) | ce->serial;
         if (priv->recv_sbc > priv->send_sbc)
            priv->recv_sbc -= 0x100000000;
         switch (ce->mode) {
         case XCB_PRESENT_COMPLETE_MODE_FLIP:
            priv->flipping = true;
            break;
         case XCB_PRESENT_COMPLETE_MODE_COPY:
            priv->flipping = false;
            break;
         }
         dri3_update_num_back(priv);
      } else {
         priv->recv_msc_serial = ce->serial;
      }
      priv->ust = ce->ust;
      priv->msc = ce->msc;
      break;
   }
   case XCB_PRESENT_EVENT_IDLE_NOTIFY: {
      xcb_present_idle_notify_event_t *ie = (void *) ge;
      int b;

      for (b = 0; b < sizeof (priv->buffers) / sizeof (priv->buffers[0]); b++) {
         struct dri3_buffer        *buf = priv->buffers[b];

         if (buf && buf->pixmap == ie->pixmap) {
            buf->busy = 0;
            if (priv->num_back <= b && b < DRI3_MAX_BACK) {
               dri3_free_render_buffer(priv, buf);
               priv->buffers[b] = NULL;
            }
            break;
         }
      }
      break;
   }
   }
   free(ge);
}

static bool
dri3_wait_for_event(__GLXDRIdrawable *pdraw)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   struct dri3_drawable *priv = (struct dri3_drawable *) pdraw;
   xcb_generic_event_t *ev;
   xcb_present_generic_event_t *ge;

   xcb_flush(c);
   ev = xcb_wait_for_special_event(c, priv->special_event);
   if (!ev)
      return false;
   ge = (void *) ev;
   dri3_handle_present_event(priv, ge);
   return true;
}

/** dri3_wait_for_msc
 *
 * Get the X server to send an event when the target msc/divisor/remainder is
 * reached.
 */
static int
dri3_wait_for_msc(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
                  int64_t remainder, int64_t *ust, int64_t *msc, int64_t *sbc)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   struct dri3_drawable *priv = (struct dri3_drawable *) pdraw;
   uint32_t msc_serial;

   /* Ask for the an event for the target MSC */
   msc_serial = ++priv->send_msc_serial;
   xcb_present_notify_msc(c,
                          priv->base.xDrawable,
                          msc_serial,
                          target_msc,
                          divisor,
                          remainder);

   xcb_flush(c);

   /* Wait for the event */
   if (priv->special_event) {
      while ((int32_t) (msc_serial - priv->recv_msc_serial) > 0) {
         if (!dri3_wait_for_event(pdraw))
            return 0;
      }
   }

   *ust = priv->ust;
   *msc = priv->msc;
   *sbc = priv->recv_sbc;

   return 1;
}

/** dri3_drawable_get_msc
 *
 * Return the current UST/MSC/SBC triplet by asking the server
 * for an event
 */
static int
dri3_drawable_get_msc(struct glx_screen *psc, __GLXDRIdrawable *pdraw,
                      int64_t *ust, int64_t *msc, int64_t *sbc)
{
   return dri3_wait_for_msc(pdraw, 0, 0, 0, ust, msc,sbc);
}

/** dri3_wait_for_sbc
 *
 * Wait for the completed swap buffer count to reach the specified
 * target. Presumably the application knows that this will be reached with
 * outstanding complete events, or we're going to be here awhile.
 */
static int
dri3_wait_for_sbc(__GLXDRIdrawable *pdraw, int64_t target_sbc, int64_t *ust,
                  int64_t *msc, int64_t *sbc)
{
   struct dri3_drawable *priv = (struct dri3_drawable *) pdraw;

   while (priv->recv_sbc < target_sbc) {
      if (!dri3_wait_for_event(pdraw))
         return 0;
   }

   *ust = priv->ust;
   *msc = priv->msc;
   *sbc = priv->recv_sbc;
   return 1;
}

/**
 * Asks the driver to flush any queued work necessary for serializing with the
 * X command stream, and optionally the slightly more strict requirement of
 * glFlush() equivalence (which would require flushing even if nothing had
 * been drawn to a window system framebuffer, for example).
 */
static void
dri3_flush(struct dri3_screen *psc,
           struct dri3_drawable *draw,
           unsigned flags,
           enum __DRI2throttleReason throttle_reason)
{
   struct glx_context *gc = __glXGetCurrentContext();

   if (gc) {
      struct dri3_context *dri3Ctx = (struct dri3_context *)gc;

      (*psc->f->flush_with_flags)(dri3Ctx->driContext, draw->driDrawable, flags, throttle_reason);
   }
}

static xcb_gcontext_t
dri3_drawable_gc(struct dri3_drawable *priv)
{
   if (!priv->gc) {
      uint32_t v;
      xcb_connection_t *c = XGetXCBConnection(priv->base.psc->dpy);

      v = 0;
      xcb_create_gc(c,
                    (priv->gc = xcb_generate_id(c)),
                    priv->base.xDrawable,
                    XCB_GC_GRAPHICS_EXPOSURES,
                    &v);
   }
   return priv->gc;
}

static struct dri3_buffer *
dri3_back_buffer(struct dri3_drawable *priv)
{
   return priv->buffers[DRI3_BACK_ID(priv->cur_back)];
}

static struct dri3_buffer *
dri3_fake_front_buffer(struct dri3_drawable *priv)
{
   return priv->buffers[DRI3_FRONT_ID];
}

static void
dri3_copy_area (xcb_connection_t *c  /**< */,
                xcb_drawable_t    src_drawable  /**< */,
                xcb_drawable_t    dst_drawable  /**< */,
                xcb_gcontext_t    gc  /**< */,
                int16_t           src_x  /**< */,
                int16_t           src_y  /**< */,
                int16_t           dst_x  /**< */,
                int16_t           dst_y  /**< */,
                uint16_t          width  /**< */,
                uint16_t          height  /**< */)
{
   xcb_void_cookie_t cookie;

   cookie = xcb_copy_area_checked(c,
                                  src_drawable,
                                  dst_drawable,
                                  gc,
                                  src_x,
                                  src_y,
                                  dst_x,
                                  dst_y,
                                  width,
                                  height);
   xcb_discard_reply(c, cookie.sequence);
}

static void
dri3_copy_sub_buffer(__GLXDRIdrawable *pdraw, int x, int y,
                     int width, int height,
                     Bool flush)
{
   struct dri3_drawable *priv = (struct dri3_drawable *) pdraw;
   struct dri3_screen *psc = (struct dri3_screen *) pdraw->psc;
   xcb_connection_t     *c = XGetXCBConnection(priv->base.psc->dpy);
   struct dri3_buffer *back = dri3_back_buffer(priv);

   unsigned flags;

   /* Check we have the right attachments */
   if (!priv->have_back || priv->is_pixmap)
      return;

   flags = __DRI2_FLUSH_DRAWABLE;
   if (flush)
      flags |= __DRI2_FLUSH_CONTEXT;
   dri3_flush(psc, priv, flags, __DRI2_THROTTLE_SWAPBUFFER);

   y = priv->height - y - height;

   dri3_fence_reset(c, back);
   dri3_copy_area(c,
                  dri3_back_buffer(priv)->pixmap,
                  priv->base.xDrawable,
                  dri3_drawable_gc(priv),
                  x, y, x, y, width, height);
   dri3_fence_trigger(c, back);
   /* Refresh the fake front (if present) after we just damaged the real
    * front.
    */
   if (priv->have_fake_front) {
      dri3_fence_reset(c, dri3_fake_front_buffer(priv));
      dri3_copy_area(c,
                     dri3_back_buffer(priv)->pixmap,
                     dri3_fake_front_buffer(priv)->pixmap,
                     dri3_drawable_gc(priv),
                     x, y, x, y, width, height);
      dri3_fence_trigger(c, dri3_fake_front_buffer(priv));
      dri3_fence_await(c, dri3_fake_front_buffer(priv));
   }
   dri3_fence_await(c, back);
}

static void
dri3_copy_drawable(struct dri3_drawable *priv, Drawable dest, Drawable src)
{
   struct dri3_screen *psc = (struct dri3_screen *) priv->base.psc;
   xcb_connection_t     *c = XGetXCBConnection(priv->base.psc->dpy);

   dri3_flush(psc, priv, __DRI2_FLUSH_DRAWABLE, 0);

   dri3_fence_reset(c, dri3_fake_front_buffer(priv));
   dri3_copy_area(c,
                  src, dest,
                  dri3_drawable_gc(priv),
                  0, 0, 0, 0, priv->width, priv->height);
   dri3_fence_trigger(c, dri3_fake_front_buffer(priv));
   dri3_fence_await(c, dri3_fake_front_buffer(priv));
}

static void
dri3_wait_x(struct glx_context *gc)
{
   struct dri3_drawable *priv = (struct dri3_drawable *)
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);

   if (priv == NULL || !priv->have_fake_front)
      return;

   dri3_copy_drawable(priv, dri3_fake_front_buffer(priv)->pixmap, priv->base.xDrawable);
}

static void
dri3_wait_gl(struct glx_context *gc)
{
   struct dri3_drawable *priv = (struct dri3_drawable *)
      GetGLXDRIDrawable(gc->currentDpy, gc->currentDrawable);

   if (priv == NULL || !priv->have_fake_front)
      return;

   dri3_copy_drawable(priv, priv->base.xDrawable, dri3_fake_front_buffer(priv)->pixmap);
}

/**
 * Called by the driver when it needs to update the real front buffer with the
 * contents of its fake front buffer.
 */
static void
dri3_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
{
   struct glx_context *gc;
   struct dri3_drawable *pdraw = loaderPrivate;
   struct dri3_screen *psc;

   if (!pdraw)
      return;

   if (!pdraw->base.psc)
      return;

   psc = (struct dri3_screen *) pdraw->base.psc;

   (void) __glXInitialize(psc->base.dpy);

   gc = __glXGetCurrentContext();

   dri3_flush(psc, pdraw, __DRI2_FLUSH_DRAWABLE, __DRI2_THROTTLE_FLUSHFRONT);

   dri3_wait_gl(gc);
}

static uint32_t
dri3_cpp_for_format(uint32_t format) {
   switch (format) {
   case  __DRI_IMAGE_FORMAT_R8:
      return 1;
   case  __DRI_IMAGE_FORMAT_RGB565:
   case  __DRI_IMAGE_FORMAT_GR88:
      return 2;
   case  __DRI_IMAGE_FORMAT_XRGB8888:
   case  __DRI_IMAGE_FORMAT_ARGB8888:
   case  __DRI_IMAGE_FORMAT_ABGR8888:
   case  __DRI_IMAGE_FORMAT_XBGR8888:
   case  __DRI_IMAGE_FORMAT_XRGB2101010:
   case  __DRI_IMAGE_FORMAT_ARGB2101010:
   case  __DRI_IMAGE_FORMAT_SARGB8:
      return 4;
   case  __DRI_IMAGE_FORMAT_NONE:
   default:
      return 0;
   }
}


/** dri3_alloc_render_buffer
 *
 * Use the driver createImage function to construct a __DRIimage, then
 * get a file descriptor for that and create an X pixmap from that
 *
 * Allocate an xshmfence for synchronization
 */
static struct dri3_buffer *
dri3_alloc_render_buffer(struct glx_screen *glx_screen, Drawable draw,
                         unsigned int format, int width, int height, int depth)
{
   struct dri3_screen *psc = (struct dri3_screen *) glx_screen;
   Display *dpy = glx_screen->dpy;
   struct dri3_buffer *buffer;
   xcb_connection_t *c = XGetXCBConnection(dpy);
   xcb_pixmap_t pixmap;
   xcb_sync_fence_t sync_fence;
   struct xshmfence *shm_fence;
   int buffer_fd, fence_fd;
   int stride;

   /* Create an xshmfence object and
    * prepare to send that to the X server
    */

   fence_fd = xshmfence_alloc_shm();
   if (fence_fd < 0)
      return NULL;
   shm_fence = xshmfence_map_shm(fence_fd);
   if (shm_fence == NULL)
      goto no_shm_fence;

   /* Allocate the image from the driver
    */
   buffer = calloc(1, sizeof (struct dri3_buffer));
   if (!buffer)
      goto no_buffer;

   buffer->cpp = dri3_cpp_for_format(format);
   if (!buffer->cpp)
      goto no_image;

   buffer->image = (*psc->image->createImage) (psc->driScreen,
                                               width, height,
                                               format,
                                               __DRI_IMAGE_USE_SHARE|__DRI_IMAGE_USE_SCANOUT,
                                               buffer);


   if (!buffer->image)
      goto no_image;

   /* X wants the stride, so ask the image for it
    */
   if (!(*psc->image->queryImage)(buffer->image, __DRI_IMAGE_ATTRIB_STRIDE, &stride))
      goto no_buffer_attrib;

   buffer->pitch = stride;

   if (!(*psc->image->queryImage)(buffer->image, __DRI_IMAGE_ATTRIB_FD, &buffer_fd))
      goto no_buffer_attrib;

   xcb_dri3_pixmap_from_buffer(c,
                               (pixmap = xcb_generate_id(c)),
                               draw,
                               buffer->size,
                               width, height, buffer->pitch,
                               depth, buffer->cpp * 8,
                               buffer_fd);

   xcb_dri3_fence_from_fd(c,
                          pixmap,
                          (sync_fence = xcb_generate_id(c)),
                          false,
                          fence_fd);

   buffer->pixmap = pixmap;
   buffer->own_pixmap = true;
   buffer->sync_fence = sync_fence;
   buffer->shm_fence = shm_fence;
   buffer->width = width;
   buffer->height = height;

   /* Mark the buffer as idle
    */
   dri3_fence_set(buffer);

   return buffer;

no_buffer_attrib:
   (*psc->image->destroyImage)(buffer->image);
no_image:
   free(buffer);
no_buffer:
   xshmfence_unmap_shm(shm_fence);
no_shm_fence:
   close(fence_fd);
   return NULL;
}

/** dri3_free_render_buffer
 *
 * Free everything associated with one render buffer including pixmap, fence
 * stuff and the driver image
 */
static void
dri3_free_render_buffer(struct dri3_drawable *pdraw, struct dri3_buffer *buffer)
{
   struct dri3_screen   *psc = (struct dri3_screen *) pdraw->base.psc;
   xcb_connection_t     *c = XGetXCBConnection(pdraw->base.psc->dpy);

   if (buffer->own_pixmap)
      xcb_free_pixmap(c, buffer->pixmap);
   xcb_sync_destroy_fence(c, buffer->sync_fence);
   xshmfence_unmap_shm(buffer->shm_fence);
   (*psc->image->destroyImage)(buffer->image);
   free(buffer);
}


/** dri3_flush_present_events
 *
 * Process any present events that have been received from the X server
 */
static void
dri3_flush_present_events(struct dri3_drawable *priv)
{
   xcb_connection_t     *c = XGetXCBConnection(priv->base.psc->dpy);

   /* Check to see if any configuration changes have occurred
    * since we were last invoked
    */
   if (priv->special_event) {
      xcb_generic_event_t    *ev;

      while ((ev = xcb_poll_for_special_event(c, priv->special_event)) != NULL) {
         xcb_present_generic_event_t *ge = (void *) ev;
         dri3_handle_present_event(priv, ge);
      }
   }
}

/** dri3_update_drawable
 *
 * Called the first time we use the drawable and then
 * after we receive present configure notify events to
 * track the geometry of the drawable
 */
static int
dri3_update_drawable(__DRIdrawable *driDrawable, void *loaderPrivate)
{
   struct dri3_drawable *priv = loaderPrivate;
   xcb_connection_t     *c = XGetXCBConnection(priv->base.psc->dpy);

   /* First time through, go get the current drawable geometry
    */
   if (priv->width == 0 || priv->height == 0 || priv->depth == 0) {
      xcb_get_geometry_cookie_t                 geom_cookie;
      xcb_get_geometry_reply_t                  *geom_reply;
      xcb_void_cookie_t                         cookie;
      xcb_generic_error_t                       *error;

      /* Try to select for input on the window.
       *
       * If the drawable is a window, this will get our events
       * delivered.
       *
       * Otherwise, we'll get a BadWindow error back from this request which
       * will let us know that the drawable is a pixmap instead.
       */


      cookie = xcb_present_select_input_checked(c,
                                                (priv->eid = xcb_generate_id(c)),
                                                priv->base.xDrawable,
                                                XCB_PRESENT_EVENT_MASK_CONFIGURE_NOTIFY|
                                                XCB_PRESENT_EVENT_MASK_COMPLETE_NOTIFY|
                                                XCB_PRESENT_EVENT_MASK_IDLE_NOTIFY);

      /* Create an XCB event queue to hold present events outside of the usual
       * application event queue
       */
      priv->special_event = xcb_register_for_special_xge(c,
                                                         &xcb_present_id,
                                                         priv->eid,
                                                         priv->stamp);

      geom_cookie = xcb_get_geometry(c, priv->base.xDrawable);

      geom_reply = xcb_get_geometry_reply(c, geom_cookie, NULL);

      if (!geom_reply)
         return false;

      priv->width = geom_reply->width;
      priv->height = geom_reply->height;
      priv->depth = geom_reply->depth;
      priv->is_pixmap = false;

      free(geom_reply);

      /* Check to see if our select input call failed. If it failed with a
       * BadWindow error, then assume the drawable is a pixmap. Destroy the
       * special event queue created above and mark the drawable as a pixmap
       */

      error = xcb_request_check(c, cookie);

      if (error) {
         if (error->error_code != BadWindow) {
            free(error);
            return false;
         }
         priv->is_pixmap = true;
         xcb_unregister_for_special_event(c, priv->special_event);
         priv->special_event = NULL;
      }
   }
   dri3_flush_present_events(priv);
   return true;
}

/* the DRIimage createImage function takes __DRI_IMAGE_FORMAT codes, while
 * the createImageFromFds call takes __DRI_IMAGE_FOURCC codes. To avoid
 * complete confusion, just deal in __DRI_IMAGE_FORMAT codes for now and
 * translate to __DRI_IMAGE_FOURCC codes in the call to createImageFromFds
 */
static int
image_format_to_fourcc(int format)
{

   /* Convert from __DRI_IMAGE_FORMAT to __DRI_IMAGE_FOURCC (sigh) */
   switch (format) {
   case __DRI_IMAGE_FORMAT_SARGB8: return __DRI_IMAGE_FOURCC_SARGB8888;
   case __DRI_IMAGE_FORMAT_RGB565: return __DRI_IMAGE_FOURCC_RGB565;
   case __DRI_IMAGE_FORMAT_XRGB8888: return __DRI_IMAGE_FOURCC_XRGB8888;
   case __DRI_IMAGE_FORMAT_ARGB8888: return __DRI_IMAGE_FOURCC_ARGB8888;
   case __DRI_IMAGE_FORMAT_ABGR8888: return __DRI_IMAGE_FOURCC_ABGR8888;
   case __DRI_IMAGE_FORMAT_XBGR8888: return __DRI_IMAGE_FOURCC_XBGR8888;
   }
   return 0;
}

/** dri3_get_pixmap_buffer
 *
 * Get the DRM object for a pixmap from the X server and
 * wrap that with a __DRIimage structure using createImageFromFds
 */
static struct dri3_buffer *
dri3_get_pixmap_buffer(__DRIdrawable *driDrawable,
                       unsigned int format,
                       enum dri3_buffer_type buffer_type,
                       void *loaderPrivate)
{
   struct dri3_drawable                 *pdraw = loaderPrivate;
   int                                  buf_id = dri3_pixmap_buf_id(buffer_type);
   struct dri3_buffer                   *buffer = pdraw->buffers[buf_id];
   Pixmap                               pixmap;
   xcb_dri3_buffer_from_pixmap_cookie_t bp_cookie;
   xcb_dri3_buffer_from_pixmap_reply_t  *bp_reply;
   int                                  *fds;
   Display                              *dpy;
   struct dri3_screen                   *psc;
   xcb_connection_t                     *c;
   xcb_sync_fence_t                     sync_fence;
   struct xshmfence                     *shm_fence;
   int                                  fence_fd;
   __DRIimage                           *image_planar;
   int                                  stride, offset;

   if (buffer)
      return buffer;

   pixmap = pdraw->base.xDrawable;
   psc = (struct dri3_screen *) pdraw->base.psc;
   dpy = psc->base.dpy;
   c = XGetXCBConnection(dpy);

   buffer = calloc(1, sizeof (struct dri3_buffer));
   if (!buffer)
      goto no_buffer;

   fence_fd = xshmfence_alloc_shm();
   if (fence_fd < 0)
      goto no_fence;
   shm_fence = xshmfence_map_shm(fence_fd);
   if (shm_fence == NULL) {
      close (fence_fd);
      goto no_fence;
   }

   xcb_dri3_fence_from_fd(c,
                          pixmap,
                          (sync_fence = xcb_generate_id(c)),
                          false,
                          fence_fd);

   /* Get an FD for the pixmap object
    */
   bp_cookie = xcb_dri3_buffer_from_pixmap(c, pixmap);
   bp_reply = xcb_dri3_buffer_from_pixmap_reply(c, bp_cookie, NULL);
   if (!bp_reply)
      goto no_image;
   fds = xcb_dri3_buffer_from_pixmap_reply_fds(c, bp_reply);

   stride = bp_reply->stride;
   offset = 0;

   /* createImageFromFds creates a wrapper __DRIimage structure which
    * can deal with multiple planes for things like Yuv images. So, once
    * we've gotten the planar wrapper, pull the single plane out of it and
    * discard the wrapper.
    */
   image_planar = (*psc->image->createImageFromFds) (psc->driScreen,
                                                     bp_reply->width,
                                                     bp_reply->height,
                                                     image_format_to_fourcc(format),
                                                     fds, 1,
                                                     &stride, &offset, buffer);
   close(fds[0]);
   if (!image_planar)
      goto no_image;

   buffer->image = (*psc->image->fromPlanar)(image_planar, 0, buffer);

   (*psc->image->destroyImage)(image_planar);

   if (!buffer->image)
      goto no_image;

   buffer->pixmap = pixmap;
   buffer->own_pixmap = false;
   buffer->width = bp_reply->width;
   buffer->height = bp_reply->height;
   buffer->buffer_type = buffer_type;
   buffer->shm_fence = shm_fence;
   buffer->sync_fence = sync_fence;

   pdraw->buffers[buf_id] = buffer;
   return buffer;

no_image:
   xcb_sync_destroy_fence(c, sync_fence);
   xshmfence_unmap_shm(shm_fence);
no_fence:
   free(buffer);
no_buffer:
   return NULL;
}

/** dri3_find_back
 *
 * Find an idle back buffer. If there isn't one, then
 * wait for a present idle notify event from the X server
 */
static int
dri3_find_back(xcb_connection_t *c, struct dri3_drawable *priv)
{
   int  b;
   xcb_generic_event_t *ev;
   xcb_present_generic_event_t *ge;

   for (;;) {
      for (b = 0; b < priv->num_back; b++) {
         int id = DRI3_BACK_ID((b + priv->cur_back) % priv->num_back);
         struct dri3_buffer *buffer = priv->buffers[id];

         if (!buffer || !buffer->busy) {
            priv->cur_back = id;
            return id;
         }
      }
      xcb_flush(c);
      ev = xcb_wait_for_special_event(c, priv->special_event);
      if (!ev)
         return -1;
      ge = (void *) ev;
      dri3_handle_present_event(priv, ge);
   }
}

/** dri3_get_buffer
 *
 * Find a front or back buffer, allocating new ones as necessary
 */
static struct dri3_buffer *
dri3_get_buffer(__DRIdrawable *driDrawable,
                unsigned int format,
                enum dri3_buffer_type buffer_type,
                void *loaderPrivate)
{
   struct dri3_drawable *priv = loaderPrivate;
   xcb_connection_t     *c = XGetXCBConnection(priv->base.psc->dpy);
   struct dri3_buffer      *buffer;
   int                  buf_id;

   if (buffer_type == dri3_buffer_back) {
      buf_id = dri3_find_back(c, priv);

      if (buf_id < 0)
         return NULL;
   } else {
      buf_id = DRI3_FRONT_ID;
   }

   buffer = priv->buffers[buf_id];

   /* Allocate a new buffer if there isn't an old one, or if that
    * old one is the wrong size
    */
   if (!buffer || buffer->width != priv->width || buffer->height != priv->height) {
      struct dri3_buffer   *new_buffer;

      /* Allocate the new buffers
       */
      new_buffer = dri3_alloc_render_buffer(priv->base.psc,
                                            priv->base.xDrawable,
                                            format, priv->width, priv->height, priv->depth);
      if (!new_buffer)
         return NULL;

      /* When resizing, copy the contents of the old buffer, waiting for that
       * copy to complete using our fences before proceeding
       */
      switch (buffer_type) {
      case dri3_buffer_back:
         if (buffer) {
            dri3_fence_reset(c, new_buffer);
            dri3_fence_await(c, buffer);
            dri3_copy_area(c,
                           buffer->pixmap,
                           new_buffer->pixmap,
                           dri3_drawable_gc(priv),
                           0, 0, 0, 0, priv->width, priv->height);
            dri3_fence_trigger(c, new_buffer);
            dri3_free_render_buffer(priv, buffer);
         }
         break;
      case dri3_buffer_front:
         dri3_fence_reset(c, new_buffer);
         dri3_copy_area(c,
                        priv->base.xDrawable,
                        new_buffer->pixmap,
                        dri3_drawable_gc(priv),
                        0, 0, 0, 0, priv->width, priv->height);
         dri3_fence_trigger(c, new_buffer);
         break;
      }
      buffer = new_buffer;
      buffer->buffer_type = buffer_type;
      priv->buffers[buf_id] = buffer;
   }
   dri3_fence_await(c, buffer);

   /* Return the requested buffer */
   return buffer;
}

/** dri3_free_buffers
 *
 * Free the front bufffer or all of the back buffers. Used
 * when the application changes which buffers it needs
 */
static void
dri3_free_buffers(__DRIdrawable *driDrawable,
                 enum dri3_buffer_type buffer_type,
                 void *loaderPrivate)
{
   struct dri3_drawable *priv = loaderPrivate;
   struct dri3_buffer      *buffer;
   int                  first_id;
   int                  n_id;
   int                  buf_id;

   switch (buffer_type) {
   case dri3_buffer_back:
      first_id = DRI3_BACK_ID(0);
      n_id = DRI3_MAX_BACK;
      break;
   case dri3_buffer_front:
      first_id = DRI3_FRONT_ID;
      n_id = 1;
   }

   for (buf_id = first_id; buf_id < first_id + n_id; buf_id++) {
      buffer = priv->buffers[buf_id];
      if (buffer) {
         dri3_free_render_buffer(priv, buffer);
         priv->buffers[buf_id] = NULL;
      }
   }
}

/** dri3_get_buffers
 *
 * The published buffer allocation API.
 * Returns all of the necessary buffers, allocating
 * as needed.
 */
static int
dri3_get_buffers(__DRIdrawable *driDrawable,
                 unsigned int format,
                 uint32_t *stamp,
                 void *loaderPrivate,
                 uint32_t buffer_mask,
                 struct __DRIimageList *buffers)
{
   struct dri3_drawable *priv = loaderPrivate;
   struct dri3_buffer   *front, *back;

   buffers->image_mask = 0;
   buffers->front = NULL;
   buffers->back = NULL;

   front = NULL;
   back = NULL;

   if (!dri3_update_drawable(driDrawable, loaderPrivate))
      return false;

   /* pixmaps always have front buffers */
   if (priv->is_pixmap)
      buffer_mask |= __DRI_IMAGE_BUFFER_FRONT;

   if (buffer_mask & __DRI_IMAGE_BUFFER_FRONT) {
      if (priv->is_pixmap)
         front = dri3_get_pixmap_buffer(driDrawable,
                                        format,
                                        dri3_buffer_front,
                                        loaderPrivate);
      else
         front = dri3_get_buffer(driDrawable,
                                 format,
                                 dri3_buffer_front,
                                 loaderPrivate);

      if (!front)
         return false;
   } else {
      dri3_free_buffers(driDrawable, dri3_buffer_front, loaderPrivate);
      priv->have_fake_front = 0;
   }

   if (buffer_mask & __DRI_IMAGE_BUFFER_BACK) {
      back = dri3_get_buffer(driDrawable,
                             format,
                             dri3_buffer_back,
                             loaderPrivate);
      if (!back)
         return false;
      priv->have_back = 1;
   } else {
      dri3_free_buffers(driDrawable, dri3_buffer_back, loaderPrivate);
      priv->have_back = 0;
   }

   if (front) {
      buffers->image_mask |= __DRI_IMAGE_BUFFER_FRONT;
      buffers->front = front->image;
      priv->have_fake_front = !priv->is_pixmap;
   }

   if (back) {
      buffers->image_mask |= __DRI_IMAGE_BUFFER_BACK;
      buffers->back = back->image;
   }

   priv->stamp = stamp;

   return true;
}

/* The image loader extension record for DRI3
 */
static const __DRIimageLoaderExtension imageLoaderExtension = {
   .base = { __DRI_IMAGE_LOADER, 1 },

   .getBuffers          = dri3_get_buffers,
   .flushFrontBuffer    = dri3_flush_front_buffer,
};

static const __DRIextension *loader_extensions[] = {
   &imageLoaderExtension.base,
   &systemTimeExtension.base,
   NULL
};

/** dri3_swap_buffers
 *
 * Make the current back buffer visible using the present extension
 */
static int64_t
dri3_swap_buffers(__GLXDRIdrawable *pdraw, int64_t target_msc, int64_t divisor,
                  int64_t remainder, Bool flush)
{
   struct dri3_drawable *priv = (struct dri3_drawable *) pdraw;
   struct dri3_screen *psc = (struct dri3_screen *) priv->base.psc;
   Display *dpy = priv->base.psc->dpy;
   xcb_connection_t *c = XGetXCBConnection(dpy);
   int buf_id = DRI3_BACK_ID(priv->cur_back);
   int64_t ret = 0;

   unsigned flags = __DRI2_FLUSH_DRAWABLE;
   if (flush)
      flags |= __DRI2_FLUSH_CONTEXT;
   dri3_flush(psc, priv, flags, __DRI2_THROTTLE_SWAPBUFFER);

   dri3_flush_present_events(priv);

   if (priv->buffers[buf_id] && !priv->is_pixmap) {
      dri3_fence_reset(c, priv->buffers[buf_id]);

      /* Compute when we want the frame shown by taking the last known successful
       * MSC and adding in a swap interval for each outstanding swap request
       */
      ++priv->send_sbc;
      if (target_msc == 0)
         target_msc = priv->msc + priv->swap_interval * (priv->send_sbc - priv->recv_sbc);

      priv->buffers[buf_id]->busy = 1;
      priv->buffers[buf_id]->last_swap = priv->send_sbc;
      xcb_present_pixmap(c,
                         priv->base.xDrawable,
                         priv->buffers[buf_id]->pixmap,
                         (uint32_t) priv->send_sbc,
                         0,                                    /* valid */
                         0,                                    /* update */
                         0,                                    /* x_off */
                         0,                                    /* y_off */
                         None,                                 /* target_crtc */
                         None,
                         priv->buffers[buf_id]->sync_fence,
                         XCB_PRESENT_OPTION_NONE,
                         target_msc,
                         divisor,
                         remainder, 0, NULL);
      ret = (int64_t) priv->send_sbc;

      /* If there's a fake front, then copy the source back buffer
       * to the fake front to keep it up to date. This needs
       * to reset the fence and make future users block until
       * the X server is done copying the bits
       */
      if (priv->have_fake_front) {
         dri3_fence_reset(c, priv->buffers[DRI3_FRONT_ID]);
         dri3_copy_area(c,
                        priv->buffers[buf_id]->pixmap,
                        priv->buffers[DRI3_FRONT_ID]->pixmap,
                        dri3_drawable_gc(priv),
                        0, 0, 0, 0, priv->width, priv->height);
         dri3_fence_trigger(c, priv->buffers[DRI3_FRONT_ID]);
      }
      xcb_flush(c);
      if (priv->stamp)
         ++(*priv->stamp);
   }

   return ret;
}

static int
dri3_get_buffer_age(__GLXDRIdrawable *pdraw)
{
   xcb_connection_t *c = XGetXCBConnection(pdraw->psc->dpy);
   struct dri3_drawable *priv = (struct dri3_drawable *) pdraw;
   int back_id = DRI3_BACK_ID(dri3_find_back(c, priv));

   if (back_id < 0 || !priv->buffers[back_id])
      return 0;

   if (priv->buffers[back_id]->last_swap != 0)
      return priv->send_sbc - priv->buffers[back_id]->last_swap + 1;
   else
      return 0;
}

/** dri3_open
 *
 * Wrapper around xcb_dri3_open
 */
static int
dri3_open(Display *dpy,
          Window root,
          CARD32 provider)
{
   xcb_dri3_open_cookie_t       cookie;
   xcb_dri3_open_reply_t        *reply;
   xcb_connection_t             *c = XGetXCBConnection(dpy);
   int                          fd;

   cookie = xcb_dri3_open(c,
                          root,
                          provider);

   reply = xcb_dri3_open_reply(c, cookie, NULL);
   if (!reply)
      return -1;

   if (reply->nfd != 1) {
      free(reply);
      return -1;
   }

   fd = xcb_dri3_open_reply_fds(c, reply)[0];
   fcntl(fd, F_SETFD, FD_CLOEXEC);

   return fd;
}


/** dri3_destroy_screen
 */
static void
dri3_destroy_screen(struct glx_screen *base)
{
   struct dri3_screen *psc = (struct dri3_screen *) base;

   /* Free the direct rendering per screen data */
   (*psc->core->destroyScreen) (psc->driScreen);
   driDestroyConfigs(psc->driver_configs);
   close(psc->fd);
   free(psc);
}

/** dri3_set_swap_interval
 *
 * Record the application swap interval specification,
 */
static int
dri3_set_swap_interval(__GLXDRIdrawable *pdraw, int interval)
{
   struct dri3_drawable *priv =  (struct dri3_drawable *) pdraw;
   GLint vblank_mode = DRI_CONF_VBLANK_DEF_INTERVAL_1;
   struct dri3_screen *psc = (struct dri3_screen *) priv->base.psc;

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

   priv->swap_interval = interval;
   dri3_update_num_back(priv);

   return 0;
}

/** dri3_get_swap_interval
 *
 * Return the stored swap interval
 */
static int
dri3_get_swap_interval(__GLXDRIdrawable *pdraw)
{
   struct dri3_drawable *priv =  (struct dri3_drawable *) pdraw;

  return priv->swap_interval;
}

static void
dri3_bind_tex_image(Display * dpy,
                    GLXDrawable drawable,
                    int buffer, const int *attrib_list)
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri3_context *pcp = (struct dri3_context *) gc;
   __GLXDRIdrawable *base = GetGLXDRIDrawable(dpy, drawable);
   struct dri3_drawable *pdraw = (struct dri3_drawable *) base;
   struct dri3_screen *psc;

   if (pdraw != NULL) {
      psc = (struct dri3_screen *) base->psc;

      (*psc->f->invalidate)(pdraw->driDrawable);

      XSync(dpy, false);

      (*psc->texBuffer->setTexBuffer2) (pcp->driContext,
                                        pdraw->base.textureTarget,
                                        pdraw->base.textureFormat,
                                        pdraw->driDrawable);
   }
}

static void
dri3_release_tex_image(Display * dpy, GLXDrawable drawable, int buffer)
{
   struct glx_context *gc = __glXGetCurrentContext();
   struct dri3_context *pcp = (struct dri3_context *) gc;
   __GLXDRIdrawable *base = GetGLXDRIDrawable(dpy, drawable);
   struct dri3_drawable *pdraw = (struct dri3_drawable *) base;
   struct dri3_screen *psc;

   if (pdraw != NULL) {
      psc = (struct dri3_screen *) base->psc;

      if (psc->texBuffer->base.version >= 3 &&
          psc->texBuffer->releaseTexBuffer != NULL)
         (*psc->texBuffer->releaseTexBuffer) (pcp->driContext,
                                              pdraw->base.textureTarget,
                                              pdraw->driDrawable);
   }
}

static const struct glx_context_vtable dri3_context_vtable = {
   .destroy             = dri3_destroy_context,
   .bind                = dri3_bind_context,
   .unbind              = dri3_unbind_context,
   .wait_gl             = dri3_wait_gl,
   .wait_x              = dri3_wait_x,
   .use_x_font          = DRI_glXUseXFont,
   .bind_tex_image      = dri3_bind_tex_image,
   .release_tex_image   = dri3_release_tex_image,
   .get_proc_address    = NULL,
};

/** dri3_bind_extensions
 *
 * Enable all of the extensions supported on DRI3
 */
static void
dri3_bind_extensions(struct dri3_screen *psc, struct glx_display * priv,
                     const char *driverName)
{
   const __DRIextension **extensions;
   unsigned mask;
   int i;

   extensions = psc->core->getExtensions(psc->driScreen);

   __glXEnableDirectExtension(&psc->base, "GLX_SGI_video_sync");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_swap_control");
   __glXEnableDirectExtension(&psc->base, "GLX_SGI_make_current_read");
   __glXEnableDirectExtension(&psc->base, "GLX_INTEL_swap_event");

   mask = psc->image_driver->getAPIMask(psc->driScreen);

   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context");
   __glXEnableDirectExtension(&psc->base, "GLX_ARB_create_context_profile");

   if ((mask & (1 << __DRI_API_GLES2)) != 0)
      __glXEnableDirectExtension(&psc->base,
                                 "GLX_EXT_create_context_es2_profile");

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

      if (strcmp(extensions[i]->name, __DRI2_ROBUSTNESS) == 0)
         __glXEnableDirectExtension(&psc->base,
                                    "GLX_ARB_create_context_robustness");

      if (strcmp(extensions[i]->name, __DRI2_RENDERER_QUERY) == 0) {
         psc->rendererQuery = (__DRI2rendererQueryExtension *) extensions[i];
         __glXEnableDirectExtension(&psc->base, "GLX_MESA_query_renderer");
      }
   }
}

static const struct glx_screen_vtable dri3_screen_vtable = {
   .create_context         = dri3_create_context,
   .create_context_attribs = dri3_create_context_attribs,
   .query_renderer_integer = dri3_query_renderer_integer,
   .query_renderer_string  = dri3_query_renderer_string,
};

/** dri3_create_screen
 *
 * Initialize DRI3 on the specified screen.
 *
 * Opens the DRI device, locates the appropriate DRI driver
 * and loads that.
 *
 * Checks to see if the driver supports the necessary extensions
 *
 * Initializes the driver for the screen and sets up our structures
 */

static struct glx_screen *
dri3_create_screen(int screen, struct glx_display * priv)
{
   xcb_connection_t *c = XGetXCBConnection(priv->dpy);
   const __DRIconfig **driver_configs;
   const __DRIextension **extensions;
   const struct dri3_display *const pdp = (struct dri3_display *)
      priv->dri3Display;
   struct dri3_screen *psc;
   __GLXDRIscreen *psp;
   struct glx_config *configs = NULL, *visuals = NULL;
   char *driverName, *deviceName;
   int i;

   psc = calloc(1, sizeof *psc);
   if (psc == NULL)
      return NULL;

   psc->fd = -1;

   if (!glx_screen_init(&psc->base, screen, priv)) {
      free(psc);
      return NULL;
   }

   psc->fd = dri3_open(priv->dpy, RootWindow(priv->dpy, screen), None);
   if (psc->fd < 0) {
      int conn_error = xcb_connection_has_error(c);

      glx_screen_cleanup(&psc->base);
      free(psc);
      InfoMessageF("screen %d does not appear to be DRI3 capable\n", screen);

      if (conn_error)
         ErrorMessageF("Connection closed during DRI3 initialization failure");

      return NULL;
   }
   deviceName = NULL;

   driverName = loader_get_driver_for_fd(psc->fd, 0);
   if (!driverName) {
      ErrorMessageF("No driver found\n");
      goto handle_error;
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
      if (strcmp(extensions[i]->name, __DRI_IMAGE_DRIVER) == 0)
         psc->image_driver = (__DRIimageDriverExtension *) extensions[i];
   }


   if (psc->core == NULL) {
      ErrorMessageF("core dri driver extension not found\n");
      goto handle_error;
   }

   if (psc->image_driver == NULL) {
      ErrorMessageF("image driver extension not found\n");
      goto handle_error;
   }

   psc->driScreen =
      psc->image_driver->createNewScreen2(screen, psc->fd,
                                          pdp->loader_extensions,
                                          extensions,
                                          &driver_configs, psc);

   if (psc->driScreen == NULL) {
      ErrorMessageF("failed to create dri screen\n");
      goto handle_error;
   }

   extensions = (*psc->core->getExtensions)(psc->driScreen);

   for (i = 0; extensions[i]; i++) {
      if (strcmp(extensions[i]->name, __DRI_IMAGE) == 0)
         psc->image = (__DRIimageExtension *) extensions[i];
   }

   if (psc->image == NULL) {
      ErrorMessageF("image extension not found\n");
      goto handle_error;
   }

   dri3_bind_extensions(psc, priv, driverName);

   if (!psc->f || psc->f->base.version < 4) {
      ErrorMessageF("Version 4 or later of flush extension not found\n");
      goto handle_error;
   }

   if (!psc->texBuffer || psc->texBuffer->base.version < 2 ||
       !psc->texBuffer->setTexBuffer2)
   {
      ErrorMessageF("Version 2 or later of texBuffer extension not found\n");
      goto handle_error;
   }

   configs = driConvertConfigs(psc->core, psc->base.configs, driver_configs);
   visuals = driConvertConfigs(psc->core, psc->base.visuals, driver_configs);

   if (!configs || !visuals)
       goto handle_error;

   glx_config_destroy_list(psc->base.configs);
   psc->base.configs = configs;
   glx_config_destroy_list(psc->base.visuals);
   psc->base.visuals = visuals;

   psc->driver_configs = driver_configs;

   psc->base.vtable = &dri3_screen_vtable;
   psp = &psc->vtable;
   psc->base.driScreen = psp;
   psp->destroyScreen = dri3_destroy_screen;
   psp->createDrawable = dri3_create_drawable;
   psp->swapBuffers = dri3_swap_buffers;

   psp->getDrawableMSC = dri3_drawable_get_msc;
   psp->waitForMSC = dri3_wait_for_msc;
   psp->waitForSBC = dri3_wait_for_sbc;
   psp->setSwapInterval = dri3_set_swap_interval;
   psp->getSwapInterval = dri3_get_swap_interval;
   __glXEnableDirectExtension(&psc->base, "GLX_OML_sync_control");

   psp->copySubBuffer = dri3_copy_sub_buffer;
   __glXEnableDirectExtension(&psc->base, "GLX_MESA_copy_sub_buffer");

   psp->getBufferAge = dri3_get_buffer_age;
   __glXEnableDirectExtension(&psc->base, "GLX_EXT_buffer_age");

   free(driverName);
   free(deviceName);

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

/** dri_destroy_display
 *
 * Called from __glXFreeDisplayPrivate.
 */
static void
dri3_destroy_display(__GLXDRIdisplay * dpy)
{
   free(dpy);
}

/** dri3_create_display
 *
 * Allocate, initialize and return a __DRIdisplayPrivate object.
 * This is called from __glXInitialize() when we are given a new
 * display pointer. This is public to that function, but hidden from
 * outside of libGL.
 */
_X_HIDDEN __GLXDRIdisplay *
dri3_create_display(Display * dpy)
{
   struct dri3_display                  *pdp;
   xcb_connection_t                     *c = XGetXCBConnection(dpy);
   xcb_dri3_query_version_cookie_t      dri3_cookie;
   xcb_dri3_query_version_reply_t       *dri3_reply;
   xcb_present_query_version_cookie_t   present_cookie;
   xcb_present_query_version_reply_t    *present_reply;
   xcb_generic_error_t                  *error;
   const xcb_query_extension_reply_t    *extension;

   xcb_prefetch_extension_data(c, &xcb_dri3_id);
   xcb_prefetch_extension_data(c, &xcb_present_id);

   extension = xcb_get_extension_data(c, &xcb_dri3_id);
   if (!(extension && extension->present))
      return NULL;

   extension = xcb_get_extension_data(c, &xcb_present_id);
   if (!(extension && extension->present))
      return NULL;

   dri3_cookie = xcb_dri3_query_version(c,
                                        XCB_DRI3_MAJOR_VERSION,
                                        XCB_DRI3_MINOR_VERSION);


   present_cookie = xcb_present_query_version(c,
                                   XCB_PRESENT_MAJOR_VERSION,
                                   XCB_PRESENT_MINOR_VERSION);

   pdp = malloc(sizeof *pdp);
   if (pdp == NULL)
      return NULL;

   dri3_reply = xcb_dri3_query_version_reply(c, dri3_cookie, &error);
   if (!dri3_reply) {
      free(error);
      goto no_extension;
   }

   pdp->dri3Major = dri3_reply->major_version;
   pdp->dri3Minor = dri3_reply->minor_version;
   free(dri3_reply);

   present_reply = xcb_present_query_version_reply(c, present_cookie, &error);
   if (!present_reply) {
      free(error);
      goto no_extension;
   }
   pdp->presentMajor = present_reply->major_version;
   pdp->presentMinor = present_reply->minor_version;
   free(present_reply);

   pdp->base.destroyDisplay = dri3_destroy_display;
   pdp->base.createScreen = dri3_create_screen;

   loader_set_logger(dri_message);

   pdp->loader_extensions = loader_extensions;

   return &pdp->base;
no_extension:
   free(pdp);
   return NULL;
}

#endif /* GLX_DIRECT_RENDERING */
