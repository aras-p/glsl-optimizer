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

#include "xm_api.h"
#include "xm_st.h"

/* support OpenGL by default */
#ifndef XMESA_ST_MODULE
#define XMESA_ST_MODULE st_module_OpenGL
#endif

struct xmesa_st_framebuffer {
   struct pipe_screen *screen;
   XMesaBuffer buffer;

   struct st_visual stvis;

   unsigned texture_width, texture_height;
   struct pipe_texture *textures[ST_ATTACHMENT_COUNT];

   struct pipe_surface *display_surface;
};

static INLINE struct xmesa_st_framebuffer *
xmesa_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   return (struct xmesa_st_framebuffer *) stfbi->st_manager_private;
}

/**
 * Display an attachment to the xlib_drawable of the framebuffer.
 */
static boolean
xmesa_st_framebuffer_display(struct st_framebuffer_iface *stfbi,
                             enum st_attachment_type statt)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   struct pipe_texture *ptex = xstfb->textures[statt];
   struct pipe_surface *psurf;

   if (!ptex)
      return TRUE;

   psurf = xstfb->display_surface;
   /* (re)allocate the surface for the texture to be displayed */
   if (!psurf || psurf->texture != ptex) {
      pipe_surface_reference(&xstfb->display_surface, NULL);

      psurf = xstfb->screen->get_tex_surface(xstfb->screen,
            ptex, 0, 0, 0, PIPE_BUFFER_USAGE_CPU_READ);
      if (!psurf)
         return FALSE;

      xstfb->display_surface = psurf;
   }

   xstfb->screen->flush_frontbuffer(xstfb->screen, psurf, &xstfb->buffer->ws);

   return TRUE;
}

/**
 * Remove outdated textures and create the requested ones.
 */
static void
xmesa_st_framebuffer_validate_textures(struct st_framebuffer_iface *stfbi,
                                       const enum st_attachment_type *statts,
                                       unsigned count)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   struct pipe_texture templ;
   unsigned request_mask, i;

   request_mask = 0;
   for (i = 0; i < count; i++)
      request_mask |= 1 << statts[i];

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.width0 = xstfb->texture_width;
   templ.height0 = xstfb->texture_height;
   templ.depth0 = 1;
   templ.last_level = 0;

   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      struct pipe_texture *ptex = xstfb->textures[i];
      enum pipe_format format;
      unsigned tex_usage;

      /* remove outdated textures */
      if (ptex && (ptex->width0 != xstfb->texture_width ||
                   ptex->height0 != xstfb->texture_height)) {
         pipe_texture_reference(&xstfb->textures[i], NULL);
         ptex = NULL;
      }

      /* the texture already exists or not requested */
      if (ptex || !(request_mask & (1 << i)))
         continue;

      switch (i) {
      case ST_ATTACHMENT_FRONT_LEFT:
      case ST_ATTACHMENT_BACK_LEFT:
      case ST_ATTACHMENT_FRONT_RIGHT:
      case ST_ATTACHMENT_BACK_RIGHT:
         format = xstfb->stvis.color_format;
         tex_usage = PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                     PIPE_TEXTURE_USAGE_RENDER_TARGET;
         break;
      case ST_ATTACHMENT_DEPTH_STENCIL:
         format = xstfb->stvis.depth_stencil_format;
         tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
         break;
      default:
         format = PIPE_FORMAT_NONE;
         break;
      }

      if (format != PIPE_FORMAT_NONE) {
         templ.format = format;
         templ.tex_usage = tex_usage;

         xstfb->textures[i] =
            xstfb->screen->texture_create(xstfb->screen, &templ);
      }
   }
}

static boolean 
xmesa_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                              const enum st_attachment_type *statts,
                              unsigned count,
                              struct pipe_texture **out)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   unsigned i;

   /* revalidate textures */
   if (xstfb->buffer->width != xstfb->texture_width ||
       xstfb->buffer->height != xstfb->texture_height) {
      xstfb->texture_width = xstfb->buffer->width;
      xstfb->texture_height = xstfb->buffer->height;

      xmesa_st_framebuffer_validate_textures(stfbi, statts, count);
   }

   for (i = 0; i < count; i++) {
      out[i] = NULL;
      pipe_texture_reference(&out[i], xstfb->textures[statts[i]]);
   }

   return TRUE;
}

static boolean
xmesa_st_framebuffer_flush_front(struct st_framebuffer_iface *stfbi,
                                 enum st_attachment_type statt)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   boolean ret;

   ret = xmesa_st_framebuffer_display(stfbi, statt);
   if (ret)
      xmesa_check_buffer_size(xstfb->buffer);

   return ret;
}

struct st_framebuffer_iface *
xmesa_create_st_framebuffer(struct pipe_screen *screen, XMesaBuffer b)
{
   struct st_framebuffer_iface *stfbi;
   struct xmesa_st_framebuffer *xstfb;

   stfbi = CALLOC_STRUCT(st_framebuffer_iface);
   xstfb = CALLOC_STRUCT(xmesa_st_framebuffer);
   if (!stfbi || !xstfb) {
      if (stfbi)
         FREE(stfbi);
      if (xstfb)
         FREE(xstfb);
      return NULL;
   }

   xstfb->screen = screen;
   xstfb->buffer = b;
   xstfb->stvis = b->xm_visual->stvis;

   stfbi->visual = &xstfb->stvis;
   stfbi->flush_front = xmesa_st_framebuffer_flush_front;
   stfbi->validate = xmesa_st_framebuffer_validate;
   stfbi->st_manager_private = (void *) xstfb;

   return stfbi;
}

void
xmesa_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);

   FREE(xstfb);
   FREE(stfbi);
}

void
xmesa_swap_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   struct xmesa_st_framebuffer *xstfb = xmesa_st_framebuffer(stfbi);
   boolean ret;

   ret = xmesa_st_framebuffer_display(stfbi, ST_ATTACHMENT_BACK_LEFT);
   if (ret) {
      struct pipe_texture **front, **back, *tmp;

      front = &xstfb->textures[ST_ATTACHMENT_FRONT_LEFT];
      back = &xstfb->textures[ST_ATTACHMENT_BACK_LEFT];
      /* swap textures only if the front texture has been allocated */
      if (*front) {
         tmp = *front;
         *front = *back;
         *back = tmp;
      }

      xmesa_check_buffer_size(xstfb->buffer);
   }
}

void
xmesa_copy_st_framebuffer(struct st_framebuffer_iface *stfbi,
                          enum st_attachment_type src,
                          enum st_attachment_type dst,
                          int x, int y, int w, int h)
{
   /* TODO */
}

struct st_api *
xmesa_create_st_api(void)
{
   return XMESA_ST_MODULE.create_api();
}
