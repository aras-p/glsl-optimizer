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
      if (new_stamp && drawable->update_drawable_info)
         drawable->update_drawable_info(drawable);

      drawable->allocate_textures(drawable, statts, count);

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

   /* XXX remove this and just set the correct one on the framebuffer */
   drawable->flush_frontbuffer(drawable, statt);

   return TRUE;
}

/**
 * This is called when we need to set up GL rendering to a new X window.
 */
boolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const struct gl_config * visual, boolean isPixmap)
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

   drawable->screen = screen;
   drawable->sPriv = sPriv;
   drawable->dPriv = dPriv;
   dPriv->driverPrivate = (void *)drawable;

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

   pipe_surface_reference(&drawable->drisw_surface, NULL);

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++)
      pipe_resource_reference(&drawable->textures[i], NULL);

   FREE(drawable);
}

/**
 * Validate the texture at an attachment.  Allocate the texture if it does not
 * exist.  Used by the TFP extension.
 */
static void
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

   drawable->base.validate(&drawable->base, statts, count, NULL);
}

/**
 * These are used for GLX_EXT_texture_from_pixmap
 */
static void
dri_set_tex_buffer2(__DRIcontext *pDRICtx, GLint target,
                    GLint format, __DRIdrawable *dPriv)
{
   struct dri_context *ctx = dri_context(pDRICtx);
   struct dri_drawable *drawable = dri_drawable(dPriv);
   struct pipe_resource *pt;

   dri_drawable_validate_att(drawable, ST_ATTACHMENT_FRONT_LEFT);

   pt = drawable->textures[ST_ATTACHMENT_FRONT_LEFT];

   if (pt) {
      enum pipe_format internal_format = pt->format;

      if (format == __DRI_TEXTURE_FORMAT_RGB)  {
         /* only need to cover the formats recognized by dri_fill_st_visual */
         switch (internal_format) {
         case PIPE_FORMAT_B8G8R8A8_UNORM:
            internal_format = PIPE_FORMAT_B8G8R8X8_UNORM;
            break;
         case PIPE_FORMAT_A8R8G8B8_UNORM:
            internal_format = PIPE_FORMAT_X8R8G8B8_UNORM;
            break;
         default:
            break;
         }
      }

      ctx->st->teximage(ctx->st,
            (target == GL_TEXTURE_2D) ? ST_TEXTURE_2D : ST_TEXTURE_RECT,
            0, internal_format, pt, FALSE);
   }
}

static void
dri_set_tex_buffer(__DRIcontext *pDRICtx, GLint target,
                   __DRIdrawable *dPriv)
{
   dri_set_tex_buffer2(pDRICtx, target, __DRI_TEXTURE_FORMAT_RGBA, dPriv);
}

const __DRItexBufferExtension driTexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   dri_set_tex_buffer,
   dri_set_tex_buffer2,
   NULL,
};

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
