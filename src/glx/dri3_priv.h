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

/* This file was derived from dri2_priv.h which carries the following
 * copyright:
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

#include <xcb/xcb.h>
#include <xcb/dri3.h>
#include <xcb/present.h>
#include <xcb/sync.h>

/* From xmlpool/options.h, user exposed so should be stable */
#define DRI_CONF_VBLANK_NEVER 0
#define DRI_CONF_VBLANK_DEF_INTERVAL_0 1
#define DRI_CONF_VBLANK_DEF_INTERVAL_1 2
#define DRI_CONF_VBLANK_ALWAYS_SYNC 3

enum dri3_buffer_type {
   dri3_buffer_back = 0,
   dri3_buffer_front = 1
};

struct dri3_buffer {
   __DRIimage   *image;
   uint32_t     pixmap;

   /* Synchronization between the client and X server is done using an
    * xshmfence that is mapped into an X server SyncFence. This lets the
    * client check whether the X server is done using a buffer with a simple
    * xshmfence call, rather than going to read X events from the wire.
    *
    * However, we can only wait for one xshmfence to be triggered at a time,
    * so we need to know *which* buffer is going to be idle next. We do that
    * by waiting for a PresentIdleNotify event. When that event arrives, the
    * 'busy' flag gets cleared and the client knows that the fence has been
    * triggered, and that the wait call will not block.
    */

   uint32_t     sync_fence;     /* XID of X SyncFence object */
   int32_t      *shm_fence;     /* pointer to xshmfence object */
   GLboolean    busy;           /* Set on swap, cleared on IdleNotify */
   void         *driverPrivate;

   uint32_t     size;
   uint32_t     pitch;
   uint32_t     cpp;
   uint32_t     flags;
   uint32_t     width, height;

   enum dri3_buffer_type        buffer_type;
};

struct dri3_display
{
   __GLXDRIdisplay base;

   const __DRIextension *loader_extensions[8];

   /* DRI3 bits */
   int dri3Major;
   int dri3Minor;

   /* Present bits */
   int hasPresent;
   int presentMajor;
   int presentMinor;
};

struct dri3_screen {
   struct glx_screen base;

   __DRIscreen *driScreen;
   __GLXDRIscreen vtable;

   const __DRIimageExtension *image;
   const __DRIimageDriverExtension *image_driver;
   const __DRIcoreExtension *core;
   const __DRI2flushExtension *f;
   const __DRI2configQueryExtension *config;
   const __DRItexBufferExtension *texBuffer;
   const __DRIconfig **driver_configs;

   void *driver;
   int fd;

   Bool show_fps;
};

struct dri3_context
{
   struct glx_context base;
   __DRIcontext *driContext;
};

#define DRI3_MAX_BACK   2
#define DRI3_BACK_ID(i) (i)
#define DRI3_FRONT_ID   (DRI3_MAX_BACK)

static inline int
dri3_buf_id_next(int buf_id)
{
   if (buf_id == DRI3_MAX_BACK - 1)
      return 0;
   return buf_id + 1;
}

static inline int
dri3_buf_id_prev(int buf_id)
{
   if (buf_id == 0)
      return DRI3_MAX_BACK - 1;
   return buf_id - 1;
}

static inline int
dri3_pixmap_buf_id(enum dri3_buffer_type buffer_type)
{
   if (buffer_type == dri3_buffer_back)
      return DRI3_BACK_ID(0);
   else
      return DRI3_FRONT_ID;
}

struct dri3_drawable {
   __GLXDRIdrawable base;
   __DRIdrawable *driDrawable;
   int width, height;
   int swap_interval;
   uint8_t have_back;
   uint8_t have_fake_front;
   uint8_t is_pixmap;

   uint32_t present_request_serial;
   uint32_t present_event_serial;

   uint64_t sbc;

   uint64_t ust, msc;

   /* For WaitMSC */
   uint32_t present_msc_request_serial;
   uint32_t present_msc_event_serial;
   
   uint64_t previous_time;
   unsigned frames;

   struct dri3_buffer *buffers[1 + DRI3_MAX_BACK];
   int cur_back;
   int depth;

   uint32_t *stamp;

   xcb_present_event_t eid;
   xcb_gcontext_t gc;
   xcb_special_event_t *special_event;
};

char *
dri3_get_driver_for_fd(int fd);
