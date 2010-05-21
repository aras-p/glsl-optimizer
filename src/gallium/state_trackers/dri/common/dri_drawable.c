/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri1_helper.h"

#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
 

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

   if (new_stamp || new_mask || screen->broken_invalidate) {
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
 * This is called when we need to set up GL rendering to a new X window.
 */
boolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const __GLcontextModes * visual, boolean isPixmap)
{
   struct dri_screen *screen = sPriv->private;
   struct dri_drawable *drawable = NULL;

   if (isPixmap)
      goto fail;		       /* not implemented */

   drawable = CALLOC_STRUCT(dri_drawable);
   if (drawable == NULL)
      goto fail;

   dri_fill_st_visual(&drawable->stvis, screen, visual);

   /* setup the st_framebuffer_iface */
   drawable->base.visual = &drawable->stvis;
   drawable->base.flush_front = dri_st_framebuffer_flush_front;
   drawable->base.validate = dri_st_framebuffer_validate;
   drawable->base.st_manager_private = (void *) drawable;

   drawable->sPriv = sPriv;
   drawable->dPriv = dPriv;
   dPriv->driverPrivate = (void *)drawable;

   drawable->desired_fences = 2;

   return GL_TRUE;
fail:
   FREE(drawable);
   return GL_FALSE;
}

void
dri_destroy_buffer(__DRIdrawable * dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);
   int i;

   dri1_swap_fences_clear(drawable);

   dri1_destroy_pipe_surface(drawable);

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_resource_reference(&drawable->textures[i], NULL);

   drawable->desired_fences = 0;

   FREE(drawable);
}

/**
 * Validate the texture at an attachment.  Allocate the texture if it does not
 * exist.
 */
void
dri_drawable_validate_att(struct dri_drawable *drawable,
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

/**
 * Get the format and binding of an attachment.
 */
void
dri_drawable_get_format(struct dri_drawable *drawable,
                        enum st_attachment_type statt,
                        enum pipe_format *format,
                        unsigned *bind)
{
   switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
   case ST_ATTACHMENT_BACK_LEFT:
   case ST_ATTACHMENT_FRONT_RIGHT:
   case ST_ATTACHMENT_BACK_RIGHT:
      *format = drawable->stvis.color_format;
      *bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
      break;
   case ST_ATTACHMENT_DEPTH_STENCIL:
      *format = drawable->stvis.depth_stencil_format;
      *bind = PIPE_BIND_DEPTH_STENCIL; /* XXX sampler? */
      break;
   default:
      *format = PIPE_FORMAT_NONE;
      *bind = 0;
      break;
   }
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
