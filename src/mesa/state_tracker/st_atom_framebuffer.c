/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */
 
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_fbo.h"
#include "st_public.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "cso_cache/cso_context.h"
#include "util/u_rect.h"
#include "util/u_math.h"
#include "util/u_inlines.h"



/**
 * When doing GL render to texture, we have to be sure that finalize_texture()
 * didn't yank out the pipe_texture that we earlier created a surface for.
 * Check for that here and create a new surface if needed.
 */
static void
update_renderbuffer_surface(struct st_context *st,
                            struct st_renderbuffer *strb)
{
   struct pipe_screen *screen = st->pipe->screen;
   struct pipe_texture *texture = strb->rtt->pt;
   int rtt_width = strb->Base.Width;
   int rtt_height = strb->Base.Height;

   if (!strb->surface ||
       strb->surface->texture != texture ||
       strb->surface->width != rtt_width ||
       strb->surface->height != rtt_height) {
      GLuint level;
      /* find matching mipmap level size */
      for (level = 0; level <= texture->last_level; level++) {
         if (u_minify(texture->width0, level) == rtt_width &&
             u_minify(texture->height0, level) == rtt_height) {

            pipe_surface_reference(&strb->surface, NULL);

            strb->surface = screen->get_tex_surface(screen,
                                              texture,
                                              strb->rtt_face,
                                              level,
                                              strb->rtt_slice,
                                              PIPE_BUFFER_USAGE_GPU_READ |
                                              PIPE_BUFFER_USAGE_GPU_WRITE);
#if 0
            printf("-- alloc new surface %d x %d into tex %p\n",
                   strb->surface->width, strb->surface->height,
                   texture);
#endif
            break;
         }
      }
   }
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

   framebuffer->width = fb->Width;
   framebuffer->height = fb->Height;

   /*printf("------ fb size %d x %d\n", fb->Width, fb->Height);*/

   /* Examine Mesa's ctx->DrawBuffer->_ColorDrawBuffers state
    * to determine which surfaces to draw to
    */
   framebuffer->nr_cbufs = 0;
   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
      strb = st_renderbuffer(fb->_ColorDrawBuffers[i]);

      if (strb) {
         /*printf("--------- framebuffer surface rtt %p\n", strb->rtt);*/
         if (strb->rtt) {
            /* rendering to a GL texture, may have to update surface */
            update_renderbuffer_surface(st, strb);
         }

         if (strb->surface) {
            pipe_surface_reference(&framebuffer->cbufs[framebuffer->nr_cbufs],
                                   strb->surface);
            framebuffer->nr_cbufs++;
         }
         strb->defined = GL_TRUE; /* we'll be drawing something */
      }
   }
   for (i = framebuffer->nr_cbufs; i < PIPE_MAX_COLOR_BUFS; i++) {
      pipe_surface_reference(&framebuffer->cbufs[i], NULL);
   }

   /*
    * Depth/Stencil renderbuffer/surface.
    */
   strb = st_renderbuffer(fb->Attachment[BUFFER_DEPTH].Renderbuffer);
   if (strb) {
      strb = st_renderbuffer(strb->Base.Wrapped);
      if (strb->rtt) {
         /* rendering to a GL texture, may have to update surface */
         update_renderbuffer_surface(st, strb);
      }
      pipe_surface_reference(&framebuffer->zsbuf, strb->surface);
   }
   else {
      strb = st_renderbuffer(fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      if (strb) {
         strb = st_renderbuffer(strb->Base.Wrapped);
         assert(strb->surface);
         pipe_surface_reference(&framebuffer->zsbuf, strb->surface);
      }
      else
         pipe_surface_reference(&framebuffer->zsbuf, NULL);
   }

   cso_set_framebuffer(st->cso_context, framebuffer);

   if (fb->_ColorDrawBufferIndexes[0] == BUFFER_FRONT_LEFT) {
      if (st->frontbuffer_status == FRONT_STATUS_COPY_OF_BACK) {
         /* copy back color buffer to front color buffer */
         struct st_framebuffer *stfb = (struct st_framebuffer *) fb;
	 struct pipe_surface *surf_front, *surf_back;
         (void) st_get_framebuffer_surface(stfb, ST_SURFACE_FRONT_LEFT, &surf_front);
         (void) st_get_framebuffer_surface(stfb, ST_SURFACE_BACK_LEFT, &surf_back);

         if (st->pipe->surface_copy) {
            st->pipe->surface_copy(st->pipe,
                                   surf_front, 0, 0,  /* dest */
                                   surf_back, 0, 0,   /* src */
                                   fb->Width, fb->Height);
         } else {
            util_surface_copy(st->pipe, FALSE,
                              surf_front, 0, 0,
                              surf_back, 0, 0,
                              fb->Width, fb->Height);
         }
      }
      /* we're assuming we'll really draw to the front buffer */
      st->frontbuffer_status = FRONT_STATUS_DIRTY;
   }
}


const struct st_tracked_state st_update_framebuffer = {
   "st_update_framebuffer",				/* name */
   {							/* dirty */
      _NEW_BUFFERS,					/* mesa */
      ST_NEW_FRAMEBUFFER,				/* st */
   },
   update_framebuffer_state				/* update */
};

