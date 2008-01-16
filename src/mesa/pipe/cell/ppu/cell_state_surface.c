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


#include "pipe/p_inlines.h"
#include "cell_batch.h"
#include "cell_context.h"
#include "cell_state.h"
#include "cell_spu.h"


void
cell_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
   struct cell_context *cell = cell_context(pipe);

   /* XXX revisit this memcmp! */
   if (memcmp(&cell->framebuffer, fb, sizeof(*fb))) {
      struct pipe_surface *csurf = fb->cbufs[0];
      struct pipe_surface *zsurf = fb->zbuf;
      uint i;

      /* change in fb state */

      /* unmap old surfaces */
      for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
         if (cell->framebuffer.cbufs[i] && cell->cbuf_map[i]) {
            pipe_surface_unmap(cell->framebuffer.cbufs[i]);
            cell->cbuf_map[i] = NULL;
         }
      }

      if (cell->framebuffer.zbuf && cell->zbuf_map) {
         pipe_surface_unmap(cell->framebuffer.zbuf);
         cell->zbuf_map = NULL;
      }

      /* update my state */
      cell->framebuffer = *fb;

      /* map new surfaces */
      if (csurf)
         cell->cbuf_map[0] = pipe_surface_map(csurf);

      if (zsurf)
         cell->zbuf_map = pipe_surface_map(zsurf);

#if 0
      for (i = 0; i < cell->num_spus; i++) {
         struct cell_command_framebuffer *fb = &cell_global.command[i].fb;
         fb->opcode = CELL_CMD_FRAMEBUFFER;
         fb->color_start = csurf->map;
         fb->color_format = csurf->format;
         fb->depth_start = zsurf ? zsurf->map : NULL;
         fb->depth_format = zsurf ? zsurf->format : PIPE_FORMAT_NONE;
         fb->width = csurf->width;
         fb->height = csurf->height;
         send_mbox_message(cell_global.spe_contexts[i], CELL_CMD_FRAMEBUFFER);
      }
#endif
#if 1
      {
         struct cell_command_framebuffer *fb
            = cell_batch_alloc(cell, sizeof(*fb));
         fb->opcode = CELL_CMD_FRAMEBUFFER;
         fb->color_start = cell->cbuf_map[0];
         fb->color_format = csurf->format;
         fb->depth_start = cell->zbuf_map;
         fb->depth_format = zsurf ? zsurf->format : PIPE_FORMAT_NONE;
         fb->width = csurf->width;
         fb->height = csurf->height;
         /*cell_batch_flush(cell);*/
         /*cell_flush(&cell->pipe, 0x0);*/
      }
#endif
      cell->dirty |= CELL_NEW_FRAMEBUFFER;
   }

#if 0
   struct pipe_surface *ps;
   uint i;

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      /* check if changing cbuf */
      if (sp->framebuffer.cbufs[i] != fb->cbufs[i]) {
         /* flush old */
         sp_flush_tile_cache(sp, sp->cbuf_cache[i]);
         /* unmap old */
         ps = sp->framebuffer.cbufs[i];
         if (ps && ps->map)
            pipe_surface_unmap(ps);
         /* map new */
         ps = fb->cbufs[i];
         if (ps)
            pipe_surface_map(ps);
         /* assign new */
         sp->framebuffer.cbufs[i] = fb->cbufs[i];

         /* update cache */
         sp_tile_cache_set_surface(sp, sp->cbuf_cache[i], ps);
      }
   }

   sp->framebuffer.num_cbufs = fb->num_cbufs;

   /* zbuf changing? */
   if (sp->framebuffer.zbuf != fb->zbuf) {
      /* flush old */
      sp_flush_tile_cache(sp, sp->zbuf_cache);
      /* unmap old */
      ps = sp->framebuffer.zbuf;
      if (ps && ps->map)
         pipe_surface_unmap(ps);
      if (sp->framebuffer.sbuf == sp->framebuffer.zbuf) {
         /* combined z/stencil */
         sp->framebuffer.sbuf = NULL;
      }
      /* map new */
      ps = fb->zbuf;
      if (ps)
         pipe_surface_map(ps);
      /* assign new */
      sp->framebuffer.zbuf = fb->zbuf;

      /* update cache */
      sp_tile_cache_set_surface(sp, sp->zbuf_cache, ps);
   }

   /* XXX combined depth/stencil here */

   /* sbuf changing? */
   if (sp->framebuffer.sbuf != fb->sbuf) {
      /* flush old */
      sp_flush_tile_cache(sp, sp->sbuf_cache_sep);
      /* unmap old */
      ps = sp->framebuffer.sbuf;
      if (ps && ps->map)
         pipe_surface_unmap(ps);
      /* map new */
      ps = fb->sbuf;
      if (ps && fb->sbuf != fb->zbuf)
         pipe_surface_map(ps);
      /* assign new */
      sp->framebuffer.sbuf = fb->sbuf;

      /* update cache */
      if (fb->sbuf != fb->zbuf) {
         /* separate stencil buf */
         sp->sbuf_cache = sp->sbuf_cache_sep;
         sp_tile_cache_set_surface(sp, sp->sbuf_cache, ps);
      }
      else {
         /* combined depth/stencil */
         sp->sbuf_cache = sp->zbuf_cache;
         sp_tile_cache_set_surface(sp, sp->sbuf_cache, ps);
      }
   }

   sp->dirty |= SP_NEW_FRAMEBUFFER;
#endif
}

