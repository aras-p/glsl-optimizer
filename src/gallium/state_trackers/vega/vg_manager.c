/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "state_tracker/st_api.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_sampler.h"

#include "vg_api.h"
#include "vg_manager.h"
#include "vg_context.h"
#include "image.h"
#include "mask.h"
#include "api.h"

static struct pipe_resource *
create_texture(struct pipe_context *pipe, enum pipe_format format,
                    VGint width, VGint height)
{
   struct pipe_resource templ;

   memset(&templ, 0, sizeof(templ));

   if (format != PIPE_FORMAT_NONE) {
      templ.format = format;
   }
   else {
      templ.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   }

   templ.target = PIPE_TEXTURE_2D;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.last_level = 0;

   if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 1)) {
      templ.bind = PIPE_BIND_DEPTH_STENCIL;
   } else {
      templ.bind = (PIPE_BIND_DISPLAY_TARGET |
                    PIPE_BIND_RENDER_TARGET |
                    PIPE_BIND_SAMPLER_VIEW);
   }

   return pipe->screen->resource_create(pipe->screen, &templ);
}

static struct pipe_sampler_view *
create_tex_and_view(struct pipe_context *pipe, enum pipe_format format,
                    VGint width, VGint height)
{
   struct pipe_resource *texture;
   struct pipe_sampler_view view_templ;
   struct pipe_sampler_view *view;

   texture = create_texture(pipe, format, width, height);

   if (!texture)
      return NULL;

   u_sampler_view_default_template(&view_templ, texture, texture->format);
   view = pipe->create_sampler_view(pipe, texture, &view_templ);
   /* want the texture to go away if the view is freed */
   pipe_resource_reference(&texture, NULL);

   return view;
}

static void
vg_context_update_alpha_mask_view(struct vg_context *ctx,
                                  uint width, uint height)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_sampler_view *old_sampler_view = stfb->alpha_mask_view;
   struct pipe_context *pipe = ctx->pipe;

   if (old_sampler_view &&
       old_sampler_view->texture->width0 == width &&
       old_sampler_view->texture->height0 == height)
      return;

   /*
     we use PIPE_FORMAT_B8G8R8A8_UNORM because we want to render to
     this texture and use it as a sampler, so while this wastes some
     space it makes both of those a lot simpler
   */
   stfb->alpha_mask_view = create_tex_and_view(pipe,
         PIPE_FORMAT_B8G8R8A8_UNORM, width, height);

   if (!stfb->alpha_mask_view) {
      if (old_sampler_view)
         pipe_sampler_view_reference(&old_sampler_view, NULL);
      return;
   }

   /* XXX could this call be avoided? */
   vg_validate_state(ctx);

   /* alpha mask starts with 1.f alpha */
   mask_fill(0, 0, width, height, 1.f);

   /* if we had an old surface copy it over */
   if (old_sampler_view) {
      struct pipe_subresource subsurf, subold_surf;
      subsurf.face = 0;
      subsurf.level = 0;
      subold_surf.face = 0;
      subold_surf.level = 0;
      pipe->resource_copy_region(pipe,
                                 stfb->alpha_mask_view->texture,
                                 subsurf,
                                 0, 0, 0,
                                 old_sampler_view->texture,
                                 subold_surf,
                                 0, 0, 0,
                                 MIN2(old_sampler_view->texture->width0,
                                      stfb->alpha_mask_view->texture->width0),
                                 MIN2(old_sampler_view->texture->height0,
                                      stfb->alpha_mask_view->texture->height0));
   }

   /* Free the old texture
    */
   if (old_sampler_view)
      pipe_sampler_view_reference(&old_sampler_view, NULL);
}

static void
vg_context_update_blend_texture_view(struct vg_context *ctx,
                                     uint width, uint height)
{
   struct pipe_context *pipe = ctx->pipe;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_sampler_view *old = stfb->blend_texture_view;

   if (old &&
       old->texture->width0 == width &&
       old->texture->height0 == height)
      return;

   stfb->blend_texture_view = create_tex_and_view(pipe,
         PIPE_FORMAT_B8G8R8A8_UNORM, width, height);

   pipe_sampler_view_reference(&old, NULL);
}

static boolean
vg_context_update_depth_stencil_rb(struct vg_context * ctx,
                                   uint width, uint height)
{
   struct st_renderbuffer *dsrb = ctx->draw_buffer->dsrb;
   struct pipe_context *pipe = ctx->pipe;
   unsigned surface_usage;

   if ((dsrb->width == width && dsrb->height == height) && dsrb->texture)
      return FALSE;

   /* unreference existing ones */
   pipe_surface_reference(&dsrb->surface, NULL);
   pipe_resource_reference(&dsrb->texture, NULL);
   dsrb->width = dsrb->height = 0;

   /* Probably need dedicated flags for surface usage too:
    */
   surface_usage = PIPE_BIND_DEPTH_STENCIL; /* XXX: was: RENDER_TARGET */

   dsrb->texture = create_texture(pipe, dsrb->format, width, height);
   if (!dsrb->texture)
      return TRUE;

   dsrb->surface = pipe->screen->get_tex_surface(pipe->screen,
                                                 dsrb->texture,
                                                 0, 0, 0,
                                                 surface_usage);
   if (!dsrb->surface) {
      pipe_resource_reference(&dsrb->texture, NULL);
      return TRUE;
   }

   dsrb->width = width;
   dsrb->height = height;

   assert(dsrb->surface->width == width);
   assert(dsrb->surface->height == height);

   return TRUE;
}

static boolean
vg_context_update_color_rb(struct vg_context *ctx, struct pipe_resource *pt)
{
   struct st_renderbuffer *strb = ctx->draw_buffer->strb;
   struct pipe_screen *screen = ctx->pipe->screen;

   if (strb->texture == pt) {
      pipe_resource_reference(&pt, NULL);
      return FALSE;
   }

   /* unreference existing ones */
   pipe_surface_reference(&strb->surface, NULL);
   pipe_resource_reference(&strb->texture, NULL);
   strb->width = strb->height = 0;

   strb->texture = pt;
   strb->surface = screen->get_tex_surface(screen, strb->texture, 0, 0, 0,
                                           PIPE_BIND_RENDER_TARGET);
   if (!strb->surface) {
      pipe_resource_reference(&strb->texture, NULL);
      return TRUE;
   }

   strb->width = pt->width0;
   strb->height = pt->height0;

   return TRUE;
}

/**
 * Flush the front buffer if the current context renders to the front buffer.
 */
void
vg_manager_flush_frontbuffer(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;

   if (!stfb)
      return;

   switch (stfb->strb_att) {
   case ST_ATTACHMENT_FRONT_LEFT:
   case ST_ATTACHMENT_FRONT_RIGHT:
      stfb->iface->flush_front(stfb->iface, stfb->strb_att);
      break;
   default:
      break;
   }
}

/**
 * Re-validate the framebuffer.
 */
void
vg_manager_validate_framebuffer(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct pipe_resource *pt;

   /* no binding surface */
   if (!stfb)
      return;

   if (!p_atomic_read(&ctx->draw_buffer_invalid))
      return;

   /* validate the fb */
   if (!stfb->iface->validate(stfb->iface, &stfb->strb_att, 1, &pt) || !pt)
      return;

   p_atomic_set(&ctx->draw_buffer_invalid, FALSE);

   if (vg_context_update_color_rb(ctx, pt) ||
       stfb->width != pt->width0 ||
       stfb->height != pt->height0)
      ctx->state.dirty |= FRAMEBUFFER_DIRTY;

   if (vg_context_update_depth_stencil_rb(ctx, pt->width0, pt->height0))
      ctx->state.dirty |= DEPTH_STENCIL_DIRTY;

   stfb->width = pt->width0;
   stfb->height = pt->height0;

   /* TODO create as needed */
   vg_context_update_alpha_mask_view(ctx, stfb->width, stfb->height);
   vg_context_update_blend_texture_view(ctx, stfb->width, stfb->height);
}

static void
vg_context_notify_invalid_framebuffer(struct st_context_iface *stctxi,
                                      struct st_framebuffer_iface *stfbi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   p_atomic_set(&ctx->draw_buffer_invalid, TRUE);
}

static void
vg_context_flush(struct st_context_iface *stctxi, unsigned flags,
                 struct pipe_fence_handle **fence)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   ctx->pipe->flush(ctx->pipe, flags, fence);
   if (flags & PIPE_FLUSH_RENDER_CACHE)
      vg_manager_flush_frontbuffer(ctx);
}

static void
vg_context_destroy(struct st_context_iface *stctxi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   vg_destroy_context(ctx);
}

static struct st_context_iface *
vg_api_create_context(struct st_api *stapi, struct st_manager *smapi,
                      const struct st_context_attribs *attribs,
                      struct st_context_iface *shared_stctxi)
{
   struct vg_context *shared_ctx = (struct vg_context *) shared_stctxi;
   struct vg_context *ctx;
   struct pipe_context *pipe;

   if (!(stapi->profile_mask & (1 << attribs->profile)))
      return NULL;

   /* only 1.0 is supported */
   if (attribs->major > 1 || (attribs->major == 1 && attribs->minor > 0))
      return NULL;

   pipe = smapi->screen->context_create(smapi->screen, NULL);
   if (!pipe)
      return NULL;
   ctx = vg_create_context(pipe, NULL, shared_ctx);
   if (!ctx) {
      pipe->destroy(pipe);
      return NULL;
   }

   ctx->iface.destroy = vg_context_destroy;

   ctx->iface.notify_invalid_framebuffer =
      vg_context_notify_invalid_framebuffer;
   ctx->iface.flush = vg_context_flush;

   ctx->iface.teximage = NULL;
   ctx->iface.copy = NULL;

   ctx->iface.st_context_private = (void *) smapi;

   return &ctx->iface;
}

static struct st_renderbuffer *
create_renderbuffer(enum pipe_format format)
{
   struct st_renderbuffer *strb;

   strb = CALLOC_STRUCT(st_renderbuffer);
   if (strb)
      strb->format = format;

   return strb;
}

static void
destroy_renderbuffer(struct st_renderbuffer *strb)
{
   pipe_surface_reference(&strb->surface, NULL);
   pipe_resource_reference(&strb->texture, NULL);
   FREE(strb);
}

/**
 * Decide the buffer to render to.
 */
static enum st_attachment_type
choose_attachment(struct st_framebuffer_iface *stfbi)
{
   enum st_attachment_type statt;

   statt = stfbi->visual->render_buffer;
   if (statt != ST_ATTACHMENT_INVALID) {
      /* use the buffer given by the visual, unless it is unavailable */
      if (!st_visual_have_buffers(stfbi->visual, 1 << statt)) {
         switch (statt) {
         case ST_ATTACHMENT_BACK_LEFT:
            statt = ST_ATTACHMENT_FRONT_LEFT;
            break;
         case ST_ATTACHMENT_BACK_RIGHT:
            statt = ST_ATTACHMENT_FRONT_RIGHT;
            break;
         default:
            break;
         }

         if (!st_visual_have_buffers(stfbi->visual, 1 << statt))
            statt = ST_ATTACHMENT_INVALID;
      }
   }

   return statt;
}

/**
 * Bind the context to the given framebuffers.
 */
static boolean
vg_context_bind_framebuffers(struct st_context_iface *stctxi,
                             struct st_framebuffer_iface *stdrawi,
                             struct st_framebuffer_iface *streadi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;
   struct st_framebuffer *stfb;
   enum st_attachment_type strb_att;

   /* the draw and read framebuffers must be the same */
   if (stdrawi != streadi)
      return FALSE;

   p_atomic_set(&ctx->draw_buffer_invalid, TRUE);

   strb_att = (stdrawi) ? choose_attachment(stdrawi) : ST_ATTACHMENT_INVALID;

   if (ctx->draw_buffer) {
      stfb = ctx->draw_buffer;

      /* free the existing fb */
      if (!stdrawi ||
          stfb->strb_att != strb_att ||
          stfb->strb->format != stdrawi->visual->color_format) {
         destroy_renderbuffer(stfb->strb);
         destroy_renderbuffer(stfb->dsrb);
         FREE(stfb);

         ctx->draw_buffer = NULL;
      }
   }

   if (!stdrawi)
      return TRUE;

   if (strb_att == ST_ATTACHMENT_INVALID)
      return FALSE;

   /* create a new fb */
   if (!ctx->draw_buffer) {
      stfb = CALLOC_STRUCT(st_framebuffer);
      if (!stfb)
         return FALSE;

      stfb->strb = create_renderbuffer(stdrawi->visual->color_format);
      if (!stfb->strb) {
         FREE(stfb);
         return FALSE;
      }

      stfb->dsrb = create_renderbuffer(ctx->ds_format);
      if (!stfb->dsrb) {
         FREE(stfb->strb);
         FREE(stfb);
         return FALSE;
      }

      stfb->width = 0;
      stfb->height = 0;
      stfb->strb_att = strb_att;

      ctx->draw_buffer = stfb;
   }

   ctx->draw_buffer->iface = stdrawi;

   return TRUE;
}

static boolean
vg_api_make_current(struct st_api *stapi, struct st_context_iface *stctxi,
                    struct st_framebuffer_iface *stdrawi,
                    struct st_framebuffer_iface *streadi)
{
   struct vg_context *ctx = (struct vg_context *) stctxi;

   if (stctxi)
      vg_context_bind_framebuffers(stctxi, stdrawi, streadi);
   vg_set_current_context(ctx);

   return TRUE;
}

static struct st_context_iface *
vg_api_get_current(struct st_api *stapi)
{
   struct vg_context *ctx = vg_current_context();

   return (ctx) ? &ctx->iface : NULL;
}

static st_proc_t
vg_api_get_proc_address(struct st_api *stapi, const char *procname)
{
   return api_get_proc_address(procname);
}

static void
vg_api_destroy(struct st_api *stapi)
{
}

static const struct st_api vg_api = {
   "Vega " VEGA_VERSION_STRING,
   ST_API_OPENVG,
   ST_PROFILE_DEFAULT_MASK,
   vg_api_destroy,
   vg_api_get_proc_address,
   vg_api_create_context,
   vg_api_make_current,
   vg_api_get_current,
};

const struct st_api *
vg_api_get(void)
{
   return &vg_api;
}
