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
#include <xorg/fourcc.h>
#include <vl_winsys.h>
#include <pipe/p_screen.h>
#include <pipe/p_video_context.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>
#include <util/u_math.h>
#include <util/u_format.h>
#include "xvmc_private.h"

#define FOURCC_RGB 0x0000003

static enum pipe_format XvIDToPipe(int xvimage_id)
{
   switch (xvimage_id) {
      case FOURCC_RGB:
         return PIPE_FORMAT_B8G8R8X8_UNORM;

      case FOURCC_AI44:
      case FOURCC_IA44:
         return PIPE_FORMAT_L4A4_UNORM;

      default:
         XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized Xv image ID 0x%08X.\n", xvimage_id);
         return PIPE_FORMAT_NONE;
   }
}

static int PipeToComponentOrder(enum pipe_format format, char *component_order)
{
   assert(component_order);

   switch (format) {
      case PIPE_FORMAT_B8G8R8X8_UNORM:
      case PIPE_FORMAT_L4A4_UNORM:
         return 0;
      default:
         XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized PIPE_FORMAT 0x%08X.\n", format);
         component_order[0] = 0;
         component_order[1] = 0;
         component_order[2] = 0;
         component_order[3] = 0;
   }
   return 0;
}

static Status Validate(Display *dpy, XvPortID port, int surface_type_id, int xvimage_id)
{
   XvImageFormatValues *subpictures;
   int num_subpics;
   unsigned int i;

   subpictures = XvMCListSubpictureTypes(dpy, port, surface_type_id, &num_subpics);
   if (num_subpics < 1) {
      if (subpictures)
         XFree(subpictures);
      return BadMatch;
   }
   if (!subpictures)
      return BadAlloc;

   for (i = 0; i < num_subpics; ++i) {
      if (subpictures[i].id == xvimage_id) {
         XVMC_MSG(XVMC_TRACE, "[XvMC] Found requested subpicture format.\n" \
                              "[XvMC]   port=%u\n" \
                              "[XvMC]   surface id=0x%08X\n" \
                              "[XvMC]   image id=0x%08X\n" \
                              "[XvMC]   type=%08X\n" \
                              "[XvMC]   byte order=%08X\n" \
                              "[XvMC]   bits per pixel=%u\n" \
                              "[XvMC]   format=%08X\n" \
                              "[XvMC]   num planes=%d\n",
                              port, surface_type_id, xvimage_id, subpictures[i].type, subpictures[i].byte_order,
                              subpictures[i].bits_per_pixel, subpictures[i].format, subpictures[i].num_planes);
         if (subpictures[i].type == XvRGB) {
            XVMC_MSG(XVMC_TRACE, "[XvMC]   depth=%d\n" \
                                 "[XvMC]   red mask=0x%08X\n" \
                                 "[XvMC]   green mask=0x%08X\n" \
                                 "[XvMC]   blue mask=0x%08X\n",
                                 subpictures[i].depth, subpictures[i].red_mask,
                                 subpictures[i].green_mask, subpictures[i].blue_mask);
         }
         else if (subpictures[i].type == XvYUV) {
            XVMC_MSG(XVMC_TRACE, "[XvMC]   y sample bits=0x%08X\n" \
                                 "[XvMC]   u sample bits=0x%08X\n" \
                                 "[XvMC]   v sample bits=0x%08X\n" \
                                 "[XvMC]   horz y period=%u\n" \
                                 "[XvMC]   horz u period=%u\n" \
                                 "[XvMC]   horz v period=%u\n" \
                                 "[XvMC]   vert y period=%u\n" \
                                 "[XvMC]   vert u period=%u\n" \
                                 "[XvMC]   vert v period=%u\n",
                                 subpictures[i].y_sample_bits, subpictures[i].u_sample_bits, subpictures[i].v_sample_bits,
                                 subpictures[i].horz_y_period, subpictures[i].horz_u_period, subpictures[i].horz_v_period,
                                 subpictures[i].vert_y_period, subpictures[i].vert_u_period, subpictures[i].vert_v_period);
         }
         break;
      }
   }

   XFree(subpictures);

   return i < num_subpics ? Success : BadMatch;
}

PUBLIC
Status XvMCCreateSubpicture(Display *dpy, XvMCContext *context, XvMCSubpicture *subpicture,
                            unsigned short width, unsigned short height, int xvimage_id)
{
   XvMCContextPrivate *context_priv;
   XvMCSubpicturePrivate *subpicture_priv;
   struct pipe_video_context *vpipe;
   struct pipe_resource tex_templ, *tex;
   struct pipe_sampler_view sampler_templ;
   Status ret;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Creating subpicture %p.\n", subpicture);

   assert(dpy);

   if (!context)
      return XvMCBadContext;

   context_priv = context->privData;
   vpipe = context_priv->vctx->vpipe;

   if (!subpicture)
      return XvMCBadSubpicture;

   if (width > context_priv->subpicture_max_width ||
       height > context_priv->subpicture_max_height)
      return BadValue;

   ret = Validate(dpy, context->port, context->surface_type_id, xvimage_id);
   if (ret != Success)
      return ret;

   subpicture_priv = CALLOC(1, sizeof(XvMCSubpicturePrivate));
   if (!subpicture_priv)
      return BadAlloc;

   memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.target = PIPE_TEXTURE_2D;
   tex_templ.format = XvIDToPipe(xvimage_id);
   tex_templ.last_level = 0;
   if (vpipe->get_param(vpipe, PIPE_CAP_NPOT_TEXTURES)) {
      tex_templ.width0 = width;
      tex_templ.height0 = height;
   }
   else {
      tex_templ.width0 = util_next_power_of_two(width);
      tex_templ.height0 = util_next_power_of_two(height);
   }
   tex_templ.depth0 = 1;
   tex_templ.array_size = 1;
   tex_templ.usage = PIPE_USAGE_DYNAMIC;
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   tex_templ.flags = 0;

   tex = vpipe->screen->resource_create(vpipe->screen, &tex_templ);

   memset(&sampler_templ, 0, sizeof(sampler_templ));
   subpicture_priv->sampler = vpipe->create_sampler_view(vpipe, tex, &sampler_templ);
   pipe_resource_reference(&tex, NULL);
   if (!subpicture_priv->sampler) {
      FREE(subpicture_priv);
      return BadAlloc;
   }

   subpicture_priv->context = context;
   subpicture->subpicture_id = XAllocID(dpy);
   subpicture->context_id = context->context_id;
   subpicture->xvimage_id = xvimage_id;
   subpicture->width = width;
   subpicture->height = height;
   subpicture->num_palette_entries = 0;
   subpicture->entry_bytes = PipeToComponentOrder(tex_templ.format, subpicture->component_order);
   subpicture->privData = subpicture_priv;

   SyncHandle();

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p created.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCClearSubpicture(Display *dpy, XvMCSubpicture *subpicture, short x, short y,
                           unsigned short width, unsigned short height, unsigned int color)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;
   float color_f[4];

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   /* Convert color to float */
   util_format_read_4f(PIPE_FORMAT_B8G8R8A8_UNORM,
                       color_f, 1, &color, 4,
                       0, 0, 1, 1);

   subpicture_priv = subpicture->privData;
   context_priv = subpicture_priv->context->privData;
   /* TODO: Assert clear rect is within bounds? Or clip? */
   //context_priv->vctx->vpipe->clear_render_target(context_priv->vctx->vpipe,
   //                                               subpicture_priv->sampler, x, y,
   //                                               color_f, width, height);

   return Success;
}

PUBLIC
Status XvMCCompositeSubpicture(Display *dpy, XvMCSubpicture *subpicture, XvImage *image,
                               short srcx, short srcy, unsigned short width, unsigned short height,
                               short dstx, short dsty)
{
   XvMCSubpicturePrivate *subpicture_priv;
   XvMCContextPrivate *context_priv;
   struct pipe_video_context *vpipe;

   unsigned char *src, *dst, *dst_line;
   unsigned x, y;
   struct pipe_box dst_box = {dstx, dsty, 0, width, height, 1};

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
   vpipe = context_priv->vctx->vpipe;

   /* TODO: Assert rects are within bounds? Or clip? */
   vpipe->upload_sampler(vpipe, subpicture_priv->sampler, &dst_box,
                         image->data, width*3, srcx, srcy);

#if 0
   switch (image->id) {
      case FOURCC_RGB:
         assert(subpicture_priv->sfc->format == XvIDToPipe(image->id));
         for (y = 0; y < height; ++y) {
            dst_line = dst;
            for (x = 0; x < width; ++x, src += 3, dst_line += 4) {
               dst_line[0] = src[2]; /* B */
               dst_line[1] = src[1]; /* G */
               dst_line[2] = src[0]; /* R */
            }
            dst += xfer->stride;
         }
         break;
      default:
         XVMC_MSG(XVMC_ERR, "[XvMC] Unrecognized Xv image ID 0x%08X.\n", image->id);
   }
#endif

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p composited.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCDestroySubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   XvMCSubpicturePrivate *subpicture_priv;

   XVMC_MSG(XVMC_TRACE, "[XvMC] Destroying subpicture %p.\n", subpicture);

   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   subpicture_priv = subpicture->privData;
   pipe_sampler_view_reference(&subpicture_priv->sampler, NULL);
   FREE(subpicture_priv);

   XVMC_MSG(XVMC_TRACE, "[XvMC] Subpicture %p destroyed.\n", subpicture);

   return Success;
}

PUBLIC
Status XvMCSetSubpicturePalette(Display *dpy, XvMCSubpicture *subpicture, unsigned char *palette)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   assert(palette);

   /* We don't support paletted subpictures */
   return BadMatch;
}

PUBLIC
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

PUBLIC
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

PUBLIC
Status XvMCSyncSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

PUBLIC
Status XvMCFlushSubpicture(Display *dpy, XvMCSubpicture *subpicture)
{
   assert(dpy);

   if (!subpicture)
      return XvMCBadSubpicture;

   return Success;
}

PUBLIC
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
