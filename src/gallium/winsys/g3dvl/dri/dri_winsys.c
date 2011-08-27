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

#include "vl_winsys.h"
#include "driclient.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_memory.h"
#include "util/u_hash.h"
#include "util/u_hash_table.h"
#include "util/u_inlines.h"
#include "state_tracker/drm_driver.h"
#include <X11/Xlibint.h>

struct vl_dri_screen
{
   struct vl_screen base;
   dri_screen_t *dri_screen;
   struct util_hash_table *drawable_table;
   Drawable last_seen_drawable;
};

struct vl_dri_context
{
   struct vl_context base;
   int fd;
};

static struct pipe_surface*
vl_dri2_get_front(struct vl_context *vctx, Drawable drawable)
{
   int w, h;
   unsigned int attachments[1] = {DRI_BUFFER_FRONT_LEFT};
   int count;
   DRI2Buffer *dri2_front;
   struct pipe_resource *front_tex;
   struct pipe_surface *front_surf = NULL;

   assert(vctx);

   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vctx->vscreen;
   assert(vl_dri_scrn);

   dri2_front = DRI2GetBuffers(vl_dri_scrn->dri_screen->display,
                               drawable, &w, &h, attachments, 1, &count);

   assert(count == 1);

   if (dri2_front) {
      struct winsys_handle dri2_front_handle =
      {
         .type = DRM_API_HANDLE_TYPE_SHARED,
         .handle = dri2_front->name,
         .stride = dri2_front->pitch
      };
      struct pipe_resource template;
      struct pipe_surface surf_template;

      memset(&template, 0, sizeof(struct pipe_resource));
      template.target = PIPE_TEXTURE_2D;
      template.format = PIPE_FORMAT_B8G8R8X8_UNORM;
      template.last_level = 0;
      template.width0 = w;
      template.height0 = h;
      template.depth0 = 1;
      template.usage = PIPE_USAGE_STATIC;
      template.bind = PIPE_BIND_RENDER_TARGET;
      template.flags = 0;

      front_tex = vl_dri_scrn->base.pscreen->resource_from_handle(vl_dri_scrn->base.pscreen, &template, &dri2_front_handle);
      if (front_tex) {
         memset(&surf_template, 0, sizeof(surf_template));
         surf_template.format = front_tex->format;
         surf_template.usage = PIPE_BIND_RENDER_TARGET;
         front_surf = vctx->pipe->create_surface(vctx->pipe, front_tex, &surf_template);
      }
      pipe_resource_reference(&front_tex, NULL);
      Xfree(dri2_front);
   }

   return front_surf;
}

static void
vl_dri2_flush_frontbuffer(struct pipe_screen *screen,
                          struct pipe_resource *resource,
                          unsigned level, unsigned layer,
                          void *context_private)
{
   struct vl_dri_context *vl_dri_ctx = (struct vl_dri_context*)context_private;
   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vl_dri_ctx->base.vscreen;

   assert(screen);
   assert(resource);
   assert(context_private);

   dri2CopyDrawable(vl_dri_scrn->dri_screen, vl_dri_scrn->last_seen_drawable,
                    DRI_BUFFER_FRONT_LEFT, DRI_BUFFER_FAKE_FRONT_LEFT);
}

struct pipe_surface*
vl_drawable_surface_get(struct vl_context *vctx, Drawable drawable)
{
   assert(vctx);

   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vctx->vscreen;
   assert(vl_dri_scrn);

   if (vl_dri_scrn->last_seen_drawable != drawable) {
      /* Hash table business depends on this equality */
      assert(None == NULL);
      Drawable lookup_drawable = (Drawable)util_hash_table_get(vl_dri_scrn->drawable_table, (void*)drawable);
      if (lookup_drawable == None) {
         dri2CreateDrawable(vl_dri_scrn->dri_screen, drawable);
         util_hash_table_set(vl_dri_scrn->drawable_table, (void*)drawable, (void*)drawable);
      }
      vl_dri_scrn->last_seen_drawable = drawable;
   }

   return vl_dri2_get_front(vctx, drawable);
}

void*
vl_contextprivate_get(struct vl_context *vctx, struct pipe_surface *displaytarget)
{
   return vctx;
}

static unsigned drawable_hash(void *key)
{
   Drawable drawable = (Drawable)key;
   assert(drawable != None);
   return util_hash_crc32(&drawable, sizeof(Drawable));
}

static int drawable_cmp(void *key1, void *key2)
{
   Drawable d1 = (Drawable)key1;
   Drawable d2 = (Drawable)key2;
   assert(d1 != None);
   assert(d2 != None);
   return d1 != d2;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_dri_screen *vl_dri_scrn;

   assert(display);

   vl_dri_scrn = CALLOC_STRUCT(vl_dri_screen);
   if (!vl_dri_scrn)
      goto no_struct;

   if (dri2CreateScreen(display, screen, &vl_dri_scrn->dri_screen))
      goto no_dri2screen;

   vl_dri_scrn->base.pscreen = driver_descriptor.create_screen(vl_dri_scrn->dri_screen->fd);

   if (!vl_dri_scrn->base.pscreen)
      goto no_pscreen;

   vl_dri_scrn->drawable_table = util_hash_table_create(&drawable_hash, &drawable_cmp);
   if (!vl_dri_scrn->drawable_table)
      goto no_hash;

   vl_dri_scrn->last_seen_drawable = None;
   vl_dri_scrn->base.pscreen->flush_frontbuffer = vl_dri2_flush_frontbuffer;

   return &vl_dri_scrn->base;

no_hash:
   vl_dri_scrn->base.pscreen->destroy(vl_dri_scrn->base.pscreen);
no_pscreen:
   dri2DestroyScreen(vl_dri_scrn->dri_screen);
no_dri2screen:
   FREE(vl_dri_scrn);
no_struct:
   return NULL;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vscreen;

   assert(vscreen);

   util_hash_table_destroy(vl_dri_scrn->drawable_table);
   vl_dri_scrn->base.pscreen->destroy(vl_dri_scrn->base.pscreen);
   dri2DestroyScreen(vl_dri_scrn->dri_screen);
   FREE(vl_dri_scrn);
}

struct vl_context*
vl_video_create(struct vl_screen *vscreen)
{
   struct vl_dri_screen *vl_dri_scrn = (struct vl_dri_screen*)vscreen;
   struct vl_dri_context *vl_dri_ctx;

   vl_dri_ctx = CALLOC_STRUCT(vl_dri_context);
   if (!vl_dri_ctx)
      goto no_struct;

   vl_dri_ctx->base.pipe = vscreen->pscreen->context_create(vscreen->pscreen, vl_dri_ctx);
   if (!vl_dri_ctx->base.pipe) {
      debug_printf("[G3DVL] No video support found on %s/%s.\n",
                   vscreen->pscreen->get_vendor(vscreen->pscreen),
                   vscreen->pscreen->get_name(vscreen->pscreen));
      goto no_pipe;
   }

   vl_dri_ctx->base.vscreen = vscreen;
   vl_dri_ctx->fd = vl_dri_scrn->dri_screen->fd;

   return &vl_dri_ctx->base;

no_pipe:
   FREE(vl_dri_ctx);

no_struct:
   return NULL;
}

void vl_video_destroy(struct vl_context *vctx)
{
   struct vl_dri_context *vl_dri_ctx = (struct vl_dri_context*)vctx;

   assert(vctx);

   vl_dri_ctx->base.pipe->destroy(vl_dri_ctx->base.pipe);
   FREE(vl_dri_ctx);
}
