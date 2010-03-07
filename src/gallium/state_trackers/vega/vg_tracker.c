/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

#include "vg_context.h"
#include "vg_tracker.h"
#include "mask.h"

#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_rect.h"

/* advertise OpenVG support */
PUBLIC const int st_api_OpenVG = 1;

static struct pipe_texture *
create_texture(struct pipe_context *pipe, enum pipe_format format,
               VGint width, VGint height)
{
   struct pipe_texture templ;

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
      templ.tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
   } else {
      templ.tex_usage = (PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                         PIPE_TEXTURE_USAGE_RENDER_TARGET |
                         PIPE_TEXTURE_USAGE_SAMPLER);
   }

   return pipe->screen->texture_create(pipe->screen, &templ);
}

/**
 * Allocate a renderbuffer for a an on-screen window (not a user-created
 * renderbuffer).  The window system code determines the format.
 */
static struct st_renderbuffer *
st_new_renderbuffer_fb(enum pipe_format format)
{
   struct st_renderbuffer *strb;

   strb = CALLOC_STRUCT(st_renderbuffer);
   if (!strb) {
      /*_vega_error(NULL, VG_OUT_OF_MEMORY, "creating renderbuffer");*/
      return NULL;
   }

   strb->format = format;

   return strb;
}


/**
 * This is called to allocate the original drawing surface, and
 * during window resize.
 */
static VGboolean
st_renderbuffer_alloc_storage(struct vg_context * ctx,
                              struct st_renderbuffer *strb,
                              VGuint width, VGuint height)
{
   struct pipe_context *pipe = ctx->pipe;
   unsigned surface_usage;

   /* Free the old surface and texture
    */
   pipe_surface_reference(&strb->surface, NULL);
   pipe_texture_reference(&strb->texture, NULL);


   /* Probably need dedicated flags for surface usage too:
    */
   surface_usage = (PIPE_BUFFER_USAGE_GPU_READ  |
                    PIPE_BUFFER_USAGE_GPU_WRITE);

   strb->texture = create_texture(pipe, strb->format,
                                  width, height);

   if (!strb->texture)
      return FALSE;

   strb->surface = pipe->screen->get_tex_surface(pipe->screen,
                                                 strb->texture,
                                                 0, 0, 0,
                                                 surface_usage);
   strb->width = width;
   strb->height = height;

   assert(strb->surface->width == width);
   assert(strb->surface->height == height);

   return strb->surface != NULL;
}

struct vg_context * st_create_context(struct pipe_context *pipe,
                                      const void *visual,
                                      struct vg_context *share)
{
   struct vg_context *ctx = vg_create_context(pipe, visual, share);
   /*debug_printf("--------- CREATE CONTEXT %p\n", ctx);*/
   return ctx;
}

void st_destroy_context(struct vg_context *st)
{
   /*debug_printf("--------- DESTROY CONTEXT %p\n", st);*/
   vg_destroy_context(st);
}

void st_copy_context_state(struct vg_context *dst, struct vg_context *src,
                           uint mask)
{
   fprintf(stderr, "FIXME: %s\n", __FUNCTION__);
}

void st_get_framebuffer_dimensions(struct st_framebuffer *stfb,
				   uint *width,
				   uint *height)
{
   *width = stfb->strb->width;
   *height = stfb->strb->height;
}

struct st_framebuffer * st_create_framebuffer(const void *visual,
                                              enum pipe_format colorFormat,
                                              enum pipe_format depthFormat,
                                              enum pipe_format stencilFormat,
                                              uint width, uint height,
                                              void *privateData)
{
   struct st_framebuffer *stfb = CALLOC_STRUCT(st_framebuffer);
   if (stfb) {
      struct st_renderbuffer *rb =
         st_new_renderbuffer_fb(colorFormat);
      stfb->strb = rb;
#if 0
      if (doubleBuffer) {
         struct st_renderbuffer *rb =
            st_new_renderbuffer_fb(colorFormat);
      }
#endif

      /* we want to combine the depth/stencil */
      if (stencilFormat == depthFormat)
         stfb->dsrb = st_new_renderbuffer_fb(stencilFormat);
      else
         stfb->dsrb = st_new_renderbuffer_fb(PIPE_FORMAT_Z24S8_UNORM);

      /*### currently we always allocate it but it's possible it's
        not necessary if EGL_ALPHA_MASK_SIZE was 0
      */
      stfb->alpha_mask = 0;

      stfb->width = width;
      stfb->height = height;
      stfb->privateData = privateData;
   }

   return stfb;
}

static void setup_new_alpha_mask(struct vg_context *ctx,
                                 struct st_framebuffer *stfb,
                                 uint width, uint height)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_texture *old_texture = stfb->alpha_mask;

   /*
     we use PIPE_FORMAT_B8G8R8A8_UNORM because we want to render to
     this texture and use it as a sampler, so while this wastes some
     space it makes both of those a lot simpler
   */
   stfb->alpha_mask =
      create_texture(pipe, PIPE_FORMAT_B8G8R8A8_UNORM, width, height);

   if (!stfb->alpha_mask) {
      if (old_texture)
         pipe_texture_reference(&old_texture, NULL);
      return;
   }

   vg_validate_state(ctx);

   /* alpha mask starts with 1.f alpha */
   mask_fill(0, 0, width, height, 1.f);

   /* if we had an old surface copy it over */
   if (old_texture) {
      struct pipe_surface *surface = pipe->screen->get_tex_surface(
         pipe->screen,
         stfb->alpha_mask,
         0, 0, 0,
         PIPE_BUFFER_USAGE_GPU_WRITE);
      struct pipe_surface *old_surface = pipe->screen->get_tex_surface(
         pipe->screen,
         old_texture,
         0, 0, 0,
         PIPE_BUFFER_USAGE_GPU_READ);
      if (pipe->surface_copy) {
         pipe->surface_copy(pipe,
                            surface,
                            0, 0,
                            old_surface,
                            0, 0,
                            MIN2(old_surface->width, width),
                            MIN2(old_surface->height, height));
      } else {
         util_surface_copy(pipe, FALSE,
                           surface,
                           0, 0,
                           old_surface,
                           0, 0,
                           MIN2(old_surface->width, width),
                           MIN2(old_surface->height, height));
      }
      if (surface)
         pipe_surface_reference(&surface, NULL);
      if (old_surface)
         pipe_surface_reference(&old_surface, NULL);
   }

   /* Free the old texture
    */
   if (old_texture)
      pipe_texture_reference(&old_texture, NULL);
}

void st_resize_framebuffer(struct st_framebuffer *stfb,
                           uint width, uint height)
{
   struct vg_context *ctx = vg_current_context();
   struct st_renderbuffer *strb = stfb->strb;
   struct pipe_framebuffer_state *state;

   if (!ctx)
      return;

   state = &ctx->state.g3d.fb;

   /* If this is a noop, exit early and don't do the clear, etc below.
    */
   if (stfb->width == width &&
       stfb->height == height &&
       state->zsbuf)
      return;

   stfb->width = width;
   stfb->height = height;

   if (strb->width != width || strb->height != height)
      st_renderbuffer_alloc_storage(ctx, strb,
                                 width, height);

   if (stfb->dsrb->width != width || stfb->dsrb->height != height)
      st_renderbuffer_alloc_storage(ctx, stfb->dsrb,
                                 width, height);

   {
      VGuint i;

      memset(state, 0, sizeof(struct pipe_framebuffer_state));

      state->width  = width;
      state->height = height;

      state->nr_cbufs = 1;
      state->cbufs[0] = strb->surface;
      for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
         state->cbufs[i] = 0;

      state->zsbuf = stfb->dsrb->surface;

      cso_set_framebuffer(ctx->cso_context, state);
   }

   ctx->state.dirty |= VIEWPORT_DIRTY;
   ctx->state.dirty |= DEPTH_STENCIL_DIRTY;/*to reset the scissors*/

   ctx->pipe->clear(ctx->pipe, PIPE_CLEAR_DEPTHSTENCIL,
                    NULL, 0.0, 0);

   /* we need all the other state already set */

   setup_new_alpha_mask(ctx, stfb, width, height);

   pipe_texture_reference( &stfb->blend_texture, NULL );
   stfb->blend_texture = create_texture(ctx->pipe, PIPE_FORMAT_B8G8R8A8_UNORM,
                                        width, height);
}

void st_set_framebuffer_surface(struct st_framebuffer *stfb,
                                uint surfIndex, struct pipe_surface *surf)
{
   struct st_renderbuffer *rb = stfb->strb;

   /* unreference existing surfaces */
   pipe_surface_reference( &rb->surface, NULL );
   pipe_texture_reference( &rb->texture, NULL );

   /* reference new ones */
   pipe_surface_reference( &rb->surface, surf );
   pipe_texture_reference( &rb->texture, surf->texture );

   rb->width  = surf->width;
   rb->height = surf->height;
}

int st_get_framebuffer_surface(struct st_framebuffer *stfb,
                               uint surfIndex, struct pipe_surface **surf)
{
   struct st_renderbuffer *rb = stfb->strb;
   *surf = rb->surface;
   return VG_TRUE;
}

int st_get_framebuffer_texture(struct st_framebuffer *stfb,
                               uint surfIndex, struct pipe_texture **tex)
{
   struct st_renderbuffer *rb = stfb->strb;
   *tex = rb->texture;
   return VG_TRUE;
}

void * st_framebuffer_private(struct st_framebuffer *stfb)
{
   return stfb->privateData;
}

void st_unreference_framebuffer(struct st_framebuffer *stfb)
{
   /* FIXME */
}

boolean st_make_current(struct vg_context *st,
                        struct st_framebuffer *draw,
                        struct st_framebuffer *read)
{
   vg_set_current_context(st);
   if (st) {
      st->draw_buffer = draw;
   }
   return VG_TRUE;
}

struct vg_context *st_get_current(void)
{
   return vg_current_context();
}

void st_flush(struct vg_context *st, uint pipeFlushFlags,
              struct pipe_fence_handle **fence)
{
   st->pipe->flush(st->pipe, pipeFlushFlags, fence);
}

void st_finish(struct vg_context *st)
{
   struct pipe_fence_handle *fence = NULL;

   st_flush(st, PIPE_FLUSH_RENDER_CACHE, &fence);

   st->pipe->screen->fence_finish(st->pipe->screen, fence, 0);
   st->pipe->screen->fence_reference(st->pipe->screen, &fence, NULL);
}

void st_notify_swapbuffers(struct st_framebuffer *stfb)
{
   struct vg_context *ctx = vg_current_context();
   if (ctx && ctx->draw_buffer == stfb) {
      st_flush(ctx,
	       PIPE_FLUSH_RENDER_CACHE | 
	       PIPE_FLUSH_SWAPBUFFERS |
	       PIPE_FLUSH_FRAME,
               NULL);
   }
}

void st_notify_swapbuffers_complete(struct st_framebuffer *stfb)
{
}

int st_bind_texture_surface(struct pipe_surface *ps, int target, int level,
                            enum pipe_format format)
{
   return 0;
}

int st_unbind_texture_surface(struct pipe_surface *ps, int target, int level)
{
   return 0;
}

st_proc st_get_proc_address(const char *procname)
{
   return NULL;
}
