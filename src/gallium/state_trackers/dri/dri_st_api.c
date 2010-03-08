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
#include "state_tracker/drm_api.h"
#include "state_tracker/st_manager.h" /* for st_manager_create_api */

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"
#include "dri1.h"

static struct {
   int32_t refcnt;
   struct st_api *stapi;
} dri_st_api;

/**
 * Get the format of an attachment.
 */
static INLINE enum pipe_format
dri_drawable_get_format(struct dri_drawable *drawable,
                        enum st_attachment_type statt)
{
   enum pipe_format format;

   switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
   case ST_ATTACHMENT_BACK_LEFT:
   case ST_ATTACHMENT_FRONT_RIGHT:
   case ST_ATTACHMENT_BACK_RIGHT:
      format = drawable->stvis.color_format;
      break;
   case ST_ATTACHMENT_DEPTH_STENCIL:
      format = drawable->stvis.depth_stencil_format;
      break;
   default:
      format = PIPE_FORMAT_NONE;
      break;
   }

   return format;
}

/**
 * Process __DRIbuffer and convert them into pipe_textures.
 */
static void
dri_drawable_process_buffers(struct dri_drawable *drawable,
                             __DRIbuffer *buffers, unsigned count)
{
   struct dri_screen *screen = dri_screen(drawable->sPriv);
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct pipe_texture templ;
   struct winsys_handle whandle;
   boolean have_depth = FALSE;
   unsigned i;

   if (drawable->old_num == count &&
       drawable->old_w == dri_drawable->w &&
       drawable->old_h == dri_drawable->h &&
       memcmp(drawable->old, buffers, sizeof(__DRIbuffer) * count) == 0)
      return;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_texture_reference(&drawable->textures[i], NULL);

   memset(&templ, 0, sizeof(templ));
   templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = dri_drawable->w;
   templ.height0 = dri_drawable->h;
   templ.depth0 = 1;

   memset(&whandle, 0, sizeof(whandle));

   for (i = 0; i < count; i++) {
      __DRIbuffer *buf = &buffers[i];
      enum st_attachment_type statt;
      enum pipe_format format;

      switch (buf->attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
	 if (!screen->auto_fake_front) {
            statt = ST_ATTACHMENT_INVALID;
            break;
         }
	 /* fallthrough */
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
         statt = ST_ATTACHMENT_FRONT_LEFT;
	 break;
      case __DRI_BUFFER_BACK_LEFT:
         statt = ST_ATTACHMENT_BACK_LEFT;
	 break;
      case __DRI_BUFFER_DEPTH:
      case __DRI_BUFFER_DEPTH_STENCIL:
      case __DRI_BUFFER_STENCIL:
         statt = ST_ATTACHMENT_DEPTH_STENCIL;
         /* use only the first depth/stencil buffer */
         if (have_depth)
            statt = ST_ATTACHMENT_INVALID;
         else
            have_depth = TRUE;
	 break;
      default:
         statt = ST_ATTACHMENT_INVALID;
         break;
      }

      format = dri_drawable_get_format(drawable, statt);
      if (statt == ST_ATTACHMENT_INVALID || format == PIPE_FORMAT_NONE)
         continue;

      templ.format = format;
      whandle.handle = buf->name;
      whandle.stride = buf->pitch;
   
      drawable->textures[statt] =
         screen->pipe_screen->texture_from_handle(screen->pipe_screen,
               &templ, &whandle);
   }

   drawable->old_num = count;
   drawable->old_w = dri_drawable->w;
   drawable->old_h = dri_drawable->h;
   memcpy(drawable->old, buffers, sizeof(__DRIbuffer) * count);
}

/**
 * Retrieve __DRIbuffer from the DRI loader.
 */
static __DRIbuffer *
dri_drawable_get_buffers(struct dri_drawable *drawable,
                         const enum st_attachment_type *statts,
                         unsigned *count)
{
   __DRIdrawable *dri_drawable = drawable->dPriv;
   struct __DRIdri2LoaderExtensionRec *loader = drawable->sPriv->dri2.loader;
   boolean with_format;
   __DRIbuffer *buffers;
   int num_buffers;
   unsigned attachments[8];
   unsigned num_attachments, i;

   assert(loader);
   with_format = (loader->base.version > 2 && loader->getBuffersWithFormat);

   num_attachments = 0;
   for (i = 0; i < *count; i++) {
      enum pipe_format format;
      int att;

      format = dri_drawable_get_format(drawable, statts[i]);
      if (format == PIPE_FORMAT_NONE)
         continue;

      switch (statts[i]) {
      case ST_ATTACHMENT_FRONT_LEFT:
         att = __DRI_BUFFER_FRONT_LEFT;
         break;
      case ST_ATTACHMENT_BACK_LEFT:
         att = __DRI_BUFFER_BACK_LEFT;
         break;
      case ST_ATTACHMENT_FRONT_RIGHT:
         att = __DRI_BUFFER_FRONT_RIGHT;
         break;
      case ST_ATTACHMENT_BACK_RIGHT:
         att = __DRI_BUFFER_BACK_RIGHT;
         break;
      case ST_ATTACHMENT_DEPTH_STENCIL:
         att = __DRI_BUFFER_DEPTH_STENCIL;
         break;
      default:
         att = -1;
         break;
      }

      if (att >= 0) {
         attachments[num_attachments++] = att;
         if (with_format) {
            attachments[num_attachments++] =
               util_format_get_blocksizebits(format);
         }
      }
   }

   if (with_format) {
      num_attachments /= 2;
      buffers = loader->getBuffersWithFormat(dri_drawable,
            &dri_drawable->w, &dri_drawable->h,
            attachments, num_attachments,
            &num_buffers, dri_drawable->loaderPrivate);
   }
   else {
      buffers = loader->getBuffers(dri_drawable,
            &dri_drawable->w, &dri_drawable->h,
            attachments, num_attachments,
            &num_buffers, dri_drawable->loaderPrivate);
   }

   if (buffers) {
      /* set one cliprect to cover the whole dri_drawable */
      dri_drawable->x = 0;
      dri_drawable->y = 0;
      dri_drawable->backX = 0;
      dri_drawable->backY = 0;
      dri_drawable->numClipRects = 1;
      dri_drawable->pClipRects[0].x1 = 0;
      dri_drawable->pClipRects[0].y1 = 0;
      dri_drawable->pClipRects[0].x2 = dri_drawable->w;
      dri_drawable->pClipRects[0].y2 = dri_drawable->h;
      dri_drawable->numBackClipRects = 1;
      dri_drawable->pBackClipRects[0].x1 = 0;
      dri_drawable->pBackClipRects[0].y1 = 0;
      dri_drawable->pBackClipRects[0].x2 = dri_drawable->w;
      dri_drawable->pBackClipRects[0].y2 = dri_drawable->h;

      *count = num_buffers;
   }

   return buffers;
}

static boolean 
dri_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                            const enum st_attachment_type *statts,
                            unsigned count,
                            struct pipe_texture **out)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
   unsigned statt_mask, i;

   statt_mask = 0x0;
   for (i = 0; i < count; i++)
      statt_mask |= (1 << statts[i]);

   /*
    * dPriv->pStamp is the server stamp.  It should be accessed with a lock, at
    * least for DRI1.  dPriv->lastStamp is the client stamp.  It has the value
    * of the server stamp when last checked.
    *
    * This function updates the textures and records the stamp of the textures.
    */
   if (drawable->texture_stamp != drawable->dPriv->lastStamp ||
       (statt_mask & ~drawable->texture_mask)) {
      if (__dri1_api_hooks) {
         /* TODO */
         return FALSE;
      }
      else {
         __DRIbuffer *buffers;
         unsigned num_buffers = count;

         buffers = dri_drawable_get_buffers(drawable, statts, &num_buffers);
         dri_drawable_process_buffers(drawable, buffers, num_buffers);
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
   struct __DRIdri2LoaderExtensionRec *loader =
      drawable->sPriv->dri2.loader;

   /* TODO */
   if (__dri1_api_hooks)
      return FALSE;

   if (statt == ST_ATTACHMENT_FRONT_LEFT && loader->flushFrontBuffer) {
      loader->flushFrontBuffer(drawable->dPriv,
            drawable->dPriv->loaderPrivate);
   }

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
   FREE(stfbi);
}

/**
 * Return the texture at an attachment.  Allocate the texture if it does not
 * exist.
 */
struct pipe_texture *
dri_get_st_framebuffer_texture(struct st_framebuffer_iface *stfbi,
                               enum st_attachment_type statt)
{
   struct dri_drawable *drawable =
      (struct dri_drawable *) stfbi->st_manager_private;
   
   if (!(drawable->texture_mask & (1 << statt))) {
      enum st_attachment_type statts[ST_ATTACHMENT_COUNT];
      unsigned i, count = 0;

      /* make sure DRI2 does not destroy existing buffers */
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
         if (drawable->texture_mask & (1 << i)) {
            statts[count++] = i;
         }
      }
      statts[count++] = statt;

      drawable->texture_stamp = drawable->dPriv->lastStamp - 1;
      dri_st_framebuffer_validate(stfbi, statts, count, NULL);
   }

   return drawable->textures[statt];
}

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
