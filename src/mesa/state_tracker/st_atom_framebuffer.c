/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  *   Brian Paul
  */
 
#include <limits.h>

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bitmap.h"
#include "st_cb_fbo.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "cso_cache/cso_context.h"
#include "util/u_math.h"
#include "util/u_inlines.h"
#include "util/u_format.h"


/**
 * Update framebuffer size.
 *
 * We need to derive pipe_framebuffer size from the bound pipe_surfaces here
 * instead of copying gl_framebuffer size because for certain target types
 * (like PIPE_TEXTURE_1D_ARRAY) gl_framebuffer::Height has the number of layers
 * instead of 1.
 */
static void
update_framebuffer_size(struct pipe_framebuffer_state *framebuffer,
                        struct pipe_surface *surface)
{
   assert(surface);
   assert(surface->width  < UINT_MAX);
   assert(surface->height < UINT_MAX);
   framebuffer->width  = MIN2(framebuffer->width,  surface->width);
   framebuffer->height = MIN2(framebuffer->height, surface->height);
}


/**
 * Update framebuffer state (color, depth, stencil, etc. buffers)
 */
static void
update_framebuffer_state( struct st_context *st )
{
   struct pipe_framebuffer_state *framebuffer = &st->state.framebuffer;
   struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   struct st_renderbuffer *strb;
   GLuint i;

   st_flush_bitmap_cache(st);

   st->state.fb_orientation = st_fb_orientation(fb);
   framebuffer->width  = UINT_MAX;
   framebuffer->height = UINT_MAX;

   /*printf("------ fb size %d x %d\n", fb->Width, fb->Height);*/

   /* Examine Mesa's ctx->DrawBuffer->_ColorDrawBuffers state
    * to determine which surfaces to draw to
    */
   framebuffer->nr_cbufs = fb->_NumColorDrawBuffers;

   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
      pipe_surface_reference(&framebuffer->cbufs[i], NULL);

      strb = st_renderbuffer(fb->_ColorDrawBuffers[i]);

      if (strb) {
         if (strb->is_rtt || (strb->texture &&
             _mesa_get_format_color_encoding(strb->Base.Format) == GL_SRGB)) {
            /* rendering to a GL texture, may have to update surface */
            st_update_renderbuffer_surface(st, strb);
         }

         if (strb->surface) {
            pipe_surface_reference(&framebuffer->cbufs[i], strb->surface);
            update_framebuffer_size(framebuffer, strb->surface);
         }
         strb->defined = GL_TRUE; /* we'll be drawing something */
      }
   }

   for (i = framebuffer->nr_cbufs; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&framebuffer->cbufs[i], NULL);
   }

   /* Remove trailing GL_NONE draw buffers. */
   while (framebuffer->nr_cbufs &&
          !framebuffer->cbufs[framebuffer->nr_cbufs-1]) {
      framebuffer->nr_cbufs--;
   }

   /*
    * Depth/Stencil renderbuffer/surface.
    */
   strb = st_renderbuffer(fb->Attachment[BUFFER_DEPTH].Renderbuffer);
   if (strb) {
      if (strb->is_rtt) {
         /* rendering to a GL texture, may have to update surface */
         st_update_renderbuffer_surface(st, strb);
      }
      pipe_surface_reference(&framebuffer->zsbuf, strb->surface);
      update_framebuffer_size(framebuffer, strb->surface);
   }
   else {
      strb = st_renderbuffer(fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      if (strb) {
         assert(strb->surface);
         pipe_surface_reference(&framebuffer->zsbuf, strb->surface);
         update_framebuffer_size(framebuffer, strb->surface);
      }
      else
         pipe_surface_reference(&framebuffer->zsbuf, NULL);
   }

#ifdef DEBUG
   /* Make sure the resource binding flags were set properly */
   for (i = 0; i < framebuffer->nr_cbufs; i++) {
      assert(!framebuffer->cbufs[i] ||
             framebuffer->cbufs[i]->texture->bind & PIPE_BIND_RENDER_TARGET);
   }
   if (framebuffer->zsbuf) {
      assert(framebuffer->zsbuf->texture->bind & PIPE_BIND_DEPTH_STENCIL);
   }
#endif

   if (framebuffer->width == UINT_MAX)
      framebuffer->width = 0;
   if (framebuffer->height == UINT_MAX)
      framebuffer->height = 0;

   cso_set_framebuffer(st->cso_context, framebuffer);
}


const struct st_tracked_state st_update_framebuffer = {
   "st_update_framebuffer",				/* name */
   {							/* dirty */
      _NEW_BUFFERS,					/* mesa */
      ST_NEW_FRAMEBUFFER,				/* st */
   },
   update_framebuffer_state				/* update */
};

