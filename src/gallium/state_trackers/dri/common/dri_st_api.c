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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_debug.h"
#include "state_tracker/st_manager.h" /* for st_manager_create_api */

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"
#ifndef __NOT_HAVE_DRM_H
#include "dri1.h"
#include "dri2.h"
#else
#include "drisw.h"
#endif

static boolean
dri_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                            const enum st_attachment_type *statts,
                            unsigned count,
                            struct pipe_texture **out)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
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

#ifndef __NOT_HAVE_DRM_H
      if (__dri1_api_hooks) {
         dri1_allocate_textures(drawable, statt_mask);
      }
      else {
         dri2_allocate_textures(drawable, statts, count);
      }
#else
      if (new_stamp)
         drisw_update_drawable_info(drawable);

      drisw_allocate_textures(drawable, statt_mask);
#endif

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
      pipe_texture_reference(&out[i], drawable->textures[statts[i]]);
   }

   return TRUE;
}

static boolean
dri_st_framebuffer_flush_front(struct st_framebuffer_iface *stfbi,
                               enum st_attachment_type statt)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;

#ifndef __NOT_HAVE_DRM_H
   if (__dri1_api_hooks) {
      dri1_flush_frontbuffer(drawable, statt);
   }
   else {
      dri2_flush_frontbuffer(drawable, statt);
   }
#else
   drisw_flush_frontbuffer(drawable, statt);
#endif

   return TRUE;
}

/**
 * Create a framebuffer from the given drawable.
 */
struct st_framebuffer_iface *
dri_create_st_framebuffer(struct dri_drawable *drawable)
{
   struct st_framebuffer_iface *stfbi;

   stfbi = CALLOC_STRUCT(st_framebuffer_iface);
   if (stfbi) {
      stfbi->visual = &drawable->stvis;
      stfbi->flush_front = dri_st_framebuffer_flush_front;
      stfbi->validate = dri_st_framebuffer_validate;
      stfbi->st_manager_private = (void *) drawable;
   }

   return stfbi;
}

/**
 * Destroy a framebuffer.
 */
void
dri_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
   int i;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_texture_reference(&drawable->textures[i], NULL);

   FREE(stfbi);
}

/**
 * Validate the texture at an attachment.  Allocate the texture if it does not
 * exist.
 */
void
dri_st_framebuffer_validate_att(struct st_framebuffer_iface *stfbi,
                                enum st_attachment_type statt)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
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

   stfbi->validate(stfbi, statts, count, NULL);
}

/**
 * Reference counted st_api.
 */
static struct {
   int32_t refcnt;
   struct st_api *stapi;
} dri_st_api;

/**
 * Add a reference to the st_api of the state tracker.
 */
static void
_dri_get_st_api(void)
{
   p_atomic_inc(&dri_st_api.refcnt);
   if (p_atomic_read(&dri_st_api.refcnt) == 1)
      dri_st_api.stapi = st_manager_create_api();
}

/**
 * Remove a reference to the st_api of the state tracker.
 */
static void
_dri_put_st_api(void)
{
   struct st_api *stapi = dri_st_api.stapi;

   if (p_atomic_dec_zero(&dri_st_api.refcnt)) {
      stapi->destroy(dri_st_api.stapi);
      dri_st_api.stapi = NULL;
   }
}

static boolean
dri_st_manager_get_egl_image(struct st_manager *smapi,
                             struct st_egl_image *stimg)
{
   __DRIimage *img = NULL;

#ifndef __NOT_HAVE_DRM_H
   if (!__dri1_api_hooks) {
      struct dri_context *ctx = (struct dri_context *)
         stimg->stctxi->st_manager_private;
      img = dri2_lookup_egl_image(ctx, stimg->egl_image);
   }
#endif
   if (!img)
      return FALSE;

   stimg->texture = NULL;
   pipe_texture_reference(&stimg->texture, img->texture);
   stimg->face = img->face;
   stimg->level = img->level;
   stimg->zslice = img->zslice;

   return TRUE;
}

/**
 * Create a state tracker manager from the given screen.
 */
struct st_manager *
dri_create_st_manager(struct dri_screen *screen)
{
   struct st_manager *smapi;

   smapi = CALLOC_STRUCT(st_manager);
   if (smapi) {
      smapi->screen = screen->pipe_screen;
      smapi->get_egl_image = dri_st_manager_get_egl_image;
      _dri_get_st_api();
   }

   return smapi;
}

/**
 * Destroy a state tracker manager.
 */
void
dri_destroy_st_manager(struct st_manager *smapi)
{
   _dri_put_st_api();
   FREE(smapi);
}

/**
 * Return the st_api of OpenGL state tracker.
 */
struct st_api *
dri_get_st_api(void)
{
   assert(dri_st_api.stapi);
   return dri_st_api.stapi;
}
