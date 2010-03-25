/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "state_tracker/sw_winsys.h"
#include "dri_sw_winsys.h"


struct xm_displaytarget
{
   void *data;
   void *mapped;
};


/** Cast wrapper */
static INLINE struct xm_displaytarget *
xm_displaytarget( struct sw_displaytarget *dt )
{
   return (struct xm_displaytarget *)dt;
}


/* pipe_screen::is_format_supported */
static boolean
xm_is_displaytarget_format_supported( struct sw_winsys *ws,
                                      unsigned tex_usage,
                                      enum pipe_format format )
{
   /* TODO: check visuals or other sensible thing here */
   return TRUE;
}

static INLINE int
bytes_per_line(unsigned stride, unsigned mul)
{
   unsigned mask = mul - 1;

   return ((stride * 8 + mask) & ~mask) / 8;
}

/* pipe_screen::texture_create DISPLAY_TARGET / SCANOUT / SHARED */
static struct sw_displaytarget *
xm_displaytarget_create(struct sw_winsys *winsys,
                        unsigned tex_usage,
                        enum pipe_format format,
                        unsigned width, unsigned height,
                        unsigned alignment,
                        unsigned *stride)
{
   struct xm_displaytarget *xm_dt;
   unsigned nblocksy, size, xm_stride, loader_stride, format_stride;

   xm_dt = CALLOC_STRUCT(xm_displaytarget);
   if(!xm_dt)
      goto no_xm_dt;

   format_stride = util_format_get_stride(format, width);
   xm_stride = align(format_stride, alignment);
   loader_stride = bytes_per_line(format_stride, 32);

   nblocksy = util_format_get_nblocksy(format, height);
   size = xm_stride * nblocksy;

#ifdef DEBUG
   debug_printf("swrast format stride: %8d\n", format_stride);
   debug_printf("swrast pipe stride  : %8d\n", xm_stride);
   debug_printf("swrast loader stride: %8d\n", loader_stride);
#endif

   /*
    * Allocate with the aligned stride required by the pipe but set the stride
    * to the one hardcoded in the loaders XXX
    */

   xm_dt->data = align_malloc(size, alignment);
   if(!xm_dt->data)
      goto no_data;

   *stride = loader_stride;
   return (struct sw_displaytarget *)xm_dt;

no_data:
   FREE(xm_dt);
no_xm_dt:
   return NULL;
}

/* pipe_screen::texture_destroy */
static void
xm_displaytarget_destroy(struct sw_winsys *ws,
                         struct sw_displaytarget *dt)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);

   if (xm_dt->data) {
      FREE(xm_dt->data);
   }

   FREE(xm_dt);
}

/* pipe_context::transfer_map */
static void *
xm_displaytarget_map(struct sw_winsys *ws,
                     struct sw_displaytarget *dt,
                     unsigned flags)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   xm_dt->mapped = xm_dt->data;
   return xm_dt->mapped;
}

/* pipe_context::transfer_unmap */
static void
xm_displaytarget_unmap(struct sw_winsys *ws,
                       struct sw_displaytarget *dt)
{
   struct xm_displaytarget *xm_dt = xm_displaytarget(dt);
   xm_dt->mapped = NULL;
}

/* pipe_screen::texture_from_handle */
static struct sw_displaytarget *
xm_displaytarget_from_handle(struct sw_winsys *winsys,
                             const struct pipe_texture *templ,
                             struct winsys_handle *whandle,
                             unsigned *stride)
{
   assert(0);
   return NULL;
}

/* pipe_screen::texture_get_handle */
static boolean
xm_displaytarget_get_handle(struct sw_winsys *winsys,
                            struct sw_displaytarget *dt,
                            struct winsys_handle *whandle)
{
   assert(0);
   return FALSE;
}

/* pipe_screen::flush_frontbuffer */
static void
xm_displaytarget_display(struct sw_winsys *ws,
                         struct sw_displaytarget *dt,
                         void *context_private)
{
   assert(0);
}


static void
dri_destroy_sw_winsys(struct sw_winsys *winsys)
{
   FREE(winsys);
}

struct sw_winsys *
dri_create_sw_winsys(void)
{
   struct sw_winsys *ws;

   ws = CALLOC_STRUCT(sw_winsys);
   if (!ws)
      return NULL;

   ws->destroy = dri_destroy_sw_winsys;

   ws->is_displaytarget_format_supported = xm_is_displaytarget_format_supported;

   /* screen texture functions */
   ws->displaytarget_create = xm_displaytarget_create;
   ws->displaytarget_destroy = xm_displaytarget_destroy;
   ws->displaytarget_from_handle = xm_displaytarget_from_handle;
   ws->displaytarget_get_handle = xm_displaytarget_get_handle;

   /* texture functions */
   ws->displaytarget_map = xm_displaytarget_map;
   ws->displaytarget_unmap = xm_displaytarget_unmap;

   ws->displaytarget_display = xm_displaytarget_display;

   return ws;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
