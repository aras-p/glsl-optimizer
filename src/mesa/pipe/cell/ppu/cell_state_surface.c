

#include "cell_context.h"
#include "cell_state.h"

void
cell_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
   struct cell_context *cell = cell_context(pipe);

   cell->framebuffer = *fb;

   cell->dirty |= CELL_NEW_FRAMEBUFFER;

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

