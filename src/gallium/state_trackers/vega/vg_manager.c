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

#include "state_tracker/st_api.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "vg_manager.h"
#include "vg_context.h"
#include "vg_tracker.h" /* for st_resize_framebuffer */
#include "image.h"

/**
 * Flush the front buffer if the current context renders to the front buffer.
 */
void
vg_manager_flush_frontbuffer(struct vg_context *ctx)
{
   struct st_framebuffer *stfb = ctx->draw_buffer;

   if (!stfb)
      return;

   /* st_public.h is used */
   if (!stfb->iface) {
      struct pipe_screen *screen = ctx->pipe->screen;
      if (screen->flush_frontbuffer) {
         screen->flush_frontbuffer(screen,
               stfb->strb->surface, ctx->pipe->priv);
      }
      return;
   }

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
   struct pipe_screen *screen = ctx->pipe->screen;
   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *rb;
   struct pipe_texture *pt;

   /* no binding surface */
   if (!stfb)
      return;

   /* st_public.h is used */
   if (!stfb->iface) {
      struct pipe_screen *screen = ctx->pipe->screen;
      if (screen->update_buffer)
         screen->update_buffer(screen, ctx->pipe->priv);
      return;
   }

   if (!p_atomic_read(&ctx->draw_buffer_invalid))
      return;

   /* validate the fb */
   if (!stfb->iface->validate(stfb->iface, &stfb->strb_att, 1, &pt) || !pt)
      return;

   rb = stfb->strb;
   if (rb->texture == pt) {
      pipe_texture_reference(&pt, NULL);
      return;
   }

   /* unreference existing ones */
   pipe_surface_reference(&rb->surface, NULL);
   pipe_texture_reference(&rb->texture, NULL);

   rb->texture = pt;
   rb->surface = screen->get_tex_surface(screen, rb->texture, 0, 0, 0,
         PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE);

   rb->width = rb->surface->width;
   rb->height = rb->surface->height;

   st_resize_framebuffer(stfb, rb->width, rb->height);

   p_atomic_set(&ctx->draw_buffer_invalid, FALSE);
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
                      const struct st_visual *visual,
                      struct st_context_iface *shared_stctxi)
{
   struct vg_context *shared_ctx = (struct vg_context *) shared_stctxi;
   struct vg_context *ctx;
   struct pipe_context *pipe;

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
   pipe_texture_reference(&strb->texture, NULL);
   free(strb);
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
          stfb->strb->format != stdrawi->visual->color_format ||
          stfb->dsrb->format != stdrawi->visual->depth_stencil_format) {
         destroy_renderbuffer(stfb->strb);
         destroy_renderbuffer(stfb->dsrb);
         free(stfb);

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
         free(stfb);
         return FALSE;
      }

      stfb->dsrb = create_renderbuffer(stdrawi->visual->depth_stencil_format);
      if (!stfb->dsrb) {
         free(stfb->strb);
         free(stfb);
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

static boolean
vg_api_is_visual_supported(struct st_api *stapi,
                           const struct st_visual *visual)
{
   /* the impl requires a depth/stencil buffer */
   if (visual->depth_stencil_format == PIPE_FORMAT_NONE)
      return FALSE;

   return TRUE;
}

static st_proc_t
vg_api_get_proc_address(struct st_api *stapi, const char *procname)
{
   /* TODO */
   return (st_proc_t) NULL;
}

static void
vg_api_destroy(struct st_api *stapi)
{
   free(stapi);
}

static struct st_api *
vg_module_create_api(void)
{
   struct st_api *stapi;

   stapi = CALLOC_STRUCT(st_api);
   if (stapi) {
      stapi->destroy = vg_api_destroy;
      stapi->get_proc_address = vg_api_get_proc_address;
      stapi->is_visual_supported = vg_api_is_visual_supported;

      stapi->create_context = vg_api_create_context;
      stapi->make_current = vg_api_make_current;
      stapi->get_current = vg_api_get_current;
   }

   return stapi;
}

PUBLIC const struct st_module st_module_OpenVG = {
   .api = ST_API_OPENVG,
   .create_api = vg_module_create_api,
};
