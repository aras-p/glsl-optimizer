/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
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

#include <assert.h>
#include <X11/Xlibint.h>
#include <X11/extensions/XvMClib.h>
#include <vl_winsys.h>
#include <pipe/p_screen.h>
#include <pipe/p_video_context.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>
#include <util/u_math.h>
#include "xvmc_private.h"

#define FOURCC_RGB 0x0000003

Status XvMCCreateSubpicture(Display *dpy, XvMCContext *context, XvMCSubpicture *subpicture,
                            unsigned short width, unsigned short height, int xvimage_id)
{
   XvMCContextPrivate *context_priv;
   XvMCSubpicturePrivate *subpicture_priv;
   struct pipe_video_context *vpipe;
   struct pipe_texture template;
   struct pipe_texture *tex;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Creating subpicture %p.\n", subpicture);

   assert(dpy);

   if (!context)
      return XvMCBadContext;

   context_priv = context->privData;
   vpipe = context_priv->vctx->vpipe;

   if (!subpicture)
      return XvMCBadSubpicture;

   /* TODO: Check against surface max width, height */
   if (width > 2048 || height > 2048)
      return BadValue;

   if (xvimage_id != FOURCC_RGB)
      return BadMatch;

   subpicture_priv = CALLOC(1, sizeof(XvMCSubpicturePrivate));
   if (!subpicture_priv)
      return BadAlloc;

   memset(&template, 0, sizeof(struct pipe_texture));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_X8R8G8B8_UNORM;
   template.last_level = 0;
   if (vpipe->screen->get_param(vpipe->screen, PIPE_CAP_NPOT_TEXTURES)) {
      template.width0 = width;
      template.height0 = height;
   }
   else {
      template.width0 = util_next_power_of_two(width);
      template.height0 = util_next_power_of_two(height);
   }
   template.depth0 = 1;
   template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER;

   subpicture_priv->context = context;
   tex = vpipe->screen->texture_create(vpipe->screen, &template);
   subpicture_priv->sfc = vpipe->screen->get_tex_surface(vpipe->screen, tex, 0, 0, 0,
                                                         PIPE_BUFFER_USAGE_CPU_WRITE |
                                                         PIPE_BUFFER_USAGE_GPU_READ);
   pipe_texture_reference(&tex, NULL);
   if (!subpicture_priv->sfc)
   {
      FREE(subpicture_priv);
      return BadAlloc;
   }

   subpicture->subpicture_id = XAllocID(dpy);
   subpicture->context_id = context->context_id;
   subpicture->xvimage_id = xvimage_id;
   subpicture->width = width;
   subpicture->height = height;
   subpicture->num_palette_entries = 0;
   subpicture->entry_bytes = 0;
   subpicture->component_order[0] = 0;
   subpicture->component_order[1] = 0;
   subpicture->component_order[2] = 0;
   subpicture->component_order[3] = 0;
   subpicture->privData = subpicture_priv;

   SyncHandle();

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p created.\n", subpicture);

   return Success;
}

Status XvMCClearSubpicture(Display *dpy, XvMCSubpicture *subpicture, short x, short y,
                           unsigned short width, unsigned short height, unsigned int color)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   subpicture_priv = subpicture->privData;
   context_priv = subpicture_priv->context->privData;
   /* TODO: Assert clear rect is within bounds? Or clip? */
   context_priv->vctx->vpipe->surface_fill(context_priv->vctx->vpipe,
                                           subpicture_priv->sfc, x, y,
                                           width, height, color);

   return Success;
}

Status XvMCCompositeSubpicture(Display *dpy, XvMCSubpicture *subpicture, XvImage *image,
                               short srcx, short srcy, unsigned short width, unsigned short height,
                               short dstx, short dsty)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;
   struct pipe_screen *screen;
   struct pipe_transfer *xfer;
   unsigned char *src, *dst;
   unsigned x, y;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Compositing subpicture %p.\n", subpicture);

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(image);

   if (subpicture->xvimage_id != image->id)
      return BadMatch;

   /* No planar support for now */
   if (image->num_planes != 1)
      return BadMatch;

   subpicture_priv = subpicture->privData;
   context_priv = subpicture_priv->context->privData;
   screen = context_priv->vctx->vpipe->screen;

   /* TODO: Assert rects are within bounds? Or clip? */

   xfer = screen->get_tex_transfer(screen, subpicture_priv->sfc->texture, 0, 0, 0,
                                   PIPE_TRANSFER_WRITE, dstx, dsty, width, height);
   if (!xfer)
      return BadAlloc;

   src = image->data;
   dst = screen->transfer_map(screen, xfer);
   if (!dst) {
      screen->tex_transfer_destroy(xfer);
      return BadAlloc;
   }

   switch (image->id)
   {
      case FOURCC_RGB:
         assert(subpicture_priv->sfc->format == PIPE_FORMAT_X8R8G8B8_UNORM);
         for (y = 0; y < height; ++y) {
            for (x = 0; x < width; ++x, src += 3, dst += 4) {
               /* TODO: Confirm or fix */
               dst[0] = src[0];
               dst[1] = src[1];
               dst[2] = src[2];
            }
         }
         break;
      default:
         assert(false);
   }

   screen->transfer_unmap(screen, xfer);
   screen->tex_transfer_destroy(xfer);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p composited.\n", subpicture);

   return Success;
}

Status XvMCDestroySubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   XvMCSubpicturePrivate *subpicture_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Destroying subpicture %p.\n", subpicture);

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   subpicture_priv = subpicture->privData;
   pipe_surface_reference(&subpicture_priv->sfc, NULL);
   FREE(subpicture_priv);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p destroyed.\n", subpicture);

   return Success;
}

Status XvMCSetSubpicturePalette(Display *dpy, XvMCSubpicture *subpicture, unsigned char *palette)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(palette);

   /* We don't support paletted subpictures */
   return BadMatch;
}

Status XvMCBlendSubpicture(Display *dpy, XvMCSurface *target_surface, XvMCSubpicture *subpicture,
                           short subx, short suby, unsigned short subw, unsigned short subh,
                           short surfx, short surfy, unsigned short surfw, unsigned short surfh)
{
   XvMCSurfacePrivate *surface_priv;
   XvMCSubpicturePrivate *subpicture_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Associating subpicture %p with surface %p.\n", subpicture, target_surface);

   assert(dpy);

   if (!target_surface)
      return XvMCBadSurface;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (target_surface->context_id != subpicture->context_id)
      return BadMatch;

   /* TODO: Verify against subpicture independent scaling */

   surface_priv = target_surface->privData;
   subpicture_priv = subpicture->privData;

   /* TODO: Assert rects are within bounds? Or clip? */

   surface_priv->subpicture = subpicture;
   surface_priv->subx = subx;
   surface_priv->suby = suby;
   surface_priv->subw = subw;
   surface_priv->subh = subh;
   surface_priv->surfx = surfx;
   surface_priv->surfy = surfy;
   surface_priv->surfw = surfw;
   surface_priv->surfh = surfh;
   subpicture_priv->surface = target_surface;

   return Success;
}

Status XvMCBlendSubpicture2(Display *dpy, XvMCSurface *source_surface, XvMCSurface *target_surface,
                            XvMCSubpicture *subpicture, short subx, short suby, unsigned short subw, unsigned short subh,
                            short surfx, short surfy, unsigned short surfw, unsigned short surfh)
{
   assert(dpy);

   if (!source_surface || !target_surface)
      return XvMCBadSurface;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (source_surface->context_id != subpicture->context_id)
      return BadMatch;

   if (source_surface->context_id != subpicture->context_id)
      return BadMatch;

   /* TODO: Assert rects are within bounds? Or clip? */

   return Success;
}

Status XvMCSyncSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

Status XvMCFlushSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

Status XvMCGetSubpictureStatus(Display *dpy, XvMCSubpicture *subpicture, int *status)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(status);

   /* TODO */
   *status = 0;

   return Success;
}
