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

/* TODO:
 *
 * xshm / texture_from_pixmap / EGLImage:
 *
 * Allow the loaders to use the XSHM extension. It probably requires callbacks
 * for createImage/destroyImage similar to DRI2 getBuffers.
 */

#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "pipe/p_context.h"
#include "state_tracker/drisw_api.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"
#include "dri1_helper.h"
#include "drisw.h"


static INLINE void
get_drawable_info(__DRIdrawable *dPriv, int *w, int *h)
{
   __DRIscreen *sPriv = dPriv->driScreenPriv;
   const __DRIswrastLoaderExtension *loader = sPriv->swrast_loader;
   int x, y;

   loader->getDrawableInfo(dPriv,
                           &x, &y, w, h,
                           dPriv->loaderPrivate);
}

static INLINE void
put_image(__DRIdrawable *dPriv, void *data, unsigned width, unsigned height)
{
   __DRIscreen *sPriv = dPriv->driScreenPriv;
   const __DRIswrastLoaderExtension *loader = sPriv->swrast_loader;

   loader->putImage(dPriv, __DRI_SWRAST_IMAGE_OP_SWAP,
                    0, 0, width, height,
                    data, dPriv->loaderPrivate);
}

void
drisw_update_drawable_info(struct dri_drawable *drawable)
{
   __DRIdrawable *dPriv = drawable->dPriv;

   get_drawable_info(dPriv, &dPriv->w, &dPriv->h);
}

static void
drisw_put_image(struct dri_drawable *drawable,
                void *data, unsigned width, unsigned height)
{
   __DRIdrawable *dPriv = drawable->dPriv;

   put_image(dPriv, data, width, height);
}

static INLINE void
drisw_present_texture(__DRIdrawable *dPriv,
                      struct pipe_texture *ptex)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   struct pipe_surface *psurf;

   psurf = dri1_get_pipe_surface(drawable, ptex);
   if (!psurf)
      return;

   screen->pipe_screen->flush_frontbuffer(screen->pipe_screen, psurf, drawable);
}

static INLINE void
drisw_invalidate_drawable(__DRIdrawable *dPriv)
{
   struct dri_context *ctx = dri_get_current();
   struct dri_drawable *drawable = dri_drawable(dPriv);

   drawable->texture_stamp = dPriv->lastStamp - 1;

   /* check if swapping currently bound buffer */
   if (ctx && ctx->dPriv == dPriv)
      ctx->st->notify_invalid_framebuffer(ctx->st, drawable->stfb);
}

static INLINE void
drisw_copy_to_front(__DRIdrawable * dPriv,
                    struct pipe_texture *ptex)
{
   drisw_present_texture(dPriv, ptex);

   drisw_invalidate_drawable(dPriv);
}

/*
 * Backend functions for st_framebuffer interface and swap_buffers.
 */

void
drisw_swap_buffers(__DRIdrawable *dPriv)
{
   struct dri_context *ctx = dri_get_current();
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_texture *ptex;

   if (!ctx)
      return;

   ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT];

   if (ptex) {
      ctx->st->flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);

      drisw_copy_to_front(dPriv, ptex);
   }
}

void
drisw_flush_frontbuffer(struct dri_drawable *drawable,
                        enum st_attachment_type statt)
{
   struct dri_context *ctx = dri_get_current();
   struct pipe_texture *ptex;

   if (!ctx)
      return;

   ptex = drawable->textures[statt];

   if (ptex) {
      drisw_copy_to_front(ctx->dPriv, ptex);
   }
}

/**
 * Allocate framebuffer attachments.
 *
 * During fixed-size operation, the function keeps allocating new attachments
 * as they are requested. Unused attachments are not removed, not until the
 * framebuffer is resized or destroyed.
 *
 * It should be possible for DRI1 and DRISW to share this function, but it
 * seems a better seperation and safer for each DRI version to provide its own
 * function.
 */
void
drisw_allocate_textures(struct dri_drawable *drawable,
                        unsigned mask)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   struct pipe_texture templ;
   unsigned width, height;
   boolean resized;
   int i;

   width  = drawable->dPriv->w;
   height = drawable->dPriv->h;

   resized = (drawable->old_w != width ||
              drawable->old_h != height);

   /* remove outdated textures */
   if (resized) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
         pipe_texture_reference(&drawable->textures[i], NULL);
   }

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.last_level = 0;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      enum pipe_format format;
      unsigned tex_usage;

      /* the texture already exists or not requested */
      if (drawable->textures[i] || !(mask & (1 << i))) {
         continue;
      }

      switch (i) {
      case ST_ATTACHMENT_FRONT_LEFT:
      case ST_ATTACHMENT_BACK_LEFT:
      case ST_ATTACHMENT_FRONT_RIGHT:
      case ST_ATTACHMENT_BACK_RIGHT:
         format = drawable->stvis.color_format;
         tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                     PIPE_TEXTURE_USAGE_RENDER_TARGET;
         break;
      case ST_ATTACHMENT_DEPTH_STENCIL:
         format = drawable->stvis.depth_stencil_format;
         tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
         break;
      default:
         format = PIPE_FORMAT_NONE;
         break;
      }

      if (format != PIPE_FORMAT_NONE) {
         templ.format = format;
         templ.tex_usage = tex_usage;

         drawable->textures[i] =
            screen->pipe_screen->texture_create(screen->pipe_screen, &templ);
      }
   }

   drawable->old_w = width;
   drawable->old_h = height;
}

/*
 * Backend function for init_screen.
 */

static const __DRIextension *drisw_screen_extensions[] = {
   NULL
};

static struct drisw_loader_funcs drisw_lf = {
   .put_image = drisw_put_image
};

const __DRIconfig **
drisw_init_screen(__DRIscreen * sPriv)
{
   const __DRIconfig **configs;
   struct dri_screen *screen;
   struct drisw_create_screen_arg arg;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->api = drm_api_create();
   screen->sPriv = sPriv;
   screen->fd = -1;

   sPriv->private = (void *)screen;
   sPriv->extensions = drisw_screen_extensions;

   arg.base.mode = DRM_CREATE_DRISW;
   arg.lf = &drisw_lf;

   configs = dri_init_screen_helper(screen, &arg.base, 32);
   if (!configs)
      goto fail;

   return configs;
fail:
   dri_destroy_screen_helper(screen);
   FREE(screen);
   return NULL;
}

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driSWRastExtension.base,
    NULL
};

/* vim: set sw=3 ts=8 sts=3 expandtab: */
