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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */
#include "sp_context.h"
#include "sp_state.h"
#include "sp_surface.h"
#include "sp_tile_cache.h"


/**
 * XXX this might get moved someday
 * Set the framebuffer surface info: color buffers, zbuffer, stencil buffer.
 * Here, we map the surfaces and update the tile cache to point to the new
 * surfaces.
 */
void
softpipe_set_framebuffer_state(struct pipe_context *pipe,
                               const struct pipe_framebuffer_state *fb)
{
   struct softpipe_context *sp = softpipe_context(pipe);
   struct softpipe_surface *sps;
   uint i;

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      /* check if changing cbuf */
      if (sp->framebuffer.cbufs[i] != fb->cbufs[i]) {
         /* flush old */
         sp_flush_tile_cache(sp->cbuf_cache[i]);
         /* unmap old */
         sps = softpipe_surface(sp->framebuffer.cbufs[i]);
         if (sps && sps->surface.region)
            pipe->region_unmap(pipe, sps->surface.region);
         /* map new */
         sps = softpipe_surface(fb->cbufs[i]);
         if (sps)
            pipe->region_map(pipe, sps->surface.region);
         /* assign new */
         sp->framebuffer.cbufs[i] = fb->cbufs[i];

         /* update cache */
         sp_tile_cache_set_surface(sp->cbuf_cache[i], sps);
      }
   }

   sp->framebuffer.num_cbufs = fb->num_cbufs;

   /* zbuf changing? */
   if (sp->framebuffer.zbuf != fb->zbuf) {
      /* flush old */
      sp_flush_tile_cache(sp->zbuf_cache);
      /* unmap old */
      sps = softpipe_surface(sp->framebuffer.zbuf);
      if (sps && sps->surface.region)
         pipe->region_unmap(pipe, sps->surface.region);
      if (sp->framebuffer.sbuf == sp->framebuffer.zbuf) {
         /* combined z/stencil */
         sp->framebuffer.sbuf = NULL;
      }
      /* map new */
      sps = softpipe_surface(fb->zbuf);
      if (sps)
         pipe->region_map(pipe, sps->surface.region);
      /* assign new */
      sp->framebuffer.zbuf = fb->zbuf;

      /* update cache */
      sp_tile_cache_set_surface(sp->zbuf_cache, sps);
   }

   /* XXX combined depth/stencil here */

   /* sbuf changing? */
   if (sp->framebuffer.sbuf != fb->sbuf) {
      /* flush old */
      sp_flush_tile_cache(sp->sbuf_cache_sep);
      /* unmap old */
      sps = softpipe_surface(sp->framebuffer.sbuf);
      if (sps && sps->surface.region)
         pipe->region_unmap(pipe, sps->surface.region);
      /* map new */
      sps = softpipe_surface(fb->sbuf);
      if (sps && fb->sbuf != fb->zbuf)
         pipe->region_map(pipe, sps->surface.region);
      /* assign new */
      sp->framebuffer.sbuf = fb->sbuf;

      /* update cache */
      if (fb->sbuf != fb->zbuf) {
         /* separate stencil buf */
         sp->sbuf_cache = sp->sbuf_cache_sep;
         sp_tile_cache_set_surface(sp->sbuf_cache, sps);
      }
      else {
         /* combined depth/stencil */
         sp->sbuf_cache = sp->zbuf_cache;
         sp_tile_cache_set_surface(sp->sbuf_cache, sps);
      }
   }

   sp->dirty |= SP_NEW_FRAMEBUFFER;
}




void
softpipe_set_clear_color_state(struct pipe_context *pipe,
                               const struct pipe_clear_color_state *clear)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->clear_color = *clear; /* struct copy */
}
