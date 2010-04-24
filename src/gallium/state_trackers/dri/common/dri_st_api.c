/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_debug.h"
#include "state_tracker/st_gl_api.h" /* for st_gl_api_create */

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"

static boolean
dri_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                            const enum st_attachment_type *statts,
                            unsigned count,
                            struct pipe_resource **out)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   unsigned statt_mask, new_mask;
   boolean new_stamp;
   int i;

   statt_mask = 0x0;
   for (i = 0; i < count; i++)
      statt_mask |= (1 << statts[i]);

   /* record newly allocated textures */
   new_mask = (statt_mask & ~drawable->texture_mask);

   /*
    * dPriv->pStamp is the server stamp.  It should be accessed with a lock, at
    * least for DRI1.  dPriv->lastStamp is the client stamp.  It has the value
    * of the server stamp when last checked.
    */
   new_stamp = (drawable->texture_stamp != drawable->dPriv->lastStamp);

   if (new_stamp || new_mask) {
      if (new_stamp && screen->update_drawable_info)
         screen->update_drawable_info(drawable);

      screen->allocate_textures(drawable, statts, count);

      /* add existing textures */
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
         if (drawable->textures[i])
            statt_mask |= (1 << i);
      }

      drawable->texture_stamp = drawable->dPriv->lastStamp;
      drawable->texture_mask = statt_mask;
   }

   if (!out)
      return TRUE;

   for (i = 0; i < count; i++) {
      out[i] = NULL;
      pipe_resource_reference(&out[i], drawable->textures[statts[i]]);
   }

   return TRUE;
}

static boolean
dri_st_framebuffer_flush_front(struct st_framebuffer_iface *stfbi,
                               enum st_attachment_type statt)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
   struct dri_screen *screen = dri_screen(drawable->sPriv);

   /* XXX remove this and just set the correct one on the framebuffer */
   screen->flush_frontbuffer(drawable, statt);

   return TRUE;
}

/**
 * Init a framebuffer from the given drawable.
 */
void
dri_init_st_framebuffer(struct dri_drawable *drawable)
{
   drawable->base.visual = &drawable->stvis;
   drawable->base.flush_front = dri_st_framebuffer_flush_front;
   drawable->base.validate = dri_st_framebuffer_validate;
   drawable->base.st_manager_private = (void *) drawable;
}

/**
 * Destroy a framebuffer.
 */
void
dri_close_st_framebuffer(struct dri_drawable *drawable)
{
   int i;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_resource_reference(&drawable->textures[i], NULL);
}

/**
 * Validate the texture at an attachment.  Allocate the texture if it does not
 * exist.
 */
void
dri_st_framebuffer_validate_att(struct dri_drawable *drawable,
                                enum st_attachment_type statt)
{
   enum st_attachment_type statts[ST_ATTACHMENT_COUNT];
   unsigned i, count = 0;

   /* check if buffer already exists */
   if (drawable->texture_mask & (1 << statt))
      return;

   /* make sure DRI2 does not destroy existing buffers */
   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      if (drawable->texture_mask & (1 << i)) {
         statts[count++] = i;
      }
   }
   statts[count++] = statt;

   drawable->texture_stamp = drawable->dPriv->lastStamp - 1;

   /* this calles into the manager */
   drawable->base.validate(&drawable->base, statts, count, NULL);
}

static boolean
dri_st_manager_get_egl_image(struct st_manager *smapi,
                             struct st_egl_image *stimg)
{
   struct dri_context *ctx =
      (struct dri_context *)stimg->stctxi->st_manager_private;
   struct dri_screen *screen = dri_screen(ctx->sPriv);
   __DRIimage *img = NULL;

   if (screen->lookup_egl_image) {
      img = screen->lookup_egl_image(ctx, stimg->egl_image);
   }

   if (!img)
      return FALSE;

   stimg->texture = NULL;
   pipe_resource_reference(&stimg->texture, img->texture);
   stimg->face = img->face;
   stimg->level = img->level;
   stimg->zslice = img->zslice;

   return TRUE;
}

/**
 * Create a state tracker manager from the given screen.
 */
boolean
dri_init_st_manager(struct dri_screen *screen)
{
   screen->base.screen = screen->pipe_screen;
   screen->base.get_egl_image = dri_st_manager_get_egl_image;
   screen->st_api = st_gl_api_create();

   if (!screen->st_api)
      return FALSE;

   return TRUE;
}

void
dri_close_st_manager(struct dri_screen *screen)
{
   if (screen->st_api && screen->st_api->destroy)
      screen->st_api->destroy(screen->st_api);
}
